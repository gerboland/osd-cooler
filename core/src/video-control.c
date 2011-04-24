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
 * Video signal management routines.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2006-07-24 MG
 *
 */
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "nc-type.h"
#include "cooler-core.h"

static int open_fd(void)
{
    static int fd = -1;
	if (-1 == fd)
	  {
		fd = open("/dev/tvp5150", O_RDWR);
		if (fd < 0) 
		  {
			WARNLOG("unable to open tvp5150 control.\n");
			return -1;
		  }
	  }
	return fd;
}

/** video pass through control. 
 *
 * @param enable
 *        TRUE to enable, FALSE to disable.
 * @return
 *        1 if any action is taken, 0 otherwise.
 */

int CoolVideoPassthrough(int enable)
{
  	int fd;
	int ret = 0;
	
	fd = open_fd();
	if (fd >0)
	{
		int passthru = 0;
		ioctl( fd, 0x77, &passthru );

		if ((enable == TRUE) && (passthru <= 0))
		{
			ret = 1;
			ioctl( fd, 1, 2);
			// TODO: enable audio passthrough too...
		}
		else if ((enable == FALSE) && (passthru > 0))
		{
			ret = 1;
			ioctl( fd, 1, 3);
			// TODO: disable audio passthrough too...
		}
	}
	return ret;
}

/** enable composite video source, otherwise S-video.
 *
 *
 *
 */
void CoolVideoComposite(int enable)
{
   	int fd;

	fd = open_fd();
	if (fd >0)
	{
		if (enable == TRUE)
		{
			ioctl( fd, 1, 0);
		}
		else if (enable == FALSE)
		{
			ioctl( fd, 1, 1);
		}
	}
}

/** check to see if video signal is available. 
 *
 * @return
 *      0 if not present, 1 if present.
 */
int CoolVideoIsPresent(void)
{
	static int fd_vs = -1;
	int st_vs = FALSE;
	
	char buf[2];
	int count;
	
	if(fd_vs < 0)
	{
		if ((fd_vs = open("/proc/tvp5150", 0)) < 0)      
		{       
			goto bail;
		}     
	}
	lseek(fd_vs,0,0);
	count = read(fd_vs,buf,1);
	
	if(count <=0) goto bail;
	
	buf[1]=0;
	
	if(strcmp("0",buf)==0)
	{
		DBGLOG("video is not present.\n");
		st_vs = FALSE;		
	  }
	else if(strcmp("1",buf)==0)
	{
		st_vs = TRUE;
		DBGLOG("video is present.\n");
	}
	else
	{
		st_vs = FALSE;
	}
 bail:
	return  st_vs;
}


/**
 * Check to see if passthru relay is engaged. 
 *
 * @return
 *      <= 0 disabled, > 0 enabled
 */
int CoolVideoIsPassthroughRelayEngaged(void)
{
	int fd;
	int passthru = 0;
	
	fd = open_fd();
	if (fd >0)
		ioctl( fd, 0x77, &passthru );

	return passthru;
}

/**
 * Get video input mode from tvp5150
 */
int CoolVideoGetInputMode(void)
{
	int mode = 0;
	int fd = -1;
	fd = open_fd();
	ioctl( fd, 0x8C, &mode );
	DBGLOG("get input mode : %d\n", mode);
	switch (mode)
	{
	case 0x81:
	       // NTSC
	       mode = 0;
	       break;
	case 0x83:
	       // PAL B,G,H,I,N
	       mode = 1;
	       break;
	case 0x85:
	       // PAL M
	       mode = 1;
	       break;
	case 0x87: 
	       // PAL N
	       mode = 1;
	       break;
	case 0x89:
	       // NTSC 4.43
	       mode = 0;
	       break;
	case 0x8b:
	       // SECAM
	       mode = 1;
	       break;
	default:
	       mode = 0;
	       break;
	}
	return mode;
}

/**
 *	enable/disable polling 
 *	@param enable
 *	TRUE: enable poll, FALSE : disable poll
 */
void CoolVideoSetPolling(int enable)
{
	int fd = -1;
	
	fd = open_fd();
	(enable) ?
	ioctl(fd, 0xaa, NULL) :
	ioctl(fd, 0x99, NULL);
}

/**
 * Set polling time
 * @param time
 * 	unit is 50ms, for example, time = 1, represent 50ms
 */
void CoolVideoSetPollingTime(int time)
{
	int fd = -1;
	
	fd = open_fd();
	ioctl(fd, 0xbb, &time);
}
