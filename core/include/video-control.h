#ifndef VIDEO_CONTROL__H
#define VIDEO_CONTROL__H
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
 * OSD video controls management header.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2006-07-24 MG 
 *
 */

#include <stdio.h>
#include <stdlib.h>


int  CoolVideoPassthrough(int enable);
void CoolVideoComposite(int enable);
int  CoolVideoIsPresent(void);
int  CoolVideoIsPassthroughRelayEngaged(void);
int CoolVideoGetInputMode(void);
void CoolVideoSetPolling(int);
void CoolVideoSetPollingTime(int);

#endif /* VIDEO_CONTROL__H */
