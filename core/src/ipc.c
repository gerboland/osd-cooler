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
 * Generic IPC routines.
 *
 * REVISION:
 * 
 * 3) Signal caught while sem blocking is not fatal.------- 2007-05-30 MG
 * 2) Complete rewrite to avoid use of shmem and simplify.- 2007-05-25 nerochiaro
 * 1) Initial creation. ----------------------------------- 2007-04-29 MG
 *
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>  
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "ipc-helper.h"

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

/**
 * Creates or retrieves a single semaphore with specified @path + @id .
 * If the semaphore didn't already exist, it is initialized to the specified @initial value.
 *
 * Note that applications should coordinate between themselves how to use the same @path + @id 
 * to access the same semaphore, this is not part of the semaphores API.
 *
 * Warning: this operation is not atomic. It's a two- or three- step operation and if the thread
 * is scheduled out between operations the semaphore can be found in an inconsistent state by other
 * threads.
 *
 * @param path
 *        Path to an existing file that is used to generate an unique key for the semaphore.
 * @param id
 *        A single non-zero value also used to generate the key (note: it's an int but the value should fit in a char).
 * @param initial
 *        Initial value of the semaphore if not existing. Must be zero or more.
 * @param sem
 *        Upon return this will contain the semaphore handle if successful, 0 otherwise.
 * @return
 *        0 if semaphore didn't exist and it was created and initialized. 
 *        1 if the semaphore existed and it was just opened.
 *        -1 for general failure.
 */
int CoolSemCreate(const char * path, int id, int initial, int *sem)
{
    /* Implementation details:

    Note that this function assumes that if path doesn not exist, then the semaphore needs to be
    created and initialized. But if path already exist, it doesn't mean the semaphore exists too.
    We will first try to see if the semaphore already exists, and if not we create and initialize it. 
    Otherwise we will just open the existing one without changing the resource value.

    Also note that we operate on 1 single semaphore for each semid (which is actually a semaphore set,
    but we use just the first one).
    */

    key_t key;
    union semun arg;

    // gets the sem key from the file+id, the file may not exist.
    if ((key = ftok(path, id)) == -1) 
    {
        if (fopen(path, "w") == NULL) 
        {
            FATALLOG("Semaphore id file cannot be created.\n");
            *sem = 0;
            return -1;
        }
    }

    // We check if the semaphore already exists (will return -1 if it does) and
    // if it doesn't exist this operation creates it immediately.
    if ((*sem = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL)) == -1) 
    {
        if (errno == EEXIST) 
        {
            //The sem exists already, so we just open it and don't do any initialization.
            if ((*sem = semget(key, 1, 0666 )) == -1) 
            {
                FATALLOG("Opening of existing semaphore failed.\n");
                *sem = 0;
                return -1;
            }
            else 
            {
                DBGLOG("Semaphore already existing, it was just opened [ID: %d]\n", *sem);
                return 1;
            }
        } 
        else 
        {
            FATALLOG("Semaphore check/creation failed.\n");
            *sem = 0;
            return -1;
        }
    } 
    else
    {
        DBGLOG("The semaphore is [ID: %d]\n", *sem);

        //We just created a new semaphore which is not initialized. We initialize it here.
        //Due to the inherent limitation of SystemV semaphores, this has to be done in
        //this separate step, thus potentially allowing for some race condition issues.
        //There is nothing that can be done about it, but it's at least good to know it.
        
        arg.val = initial;
        if (semctl(*sem, 0, SETVAL, arg) == -1)
        {
            FATALLOG("Initialization of semaphore failed (semctl SETVAL)\n");
            //This is a last ditch attempt at cleanup. We can't do nothing even if it fails, so
            //we don't even check the result code.
            semctl(*sem, 0, IPC_RMID, arg);

            *sem = 0;
            return -1;
        }
    }

	DBGLOG("Semaphore created and initialized [ID: %d]\n", *sem);
    return 0;
}

/**
 * Retrieves a single semaphore with specified @path + @id, only if exists.
 *
 * Note that applications should coordinate between themselves how to use 
 * the same @path + @id to access the same semaphores, this is not part of the semaphores API
 *
 * This operation is atomic with respect to the sem acquisition (technically it's a two-step 
 * operation but even if the @path or the sem is deleted between operations, the result would 
 * be the same as if the same as if the operation was actually atomic).
 *
 * @param path
 *        Path to an existing file that is used to generate an unique key for the semaphore.
 * @param id
 *        A single non-zero char value (but not necessarily ASCII), also used to generate the key.
 * @param sem
 *        Upon return this will contain the semaphore handle if successful, 0 otherwise.
 * @return
 *        0 if successful, 1 if semaphore doesn't exist or -1 if there were errors.
 */
int CoolSemGet(const char * path, int id, int *sem)
{
    key_t key;

    DBGLOG("Retrieving existing semaphore [path: %s, id: %c]\n", path, id);

    // gets the sem key from the file+id, the file may not exist.
    if ((key = ftok(path, id)) == -1) 
    {
        //if the file doesn't exist, the semaphore for us doesn't exist also.
        DBGLOG("Semaphore doesn't exist (missing file)\n");
        *sem = 0;
        return 1;
    }

    // We check if the semaphore already exists (will return -1 if it does) and
    // if it doesn't exist this operation will not create it.
    if ((*sem = semget(key, 1, 0666)) == -1) 
    {
        if (errno == ENOENT) 
        {
            DBGLOG("Semaphore doesn't exist.\n");
            return 1;
        }
        else 
        {
            FATALLOG("Error while checking semaphore existence.\n");
            *sem = 0;
            return -1;
        }
    } 

    DBGLOG("Retrieved existing semaphore [ID: %d]\n", *sem);
    return 0;
}

/**
 * Destroy semaphore with given handle.
 * 
 * This operation is atomic.
 *
 * @param sem
 *        Semaphore handle created either by CoolSemCreate* or CoolSemGet
 * @return
 *        0 if successful, otherwise -1
 *
 */
int CoolSemDestroy(int sem)
{
    union semun arg;

    DBGLOG("Destroying semaphore [ID: %d]\n", sem);

    if (sem)
    {
        if (semctl(sem, 0, IPC_RMID, arg) == -1) 
        {
            FATALLOG("Destruction of semaphore %d failed (semctl IPC_RMID)\n", sem);
            return -1;
        }
    }

	DBGLOG("Semaphore destroyed [ID: %d]\n", sem);    
    return 0;
}

/** Sem increment. This operation has the meaning of releasing one unit of the resource.
 *
 *  This operation is atomic.
 *
 * @param sem
 *        Semaphore handle cretaed either by CoolSemCreate* or CoolSemGet
 * @return
 *        0 if successful, -1 otherwise.
 *
 */
int CoolSemUp(int sem)
{
    struct sembuf sb = {0,  1, 0};  /* set to release resource */

    DBGLOG("Releasing resource (UP) on semaphore [ID: %d]\n", sem);
	if (sem)
	{
		if (semop(sem, &sb, 1) == -1) 
		{
			FATALLOG("Release of resource failed (semop UP 1)\n");
            return -1;
		}
	}

	DBGLOG("Resource released on semaphore [ID: %d]\n", sem);
    return 0;
}

/** Sem set value.
 * This operation resets the resource, use with CAUTION as it may break the UP/DOWN
 * sem operation pair. 
 *
 * This operation is atomic.
 *
 * @param sem
 *        Semaphore handle cretaed either by CoolSemCreate* or CoolSemGet
 * @param val
 *        Resource value set to the semaphore.
 * @return
 *        0 if successful, -1 otherwise.
 *
 */
int CoolSemSetValue(int sem, int val)
{
	union semun arg;

	DBGLOG("Setting resource on semaphore [ID: %d] to [value: %d]\n", sem, val);

	arg.val = val;
	if (semctl(sem, 0, SETVAL, arg) == -1)
	{
		FATALLOG("Setting resource failed (semctl SETVAL val)\n");
		return -1;
	}

	DBGLOG("Resource set on semaphore [ID: %d] to [value: %d]\n", sem, val);
	return 0;
}

/** Sem get value.
 * This operation gets the current value of the resource. 
 *
 * This operation is atomic.
 *
 * @param sem
 *        Semaphore handle cretaed either by CoolSemCreate* or CoolSemGet
 * @param val
 *        Will be filled with value on return, if successful.
 * @return
 *        0 if successful, -1 otherwise.
 *
 */
int CoolSemGetValue(int sem, int *val)
{
	DBGLOG("Getting resource value on semaphore [ID: %d]\n", sem);

	*val = semctl(sem, 0, GETVAL, 0);
	if (*val == -1)
	{
		FATALLOG("Getting resource value failed (semctl SETVAL val)\n");
		return -1;
	}

	DBGLOG("Resource get value on semaphore [ID: %d] has [value: %d]\n", sem, *val);
	return 0;
}

/** Sem increment (binary).
 * This operation shall be called only on semaphores that shall have "binary" semantics. 
 * In other words, semaphores where the resource count can only be 0 or 1.
 *
 * This operation has the meaning of releasing one unit of the resource, but with the 
 * important distinction that is can be called any number of times without the resource value
 * ever raising above the limit of 1.
 *
 * Please use this function only when you are sure you really need binary semantics and you 
 * cannot obtain them with norma @CoolSemUp (in most cases you can do that).
 *
 * This operation is atomic.
 *
 * @param sem
 *        Semaphore handle cretaed either by CoolSemCreate* or CoolSemGet
 * @return
 *        0 if successful, -1 otherwise.
 *
 */
#if 0 // this has been defined to use CoolSemSetValue API,
      // function is left here for reference.
int CoolSemUpBinary(int sem)
{
	union semun arg;

	DBGLOG("Releasing resource (UP,binary) on semaphore [ID: %d]\n", sem);

	arg.val = 1;
	if (semctl(sem, 0, SETVAL, arg) == -1)
	{
		FATALLOG("Release of resource failed (semctl SETVAL 1)\n");
		return -1;
	}

	DBGLOG("Resource released on semaphore [ID: %d]\n", sem);
	return 0;
}
#endif


/** Sem decrement. This operation has the meaning of trying to acquire one unit of the resource.
 * If the resouce count is zero (depleted), this call will block until resource becomes available again.
 * If you don't want to block, pass 0 to @blocking and the call will return immediately and notify if the
 * resource was depleted or if it was acquired successfully.
 *
 *  This operation is atomic.
 *
 * @param sem
 *        Semaphore handle created either by @CoolSemCreate or @CoolSemCreateBinary.
 * @param blocking
 *        1 to block caller, 0 to indicate non-block operation.
 * @return
 *        0 if acquired successfully. 
 *        1 if resource depleted (when non @blocking) or if semaphore destroyed while waiting (when @blocking). 
 *        -1 in case of failure.
 *        
 */
int CoolSemDown(int sem, int blocking)
{
    struct sembuf sb;
    int ret = -1;

    DBGLOG("Acquiring resource (DOWN) on semaphore [ID: %d, blocking:%d]\n", sem, blocking);
    //	sb = {0, -1, 0};  /* set to allocate resource */
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = (blocking)? 0:IPC_NOWAIT;

    if (sem)
    {
        ret = semop(sem, &sb, 1);
        if (ret == -1) 
        {
            switch (errno)
			{
			case EAGAIN:
                DBGLOG("Time limit reached while in DOWN-blocking on it [ID: %d]\n", sem);
                return 1;				
            case EINTR:
                DBGLOG("Signal caught while in DOWN-blocking on it [ID: %d]\n", sem);
                return 1;
            case EIDRM:
                DBGLOG("Semaphore was destroyed while in DOWN-blocking on it [ID: %d]\n", sem);
                return 1;
			default:
				FATALLOG("Acquisition of resource failed (semop DOWN 1) [errno: %d]\n", errno);
				return -1;
			}
        }
    }

	DBGLOG("Resource acquired on semaphore [ID: %d, blocking:%d]\n", sem, blocking);
    return 0;
}

#if 0 //FIXME: semtimedop not as part of ARM Linux??

/** Sem decrement with a timeout value. This operation has the meaning of trying to acquire one unit of the resource.
 * If the resouce count is zero (depleted), this call will block until resource becomes available again
 * or the specified timeout value times out.
 *
 *  This operation is atomic.
 *
 * @param sem
 *        Semaphore handle created either by @CoolSemCreate or @CoolSemCreateBinary.
 * @param ms
 *        time out value in mili-seconds.
 * @return
 *        0 if acquired successfully. 
 *        1 if resource depleted (when non @blocking) or if semaphore destroyed while waiting (when @blocking). 
 *        -1 in case of failure.
 *        
 */
int CoolSemTimedDown(int sem, int ms)
{
    struct sembuf sb;
    int ret = -1;
	struct timespec tm;
	unsigned long utm;

    DBGLOG("Acquiring resource (DOWN) on semaphore [ID: %d, blocking:%d]\n", sem, blocking);
    //	sb = {0, -1, 0};  /* set to allocate resource */
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = IPC_NOWAIT;

	utm = ms%1000000;
	tm.tv_sec = ms/1000000;
	tm.tv_nsec = utm*1000000;

    if (sem)
    {
        ret = (tm.tv_sec||tm.tv_nsec)? semtimedop(sem, &sb, 1, &tm):semop(sem, &sb, 1);
        if (ret == -1) 
        {
            switch (errno)
			{
			case EAGAIN:
                DBGLOG("Time limit reached while in DOWN-blocking on it [ID: %d]\n", sem);
                return 1;				
            case EINTR:
                DBGLOG("Signal caught while in DOWN-blocking on it [ID: %d]\n", sem);
                return 1;
            case EIDRM:
                DBGLOG("Semaphore was destroyed while in DOWN-blocking on it [ID: %d]\n", sem);
                return 1;
			default:
				FATALLOG("Acquisition of resource failed (semop DOWN 1) [errno: %d]\n", errno);
				return -1;
			}
        }
    }

	DBGLOG("Resource acquired on semaphore [ID: %d, blocking:%d]\n", sem, blocking);
    return 0;
}
#endif
