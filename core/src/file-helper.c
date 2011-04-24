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
 * Various file type/name helper routines.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2006-09-02 MG
 *
 */
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <linux/leds.h>
#include <limits.h>
#include <ctype.h>

#include "nc-type.h"

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "file-helper.h"
#include "list.h"
#include "run.h"
#include "stringutil.h"

static const char * const video[] =
  {
  	".3g2",
	".3gp",
	".avi",
	".gvi",
	".mp4",
	".mpg",
	".wmv",
	".asf",
	".divx",
	".mpeg",
	".dat",
	".mov",
	".asx",
	".dvr-ms",
	".ifo",
	".m1v",
	".m2v",
	".m4e",
	".m4v",
	".moov",
	".movie",
	".mpe",
	".mpv2",
	".mswmm",
	".prx",
	".qt",
	".qtch",
	".rts",
	".vfw",
	".vid",
	".vob",
	".vro",
	".wm",
	".flv",
	NULL
  };

static const char * const audio[] =
  {
	".mp3",
	".ogg",
	".wav",
	".flac",
	".aac",
	".m4a",
	".ac3",
	".wma",
	NULL
  };

static const char * const audio_xmms2[] =
  {
	".mp3",
	".ogg",
	".flac",
	".wav",
	".aac",
	".m4a",
	".wma",
	NULL
  };

static const char * const image[] =
  {
	".gif",
	".bmp",
	".jpg",
	".jpeg",
	".tif",
	NULL
  };

static const char * const upk[] =
  {
	".upk",
	NULL
  };

static const char * const neux_exec[] =
  {
	".nx",
	NULL
  };

static const char * const osd_exec[] =
  {
        ".lua",
        ".sh",
	".ox",
	NULL
  };

static const char * const exec[] =
  {
	".lua",
	".ox",
	".sh",
	".nx",
	NULL
  };

static const char * const playlist[] =
  {
	".m3u",
	".pls",
	NULL
  };

static const char * const MP4[] =
  {
	".mp4",
	NULL
  };

static int isTheFile(const char * name, const char * const * p)
{
    char * ext;

	ext = strrchr(name, '.');
	if(NULL==ext) return 0;

	while(*p)
	  if (!strcasecmp(ext, *p++)) return 1;

	return 0;
}


/** is it our video file. */
int CoolIsVideoFile(const char *name)
{
    return isTheFile(name, &video[0]);
}

/** is it our audio file. */
int CoolIsAudioFile(const char *name)
{
    return isTheFile(name, &audio[0]);
}

/** is it an audio file that xmms2 can handle. */
int CoolIsAudioFileXmms2(const char *name)
{
    return isTheFile(name, &audio_xmms2[0]);
}

/** Is it a theme file */
int CoolIsThemeFile(const char *name)
{
	char *n;
	n = strrchr(name, '/');
	if (NULL == n) return 0;
	if (!strcasecmp(n, "/theme.ini")) {
		return 1;
	}
	return 0;
}

/** is it our image file. */
int CoolIsImageFile(const char *name)
{
    return isTheFile(name, &image[0]);
}

/** is it a directory. */
int CoolIsDirectory(const char *name)
{
    struct stat st;
	if ( 0 == stat(name, &st) ) 
	  {
		if ( S_ISDIR(st.st_mode) ) 
		  {  
			return 1;
		  }
	  }
	return 0;
}

/** is it a upk file. */
int CoolIsUpkFile(const char *name)
{
    return isTheFile(name, &upk[0]);
}

/** is it our MP4 file. */
int CoolIsMP4File(const char *name)
{
    return isTheFile(name, &MP4[0]);
}

/** is it an executable file. */
int CoolIsExecFile(const char *name)
{
    return isTheFile(name, &exec[0]);
}

/** is it a Neux executable file. */
int CoolIsNeuxExecFile(const char *name)
{
    return isTheFile(name, &neux_exec[0]);
}

/** is it a normal executable file. */
int CoolIsOsdExecFile(const char *name)
{
    return isTheFile(name, &osd_exec[0]);
}

/** is it a playlist file. */
int CoolIsPlaylistFile(const char *name)
{
    return isTheFile(name, &playlist[0]);
}

/** is file present. */
int CoolIsFilePresent(const char * name)
{
	struct stat st;

	return !stat(name, &st);
}

/** check OSD file type. 
 *
 * @param name
 *        file name.
 * @return
 *        OSD file type if successful, otherwise -1.
 *
 */
EM_OSD_FILE_TYPE CoolOsdFileType(const char *name)
{
	struct stat		statbuf;
	
	//FIXME: ugly, a better algorithm is needed here, what about ftp? https?.
	if ((*(name+0) == 'h' &&
		 *(name+1) == 't' &&
		 *(name+2) == 't' &&
		 *(name+3) == 'p' &&
		 *(name+4) == ':') ||
		(*(name+0) == 'f' &&
		 *(name+1) == 't' &&
		 *(name+2) == 'p' &&
		 *(name+3) == ':'))
		goto skip_local_check;

	if (stat(name, &statbuf)<0)
	{
		WARNLOG("file status error: %s", name);
		return -1;
	}
	else
	{
		if (S_ISDIR(statbuf.st_mode))//S_ISREG
		{
			return oftDIRECTORY;
		}
		else
		{
		skip_local_check:
			if (CoolIsImageFile(name))
				return oftIMAGE;
			else if (CoolIsAudioFile(name))
				return oftAUDIO;
			else if (CoolIsVideoFile(name))
				return oftVIDEO;	
			else if (CoolIsUpkFile(name))
				return oftUPK;
			else if (CoolIsExecFile(name))
				return oftEXECUTABLE;
			else if (CoolIsPlaylistFile(name))
				return oftPLAYLIST;
			else if (CoolIsThemeFile(name))
				return oftTHEME;
		}
	}	
	return oftFILE;
}

/*make sure the path end of slash
 *
 */
void
CoolAddSlashToPath(char *path,int bufsize)
{
	if(!path || bufsize <=2) 
		return;
	
	int len=strlen(path);
	if(len == 0)
	{
		*path++ = '/';
		*path = '\0';
		return;
	}
	if(len+1 >= bufsize)
		return;

	if (*(path+len-1)!='/')
	{
		*(path+len) = '/';
		*(path+len+1) = 0;
	}
}

/** concatenate filename to given path, insert '/' if needed. 
 *
 */
char * CoolCatFilename2Path(char *       destbuf, 
						const int bufsize,
						const char * path, 
						const char * filename)
{
	strlcpy(destbuf,path,bufsize);
	CoolAddSlashToPath(destbuf,bufsize);
	strlcat(destbuf,filename,bufsize);
	return destbuf;
}

/** get the file extension if available. 
 *
 */
char * CoolGetFileExtension(const char * name)
{
    return strrchr(name, '.');
}

/** get filename off the full path. 
 *
 */
char * CoolGetFilenameFromPath(const char *fullname)
{
    char * name;

	name = strrchr(fullname, '/');
	if (NULL == name)
	  name = (char*)fullname-1;

	return name+1;
}

/** get path off the fullname. 
 *
 */
void CoolGetPathFromFullname(const char *fullname,char *pathbuf,const int bufsize)
{
	char *tmp;

	strlcpy(pathbuf, fullname, bufsize);
	tmp = strrchr(pathbuf, '/');
	if (NULL != tmp)
		*tmp='\0';
}

/** get realname off the filename. 
 *
 */
void CoolGetRealnameOfFilename(const char *filename,char *realnamebuf,const int bufsize)
{
	char *tmp;

	strlcpy(realnamebuf, filename, bufsize);
	tmp = strrchr(realnamebuf, '.');
	if (tmp != NULL) *tmp = '\0';
}


/** make unique file/directory name. 
 *
 */
//FIXME: keep trying unless there is no disk space left.
char *
CoolMakeUniqueFilename(char *       destbuf,
				   const int bufsize,
				   const char * dir,
				   const char * prefix,
				   const char * extname,
				   int          slen,
//TODO: remove these three following parameters, they are currently ignored
				   int          useyear,
				   int          usemon,
				   int          useday)
{
	char *filename;
	char str[20];
	char str1[20];
	char str2[PATH_MAX];
	int i;
	int count;

	filename=destbuf;

	if(CoolIsFilePresent(dir)==0)
	{
		if(CoolRunCommand("mkdir","-p",dir,NULL)!=0)
			DBGLOG("Can't create folder %s!\n",dir);
	}
	
	CoolCatFilename2Path(str2,PATH_MAX,dir,prefix);

	if(slen<1)
		slen=1;
	snprintf(str1,20,"%%0%dd",slen);
	for(i=1,count=1;i<=slen;i++)
		count=count*10;
	for(i=1;i<count;i++)
	{
		snprintf(str,20,str1,i);
		strlcpy(filename,str2,bufsize);
		strlcat(filename,str,bufsize);
		strlcat(filename,extname,bufsize);

		if(!CoolIsFilePresent(filename))
			return filename;

	}
	return NULL;
}

/** transferred meaning symbol. 
 *	it will add \ to src string for each chars.
 * @param src
 *        src string will be transfer.
 * @return
 *        result, slash preserve string. 
 *	NOTE: caller must free result, because it is malloc in function inside.
 */
char *
CoolStrSlashPreserve(const char *src)
{
    int i, j;
    char * dst;

    for( i = 0, j = 0 ; src[ i ] ; ++i )
        if( src[ i ] == '\'' ) ++j;

    /* original length
       + 3 extra chars per escaped quote
       + 2 chars for surrounding quotes
       + NUL byte */
    dst = malloc( i + j * 3 + 3 );

    if( ! dst ) return NULL;

    dst[ 0 ] = '\'';

    for( i = 1, j = 0; ( dst[ i ] = src[ j ] ); ++i, ++j )
        if( '\'' == dst[ i ] ) {
            dst[ ++i ] = '\\';
            dst[ ++i ] = '\'';
            dst[ ++i ] = '\'';
        }

    dst[ i++ ] = '\'';
    dst[ i++ ] = '\0';

    return dst;

}


/** Trim string space at head and at end
 * @param src
 *        src string will be transfer.
 * @return
 *        result, trim string. 
 *	NOTE: caller must free result, because it is malloc in function inside.
 */
char * CoolStringTrim(const char *src)
{
	int i, j, len;
	char *dst = NULL;
	
	if (!src) goto bail;
	
	len = strlen(src);	
	for (i = 0; i <= len - 1; i++)
	{
		if (!isspace(src[i])) break;
	}
	if (i != len)
	{
		for (j = len - 1; j >= 0; j--)
		{
			if (!isspace(src[j])) break;
		}
		len = j - i + 1;
		dst = (char *)malloc(len + 1);
		strncpy(dst, src + i, len);
		dst[len] = '\0';
		return dst;
	}
bail:
	dst = strdup("");
	return dst;
}

/**Check if the file is in use.
 * @param fullname
 * 		  fullname of the file to check
 * @return
 *        if the file in use return 1, otherwise return 0  
 */
int   
CoolIsFileInUse(const char *fullname)
{	
	return !CoolRunCommand("fuser",fullname,NULL);
}

int CoolIsFileIllegal(const char *filename)
{
	int temlen;
	if(filename==NULL) 
		return 1;
	
	temlen=strlen(filename);
	if(temlen==0) 
		return 1;
	
	return strcspn(filename, "/\\?:|*<>\"") != temlen;
}

int CoolIsFileHidden(const char *filename)
{
	return *filename == '.';
}

void CoolCheckAndCreateDirectory(const char *path)
{
	struct stat st;
	char tmp[PATH_MAX];
	char *tmpstr;
	size_t sz;

	//initial checks for safety and in case we don't need to do anything
	if (path == NULL) return;
	sz = strlen(path);
	if (sz == 0 || (sz == 1 && path[0] == '/')) return; //no need to check the existence of root
	if (!stat(path, &st)) return;  //do nothing if path already exists completely

	strlcpy(tmp, path, sizeof(tmp)); //copy string to internal buffer so we can manipulate it

	// Loop over each path piece, skipping the root.
	tmpstr = (tmp[0] == '/') ? tmp + 1 : tmp;
	tmpstr = strchr(tmpstr, '/');
	while (tmpstr)
	{
		//temporarily terminate the string, so stat can check
		*tmpstr = '\0';

		if (stat(tmp, &st))
			mkdir(tmp, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

		//restore the string and go on checking
		*tmpstr = '/';
		tmpstr = strchr(tmpstr + 1, '/');
	}
	mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

/*
 *Function Name:CoolParseArgsFromString
 *
 *Parameters:
 * string------shell command don't include application name
 * argc------parameter count
 * argv------parameter array, parse result, caller must be call CoolFreeParsedArgs to free it
 *
 *Description:
 * translate shell command into application parameter
 *
 *Returns:
 *
 */
void CoolParseArgsFromString(const char *string, int *argc, char ***argv)
{
	char *str = (char*)string;
	BOOL flag = FALSE;
	int curpos = 0;
	char tempbuf[PATH_MAX];
	slist_t *argvlist = NULL;
	char *data = NULL;

	*argv = NULL;
	*argc = 0;
	while( *str )
	{
		if( *str == '\'' && *(str+1) == '\\' )
		{
			tempbuf[curpos++] = '\'';
			str += 4;
		}
		else
		{
			if( flag == FALSE )
			{
				if( *str == '\'' )
				{
					flag = TRUE;
				}
				else
				{
					if( isspace(*str) )
					{
						if( curpos>0 )
						{
							tempbuf[curpos] = 0;
							(*argc)++;
							curpos = 0;
							// save this argument to temp list
							data = strdup(tempbuf);
							if( argvlist )
							{
								slist_t* ptail = CoolSlistTail( argvlist);
								slist_t* pitem = CoolSlistNew( data );
								ptail->next = pitem;
							}
							else
							{
								argvlist = CoolSlistNew( data );
							}
						}
					}
					else
					{
						if( curpos+1<PATH_MAX )
							tempbuf[curpos++] = *str;
					}
				}
			}
			else
			{
				if( *str == '\'' )
				{
					tempbuf[curpos] = 0;
					(*argc)++;
					curpos = 0;
					flag = FALSE;
					// save this argument to temp list
					data = strdup(tempbuf);
					if( argvlist )
					{
						slist_t* ptail = CoolSlistTail( argvlist);
						slist_t* pitem = CoolSlistNew( data );
						ptail->next = pitem;
					}
					else
					{
						argvlist = CoolSlistNew( data );
					}
				}
				else
				{
					if( curpos+1<PATH_MAX )
						tempbuf[curpos++] = *str;
				}
			}
			str++;
		}
	}
	if( curpos>0 )
	{
		tempbuf[curpos] = 0;
		(*argc)++;
		// save this argument to temp list
		data = strdup(tempbuf);
		if( argvlist )
		{
			slist_t* ptail = CoolSlistTail( argvlist);
			slist_t* pitem = CoolSlistNew( data );
			ptail->next = pitem;
		}
		else
		{
			argvlist = CoolSlistNew( data );
		}
	}
	if( *argc )
	{
		*argv = (char**)malloc(sizeof(char*)*(*argc));
		if( *argv )
		{
			int i;
			for( i=0; i < *argc && argvlist; i++ )
			{
				(*argv)[i] = argvlist->data;
				argvlist = CoolSlistRemove(argvlist,argvlist);
			}
		}
	}
}

void CoolFreeParsedArgs(int argc, char ***argv)
{
	if( *argv )
	{
		int i;
		for( i=0; i<argc; i++ )
			free((*argv)[i]);
		free(*argv);
		*argv = NULL;
	}
}
