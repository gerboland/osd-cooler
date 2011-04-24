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
 * RTC management routines.
 *
 * REVISION:
 * 
 *
 * 2) Added CoolOpenRTC function for irrtc split -------------- 2006-12-20 nerochiaro
 * 1) Initial creation. ----------------------------------- 2006-07-20 MG
 *
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <linux/neuros_rtc.h>

//#define OSD_DBG_MSG
#include "nc-err.h"

#include "nc-config.h"
#include "nc-type.h"
#include "rtc.h"

/** Open RTC device
 */
int CoolOpenRTC(void)
{
	static int fd = 0;

	if (!fd)
	{
		fd = open(RTC_DEV, O_RDWR);
		if (fd < 0)
		{
			fd = 0;
			WARNLOG("error opening %s\n", RTC_DEV);
			return -1;
		}
	}
	return fd;
}

/** get current system time. */
int CoolGetTime(osd_time_t * otm)
{
	time_t timep;
	struct tm *p;

	time(&timep);
	p=localtime(&timep);
	otm->sec  = p->tm_sec;
	otm->min  = p->tm_min;
	otm->hour = p->tm_hour;
	otm->day  = p->tm_mday;
	otm->mon  = p->tm_mon+1;
	otm->year = p->tm_year+1900;

	return 0;
}

/** set system time. */
int CoolSetTime(osd_time_t * otm)
{
	struct timeval tv;
	struct timezone tz;
	time_t timep;
	struct tm tm;

	tm.tm_sec  = otm->sec;
	tm.tm_min  = otm->min;
	tm.tm_hour = otm->hour;
	tm.tm_mday = otm->day;
	tm.tm_mon  = otm->mon-1;
	tm.tm_year = otm->year-1900;
	timep = mktime(&tm);	

	gettimeofday(&tv,&tz);
	tv.tv_sec = timep;
	return settimeofday(&tv,&tz);
}

/** get RTC time. */
int CoolGetRTCtime(osd_time_t * otm)
{
	struct rtc_time_type rtc_time;
	struct rtc_date_type rtc_date;
	int fd;

	fd = CoolOpenRTC();
	if (fd < 0) return -1;

	if (ioctl(fd, RRB_GET_TIME, &rtc_time) != 0)
	{
		WARNLOG("ERROR: getting time\n");
		return -2;
	}

	if (ioctl(fd, RRB_GET_DATE, &rtc_date) != 0)
	{
		WARNLOG("ERROR: getting date\n");
		return -3;
	}

	otm->sec  = rtc_time.sec;
	otm->min  = rtc_time.min;
	otm->hour = rtc_time.hour;
	otm->day = rtc_date.day;
	otm->mon  = rtc_date.month;
	otm->year = rtc_date.year+1900;

	return 0;
}

/** set RTC time. */
int CoolSetRTCtime(osd_time_t * otm)
{
	struct rtc_time_type rtc_time;
	struct rtc_date_type rtc_date;
	int fd;

	fd = CoolOpenRTC();
	if (fd < 0) return -1;

	rtc_time.sec   = otm->sec;
	rtc_time.min   = otm->min;
	rtc_time.hour  = otm->hour;
	rtc_date.day   = otm->day;
	rtc_date.month = otm->mon;
	rtc_date.year  = otm->year-1900;	
	
	if (ioctl(fd, RRB_SET_TIME, &rtc_time) != 0)
	{
		WARNLOG("ERROR: setting time\n");
		return -2;
	}

	if (ioctl(fd, RRB_SET_DATE, &rtc_date) != 0)
	{
		WARNLOG("ERROR: setting date\n");
		return -3;
	}

	return 0;
}
