#ifndef WEBCLIENT__H
#define WEBCLIENT__H
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
 * Download file from web server header.
 *
 * REVISION:
 * 
 * 
 * 2) Modify API. ----------------------------------------- 2006-08-28 JC
 * 1) Initial creation. ----------------------------------- 2006-08-28 EY 
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

/**
*Function: CoolNewSwDetect
*Description: check whether there is newer version in the web server
*Parameters:
*	None
*Return:
*	1: there is newer version
*	0: there is no newer version
*      -1: there is error happened in net connection
*/
int CoolNewSwDetect(void);

/**
*Function: CoolUrlDownloadUpk
*Description: send get method to server, request to download file
*Parameters:
*	const char* url: the file name which is to be downloaded
*Return:
*	0:download successfully, otherwise failed
*	1:net connection error
*	-1:stop downloading by user
*/
int CoolUrlDownloadUpk(const char * url);

/**
*Function: CoolStopDownloadUPK
*Description: stop downloading OSD firmware file
*Parameters:
*	None
*Return:
*	None
*/
void CoolStopDownloadUPK(void);


/**
*Function: CoolSetDownloadUPK
*Description: setup parameters for downloading OSD firmware, such as web server IP, port etc..
*Parameters:
*	host: the web server IP address
*	port: the web server http port
*	osdVerUrl: the OSD firmware version file location in the web server
*	savePath: the firmware's saving path in the OSD
* curVersion: current OSD firmware version
*Return:
*	None
*/
void CoolSetDownloadUPK(const char* host, const char* port, const char* osdVerUrl, const char* savePath, const char* curVersion);

/**
*Description: get downloaded file's total size by byte
*Parameter:
*	None
*Return:
*	file's total size 
*/
int CoolGetDownFileSize(void);

/**
*Function: CoolGetCurSize
*Description: get current downloaded size by byte
*Parameter:
*	None
*Return:
*	current downloaded size 
*/
int CoolGetCurSize(void);

/**
*Function: CoolGetRemoteVersion
*Description: get remote OSD firmware version
*Parameter:
*	None
*Return:
*	remote OSD firmware version 
*/
char* CoolGetRemoteVersion(void);

/**
 * Function:CoolGetDownUrl
 * Desctription: Get the Url from the file download from server
 * Parameters : None
 * Return:
 *     the upk url in the file from server
 */
char * CoolGetDownUrl(void);

/**
*Function: CoolGetUpkWholeUrl
*Description: get downloaded upk whole url
*Parameter:
*	None
*Return:
*	upk whole url 
*/
char * CoolGetUpkWholeUrl(void);


#endif /* WEBCLIENT__H */
