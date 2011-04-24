/*
 *  Copyright(C) 2007 Neuros Technology International LLC. 
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
 * System routines.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2007-01-20 MG
 * 2) Add two APIs ---------------------------------------- 2007-12-04 TQ
 *
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <linux/neuros_generic.h>
#include "nc-system.h"
//#define OSD_DBG_MSG
#include "nc-err.h"

/**
 * Set ARM system clock - EXPERIMENTAL
 * Clock will be rounded to the closest system acceptable range.
 * @param clock
 *       target system clock in MHz.
 *
 */
void CoolSetARMclock(int clock)
{
	int fd;
	fd = open("/dev/neuros_generic", O_RDWR);
	if (fd < 0) 
	{
		WARNLOG("unable to open neuros_generic driver.\n");
	}
	else
	{
		ioctl( fd, NT_GENERIC_SET_ARM_CLOCK, &clock);
		close(fd);
	}
}


/**
 * Function: CoolIsNandFlash
 * Description: detect if the board is nand flash board 
 * parameters: None
 * retrun: 1 is nand flash board
 *
 */
int CoolIsNandFlash(void)
{
     int status=0;

     /* Tried CoolRunCommand("cat","/proc/mtd" "|", "grep", "NAND", NULL), 
      *	but it does not work, might be due to pipe support??" 
      */
     status = system("cat /proc/mtd | grep NAND");
     if (status == 0) return 1;
     else return 0;
}

/**
 * Function: CoolIsNorFlash
 * Description: detect if the board is nor flash board 
 * parameters: None
 * retrun: 1 is nor flash board
 *
 */
int CoolIsNorFlash(void)
{
     int status;
     
     /* Tried CoolRunCommand("cat","/proc/mtd" "|", "grep", "NOR", NULL), 
      *	but it does not work, might be due to pipe support??" 
      */
     status = system("cat /proc/mtd | grep NOR");
     if (status == 0) return 1;
     else return 0;
}
