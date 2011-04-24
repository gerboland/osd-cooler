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
 * framebuffer generic header.
 *
 * REVISION:
 * 
 *
 * 2) Add graphic function. ------------------------------- 2006-03-28 EY
 * 1) Initial creation. ----------------------------------- 2006-02-28 EY
 *
 */

#ifndef _FB_H_
#define _FB_H_

#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <fcntl.h>


typedef enum 
{
	fmNormal, /** menu layer is not transparent to any color, 
				  while alpha overlaying with video. */
	fmClear,  /** The same as fmNormal, 
				  but with more clear menu layer */
	fmMonitor,/** menu layer is only transparent to black color,
				  while every other color blocks video. */
	fmPlay,   /** NOT DEFINED YET!!*/
	fmNothing, /** NOT DEFINED YET!!*/
} EM_FRAMEBUFFER_MODE;

#define FB_CONTRIBUTION_0_8	0		
#define FB_CONTRIBUTION_1_8	1
#define FB_CONTRIBUTION_2_8	2
#define FB_CONTRIBUTION_3_8	3
#define FB_CONTRIBUTION_4_8	4
#define FB_CONTRIBUTION_5_8	5
#define FB_CONTRIBUTION_6_8	6
#define FB_CONTRIBUTION_8_8	7
#define FB_TRANSPARENT_ON		1
#define FB_TRANSPARENT_OFF	0

typedef enum 
{
	fssHide,	/**hide screen saver,restore prev fbmode **/
	fssShow,/**show screen saver,fb will contribute all and remember perv fbmode**/

} EM_FB_SSAVER_STATE;

typedef struct
{
	short x;
	short y;
	unsigned short w;
	unsigned short h;
	char * buf;
} bmp_buf_handle_t;

void
CoolFbShowFixInfo(const struct fb_fix_screeninfo * );
void
CoolFbShowVarInfo(const struct fb_var_screeninfo * );
void
CoolFbGetVarInfo(struct fb_var_screeninfo *);
void
CoolFbGetInfo(struct fb_fix_screeninfo *,struct fb_var_screeninfo *);
void
CoolFbSetInfo(const struct fb_var_screeninfo * );
void
CoolFbSetDisplay( struct fb_var_screeninfo * );
void
CoolFbSetVisibleRegion(int,int,int,int);
void
CoolFbSetVDMode(int );
void
CoolFbSetOSD0Enable(void);
void
CoolFbSetOSD0Disable(void);
void
CoolFbInitializeVID0(void);
void
CoolFbSetVID0Enable(void);
void
CoolFbSetVID0Disable(void);
void
CoolFbSetVID1Enable(void);
void
CoolFbSetVID1Disable(void);
void
CoolFbSetAlphaBlend(int);
void
CoolFbSetTransparent(int);
void
CoolFbSsaver(EM_FB_SSAVER_STATE );  
void
CoolFbSetMode(EM_FRAMEBUFFER_MODE);

void 
CoolFbTextOut(short ,short ,const char *,short ,short);
void 
CoolFbTextOut2(short ,short ,const char *,short ,short);
void 
CoolFbShowBuf(const char *,short ,short );
void 
CoolFbShowBmpFile(const char *,short ,short );
void 
CoolFbClearScreen(short);

void 
CoolFbShowBuf2(bmp_buf_handle_t *, unsigned short);
void 
CoolFbShowBmpFile2(const char *, short, short, unsigned short, bmp_buf_handle_t *);
bmp_buf_handle_t * 
CoolFbSnapScreen(short, short, unsigned short, unsigned short);

void
CoolFbLine(short x1, short y1, short x2, short y2);
void 
CoolFbRectangle(short x1, short y1, short x2, short y2);
void 
CoolFbDashedLine(short x1, short y1, short x2, short y2);
void 
CoolFbFillRectangle(short x1, short y1, short x2, short y2);
void 
CoolFbXYClearScreen(unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2);
void 
CoolFbDraw565Bmp(short x,short y,unsigned short width,unsigned short height,char *buf);

#endif //_FB_H_

