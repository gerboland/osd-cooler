#ifndef NMSPLUGIN__H
#define NMSPLUGIN__H
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
 * plugin module header.
 *
 * REVISION:
 *
 *12) API change: add propotions support to output plugin init 2008-04-10 nerochiaro
 *11) API change: enc requirements and enc finish return value 2007-10-30 nerochiaro
 *10) API change: to support getting bytes in video out buf 2007-09-03 nerochiaro
 * 9) API change: output mode change behavior also depends
 *    on whether we need to track video input or not. ----- 2007-08-07 MG
 * 8) API change: to support getting bytes in audio out buf 2007-06-15 nerochiaro
 * 7) API change: added input plugin capability support. ---2007-06-08 MG
 * 6) DM320 lock handling.--------------------------------- 2007-05-23 nerochiaro
 * 5) New simple interface definition.--------------------- 2006-03-11 MG
 * 4) Added encoding support.------------------------------ 2006-01-06 MG
 * 3) Added audio stream plugin support. ------------------ 2005-11-25 MG
 * 2) Added video/audio parameters structure definition. -- 2005-11-07 MG
 * 1) Initial creation. ----------------------------------- 2005-09-23 MG 
 *
 */
#if BUILD_TARGET_ARM
    #include <linux/iencode.h>
    #include <linux/idecode.h>
    #include <linux/iqueue.h>
#endif
#include "com-nms.h"
#include <stdint.h>

#define NMS_PLUGIN_API_VERSION "1.2.0"

#define DM320_MEDIA_SEM_PATH "/var/tmp/sem-dm320-media"
#define DM320_AUDIO_SEM_ID   'a'
#define DM320_VEDEO_SEM_ID   'v'
#define DM320_IMAGE_SEM_ID   'i'

#define TV_INPUT_WIDTH 640
#define NTSC_TV_INPUT_HEIGHT 480
#define PAL_TV_INPUT_HEIGHT 576
#if 0
#ifndef BOOL
        #define BOOL  unsigned char
#endif
#ifndef TRUE
        #define TRUE  1
#endif
#ifndef FALSE
        #define FALSE 0
#endif
#endif

#ifndef _TAG_LENGTH_
        #define _TAG_LENGTH_
        #define MAX_TAG_GENRE_LENGTH        51
        #define MAX_TAG_TITLE_LENGTH        101
        #define MAX_TAG_ARTIST_LENGTH       101
        #define MAX_TAG_ALBUM_LENGTH        101
        #define MAX_TAG_COMMENT_LENGTH      101
        #define MAX_TAG_YEAR_LENGTH         11
#endif

#define NMS_PLUGIN_SYMBOL_INPUT      "GetNmsInputPlugin"
#define NMS_PLUGIN_SYMBOL_OUTPUT     "GetNmsOutputPlugin"
#define NMS_PLUGIN_SYMBOL_AUDIOENC   "GetNmsAudioEncodePlugin"
#define NMS_PLUGIN_SYMBOL_AUDIODEC   "GetNmsAudioDecodePlugin"
#define NMS_PLUGIN_SYMBOL_ENCINPUT   "GetNmsEncInputPlugin"
#define NMS_PLUGIN_SYMBOL_ENCOUTPUT  "GetNmsEncOutputPlugin"
#define NMS_PLUGIN_SYMBOL_CAPTURE    "GetNmsCapturePlugin"

/** Plugin types. */
enum
  {
	NMS_PLUGIN_MULTIMEDIA = 1,
	NMS_PLUGIN_UNKNOWN = 0
  };

/** Neuros Capture types. */
typedef enum
  {
	NMS_CAPTURE_INPUT,
	NMS_CAPTURE_PLAYBACK
  } NMS_CAPTURE_MODE_t;

/** Neuros media server audio codec types. */
typedef enum
  {
	/** x_ARM_x stands for ARM side codec.*/
	NMS_AC_ARM_AAC,
	NMS_AC_ARM_AC3,
	NMS_AC_ARM_BSAC,
	NMS_AC_ARM_G711,
	NMS_AC_ARM_G722,
	NMS_AC_ARM_G7221,
	NMS_AC_ARM_G723,
	NMS_AC_ARM_G728,
	NMS_AC_ARM_G729,
	NMS_AC_ARM_GSMAMR,
	NMS_AC_ARM_MP1,
	NMS_AC_ARM_MP2,
	NMS_AC_ARM_MP3,
	NMS_AC_ARM_OGG,
	NMS_AC_G726,
	NMS_AC_PCM,
	NMS_AC_ARM_WMA,
    NMS_AC_NO_AUDIO
  } NMS_AUDIO_t;

/** MP4 brand */
typedef enum {
   /** ISO compliant. */
   NMS_MP4C_BRAND_ISO = 0,
   /**SKM compliant. */
   NMS_MP4C_BRAND_SKM = 1,
   /** Sony Portable PlayStation compliant. */
   NMS_MP4C_BRAND_PSP = 2,
   /** 3GP compliant. */
   NMS_MP4C_BRAND_3GP = 4
}  NMS_MP4C_BRAND_t;


/** Neuros media server video codec types. */
typedef enum 
  {
	NMS_VC_DIVX,
	NMS_VC_H263,
	NMS_VC_H264,
	NMS_VC_JPEG,
	NMS_VC_MJPEG,
	NMS_VC_MPEG1,
	NMS_VC_MPEG2,
	NMS_VC_MPEG4,
	NMS_VC_MSMPEG4,
	NMS_VC_WMV7,
	NMS_VC_WMV8,
	NMS_VC_WMV9,
	NMS_VC_YUV422,
	NMS_VC_RGB888,
	NMS_VC_NO_VIDEO
  } NMS_VIDEO_t;

/** Neuros media server subtitle types. */
typedef enum
  {
	NMS_SC_DIVX,
	NMS_SC_NO_SUBTITLE
  } NMS_SUBTITLE_t;

/** Neuros media server wrapper types. */
typedef enum
  {
	NMS_WP_ASF,
	NMS_WP_AVI,
	NMS_WP_MP4,
	NMS_WP_MPG,
	NMS_WP_OGM,
	NMS_WP_ADO,
	NMS_WP_INVALID
  } NMS_FILE_t;

typedef enum
  {
	NMS_BUFFER_AUDIO,
	NMS_BUFFER_VIDEO
  } NMS_OUTPUT_BUFFER_t;

#if BUILD_TARGET_ARM
/** queue entry structure. */
typedef struct iqueue_entry q_buf_t;
#else
typedef int    q_buf_t;
#endif

/** audio description. */
typedef struct
  {
	/** Audio Type. */
    NMS_AUDIO_t audio_type;
    /** Number of Channels. */
    uint32_t num_channels;
    /** Sample Rate. */
    uint32_t sample_rate;
    /** Bitrate kbps. */
    uint32_t bitrate;
	/** WMA Encode Options. */
    uint32_t wma_encode_opt;
    /** WMA Block Align Parameters. */
    uint32_t wma_block_align;
} aud_desc_t;	

/** capture description. */
typedef struct
  {
	/** Capture Type. */
    NMS_CAPTURE_MODE_t capture_type;
    /** width */
    int width;
    /** height */
    int height;
} capture_desc_t;

/** capture return value description. */
typedef struct
  {
	/** Capture return value. */
    int ret;
    /** width */
    int width;
    /** height */
    int height;
} capture_ret_t;

/** frame description. */
typedef struct
  {
	/** frame data. */
    char* data;
    /** size */
    uint32_t size;
} frame_desc_t;


/** video description. */
typedef struct 
  {
	/** Video Type. */
    NMS_VIDEO_t video_type;
    /** Width. */
    int width;
    /** Height. */
    int height;
	/** frame rate. */
	int frame_rate;
	/** video bitrate. */
	int bitrate;
	/** index frame rate. */
	int iframe_rate;
	/**
	 * capture_port , 0 : ntsc input, 1 : pal input 
	 */
	int capture_port;
	/** brand of mp4 file */
	NMS_MP4C_BRAND_t brand;
} vid_desc_t;

/** subtitle description. */
typedef struct
  {
	/** type. */
	NMS_SUBTITLE_t subtitle_type;
} sub_desc_t;

/** media description. */
typedef struct
  {
	aud_desc_t adesc;
	vid_desc_t vdesc;
	sub_desc_t sdesc;
	NMS_FILE_t ftype;
} media_desc_t;

/** Media buffer structure. */
typedef struct
  {
	/** video stream buffer. */
	q_buf_t    vbuf;
	/** audio stream buffer. */
	q_buf_t    abuf;
	/** subtitle stream buffer. */
	q_buf_t    sbuf;
	/** Current media buffer pointer.
	 * points to one of the above buffers to indicate
	 * where the data should be routed to. 
	 * In decoding mode, output plugin relies on this information to 
	 * write data to HW.
	 */
	q_buf_t *  curbuf; 
} media_buf_t;

/** Media meta info structure. */
typedef struct
  {
	char   available;
	char   title[MAX_TAG_TITLE_LENGTH];
	char   artist[MAX_TAG_ARTIST_LENGTH];
	char   album[MAX_TAG_ALBUM_LENGTH];
	char   genre[MAX_TAG_GENRE_LENGTH];
	char   comment[MAX_TAG_COMMENT_LENGTH];
	char   year[MAX_TAG_YEAR_LENGTH];
	int    bps;
	long   duration;
	long   bytes;
} media_info_t;

/** input plugin capability structure. */
typedef struct
{
	int   can_fwd;
    int   can_rwd;
} input_capability_t;

/** encoder output plugin requirements structure */
typedef struct
{
	// The amount of free disk space (on the designated recording media) that the plugin 
	// needs for scratch buffers (if any). Note that this is amount is valid at the time of the
	// call to the function that fills this structure, and may change if media is manipulated
	// afterwards (for example by deleting pre-existing scratch buffers that would otherwise be re-used).
	unsigned int disk_scratch_space;

	// The amount of free disk space that this plugin needs to finalize correctly a recording.
	// If this amount of space is not available at the moment of finalization, the file will likely 
	// be unplayable by most players.
	// This is *in addition* to any space reserved by the disk_scratch_space member above.
	unsigned int finalization;

	// Reserved since we may want to return other kind of plugin-specific requirements (like runtime buffer 
	// size or whatever else) without messing too much with this structure.
	unsigned int reserved[2];
} encoding_requirements_t;

/** Multimedia decoder output device plugin.*/
typedef struct
  {
	/** plugin API version. */
	const char *ver;

	/** plugin name. -- one line brief description. */
	const char *brief;

	/** plugin detailed description.*/ 
	const char *description;

    /** plugin type, see above plugin type definitions. */
    const int type;

	/** initialize interface. init output mode, if mdesc == NULL
    * mode: 0 for NTSC and 1 for PAL
    * proportions: 0 for normal (4:3), 1 for widescreen (16:9)
    * It returns 0 on success, -1 on errors.
    * If the DM320 I/O lock is exclusively held by someone else, it will return 1.
    */
	int            (*init) (const media_desc_t * mdesc, int mode, int proportions);

	/** finish interface, wait in miliseconds. */
	void           (*finish) (int wait);

	/** get volume.	*/
	void           (*getVolume) (int *l, int *r);

	/** Set volume. */
	void           (*setVolume) (int l, int r);

	/** start device. */
 	int            (*start) (void);

    /** get media buffer.
	 * if preload = 1, logic needs to be built in to detect whether all 
	 * buffers are full.
	 * return 0: if buffer is available. 1: if all buffers are full.
	 * return negative value if error happened while polling.
     */
    int            (*getBuffer) (media_buf_t * buf, int timeout, int preload);

	/** input plugin calls this to write data to the output buffer. */
	void           (*write) (const media_buf_t * buf);

	/** flush the buffer
	 * and set the plugins internal timers to time 
	 * in ms.
	 */
	void           (*flush) (int time);	
	
	/** pause or unpause the output. */
	void           (*pause) (int pause);

	/** mute or unmute the output. */
	void           (*mute) (int mute);
	
	/** Is playback active.
	 * returns TRUE if the plugin currently is playing some audio,
	 * otherwise return FALSE 
	 */
	int            (*isBufferPlaying) (void);

	/** return the current playing time in ms. */
	int            (*outputTime) (void);

	
	/** Set output mode, mode: 0:NTSC, 1:PAL 
	 *      tracking_video_in: 0:to set output mode for video decoding only. 
	 *                         1:to set output mode to match input video.
	 */
	int            (*setOutputMode) (int mode, int tracking_video_in);

	/** Return the amount of data in audio or video output buffer. */
	unsigned long  (*bufferedData) (NMS_OUTPUT_BUFFER_t buffer);

	/** Set proportions of video output.
	 *  proportions : 0: normal (4:3), 1: widescreen (16:9)
	 *  return: 0 for success, negative values for errors
	 */
	int            (*setProportions) (int proportions);

} media_output_plugin_t;

/** Multimedia decoder input device plugin. */
typedef struct
  {
	/** version.    -- x.y.z */
	const char *ver;

	/** plugin name. -- one line brief description. */
	const char *brief;

	/** plugin detailed description. */
	const char *description;

    /** plugin type. */
    const int type;

	/** Check what file it is. 
	 * return nonzero if the plugin can handle the file.
	 * return code: 0: not our file
	 *              1: is our file
	 */
	int            (*isOurFile)      (const char *filename );

	/** get media info, this shall be a stand-alone handle. 
	 *  return code : 0: successful.
	 *                nonzero: failed.
	 */	
	int            (*getInfo)        (const char *filename, media_info_t * minfo);

    /** get input plugin capability.
	 *
	 *  return code : 0: successful.
	 */
    int            (*getCapability)  (input_capability_t * cap);

	/** initialize interface
	 * return 0 if successful. 1 if dm320 locked. otherwise errors.
	 */
	int            (*init)           (const char *filename, media_desc_t * mdesc);

	/** finish interface.
	 * release allocated data objects if any.
	 */
	void           (*finish)         (void);

	/** start the plugin
	 * shall put the plugin into a stream ready state. 
	 * return 0 if successful.
	 */
	int            (*start)          (const char *filename);

	/** seek to given time in ms.
	 * return actual time stamp if successful, otherwise negative value.  
	 */
	int            (*seek)           (int time);

	/** get media data
	 * return length in bytes, will set or clear end of file flag. 
	 */
    int            (*getData)        (media_buf_t *buf, int * eof);
} media_input_plugin_t;


/** Audio stream decoder device plugin. */
typedef struct
  {
	/** version.    -- x.y.z */
	const char *ver;

	/** plugin name. -- one line brief description. */
	const char *brief;

	/** plugin detailed description. */
	const char *description;

    /** plugin codec. */
    const int codec;

	/** initialize interface
	 * the rest of the handles can only be called
	 * after this.
	 */
	int            (*init)           (const aud_desc_t * adesc);

	/** finish interface. */
	void           (*finish)         (void);

	/** decode stream data, return length in bytes. */
    int            (*decode)         (const q_buf_t * input,
									  q_buf_t * output);

} audio_decode_plugin_t;

/* Each encoder output plugin can return any error code it wants from its functions, but unfortunately there are some of
	these that we need to handle specially in the calling application. Thus the rule is that each enc output plugin
	can still return any error code it wants, but error codes larger than 0x0800 are reserved and registered in this
	enum and have a special meaning for caller application (more than one plugin can use them, if appropriate.
	So generally speaking always return positive values less than 0x0800 for "normal" error codes.
*/
typedef enum
{
	KNOWNERR_BASE = 0x0800,
	KNOWNERR_MAX_FRAMES_LIMIT
} well_known_error_codes;

/** Multimedia encoder output device plugin. */
typedef struct
  {
	/** version.    -- x.y.z */
	const char *ver;

	/** plugin name. -- one line brief description. */
	const char *brief;

	/** plugin detailed description. */
	const char *description;

    /** plugin type, see above plugin type definitions. */
    const int type;

	/** Check if this plugin supports encoding with the specified parameters.
	 * NOTE: Please notice that calling this function is NOT optional. Even if you
	 * are absolutely sure that a certain plugin supports the parameters, you should
	 * still call this function.
	 * FIXME: in other words, this should really be named something like setParametersAndCheck,
	 * because in fact it does initialize the recording parameters of the plugin, besides doing
	 * the compatibility check.
	 *
	 * return code: 0: plugin does not support this kind of recording
	 *              1: plugin supports this kind of recording 
	 */
	int            (*isOurFormat)    (const rec_ctrl_t * ctrl,
									  const char *filename,
									  media_desc_t * mdesc);

	/** Initialize interface
	 * Return 0 if successful.
	 * Other values represent plugin-specific errors that may be useful to report in order to aid debugging.
	 */
	int            (*init)           (void);

	/** start the plugin, waiting for data stream. 
	 * return 0 if successful.
	 */
	int            (*start)          (void);

	/** finish interface.
	 * Perform encoded data finalization and release allocated data objects if any.
	 */
	int           (*finish)         (void);

	/** commit media data.
	 * return 0 if successful, an error code dependent on plugin otherwise.
	 */
	int            (*commit)         (const media_buf_t * buf);

	/** Query the requirements that this plugin has to be able to succesfully encode with 
	 * the parameters that were passed to isOurFormat.
	 * See the comments on the encoding_requirements_t structure for more info.
	 * @param requirements
	 *			 will be filled with the data upon return.
	 */
	void           (*getRequirements) (encoding_requirements_t * requirements);

} media_enc_output_plugin_t;

/** Multimedia encoder input device plugin. */
typedef struct
  {
	/** version.    -- x.y.z */
	const char *ver;

	/** plugin name. -- one line brief description. */
	const char *brief;

	/** plugin detailed description. */
	const char *description;

    /** plugin type. */
    const int type;

	/** initialize interface
	 * return 0 if successful, 1 if dm320 locked, otherwise errors
	 */
	int            (*init)           (const media_desc_t * mdesc, int is_pal);

	/** start the plugin
	 * put the plugin into a stream ready state. 
	 * return 0 if successful.
	 */
	int            (*start)          (void);

	/** finish interface.
	 * release allocated data objects if any.
	 */
	void           (*finish)         (void);

    /** get audio buffer.
	 * return 0: if buffer is available.
	 * return negative value if error happened while polling.
     */
    int            (*getAudioBuffer) (media_buf_t * buf, int timeout);

    /** get video buffer.
	 * return 0: if buffer is available.
	 * return negative value if error happened while polling.
     */
    int            (*getVideoBuffer) (media_buf_t * buf, int timeout);

	/** input plugin calls this to release audio buffer. */
	void           (*putAudioBuffer) (const media_buf_t * buf);	

	/** input plugin calls this to release video buffer. */
	void           (*putVideoBuffer) (const media_buf_t * buf);	

	/** get gain. */
	void           (*getGain)        (int *l, int *r);

	/** Set gain. */
	void           (*setGain)        (int l, int r);	
} media_enc_input_plugin_t;


/** Audio stream encoder device plugin. */
typedef struct
  {
	/** version.    -- x.y.z  */
	const char *ver;

	/** plugin name. -- one line brief description. */
	const char *brief;

	/** plugin detailed description. */
	const char *description;

    /** plugin codec. */
    const int codec;

	/** initialize interface
	 * the rest of the handles can only be called
	 * after this.
	 */
	int            (*init)           (const aud_desc_t * adesc);


	/** finish interface. */
	void           (*finish)         (void);

	/** encode stream data, return length in bytes. */
    int            (*encode)         (const q_buf_t * input,
									  q_buf_t * output);


} audio_encode_plugin_t;


/** Video frame capture plugin */
typedef struct
  {
	/** version.    -- x.y.z  */
	const char *ver;

	/** plugin name. -- one line brief description. */
	const char *brief;

	/** plugin detailed description. */
	const char *description;

	/** initialize interface
	 * the rest of the handles can only be called
	 * after this.
	 */
	int  (*init)         (capture_desc_t * cadesc);
	
	/*get frame buf  interface
	*/
	int  (*getframe)     (frame_desc_t * fdesc);
	
	/*release frame buf  interface*/
	int (*releaseframe) (void);

	/** finish interface. */
	int  (*finish)       (void);

} media_capture_plugin_t;

typedef media_input_plugin_t * (*GetNmsInputPluginFunc)(void);
typedef media_output_plugin_t * (*GetNmsOutputPluginFunc)(void);
typedef audio_decode_plugin_t * (*GetNmsAudioDecodePluginFunc)(void);
typedef audio_encode_plugin_t * (*GetNmsAudioEncodePluginFunc)(void);
typedef media_enc_input_plugin_t * (*GetNmsEncInputPluginFunc)(void);
typedef media_enc_output_plugin_t* (*GetNmsEncOutputPluginFunc)(void);
typedef media_capture_plugin_t*    (*GetNmsCapturePluginFunc)(void);


#endif /* NMSPLUGIN__H */
