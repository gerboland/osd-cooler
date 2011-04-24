/*
 *  Copyright(C) 2006 Neuros Technology International LLC. 
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
 * Download file from web server just like read a file from local file.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2007-11-5 GL 
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include "fopen.h"

#define BUFFER_SIZE 1024
#define FILE_LENGTH 50

/* we use a global one for convenience */
CURLM *multi_handle;
/* curl calls this routine to get more data */
static size_t
write_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp)
{
    char *newbuff;
    int rembuff;

    URL_FILE *url = (URL_FILE *)userp;
    size *= nitems;

    rembuff=url->buffer_len - url->buffer_pos;//remaining space in buffer

    if(size > rembuff)
    {
        //not enuf space in buffer
        newbuff=realloc(url->buffer,url->buffer_len + (size - rembuff));
        if(newbuff==NULL)
        {
            fprintf(stderr,"callback buffer grow failed\n");
            size=rembuff;
        }
        else
        {
            /* realloc suceeded increase buffer size*/
            url->buffer_len+=size - rembuff;
            url->buffer=newbuff;

            /*printf("Callback buffer grown to %d bytes\n",url->buffer_len);*/
        }
    }

    memcpy(&url->buffer[url->buffer_pos], buffer, size);
    url->buffer_pos += size;

    /*fprintf(stderr, "callback %d size bytes\n", size);*/

    return size;
}

static size_t
write_head_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp)
{
	strncat(userp,buffer,size*nitems);
	return size*nitems;

}

/* use to attempt to fill the read buffer up to requested number of bytes */
static int
fill_buffer(URL_FILE *file,int want,int waittime)
{
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd;
    struct timeval timeout;
    int rc;

    /* only attempt to fill buffer if transactions still running and buffer
     * doesnt exceed required size already
     */
    if((!file->still_running) || (file->buffer_pos > want))
        return 0;

    /* attempt to fill buffer */
    do
    {
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        /* set a suitable timeout to fail on */
        timeout.tv_sec = 60; /* 1 minute */
        timeout.tv_usec = 0;
        /* get file descriptors from the transfers */
        curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

        /* In a real-world program you OF COURSE check the return code of the
           function calls, *and* you make sure that maxfd is bigger than -1
           so that the call to select() below makes sense! */

        rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);

        switch(rc) {
        case -1:
            /* select error */
            break;

        case 0:
            break;

        default:
            /*timeout or readable/writable sockets */
            /* note we *could* be more efficient and not wait for
             * CURLM_CALL_MULTI_PERFORM to clear here and check it on re-entry
             * but that gets messy */
            while(curl_multi_perform(multi_handle, &file->still_running) ==
                  CURLM_CALL_MULTI_PERFORM);

            break;
        }
    } while(file->still_running && (file->buffer_pos < want));
    return 1;
}

// use to remove want bytes from the front of a files buffer 
static int
use_buffer(URL_FILE *file,int want)
{
    // sort out buffer 
    if((file->buffer_pos - want) <=0)
    {
        // ditch buffer - write will recreate 
        if(file->buffer)
            free(file->buffer);

        file->buffer=NULL;
        file->buffer_pos=0;
        file->buffer_len=0;
    }
    else
    {
        //move rest down make it available for later 
        memmove(file->buffer,
                &file->buffer[want],
                (file->buffer_pos - want));

        file->buffer_pos -= want;
    }
    return 0;
}

static long
get_file_size_from_head(const char *buf)
{
	int i;
	char *pchar;
	char length[FILE_LENGTH];
	pchar = strstr(buf,"Content-Length:");
	if(pchar == NULL)
		return 0;
	pchar += 16;

	i=0;
	while(*pchar != '\r')
	{
		length[i]=*pchar;
		pchar ++;
		i++;
	}

	return atol(length);
}

URL_FILE *
CoolUrlFopen(const char *url,const char *operation)
{
    /* this code could check for URLs or types in the 'url' and
       basicly use the real fopen() for standard files */

    URL_FILE *file;
	int ret;
    (void)operation;
	char head_buffer[BUFFER_SIZE];
	
    file = (URL_FILE *)malloc(sizeof(URL_FILE));
    if(!file)
        return NULL;

    memset(file, 0, sizeof(URL_FILE));
	if(url)
		file->url=strdup(url);
	else
		file->url=NULL;

        curl_global_init(CURL_GLOBAL_ALL);
	//get http head
	file->curl = curl_easy_init();

	curl_easy_setopt(file->curl, CURLOPT_URL, file->url);
	curl_easy_setopt(file->curl, CURLOPT_WRITEDATA, head_buffer);
	curl_easy_setopt(file->curl, CURLOPT_NOBODY, 1);
	curl_easy_setopt(file->curl, CURLOPT_HEADER, 1);
	curl_easy_setopt(file->curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(file->curl, CURLOPT_WRITEFUNCTION, write_head_callback);
	ret = curl_easy_perform(file->curl);
	curl_easy_cleanup(file->curl);
	if(ret != CURLE_OK)
	{
		printf("get file size error url=%s",file->url);
		return NULL;
	}
	file->size = get_file_size_from_head(head_buffer);
	if(!file->size)
		return NULL;

	file->curl = curl_easy_init();
	curl_easy_setopt(file->curl, CURLOPT_URL, file->url);
	curl_easy_setopt(file->curl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt(file->curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(file->curl, CURLOPT_WRITEFUNCTION, write_callback);

	multi_handle = curl_multi_init();

	curl_multi_add_handle(multi_handle, file->curl);

    // lets start the fetch 
	while(curl_multi_perform(multi_handle, &file->still_running) ==
			CURLM_CALL_MULTI_PERFORM );

	if((file->buffer_pos == 0) && (!file->still_running))
	{
		// if still_running is 0 now, we should return NULL 

		// make sure the easy handle is not in the multi handle anymore 
		curl_multi_remove_handle(multi_handle, file->curl);

		// cleanup 
		curl_easy_cleanup(file->curl);

		free(file);

		file = NULL;
	}
    return file;
}

int
CoolUrlFclose(URL_FILE *file)
{
    int ret=0;

    // make sure the easy handle is not in the multi handle anymore 
	curl_multi_remove_handle(multi_handle, file->curl);
	curl_multi_cleanup(multi_handle);
	multi_handle = NULL;
    // cleanup 
	curl_easy_cleanup(file->curl);
        curl_global_cleanup();

    if(file->buffer)
        free(file->buffer);// free any allocated buffer space 
	if(file->url)
		free(file->url);
    free(file);

    return ret;
}

int
CoolUrlFeof(URL_FILE *file)
{
    int ret=0;

	if((file->buffer_pos == 0) && (!file->still_running))
		ret = 1;

    return ret;
}

size_t
CoolUrlFread(void *ptr, size_t size, size_t nmemb, URL_FILE *file)
{
    size_t want;


	want = nmemb * size;

	fill_buffer(file,want,1);

    // check if theres data in the buffer - if not fill_buffer()
    // either errored or EOF 
	if(!file->buffer_pos)
		return 0;

    // ensure only available data is considered 
	if(file->buffer_pos < want)
		want = file->buffer_pos;

    // xfer data to caller */
	memcpy(ptr, file->buffer, want);

	use_buffer(file,want);

	want = want / size;     // number of items - nb correct op - checked
                            //      with glibc code
    return want;
}

char *
CoolUrlFgets(char *ptr, int size, URL_FILE *file)
{
    int want = size - 1;// always need to leave room for zero termination 
    int loop;

	fill_buffer(file,want,1);

    //check if theres data in the buffer - if not fill either errored or
    //EOF 
	if(!file->buffer_pos)
		return NULL;

    //ensure only available data is considered
	if(file->buffer_pos < want)
		want = file->buffer_pos;

    //buffer contains data 
    // look for newline or eof 
	for(loop=0;loop < want;loop++)
	{
		if(file->buffer[loop] == '\n')
		{
			want=loop+1;//nclude newline 
			break;
		}
	}

    //xfer data to caller 
	memcpy(ptr, file->buffer, want);
	ptr[want]=0;//allways null terminate 

	use_buffer(file,want);

    return ptr;//success 
}

void
CoolUrlRewind(URL_FILE *file)
{
	//halt transaction 
	curl_multi_remove_handle(multi_handle, file->curl);

    // restart 
	curl_multi_add_handle(multi_handle, file->curl);

    // ditch buffer - write will recreate - resets stream pos
	if(file->buffer)
		free(file->buffer);

	file->buffer=NULL;
	file->buffer_pos=0;
	file->buffer_len=0;

}

void
CoolUrlSeek(URL_FILE *file,long offset)
{
	char range[FILE_LENGTH];
	if(offset > file->size)
		return;
	sprintf(range,"%ld-",offset);

	curl_multi_remove_handle(multi_handle, file->curl);
	curl_multi_cleanup(multi_handle);
	multi_handle = curl_multi_init();

	curl_easy_reset(file->curl);
	curl_easy_setopt(file->curl, CURLOPT_URL, file->url);
	curl_easy_setopt(file->curl, CURLOPT_WRITEDATA, file);
	curl_easy_setopt(file->curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(file->curl, CURLOPT_RANGE, range);
	curl_easy_setopt(file->curl, CURLOPT_WRITEFUNCTION, write_callback);	
	curl_multi_add_handle(multi_handle, file->curl);

	if(file->buffer)
		free(file->buffer);

	file->buffer=NULL;
	file->buffer_pos=0;
	file->buffer_len=0;
	while(curl_multi_perform(multi_handle, &file->still_running) 
			==	CURLM_CALL_MULTI_PERFORM );
}

long
CoolUrlSize(URL_FILE *file)
{
	return file->size;
}


