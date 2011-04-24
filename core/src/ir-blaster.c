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
 * IR remote and IR blaster management routines.
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
#include <linux/neuros_ir_blaster.h>

//#define OSD_DBG_MSG
#include "nc-err.h"

#include "nc-config.h"
#include "nc-type.h"
#include "ir-blaster.h"


/* Open IR blaster device */
static int blaster_open(void)
{
	static int fd = 0;
	
	if (!fd)
	{
		fd = open(IR_BLASTER_DEV, O_RDWR);
		if (fd < 0)
		{
			fd = 0;
			WARNLOG("error opening %s\n", IR_BLASTER_DEV);
			return -1;
		}
	}
	return fd;
}

/** Blaster out IR signal.
 *
 * @param manID
 *        manufacture ID.
 * @param devID
 *        target device ID
 * @param code
 *        output code
 * @param code length in bytes.
 *
 */
void 
CoolBlasterOut (int manID, int devID, const char * code, int codeLength)
{
    int fd;
    if((fd=blaster_open())<0) return;

  if (NULL == code)
	{
	  DBGLOG("blastering OSD test code.\n");
	  //send out Neuros OSD test code.
#ifdef BLASTER_THROUGH_ARM
	  struct blaster_data_pack bls_dat =	
		{.bitstimes=0x801A,
                 .mbits[0]=3,
                 .mbits[1]=0,
                 .mbits[2]=0,
                 .mbits[3]=2,
                 .dbits[0]=0x13,
                 .dbits[1]=0x45,
                 .dbits[2]=0x04,
                 .dbits[3]=0x02,
                 .bit0=600,
                 .bit1=1230,
                 .specbits[0]=2390,
                 .specbits[1]=600,
                 .specbits[2]=25660};
#else
	  struct blaster_data_pack bls_dat =	
		{.bitstimes=0x9A01,
                 .mbits[0]=3,
                 .mbits[1]=0,
                 .mbits[2]=0,
                 .mbits[3]=2,
                 .dbits[0]=0x13,
                 .dbits[1]=0x45,
                 .dbits[2]=0x04,
                 .dbits[3]=0x02,
                 .bit0=600,
                 .bit1=1230,
                 .specbits[0]=2390,
                 .specbits[1]=600,
                 .specbits[2]=25660};
#endif
	  ioctl(fd, RRB_FACTORY_TEST);
	  ioctl(fd, RRB_BLASTER_KEY, &bls_dat);
	}
}
