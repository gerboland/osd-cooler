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
 * simple raw keyboard polling routines.
 *
 * REVISION:
 * 
 * 1) Initial creation. ----------------------------------- 2007-11-22 MG
 *
 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "kbd.h"

//#define OSD_DBG_MSG
#include "nc-err.h"

#define IR_DEV "/dev/neuros_ir"
static int lastkey;

static int openIr(void)
{
     int fd;
     fd = open(IR_DEV, O_RDWR);   
     return fd;
}

static void closeIr(int fd)
{
    close(fd);
}

/** polling keyboard (IR remote control) in a constant polling rate.
 *
 * Polling assume keyboard (IR remote control) is already debounced.

 * @return
 *        current active button.
 *
 */
RAW_BUTTON CoolKbdPolling( void )
{
	RAW_BUTTON btn =  RAW_BTN_null;
	int fd;
	int key, status;

	// polling kbd every 50ms is good enough
	usleep(50000);

	fd = openIr();
	status = read(fd, &key, sizeof(int));
	if(status == 4)
	{
	     if(key == 0) //keyup
	     {
		  if(lastkey != 0)
		  {
		       btn = lastkey;
		       lastkey = key;
		  }
	     }
	     else  //keydown
	     {
		  lastkey = key;
		  btn =  RAW_BTN_null;
	     }
	}

	closeIr(fd);
	return btn;
}
