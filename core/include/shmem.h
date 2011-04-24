#ifndef SH_MEM__H
#define SH_MEM__H
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
 * Share memory module header.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2007-02-24 MG 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

// this is where shared memory descriptor file sits. */
#define STATE_FILE "/var/tmp/shm-" 

typedef struct {
	int semid;  //semaphore ID
	int rc;     //reference counter
} shm_lock_t;

typedef struct{
	void * shm;
}shm_handle_t;

void * CoolShmGet(const char * id, int bytes, int * creator, shm_lock_t ** lock);
void   CoolShmPut(const char * id);
void   CoolShmReadLock(shm_lock_t * lock);
void   CoolShmReadUnlock(shm_lock_t * lock);
void   CoolShmWriteLock(shm_lock_t * lock);
void   CoolShmWriteUnlock(shm_lock_t * lock);

int    CoolShmHelperOpen(shm_handle_t* handle,const char *id, int bytes);
int    CoolShmHelperRead(shm_handle_t* handle,void *dat,int offset, int bytes);
int    CoolShmHelperWrite(shm_handle_t* handle,void *dat, int offset, int bytes);
void   CoolShmHelperClose(shm_handle_t* handle);

#endif /* SH_MEM__H */
