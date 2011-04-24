/*
 *  Copyright(C) 2007 Neuros Technology International LLC. 
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
 *  
 *
 *  This program is distributed in the hope that, in addition to its 
 *  original purpose to support Neuros hardware, it will be useful 
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied 
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************
 *
 * An implementation of simple double-buffering
 *
 * REVISION:
 * 
 * 1) Initial creation. ----------------------------------- 2007-11-20 nerochiaro
 *
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <inttypes.h>

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "cooler-core.h"

#include "av-double-buffer.h"

/**
Create a new double buffer.

@param fd The file where to write to. Sequential only.
@param pbuffer A pointer that will be filled with the address of the buffer, or NULL in case of failure.
@return 0 for success, otherwise:
		 -1 invalid pointer to buffer
		 -2 not enough memory to allocate buffer
*/
int NmsDoubleBufferInit(int fd, double_buffer_t* *pbuffer)
{
	if (pbuffer == NULL) return -1;

	double_buffer_t *buffer = malloc(sizeof(double_buffer_t));
	if (buffer == NULL) {
		*pbuffer = NULL;
		return -2;
	}

	buffer->write = malloc(DOUBLE_BUFFER_SIZE);
	buffer->read = malloc(DOUBLE_BUFFER_SIZE);
	if (buffer->write == NULL || buffer->read == NULL) {
		*pbuffer = NULL;
		return -2;
	}

	buffer->size = 0;
	buffer->read_flag = FALSE;
	buffer->fd = fd;

	pthread_mutex_init(&buffer->lock, NULL);

	*pbuffer = buffer;
	return 0;
}

/**
Writes data to a double buffer. Return immediately after all data is out.
It may block if the amount of data overflows the write buffer, since we internally retry for some times.

If this function returns success, all of the data was committed.
If this function returns failure, none of the data was committed, so it's safe to call again with the same
parameters again, if wanted.

NOTE: you should not call this function on the same buffer from multiple threads.
NOTE: size should never be larger than DOUBLE_BUFFER_SIZE

@param buffer The double buffer to use
@param data The buffer to write out
@param size The size of the data to write out
@return 0 if all data was sucessfully sent out. Otherwise:
		 -1 invalid pointer to double buffer
		 -2 buffer was not available for writing even after some waiting. Nothing was 
*/
int NmsDoubleBufferWrite(double_buffer_t *buffer, const void *data, size_t size)
{
	if (buffer == NULL) return -1;
	
	// Lots of room in the buffer, copy memory and go ahead.
	if (size + buffer->size < DOUBLE_BUFFER_SIZE) 
	{
		memcpy(buffer->write + buffer->size, data, size);
		buffer->size += size;
		return 0;
	}
	else 
	{
		// Fill up the write buffer
		size_t towrite = DOUBLE_BUFFER_SIZE - buffer->size;
		memcpy(buffer->write + buffer->size, data, towrite);
		size -= towrite;

		// Lock the mutext and begin to test when we can do the flip.

		pthread_mutex_lock(&buffer->lock);
		// The read buffer have not been drained yet, so we can't swap them.
		// Let's wait some time and hope it gets drained, before giving up and reporting an error.
		unsigned int retry = 0;
		while (buffer->read_flag == TRUE)
		{
			if (retry == DOUBLE_BUFFER_RETRY_MAX) 
			{	
				pthread_mutex_unlock(&buffer->lock); // unlock here otherwise we deadlock the drain thread.
				WARNLOG("Double buffer for fd %d is full. Giving up, reporting error.\n", buffer->fd);
				return -2;

				// Notice that in this case buffer->size was not changed. we merely wrote some data to it, but next write
				// will happen again in the same position. This allows the callers to retry too, if they want.
			}
			retry++;
			WARNLOG("Double buffer for fd %d is full. Retying (%u) after some sleep.\n", buffer->fd, retry+1);

			pthread_mutex_unlock(&buffer->lock); // unlock here otherwise we deadlock the drain thread
			usleep(DOUBLE_BUFFER_RETRY_TIME);
			pthread_mutex_lock(&buffer->lock);   // get the lock again so we can check read_flag safely
		}

		DBGLOG("Double buffer flipping: %d\n", buffer->fd);

		// Go ahead and do the flip
		char *tmp = buffer->read;
		buffer->read = buffer->write;
		buffer->write = tmp;
		buffer->read_flag = TRUE;
		buffer->size = 0;

		// Fill the write buffer with the remaining amount of data
		if (size > 0) memcpy(buffer->write, data, size);
		buffer->size += size;
	
		// Let the drain thread rolling again
		pthread_mutex_unlock(&buffer->lock);
	}

	return 0;
}

/**
Checks if the read buffer is full and drains it to disk if so.
NOTE: this function should be called only from the drain thread.
@param buffer The buffer to work without
@return 0 for success, otherwise:
		-1 invalid buffer pointer
		-2 failure during the write. this error is not recoverable.
*/
int NmsDoubleBufferRead(double_buffer_t *buffer)
{
	if (buffer == NULL) return -1;

	int status = 0;
	pthread_mutex_lock(&buffer->lock);
	if (buffer->read_flag == TRUE)
	{
		ssize_t wrote;
		wrote = write(buffer->fd, buffer->read, DOUBLE_BUFFER_SIZE);
		DBGLOG("Double buffer drain: %d, bytes: %zu => %zd\n", buffer->fd, DOUBLE_BUFFER_SIZE, wrote);
		if (wrote != DOUBLE_BUFFER_SIZE)
		{
			WARNLOG("Failed writing to disk the double buffer for fd %d (%s)\n", buffer->fd, strerror(errno));
			status = -2;
		}

		buffer->read_flag = FALSE; //clear the flag even if something failed, so we can re-use the buffer
	}	
	pthread_mutex_unlock(&buffer->lock);

	return status;
}

/**
Tries to drain to disk the read buffer, then drains also the write buffer.
Finally it free the buffers and destroy the buffer. You should not use the buffer pointer afterwards.
NOTE: this function should be called after the drain thread is stopped, or you should protect the buffer
      from being accessed othrewise. No lock protection is done within this function.

@param buffer The buffer to close
@return 0 for success, otherwise:
		-1 invalid buffer pointer
		-2 failure during the flush of the read buffer. this error is not recoverable.
		-3 failure during the flush of the write buffer. this error is not recoverable.
*/
int NmsDoubleBufferClose(double_buffer_t *buffer)
{
	ssize_t wrote;
	int status = 0;

	if (buffer == NULL) return -1;

	pthread_mutex_destroy(&buffer->lock);

	if (buffer->read_flag == TRUE)
	{
		DBGLOG("Write buffer %d had read flag.\n", buffer->fd);
		wrote = write(buffer->fd, buffer->read, DOUBLE_BUFFER_SIZE);
		if (wrote != DOUBLE_BUFFER_SIZE) {
			WARNLOG("Failed writing to disk the double buffer(r) for fd %d (%s) (during close)\n", buffer->fd, strerror(errno));
			status = -2;
		}
	}
	else DBGLOG("Write buffer %d didn't have read flag.\n", buffer->fd);

	free(buffer->read);

	if (buffer->size > 0) 
	{
		wrote = write(buffer->fd, buffer->write, buffer->size);
		DBGLOG("Write buffer %d has size %zu => %zd.\n", buffer->fd, buffer->size, wrote);
		if (wrote != buffer->size) {
			WARNLOG("Failed writing to disk the double buffer(w) for fd %d (%s) (during close)\n", buffer->fd, strerror(errno));
			status = -3;
		}
	}
	else DBGLOG("Write buffer %d has size zero.\n", buffer->fd);

	free(buffer->write);

	free(buffer);
	return status;
}
