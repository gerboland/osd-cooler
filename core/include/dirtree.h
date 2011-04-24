#ifndef DIRTREE__H
#define DIRTREE__H
/*
 *  Copyright(C) 2005 Neuros Technology International LLC. 
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
 * directory tree module header.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2005-09-23 MG 
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include "nc-type.h"

enum DIR_FILTER_MODE {
	DF_ALL,                     /* filter for all. */
	DF_FILTERED                 /* filter on current filtered output. */
};



typedef enum
{
	DMT_SINGLE, /*not mt, is a single thread */
	DMT_RUNNING, /*multi-thread is running*/
	DMT_FINISHED, /*multi-thread is finished*/

}DF_MULTI_THREAD_STATE;

typedef struct
{
	DF_MULTI_THREAD_STATE   mtstate;			/*mt state */
	pthread_t 				sorttid ; 		/* sort thread id*/
	pthread_t 				filtertid ;		/* filter thread id */
	unsigned int 				cindex;			/*current process index */
}dir_mt_t;

typedef struct
{
	dir_mt_t		dirmt;
  	BOOL             scanned;  
	/* following members are valid only if scanned = TRUE. */
	unsigned int     num;       /* total number of directory/file entries. */
	unsigned int     dnum;	    /* total number of valid directory entries. */
	unsigned int     fnum;      /* total number of valid file entries. */
	unsigned int     fdnum;	    /* filtered directory entries. */
	unsigned int     ffnum;     /* filtered file entires. */
 	unsigned int *   dindex;	/* directory name list index. */
  	unsigned int *   findex;	/* file name list index. */
 	unsigned int *   fdindex;	/* filtered directory name list index. */
  	unsigned int *   ffindex;	/* filtered file name list index. */
	char *           path;      /* path name. */
 	struct dirent ** namelist;	/* directory/file name list. */
} dir_node_t;


typedef int (*pf_dir_filter_t) (const char *);
typedef int (*pf_sort_t)(dir_node_t *);
typedef int (*pf_filter_t)(dir_node_t * , pf_dir_filter_t , int  );
typedef int (*pf_def_select_t)(const struct dirent *);
typedef int (*pf_compare_t)(const void *, const void *);

int		CoolOpenDirectorySpecific( const char * , dir_node_t * ,pf_def_select_t, pf_sort_t,unsigned char, pf_compare_t);

#define CoolOpenDirectory(path, node, defselect, sortfp, blockingsort) CoolOpenDirectorySpecific(path, node, defselect, sortfp, blockingsort, NULL)
#define     CoolBlockOpenDir(path,node)              CoolOpenDirectory(path, node,CoolDefSelectNoDotDot,CoolSortDivFileDir,TRUE )
#define     CoolNonBlockOpenDir(path,node)              CoolOpenDirectory(path, node,CoolDefSelectNoDotDot,CoolSortDivFileDir,FALSE )

int          CoolFilterDirectory(dir_node_t *, pf_dir_filter_t, int);
void         CoolCloseDirectory(dir_node_t *);

int		CoolSortDivFileDir(dir_node_t *);
int		CoolRootDirCompare(const void *, const void *);

int 		CoolDefSelectNoDotDot(const struct dirent *);
int		CoolDefSelectNoHiddenEntry(const struct dirent *);

int          CoolDirFilterHiddenEntry(const char *);
int	     CoolDirFilterDotDot(const char *);

int          CoolGetFileSize(const char *fileName);
int	     CoolGetDirectorySize(const char *dirname);

#endif /* DIRTREE__H */
