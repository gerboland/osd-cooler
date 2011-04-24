#ifndef NMS_COMMON__H
#define NMS_COMMON__H
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
 * Neuros-Cooler platform nms client/plugin common header.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2006-08-18 MG 
 *
 */
#include "nc-type.h"

#define  NMS_isAllowMultipleInstances()   0
#define  NMS_SESSION_MAX                  8
#define  NMS_SOCKET_PATH_LEN_MAX          200

enum
  {
	/* NMS playback type. */
	NPT_FILE,
	NPT_DIR,
	NPT_DB,
	NPT_PLAYLIST,
	NPT_HISTORY,
	NPT_OTHER
  };

typedef enum {
	NMS_PLAYBACK_REPEAT_OFF,
	NMS_PLAYBACK_REPEAT_ON,
	NMS_PLAYBACK_REPEAT_A,
} NMS_SRV_PLAYBACK_REPEAT_STATUS_t;

/** NMS server status. */
typedef enum {
	/** output is locked, we can't play right now. try again later. */
	NMS_STATUS_OUTPUT_LOCKED = -3,
	/** nothing can be played back within current selections. */
	NMS_STATUS_NOT_PLAYABLE = -2,
	/** recorder stopped. */
	NMS_STATUS_RECORDER_STOPPED = -1,
	/** server running ok. */
	NMS_STATUS_OK	= 0,
	/** player stopped. */
	NMS_STATUS_PLAYER_STOPPED = 1,
	/** player's status is playing. */
	NMS_STATUS_PLAYER_PLAY = 2,
	/** player's status is pause. */
	NMS_STATUS_PLAYER_PAUSE = 3,
	/** player's status is fast forward. */
	NMS_STATUS_PLAYER_FF = 4,
	/** player's status is fast rewind. */
	NMS_STATUS_PLAYER_RW = 5,
	/** player's status is slow forward. */
	NMS_STATUS_PLAYER_SF = 6,
	/** player's status is frame-by-frame. */
	NMS_STATUS_PLAYER_VF = 7 ,
	/** player is to next file */
	NMS_STATUS_PLAYER_NEXT = 8
} NMS_SRV_STATUS_t;

/** Error codes related to recording. 
 * These can happen either when trying to start a recording, or later while it's running and it
 * stops without the client sending a NmsStopRecord command.
 */
typedef enum {
	// Any error code equal to this or not in this enum, is an unexpected error and ideally shouldn't happen
	NMS_RECORD_UNEXPECTED = -1,
	// Nothing wrong, recording started normally or finished normally
	NMS_RECORD_OK = 0,
	// Failed to send the request to NMS server or to receive a response
	NMS_RECORD_SERVER_COMM,

	// ** START and STOP errors **

	// Stopped "normally" because the encoder hit an internal hard limit (for example the imedia mp4 frame limit)
	NMS_RECORD_FRAME_LIMIT,
	// Recording stopped because otherwise we would hit the max file size limit (4Gb)
	NMS_RECORD_FILE_SIZE,
	// Recording stopped because otherwise we would hit the max time limit (4 hours)
	NMS_RECORD_FILE_LENGTH,
	// We ran out of RAM in linux. this is really a BAD thing.
	NMS_RECORD_OUT_MEMORY,
	// We don't have any more room on the target media to either start the recording, or to finalize it properly.
	NMS_RECORD_OUT_DISKSPACE,

	// ** STOP-only errors **

	// Recording stopped because committing a frame to disk failed for some other reason than the ones specified above.
	// TODO: find a way to split this in meaningful error codes based on the current plugin.
	NMS_RECORD_OTHER_COMMIT_ERROR,

	// ** START-only errors **

	// There's no encoder output plugin that can handle this kind of recording
	NMS_RECORD_PARAMS_UNSUPPORTED,
	// Other unrecoverable startup error. This may be split into more detailed errors late if needed 
	// for special handling/reporting.
	NMS_RECORD_START_OTHER

} NMS_SRV_RECORD_ERROR;

typedef struct {
	NMS_SRV_RECORD_ERROR error;
	// These are server or plugin-specific details, that have meaning only as an aid to debugging.
	// Please don't take any action based on the value of these fields, and instead add one value to the
	// NMS_SRV_RECORD_ERROR, which is the only return value that should drive program logic.
	int source;
	int message;
} NMS_SRV_ERROR_DETAIL;

/** Encoder controls structure.
 * all low-level parameters are exposed, if uncertain, parameters can be 
 * left as zero, nms system will pick up the proper default value.
 * Encoding file wrapper is determined by the file name passed in from
 * recorder interface.
 */
typedef struct 
  {
	/** video codec.*/
    int v_codec;
	/** video width. */
	int v_w;
	/** video height. */
	int v_h;
	/** video framerate. */
	int v_frame_rate;
	/** video index framerate. */
	int v_iframe_rate;
	/** video bitrate. */
	int v_bitrate;

	/** audio codec.*/
	int a_codec;
	/** audio samplerate.*/
	int a_samplerate;
	/** audio bitrate. */
	int a_bitrate;
	/** audio channels. */
	int a_num_channels;
    /** MP4 brand, currently not used when recording in ASF or AVI.  */
	int brand;
	/** input mode : 0 : ntsc, 1 : pal */
	int is_pal;
	/** reserved for future use, default zero. */
	int reserved[4];
  } rec_ctrl_t;

typedef struct
{
    unsigned int dataLen;
    void *       data;
} cmd_ret_data_t;


#endif /* NMS_COMMON__H */
