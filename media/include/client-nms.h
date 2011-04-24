#ifndef NMS_CLIENT__H
#define NMS_CLIENT__H
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
 * Neuros-Cooler platform nms client module header.
 *
 * REVISION:
 *
 * 3) Changed Encoder controls interface, defined in com-nms.h, let the 
 *    application directly handle low-level parameters. --- 2006-08-18 MG
 * 2) Encoder interface definition.------------------------ 2006-01-06 MG
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
#include "nmsplugin.h"
#include "nc-type.h"
#include "com-nms.h"

/* client side APIs. */
int      NmsConnect(void);
void     NmsDisconnect(void);

char *   NmsGetVersion(void);
void	 NmsSetOutputMode(int);
void	 NmsSetInputMode(int);
void	 NmsSetOutputProportions(int);
int	 NmsGetOutputProportions(void);

/* decoder. */
int      NmsPlay(int, void *);
void	 NmsPauseUnpause(void);
void     NmsStopPlay(void);
void     NmsGetVolume(int *, int *);
void     NmsSetVolume(int, int);
int      NmsGetPlaytime(void);
int      NmsSeek(int);
int      NmsIsPlaying(void);
void     NmsFfRw(int);
void     NmsSfRw(int);
int		 NmsGetFfRwLevel(void);
int		 NmsGetSfRwLevel(void);
void	 NmsRepeatAB(int ab);
int 	 NmsGetRepeatABStatus(void);
void	 NmsFrameByFrame(int direction);

/* slide show */
int      NmsStartSlideShow(void);
void     NmsSetSlideShowImage(const char *);
void     NmsStopSlideShow(void);

int      NmsTrackChange(int);
int      NmsGetPlaymode(void);
void     NmsSetPlaymode(int);
void     NmsSetEditmode(int mode);
int      NmsGetRepeatmode(void);
void     NmsSetRepeatmode(int);
int      NmsGetFileIndex(void);
int      NmsGetTotalFiles(void);
int      NmsGetFilePath(int, char *);
void     NmsGetMediaInfo(const char *,void *);
int		 NmsGetSrvStatus(void);

/* encoder. */
NMS_SRV_RECORD_ERROR NmsRecord(rec_ctrl_t *, const char *, NMS_SRV_ERROR_DETAIL *);
void     NmsPauseRecord(int);
#define  NmsRestartRecord() NmsPauseRecord(0)
void     NmsStopRecord(void);
void     NmsGetGain(int *, int *);
void     NmsSetGain(int, int);
int      NmsGetRecordtime(void);
unsigned int NmsGetRecordsize(void);
int      NmsIsRecording(void);
NMS_SRV_RECORD_ERROR NmsGetLastRecordError( NMS_SRV_ERROR_DETAIL * );

/* video monitor. */
int      NmsStartMonitor(void);
int      NmsStartMonitorPID(pid_t pid);
void     NmsStopMonitor(void);
void     NmsStopMonitorPID(pid_t pid);
int     NmsIsMonitorEnabled(void);

/* capture. */
int   NmsCaptureInit(capture_desc_t * cdesc);
int   NmsCaptureGetFrame(frame_desc_t *fdesc);
void  NmsCaptureReleaseFrame(frame_desc_t *desc);
int   NmsCaptureFinish(void);


#endif /* NMS_CLIENT__H */
