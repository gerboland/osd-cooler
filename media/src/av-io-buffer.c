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
 * Neuros-Cooler av io buffer interface.
 *
 * REVISION:
 * 
 * 2) Add optional double buffers handled by write thread. -2007-11-22 nerochiaro
 * 1) Initial creation. ----------------------------------- 2007-04-19 JW  
 *
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <inttypes.h>

#include "av-io-buffer.h"

#ifdef USE_NMS_BUFFER

#include "nc-err.h"
#include "cooler-core.h"

#define PLAY_BUFFER_COUNT 12
#define RECORD_BUFFER_COUNT 22
#define BUFFER_COUNT 22
#define BUFFER_SIZE (1024*256)
#define BUFFER_WAIT_TIME (500*1000)  //500MS
#define WAIT_BUFFER_FOR_PLAY  200000//200ms
#define RETRY_LIMIT 20
#define MAX_BACK_COUNT 3
#define MAX_READ_PER_TIME 11
#define HIDDENFEATURE_SHM_ID "hiddenfeature"
#define TEST_TEXT_SIZE 80
#define TEST_FILE "/mnt/tmpfs/test_buffer"

static size_t write_to_buffer,write_to_file,read_from_file,read_from_buffer;
static int max_used_count;
static shm_handle_t hiddenfeature_h;
static shm_handle_t nms_used_buf_h;
static int hiddenfeature = 0;
typedef struct io_buffer_s
{
	size_t index;
	char *buffer;
	int ifseek;
	off_t seek_pos;
	int seek_type;
}io_buffer_t;
static pthread_t          BufferThread = (pthread_t)NULL;
static pthread_mutex_t    bufferMutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef USE_NMS_PLAY_BUFFER
static pthread_mutex_t    readMutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static pthread_cond_t     bufferCond  = PTHREAD_COND_INITIALIZER;
static io_buffer_t io_buffer[BUFFER_COUNT];
static int buf_fd=0,error=0,buf_count=0,quit=0,going=0;
static int max_buf_count;
static io_buffer_t *read_buf=NULL;
static io_buffer_t *write_buf=NULL;
static int rw;  //0:read 1:write
#ifdef USE_NMS_PLAY_BUFFER
int back_count = 0;
BOOL file_eof = FALSE;
off_t file_range_start=0, file_range_end=0, file_cur_pointer=0;
#endif

static slist_t *double_buffers = NULL;
static pthread_mutex_t double_buffers_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef USE_NMS_RECORD_BUFFER
/**
 * write to buffer.
 *
 * @param fd
 *        the same with write
 * @param buffer
 *        the same with write
 * @param size
 *        the same with write
 * @return
 *        0: success
 *        -1:error.
 */
int NmsWriteToBuffer(int fd, void *buffer, size_t size)
{
	int buf_count2;
	int remain;
	char *buffer_index;
	int fp;
	char test_text[TEST_TEXT_SIZE];
	unsigned int retry_count=0;
	remain=size;
	buffer_index=buffer;
	if(error)
		return error;
	if(buf_fd!=fd)
	{
		buf_fd=fd;
	}
retry:

	if((++retry_count)>RETRY_LIMIT)
	{
		if (hiddenfeature)
		{
			fp=open(TEST_FILE, O_RDWR | O_CREAT | O_APPEND, 0x777);
			if (fp!=-1)
			{
				pthread_mutex_lock(&bufferMutex);
				buf_count2=buf_count;
				pthread_mutex_unlock(&bufferMutex);
				snprintf(test_text,TEST_TEXT_SIZE,"buffer full! buffer count=%d\n",buf_count2);
				write(fp,test_text,strlen(test_text));
				close(fp);
			}
		}
		
		WARNLOG("AV buffer overflowed, giving up and dropping data.\n");
		return -1;
	}
	while (1)
	{
		pthread_mutex_lock(&bufferMutex);
		buf_count2=buf_count;
		pthread_mutex_unlock(&bufferMutex);
		if (!(buf_count2<max_buf_count))
		{
			WARNLOG("AV buffer overflowed, retrying (%d)...\n", retry_count);
			usleep(BUFFER_WAIT_TIME);
			goto retry;
		}
		if (remain<(BUFFER_SIZE - write_buf->index)&&remain>0)
		{
			memcpy((write_buf->buffer+write_buf->index), buffer_index, remain);
			write_buf->index+=remain;
			break;
		}
		else
		{
			if(remain==0)
				break;
			pthread_mutex_lock(&bufferMutex);
			buf_count2=buf_count;
			pthread_mutex_unlock(&bufferMutex);
			if (buf_count2>=(max_buf_count-1))
			{
				WARNLOG("AV buffer overflowed, retrying (%d)...\n", retry_count);
				usleep(BUFFER_WAIT_TIME);
				goto retry;
			}
			memcpy((write_buf->buffer+write_buf->index), buffer_index, (BUFFER_SIZE - write_buf->index));
			remain-=(BUFFER_SIZE - write_buf->index);
			buffer_index+=(BUFFER_SIZE - write_buf->index);
			write_buf->index=BUFFER_SIZE;

			if (write_buf==io_buffer+max_buf_count-1)
				write_buf=io_buffer;
			else
				write_buf++;
			pthread_mutex_lock(&bufferMutex);
			buf_count++;
			going=1;
			pthread_cond_broadcast(&bufferCond);
			pthread_mutex_unlock(&bufferMutex);
		}
	}
	if (hiddenfeature)
		write_to_buffer+=size ;
	return 0;
}

/**
 * set seek to buffer when record.
 *
 * @param fd
 *        the same as lseek
 * @param pos
 *        the same as lseek
 * @param type
 *        the same as lseek
 * @return
 *        0: success
 *        -1:error.
 */
int NmsBufferRecordSeek(int fd, off_t pos, int type)
{
	int buf_count2;
	int retry_count=0;

	if(fd!=buf_fd)
		buf_fd=fd;
retry:
	if ((++retry_count)>RETRY_LIMIT)
		return -1;
	pthread_mutex_lock(&bufferMutex);
	buf_count2=buf_count;
	pthread_mutex_unlock(&bufferMutex);
	if (buf_count2<max_buf_count)
	{
		if (write_buf->index)
		{
			pthread_mutex_lock(&bufferMutex);
			buf_count2=buf_count;
			pthread_mutex_unlock(&bufferMutex);
			if (buf_count2>=(max_buf_count-1))
			{
				WARNLOG("AV buffer overflowed, retrying (%u)...\n", retry_count);
				usleep(BUFFER_WAIT_TIME);
				goto retry;
			}
			if (write_buf==io_buffer+max_buf_count-1)
				write_buf=io_buffer;
			else
				write_buf++;
			pthread_mutex_lock(&bufferMutex);
			buf_count++;
			going=1;
			pthread_cond_broadcast(&bufferCond);
			pthread_mutex_unlock(&bufferMutex);
		}
		write_buf->ifseek=1;
		write_buf->seek_pos=pos;
		write_buf->seek_type=type;
	}
	else
	{
		WARNLOG("AV buffer overflowed, retrying...\n");
		usleep(BUFFER_WAIT_TIME);
		goto retry;
	}
    return 0;
}

static void * buffer_write_loop( void * arg )
{
	int buf_count2;
	int fp;
	char test_text[TEST_TEXT_SIZE];

	while(1)
	{
		pthread_mutex_lock(&bufferMutex);
		going=0;
		while((!quit)&&(!going))
			pthread_cond_wait(&bufferCond, &bufferMutex);
			pthread_mutex_unlock(&bufferMutex);
		if(buf_fd)
		{
            while(1)
			{
				pthread_mutex_lock(&bufferMutex);
				buf_count2=buf_count;
				pthread_mutex_unlock(&bufferMutex);
				if(!buf_count2)
					break;
				if(read_buf->ifseek)
				{
					if (lseek(buf_fd, read_buf->seek_pos, read_buf->seek_type) == (off_t)-1)
						WARNLOG("Failed seeking in buffer1 fd %d, pos %" PRId64 ", error: %d (%s)\n", buf_fd, read_buf->seek_pos, errno, strerror(errno));
					read_buf->ifseek=0;
				}
				if(read_buf->index!=write(buf_fd, read_buf->buffer,read_buf->index ))
				{
					WARNLOG("Failed writing buffer1 to fd %d, size %d, error: %d (%s)\n", buf_fd, read_buf->index, errno, strerror(errno));

					error=-1;
					if (hiddenfeature)
					{
						fp=open(TEST_FILE, O_RDWR | O_CREAT | O_APPEND, 0x777);
						if (fp!=-1)
						{
							snprintf(test_text,TEST_TEXT_SIZE,"write to file ERRORl!\n");
							write(fp,test_text,strlen(test_text));
							close(fp);
						}
					}
					break;
				}
				if (hiddenfeature)
				{
					write_to_file+=read_buf->index ;
					if (buf_count2>max_used_count)
						max_used_count=buf_count2;	
					CoolShmHelperWrite(&nms_used_buf_h,&max_used_count, 0, sizeof(int));
				}
				read_buf->index=0;
				pthread_mutex_lock(&bufferMutex);
				buf_count--;
				pthread_mutex_unlock(&bufferMutex);
				if(read_buf==io_buffer+max_buf_count-1)
					read_buf=io_buffer;
				else
					read_buf++;
				pthread_mutex_lock(&bufferMutex);
				buf_count2=buf_count;
				pthread_mutex_unlock(&bufferMutex);
			}
		}
		else
			error=-1;

		/* drain all data from any double buffer that needs it */
		pthread_mutex_lock(&double_buffers_lock);
		slist_t *cur = double_buffers;
		double_buffer_t *dbuf;
		int status;
		while (cur != NULL) 
		{
			if ((dbuf = cur->data) == NULL) continue;

			status = NmsDoubleBufferRead(dbuf);
			if (status != 0) WARNLOG("Failed to drain the double buffer for fd %d. Error %d\n", dbuf->fd, status);

			cur = cur->next;
		}
		pthread_mutex_unlock(&double_buffers_lock);

		if(quit)
			break;
	}

	if(!buf_count)
	{
		if(read_buf->index)
		{
			if(read_buf->ifseek)
			{
				if (lseek(buf_fd, read_buf->seek_pos, read_buf->seek_type) == (off_t)-1)
					WARNLOG("Failed seeking in buffer2 fd %d, pos %" PRId64 ", error: %d (%s)\n", buf_fd, read_buf->seek_pos, errno, strerror(errno));
				read_buf->ifseek=0;
			}
			if(read_buf->index!=write(buf_fd, read_buf->buffer,read_buf->index ))
			{
				WARNLOG("Failed writing buffer2 to fd %d, size %d, error: %d (%s)\n", buf_fd, read_buf->index, errno, strerror(errno));
				error=-1;
			}
			if (hiddenfeature)
				write_to_file+=read_buf->index ;
			read_buf->index=0;
		}
	}
	else
		error =-1;

	pthread_exit(NULL);
}
#endif

#ifdef USE_NMS_PLAY_BUFFER

/**
 * read from buffer.
 *
 * @param fd
 *        the same with read
 * @param buffer
 *        the same with read
 * @param size
 *        the same with read
 * @return
 *        the same with read
 *        -1:error.
 */
ssize_t NmsReadFromBuffer(int fd, void *buffer, size_t size)
{
	off_t file_range_end2, file_cur_pointer2, file_pointer;
	BOOL file_eof2;
	ssize_t ret, read_ret, read_size;
	if(fd!=buf_fd)
		buf_fd=fd;
	ret=size;
	read_size=size;
	pthread_mutex_lock(&bufferMutex);
	file_range_end2=file_range_end;
	pthread_mutex_unlock(&bufferMutex);
	if (file_cur_pointer+size<=file_range_end2)
	{
		if (size<BUFFER_SIZE-read_buf->index-1)
		{
			memcpy(buffer, read_buf->buffer+read_buf->index, size);
			if (hiddenfeature)
				read_from_buffer+=size;
			read_buf->index+=size;
		}
		else
		{
			memcpy(buffer, read_buf->buffer+read_buf->index, BUFFER_SIZE-read_buf->index);
			if (hiddenfeature)
				read_from_buffer+=BUFFER_SIZE-read_buf->index;
			size-=BUFFER_SIZE-read_buf->index;
			buffer+=BUFFER_SIZE-read_buf->index;
			if (read_buf==io_buffer+max_buf_count-1)
				read_buf=io_buffer;
			else
				read_buf++;
			back_count++;
			while (size>BUFFER_SIZE)
			{
				memcpy(buffer,read_buf->buffer,BUFFER_SIZE);
				if (hiddenfeature)
					read_from_buffer+=BUFFER_SIZE;
				if (read_buf==io_buffer+max_buf_count-1)
					read_buf=io_buffer;
				else
					read_buf++;
				size-=BUFFER_SIZE;
				buffer+=BUFFER_SIZE;
				back_count++;
			}
			if (size>0)
			{
				memcpy(buffer,read_buf->buffer,size);
				if (hiddenfeature)
					read_from_buffer+=size;
			}
			read_buf->index=size;
			if (back_count>MAX_BACK_COUNT)
			{
				pthread_mutex_lock(&bufferMutex);
				if (!file_eof)
				{
					buf_count-=back_count-MAX_BACK_COUNT;
					file_range_start+=(back_count-MAX_BACK_COUNT)*BUFFER_SIZE;
					back_count=MAX_BACK_COUNT;
					going=1;
					pthread_cond_broadcast(&bufferCond);
				}
				pthread_mutex_unlock(&bufferMutex);
			}
		}
	}
	else  //out of buffer range
	{
		pthread_mutex_lock(&bufferMutex);
		file_eof2=file_eof;
		pthread_mutex_unlock(&bufferMutex);
		if(file_eof2)
		{
			return 0;
		}
		file_cur_pointer2=file_cur_pointer;
		
		read_size=0;
		while (1)
		{
			pthread_mutex_lock(&bufferMutex);
			file_range_end2=file_range_end;
			pthread_mutex_unlock(&bufferMutex);
			if (file_cur_pointer2>=file_range_end2)
			{
				break;
			}
			memcpy(buffer, read_buf->buffer+read_buf->index, BUFFER_SIZE-read_buf->index);
			if (hiddenfeature)
				read_from_buffer+=BUFFER_SIZE-read_buf->index;
			size-=BUFFER_SIZE-read_buf->index;
			buffer+=BUFFER_SIZE-read_buf->index;
			read_size+=BUFFER_SIZE-read_buf->index;
			file_cur_pointer2+=BUFFER_SIZE-read_buf->index;
			if (read_buf==io_buffer+max_buf_count-1)
				read_buf=io_buffer;
			else
				read_buf++;
			read_buf->index=0;
		}
		if (size>0)
		{
			pthread_mutex_lock(&readMutex);
			file_pointer=lseek(buf_fd, 0, SEEK_CUR); // preserve the file pointer
			lseek(buf_fd, file_cur_pointer2, SEEK_SET);
            read_ret=read(buf_fd, buffer, size);
			lseek(buf_fd, file_pointer, SEEK_SET);    // revert file pointer
			pthread_mutex_unlock(&readMutex);
			if (read_ret!=size)
			{
				ret=0;
			}
			else
			{
				read_size+=size;
				file_cur_pointer2+=size;
				if (hiddenfeature)
					read_from_buffer+=size;
			}
		}
		pthread_mutex_lock(&bufferMutex);
		if (ret!=0)
		{
			if (buf_count==max_buf_count)
			{
				file_range_start=file_cur_pointer2;
				file_range_end=file_range_start;
				read_buf=io_buffer;
				read_buf->index=0;
				write_buf=io_buffer;
				write_buf->ifseek=1;
				write_buf->seek_pos=file_range_start;
				write_buf->seek_type=SEEK_SET;
				buf_count=0;
				back_count=0;
			}
			else
			{
				read_buf->index+=size;
			}
			going=1;
			pthread_cond_broadcast(&bufferCond);
		}
		else
		{
			file_eof = TRUE;
		}
		pthread_mutex_unlock(&bufferMutex);
	}
	if (hiddenfeature)
		read_from_file+=read_size;
	file_cur_pointer+=read_size;
	return ret;
}

static int seek_from_current(off_t pos)
{
	off_t file_range_start2;
	off_t file_range_end2;

	if(pos==0)
		return 0;
	pthread_mutex_lock(&bufferMutex);
	file_range_start2 = file_range_start;
	file_range_end2 = file_range_end;
	pthread_mutex_unlock(&bufferMutex);
	if (pos>0&&pos<=(file_range_end2-file_cur_pointer))
	{
		if (pos<=(BUFFER_SIZE - read_buf->index-1))
		{
			read_buf->index+=pos;
		} 
		else
		{
			pos-=(BUFFER_SIZE - read_buf->index-1);
			if (read_buf==io_buffer+max_buf_count-1)
				read_buf=io_buffer;
			else
				read_buf++;
			back_count++;
			while (pos>BUFFER_SIZE)
			{
				pos-=BUFFER_SIZE;
				if (read_buf==io_buffer+max_buf_count-1)
					read_buf=io_buffer;
				else
					read_buf++;
				back_count++;
			}
			read_buf->index=pos-1;
			if (back_count>MAX_BACK_COUNT)
			{
				pthread_mutex_lock(&bufferMutex);
				if (!file_eof)
				{
					buf_count-=back_count-MAX_BACK_COUNT;
					file_range_start+=(back_count-MAX_BACK_COUNT)*BUFFER_SIZE;
					back_count=MAX_BACK_COUNT;
					going=1;
					pthread_cond_broadcast(&bufferCond);
				}
				pthread_mutex_unlock(&bufferMutex);
			}
		}
	} 
	else if (pos<0&&pos>=(file_range_start2-file_cur_pointer))
	{
		pos*=-1;
		if (pos<=read_buf->index)
		{
			read_buf->index-=pos;
		} 
		else
		{
			pos-=read_buf->index;
			if (read_buf==io_buffer)
				read_buf=io_buffer+max_buf_count-1;
			else
				read_buf--;
			back_count--;
			while (pos>BUFFER_SIZE)
			{
				pos-=BUFFER_SIZE;
				if (read_buf==io_buffer)
					read_buf=io_buffer+max_buf_count-1;
				else
					read_buf--;
				back_count--;
			}
			read_buf->index=BUFFER_SIZE-pos;
		}
	} 
	else
	{
		return -1;
	}
	return 0;
}

/**
 * set seek to buffer when play.
 *
 * @param fd
 *        the same as lseek
 * @param pos
 *        the same as lseek
 * @param type
 *        the same as lseek
 * @return
 *        the same as lseek
 *        -1:error.
 */
off_t NmsBufferPlaySeek(int fd, off_t pos, int type)
{
	off_t file_range_start2;
	off_t file_range_end2;
	off_t tmp_pos;
	int i, going2;

	if(fd!=buf_fd)
		buf_fd=fd;
	tmp_pos=pos;
	switch (type)
	{
	case SEEK_SET:
		pthread_mutex_lock(&bufferMutex);
		file_range_start2=file_range_start;
		file_range_end2=file_range_end;
		pthread_mutex_unlock(&bufferMutex);
		if(pos>=file_range_start2&&pos<=file_range_end2)
		{
			pos-=file_cur_pointer;
			if(seek_from_current(pos))
				goto out_of_range;
			file_cur_pointer=tmp_pos;
		}
		else
		{
			goto out_of_range;
		}
		break;
	case SEEK_CUR:
		if(seek_from_current(pos))
			goto out_of_range;
		file_cur_pointer+=tmp_pos;
		break;
	case SEEK_END:
out_of_range:

        pthread_mutex_lock(&bufferMutex);
		if(type==SEEK_SET)
		{
			file_range_start=tmp_pos;
		}
		else if(type==SEEK_CUR)
		{
			file_range_start=file_cur_pointer+tmp_pos;
		}
		if(file_eof&&file_cur_pointer>file_range_start)
		{
			file_eof = FALSE;
		}
		file_cur_pointer=file_range_start;
		file_range_end=file_range_start;
		read_buf=io_buffer;
		read_buf->index=0;
		write_buf=io_buffer;
		write_buf->ifseek=1;
		write_buf->seek_pos=file_range_start;
		write_buf->seek_type=SEEK_SET;
		buf_count=0;
		back_count=0;
		going=1;
		pthread_cond_broadcast(&bufferCond);
		pthread_mutex_unlock(&bufferMutex);
		if (file_cur_pointer<100)
		{
			for (i=0; i<max_buf_count; i+=MAX_READ_PER_TIME)
			{
				pthread_mutex_lock(&bufferMutex);
				going=1;
				pthread_cond_broadcast(&bufferCond);
				pthread_mutex_unlock(&bufferMutex);
				while (1)
				{
					pthread_mutex_lock(&bufferMutex);
					going2=going;
					pthread_mutex_unlock(&bufferMutex);
					if (!going2)
						break;
					usleep(WAIT_BUFFER_FOR_PLAY);
				}
			}
		}
		break;
	}
	return file_cur_pointer;
}

static void * buffer_read_loop( void * arg )
{
	int buf_count2;
	int pre_buf_count;
	int read_count;
	BOOL file_eof2;
	off_t seek_pos=0;
	int seek_type=0;
	int ifseek=0;

	while(1)
	{
		pthread_mutex_lock(&bufferMutex);
		while((!quit)&&(!going))
			pthread_cond_wait(&bufferCond, &bufferMutex);
		file_eof2=file_eof;
        pthread_mutex_unlock(&bufferMutex);
		if(quit)
		{
			break;
		}
		if(file_eof2)
		{
			pthread_mutex_lock(&bufferMutex);
			going=0;
			pthread_mutex_unlock(&bufferMutex);
			continue;
		}
		if(buf_fd)
		{
			pthread_mutex_lock(&bufferMutex);
			pre_buf_count=buf_count;
			pthread_mutex_unlock(&bufferMutex);
            while(1)
			{
				pthread_mutex_lock(&bufferMutex);
				buf_count2=buf_count;
				pthread_mutex_unlock(&bufferMutex);
				if (hiddenfeature)
				{
					static int i=0;
					if (i<10)
					{
						i++;
					}
					else
					{
						if (max_used_count<(max_buf_count-buf_count2))
							max_used_count=(max_buf_count-buf_count2);
					}
				}
				if(buf_count2==max_buf_count)
				{
					break;
				}
				if(buf_count2>=pre_buf_count+MAX_READ_PER_TIME)
				{
					break;
				}
				ifseek=0;
				pthread_mutex_lock(&bufferMutex);
				if(write_buf->ifseek)
				{
					seek_pos=write_buf->seek_pos;
					seek_type=write_buf->seek_type;
					ifseek=1;
					write_buf->ifseek=0;
				}
				pthread_mutex_unlock(&bufferMutex);
				pthread_mutex_lock(&readMutex);
				if(ifseek)
				{
					read_count=lseek(buf_fd, seek_pos, seek_type);
					ifseek=0;
				}
				read_count=read(buf_fd, write_buf->buffer, BUFFER_SIZE);
				pthread_mutex_unlock(&readMutex);
				pthread_mutex_lock(&bufferMutex);
				if (write_buf->ifseek==0)
				{
					if (write_buf==io_buffer+max_buf_count-1)
						write_buf=io_buffer;
					else
						write_buf++;
					buf_count++;
					file_range_end+=read_count;
					if(read_count<BUFFER_SIZE)
					{
						file_eof = TRUE;
					}
				}
				pthread_mutex_unlock(&bufferMutex);
				if(read_count<BUFFER_SIZE)
				{
					break;
				}
			}
		}
		else
			error=-1;
		pthread_mutex_lock(&bufferMutex);
		going=0;
		pthread_mutex_unlock(&bufferMutex);
	}
	pthread_exit(NULL);
}
#endif

/**
 * start io buffer thread.
 *
 * @param type
 *        1: record
 *        0: play.
 * @param fd
 *        file description.
 * @return 0: success
 *        -1:error.
 */
int NmsBufferThreadStart(int type, int fd)
{
	int i;

	if(type==PLAY)
		max_buf_count=PLAY_BUFFER_COUNT;
	else
		max_buf_count=RECORD_BUFFER_COUNT;
	CoolShmHelperOpen(&hiddenfeature_h,HIDDENFEATURE_SHM_ID,sizeof(int));
	CoolShmHelperRead(&hiddenfeature_h,&hiddenfeature, 0,sizeof(int));
	if (hiddenfeature)
	{
		write_to_buffer=0;
		write_to_file=0;
		read_from_buffer=0;
		read_from_file=0;
		max_used_count=0;
		CoolShmHelperOpen(&nms_used_buf_h,NMS_USED_BUF_SHM_ID,sizeof(int)*2);
	}
	memset(io_buffer,0,sizeof(io_buffer_t)*max_buf_count);
	BufferThread = (pthread_t)NULL;
	quit=0;
	buf_fd=fd;
	for(i=0;i<max_buf_count;i++)
	{
		if((io_buffer[i].buffer=malloc(BUFFER_SIZE))==NULL)
		{
			goto bail;
		}
	}
	buf_count=0;
	read_buf=io_buffer;
	write_buf=io_buffer;
	error=0;
	rw=type;
	if(type==RECORD)
	{
#ifdef USE_NMS_RECORD_BUFFER
		if (pthread_create(&BufferThread, NULL, buffer_write_loop, NULL))
		{
			ERRLOG("buffer thread was not created!");
			goto bail;
		}
#endif
	}
	else
	{
#ifdef USE_NMS_PLAY_BUFFER
		int going2;
		file_range_start=0;
		file_range_end=0;
		file_cur_pointer=0;
		back_count=0;
		file_eof = FALSE;
		if (pthread_create(&BufferThread, NULL, buffer_read_loop, NULL))
		{
			ERRLOG("buffer thread was not created!");
			goto bail;
		}
		for (i=0; i<max_buf_count; i+=MAX_READ_PER_TIME)
		{
			pthread_mutex_lock(&bufferMutex);
			going=1;
			pthread_cond_broadcast(&bufferCond);
			pthread_mutex_unlock(&bufferMutex);
			while (1)
			{
				pthread_mutex_lock(&bufferMutex);
				going2=going;
				pthread_mutex_unlock(&bufferMutex);
				if (!going2)
					break;
				usleep(WAIT_BUFFER_FOR_PLAY);
			}
		}
#endif
	}
	CoolShmHelperWrite(&nms_used_buf_h,&max_buf_count, sizeof(int), sizeof(int));
	return 0;
bail:
	if (hiddenfeature)
		CoolShmHelperClose(&nms_used_buf_h);
	CoolShmHelperClose(&hiddenfeature_h);
	if (BufferThread)
	{
		DBGLOG("Removing buffer thread!");
		pthread_mutex_lock(&bufferMutex);
		quit=1;
		pthread_cond_broadcast(&bufferCond);
		pthread_mutex_unlock(&bufferMutex);
		pthread_join(BufferThread, NULL);
		BufferThread = (pthread_t)NULL;
		DBGLOG("buffer thread removed!");
	}
	for (i=0;i<max_buf_count;i++)
	{
		if (io_buffer[i].buffer)
		{
			free(io_buffer[i].buffer);
			io_buffer[i].buffer=NULL;
		}
	}
	return -1;
}

/**
 * stop io buffer thread.
 *
 * @return
 *        0: success
 *        -1:error.
 */
int NmsBufferThreadStop()
{
	int i;
	int fp;
	char test_text[TEST_TEXT_SIZE];
	if (BufferThread)
	{
		pthread_mutex_lock(&bufferMutex);
		quit=1;
		pthread_cond_broadcast(&bufferCond);
		pthread_mutex_unlock(&bufferMutex);
		DBGLOG("Removing buffer thread!");
		pthread_join(BufferThread, NULL);
		BufferThread = (pthread_t)NULL;
		DBGLOG("buffer thread removed!");
	}
	for(i=0;i<max_buf_count;i++)
	{
		if(io_buffer[i].buffer)
		{
			free(io_buffer[i].buffer);
			io_buffer[i].buffer=NULL;
		}
	}
	if (hiddenfeature)
	{
		fp=open(TEST_FILE, O_RDWR | O_CREAT | O_APPEND, 0x777);
		if (fp!=-1)
		{
			if (rw==RECORD)
			{
				if (write_to_buffer==write_to_file)
					snprintf(test_text,TEST_TEXT_SIZE,"Record successful! max buffer used=%d\n",max_used_count);
				else
					snprintf(test_text,TEST_TEXT_SIZE,"Record ERROR:write count not consistent! max buffer used=%d\n",max_used_count);
			}
			else
			{
				if (read_from_buffer==read_from_file)
					snprintf(test_text,TEST_TEXT_SIZE,"Play successful! max buffer used=%d\n",max_used_count);
				else
					snprintf(test_text,TEST_TEXT_SIZE,"Play ERROR:read count not consistent! max buffer used=%d\n",max_used_count);
			}
			write(fp,test_text,strlen(test_text));
			close(fp);
		}
		CoolShmHelperClose(&nms_used_buf_h);
	}
	CoolShmHelperClose(&hiddenfeature_h);

	// Delete the double buffers list. We will free the buffers later.
	NmsBufferDelDoubleBuffers();
	
	return error;
}

/**
Add a new double buffer to the list of those that need to be processed by the drain thread.

@param buffer The buffer to be added
*/
void NmsBufferAddDoubleBuffer(double_buffer_t *buffer)
{
	pthread_mutex_lock(&double_buffers_lock);

	slist_t *item = CoolSlistNew(buffer);

	if (double_buffers == NULL) double_buffers = item;
	else CoolSlistInsert(double_buffers, item);

	pthread_mutex_unlock(&double_buffers_lock);
}

/**
Destroy the double buffer list. This does not free or flush the single buffers.
*/
void NmsBufferDelDoubleBuffers()
{
	/* Dispose of all the double buffers in the list */
	pthread_mutex_lock(&double_buffers_lock);
	slist_t *cur = double_buffers;
	while (cur != NULL) {
		slist_t *item = cur;
		cur = cur->next;
		free(item);
	}
	double_buffers = NULL;
	pthread_mutex_unlock(&double_buffers_lock);
}

#endif
