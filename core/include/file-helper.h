#ifndef FILE_HELPER__H
#define FILE_HELPER__H
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
 * Various file name/type helper module header.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2006-09-02 MG 
 *
 */

#include <stdio.h>
#include <stdlib.h>

typedef enum 
{
	oftFILE,
	oftDIRECTORY,
	oftIMAGE,
	oftAUDIO,
	oftVIDEO,
	oftUPK,
	oftEXECUTABLE,
	oftPLAYLIST,
	oftTHEME,
} EM_OSD_FILE_TYPE;

EM_OSD_FILE_TYPE  CoolOsdFileType(const char *name);
int    CoolIsVideoFile(const char *name);
int    CoolIsAudioFile(const char *name);
int    CoolIsAudioFileXmms2(const char *name);
int    CoolIsImageFile(const char *name);
int    CoolIsDirectory(const char *name);
int    CoolIsUpkFile(const char *name);
int    CoolIsMP4File(const char *name);
int    CoolIsExecFile(const char *name);
int    CoolIsNeuxExecFile(const char *name);
int    CoolIsOsdExecFile(const char *name);
int    CoolIsPlaylistFile(const char *name);
void CoolCheckAndCreateDirectory(const char *path);

int    CoolIsFilePresent(const char * name);
void   CoolAddSlashToPath(char *path,int bufsize);
char * CoolCatFilename2Path(char *       desbuf, 
							const int bufsize,
							const char * path, 
							const char * name);

char * CoolGetFileExtension(const char * name);
char * CoolGetFilenameFromPath(const char *fullname);
void   CoolGetPathFromFullname(const char *fullname,char *pathbuf,const int bufsize);
void   CoolGetRealnameOfFilename(const char *filename,char *realnamebuf,const int bufsize);
char * CoolMakeUniqueFilename(char *       destbuf,
							  const int bufsize,
							  const char * dir,
							  const char * prefix,
							  const char * extname,
							  int          slen,
							  int          useyear,
							  int          usemon,
							  int          useday);
char * CoolStrSlashPreserve(const char *src);
char * CoolStringTrim(const char *src);
int    CoolIsFileInUse(const char *fullname);
int    CoolIsFileIllegal(const char *filename);
int    CoolIsFileHidden(const char *filename);
void CoolParseArgsFromString(const char *string, int *argc, char ***argv);
void CoolFreeParsedArgs(int argc, char ***argv);


#endif /* FILE_HELPER__H */
