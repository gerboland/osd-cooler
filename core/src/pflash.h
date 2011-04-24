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
 * flash hook routines header.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2006-05-03 MG
 *
 */

#ifndef PFLASH__H
#define PFLASH__H
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct env_para_t
{
  int sector;
  char name[20];
  char value[200];
}env_para;

#define WRITE_FLASH  0
#define READ_FLASH   1
#define SAVE_FLASH   2

int
GetKeyValue(int sector,const char *name,char *defvalue);
int
SetKeyValue(int sector,const char *name, const char *value);
int
SaveKeyValue(int sector);

#endif //PFLASH__H

