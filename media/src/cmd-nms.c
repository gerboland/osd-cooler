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
 * Neuros-Cooler platform nms command interface.
 *
 * REVISION:
 * 
 * 2) nmsd implementation, here contains interface code shared
 *    by client and server.-------------------------------- 2006-12-09 MG
 * 1) Initial creation. ----------------------------------- 2005-09-19 MG 
 *
 */

#include <pthread.h>
#include <unistd.h>
#include "cmd-nms.h"
#include "nc-err.h"

#if 0
#define DBGLOG(fmt, arg...) DPRINT(fmt, ##arg)
#else
#define DBGLOG(fmt, arg...) do {;}while(0)
#endif

#define CMD_RETRY_COUNT   16

#if 0
static dprint(void * buf, int bytes)
{
	int ii;
	char * p = (char*)buf;
	for (ii = 0; ii < bytes; ii++)
	{
		if ((ii%8) == 0) DBGLOG("\n");
		DBGLOG("%02x ", p[ii]);
	}
}
#else
#define dprint(x, y) ;
#endif


/** Get all data that is available.
 *
 * @param fd
 *        socket file descriptor.
 * @param buf
 *        buffer to hold the data.
 * @param count
 *        number of data expected in bytes.
 * @return
 *        number of data actually received in bytes.
 */
int 
CoolCmdGetAll(int fd, void *buf, int count)
{
	int left = count;
	int r;
	int retry = 0;

	do 
	{
		DBGLOG("reading fd:%d count:%x left:%x\n", fd, count, left);
        r = read(fd, buf, left);
		DBGLOG("reading returned:%d\n", r);
		if (r < 0) return -1;
		left -= r;
		buf = (char *)buf + r;

		if (r == 0)
		{
			usleep(1000);
			if (retry++ > CMD_RETRY_COUNT) 
			{
				WARNLOG("retry timeout!\n");
				break;
			}
		}
		else retry = 0;
	} while (left > 0);
	
	if (left) 
	{
		WARNLOG("CMD rcv error: [count:=%d left:=%d]!\n", count, left);
	}
	DBGLOG("reading returning: %d\n", count-left);

	dprint(buf - (count-left), count);

    return count - left;
}


/** Send all data that is available.
 *
 * @param fd
 *        socket file descriptor.
 * @param buf
 *        buffer to hold the data.
 * @param count
 *        number of data expected in bytes.
 * @return
 *        number of data actually sent in bytes.
 */
int 
CoolCmdSendAll( int fd, const void * buf, int count )
{
	int left = count;
	//int loop = 0;

	dprint(buf, count);

	do 
	{
		int written = write(fd, buf, left);
		if (written < 0) 
		{
			ERRLOG("Failed to send data to nms!\n");
			return -1;
		}
		left -= written;
		buf = (char *)buf + written;
		//DBGLOG("CMD writing loop%d!\n", loop++);
	} while (left > 0);
	
	return count;
}

/**
 * Get socket path for current session.
 *
 * @param sid
 *        session ID.
 * @param path
 *        socket path.
 */
void  
CoolCmdGetSockPath( int sid, char * path )
{
#if BUILD_TARGET_ARM
	//snprintf(path, 108, "/usr/local/bin/nms_mgao.%d", sid);
	snprintf(path, 108, "/var/tmp/nms_mgao.%d", sid);
#else
	//snprintf(path, 108, "/var/tmp/nms_%s.%d", getenv("USER"), sid);
	snprintf(path, 108, "/var/tmp/nms_mgao.%d", sid);
#endif
}

/**
 * Connect to command session.
 *
 * @param sid
 *        Session ID.
 * @return
 *        Client socket file descriptor.
 */
int 
CoolCmdConnectToSession( int sid )
{
	int fd;
	uid_t savedUid, euid;
	struct sockaddr_un saddr;
    
	if (-1 == sid) return -1;

    DBGLOG("Connecting to session: %d\n", sid);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1) 
	{
	    saddr.sun_family = AF_UNIX;
		savedUid = getuid();
		euid     = geteuid();
		setuid(euid);
		CoolCmdGetSockPath(sid, saddr.sun_path);
		setreuid(savedUid, euid);
		if (connect(fd, (struct sockaddr *) &saddr, sizeof (saddr)) != -1)
		{
			DBGLOG("Connected to session: %d  fd: %d\n", sid, fd);
		    return fd;
		}
		else close(fd);
	}
	
	DBGLOG("Unable to connect to NSM cmd session!\n");
    return -1;
}

/**
 * Get packet. 
 * 
 * @param fd
 *        socket file descriptor.
 * @param hdr
 *        packet header.
 * @return
 *        pakcet data if any.
 */
void * 
CoolCmdGetPacket( int fd, pkt_hdr_t * hdr )
{
	void * data = NULL;
	int len;
	DBGLOG("getting packet\n");
	if ( CoolCmdGetAll(fd, hdr, sizeof(pkt_hdr_t) ) == sizeof(pkt_hdr_t)) 
	{
		DBGLOG("getting packet done\n");
		if (hdr->dataLen) 
		{
			DBGLOG("expected: %d\n", hdr->dataLen);
		  	data = (void *)calloc(hdr->dataLen, sizeof(char));
			if ((len = CoolCmdGetAll(fd, data, hdr->dataLen)) != hdr->dataLen) 
			{
			  	free(data);
				data = NULL;
				WARNLOG("CMD packet error! [received/expected %d/%d]\n", len, hdr->dataLen);
			}
		}
	}
    
	return data;
}

/**
 * Send packet over the socket.
 *
 * @param fd
 *        socket file descriptor.
 * @param cmd
 *        packet command ID.
 * @param data
 *        packet data.
 * @param len
 *        data length.
 */
void 
CoolCmdSendPacket( int fd, unsigned int cmd, void * data, 
			unsigned int len )
{
	pkt_hdr_t hdr;

	hdr.ver = NMS_PROTOCOL_VERSION;
    hdr.cmd = cmd;
	hdr.dataLen = len;
	if (CoolCmdSendAll(fd, &hdr, sizeof(pkt_hdr_t)) < 0)
		return;
	if (len && data)
		CoolCmdSendAll(fd, data, len);
}

/** determines if given cmd session is running.
 *
 * @param sid
 *        session ID.
 * @return
 *        TRUE if session is running, FALSE otherwise.
 */
BOOL 
CoolCmdIsSessionRunning( int sid )
{
	BOOL ret = FALSE;
	int fd;
	void * d;
	pkt_hdr_t hdr;

    if ((fd = CoolCmdConnectToSession(sid)) == -1) return FALSE;
    
    CoolCmdSendPacket(fd, CMD_PING, NULL, 0);
	d = CoolCmdGetPacket(fd, &hdr);

	if ( (hdr.cmd&NMS_CMD_MASK) != CMD_PING )
	{
		DBGLOG("Command interface out of sync!");
		ret = FALSE;
	}
	else 
	{
		ret = ((hdr.cmd&NMS_CMD_ACK)==NMS_CMD_ACK)? TRUE:FALSE;
		if ( d ) free(d);
	}
	
    close(fd);
    
    return ret;
}
