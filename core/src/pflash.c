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
 * Program flash as key-value pair storage routines.
 *
 * REVISION:
 * 
 * 1) Initial creation (moved out of R3 UI).------------------ 2006-05-03 MG
 *
 */

#include "nc-err.h"
#include "pflash.h"
#include <sys/ioctl.h>

static int flash_fd = 0;


static int open_flash(void)
{
    if (!flash_fd)
	{
		flash_fd = open("/dev/pflash", O_RDWR);
		
		if(flash_fd < 0) 
		{
		    ERRLOG("Error opening /dev/pflash. Check kernel config\n");
			return -1;
		}
	}
	return flash_fd;
}

int
GetKeyValue(int sector,const char *name,char *defvalue)
{
#if 	BUILD_TARGET_ARM
    int fd = open_flash();
	env_para k_para;

	k_para.sector=sector;
	strcpy(k_para.name,name);
	strcpy(k_para.value,defvalue);

	if(ioctl(fd, READ_FLASH, &k_para) == -1 ) 
	{
		ERRLOG("Error reading pflash: %m\n");
		return -2;
	}

	//DBGLOG("%shh%d\n",k_para.value,strlen(k_para.value));
	strcpy(defvalue, k_para.value);
	return 1;
#endif
}

int
SetKeyValue(int sector,const char *name, const char *value)
{
#if 	BUILD_TARGET_ARM
    int fd = open_flash();
	env_para k_para;

	k_para.sector=sector;
	strcpy(k_para.name,name);
	strcpy(k_para.value,value);

	if(ioctl(fd, WRITE_FLASH, &k_para) == -1 ) 
	{
		ERRLOG("Error reading pflash: %m\n");
		return -2;
	}
	
	return 1;
#endif
}

/** write to flash. */
int
SaveKeyValue(int sector)
{
#if 	BUILD_TARGET_ARM
    int fd = open_flash();
	env_para k_para;

	k_para.sector=sector;

	if(ioctl(fd, SAVE_FLASH, &k_para) == -1 ) 
	{
		ERRLOG("Error save pflash: %m\n");
		return -2;
	}
	
	return 1;
#endif
}
