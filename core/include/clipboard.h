#ifndef CLIPBOARD__H
#define CLIPBOARD__H
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
 * Clipboard module header.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2007-07-26 NW 
 *
 */

#include <stdio.h>
#include <stdlib.h>

typedef enum
{
	CB_TEXT,
	CB_FILES,
	CB_BITMAP
}CLIPBOARD_TYPE;

typedef enum
{
	CB_COPY,
	CB_CUT
}CLIPBOARD_OPERATION;

typedef struct
{
	CLIPBOARD_TYPE type;
	CLIPBOARD_OPERATION operation;
	int datalength;
}CLIPBOARD_HEAD;

int CoolClipboardReadHead(CLIPBOARD_HEAD *head);
int CoolClipboardReadData(void *data);
void CoolClipboardWrite(CLIPBOARD_HEAD head, void *data);
void CoolClipboardClear( );

#endif /* CLIPBOARD__H */
