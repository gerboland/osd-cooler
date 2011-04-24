#ifndef CURL_FOPEN__H
#define CURL_FOPEN__H
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
#include <curl/curl.h>

enum fcurl_type_e { CFTYPE_NONE=0, CFTYPE_FILE=1, CFTYPE_CURL=2 };

struct fcurl_data
{
    CURL *curl;
    long size;					/* the size of the file on remote server*/
    char *buffer;               /* buffer to store cached data*/
	char *url;
    int buffer_len;             /* currently allocated buffers length */
    int buffer_pos;             /* end of data in buffer*/
    int still_running;          /* Is background url fetch still in progress */
};

typedef struct fcurl_data URL_FILE;


URL_FILE *CoolUrlFopen(const char *url, const char *operation);
int       CoolUrlFclose(URL_FILE *file);
int       CoolUrlFeof(URL_FILE *file);
size_t    CoolUrlFread(void *ptr, size_t size, size_t nmemb, URL_FILE *file);
char *    CoolUrlFgets(char *ptr, int size, URL_FILE *file);
void      CoolUrlRewind(URL_FILE *file);
void      CoolUrlSeek(URL_FILE *file, long offset);
long      CoolUrlSize(URL_FILE *file);


#endif /* CURL_FOPEN__H */
