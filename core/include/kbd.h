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
 * simple raw keyboard generic header.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2007-11-22 MG
 *
 */

#ifndef _KBD_H_
#define _KBD_H_

#include <stdio.h>
#include <stdlib.h>

typedef enum 
	{
		RAW_BTN_BACK   = 0x13,
		RAW_BTN_UP     = 0x15,
		RAW_BTN_DOWN   = 0x16,
		RAW_BTN_LEFT   = 0x17,
		RAW_BTN_RIGHT  = 0x18,
		RAW_BTN_ENTER  = 0x19,
		RAW_BTN_null   = 0xff,
	} RAW_BUTTON;

RAW_BUTTON CoolKbdPolling( void );

#endif //_KBD_H_

