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
 * Neuros-Cooler platform nms client module.
 *
 * REVISION:
 * 
 * 4) Handle PLAY cmd new return values . ----------------- 2007-05-25 nerochiaro
 * 3) nmsd implementation, client becomes thin. ----------- 2006-12-09 MG
 * 2) Added encoder interface API funtions. --------------- 2006-01-06 MG
 * 1) Initial creation. ----------------------------------- 2005-09-19 MG 
 *
 */

#include <pthread.h>
#include <unistd.h>
//#define  OSD_DBG_MSG
#include "nc-err.h"

#include "cmd-nms.h"
#include "client-nms.h"
#include "com-nms.h"
#include "nmsplugin.h"

#define RETRY_COUNT__ 2

static int sessionId = -1;

static pthread_mutex_t nmcMutex = PTHREAD_MUTEX_INITIALIZER;
typedef void (*pf_cleancb_t)(void *);
static int oldcanceltype;
  /* LOCK_NMC and UNLOCK_NMC are macros and must always
   be used in matching pairs at the same nesting level of braces. */
#define LOCK_NMC()  \
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldcanceltype); \
	pthread_cleanup_push( (pf_cleancb_t)pthread_mutex_unlock,&nmcMutex); \
	pthread_mutex_lock(&nmcMutex);				

#define UNLOCK_NMC()  \
	pthread_cleanup_pop(1); 	\
	pthread_setcanceltype(oldcanceltype, NULL);


#ifndef _TAG_LENGTH_
        #define _TAG_LENGTH_
        #define MAX_TAG_GENRE_LENGTH        51
        #define MAX_TAG_TITLE_LENGTH        101
        #define MAX_TAG_ARTIST_LENGTH       101
        #define MAX_TAG_ALBUM_LENGTH        101
        #define MAX_TAG_COMMENT_LENGTH      101
        #define MAX_TAG_YEAR_LENGTH         11
#endif

#define restartSrv() do{ \
		system("/etc/init.d/nmsd stop");\
		system("/etc/init.d/nmsd start");\
	}while(0)

/*
 * Send command, returned data if any has to be released by caller.
 *
 * @param rdata
 *           Command returned data object.
 * @param cmd
 *           Command.
 * @param data
 *           Data pointer.
 * @param len
 *           Data length.
 * @return
 *           TRUE if command successfully sent, otherwise FALSE.
 */
static BOOL 
sendNmsCmd( cmd_ret_data_t * rdata, 
			unsigned int cmd, void * data, unsigned int len )
{
	BOOL ret = FALSE;
	int fd;
	void * d;
	pkt_hdr_t hdr;
	int  retryCount = 0;
	BOOL needretry;


	while(retryCount++ < RETRY_COUNT__)
	{
		LOCK_NMC();	
		needretry=FALSE;
		if ((fd = CoolCmdConnectToSession(sessionId)) == -1) 
		{	
			needretry=TRUE;
			goto unlock;
		}
		CoolCmdSendPacket(fd, cmd, data, len);
		DBGLOG("Packet sent: cmd=%x len = %x\n", cmd, len);
		d = CoolCmdGetPacket(fd, &hdr);
		DBGLOG("Packet returned: hdr.cmd = %x\n", hdr.cmd);
		if ( (hdr.cmd&NMS_CMD_MASK) != cmd ) 
		{
			WARNLOG("Command interface out of sync!\n");
			close(fd);
			needretry=TRUE;
			goto unlock;
		}
		else if ( rdata ) 
		{
			if ( NULL == d )
			{
				WARNLOG("No data packet arrived!\n");
				close(fd);
				needretry=TRUE;
				goto unlock;
			}
			else 
			{
				rdata->dataLen = hdr.dataLen;
				rdata->data = d;
				ret = ((hdr.cmd&NMS_CMD_ACK)==NMS_CMD_ACK)? TRUE:FALSE;
			}
		} 
		else 
		{
			ret = ((hdr.cmd&NMS_CMD_ACK)==NMS_CMD_ACK)? TRUE:FALSE;
			if ( d ) free(d);
		}
		
#if 0
		// ack is now combined with the data returned if any.
		/* Get ack, currently ack does nothing. */
		if ( d )
		{
			d = CoolCmdGetPacket(fd, &hdr);
			if (d) free(d);
		}
#endif
		
		//FIXME: why can't we leave the fd open and close it at disconnection time??
		//       reopen the socket for every communication doesn't seem efficient.
		close(fd);
unlock:
		UNLOCK_NMC();	

		if(needretry)
		{
			// reconnect before retry.
			WARNLOG("Unable to send command, retrying: %d cmd: 0x%x\n", retryCount,cmd);
			if( NmsConnect()) break;
		}
		else
			break;
	}

	return ret;
}

/**
 * Connect to NMS server.
 * 
 * @return
 *        0 if successful, otherwise non-zero.
 */
int 
NmsConnect( void )
{
	char path[NMS_SOCKET_PATH_LEN_MAX];
	int ii;
	int retry = 0;
	int ret = 0;

	LOCK_NMC();
	if (sessionId != -1) goto bail;
	for ( ii = 0;; ii++ ) 
	{
		CoolCmdGetSockPath(ii, path);
		DBGLOG("socket path: %s\n", path);
		if ( FALSE == CoolCmdIsSessionRunning(ii) )
		{
			DBGLOG("unlinking: %s\n", path);
			unlink(path);
			
			if (retry++ < RETRY_COUNT__)
			{
				//HACK: start server if server is not running.
				WARNLOG("Restarting NMS server.\n");
				restartSrv();
				ii--;
				continue;
			}
			else 
			{
				ret = 1;
				goto bail;
			}
		}
#if 0 //FIXME: below logic does not make sense, currently only one
	  //       instance of nmsd is running anyway... MG 12-26-2006
		else if ( NMS_isAllowMultipleInstances() )
		{
			DBGLOG("Allow multiple instances, continue.\n");
			if (ii < NMS_SESSION_MAX) continue;
			WARNLOG("Server is not running.\n");
			ret = 1;
			goto bail;
		}
#endif
		DBGLOG("Active session found. %d\n", ii);
		break;
	}
	DBGLOG("session ID = %d\b", ii);
	sessionId = ii;

 bail:
	UNLOCK_NMC();
    return ret;
}

/**
 * Disconnect from NMS server.
 */
void
NmsDisconnect( void )
{
	LOCK_NMC();
	sessionId = -1;
	UNLOCK_NMC();
}

/**
 * return current NMS version string. Caller is responsible to release the 
 * returned data if not NULL.
 *
 * @return
 *        Current version string if successful, otherwise NULL.
 */
char *
NmsGetVersion( void )
{
	cmd_ret_data_t rdata;
	char * ret = NULL;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_VERSION.");
	if ( TRUE == sendNmsCmd(&rdata, CMD_GET_VERSION, NULL, 0) )
	{
	    ret = (char*)rdata.data;
		goto bail;
	}
	else WARNLOG("unable to get server version.\n");
 bail:
	//UNLOCK_NMC();
	return ret;
}

/**
 * set output mode
 * @param is_pal
 *		0 : ntsc output, 1 : pal output
 */
 void
 NmsSetOutputMode(int is_pal)
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_SET_OUTPUT_MODE.");
	sendNmsCmd(NULL, CMD_SET_OUTPUT_MODE, &is_pal, sizeof(int));
	DBGLOG("setting output mode done.\n");
	//UNLOCK_NMC();	
}

/**
 * Set output proportions for video that will be playback.
 * This setting will be effective only after the next call to NmsPlay.
 * It will not affect any currently playing video.
 *
 * @param proportions
 *        0 : normal (4:3), 1 : widescreen (16:9)
 */
void
NmsSetOutputProportions(int proportions)
{
	DBGLOG("CLIENT: CMD_SET_OUTPUT_PROPORTIONS.");
	sendNmsCmd(NULL, CMD_SET_OUTPUT_PROPORTIONS, &proportions, sizeof(int));
	DBGLOG("setting output proportions done.\n");
}

/**
 * Get output proportions for next video that will be playback.
 * This setting may not be the effective proportions for currently playing video.
 * See @ref NmsSetOutputProportions for more details.
 *
 * @return The current proportions. 0 : normal (4:3), 1 : widescreen (16:9)
 */
int
NmsGetOutputProportions()
{
	cmd_ret_data_t rdata;
	int proportions = 0;

	DBGLOG("CLIENT: CMD_GET_OUTPUT_PROPORTIONS.");
	sendNmsCmd(&rdata, CMD_GET_OUTPUT_PROPORTIONS, NULL, 0);
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		proportions = *p;
		free(rdata.data);
	}
	else return 0; //default to 4:3 proportions in case of error

	DBGLOG("getting output proportions done.\n");
	return proportions;
}

/**
 * set input mode
 * @param is_pal
 *		0 : ntsc input, 1 : pal output
 */
 void
 NmsSetInputMode(int is_pal)
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_SET_INPUT_MODE.");
	sendNmsCmd(NULL, CMD_SET_INPUT_MODE, &is_pal, sizeof(int));
	DBGLOG("setting output mode done.\n");
	//UNLOCK_NMC();	
}

/*------------------------------------------------------------------------*/
/*---------------------- decoder interface starts. -----------------------*/
/*------------------------------------------------------------------------*/

int
NmsStartSlideShow(void)
{
	cmd_ret_data_t rdata;
	int startslideshow = 0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_START_SLIDE_SHOW.");
	sendNmsCmd(&rdata, CMD_START_SLIDE_SHOW, NULL, 0);
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		startslideshow = *p;
		free(rdata.data);
	}
	DBGLOG("slide show started.\n");
	//UNLOCK_NMC();	
	return startslideshow;
}

void
NmsStopSlideShow(void)
{
	DBGLOG("CLIENT: CMD_STOP_SLIDE_SHOW.");
	sendNmsCmd(NULL, CMD_STOP_SLIDE_SHOW, NULL, 0);
}

void 
NmsSetSlideShowImage(const char *data)
{
	DBGLOG("CLIENT: CMD_SET_SLIDE_SHOW_IMAGE.");
	sendNmsCmd(NULL, CMD_SET_SLIDE_SHOW_IMAGE, (void *)data, strlen(data) + 1);
}

/**
 * command NMS to start to playback given contents, which can be a single
 * file, a directory, a playlist, or query result from DB.
 *
 * @param type
 *        contents type. 
 *        0: single file
 *        1: directory
 *        2: playlist
 *        3: Neuros DB
 *        4: other
 * @param content
 *        contents specifier.
 * @return
 *        0 if playback started successfully,
 *        1 if output was locked,
 *        -1 if there are errors.
 */
int
NmsPlay( int type, void * contents )
{
	char * data;
	cmd_ret_data_t rdata;
	int dlen;
	int ret = -1;
	//LOCK_NMC();
	DBGLOG("play multimedia.\n");
	switch (type)
	{
	case NPT_FILE:     /* single file. */
		{
			data = (char*)contents;
			dlen = sizeof(int)+strlen(data)+1;
			data = (char*)malloc(dlen);
			memcpy(data, &type, sizeof(int));
			strcpy(data+sizeof(int), (char*)contents);
			break;
		}
	case NPT_DIR:      /* directory.   */
		{
			data = (char*)contents;
			dlen = sizeof(int)+strlen(data)+1;
			data = (char*)malloc(dlen);
			memcpy(data, &type, sizeof(int));
			strcpy(data+sizeof(int), (char*)contents);
			break;
		}
	case NPT_HISTORY:  /* history.     */
		{
			dlen = sizeof(int);
			data = (char*)malloc(dlen);
			memcpy(data, &type, sizeof(int));
			break;
		}
	case NPT_PLAYLIST: /* playlist.    */
	case NPT_DB:       /* Neuros DB.   */
	case NPT_OTHER:    /* other.       */
	default: 
		WARNLOG("This play type is currently unsupported.\n");
		goto bail;
	}

	DBGLOG("CLIENT: CMD_PLAY --- data:[%s] dlen:[%d].\n", data, dlen);
	sendNmsCmd(&rdata, CMD_PLAY, data, dlen);
	if (rdata.data)
	{
		int* pstatus = (int*) rdata.data;
		DBGLOG("playback command rdata: %d.\n", *pstatus);

		switch (*pstatus) 
		{
		case 0: //success
		case 1: //output locked
			ret = *pstatus; 
			break;
		default: 
			ret = -1; 
			break;
		}
		free(rdata.data);
	}

	if (ret == -1) WARNLOG("Play command sent failed!\n");

	free(data);
 bail:
	//UNLOCK_NMC();	
	return ret;
}

/**
 * Pause/unpause current playback.
 */
void
NmsPauseUnpause( void )
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_PAUSE_UNPAUSE.");
	sendNmsCmd(NULL, CMD_PAUSE_UNPAUSE, NULL, 0);
	//UNLOCK_NMC();	
}

/**
 * Stop current playback.
 */
void
NmsStopPlay( void )
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_STOP_PLAY.");
	sendNmsCmd(NULL, CMD_STOP_PLAY, NULL, 0);
	//UNLOCK_NMC();	
}

/**
 *  Get server status
 */
 int
 NmsGetSrvStatus(void)
{
	cmd_ret_data_t rdata;
	int errorStatus = -1;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_SRV_STATUS.");
	sendNmsCmd(&rdata, CMD_GET_SRV_STATUS, NULL, 0); 
	if (rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		errorStatus = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return errorStatus;
}

/**
 * Get current volume.
 *
 * @param left
 *        left channel volume.
 * @param right
 *        right channel volume.
 */
void
NmsGetVolume( int * left, int * right )
{
	cmd_ret_data_t rdata;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_VOLUME.");
	sendNmsCmd(&rdata, CMD_GET_VOLUME, NULL, 0);
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		*left = *p++;
		*right = *p;
		free(rdata.data);
	}
	else
	{
		*left = -1;
		*right = -1;
	}
	//UNLOCK_NMC();	
}

/**
 * Set volume.
 *
 * @param left
 *        left channel volume.
 * @param right
 *        right channel volume.
 */
void
NmsSetVolume( int left, int right )
{
	int vol[2];
	//LOCK_NMC();	
	vol[0] = left;
	vol[1] = right;
	DBGLOG("CLIENT: CMD_SET_VOLUME.");
	sendNmsCmd(NULL, CMD_SET_VOLUME, vol, sizeof(vol));
	//UNLOCK_NMC();	
}

/**
 * Get current playback time stamp.
 *
 * @return
 *         current playback time stamp in mili-seconds.
 */
int
NmsGetPlaytime( void )
{
	cmd_ret_data_t rdata;
	int t = -2;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_PLAY_TIME.");
	sendNmsCmd(&rdata, CMD_GET_PLAY_TIME, NULL, 0); 
	if (rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		t = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return t;
}

/**
 * Seek current playback to given time stamp.
 *
 * @param t
 *        time stamp in mili-seconds.
 * @return
 *        actual time stamp after been seeked.
 */
int  
NmsSeek( int t )
{
	cmd_ret_data_t rdata;
	int ret_t=0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_SEEK.");
	sendNmsCmd(&rdata, CMD_SEEK, &t, sizeof(int));
	if (rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		ret_t = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return ret_t;
}

/**
 * Fast forward/rewind current playback.
 *
 * @param level
 *        scan level.
 */
void
NmsFfRw( int level )
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_FF_RW.");
	sendNmsCmd(NULL, CMD_FF_RW, &level, sizeof(int));
	//UNLOCK_NMC();	
}

/**
 * Slow fast forward/rewind current playback.
 *
 * @param level
 *        scan level.
 */
void
NmsSfRw( int level )
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_SF_RW.");
	sendNmsCmd(NULL, CMD_SF_RW, &level, sizeof(int));
	//UNLOCK_NMC();	
}


/**
 * Get fast forward/rewind level
 */
int 
NmsGetFfRwLevel(void)
{
	cmd_ret_data_t rdata;
	int t = -2;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_FFRW_LEVEL.");
	sendNmsCmd(&rdata, CMD_GET_FFRW_LEVEL, NULL, 0); 
	if (rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		t = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return t;
}

/**
 * Get slow fast forward/rewind level
 */
int 
NmsGetSfRwLevel(void)
{
	cmd_ret_data_t rdata;
	int t = -2;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_SFRW_LEVEL.");
	sendNmsCmd(&rdata, CMD_GET_SFRW_LEVEL, NULL, 0); 
	if (rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		t = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return t;
}

/** Track change current playback.
 *
 * @param track
 *        -1: previous track
 *         0: restart current track.
 *         1: next track.
 * @return
 *        0 if track change successful, nonzero otherwise.
 *
 */
int
NmsTrackChange(int track)
{
	cmd_ret_data_t rdata;
	int ret_t=0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_TRACK_CHANGE.");
	sendNmsCmd(&rdata, CMD_TRACK_CHANGE, &track, sizeof(int));
	if (rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		ret_t = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return ret_t;
}

/**
 * Tell if playback is active.
 *
 * @return
 *         1 if playback is active, otherwise zero.
 */
int 
NmsIsPlaying( void )
{
	cmd_ret_data_t rdata;
	int playing = 0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_IS_PLAYING.");
	sendNmsCmd(&rdata, CMD_IS_PLAYING, NULL, 0);  
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		playing = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return playing;
}

/**
 * repeat A-B
 * @param ab,
 *	ab == -1,  set A = current play time
 *    ab == 1, 	   set B = current play time
 *	ab == 0,	   disable Repeat A-B and set A = 0, B = 0
 *
 */
void
NmsRepeatAB(int ab)
{
	DBGLOG("CLIENT: CMD_REPEAT_A_B.");
	sendNmsCmd(NULL, CMD_REPEAT_A_B, &ab, sizeof(int));
}

/** play frame by frame. */
void
NmsFrameByFrame(int direction)
{
	DBGLOG("CLIENT: CMD_FRAME_BY_FRAME.");
	sendNmsCmd(NULL, CMD_FRAME_BY_FRAME, &direction, sizeof(int));
}

/** 
 * get repeat A-B status 
 * @return
 *	repeat A-B status belows:
 *	REPEAT_OFF,
 *	REPEAT_ON,
 *	REPEAT_A,
 *	REPEAT_B,
 */
int 
NmsGetRepeatABStatus(void)
{
	cmd_ret_data_t rdata;
	int status = 0;
	DBGLOG("CLIENT: CMD_GET_REPEAT_AB_STATUS.");
	sendNmsCmd(&rdata, CMD_GET_REPEAT_AB_STATUS, NULL, 0);  
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		status = *p;
		free(rdata.data);
	}
	return status;
}


/** get playmode. */
int      
NmsGetPlaymode(void)
{
	cmd_ret_data_t rdata;
	int mode = 0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_PLAYMODE.");
	sendNmsCmd(&rdata, CMD_GET_PLAYMODE, NULL, 0);  
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		mode = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return mode;
}

/** set playmode. */
void     
NmsSetPlaymode(int mode)
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_SET_PLAYMODE.");
	sendNmsCmd(NULL, CMD_SET_PLAYMODE, &mode, sizeof(int));
	//UNLOCK_NMC();	
}

/** set edit mode. */
void     
NmsSetEditmode(int mode)
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_SET_EDIT_MODE.");
	sendNmsCmd(NULL, CMD_SET_EDITMODE, &mode, sizeof(int));
	//UNLOCK_NMC();	
}


/** get repeat mode. */
int      
NmsGetRepeatmode(void)
{
	cmd_ret_data_t rdata;
	int mode = 0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_REPEAT_MODE.");
	sendNmsCmd(&rdata, CMD_GET_REPEATMODE, NULL, 0);  
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		mode = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return mode;
}

/** set repeat mode. */
void    
NmsSetRepeatmode(int mode)
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_SET_REPEATMODE.");
	sendNmsCmd(NULL, CMD_SET_REPEATMODE, &mode, sizeof(int));
	//UNLOCK_NMC();	
}

/** get current file index. */
int NmsGetFileIndex(void)
{
	cmd_ret_data_t rdata;
	int idx = 0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_FILE_INDEX.");
	sendNmsCmd(&rdata, CMD_GET_FILE_INDEX, NULL, 0);  
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		idx = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return idx;

}

/** get total files in playing queue. */
int NmsGetTotalFiles(void)
{
	cmd_ret_data_t rdata;
	int total = 0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_TOTAL_FILES.");
	sendNmsCmd(&rdata, CMD_GET_TOTAL_FILES, NULL, 0);  
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		total = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return total;	
}

/** get playing file path. */
int NmsGetFilePath(int idx, char * path)
{
	cmd_ret_data_t rdata;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_FILE_PATH.");
	sendNmsCmd(&rdata, CMD_GET_FILE_PATH, &idx, sizeof(int));
	if (rdata.data)
	{
		char * p;
		p = (char*)rdata.data;
		strcpy(path, p);
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return 0;
}


/** 
 *  Get media infomation.
 */
void     
NmsGetMediaInfo(const char *filename,void *minfo)
{
	media_info_t *media_info;
	cmd_ret_data_t rdata;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_MEDIA_INFO.");
	if (sendNmsCmd(&rdata, CMD_MEDIA_INFO , (void *)filename , strlen(filename)+1))
	{
		media_info = (media_info_t *)rdata.data;
		if (media_info->available)
		{
			memcpy(minfo, media_info, sizeof(media_info_t));
		}
		else
		{
			memset(minfo, 0, sizeof(media_info_t));
		}
	}
	//UNLOCK_NMC();	
}

/*------------------------------------------------------------------------*/
/*---------------------- encoder interface starts. -----------------------*/
/*------------------------------------------------------------------------*/

/**
 * command NMS to start to record input stream to given video/audio format.
 *
 * @param ctrl
 *        encoder controls. 
 * @param detail
 *        if not NULL on return it will contain the error details of the start record operation
 * @param fname
 *        encoder output file name.
 * @return
 *        0 (NMS_RECORD_OK) if recording started successfully.
 *        otherwise nonzero (see NMS_SRV_RECORD_ERROR_t for meanings).
 */
NMS_SRV_RECORD_ERROR
NmsRecord( rec_ctrl_t * ctrl, const char * fname, NMS_SRV_ERROR_DETAIL * detail)
{
	char * data;
	int dlen;
	cmd_ret_data_t rdata;
	NMS_SRV_RECORD_ERROR ret = NMS_RECORD_SERVER_COMM;
	
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_RECORD.");

	dlen = sizeof(rec_ctrl_t)+strlen(fname)+1;

	data = (char*)malloc(dlen);
	memcpy(data, ctrl, sizeof(rec_ctrl_t));
	strcpy(data + sizeof(rec_ctrl_t), (char*)fname);

	BOOL sendret = sendNmsCmd(&rdata, CMD_RECORD, data, dlen);
	free(data);
	if (sendret != TRUE) return ret;
	if (rdata.data == NULL) return ret;

	ret = ((NMS_SRV_ERROR_DETAIL*)rdata.data)->error;
	if (detail != NULL) 
	{
		memcpy(detail, rdata.data, sizeof(NMS_SRV_ERROR_DETAIL));
	}
	free(rdata.data);

	//UNLOCK_NMC();	
	return ret;
}

/**
 * Pause/unpause current recording.
 *
 * @param p
 *        1 to pause, 0 to unpause.
 */
void
NmsPauseRecord( int p )
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_PAUSE_UNPAUSE_RECORD.");
	sendNmsCmd(NULL, CMD_PAUSE_UNPAUSE_RECORD, &p, sizeof(int));
	//UNLOCK_NMC();	
}

/**
 * Stop current record.
 */
void
NmsStopRecord( void )
{
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_STOP_RECORD.");
	sendNmsCmd(NULL, CMD_STOP_RECORD, NULL, 0);
	//UNLOCK_NMC();	
}

/**
 * Get current recording gain.
 *
 * @param left
 *        left channel gain.
 * @param right
 *        right channel gain.
 */
void
NmsGetGain( int * left, int * right )
{
	cmd_ret_data_t rdata;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_GAIN.");
	sendNmsCmd(&rdata, CMD_GET_GAIN, NULL, 0);
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		*left = *p++;
		*right = *p;
		free(rdata.data);
	}
	else
	{
		*left = -1;
		*right = -1;
	}
	//UNLOCK_NMC();	
}

/**
 * Set recording gain.
 *
 * @param left
 *        left channel gain.
 * @param right
 *        right channel gain.
 */
void
NmsSetGain( int left, int right )
{
	int gain[2];
	//LOCK_NMC();	
	gain[0] = left;
	gain[1] = right;
	DBGLOG("CLIENT: CMD_SET_GAIN.");
	sendNmsCmd(NULL, CMD_SET_GAIN, gain, sizeof(gain));
	//UNLOCK_NMC();	
}

/**
 * Get current recording time stamp.
 *
 * @return
 *         current recording time stamp in mili-seconds.
 */
int
NmsGetRecordtime( void )
{
	cmd_ret_data_t rdata;
	int t = -2;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_RECORD_TIME.");
	sendNmsCmd(&rdata, CMD_GET_RECORD_TIME, NULL, 0); 
	if (rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		t = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return t;
}

/**
 * Get current recording file size .
 *
 * @return
 *         current recording file size in byte.
 */
unsigned int
NmsGetRecordsize( void )
{
	cmd_ret_data_t rdata;
	unsigned int size = 0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_GET_RECORD_SIZE.");
	sendNmsCmd(&rdata, CMD_GET_RECORD_SIZE, NULL, 0); 
	if (rdata.data)
	{
		unsigned int * p;
		p = (unsigned int*)rdata.data;
		size = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return size;
}

/**
 * Return last NMS recording error code.
 * This is not thread-safe and the error is reset at each call of NmsRecord,
 * regardless of which client called it.
 * 
 * HACK: 
 * This is in reality a very ugly hack to be able to know what the hell happened when recording
 * ends because of an error or because of an hard limit of the system (such as FS limit, frames limit etc).
 * The ideal thing would be making NMS capable of sending out notifications, but 
 * that's a change i don't really have time to implement right now (even though we really 
 * need it also for other things, such as the timestamp notifications). --nerochiaro
 *
 * @param detail
 *        if not NULL on return it will contain more detailed error reporting
 * @return
 *        Last recording error message.
 */
NMS_SRV_RECORD_ERROR
NmsGetLastRecordError( NMS_SRV_ERROR_DETAIL *detail )
{
	cmd_ret_data_t rdata;
	NMS_SRV_RECORD_ERROR err = NMS_RECORD_SERVER_COMM;

	//LOCK_NMC();
	DBGLOG("CLIENT: CMD_GET_RECORD_ERROR.");

	if (!sendNmsCmd(&rdata, CMD_GET_RECORD_ERROR, NULL, 0)) return err;
	if (rdata.data == NULL) return err;
	
	err = ((NMS_SRV_ERROR_DETAIL*)rdata.data)->error;
	if (detail != NULL) 
	{
		memcpy(detail, rdata.data, sizeof(NMS_SRV_ERROR_DETAIL));
	}
	free(rdata.data);
	
	//UNLOCK_NMC();
	return err;
}


/**
 * Tell if recording is active.
 *
 * @return
 *         1 if recording is active, otherwise zero.
 */
int 
NmsIsRecording( void )
{
	cmd_ret_data_t rdata;
	int recording = 0;
	//LOCK_NMC();	
	DBGLOG("CLIENT: CMD_IS_RECORDING.");
	sendNmsCmd(&rdata, CMD_IS_RECORDING, NULL, 0);  
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		recording = *p;
		free(rdata.data);
	}
	//UNLOCK_NMC();	
	return recording;
}

/*------------------------------------------------------------------------*/
/*----------------- video monitor interface starts. ----------------------*/
/*------------------------------------------------------------------------*/
/**
 * Inform monitor that the calling process does not need the monitor to remain stopped anymore.
 * If there are no more processes requesting the monitor to remain stopped, then the monitor will restart.
 * If there are other processes that need the monitor to remain stopped, then monitor will NOT restart. This is 
 * not an error, but a normal condition. You cannot never force the monitor to restart.
 *
 *
 * WARNING: Since in Linux each thread is actually a different PID, you should be careful if you call this function from a
 * thread other than the main thread. Use NmsStartMonitorPID in that case and pass the PID of the main thread as argument.
 *
 * @return
 *      0 if restarted successfully.
 *      1 if not started because there were other processes requesting monitor to stay stopped.
 *      -1 on errors.
*/
int 
NmsStartMonitor(void)
{
	pid_t pid = getpid();
	return NmsStartMonitorPID(pid);
}

/**
 * Works the same as NmsStartMonitor except that lets you specify the PID of the requesting process instead of automatically
 * detecting it. Use this function when you need to stop the monitor from a thread different than the main application thread.
 *
 * @param pid The PID of the process requesting monitor to be restarted.
 */
int 
NmsStartMonitorPID( pid_t pid )
{
	cmd_ret_data_t rdata;
	int startmonitor = 0;

	DBGLOG("CLIENT: CMD_START_MONITOR.");
	sendNmsCmd(&rdata, CMD_START_MONITOR, &pid, sizeof(pid_t));
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		startmonitor = *p;
		free(rdata.data);
	}
	DBGLOG("video monitor started.\n");

	return startmonitor;
}


/**
 * Stop video monitor and add calling process to the list of processes that are requesting to keep the monitor stopped.
 * As long as you don't call NmsStartMonitor from the same process, the monitor will NEVER restart until the process exits.
 *
 * WARNING: Since in Linux each thread is actually a different PID, you should be careful if you call this function from a
 * thread other than the main thread. Use NmsStopMonitorPID in that case and pass the PID of the main thread as argument.
 */
void 
NmsStopMonitor( void )
{
	pid_t pid = getpid();
	NmsStopMonitorPID(pid);
}

/**
 * Works the same as NmsStopMonitor except that lets you specify the PID of the requesting process instead of automatically
 * detecting it. Use this function when you need to stop the monitor from a thread different than the main application thread.
 *
 * @param pid The PID of the process requesting monitor to stop.
 */
void 
NmsStopMonitorPID( pid_t pid )
{
	DBGLOG("CLIENT: CMD_STOP_MONITOR.");
	sendNmsCmd(NULL, CMD_STOP_MONITOR, &pid, sizeof(pid_t));
}

/**
 * Tell if monitor is enabled.
 *
 * @return
 *         1 if monitor is enabled, otherwise zero.
 */
int 
NmsIsMonitorEnabled( void )
{
	cmd_ret_data_t rdata;
	int enabled = 0;

	DBGLOG("CLIENT: CMD_IS_MONITOR_ENABLED.");
	sendNmsCmd(&rdata, CMD_IS_MONITOR_ENABLED, NULL, 0);  
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		enabled = *p;
		free(rdata.data);
	}
	return enabled;
}


/**
 * Initialize the frame capture.
 * You must call this function before being able to call any other
 * NmsCapture* function.
 *
 * @desc
 *            On input set the dimensions we require. It will return
 *            the real dimensions.
 *            After call this function please check this parameter, because
 *            they may not match.
 * @return
 *            On success return 0, and a negative value on failure.
 */


int     
NmsCaptureInit(capture_desc_t * cdesc)
{      
	cmd_ret_data_t rdata;
	capture_ret_t capret;
	int ret = -1;
	if(!cdesc)
		return ret;
	DBGLOG("CLIENT: CMD_CAP_INIT.");
	sendNmsCmd(&rdata, CMD_CAP_INIT, 
				&cdesc->capture_type,
				sizeof(NMS_CAPTURE_MODE_t));  
	if(rdata.data)
	{
		char * p;
		p = (char*)rdata.data;
		memcpy(&capret, p, sizeof(capture_ret_t));
		cdesc ->width = capret.width;
		cdesc ->height= capret.height;
		ret = capret.ret;
		free(rdata.data);
	}
	return ret;
}


/**
 * Capture the current frame from the capture port selected in NmsCaptureInput.
 * This function allocates memory, not the caller. The memory allocated by this
 * function must be freed by calling NmsCaptureReleaseFrame, or it will leak.
 *
 * @param fdesc
 *         The captured frame (data and memory size). fdesc.data will
 *         be NULL on failure
 * @return
 *         On success zero, a negative value on failure.
 */
int    
NmsCaptureGetFrame(frame_desc_t *fdesc)
{
	cmd_ret_data_t rdata;
	int ret = -1;		
	DBGLOG("CLIENT: CMD_CAP_GET_FRAME.1");
	if(!fdesc)
		return -10;
	fdesc->data = NULL;
	fdesc->size = 0;
	sendNmsCmd(&rdata, CMD_CAP_GET_FRAME,  NULL, 0);	
	if(rdata.data)
	{
		if(rdata.dataLen > sizeof(int))
		{
			char * buf = malloc(rdata.dataLen);
			if(buf)
			{
				fdesc->data = buf;
				char * p;
				p = (char*)rdata.data;
				memcpy(fdesc->data, p,rdata.dataLen);
				fdesc->size = rdata.dataLen;
				ret = 0;
			}
			else
			{
				WARNLOG("CLIENT: no enough memory");
				ret = -11;
			}
		}
		else
		{
			WARNLOG("CLIENT: get frame faile");
			ret = *(int*)rdata.data;
		}
		free(rdata.data);
	}
	return ret;
}



/**
 * Free the memory of the last frame that was captured. You should
 * call this after you are finished working with the frame's memory 
 * captured by NmsCaptureGetFrame or you
 * will get a memory leak.
 */
void   
NmsCaptureReleaseFrame(frame_desc_t *desc)
{
	if(!desc)
		return;
	if(desc->data)
	{
		free(desc->data);
		desc->data = NULL;
	}
}


/**
 * When you don't need to capture any more frames, this function must be called
 * to finalize the capture process and free resources.
 * If you don't call this function, you will not be able to start another capture session.
 *
 * @return
 *         0 on success, a negative value on failure
 */
int
NmsCaptureFinish(void)
{
	cmd_ret_data_t rdata;
	int ret = -1;
		
	DBGLOG("CLIENT: CMD_CAP_FINISH.");
	sendNmsCmd(&rdata, CMD_CAP_FINISH, NULL, 0);	
	if(rdata.data)
	{
		int * p;
		p = (int*)rdata.data;
		ret = *p;
		free(rdata.data);
	}
	return ret;
}



