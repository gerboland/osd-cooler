#ifndef IPC_HELPER__H
#define IPC_HELPER__H
/*
 *  Copyright(C) 2007 Neuros Technology International LLC. 
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License.
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
 * OSD IPC module header.
 *
 * REVISION:
 * 
 * 
 * 2) Complete rewrite to avoid use of shmem and simplify.- 2007-05-23 nerochiaro
 * 1) Initial creation. ----------------------------------- 2007-04-29 MG 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

int     CoolSemCreate(const char * path, int id, int initial, int *sem);
#define CoolSemCreateBinary(path, id, sem) CoolSemCreate(path, id, 1, sem)
int     CoolSemGet(const char * path, int id, int *sem);
int     CoolSemDestroy(int sem);
int     CoolSemUp(int sem);
int     CoolSemDown(int sem, int block);
int     CoolSemGetValue(int sem, int *val);
int     CoolSemSetValue(int sem, int val);
#define CoolSemUpBinary(sem) CoolSemSetValue(sem, 1)
#define CoolSemDeplete(sem)    CoolSemSetValue(sem, 0)

#endif /* IPC_HELPER__H */
