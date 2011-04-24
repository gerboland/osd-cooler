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
 * framebuffer generic routines.
 *
 * REVISION:
 * 
 * 4) 16bit BMP fade in-out support. ---------------------- 2007-07-07 MG
 * 3) Cleaned up and optimize. ---------------------------- 2006-05-03 MG
 * 2) Add graphic function. ------------------------------- 2006-03-28 EY
 * 1) Initial creation. ----------------------------------- 2006-02-28 EY
 *
 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "fb.h"
#include "fb-internal.h"
#include "font_16x32.h"
#include "font_32x32.h"

//#define OSD_DBG_MSG
#include "nc-err.h"
#if BUILD_TARGET_ARM	
#include <linux/itfb.h>
#endif

#define FONT_WIDTH_1   16
#define FONT_HEIGHT_1  32
#define FONT_WIDTH_2   32
#define FONT_HEIGHT_2  32

static short masktab[]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
static int screen_fd = 0;
static int graph_inited = 0;
static unsigned char *screen_ptr=(unsigned char*)(0x0400);
static unsigned char *E_Font_1=(unsigned char*)(0x8500);
static unsigned char *E_Font_2;
static short screen_width  = 240;
static short screen_height = 320;

static int open_fb(void)
{
	char *env;

	if (!screen_fd)
	{ 
	    if(!(env = getenv("FRAMEBUFFER"))) env = "/dev/fb0";
		screen_fd = open(env, O_RDWR);
		if(screen_fd < 0) 
		{
		    ERRLOG("Error opening %s. Check kernel config\n", env);
			return 0;
		}
	}
	return screen_fd;
}

static void close_fb(void)
{
	if (screen_fd)
	{ 
	    close(screen_fd);
		screen_fd = 0;
	}
}

void
CoolFbShowFixInfo(const struct fb_fix_screeninfo * fix)
{
#if 	BUILD_TARGET_ARM	
	DBGLOG("id %s\n",fix->id);
	DBGLOG("smem_start %d\n",fix->smem_start); /* Start of frame buffer mem */
	                                          /* (physical address) */
	DBGLOG("smem_len %d\n",fix->smem_len);			/* Length of frame buffer mem */
	DBGLOG("type %d\n",fix->type);			/* see FB_TYPE_*		*/
	DBGLOG("type_aux %d\n",fix->type_aux);			/* Interleave for interleaved Planes */
	DBGLOG("visual %d\n",fix->visual);			/* see FB_VISUAL_*		*/ 
	DBGLOG("xpanstep %d\n",fix->xpanstep);			/* zero if no hardware panning  */
	DBGLOG("ypanstep %d\n",fix->ypanstep);			/* zero if no hardware panning  */
	DBGLOG("ywrapstep %d\n",fix->ywrapstep);		/* zero if no hardware ywrap    */
	DBGLOG("line_length %d\n",fix->line_length);		/* length of a line in bytes    */
	DBGLOG("mmio_start %d\n",fix->mmio_start);	/* Start of Memory Mapped I/O   */
	                                            /* (physical address) */
	DBGLOG("mmio_len %d\n",fix->mmio_len);			/* Length of Memory Mapped I/O  */
	DBGLOG("accel %d\n",fix->accel);			/* Type of acceleration available */
	DBGLOG("reserved %d\n",fix->reserved);		/* Reserved for future compatibility */	
#endif
	return;
}

void
CoolFbShowVarInfo(const struct fb_var_screeninfo * var)
{
#if 	BUILD_TARGET_ARM
	DBGLOG("xres %d\n",var->xres);			/* visible resolution		*/
	DBGLOG("yres %d\n",var->yres);
	DBGLOG("xres_virtual %d\n",var->xres_virtual);		/* virtual resolution		*/
	DBGLOG("yres_virtual %d\n",var->yres_virtual);
	DBGLOG("xoffset %d\n",var->xoffset);			/* offset from virtual to visible */
	DBGLOG("yoffset %d\n",var->yoffset);			/* resolution			*/

	DBGLOG("bits_per_pixel %d\n",var->bits_per_pixel);		/* guess what			*/
	DBGLOG("grayscale %d\n",var->grayscale);		/* != 0 Graylevels instead of colors */

	DBGLOG("red offset %d length %d msb_right %d \n",
		   var->red.offset,var->red.length,var->red.msb_right);		/* bitfield in fb mem if true color, */
	DBGLOG("green offset %d length %d msb_right %d \n",
		   var->green.offset,var->green.length,var->green.msb_right);	/* else only length is significant */
	DBGLOG("blue offset %d length %d msb_right %d \n",
		   var->blue.offset,var->blue.length,var->blue.msb_right);
	DBGLOG("transp offset %d length %d msb_right %d \n",
		   var->transp.offset,var->transp.length,var->transp.msb_right);	/* transparency			*/	

	DBGLOG("nonstd %d\n",var->nonstd);			/* != 0 Non standard pixel format */

	DBGLOG("activate %d\n",var->activate);			/* see FB_ACTIVATE_*		*/

	DBGLOG("height %d\n",var->height);			/* height of picture in mm    */
	DBGLOG("width %d\n",var->width);			/* width of picture in mm     */

	DBGLOG("accel_flags %d\n",var->accel_flags);		/* acceleration flags (hints)	*/

	/* Timing: All values in pixclocks, except pixclock (of course) */
	DBGLOG("pixclock %d\n",var->pixclock);			/* pixel clock in ps (pico seconds) */
	DBGLOG("left_margin %d\n",var->left_margin);		/* time from sync to picture	*/
	DBGLOG("right_margin %d\n",var->right_margin);		/* time from picture to sync	*/
	DBGLOG("upper_margin %d\n",var->upper_margin);		/* time from sync to picture	*/
	DBGLOG("lower_margin %d\n",var->lower_margin);
	DBGLOG("hsync_len %d\n",var->hsync_len);		/* length of horizontal sync	*/
	DBGLOG("vsync_len %d\n",var->vsync_len);		/* length of vertical sync	*/
	DBGLOG("sync %d\n",var->sync);			/* see FB_SYNC_*		*/
	DBGLOG("vmode %d\n",var->vmode);			/* see FB_VMODE_*		*/
	DBGLOG("rotate %d\n",var->rotate);			/* angle we rotate counter clockwise */
	DBGLOG("reserved %d\n",var->reserved);		/* Reserved for future compatibility */
#endif
	return;
}

void
CoolFbGetInfo(struct fb_fix_screeninfo *fb_fix,struct fb_var_screeninfo *fb_var)
{
#if 	BUILD_TARGET_ARM
    int fb = open_fb();

	if(ioctl(fb, FBIOGET_FSCREENINFO, fb_fix) == -1 ||
		ioctl(fb, FBIOGET_VSCREENINFO, fb_var) == -1) 
	{
	    ERRLOG("Error reading screen info. \n");
		close_fb();
		return;
	}

	CoolFbShowFixInfo(fb_fix);
	CoolFbShowVarInfo(fb_var);
#endif	
}

void
CoolFbGetVarInfo(struct fb_var_screeninfo * fb_var)
{
#if 	BUILD_TARGET_ARM
    int fb = open_fb();

	if(ioctl(fb, FBIOGET_VSCREENINFO, fb_var) == -1) 
	{
	    ERRLOG("Error reading screen info. \n");
		close_fb();
		return;
	}

	CoolFbShowVarInfo(fb_var);
#endif	
}


void
CoolFbSetInfo(const struct fb_var_screeninfo * fb_var)
{
#if 	BUILD_TARGET_ARM
    int fb = open_fb();
	
	if(ioctl(fb, FBIOPUT_VSCREENINFO, fb_var) == -1) 
    {
	    ERRLOG("Error writing screen info. \n");
		close_fb();
		return;
	}
#endif	
}

void
CoolFbSetDisplay(struct fb_var_screeninfo * fb_var)
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if(ioctl(fb, ITFB_SET_VISIBLE_REGION, fb_var) == -1) 
    {
	    ERRLOG("Error writing screen info. \n");
		close(fb);
		return;
	}
#endif	
}

void
CoolFbSetVisibleRegion(int posx,int posy,int width,int height)
{
#if 0// 	BUILD_TARGET_ARM
	struct fb_var_screeninfo fb_var;
	CoolFbGetVarInfo(&fb_var);
	fb_var.xres=width;
	fb_var.yres=height;
	fb_var.xoffset=posx;
	fb_var.yoffset=posy;
	CoolFbSetInfo(&fb_var);
#endif
	return;
}

void
CoolFbSetVDMode(int vdmode)  //0:ntsc-i  1:pal-i
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_SET_VD_MODE, vdmode) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSetOSD0Enable(void)  
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_ENABLE_OSD0, 0) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSetOSD0Disable(void)  
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_DISABLE_OSD0, 0) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbInitializeVID0(void)  
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_INITIALIZE_VID0, 0) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSetVID0Enable(void)  
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_ENABLE_VID0, 0) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSetVID1Enable(void)  
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_ENABLE_VID1, 0) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSetVID0Disable(void)  
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_DISABLE_VID0, 0) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSetVID1Disable(void)  
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_DISABLE_VID1, 0) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSetAlphaBlend(int blends)  //0-7
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();
	
	if((ioctl(fb, ITFB_SET_BLEND, blends) == -1) ) 
    {
	    ERRLOG("Error writing screen info. \n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSetTransparent(int transp)  //0:transp
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_ENABLE_TRANSP, transp) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSsaver(EM_FB_SSAVER_STATE fs)  
{
#if 	BUILD_TARGET_ARM
	int fb = open_fb();

	if((ioctl(fb, ITFB_SSAVER_STATE, fs) == -1) ) 
	{
	    ERRLOG("Error writing screen info.\n");
		close(fb);
		return;
	}
#endif
}

void
CoolFbSetMode(EM_FRAMEBUFFER_MODE mode)
{
	switch(mode)
	{
	case fmMonitor:
		CoolFbSetAlphaBlend(FB_CONTRIBUTION_0_8);
		CoolFbSetTransparent(FB_TRANSPARENT_ON);
		break;
	case fmPlay:
		CoolFbSetAlphaBlend(FB_CONTRIBUTION_0_8);
		CoolFbSetTransparent(FB_TRANSPARENT_OFF);
		break;
	case fmNothing:
		CoolFbSetAlphaBlend(FB_CONTRIBUTION_0_8);
		CoolFbSetTransparent(FB_TRANSPARENT_OFF);
		break;	
	case fmNormal:
		CoolFbSetAlphaBlend(FB_CONTRIBUTION_5_8);
		CoolFbSetTransparent(FB_TRANSPARENT_ON);	
		break;	
	case fmClear:
		CoolFbSetAlphaBlend(FB_CONTRIBUTION_0_8);
		CoolFbSetTransparent(FB_TRANSPARENT_ON);	
		break;
	default:
		CoolFbSetAlphaBlend(FB_CONTRIBUTION_6_8);
		CoolFbSetTransparent(FB_TRANSPARENT_ON);	
	}

	return;
}


static short 
InitGraph(void)
{
	struct fb_var_screeninfo screeninfo;
	int fd;

	if (graph_inited) return 1;

	fd = open_fb();
	
	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo)==-1) 
	{
		ERRLOG("Unable to retrieve framebuffer information");
		return 0;
	}

	screen_width = screeninfo.xres_virtual;
	screen_height = screeninfo.yres_virtual;

	E_Font_1=fontdata_16x32;
	E_Font_2=fontdata_32x32;
	screen_ptr = mmap(NULL, ((screen_height * screen_width*2)/4096+1)*4096, 
					  PROT_READ|PROT_WRITE, /*0*/MAP_SHARED, fd, 0);

	DBGLOG("sc_ptr=%x\n",screen_ptr);

	if (screen_ptr==MAP_FAILED) 
	{
		ERRLOG("Unable to mmap frame buffer");
		close_fb();
		graph_inited=0;
		return 0;
	}
	graph_inited=1;
	return 1;
}

/*
static void CloseGraph()
{
    if (graph_inited)
	{
		close_fb();
		graph_inited = 0;
	}
}
*/

static void 
XY_ClearScreen(unsigned short x1,unsigned short y1,
			   unsigned short x2,unsigned short y2)
{
	unsigned short h_y=y2-y1+1,w_x=x2-x1+1;
	memset(screen_ptr+screen_width*2*y1+2*x1,0,h_y*w_x*2);
}

static void 
ClearScreen(short color)  //0:black 0xff:white
{
	memset(screen_ptr,color,screen_width*screen_height*2);
}

static void 
V_scroll_screen(short height) //Up/Down Scroll
{
   short dir=(height<0);	

   if(dir) height=-height;
	   
   if(height<screen_height)
 	{
        long nBytes=height*screen_width*2;///////////
        long nCount=screen_width*2*(screen_height-height);
	    if(dir)  //Down Scroll
	       memmove(screen_ptr+nBytes,screen_ptr,nCount);
	      else     //Up Scroll
	       memmove(screen_ptr,screen_ptr+nBytes,nCount);
	} 
   else
       ClearScreen(0x0);	   
}

/*
static void 
H_scroll_screen(short width) //Left Scroll
{
    short dir=(width<0);
 
	if(dir) width=-width;
	
	if(width<screen_width)
	{
	    if(width&7)   //Check whether it is byte aligned
		{
		    short nCount=screen_width*screen_height>>3;
			static unsigned char mskr[]={0,1,3,7,0xf,0x1f,0x3f,0x7f},
			  mskl[]={0,0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe},
				buf[3200];
			unsigned char c,flag=0,d;
			short start=(width>>3),length=(width&7),i,j,wid=(screen_width>>3),off;
				
			memcpy(buf,screen_ptr,nCount);
			if(dir)
			{
			    for(off=0,j=0;j<screen_height;j++,off+=wid)
				{
				    for(flag=0,i=wid-1;i>=start;i--)
					{
					    d=c=*(buf+off+i);
						c<<=length;
						if(flag) c|=(flag>>(8-length));
						flag=d&mskl[length];
						*(buf+off+i)=c;
					}
					if(start)
					{
					    for(i=wid-start;i<wid;i++)
					    {
						  *(buf+off+i)=0;
						}
					}
				}
			}
			else
			{
			    for(off=0,j=0;j<screen_height;j++,off+=wid)
				{
				    for(flag=0,i=start;i<wid;i++)
					{
					  d=c=*(buf+off+i);
					  c>>=length;
					  if(flag) c|=(flag<<(8-length));
					  flag=d&mskr[length];
					  *(buf+off+i)=c;
					}
					if(start)
					{
					    for(i=0;i<start;i++)
						{
						  *(buf+off+i)=0;
						}
					}
				}
				memcpy(screen_ptr,buf,nCount); 
			}
		}
		else
		{
		    short i,j=0,wid=screen_width>>3;
						
			width>>=3;
			for(i=0;i<screen_height;i++,j+=wid)
			{
			  memmove(screen_ptr+j+width,screen_ptr+j,wid-width);
			  memset(screen_ptr+j,0,width);
			}
		}
	}
	else
  	    ClearScreen(0x0);
}
*/
static void 
draw_bmp(short sx, short sy, short rwidth, short height, 
		 char* pixel,unsigned short color,unsigned short groundcolor)
{
	short i, j, k;
	char flag=0;
	char *loc=screen_ptr+sx*2+sy*screen_width*2;
	for(j=0;j<height;j++)
	{
	    for(k=0;k<rwidth;k++)
		{
		    for(i=0;i<8;i++)
			{
			    flag= *(pixel+j*rwidth+k) & masktab[i];
				if(flag)
				{
				    // *(loc+j*screen_width*2+k*16+i*2)=0x00;
					// *(loc+j*screen_width*2+k*16+i*2+1)=0x00;
  				    *(unsigned short*)(loc+j*screen_width*2+k*16+i*2)=color;
				}
				else
				{
				    // *(loc+j*screen_width*2+k*16+i*2)=0xff;
				    // *(loc+j*screen_width*2+k*16+i*2+1)=0xff;
				    *(unsigned short*)(loc+j*screen_width*2+k*16+i*2)=groundcolor;                 
				}
			}
		}
	}
}

void 
CoolFbTextOut(short x,short y, const char *buf,short color,short groundcolor)
{
	int i,count=strlen(buf);

	if (InitGraph()!=1)	return;

	if(x<0) x=0;
	else if(x>screen_width) x=screen_width;

	if(y<0) y=0;
	else if(y>screen_height) y=screen_height;

	for(i=0;i<count;) 
	{
		if(buf[i]=='\n') 
		{
			if(y+FONT_HEIGHT_1>screen_height) 
			{
			    V_scroll_screen(y+FONT_HEIGHT_1-screen_height);////up
				XY_ClearScreen(0,screen_height-FONT_HEIGHT_1,screen_width-1,screen_height-1);
				y=screen_height-FONT_HEIGHT_1;
			}
			else y+=FONT_HEIGHT_1;
			x=0;	     
		}
		else
		{
			if((x+FONT_WIDTH_1)>screen_width) 
			{
				y+=FONT_HEIGHT_1;
				x=0;
			}

			if(y+FONT_HEIGHT_1>screen_height) 
			{
				V_scroll_screen(y+FONT_HEIGHT_1-screen_height);////up
				XY_ClearScreen(0,screen_height-FONT_HEIGHT_1,screen_width-1,screen_height-1);
				y=screen_height-FONT_HEIGHT_1;
			}
			draw_bmp(x,y,(FONT_WIDTH_1+7)/8,FONT_HEIGHT_1,E_Font_1+(buf[i]*((FONT_WIDTH_1+7)/8 *FONT_HEIGHT_1)),color,groundcolor);	
			x+=FONT_WIDTH_1;
		}
		i++;
	}//end 1
}

void 
CoolFbTextOut2(short x,short y, const char *buf,short color,short groundcolor)
{
	int i,count=strlen(buf);

	if (InitGraph()!=1)	return;

	if(x<0) x=0;
	else if(x>screen_width) x=screen_width;

	if(y<0) y=0;
	else if(y>screen_height) y=screen_height;

	for(i=0;i<count;) 
	{
		if(buf[i]=='\n') 
		{
			if(y+FONT_HEIGHT_2>screen_height) 
			{
			    V_scroll_screen(y+FONT_HEIGHT_2-screen_height);////up
				XY_ClearScreen(0,screen_height-FONT_HEIGHT_2,screen_width-1,screen_height-1);
				y=screen_height-FONT_HEIGHT_2;
			}
			else y+=FONT_HEIGHT_2;
			x=0;	     
		}
		else
		{
			if((x+FONT_WIDTH_2)>screen_width) 
			{
				y+=FONT_HEIGHT_2;
				x=0;
			}

			if(y+FONT_HEIGHT_2>screen_height) 
			{
				V_scroll_screen(y+FONT_HEIGHT_2-screen_height);////up
				XY_ClearScreen(0,screen_height-FONT_HEIGHT_2,screen_width-1,screen_height-1);
				y=screen_height-FONT_HEIGHT_2;
			}
			draw_bmp(x,y,(FONT_WIDTH_2+7)/8,FONT_HEIGHT_2,E_Font_2+(buf[i]*((FONT_WIDTH_2+7)/8 *FONT_HEIGHT_2)),color,groundcolor);	
			x+=FONT_WIDTH_2;
		}
		i++;
	}//end 1
}

static void 
my_draw_bmp(short x,short y,unsigned short width,unsigned short height,char *pixel)
{
    short i,j;
	long aver_size=((width+31)/32)*4;
	//char masktab[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
	for(j=0;(y+j)<screen_height&&(y+j)>=0&&j<height;j++)     //0---height;
	{
	    for(i=0;(x+i)<screen_width&&(x+i)>=0&&i<width;i++)      //0---width ;
		{
            if(*(pixel+j*aver_size+i/8) & masktab[i%8])//judge pixel "0"or "1";       
			{
			    *(screen_ptr+(y+j)*screen_width*2+2*(i+x))=0x00;
				*(screen_ptr+(y+j)*screen_width*2+2*(i+x)+1)=0x00;
			}
			else
			{ 
			    *(screen_ptr+(y+j)*screen_width*2+2*(i+x))=0xff;
				*(screen_ptr+(y+j)*screen_width*2+2*(i+x)+1)=0xff;
			}
		}
	}
}

static void 
color_555_draw_bmp(short x,short y,unsigned short width,unsigned short height,char *buf)
{   
    long i,j,offset;
    unsigned short tmp,red,green;
	for(j=y;j<screen_height&&(j-y)<height;j++)
	{
        for(i=x;i<screen_width&&(i-x)<width;i++)
		{
		    offset=(j-y)*width*2+(i-x)*2;
			tmp=*(unsigned short*)(buf+offset);
			red=(tmp&0x7c00)<<1;
			green=(tmp&0x03e0)<<1;
			tmp&=0x001f;
			tmp|=red|green;
            *(unsigned short*)(screen_ptr+j*screen_width*2+2*i)=tmp;
		}
	} 
}

static void
show_buf(char * buf, short x, short y, unsigned short w, unsigned short h, unsigned short lum)
{
    long i, j, offset;
    unsigned short tmp, r, g, b;
	unsigned short enu = (lum>>8)&0xff;
	unsigned short deno = lum & 0xff;

	if ( !enu || !deno ) 
	{
		for(j = y; j < screen_height && (j-y) < h;j++)
		{
			for(i = x; i < screen_width && (i-x) < w; i++)
			{
				*(unsigned short*)(screen_ptr + j * screen_width * 2 + 2 * i) = 0;
			}
		} 
	}
	else
	{
		for(j = y; j < screen_height && (j-y) < h;j++)
		{
			for(i = x; i < screen_width && (i-x) < w; i++)
			{
				offset = (j - y) * w * 2 + (i - x) * 2;
				tmp = *(unsigned short*)(buf + offset);

				r = (((tmp >> 10) & 0x1f) * enu / deno) & 0x1f;
				g = (((tmp >>  5) & 0x1f) * enu / deno) & 0x1f;
				b = ((tmp & 0x1f) * enu / deno) & 0x1f;
				
				*(unsigned short*)(screen_ptr + j * screen_width * 2 + 2 * i) = 
					(r << 11) | (g << 6) | b;
			}
		}
	}
}

static void 
color_565_draw_bmp(short x,short y,unsigned short width,unsigned short height,char *buf)
{   
    long i,j,offset;
    unsigned short tmp;

    for(j=y; j<screen_height && (j-y)<height; j++)
    {
	 for(i=x; i<screen_width && (i-x)<width; i++)
	 { 
	      offset = (j-y)*width*2 + (i-x)*2;
	      tmp= *(unsigned short*)(buf+offset);
	      *(unsigned short*)(screen_ptr + j*screen_width*2 + 2*i) = tmp;
	 }
    }
}

void 
CoolFbShowBuf(const char *Buf,short x,short y)
{
#if 	BUILD_TARGET_ARM
  BMPHEAD bmp_inf;
  char Tmp,*buf,*buf1;
  int red,green,blue;
  long aver_size,size,i,m,j,k;

  if (InitGraph()!=1) return;

  
  memcpy(&bmp_inf.bfSize,(Buf+2),52);
  
  //DBGLOG("biBiCount %x\n",bmp_inf.biBitCount);
  if((bmp_inf.biBitCount!=1) && 
  	 (bmp_inf.biBitCount!=16) &&
  	 (bmp_inf.biBitCount!=24))
  {
	  //fclose(fp);
	  puts("Unsupported 2|16color bitmap!\n");
	  DBGLOG("bitcolor=%d\n",*((short*)&bmp_inf.biBitCount));
	  return;
  }
  //Show black and white                         
  else if(bmp_inf.biBitCount==1)                    
  {                           
	  DBGLOG("%d* %d\n",bmp_inf.biWidth,bmp_inf.biHeight);
	  aver_size=((bmp_inf.biWidth+15)/16)*2; //line size
	  DBGLOG("aversize=%d\n",aver_size);
	  size=aver_size*bmp_inf.biHeight;
	  buf=(char*)malloc(size);
	  memcpy(buf,(Buf+62),size);
	  
	  //fseek(fp,62L,0);
	  // fread(buf,1,size,fp);
	  // fclose(fp);
	  for(i=0;i<(bmp_inf.biHeight>>1);i++)
	  {
		  for(k=i*aver_size,j=0;j<aver_size;j++)
		  {
			  Tmp=buf[k+j];
			  buf[k+j]=buf[size-k-aver_size+j];
			  buf[size-k-aver_size+j]=Tmp;
		  }
	  }
	  for(i=0;i<size;i++)	buf[i]^=0xff;
	  DBGLOG("the width=%d\nthe height=%d\n",bmp_inf.biWidth,bmp_inf.biHeight);
	  my_draw_bmp(x,y,bmp_inf.biWidth,bmp_inf.biHeight,buf);
	  free(buf);
  }
  //show 16_bit BMP   SHOW_16_color_bmp
  else if(bmp_inf.biBitCount==16)                              
  {
	  size=bmp_inf.biWidth*bmp_inf.biHeight*2;
	  buf=(char*)malloc(size);
	  memcpy(buf,(Buf+54),size);
	  for(i=0;i<(bmp_inf.biHeight>>1);i++)
	  {
		  for(k=i*bmp_inf.biWidth*2,j=0;j<bmp_inf.biWidth*2;j++)
		  {
			  Tmp=buf[k+j];
			  buf[k+j]=buf[size-k-bmp_inf.biWidth*2+j];
			  buf[size-k-bmp_inf.biWidth*2+j]=Tmp;
		  }
	  }
	  color_555_draw_bmp(x,y, bmp_inf.biWidth,bmp_inf.biHeight,buf);
	  // else
	  //  color_565_draw_bmp(x,y,bmp_inf.biWidth,bmp_inf.biHeight,buf);
	  free(buf);
  }
  else if(bmp_inf.biBitCount==24)
  { 
	  DBGLOG("%d* %d\n",bmp_inf.biWidth,bmp_inf.biHeight);
	  size=bmp_inf.biHeight*((bmp_inf.biWidth*3+3)/4*4);
	  buf=(char*)malloc(size);
	  buf1=(char*)malloc(bmp_inf.biWidth*bmp_inf.biHeight*2);
	  memcpy(buf,(Buf+54),size);
	  
	  for(i=0;i<(bmp_inf.biHeight>>1);i++)
	  {
		  for(k=i*((bmp_inf.biWidth*3+3)/4)*4,j=0;j<bmp_inf.biWidth*3;j++)
		  {
			Tmp=buf[k+j];
			buf[k+j]=buf[size-k-bmp_inf.biWidth*3+j];
			buf[size-k-bmp_inf.biWidth*3+j]=Tmp;
		  }
	  }
	  for(i=0;i<(bmp_inf.biHeight);i++)
	  {
		  for(k=i*((bmp_inf.biWidth*3+3)/4)*4,m=i*bmp_inf.biWidth*2,
				j=0;j<bmp_inf.biWidth;j++)
		  {
			  /*  red=(((buf[k+j*3+2])>>3))*2048;
				  green=(buf[k+j*3+1]>>3)*64;
				  blue=(buf[k+j*3]>>3);*/
			  red=((float)(buf[k+j*3+2]))/255*31+0.5;
			  green=((float)(buf[k+j*3+1]))/255*63+0.5;
			  blue=((float)(buf[k+j*3]))/255*31+0.5;
			  *(unsigned short *)(buf1+m+j*2)=(red<<11)|(green<<5)|(blue);
			  // *(unsigned short *)(buf1+m+j*2)=red|green|blue;
		  }
	  }
	  color_565_draw_bmp(x,y,bmp_inf.biWidth,bmp_inf.biHeight,buf1);
	  free(buf);
	  free(buf1);	                                                            
  }  
#endif
}
  
void 
CoolFbShowBmpFile(const char *filename,short x,short y)
{
#if 	BUILD_TARGET_ARM
    #define BUF_SIZE 700000
	char buf1[BUF_SIZE];
	struct stat st;   
	FILE *fp;

	if (InitGraph()!=1) return;

	fp=fopen(filename,"r");
	stat(filename,&st);
	if (st.st_size > BUF_SIZE)
	{
	    ERRLOG("can not hold BMP.\n");
	}
	else fread(buf1,1,st.st_size,fp);
	fclose(fp);

	CoolFbShowBuf(buf1,x,y);

	return;
#endif
}

void 
CoolFbClearScreen(short bgcolor)
{
#if 	BUILD_TARGET_ARM
    if (InitGraph()!=1) return;
	ClearScreen(bgcolor);
#endif
}

/** show bmp buffer.
 *
 * @param bufhdl
 *        bmp buffer handle.
 * @param lum
 *        specified luminence. high byte contains enumerator, low byte
 *        contains denominator.
 *
 */
void 
CoolFbShowBuf2(bmp_buf_handle_t * bufhdl, unsigned short lum)
{
	show_buf(bufhdl->buf, bufhdl->x, bufhdl->y, bufhdl->w, bufhdl->h, lum);
}

/** show bmp file with luminence specified.
 *
 * @param filename
 *        bmp file path.
 * @param x
 *        upperleft X coordination.
 * @param y
 *        upper left y coordination.
 * @param lum
 *        specified luminence. high byte contains enumerator, low byte
 *        contains denominator.
 * @param bufhdl
 *        returned buffer handle for bmp manipulation if not NULL, and it is
 *        then caller's responsibility to free this buffer.
 * 
 */
void 
CoolFbShowBmpFile2(const char *filename,short x,short y, unsigned short lum, bmp_buf_handle_t * bufhdl)
{
    #define BUF_SIZE 700000
	char Buf[BUF_SIZE];
	struct stat st;   
	FILE *fp;
	BMPHEAD bmp_inf;
	char Tmp,*buf;
	long size,i,j,k;
	
	if (InitGraph()!=1) return;

	fp=fopen(filename,"r");
	stat(filename,&st);
	if (st.st_size > BUF_SIZE)
	{
	    ERRLOG("can not hold BMP.\n");
	}
	else fread(Buf,1,st.st_size,fp);
	fclose(fp);

	memcpy(&bmp_inf.bfSize,(Buf+2),52);
	
	if (bmp_inf.biBitCount!=16)
	{
		//FIXME: support 16 bitmap only for now.
		WARNLOG("Unsupported bitcolor=%d\n",*((short*)&bmp_inf.biBitCount));
		return;
	}
	
	size=bmp_inf.biWidth*bmp_inf.biHeight*2;
	buf=(char*)malloc(size);
	memcpy(buf,(Buf+54),size);
	for(i=0;i<(bmp_inf.biHeight>>1);i++)
	{
		for(k=i*bmp_inf.biWidth*2,j=0;j<bmp_inf.biWidth*2;j++)
		{
			Tmp=buf[k+j];
			buf[k+j]=buf[size-k-bmp_inf.biWidth*2+j];
			buf[size-k-bmp_inf.biWidth*2+j]=Tmp;
		}
	}
	
	if (lum)
		show_buf(buf, x, y, bmp_inf.biWidth, bmp_inf.biHeight, lum);

	if (!bufhdl) free(buf);
	else
	{
		bufhdl->buf = buf;
		bufhdl->x = x;
		bufhdl->y = y;
		bufhdl->w = bmp_inf.biWidth;
		bufhdl->h = bmp_inf.biHeight;
	}
}

bmp_buf_handle_t * 
CoolFbSnapScreen(short x, short y, unsigned short w, unsigned short h)
{
    long i, j, offset;
	bmp_buf_handle_t * bufhdl;
	char * pb;

	if (InitGraph()!=1) return NULL;

	bufhdl = (bmp_buf_handle_t *)malloc(sizeof(bmp_buf_handle_t));
	if (!bufhdl) goto bail;
	memset(bufhdl, 0, sizeof(bmp_buf_handle_t));

	pb = (char *)malloc(w*h*2);
	if (!pb) goto bail;

	bufhdl->buf = pb;
	bufhdl->x = x;
	bufhdl->y = y;
	bufhdl->w = w;
	bufhdl->h = h;

	for(j = y; j < screen_height && (j-y) < h;j++)
	{
		for(i = x; i < screen_width && (i-x) < w; i++)
		{
			offset = (j - y) * w * 2 + (i - x) * 2;
			*(unsigned short*)(pb + offset) = 
				*(unsigned short*)(screen_ptr + j * screen_width * 2 + 2 * i);
		}
	}
	return bufhdl;

 bail:
	if (bufhdl)
	{
		if (bufhdl->buf) free(bufhdl->buf);
		free(bufhdl);
	}
	return NULL;
}

/* for draw a line */

static inline void setpixel(short x, short y, short color)
{
     if ((x<0) || (x>=screen_width) || (y<0) || (y>=screen_height))
	  return;
     {
	  unsigned char * loc = screen_ptr + ((y * screen_width*2 + x*2 ));
	  if (color)
	  {
	       *loc =0xff;
	       *(loc+1)=0xff;
	  }
	  else
	  {
	       *loc = 0x0;
	       *(loc+1)=0x0;
	  }
     }
}

/* Abrash's take on the simplest Bresenham line-drawing algorithm. 
 *
 * This isn't especially efficient, as we aren't combining the bit-mask
 * logic and addresses with the line drawing code, never mind higher
 * level optimizations like run-length slicing, etc.
 *
 */
static inline void draw_xish_line(short x, short y, short dx, short dy, short xdir)
{
     short dyX2 = dy + dy;
     short dyX2mdxX2 = dyX2 - (dx + dx);
     short error = dyX2 - dx;
  
     setpixel(x, y, 1);
     while (dx--) 
     {
	  if (error >= 0) 
	  {
	       y++;
	       error += dyX2mdxX2;
	  } 
	  else 
	       error += dyX2;

	  x += xdir;
	  setpixel(x, y, 1);
     }
}

static inline void draw_yish_line(short x, short y, short dx, short dy,short xdir)
{
     short dxX2 = dx + dx;
     short dxX2mdyX2 = dxX2 - (dy + dy);
     short error = dxX2 - dy;
     
     setpixel(x, y, 1);
     while (dy--) 
     {
	  if (error >= 0) 
	  {
	       x+= xdir;
	       error += dxX2mdyX2;
	  } 
	  else 
	       error += dxX2;

	  y++;
	  setpixel(x, y, 1);
     }
}

/** draw a line
 *  parameters: 
 *      x1, y1: start point 
 *      x2, y2: end point
 */
void CoolFbLine(short x1, short y1, short x2, short y2)
{
     short dx,dy;
     
     if ( y1 > y2) 
     {
	  short t = y1;
	  y1 = y2;
	  y2 = t;
	  t = x1;
	  x1 = x2;
	  x2 = t;
     }
     
     dx = x2-x1;
     dy = y2-y1;
     
     if (dx > 0) 
     {
	  if (dx > dy)
	       draw_xish_line(x1, y1, dx, dy, 1);
	  else
	       draw_yish_line(x1, y1, dx, dy, 1);
     } 
     else 
     {
	  dx = -dx;
	  if (dx > dy)
	       draw_xish_line(x1, y1, dx, dy, -1);
	  else
	       draw_yish_line(x1, y1, dx, dy, -1);
     }
}

/** draw a rectangle
 *  parameters: 
 *      x1, y1: left-upper point 
 *      x2, y2: right-lower point
 */
void CoolFbRectangle(short x1, short y1, short x2, short y2)
{
	CoolFbLine(x1, y1, x2, y1);
	CoolFbLine(x2, y1, x2, y2);
	CoolFbLine(x2, y2, x1, y2);
	CoolFbLine(x1, y2, x1, y1);
}

/** draw a dash line
 *  only support horizontal or vertical line, don't support skew line
 *  parameters:
 *      x1, y1: start point
 *      x2, y2: end point
 */
void CoolFbDashedLine(short x1, short y1, short x2, short y2)
{
    short tempx, tempy;
    int len1 = 10, len2 = 5;

    if( (x1 != x2) && (y1 != y2) ) return;

    if(x1 == x2)
      {
	  tempy = y1;
	  while((tempy+len1) <= y2)
	    {
		CoolFbLine(x1, tempy, x2, tempy+len1);
		tempy += (len1+len2);
	    }
	  if(tempy >= y2) return;
	  else CoolFbLine(x1, tempy, x2, y2);
	  
      }
    else if(y1 == y2)
      {
	  tempx = x1;
	  while((tempx+len1) <= x2)
	    {
		CoolFbLine(tempx, y1, tempx+len1, y2);
		tempx += (len1+len2);
	    }
	  if(tempx >= x2) return;
	  else CoolFbLine(tempx, y1, x2, y2);      
      }
}

/** fill specific region to white
 *  parameters: 
 *      x1, y1: left-upper point 
 *      x2, y2: right-lower point
 */
void  CoolFbFillRectangle(short x1, short y1, short x2, short y2)
{
  short i,j;

  for(i = x1; i<x2; i++)
    {
      for(j = y1; j < y2; j++)
	  setpixel(i, j, 1);
    }
}

/** clear specific region to black 
 *  parameters: 
 *      x1, y1: left-upper point 
 *      x2, y2: right-lower point
 */
void CoolFbXYClearScreen(unsigned short x1,unsigned short y1,unsigned short x2,unsigned short y2)
{
    short i, j;

    for(i = x1; i<x2; i++)
      {
	  for(j = y1; j < y2; j++)
	      setpixel(i, j, 0);
      }
}

/** show RGB565 bmp picture
 *  parameters:
 *     x,y   : start position on the screen
 *     width : picture's width
 *     height: picture's height
 *     buf   : picture's data
 */
void CoolFbDraw565Bmp(short x,short y,unsigned short width,unsigned short height,char *buf)
{
     color_565_draw_bmp(x, y, width, height, buf);
}
