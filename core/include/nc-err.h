#ifndef NC_ERROR__H
#define NC_ERROR__H
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
 * Neuros-Cooler platform error module header.
 *
 * REVISION:
 * 
 * 2) Log messages reports pid. --------------------------- 2007-08-07 MG
 * 1) Initial creation. ----------------------------------- 2006-02-03 MG 
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>

#define EPRINT(fmt, arg...) do {										\
		printf("\033[0;41;30m[%d] ERROR: %s--%s: %d\033[0m\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		printf("errno = %d\n\t", errno);								\
		printf(fmt, ##arg);												\
		assert(0); } while(0)

#define WPRINT(fmt, arg...) do {										\
		printf("\033[0;46;30m[%d] WARNING: %s--%s: %d\033[0m\n\t", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		printf(fmt, ##arg); } while(0)

#define MPRINT()  do {													\
		void * p; p = malloc(10);										\
		printf("[%d] %s--%s: %d\n\t", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		printf("malloc starts from:##0x%X##\n",(int)p);					\
		free(p); } while(0)

#define DPRINT(fmt, arg...) do{											\
		printf("\033[0;42;30m[%d] %s--%s: %d\033[0m\n\t", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		printf(fmt, ##arg); } while(0)

#define FATAL(fmt, arg...) do {											\
		printf("\033[0;41;30m[%d] %s--%s: %d\033[0m\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		printf("errno = %d\n\t", errno);								\
		printf(fmt "FATAL", ##arg); exit(0);} while(0)


#ifdef OSD_DBG_MSG
#define DBGMSG(fmt, arg...)  DPRINT(fmt, ##arg)
#define INFOLOG(fmt, arg...) do {										\
		syslog(LOG_INFO, "[%d] INFO: %s--%s: %d\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		syslog(LOG_INFO, fmt, ##arg); } while(0)
#define DBGLOG(fmt, arg...)  do { DBGMSG(fmt, ##arg);					\
		syslog(LOG_INFO, "[%d] DEBUG: %s--%s: %d\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		syslog(LOG_DEBUG, fmt, ##arg); } while(0)
#else
#define DBGMSG(fmt, arg...)  do {;}while(0)
#define INFOLOG(fmt, arg...) do {;}while(0)
#define DBGLOG(fmt, arg...)  do {;}while(0)
#endif
#define WARNLOG(fmt, arg...) do { WPRINT(fmt, ##arg);					\
		syslog(LOG_INFO, "[%d] WARNING: %s--%s: %d\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		syslog(LOG_WARNING, fmt, ##arg); } while(0)
#define ERRLOG(fmt, arg...)  do { EPRINT(fmt, ##arg);					\
		syslog(LOG_INFO, "[%d] ERROR: %s--%s: %d\n", getpid(), __BASE_FILE__, __FUNCTION__, __LINE__); \
		syslog(LOG_ERR, fmt, ##arg); } while(0)
#define FATALLOG(fmt, arg...)  do { EPRINT(fmt, ##arg);					\
		syslog(LOG_INFO, "[%d] FATAL: %s--%s: %d\n", getpid(),__BASE_FILE__, __FUNCTION__, __LINE__); \
		syslog(LOG_ERR, fmt, ##arg); while(1);} while(0)


#define ASSERT assert

#endif /* NC_ERROR__H */
