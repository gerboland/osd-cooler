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
 * Generic shared memory routines.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2007-02-24 MG
 *
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>  
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "shmem.h"
#include "list.h"
#include "stringutil.h"

#define _ID_LEN 32
#define SHARED_MEM_MAGIC 0x20070515

typedef struct
{
	int        magic;
	int        refCount;
	shm_lock_t lock;
} shm_ctl_t;


//NOTE: shm_ctl_t needs to be 128 bit aligned.
typedef union
{
	shm_ctl_t  ctrl;
	int        dummy[4*((sizeof(shm_ctl_t)+3)/4)];
} shm_ctl_ut;


typedef struct
{
	int    refCount;
	int    fd;
	void * shm;
	int    bytes;
	char   id[_ID_LEN];
} shm_reg_t;

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
	int val;                  /* value for SETVAL */
	struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
	unsigned short *array;    /* array for GETALL, SETALL */
	/* Linux specific part: */
	struct seminfo *__buf;    /* buffer for IPC_INFO */
};
#endif


typedef struct {
	void *       buf;  // shmem addr
	shm_lock_t * lock; // shmem lock       
    int          bytes; // shmem size, can be used to do error
    char *       id;
}shm_internal_handle_t;

/* per process data. */
static slist_t * shmHead = NULL;
static slist_t * shmTail = NULL;
#define _TOK_LEN (128) /* 128 is more than enough. */


/**
 * Get shared memory with given size.
 *
 * @param id
 *        shared memory ID string, only first 15 characters are significant.
 * @param bytes
 *        shared memory size in bytes.
 * @param creator
 *        returned status to indicate whether this is a shared mem creator.
 * @param lock
 *        returned shared mem lock object pointer.
 * @return
 *        pointer to shared memory, visible in current process address 
 *        space if successful, NULL otherwise.
 *        creator = 1: creator will have this flag set when returning.
 *
 */
//WARNING: API is currently NOT threadsafe.
void * CoolShmGet(const char * id, int bytes, int * creator, shm_lock_t ** lock)
{
	char tok[_TOK_LEN]; 
	char idb[_ID_LEN];
	int found;
	slist_t * list;
	shm_reg_t * reg;	
	shm_ctl_t * ctl;
	int retry;

	strlcpy(idb, id, sizeof(idb));

	bytes += sizeof(shm_ctl_ut); // extra bytes for internal tracking
	*creator = 0;
	found = 0;
	list = shmHead;
	// search if memory is already created.
	while (list)
	{
		reg = (shm_reg_t *)list->data;
		DBGLOG("registered id: %s\n looking for: %s\n", reg->id, idb);
		if (!strcmp(reg->id, idb)) 
		{
			found = 1;
			break;
		}
		else list = list->next;
	}

	if (!found)
	{
		int shm_fd;
		void * shm;

		// not found, lets fetch/create it.
		memset(tok, 0, _TOK_LEN);
		strlcpy(tok, STATE_FILE, sizeof(tok));
		strlcat(tok, idb, sizeof(tok));	

		if((shm_fd = shm_open(tok, (O_CREAT|O_EXCL|O_RDWR), (S_IREAD|S_IWRITE))) > 0 ) 
		{
			*creator = 1;
		}
		else if((shm_fd = shm_open(tok, (O_CREAT|O_RDWR), (S_IREAD|S_IWRITE))) < 0) 
		{
			DBGLOG("Could not create shm object.\n");
			return NULL;
		} 

		/* Set the size of the SHM to be the size of the struct. */
		ftruncate(shm_fd, bytes);
		
		/* Connect the conf pointer to set to the shared memory area,
		 * with desired permissions 
		 */
		if((shm =  mmap(0, bytes, (PROT_READ|PROT_WRITE), 
						MAP_SHARED, shm_fd, 0)) == MAP_FAILED) 
		{
			DBGLOG("Could not map share-mem.\n");
			return NULL;
		}
		DBGLOG("opened shared memory [shm:%p] [bytes:%d]\n",shm, bytes);	


		// fetch/create successful, record it.		
		reg = (shm_reg_t *)calloc(1, sizeof(shm_reg_t));
		strlcpy(reg->id, idb, sizeof(reg->id));

		reg->shm = shm + sizeof(shm_ctl_ut);
		reg->bytes = bytes;
		reg->refCount = 1;
		reg->fd = shm_fd;
		DBGLOG("opened shared memory [fd: %d] [shm:%p]\n", reg->fd, reg->shm);

		list = CoolSlistNew((void *)reg);
		DBGLOG("list = %p\n", list);
		if (shmTail) shmTail = CoolSlistInsert(shmTail, list);
		else {shmHead = shmTail = list;}
	}		
	else reg->refCount++;

	ctl = (shm_ctl_t*)(reg->shm - sizeof(shm_ctl_ut));
	
	DBGLOG("creating lock obj\n");
	// create lock object.
	if (*creator)
	{
		key_t key;
		int semid;
		union semun arg;

		DBGLOG("creating key for tok: %s\n", tok);
		if ((key = ftok(tok, 'x')) == -1) 
		{
			FATALLOG("token creation failed.\n");
		}

		DBGLOG("creating semaphore.\n");
		/* create a semaphore set with 2 semaphore: */
		if ((semid = semget(key, 2, 0666 | IPC_CREAT)) == -1) 
		{
			FATALLOG("semget failed.\n");
		}

		DBGLOG("semaphore created:= %d\n", semid);
		/* initialize semaphores 1: */
		arg.val = 1;
		if ((semctl(semid, 0, SETVAL, arg) == -1) ||
			(semctl(semid, 1, SETVAL, arg) == -1) )
		{
			FATALLOG("semctl failed.\n");
		}		
		ctl->lock.semid = semid;
		ctl->lock.rc = 0;
		ctl->refCount = 1;
		//now, validate the shared memory.
		ctl->magic = SHARED_MEM_MAGIC;
	}

	retry = 10; // wait for 10 seconds before reporting error.
	while ((retry--)&&(SHARED_MEM_MAGIC != ctl->magic))
	{
		//possibly two processes compete to create the shared memory.
		//very unlikely, however possible, let's wait for a bit.

		WARNLOG("shared memory in inconsistent state, waiting to retry...!\n");
		sleep(1);
	}

	if (!retry)
	{
		WARNLOG("shared memory in inconsistent state.!\n");

		//FIXME: release objects!!
		return NULL;
	}

	if (lock) *lock = &ctl->lock;

	// update global ref counter.
	// apply the lock itself, indeed we are accessing shared memory here as well.
	if (!(*creator))
	{
		CoolShmWriteLock(&ctl->lock);
		ctl->refCount++;
		CoolShmWriteUnlock(&ctl->lock);
	}

	DBGLOG("shared memory [ID: %s] ref count = %d/%d\n", id, reg->refCount, ctl->refCount);
	return reg->shm;
}


/**
 * put back shared memory with given ID.
 *
 * @param id
 *        shared memory ID string.
 *
 */
//WARNING: API is currently NOT threadsafe.
void CoolShmPut(const char * id)
{
	int found;
	slist_t * list;
	shm_reg_t * reg;	
	shm_ctl_t * ctl;
	char idb[_ID_LEN];
	int refCount;

	strlcpy(idb, id, sizeof(idb));

	found = 0;
	list = shmHead;
	// search if memory is already created.
	while (list)
	{
		reg = (shm_reg_t *)list->data;
		DBGLOG("registered id: %s\n looking for: %s\n", reg->id, idb);
		if (!strcmp(reg->id, idb)) 
		{
			found = 1;
			break;
		}
		else list = list->next;
	}

	if (!found)
	{
		WARNLOG("Shared memory [ID: %s] not found!\n", idb);
		return;
	}

	ctl = (shm_ctl_t*)(reg->shm - sizeof(shm_ctl_ut));

	reg->refCount--;
	DBGLOG("shared memory [ID: %s] ref count = %d/%x\n", id, reg->refCount, ctl->refCount);
	if (!reg->refCount)
	{
		DBGLOG("closing shared memory [ID: %s]\n", id);
		close(reg->fd);

		// is this the last reference?
		// check global ref counter.
		// apply the lock itself, indeed we are accessing shared memory here as well.
		CoolShmReadLock(&ctl->lock);
		refCount = ctl->refCount;
		CoolShmReadUnlock(&ctl->lock);

		if (1 == refCount) //last reference.
		{
			char tok[_TOK_LEN]; 
			union semun arg;

			DBGLOG("releasing shared memory [ID: %s]\n", id);

			// remove shared memory semaphore set.
			if (semctl(ctl->lock.semid, 0, IPC_RMID, arg) == -1) 
			{
				switch(errno)
				{
				case EACCES: DBGLOG("EACCS\n"); break;
				case EFAULT: DBGLOG("EFAULT\n"); break;
				case EIDRM:  DBGLOG("EIDRM\n"); break;
				case EINVAL: DBGLOG("EINVAL\n"); break;
				case EPERM:  DBGLOG("EPERM\n"); break;
				case ERANGE: DBGLOG("ERANGE\n"); break;
				}
				FATALLOG("semctl removal failed.\n");
			}

			strlcpy(tok, STATE_FILE, sizeof(tok));
			strlcat(tok, idb, sizeof(tok));
			DBGLOG("unlink: %s\n", tok);
			if (shm_unlink(tok))
			{
				switch(errno)
				{
				case EACCES:       DBGLOG("EACCS\n"); break;
				case EPERM:        DBGLOG("EPERM\n"); break;
				case EFAULT:       DBGLOG("EFAULT\n"); break;
				case EISDIR:       DBGLOG("EISDIR\n"); break;
				case EBUSY:        DBGLOG("EBUSY\n"); break;
				case EINVAL:       DBGLOG("EINVAL\n"); break;
				case ENAMETOOLONG: DBGLOG("ENAMETOOLONG\n"); break;
				case ENOENT:      DBGLOG("ENOENT\n"); break;					
				case ENOTDIR:      DBGLOG("ENOTDIR\n"); break;
				case ENOMEM:       DBGLOG("ENOMEM\n"); break;
				case EROFS:        DBGLOG("EROFS\n"); break;
				case ELOOP:        DBGLOG("ELOOP\n"); break;
				case EIO:          DBGLOG("EIO\n"); break;
				}
			}
		}

		// unmapping 
		munmap(reg->shm-sizeof(shm_ctl_ut), reg->bytes);

		free(reg);
		DBGLOG("removing shm registration\n");

		shmHead = CoolSlistRemove(shmHead, list);
		if (NULL == shmHead) shmTail = NULL;
		else shmTail = CoolSlistTail(shmHead);
	}
	else
	{
		// update global ref counter.
		// apply the lock itself, indeed we are accessing shared memory here as well.
		CoolShmWriteLock(&ctl->lock);
		ctl->refCount--;
		CoolShmWriteUnlock(&ctl->lock);
	}
}


/** shared memory reader lock.
 *  Locks only when writer is busy.
 *
 * @param lock
 *        shared memory lock object pointer.
 */
void CoolShmReadLock(shm_lock_t * lock)
{
	/* reader algorithm:
	   down(&rdsem);
	   rc=rc+1;
	   If (rc==1) down(&wrsem);
	   up(&rdsem);
	   Read_data();	   
	   Down(&rdsem);
	   rc=rc-1;
	   If(rc==0) up(&wrsem);
	   up(&rdsem);
	*/
    struct sembuf rdsb = {0, -1, 0};  /* set to allocate resource */
    struct sembuf wrsb = {1, -1, 0};  /* set to allocate resource */

	if (semop(lock->semid, &rdsb, 1) == -1) 
	{
		FATALLOG("semop failed.\n");
    }
	
	lock->rc++;

	if (lock->rc == 1)
	{
		if (semop(lock->semid, &wrsb, 1) == -1) 
		{
			FATALLOG("semop failed.\n");
		}
	}

	rdsb.sem_op = 1;
	if (semop(lock->semid, &rdsb, 1) == -1) 
	{
		FATALLOG("semop failed.\n");
    }
}

/** shared memory reader unlock.
 *
 * @param lock
 *        shared memory lock object pointer.
 */
void CoolShmReadUnlock(shm_lock_t * lock)
{
    struct sembuf rdsb = {0, -1, 0};  /* set to allocate resource */
    struct sembuf wrsb = {1,  1, 0};  /* set to release resource */

	if (semop(lock->semid, &rdsb, 1) == -1) 
	{
		FATALLOG("semop failed.\n");
    }
	
	lock->rc--;

	if (lock->rc == 0)
	{
		if (semop(lock->semid, &wrsb, 1) == -1) 
		{
			FATALLOG("semop failed.\n");
		}
	}

	rdsb.sem_op = 1;
	if (semop(lock->semid, &rdsb, 1) == -1) 
	{
		FATALLOG("semop failed.\n");
    }
}


/** shared memory write lock.
 *  Locks when reader or another writer is in progress.
 *
 * @param lock
 *        shared memory lock object pointer.
 */
void CoolShmWriteLock(shm_lock_t * lock)
{
	/* writer algorithm:
	   down(&wrsem);
	   write_data();
	   up(&wrsem);
	*/
    struct sembuf wrsb = {1, -1, 0};  /* set to allocate resource */

	if (semop(lock->semid, &wrsb, 1) == -1) 
	{
		FATALLOG("semop failed.\n");
    }
}


/** shared memory write unlock.
 *
 * @param lock
 *        shared memory lock object pointer.
 */
void CoolShmWriteUnlock(shm_lock_t * lock)
{
    struct sembuf wrsb = {1,  1, 0};  /* set to release resource */

	if (semop(lock->semid, &wrsb, 1) == -1) 
	{
		FATALLOG("semop failed.\n");
    }
}


/**
 * Get shared memory with given size.
 * @param handle
 *        returned shared mem handle.
 * @param id
 *        shared memory ID string, only first 15 characters are significant.
 * @param bytes
 *        shared memory size in bytes.
 * @return
 *       -1      faile
 *        0       shared memory has exist
 *        1       first creat shared memory
 *
 */ 
int
CoolShmHelperOpen(shm_handle_t* handle,const char *id, int bytes)
{
	int first;
	shm_internal_handle_t * smhandle;
	smhandle = (shm_internal_handle_t *)malloc(sizeof(shm_internal_handle_t));
	memset(smhandle, 0, sizeof(shm_internal_handle_t)); 
	smhandle->buf = CoolShmGet(id, bytes, &first, &smhandle->lock);

	if (!smhandle->buf) 
	{
		WARNLOG("Unable to fetch shared memory!\n");
		goto bail_no_shm;
	}
	smhandle->id = strdup(id);
	smhandle->bytes = bytes;
	handle->shm = smhandle;
	if (first)
	{
		return 1;
	}
	return 0;
	bail_no_shm:
	return -1;
}



void           
CoolShmHelperClose(shm_handle_t* handle)
{
	shm_internal_handle_t* tmp = (shm_internal_handle_t *)handle->shm;
	if(!tmp) return;
	handle->shm = NULL;
	CoolShmPut(tmp->id);
	free(tmp->id);
	free(tmp);
}



/**
 * Get shared memory with given size.
 * @param handle
 *        shared mem handle.
 * @param dat
 *        return to the readed data.
 * @param bytes
 *        How many bytes to read
 * @param offset
 *        the offset in shared memory start to read.
 * @return
 *     value = -1      handle illegal;
 *     value = -2      offset illegal;
 *	 value >  0       the bytes read;
 *
 */ 

int            
CoolShmHelperRead(shm_handle_t* handle,void *dat,int offset, int bytes)
{
	int nspace;
	shm_internal_handle_t* tmp = (shm_internal_handle_t *)handle->shm;
	if(!tmp )
		return -1;
	if(offset <0 || offset >= tmp->bytes) 
		return -2;

	nspace = tmp->bytes - offset;
	nspace = (nspace > bytes) ? bytes : nspace;

	CoolShmReadLock(tmp->lock);
	memcpy(dat ,tmp->buf+offset, nspace);
	CoolShmReadUnlock(tmp->lock);
	return nspace;

}

/**
 * Get shared memory with given size.
 * @param handle
 *        shared mem handle.
 * @param dat
 *        point to the data that will write.
 * @param bytes
 *        write size in bytes .
 * @param offset
 *        the offset in share memory start to write.
 * @return
 *     value = -1      handle illegal;
 *     value = -2      offset illegal;
 *	 value >  0       the bytes  written;
 *
 */ 

int            
CoolShmHelperWrite(shm_handle_t* handle,void *dat, int offset, int bytes)
{
	int nspace;
	shm_internal_handle_t* tmp = (shm_internal_handle_t *)handle->shm;
	if(!tmp )
		return -1;
	if(offset <0 || offset >= tmp->bytes) 
		return -2;
	
	nspace = tmp->bytes - offset;
	nspace = (nspace > bytes) ? bytes : nspace;
	CoolShmWriteLock(tmp->lock);
	memcpy(tmp->buf+offset, dat, nspace);
	CoolShmWriteUnlock(tmp->lock);
	return nspace;

}




