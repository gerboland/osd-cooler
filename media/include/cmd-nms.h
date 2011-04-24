#ifndef NMS_CMD__H
#define NMS_CMD__H
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
 * Neuros-Cooler platform nms command interface module header.
 *
 * REVISION:
 * 
 * 2) Added ACK support to command interface. ------------- 2006-04-14 MG
 * 1) Initial creation. ----------------------------------- 2005-09-19 MG 
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "nc-type.h"

#define NMS_PROTOCOL_VERSION 1

#define NMS_CMD_ACK  0x8000
#define NMS_CMD_MASK 0x7fff

typedef struct {
    unsigned int ver;     /* protocol version. */
    unsigned int cmd;     /* command. */
    unsigned int dataLen; /* data length. */
} pkt_hdr_t;

typedef struct {
    pkt_hdr_t    hdr; 
    void *       data;
    int          fd;
} pkt_node_t;

enum
  {
	/* decoder interface. */
	CMD_FF_RW, 
	CMD_FRAME_BY_FRAME, 
	CMD_GET_FFRW_LEVEL,
	CMD_GET_FILE_INDEX,   
	CMD_GET_FILE_PATH,
	CMD_GET_PLAYMODE,
	CMD_GET_PLAY_TIME, 
	CMD_GET_REPEATMODE,
	CMD_GET_REPEAT_AB_STATUS,  
	CMD_GET_SFRW_LEVEL,  
	CMD_GET_SRV_STATUS,
	CMD_GET_TOTAL_FILES,    
	CMD_GET_VOLUME, 
	CMD_IS_PLAYING,
	CMD_MEDIA_INFO,
	CMD_PAUSE_UNPAUSE,
	CMD_PLAY, 
	CMD_REPEAT_A_B,
	CMD_SET_INPUT_MODE,	
	CMD_SET_OUTPUT_MODE,	 
	CMD_SET_OUTPUT_PROPORTIONS,
	CMD_GET_OUTPUT_PROPORTIONS,
	CMD_SET_PLAYMODE, 
	CMD_SET_EDITMODE,
	CMD_SET_REPEATMODE,
	CMD_SET_VOLUME,
	CMD_SEEK, 
	CMD_SF_RW,
	CMD_STOP_PLAY, 	
	CMD_TRACK_CHANGE,

	/* slide show interface. */
	CMD_START_SLIDE_SHOW,
	CMD_SET_SLIDE_SHOW_IMAGE,
	CMD_STOP_SLIDE_SHOW,

	/* encoder interface. */
	CMD_GET_GAIN,
	CMD_GET_RECORD_TIME,
	CMD_GET_RECORD_SIZE, 
	CMD_GET_RECORD_ERROR,
	CMD_IS_RECORDING,
	CMD_RECORD,
	CMD_PAUSE_UNPAUSE_RECORD,
	CMD_SET_GAIN,
	CMD_STOP_RECORD, 

	/* video monitor interface. */
	CMD_START_MONITOR,      
	CMD_STOP_MONITOR,
	CMD_IS_MONITOR_ENABLED,

	/* generic interface. */
	CMD_GET_VERSION,        
	CMD_PING,
	CMD_STOP_SERVER,
	CMD_INVALID,
	
	/* capture interface. */
	CMD_CAP_INIT,        
	CMD_CAP_GET_FRAME,
	CMD_CAP_FINISH
  };

int                  CoolCmdGetAll(int, void *, int);
int                  CoolCmdSendAll(int, const void *, int);
int                  CoolCmdConnectToSession(int);
void *               CoolCmdGetPacket(int, pkt_hdr_t *);
void                 CoolCmdGetSockPath(int, char *);
void                 CoolCmdSendPacket(int, unsigned int, void *, unsigned int);
BOOL                 CoolCmdIsSessionRunning(int);

#endif /* NMS_CMD__H */
