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
 * Download file from web server routines.
 *
 * REVISION:
 * 
 * 4) Modify function GetFileNameFromPath ----------------2006-11-20 JC
 *			fixed memory leak if function caller does not free the assigned memory
 * 3) Modify  --------------------------------------------2006-09-01 JC
 * 2) Modify interface. --------------------------------- 2006-08-28 EY
 * 1) Initial creation. ----------------------------------- 2006-08-24 JC
 *
 */

#include <fcntl.h>
#include <sys/mman.h>
#include <asm/sizes.h>
#include <linux/bsp_config.h>
#include <ctype.h>

//#define OSD_DBG_MSG
#include "nc-err.h"
#include "fopen.h"
#include "upgrade.h"
#include "webclient.h"

static int GetCharValueFromFile(const char* file, const char* field, char* value);

static int mStop = 1;         //the flag of stopping downloading file
static int mtotalSize = 0;    //total file size
static int mcurrentSize = 0;  //current size which has been downloaded
static char mhost[50];        //web server IP address
static char mport[10];        //web server http port
static char mosdVerUrl[50];   //OSD version file location in the web server
static char msavePath[100];   //local OSD firmware saving path
static char mcurVersion[20];  //local OSD current firmware version
static char mremoteVersion[20];    //remote OSD firmware version
static char mLocalOSDVerFile[100]; //location of local OSD version file 
static char mDowUrlInFile[100];    //Url from the file download from server
static char mUpkUrl[200];     //Url of the upk in the webserver
static int fd_mem;

#define OSD_FIRM_VERSION 		"OSDFirmwareVersion:"
#define OSD_FIRM_LOCATION 	"OSDFirmwareLocation:"
#define OSDVERSION_SAVEPATH		"/mnt/OSD/"

/**
*Function: CheckNewVersion
*Description: check whether there is newer version in the web server
*Parameters:
*	remoteVerFile: the osd version file in the web server
*	field: the osd version file in the local
*	localVer: 	local OSD firmware version
*Return:
*	0: there is newer version
*	1: there is no newer version
*	-1: error happened, probably the osd version file's content is wrong
*/
static int CheckNewVersion(const char* remoteVerFile, const char* field, const char* localVer)
{

     int rt;
     
     rt = GetCharValueFromFile(remoteVerFile, field, mremoteVersion);
     if(rt)
     {
	  DBGMSG(" CheckNewVersion get remote version failed\n");
	  return -1;
     }
     DBGMSG("!!local verion=%s,\n", localVer);
     DBGMSG("!!remote version=%s,\n", mremoteVersion);
     if(strncmp(mremoteVersion, localVer,sizeof(mremoteVersion))!=0)
	  return 0; //there is newer version
     else
	  return 1; // no newer version(version is identical)
}

/**
*Function: DownloadFile
*Description: download file from web server by http mode
*Parameters:
*	host: the web server's address
*	port: the port to be connected
*	url: 	the file in the web server to be downloaded
*	savePath: the local file name to save the downloaded file
*Return:
*	0: if download successfully
* 1: if download failed because of net connection
* -1 : download failed because of stop downloading by user
*/
static int DownloadFile(const char* host, const char* port, const char* reURL, const char* savePath)
{ 
     URL_FILE *handle;
     FILE *outf;
     
     int nread;
     char buffer[256];
     char url[PATH_MAX];
     mtotalSize = 0;  
     mcurrentSize = 0;  
     sprintf(url,"http://%s/%s",host,reURL);
     
     outf=fopen(savePath,"w+");
     if(!outf)
     {  
	  WARNLOG("couldnt open fread output file %s\n",savePath);
	  return 1; 
     }  
     
     handle = CoolUrlFopen(url, "r");
     if(!handle) {
	  WARNLOG("couldn't CoolUrlFopen()\n");
	  fclose(outf);
	  return 1;  
     }
     
     do {
	  if(mStop==1)
	  {
	       CoolUrlFclose(handle);
	       fclose(outf);
	       return -1;
	  }
	  nread = CoolUrlFread(buffer, 1,sizeof(buffer), handle);
	  fwrite(buffer,1,nread,outf);
     } while(nread);
     
     CoolUrlFclose(handle);
     
     fclose(outf); 
     return 0;
}  

/**
*Function: GetFileNameFromPath
*Description: get the file name from given file path
*Parameters:
*	path: the file path probably including '/'
*Return:
*	return the file name excluding '/' if successful, otherwise NULL is returned
*/
static char* GetFileNameFromPath(const char* path)
{
     char * name;

     name = strrchr(path, '/');
     if(NULL == name)
	  name = (char*)path-1;
     
     return name+1;
}

/*
*Function: GetCharValueFromFile
*Description: get value from the file
*Parameter:
*	file:
* field:
* value:
*Return:
*	0: success
* -1: failed
*/
static int GetCharValueFromFile(const char* file, const char* field, char* value)
{
     FILE* f = NULL;
     char line[256];
     char* tok;
     char* v1;
     int found = 0;
     int i;

     f = fopen(file, "r");
     if(NULL == f)
     {
	  DBGMSG("GetCharValueFromFile fopen error\n");
	  return -1;
     }
     memset(line, 0, sizeof(line));
     while(fgets(line, 256, f))
     {
	  tok = strtok(line, " ");
	  
	  if(!strcmp(tok, field))
	  {//find the field, then get its value
	       v1 = strtok(NULL, " ");
	       strcpy(value, v1);
	       for(i = 0; i < strlen(value); i++)
	       {
		    if(value[i] == '\r' || value[i] == '\n' || value[i] == '\t')
		    {
			 value[i] = '\0';
			 break;
		    }
	       }
	       found = 1;
	       break;
	  }
     }
     
     fclose(f);
     if(1 == found)
	  return 0;
     else
	  return -1;
}

/**
*Function: CoolIsNewSwAvailable
*Description: check whether there is newer version in the web server
*Parameters:
*	None
*Return:
*	1: there is newer version
*	0: there is no newer version
*      -1: there is error happened in net connection
*/
int CoolNewSwDetect(void)
{
     mStop = 0;

     int tmp;
     char* osdVerFile = NULL;
     char target[100];

     osdVerFile = GetFileNameFromPath(mosdVerUrl);
     memset(target, 0, sizeof(target));

     strcpy(target,OSDVERSION_SAVEPATH);
     strcat(target, osdVerFile);
     DBGMSG("osd version file is saved at:%s;\n", target);
     tmp = DownloadFile(mhost,mport, mosdVerUrl, target);
     strcpy(mLocalOSDVerFile, target); //save the location of downloaded OSD version file
     if(0 == tmp)// download success
     {
	  tmp = CheckNewVersion(target, OSD_FIRM_VERSION, mcurVersion);
	  switch(tmp)
	  {
	  case 0://find newer version
	       return 1;
	  case 1://no newer version
	       if(0 != remove(mLocalOSDVerFile))
		    perror("Unable to delete local OSD version file!");
	       return 0;
	  case -1://error happened
	       if(0 != remove(mLocalOSDVerFile))
		    perror("Unable to delete local OSD version file!");
	       DBGMSG("error in CheckNewVersion\n");
	       return 0;
	  }
     }
     else if(1 == tmp)
     {
          DBGMSG("can't download osd version file!\n");
          return -1;
     }

     return 0;  
}

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
int CoolUrlDownloadUpk(const char * url)
{ 
     URL_FILE *handle;
     int nread;
     char buffer[256];
     char *mem;  
     char *pmem; 
 
     mStop = 0;
     mtotalSize = 0;  
     mcurrentSize = 0;  
  
     handle = CoolUrlFopen(url, "r");
     if(!handle) {
	  WARNLOG("couldn't CoolUrlFopen()\n");
	  return 1; 
     }
     mtotalSize = CoolUrlSize(handle);
     if(mtotalSize > UPTOMEM_MAX_SIZE)
	  mtotalSize = UPTOMEM_MAX_SIZE;
     mem = CoolMemupkOpen();
     if( mem==NULL )
     {
	  CoolUrlFclose(handle);
	  return 1; 
     }
     if( CoolMemupkSetSize(mem,mtotalSize)==-1 )
     {
	  CoolMemupkClose(mem);
	  CoolUrlFclose(handle);
	  return 1; 
     }
     pmem = mem+PACK_OFFSET;
     do {
	  if(mStop==1)
	  {
	       CoolUrlFclose(handle);
	       CoolMemupkClose(mem);
	       return -1; 
	  }
	  nread = CoolUrlFread(buffer, 1,sizeof(buffer), handle);
	  if( (mcurrentSize + nread) >= mtotalSize )
	  {
	       memcpy(pmem,buffer,(mtotalSize - mcurrentSize));
	       pmem += (mtotalSize - mcurrentSize);
	       mcurrentSize = mtotalSize;
	       break;
	  }
	  else
	  {
	       memcpy(pmem,buffer,nread);
	       pmem += nread;
	       mcurrentSize += nread;
	  }
     } while(nread);
     strncpy(mUpkUrl, url, sizeof(mUpkUrl)-1);
     CoolUrlFclose(handle);
     CoolMemupkClose(mem);
     return 0; 
}  

/**
 * Function:CoolGetDownUrl
 * Desctription: Get the Url from the file download from server
 * Parameters : None
 * Return:
 *     the upk url
 */
char * CoolGetDownUrl(void)
{
     if(0 != GetCharValueFromFile(mLocalOSDVerFile, OSD_FIRM_LOCATION, mDowUrlInFile))
     {
	  DBGMSG("can't get firmware download uri!\n");
	  return NULL;
     }

     return mDowUrlInFile;
}

/**
*Function: CoolStopDownloadUPK
*Description: stop downloading OSD firmware file
*Parameters:
*	None
*Return:
*	None
*/
void CoolStopDownloadUPK(void)
{
     mStop = 1;
}

/**
*Description: get downloaded file's total size by byte
*Parameter:
*	None
*Return:
*	file's total size 
*/
int CoolGetDownFileSize(void)
{
     return mtotalSize;
}

/**
*Function: CoolGetCurSize
*Description: get current downloaded size by byte
*Parameter:
*	None
*Return:
*	current downloaded size 
*/
int CoolGetCurSize(void)
{
     return mcurrentSize;
}

/**
*Function: CoolGetRemoteVersion
*Description: get remote OSD firmware version
*Parameter:
*	None
*Return:
*	remote OSD firmware version 
*/
char* CoolGetRemoteVersion(void)
{
     return mremoteVersion;
}

/**
*Function: CoolSetDownloadUPK
*Description: setup parameters for downloading OSD firmware, such as web server IP, port etc..
*Parameters:
*	host: the web server URL address
*	port: the web server http port
*	osdVerUrl: the OSD firmware version file location in the web server
*	savePath: the firmware's saving path in the OSD
* curVersion: current OSD firmware version
*Return:
*	None
*/
void CoolSetDownloadUPK(const char* host, const char* port, const char* osdVerUrl, const char* savePath, const char* curVersion)
{
    const char* pTemp = NULL;
    char temp[100];
    memset(mhost, 0, sizeof(mhost));
    memset(mport, 0, sizeof(mport));
    memset(mosdVerUrl, 0, sizeof(mosdVerUrl));
    memset(msavePath, 0, sizeof(msavePath));
    memset(mcurVersion, 0, sizeof(mcurVersion));
    memset(mLocalOSDVerFile, 0, sizeof(mLocalOSDVerFile));
    memset(temp, 0, sizeof(temp));

    if(host)
        strcpy(mhost, host);
    if(port)
        strcpy(mport, port);
    //strcpy(mhost, "192.168.1.31");
    if(osdVerUrl && strlen(osdVerUrl) > 0)
    {
        pTemp = osdVerUrl;
        if(pTemp[0] == '/')
            pTemp++;
        strcpy(mosdVerUrl, pTemp);
    }
    //strcpy(mosdVerUrl, "osdversion.ver");

    if(savePath)
    {
        strcpy(temp, savePath);
        switch(strlen(temp))
        {
        case 0:
            strcpy(temp, "/");
            break;
        case 1:
            if(temp[0] != '/')
                strcat(temp, "/");
            break;
        default:
            if(temp[strlen(temp) -1] != '/')
                strcat(temp, "/");
        }

        strcpy(msavePath, temp);
    }
    else
        strcpy(msavePath, "/");

    if(curVersion)
        strcpy(mcurVersion, curVersion);
    else
        strcpy(mcurVersion, "");
}

char * CoolGetUpkWholeUrl(void)
{
     return mUpkUrl;
}

/**
 * Function: CoolMemupkOpen
 * Description: open /dev/mem and mamp
 * Parameter: None
 * Return: the point to the mamp area
 */
char* CoolMemupkOpen(void)
{
     char *mem;
     size_t memlen = UPTOMEM_MAX_SIZE+PACK_OFFSET;
     off_t mem_start=UPTOMEM_START_ADDR;

     if((fd_mem = open ("/dev/mem", O_RDWR)) < 0)
	  return NULL;

     mem = mmap (0, memlen, PROT_READ|PROT_WRITE, MAP_SHARED, fd_mem, mem_start);
     if(mem == MAP_FAILED)
     {
	  close (fd_mem); 
	  return NULL;
     }

     return mem;
}

void CoolMemupkClose(char *pMemupk)
{
     char* mem = pMemupk;
     size_t memlen = UPTOMEM_MAX_SIZE + PACK_OFFSET;  //16MB+4KB

     munmap (mem, memlen);
     close(fd_mem);
}

int CoolMemupkGetSize(char *pMemupk)
{
     char *mem;
     int  pk_size;

     if( pMemupk == NULL )  return 0;
     mem = pMemupk;
     pk_size = *((int *)(mem+4));

     return pk_size;
}

int CoolMemupkSetSize(char *pMemupk,int upksize)
{
     char upflag[]={0xa5,0xa5,1,2,0,0,0,0,3,4};
     char *mem;

     if( pMemupk == NULL )  return -1;
     mem = pMemupk;
     *((int *)(upflag+4))=upksize;
     memcpy(mem, upflag, sizeof(upflag));

     return 0;
}

int CoolMemupkClearSize(char *pMemupk)
{
     char *mem;
     char upflag[]={0x5a,0x5a,1,2,0,0,0,0,3,4};

     if( pMemupk == NULL ) return -1;
     mem = pMemupk;
     memcpy(mem, upflag, sizeof(upflag));

     return 0;
}
