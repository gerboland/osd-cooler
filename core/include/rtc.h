#ifndef RTC__H
#define RTC__H
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
 * OSD time support module header.
 *
 * REVISION:
 * 
 * 2) Fixed devices and function calls for irrtc split ---- 2006-12-20 nerochiaro
 * 1) Initial creation. ----------------------------------- 2006-07-20 MG 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct
{
  int sec;
  int min;
  int hour;
  int day;
  int mon;
  int year;
} osd_time_t;


// time APIs.
int CoolGetTime(osd_time_t *);
int CoolSetTime(osd_time_t *);
int CoolOpenRTC(void);
int CoolGetRTCtime(osd_time_t *);
int CoolSetRTCtime(osd_time_t *);

#endif /* RTC__H */

