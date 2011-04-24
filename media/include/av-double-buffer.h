#ifndef NMS_DOUBLE_BUF__H
#define NMS_DOUBLE_BUF__H
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
#define DOUBLE_BUFFER_RETRY_MAX 6
#define DOUBLE_BUFFER_RETRY_TIME (500 * 1000)
#define DOUBLE_BUFFER_SIZE (16 * 1024)

typedef struct
{
	int fd;
	pthread_mutex_t lock;

	char *write;
	char *read;

	size_t size; //this is the fill level of the write buffer. the read buffer is always full
	unsigned int read_flag;
} double_buffer_t;

int NmsDoubleBufferInit(int fd, double_buffer_t* *pbuffer);
int NmsDoubleBufferWrite(double_buffer_t *buffer, const void *data, size_t size);
int NmsDoubleBufferRead(double_buffer_t *buffer);
int NmsDoubleBufferClose(double_buffer_t *buffer);

#endif //NMS_DOUBLE_BUF__H
