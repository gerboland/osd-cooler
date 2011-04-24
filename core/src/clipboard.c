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
 * Clipboard routines.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2007-07-26 NW
 *
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "clipboard.h"

#define CLIPBOARD_FILE		"/var/tmp/clipboard"

/*
 *Function Name:CoolClipboardReadHead
 *Parameters:
 * head------read clipboard head to buf
 *Description:
 * read head from clipboard file
 *Returns:
 * 1==success
 * 0==failure
 */
int CoolClipboardReadHead(CLIPBOARD_HEAD *head)
{
	if( head == NULL )
		return 0;

	FILE *file = NULL;
	if( (file=fopen(CLIPBOARD_FILE,"r")) != NULL )
	{
		int headsize = sizeof(CLIPBOARD_HEAD);
		if( fread(head,1,headsize,file) == headsize )
		{
			fclose(file);
			return 1;
		}
		fclose(file);
	}

	return 0;
}

/*
 *Function Name:CoolClipboardReadData
 *Parameters:
 * data-----read clipboard data to buf
 *Description:
 * read data from clipboard file
 *Returns:
 * return data length
 */
int CoolClipboardReadData(void *data)
{
	if( data == NULL )
		return 0;

	FILE *file = NULL;
	int headsize = sizeof(CLIPBOARD_HEAD);
	int filesize = 0;
	int readlength = 0;
	if( (file=fopen(CLIPBOARD_FILE,"r")) != NULL )
	{
		fseek(file,0,SEEK_END);
		filesize = ftell(file);
		if( filesize>headsize )
		{
			fseek(file,headsize,SEEK_SET);
			readlength = fread(data,1,filesize-headsize,file);
		}
		fclose(file);
	}

	return readlength;
}

/*
 *Function Name:CoolClipboardWrite
 *Parameters:
 * head-------write clipboard head
 * data--------write clipboard data
 *Description:
 * save data to clipboard file
 *Returns:
 *
 */
void CoolClipboardWrite(CLIPBOARD_HEAD head, void *data)
{
	FILE *file = NULL;
	int headsize = sizeof(CLIPBOARD_HEAD);
	if( (file=fopen(CLIPBOARD_FILE,"w+")) != NULL )
	{
		fwrite(&head, 1, headsize, file);
		if( head.datalength>0 && data != NULL )
		{
			fseek(file,0,SEEK_END);
			fwrite(data, 1, head.datalength, file);
		}
		fclose(file);
	}
}

/*
 *Function Name:CoolClipboardClear
 *Parameters:
 * 
 *Description:
 * clear clipboard file
 *Returns:
 *
 */
void CoolClipboardClear( )
{
	remove(CLIPBOARD_FILE);
}
