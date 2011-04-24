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
 * Watchdog management routines.
 *
 * REVISION:
 * 
 * 2) Stop dog before closing dog. ------------------------ 2006-11-30 nerochiaro
 * 1) Initial creation. ----------------------------------- 2006-09-12 MG
 *
 */
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <linux/watchdog.h>

#define OSD_DBG_MSG
#include "nc-err.h"

#include "nc-config.h"
#include "nc-type.h"
#include "watch-dog.h"


static int fd;
static int dogSleepTime;
static pthread_t dogThread = (pthread_t)NULL;

static void *
kickDog (void * arg)
{
	while(fd)
	{
		DBGLOG("kicking dog...\n");
		write(fd,"\0",1);
		fsync(fd);
		sleep(dogSleepTime/2);
	}
	return NULL;
}


/** start the watchdog. 
 *
 * @param sec
 *        dog sleep time in seconds.
 */
void
CoolWatchdogStart(int sec)
{
	if (!fd)
	{
		fd = open("/dev/softdog",O_WRONLY);
		
		ioctl(fd, WDIOC_SETTIMEOUT, &sec);
		if(fd==-1)
		{
			WARNLOG("Unable to open watchdog!\n");
		}
	}
	// minimum dog sleep time: 8 seconds.
	if (sec < 8) sec = 8;
	dogSleepTime = sec;
	
	if (fd)
	{
		if (pthread_create(&dogThread, NULL, kickDog, NULL))
		{
			close(fd);
			fd = 0;
			WARNLOG("Unable to activate watchdog!\n");
		}
	}
}

/** stop watch dog. */
void
CoolWatchdogStop(void)
{
	// This properly disables the watchdog. It won't reset when we stop kicking it
	write(fd,"V",1);        
	
	close(fd);
	fd = 0;
	if (dogThread)
	{
		pthread_join(dogThread, NULL);
		dogThread = (pthread_t)NULL;
	}
}
