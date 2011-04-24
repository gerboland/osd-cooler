/*
 *  Copyright(C) 2007 Neuros Technology International LLC. 
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
 * Neuros-Cooler av io buffer module header.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2007-04-19 JW 
 *
 */
#define USE_NMS_RECORD_BUFFER
#undef USE_NMS_PLAY_BUFFER
#define USE_NMS_BUFFER
#ifdef USE_NMS_BUFFER

#define NMS_USED_BUF_SHM_ID "nms_used_buf"
#define RECORD 1
#define PLAY 0

#include "av-double-buffer.h"

int NmsWriteToBuffer(int fd, void *buffer, size_t size);
int NmsBufferRecordSeek(int fd, off_t pos, int type);
off_t NmsBufferPlaySeek(int fd, off_t pos, int type);
ssize_t NmsReadFromBuffer(int fd, void *destBuf, size_t size);
int NmsBufferThreadStart(int type, int fd);
int NmsBufferThreadStop();

void NmsBufferAddDoubleBuffer(double_buffer_t *buffer);
void NmsBufferDelDoubleBuffers();

#endif
