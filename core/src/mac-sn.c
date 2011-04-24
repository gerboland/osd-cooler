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
 * Mac/SN management routines.
 *
 * REVISION:
 * 
 *
 * 2) Fixed devices and function calls for irrtc split ---- 2006-12-20 nerochiaro
 * 1) Initial creation. ----------------------------------- 2006-07-29 MG
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
#include "mac-sn.h"

/** return Mac address string. */
const char * CoolGetOSDmac(void)
{
	static char mac[20];
	int fd = CoolOpenRTC();
	
	strcpy(mac, "00:00:00:00:00:00");
	if (fd > 0)
	{
		//fetch mac address
		ioctl(fd, RRB_GET_MAC, mac);
	}
	return mac;
}

/** return serial number string. */
const char * CoolGetOSDsn(void)
{
	static char sn[20];
	int fd = CoolOpenRTC();
	
	strcpy(sn, "A00000000000");
	if (fd > 0)
	{
		//fetch serial number.
		ioctl(fd, RRB_GET_SN, sn);
	}
	return sn;
}

const char * CoolGetHWversion(void)
{
    static char ver[10];
    int fd = CoolOpenRTC();
    
    strcpy(ver, "00.00");
    if(fd > 0)
      {
	ioctl(fd, RRB_GET_VERSION, ver);
      }
    return ver;
}
