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
 * LED display management routines.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2006-07-31 MG
 *
 */
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <linux/leds.h>

#include "nc-type.h"
#include "nc-err.h"
#include "led-control.h"

static int open_fd(void)
{
    static int fd = 0;
	if (0 == fd)
	  {
		fd = open("/dev/it_led", O_RDWR);
		if (fd < 0) 
		  {
			WARNLOG("unable to open LED control.\n");
			return -1;
		  }
	  }
	return fd;
}

/** LED controls.
 *
 * @param color
 *        led color control.
 * @param onoff
 *        led on off control, TRUE: on, FALSE: off
 *
 */
void CoolLEDcontrol(int color, int onoff)
{
    int fd;
	fd = open_fd();
	if (fd < 0) return;

	switch (color)
	  {
	  case LED_GREEN:
	        //onoff logic
	        if(!onoff)
		  {
		    ioctl(fd, IT_IOCLED_OFF, IT_LED_1);
		    ioctl(fd, IT_IOCLED_OFF, IT_LED_2);
		  }
		else
		  {
		    ioctl(fd, IT_IOCLED_OFF, IT_LED_1);
		    ioctl(fd, IT_IOCLED_ON, IT_LED_2);
		  }
		break;
	  case LED_RED:
		// onoff logic
	        if(!onoff)
		  {
		    ioctl(fd, IT_IOCLED_OFF, IT_LED_1);
		    ioctl(fd, IT_IOCLED_OFF, IT_LED_2);
		  }
		else
		  {
		    ioctl(fd, IT_IOCLED_ON, IT_LED_1);
		    ioctl(fd, IT_IOCLED_OFF, IT_LED_2);
		  }
		break;
	  case LED_ALL:
		//onoff logic
	        if(!onoff)
		  {
		    ioctl(fd, IT_IOCLED_OFF, IT_LED_1);
		    ioctl(fd, IT_IOCLED_OFF, IT_LED_2);
		  }
		else
		  {
		    ioctl(fd, IT_IOCLED_ON, IT_LED_1);
		    ioctl(fd, IT_IOCLED_ON, IT_LED_2);
		  }
		break;
	  default:
		break;
	  }
}

/** blink LED at specified interval for roughly duration time. 
 *
 * @param color
 *        led color control.
 * @param interval
 *        led blink interval in ms
 * @param duration
 *        led blink duration in ms.
 */
void CoolLEDblink(int color, int interval, int duration)
{
    int fd;
	int dd = 0;

	fd = open_fd();
	if (fd < 0) return;

	while (dd < duration)
	  {
		CoolLEDcontrol(color, TRUE);
		usleep(interval*1000);
		CoolLEDcontrol(color, FALSE);
		usleep(interval*1000);
		dd += interval*2;
	  }
}
