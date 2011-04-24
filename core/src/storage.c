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
 * Memory storage management routines.
 *
 * REVISION:
 * 
 * 2) Implement storagestat function ---------------------- 2006-07-25 EY
 * 1) Initial creation. ----------------------------------- 2006-07-20 MG
 *
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
#include <mntent.h>

#include "nc-type.h"
#include "nc-err.h"
#include "i18n.h"
#include "storage.h"


#if 0
#define DBGLOG(fmt, arg...) DPRINT(fmt, ##arg)
#else
#define DBGLOG(fmt, arg...) do {;}while(0)
#endif

#if 1
const char *
CoolCreateReadableSizeString(unsigned long long size,
	unsigned long block_size, unsigned long display_unit)
{
	/* The code will adjust for additional (appended) units. */
	static const char zero_and_units[] = { '0', 0, 'K', 'M', 'G', 'T' };
	static const char fmt[] = "%Lu";
	static const char fmt_tenths[] = "%Lu.%d %c";

	static char str[21];		/* Sufficient for 64 bit unsigned integers. */

	unsigned long long val;
	int frac;
	const char *u;
	const char *f;

	u = zero_and_units;
	f = fmt;
	frac = 0;

	val = size * block_size;
	if (val == 0) {
		return u;
	}

	if (display_unit) {
		val += display_unit/2;	/* Deal with rounding. */
		val /= display_unit;	/* Don't combine with the line above!!! */
	} else {
		++u;
		while ((val >= KILOBYTE)
			   && (u < zero_and_units + sizeof(zero_and_units) - 1)) {
			f = fmt_tenths;
			++u;
			frac = ((((int)(val % KILOBYTE)) * 10) + (KILOBYTE/2)) / KILOBYTE;
			val /= KILOBYTE;
		}
		if (frac >= 10) {		
			++val;
			frac = 0;
		}

	}


	snprintf(str, sizeof(str), f, val, frac, *u);

	return str;
}
#endif

/** 
 * Enumerate the mount points and calls a user-defined function on all of them
 * @param cb A function that will be called for each mount point with arguments:
 *           - the mount point path
 *           - a pointer to arbitrary user data passed along from this function's arguments.
 *           This function shall return 1 when it wants to stop the iteration.
 * @param user_data It will be passes as-is to the callback function.
 * @return 1 if the callback returned 1, -1 on errors, 0 otherwise.
 */
int CoolStorageEnumerate(storage_enum_func cb, void* user_data)
{
	FILE *mount_table;
	struct mntent *mount_entry;

	if (!(mount_table = setmntent("/proc/mounts", "r")))
	{
		WARNLOG("Unable to read /proc/mounts: %s.\n", strerror(errno));
		return -1;
	}

	do {
		mount_entry = getmntent(mount_table);
		if (mount_entry == NULL) {
			endmntent(mount_table);
			break;
		}

		if (cb != NULL) {
			if (cb(mount_entry->mnt_dir, user_data) == 1) {
				endmntent(mount_table);
				return 1;
			}
		}
	} while (TRUE);

	return 0;
}

/** get available storage.
 *
 * @param si
 *       pointer to availabe storage ID array
 * @return
 *         number of storage available.
 *         -1 scan error.
 */
int CoolStorageAvailable(storage_info_t *si )
{
	FILE *mount_table;
	struct mntent *mount_entry = NULL;
	struct statfs s;
	int rlt=0;

	
	mount_table = NULL;
	if (!(mount_table = setmntent("/proc/mounts", "r"))) 
	{
	  	WARNLOG("unable to set mount entry.\n");
		return -1;
	}


	do {
		const char *device;
		const char *mount_point;
		if (mount_table) {
			if (!(mount_entry = getmntent(mount_table))) {
				endmntent(mount_table);
				//WARNLOG("end of mount table.\n");
				break;
			}
		}

		device = mount_entry->mnt_fsname;
		mount_point = mount_entry->mnt_dir;

		if (statfs(mount_point, &s) != 0) {
			endmntent(mount_table);
		  	WARNLOG("mount points error.\n");
			return -1;
		}
		DBGLOG("device = %s || mp = %s\n", device, mount_point);
		if ((s.f_blocks > 0) || !mount_table){

			if (strcmp(device, "rootfs") == 0) {
				continue;
			}
			if (strcmp(device, "tmpfs") == 0) {
				continue;
			}	
			if (strcmp(device, "/dev/root") == 0) {
				continue;
			}			
			rlt++;
			strcpy(si->device,device);
			strcpy(si->mount_point,mount_point);
			si->total = s.f_blocks*s.f_bsize;
			si->free = s.f_bavail*s.f_bsize;
			si++;
		
		}

	} while (1);

  	return rlt; 
}


/** helper function to special path in storage struct point.
 *
 * @param si
 *        all storage struct point.
 * @param sicount
 *	     count of all storage
 * @param path
 *        special path will be find.
 * @return
 *         index of all storage
 *         -1 storage not found.
 */
int CoolStorageSearch(const storage_info_t *si ,int sicount,const char *path)
{
	int i,l;
	storage_info_t *tmp;
	char srotage[256];
	char tempPath[256];
	sprintf(tempPath,"%s/",path);

	for(i=0;i<sicount;i++)
	{
		tmp = (storage_info_t *)(si+i);
		sprintf(srotage,"%s/",tmp->mount_point);
		l=strlen(srotage);
		if( strncmp(srotage,tempPath,l)==0 )
		{
			return i;
		}

	}

	return -1;

}

/** helper function to determine if specified storage has enough space lef.
 *
 * @param si
 *        all storage struct point.
 * @param index
 *	     index for all storage
 * @param min_reserve
 *        minimum reserved space in KB.
 * @return
 *         0 if free space is less than min_reserve
 *         1 if enough space left.
 */
int CoolIsStorageSpaceEnough(const storage_info_t *tmp , int min_reserve)
{
	if (tmp->free < min_reserve)
	  {
		// free space of device is not enough
		//WARNLOG(" free space of device is not enough.\n");
		return 0;
	  }
	return 1;
}




