#ifndef IR_BLASTER_CONTROL__H
#define IR_BLASTER_CONTROL__H
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
 * OSD IR remote and blaster controls management header.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2006-07-31 MG 
 *
 */

#include <stdio.h>
#include <stdlib.h>

/** IR manufacture ID. */
enum
  {
	IRB_NEUROS_OSD,
  };


void 
CoolBlasterOut (int manID, int devID, const char * code, int codeLength);

#endif /* IR_BLASTER_CONTROL__H */
