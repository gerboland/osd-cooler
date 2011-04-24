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
 * System upgrade management routines.
 *
 * REVISION:
 * 
 *
 * 1) Initial creation. ----------------------------------- 2006-07-30 MG
 *
 */
#include <sys/mman.h>
#include <linux/bsp_config.h>
#include <pthread.h>
#include <sys/statfs.h>

#define OSD_DBG_MSG
#include "cooler-core.h"
#include "upgrade.h"
#include "webclient.h"
#include "parted/parted.h"

static int        upStatus;
static char *     pkg = NULL;
static pthread_t  upThread = (pthread_t)NULL;
static pack_info  upkInfo;
static hw_info    upkHw;

static package_header_t upkHeader;
static image_info_t  upkImageInfo;
static unsigned long loader_addr;

#define BUFFER_LEN   4096 /* contain data download from net or read from storage */
static int extTotalSize = 0;
static int extCurSize   = 0;

#define NODE_LENGTH  9 /* "/dev/hda" */
#define RETRYTIMES   15

#define IH_TYPE_COMPRESS        10

#define EXE_FILE_SIZE		0x20000
#define EXEFILENAME			"/mnt/OSD/upktemp"

#define EXT_GZ_FILE1    EXTEND_LINK"/ext1.gz"
#define EXT_GZ_FILE2    EXTEND_LINK"/ext2.gz"
#define EXT_GZ_ALLFILES EXTEND_LINK"/ext*.gz"

static void print_package_header(package_header_t phdr)
{
     DBGMSG("phdr->p_headsize: %x\n", (unsigned int)phdr.p_headsize);
     DBGMSG("phdr->p_reserve: %x\n",  (unsigned int)phdr.p_reserve);
     DBGMSG("phdr->p_headcrc: %x\n",  (unsigned int)phdr.p_headcrc);
     DBGMSG("phdr->p_datasize: %x\n", (unsigned int)phdr.p_datasize);
     DBGMSG("phdr->p_datacrc: %x\n",  (unsigned int)phdr.p_datacrc);
     DBGMSG("phdr->p_name: %s\n",     phdr.p_name);
     DBGMSG("phdr->p_vuboot: %s\n",   phdr.p_vuboot);
     DBGMSG("phdr->p_vkernel: %s\n",  phdr.p_vkernel);
     DBGMSG("phdr->p_vrootfs: %s\n",  phdr.p_vrootfs);
     DBGMSG("phdr->p_imagenum: %x\n", (unsigned int)phdr.p_imagenum);
}

static void print_image_info(image_info_t iif)
{  
     DBGMSG("iif->i_type: %x\n",        (unsigned int)iif.i_type);
     DBGMSG("iif->i_imagesize: %x\n",   (unsigned int)iif.i_imagesize);
     DBGMSG("iif->i_startaddr_p: %x\n", (unsigned int)iif.i_startaddr_p);
     DBGMSG("iif->i_startaddr_f: %x\n", (unsigned int)iif.i_startaddr_f);
     DBGMSG("iif->i_endaddr_f: %x\n",   (unsigned int)iif.i_endaddr_f);
     DBGMSG("iif->i_name: %s\n",        iif.i_name);
     DBGMSG("iif->i_version: %s\n",     iif.i_version);
}

static void print_pack_info(pack_info pack_i)
{
     DBGMSG("pack_i->upk_desc: %s\n", pack_i.upk_desc);
     DBGMSG("pack_i->pack_id: %s\n",  pack_i.pack_id);
     DBGMSG("pack_i->hw1_ver: %s\n",  pack_i.hw1_ver);
     DBGMSG("pack_i->hw2_ver: %s\n",  pack_i.hw2_ver);
     DBGMSG("pack_i->os_ver : %s\n",  pack_i.os_ver);
     DBGMSG("pack_i->app_ver: %s\n",  pack_i.app_ver);
}

/*************************************************************
 prepare update, check the /media/ext

 ************************************************************/
static int  extFolderStatus;
static int  extDownloadStatus;
static int  exttotalsize = 0, extdownloadedsize = 0;
static int  extStop = 0;
static pthread_t  downloadExtThread = (pthread_t)NULL;
static pthread_t  getPkgHeaderThread = (pthread_t)NULL;

static ulong getFreeSpaceSize(void);

static void * urlGetPkgHeaderLoop(void *pkgname)
{
     URL_FILE *fp_pk;
     int size, upk_size;
     off_t pos;

     if( !(fp_pk = CoolUrlFopen ((char *)pkgname, "r")) )
     {
	  DBGMSG("couldn't url_open the %s\n", (char *)pkgname);
          extFolderStatus = EXT_PART_NET_WRONG;
	  goto bail;
     }    
     upk_size = CoolUrlSize(fp_pk);
     pos=upk_size-sizeof(hw_info) ;
     CoolUrlSeek(fp_pk, pos);
     size=CoolUrlFread(&upkHw, 1, sizeof(hw_info), fp_pk);
     //DBGMSG("upkHw.hw_flag = %lx\n", upkHw.hw_flag);
     if(size != sizeof(hw_info))
     {
	  CoolUrlFclose(fp_pk);
          extFolderStatus = EXT_PART_NET_WRONG;
	  goto bail;
     }
     if(upkHw.hw_flag == HW_FLAG)
     {
	  pos = upkHw.hw_len;
	  CoolUrlSeek(fp_pk, pos);
	  size = CoolUrlFread((char *)&upkHeader, 1, sizeof(package_header_t), fp_pk);
     }
     else
     {
	  pos = 0;
	  upkHw.hw_len = 0;
	  CoolUrlSeek(fp_pk, pos);
	  size = CoolUrlFread((char *)&upkHeader, 1, sizeof(package_header_t), fp_pk);
     }
     CoolUrlFclose(fp_pk);
     print_package_header(upkHeader);
     if(size != sizeof(upkHeader))
     {
          extFolderStatus = EXT_PART_NET_WRONG;
	  goto bail;
     }
     extFolderStatus = EXT_DOWNLOAD_HEADER_SUCCESS;

bail:
     return NULL;
}

int CoolNetUrlGetPkgHeader(const char *pkgname)
{
     extFolderStatus = EXT_GET_PKG_HEADER_IN_PROGRESS;
     if(pthread_create(&getPkgHeaderThread, NULL, urlGetPkgHeaderLoop, (void *)pkgname))
     {
	  extFolderStatus = EXT_PART_SYSTEM_ERROR;
	  return -1;
     }
     return 0;
}

int CoolNetUrlGetPkgHeaderStatus(void)
{
     if( (extFolderStatus != EXT_GET_PKG_HEADER_IN_PROGRESS) && (!getPkgHeaderThread) )
     {
	  pthread_join(getPkgHeaderThread, NULL);
	  getPkgHeaderThread = (pthread_t)NULL;
     }

    return extFolderStatus;
}

static ulong getNetGzSize(const char *pkgname)
{
     URL_FILE *fp_pk;
     off_t pos;
     ulong gzsize = 0, tempsize;
     int i;

     if( !(fp_pk = CoolUrlFopen (pkgname, "r")) )
     {
	  extDownloadStatus = EXT_PART_NET_WRONG;
	  goto bail;
     }
     exttotalsize = 0;
     for(i=0; i<upkHeader.p_imagenum; i++)
     {
          pos = upkHw.hw_len + sizeof(package_header_t) + i*sizeof(image_info_t);
	  CoolUrlSeek(fp_pk, pos);
	  if(CoolUrlFread((char *)&upkImageInfo, 1, sizeof(image_info_t), fp_pk) != sizeof(image_info_t))
	  {
	       extDownloadStatus = EXT_PART_NET_WRONG;
               goto bail1;
	  }
	  if(upkImageInfo.i_type != IH_TYPE_COMPRESS) continue;
	  pos = upkHw.hw_len + upkImageInfo.i_startaddr_p + upkImageInfo.i_imagesize - 9;
	  CoolUrlSeek(fp_pk, pos);
          if(CoolUrlFread(&tempsize, 1, sizeof(ulong), fp_pk) != sizeof(ulong))
          {
	       extDownloadStatus = EXT_PART_READ_WRONG;
               goto bail1;
          }
          gzsize += tempsize;
          exttotalsize += upkImageInfo.i_imagesize;
     }
     CoolUrlFclose(fp_pk);

     return gzsize;

bail1:
     CoolUrlFclose(fp_pk);
bail:
     return -1;
}

int CoolCheckExtFreeSize(const char *pkgname)
{
     ulong size;

     size = getNetGzSize(pkgname);
     if(size == -1) 
          return EXT_PART_NET_WRONG;
     else 
          size += SZ_32M; /* /scratch directory at least 32M */

     CoolRunCommand("rm", "-rf", EXTEND_SCRATCH_AREA, NULL);
     if( getFreeSpaceSize() < size )
     {
          CoolRunCommand("rm", "-rf", EXTEND_USERS_AREA, NULL);
          if( getFreeSpaceSize() < size )
               return EXT_NEED_BE_CLEAN;
     }

     return EXT_FREE_SIZE_ENOUGH;
}

void  CoolCleanExtStorage(void)
{
     system("rm -rf /media/ext/*");
}

static void * extDownloadLoop(void *pkgname)
{
     URL_FILE *fp_upk;
     int nread, currentsize, extnum = 0;
     char buffer[256];
     FILE *fp_w;
     off_t pos;
     int i;

     if( !(fp_upk = CoolUrlFopen (pkgname, "r")) )
     {
	  extDownloadStatus = NP_CONNECTION_ERROR;
	  goto bail;
     }
     
     for(i=0; i<upkHeader.p_imagenum; i++)
     {
          if(extnum > 2 ) 
          {
               extDownloadStatus = EXT_UPK_ERROR;
               goto bail1;
          }
          pos = upkHw.hw_len + sizeof(package_header_t) + i*sizeof(image_info_t);
	  CoolUrlSeek(fp_upk, pos);
	  if(CoolUrlFread((char *)&upkImageInfo, 1, sizeof(image_info_t), fp_upk) != sizeof(image_info_t))
	  {
	       extDownloadStatus = NP_CONNECTION_ERROR;
               goto bail1;
	  }
	  if(upkImageInfo.i_type != IH_TYPE_COMPRESS) continue;

          extnum ++;
          if(extnum == 1)
          {
               if( (fp_w = fopen(EXT_GZ_FILE1, "wb+")) == NULL)
               {
                    DBGMSG("can't open" EXT_GZ_FILE1 "\n");
                    extDownloadStatus = NP_SYSTEM_ERROR;
                    goto bail1;
               }
          }
          else
          {
               if( (fp_w = fopen(EXT_GZ_FILE2, "wb+")) == NULL)
               {
                    DBGMSG("can't open" EXT_GZ_FILE2 "\n");
                    extDownloadStatus = NP_SYSTEM_ERROR;
                    goto bail1;
               }
          }
          pos = upkHw.hw_len + upkImageInfo.i_startaddr_p;
          CoolUrlSeek(fp_upk, pos);
          currentsize = 0;
          do {
               /* this detect will only stop this donwload thread,      *
                * so needn't check this again before function return */
               if(extStop == 1)
               {
                    goto bail2;
               }
               nread = CoolUrlFread(buffer, 1, sizeof(buffer), fp_upk);
               if( (currentsize + nread) >= upkImageInfo.i_imagesize )
               {
                    fwrite(buffer, 1, (upkImageInfo.i_imagesize - currentsize), fp_w);
                    currentsize = upkImageInfo.i_imagesize;
                    extdownloadedsize += (upkImageInfo.i_imagesize - currentsize);
                    break;
               }
               else
               {
                    fwrite(buffer, 1, nread, fp_w);
                    currentsize += nread;
                    extdownloadedsize += nread;
               }
          }while(nread);
          fclose(fp_w);
     }
     CoolUrlFclose(fp_upk);

     extDownloadStatus = NP_PKG_DOWNLOADED;
     return NULL;

bail2:
     fclose(fp_w);
     system("rm -rf " EXT_GZ_ALLFILES);

bail1:
     CoolUrlFclose(fp_upk);
bail:
     return NULL;
}

int CoolNetUrlGetPkgExt(const char *pkgname)
{
     extDownloadStatus = NP_GET_PKG_IN_PROGRESS;
     extStop = 0;
     if(pthread_create(&downloadExtThread, NULL, extDownloadLoop, (void *)pkgname))
     {
	  extDownloadStatus = NP_SYSTEM_ERROR;
	  return -1;
     }

     return 0;
}

int CoolNetUrlGetPkgExtStatus(void)
{
     if( extDownloadStatus != NP_GET_PKG_IN_PROGRESS )
     {
	  if(downloadExtThread)
	  {
	       pthread_join(downloadExtThread, NULL);
	       downloadExtThread = (pthread_t)NULL;
	  }
     }
     
    return extDownloadStatus;
}

int CoolNetUrlGetPkgExtDownloaded(void)
{
     return extdownloadedsize;
}

int CoolNetUrlGetPkgExtSize(void)
{
     return exttotalsize;
}

/* we don't have the thread sync here for detect the extStop */
void CoolStopDownloadExt(void)
{
     extStop = 1;
     if(downloadExtThread)
     {
          pthread_join(downloadExtThread, NULL);
          downloadExtThread = (pthread_t)NULL;
     }
}

/*************************************************************/

static int 
GetUpkInfo(const char *pkname)
{
     struct stat statbuf;    
     int fd_pk, size;
     off_t pos;
     int i;

     if( pkname )
     {
	  if(stat(pkname, &statbuf)<0)
	       return 0;

	  if((fd_pk = open (pkname, O_RDONLY)) < 0)
	       return 0;

	  pos = sizeof(hw_info) ;
	  lseek(fd_pk, -pos,2);
	  if( (size = read(fd_pk, &upkHw, sizeof(hw_info))) != sizeof(hw_info) )
	  {
	       close(fd_pk);
	       return 0;
	  }
	  DBGMSG("upkHw.hw_flag = %lx\n", upkHw.hw_flag);
	  if(upkHw.hw_flag == HW_FLAG)
	  {
	       pos=(sizeof(pack_info)+sizeof(hw_info));
	       lseek(fd_pk, -pos, 2);
	       size=read(fd_pk, (char *)&upkInfo, sizeof(pack_info));
	  }
	  else
	  {
	       upkHw.hw_len = 0;
	       pos=sizeof(pack_info);
	       lseek(fd_pk, -pos, 2);
	       size=read(fd_pk,(char *)&upkInfo, sizeof(pack_info));
	  }

	  if(size != sizeof(upkInfo))  
	  {
	       close(fd_pk);
	       return 0;
	  }
	  pos = upkHw.hw_len;
	  lseek(fd_pk, pos, 0);
	  if( (size = read(fd_pk, &upkHeader, sizeof(package_header_t))) != sizeof(package_header_t) )
	  {
	       close(fd_pk);
	       return 0;
	  }
	  for(i=0; i<upkHeader.p_imagenum; i++)
	  {
	       pos = upkHw.hw_len + sizeof(package_header_t) + i*sizeof(image_info_t);
	       lseek(fd_pk, pos, 0);
	       if( (size = read(fd_pk, &upkImageInfo, sizeof(image_info_t)) ) != sizeof(image_info_t) )
	       {
		    close(fd_pk);
		    return 0;
	       }
	       print_image_info(upkImageInfo);
	       if(upkImageInfo.i_type == IH_TYPE_COMPRESS) break;
	  }
	  close(fd_pk);
     }
     else
     { 
	  int pk_size = 0;
	  char *mem = NULL;
	  
	  mem = CoolMemupkOpen();
	  if( mem==NULL )
	       return 0;
	  
	  pk_size = CoolMemupkGetSize(mem);
	  
	  if(pk_size < UPTOMEM_MAX_SIZE )
	  {
	       memcpy(&upkHw, mem+PACK_OFFSET+pk_size-sizeof(hw_info),sizeof(hw_info));
	       if(upkHw.hw_flag == HW_FLAG)
		    memcpy(&upkInfo, mem+PACK_OFFSET+pk_size-sizeof(hw_info)-sizeof(pack_info),sizeof(pack_info));
	       else
	       {
		    upkHw.hw_len = 0;
		    memcpy(&upkInfo, mem+PACK_OFFSET+pk_size-sizeof(pack_info),sizeof(pack_info));
	       }
	       memcpy(&upkHeader, mem+PACK_OFFSET+upkHw.hw_len, sizeof(package_header_t));
	       for(i=0; i<upkHeader.p_imagenum; i++)
	       {
		    memcpy( &upkImageInfo,
			    (unsigned char *)mem+PACK_OFFSET+upkHw.hw_len+sizeof(package_header_t)+i*sizeof(image_info_t),
			    sizeof(image_info_t) );
		    print_image_info(upkImageInfo);
		    if(upkImageInfo.i_type == IH_TYPE_COMPRESS) break;
	       }
	  }
	  else
	  {
	       signature_t  signature;
	       for(i=0; i<RETRYTIMES; i++)
	       {
		    memcpy((unsigned char *)&signature, 
			   (unsigned char *)(mem+PACK_OFFSET+SZ_8K*i),
			   sizeof(signature_t) );
		    if( !strncmp(signature.string, NEUROS_UPK_SIGNATURE, strlen(NEUROS_UPK_SIGNATURE)) 
			&& (CoolCrc32(0, signature.string, SIGNATURELEN) == signature.strcrc) )
		    {
			 loader_addr = (unsigned long)(mem+PACK_OFFSET+SZ_8K*i+sizeof(signature_t)); 
			 memcpy(&upkHeader, (unsigned char *)loader_addr, sizeof(package_header_t) );
			 break;
		    }
	       }
	       if(i == RETRYTIMES) 
	       {
		    WARNLOG("can't find the correct signature in the upk\n");
		    CoolMemupkClose(mem);
		    return 0;
	       }
	       memcpy( &upkInfo, 
		       (unsigned char *)(loader_addr+sizeof(package_header_t)+upkHeader.p_imagenum*sizeof(image_info_t)),
		       sizeof(pack_info) );
	       for(i=0; i<upkHeader.p_imagenum; i++)
	       {
		    memcpy( &upkImageInfo,
			    (unsigned char *)loader_addr+sizeof(package_header_t)+i*sizeof(image_info_t),
			    sizeof(image_info_t) );
		    print_image_info(upkImageInfo);
		    if(upkImageInfo.i_type == IH_TYPE_COMPRESS) break;
	       }
	       
	  }
	  CoolMemupkClose(mem);
     }
     print_package_header(upkHeader);
     print_pack_info(upkInfo);
     return 1;
}

/* check to see if upgrade package is valid. 
 * return 1 if valid, otherwise 0
 */
static int 
isPkgValid(const char *pkname)
{
     if(strcmp(upkInfo.pack_id, PACKAGE_ID)==0)
	  return 1;
     
     return 0;
}

static void * 
upLoop( void * arg )
{
     char upflag[]={0xa5,0xa5,1,2,0,0,0,0,3,4};
     int fd_pk,fd_mem,size;
     char *mem;  
     int pk_size;
     size_t mem_len=UPTOMEM_MAX_SIZE + PACK_OFFSET;  //16MB+4KB
     off_t mem_start=UPTOMEM_START_ADDR;
     struct stat     statbuf;    
     
     do
     {
	  if(stat(pkg, &statbuf)<0)
	  {
	       upStatus = UPK_DOES_NOT_EXIST;
	       break;
	  }
	  
	  if((fd_pk = open (pkg, O_RDONLY)) < 0)
	  {
	       upStatus = UPK_UNABLE_TO_OPEN;
	       break;
	  }

	  pk_size=statbuf.st_size;
	  if(pk_size > UPTOMEM_MAX_SIZE)
	       pk_size = UPTOMEM_MAX_SIZE;
	  if(upkHw.hw_flag == HW_FLAG)
	  {
	       chmod(pkg,0777);
	       CoolRunCommand(pkg,NULL);
	  }
	  if((fd_mem = open ("/dev/mem", O_RDWR)) < 0)
	  {
	       close (fd_pk); 
	       upStatus = UPK_UNABLE_TO_ACCESS_SDRAM;
	       break;
	  }
	  
	  mem = mmap (0,mem_len , PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, mem_start);
	  if(mem == MAP_FAILED)
	  {
	       close (fd_pk); 
	       close (fd_mem); 
	       upStatus = UPK_UNABLE_TO_MMAP_SDRAM;
	       break;
	  }
	  
	  *((int *)(upflag+4))=pk_size;
	  memcpy(mem, upflag, sizeof(upflag));
	  
	  size=read(fd_pk,mem+PACK_OFFSET,pk_size);
	  DBGMSG("mem addr %p  %p\n",mem,mem+PACK_OFFSET);
	  
	  munmap (mem, mem_len);

	  close(fd_pk);
	  close(fd_mem); 
	  if(size!=pk_size)
	  {
	       upStatus = UPK_READ_ERROR;    
	       break;
	  }
	  else
	  {
	       CoolRunCommand("/sbin/ubootcmd", "set", "package_dir", pkg, NULL);
	       sleep(3);
	       CoolRunCommand("reboot", NULL);
	  }
     } while(0);
     return NULL;
}

static void * 
upfrommem( void * arg )
{
     int pk_size;
     char *mem;
     FILE *file = NULL;

     if(upkHw.hw_flag == HW_FLAG)
     {
	  mem = CoolMemupkOpen();
	  if( mem==NULL )
	  {
	       upStatus = UPK_UNABLE_TO_MMAP_SDRAM;
	       return NULL;
	  }
	  pk_size = CoolMemupkGetSize(mem);
	  DBGMSG("Upk filesize is %d\n",pk_size);
	  
	  if( (file=fopen(EXEFILENAME,"w"))==NULL )
	  {
	       perror("upfrommem : Open file failed.");
	       upStatus = UPK_UNABLE_TO_OPEN;
	       CoolMemupkClose(mem);
	       return NULL;
	  }

	  if( fwrite(mem+PACK_OFFSET,1,EXE_FILE_SIZE,file)!=EXE_FILE_SIZE )
	  {
	       upStatus = UPK_READ_ERROR;
	       DBGMSG("Write executing file error.\n");
	       fclose(file);
	       CoolMemupkClose(mem);
	       return NULL;
	  }
	  fflush(file);
	  fclose(file);
	  CoolMemupkClose(mem);
	  
	  chmod(EXEFILENAME,0777);
	  CoolRunCommand(EXEFILENAME,NULL);
	  unlink(EXEFILENAME);
     }
     CoolRunCommand("/sbin/ubootcmd", "set", "package_dir", CoolGetUpkWholeUrl(), NULL);
     sleep(3);
     CoolRunCommand("reboot", NULL);
     
     return NULL;
}

/** get upgrade package desc */
char *
CoolGetUpkDesc(char * ver, const char * pkname)
{
     ver[0] = 0;

     if( !GetUpkInfo(pkname) )
	  return NULL;

     strcpy(ver, upkInfo.upk_desc);
     WARNLOG("desc = %s\n", ver);
     return ver;
}

/** get upgrade package version: hw-app. */
char * 
CoolGetUpkVersion(char * ver, const char * pkname)
{
     ver[0] = 0;

     if( !GetUpkInfo(pkname) )
	  return NULL;

     if( upkHw.hw_flag==HW_FLAG )
	  strcpy(ver, upkInfo.hw2_ver);
     else
	  strcpy(ver, CoolGetHWversion()+1);// skip first character, as it is always zero.
     *(ver+strlen(upkInfo.hw2_ver)) = '-'; 
     strcpy(ver+strlen(upkInfo.hw2_ver)+1, upkInfo.app_ver);
     WARNLOG("ver = %s\n", ver);
     
     return ver;
}

/* compare ext part version */
int CoolCompareExtVersion(const char *pkname)
{
     char temp_ver[VERLEN];
     FILE * fp_r;
     char * cptr, ch;
     
     if(upkImageInfo.i_type != IH_TYPE_COMPRESS) return EXT_VERSION_SAME;

     if( !system("/sbin/ubootcmd -s get ext_apps > /tmp/env.extver") )
     {
          if( (fp_r = fopen("/tmp/env.extver", "r")) == NULL )
               temp_ver[0] = '\0';
          else
          {
               cptr = temp_ver;
               while(!feof(fp_r))
               {
                    ch = fgetc(fp_r);
                    if( (ch == 0x0d) || (ch == 0x0a) ) ch = '\0';
                    *cptr++ = ch;
               }
               *(--cptr) = '\0';
               fclose(fp_r);
          }
     }
     else
          temp_ver[0] = '\0';
     CoolRunCommand("rm", "-r", "/tmp/env.extver",NULL);
     
     if(strncmp(upkImageInfo.i_version, temp_ver, VERLEN) == 0 )
	  return EXT_VERSION_SAME;

     CoolRunCommand("/sbin/ubootcmd", "set", "ext_apps", upkImageInfo.i_version, NULL);

     if(pkname)
	  CoolRunCommand("/sbin/ubootcmd", "set", "package_dir", pkname,  NULL);
     else
	  CoolRunCommand("/sbin/ubootcmd", "set", "package_dir", CoolGetUpkWholeUrl(), NULL);
     
     return EXT_VERSION_DIFF;
}

/** upgrade OSD from given package. */
void 
CoolUpgradeFromPackage(const char *pkname)
{
     if( pkname )
     {
	  if(!isPkgValid(pkname))
	  {
	       upStatus = UPK_INVALID;
	  }
	  else
	  {
	       pkg = (char *)malloc(strlen(pkname)+1);
	       strcpy(pkg, pkname);
	       upStatus = UPK_IN_PROGRESS;
	       if(pthread_create(&upThread, NULL, upLoop, NULL))
	       {
		    upStatus = UPK_SYSTEM_ERROR;
	       }
	  }
     }
     else
     {
	  if( !isPkgValid(NULL) )
	  {
	       upStatus = UPK_INVALID;
	  }
	  else
	  {
	       upStatus = UPK_IN_PROGRESS;
	       if(pthread_create(&upThread, NULL, upfrommem, NULL))
	       {
		    upStatus = UPK_SYSTEM_ERROR;
	       }
	  }
     }
}

/** return upgrade status. */
int 
CoolUpgradeStatus(void)
{
     if((upStatus != UPK_IN_PROGRESS) && (!upThread))
     {
	  pthread_join(upThread, NULL);
	  upThread = (pthread_t)NULL;
	  if(pkg) free(pkg);
     }
     
     return upStatus;
}

// -------------------------------------------------------------------
// starting of network package support.
// -------------------------------------------------------------------

static pthread_t  downloadThread = (pthread_t)NULL;
static int netStatus;
static int pkgSize;
static int downloaded;

static void * 
downloadLoop( void * arg )
{
     int rt = 0;

     rt = CoolUrlDownloadUpk((char *)arg);
     switch(rt)
     {
     case 0:
	  netStatus = NP_PKG_DOWNLOADED;
	  break;
     case 1:
	  netStatus = NP_CONNECTION_ERROR;
	  break;
     case -1:
	  netStatus = NP_PKG_DOWNLOADED_FAIL;
	  break;
     }
    return NULL;
}

/** get package through internet. */
int 
CoolNetUrlGetPkg(char *url)
{
     netStatus = NP_GET_PKG_IN_PROGRESS;
     if(pthread_create(&downloadThread, NULL, downloadLoop, (void *)url))
     {
	  netStatus = NP_SYSTEM_ERROR;
	  return -1;
     }
     
     return 0;
}

/** Returns various net get package status. */
int 
CoolNetGetPkgStatus(void)
{
     if( netStatus != NP_GET_PKG_IN_PROGRESS )
     {
	  if(downloadThread)
	  {
	       pthread_join(downloadThread, NULL);
	       downloadThread = (pthread_t)NULL;
	  }
     }
     
    return netStatus;
}

/** return total package size in bytes. */
int  
CoolNetGetPkgSize(void)
{
     pkgSize = CoolGetDownFileSize();
     return pkgSize;
}

/** return downloaded package size in bytes. */
int 
CoolNetGetPkgDownloaded(void)
{
     downloaded = CoolGetCurSize();
     return downloaded;
}

/**
*Function: CoolStopNetDownload
*Description: stop downloading package from net
*Parameters:
*	None
*Return:
*	None
*/
void CoolStopNetDownload(void)
{
     CoolStopDownloadUPK();
     if(downloadThread)
     {
	  pthread_join(downloadThread, NULL);
	  downloadThread = (pthread_t)NULL;
     }
}

/*****************************************************
 *  Update the extra app part support
 ******************************************************/

static pthread_t  extUpdateThread = (pthread_t)NULL;
static int extUpdateStatus;
static char upkLocation[PATH_MAX];


/* get the upk position from enviroment */
static int getPositionFromEnv(void)
{
     FILE * fp_r;
     char * cptr;

     if( system("/sbin/ubootcmd -s get package_dir > /tmp/env.upkdir") )
          return -1;
     if( (fp_r = fopen("/tmp/env.upkdir", "r")) == NULL ) return -1;
     cptr = upkLocation;
     while(!feof(fp_r))
	  *cptr++ = fgetc(fp_r);
     *(--cptr) = '\0';
     fclose(fp_r);
     CoolRunCommand("rm", "/tmp/env.upkdir", NULL);
     
     return 0;
}

static ulong getFreeSpaceSize(void)
{
     struct statfs statfsbuf;

     if(statfs(EXTEND_LINK, &statfsbuf) != 0)
          return 0;

     return (ulong)(statfsbuf.f_bsize * statfsbuf.f_bavail);
}

static int getLocalUpkHeader(void)
{
     struct stat statbuf;
     int fd_pk;
     int size;
     off_t pos;

     extUpdateStatus = EXT_PART_GET_UPK_INFO;
     if(stat(upkLocation, &statbuf)<0)
	  return -1;
    
     if((fd_pk = open (upkLocation, O_RDONLY)) < 0)
	  return -1;
    
     pos = sizeof(hw_info) ;
     lseek(fd_pk, -pos,2);
     size = read(fd_pk, &upkHw, sizeof(hw_info));
     //DBGMSG("upkHw.hw_flag = %lx\n", upkHw.hw_flag);
     if(size != sizeof(hw_info))
     {
	  close(fd_pk);
	  return -1;
     }
     if(upkHw.hw_flag == HW_FLAG)
     {
	  pos = upkHw.hw_len;
	  lseek(fd_pk, pos, 0);
	  size = read(fd_pk, (char *)&upkHeader, sizeof(package_header_t));
     }
     else
     {
	  pos = 0;
	  upkHw.hw_len = 0;
	  lseek(fd_pk, pos, 0);
	  size = read(fd_pk,(char *)&upkHeader, sizeof(package_header_t));
     }
     close(fd_pk);
     print_package_header(upkHeader);
     if(size != sizeof(upkHeader))
	  return -1;
     
     return 0;
}

static ulong getLocalGzSize(void)
{
     FILE *fp_pk;
     off_t pos;
     ulong gzsize = 0, tempsize;
     int i;

     if((fp_pk = fopen (upkLocation, "r")) == NULL)
     {
	  extUpdateStatus = EXT_PART_READ_WRONG;
	  goto bail;
     }

     for(i=0; i<upkHeader.p_imagenum; i++)
     {
          pos = upkHw.hw_len + sizeof(package_header_t) + i*sizeof(image_info_t);
	  fseek(fp_pk, pos, 0);
	  if(fread((char *)&upkImageInfo, sizeof(image_info_t), 1, fp_pk) != 1)
	  {
	       extUpdateStatus = EXT_PART_READ_WRONG;
               goto bail1;
	  }
	  if(upkImageInfo.i_type != IH_TYPE_COMPRESS) continue;
	  pos = upkHw.hw_len + upkImageInfo.i_startaddr_p + upkImageInfo.i_imagesize - 9;
	  fseek(fp_pk, pos, 0);
          if(fread(&tempsize, 1, sizeof(ulong), fp_pk) != sizeof(ulong))
          {
	       extUpdateStatus = EXT_PART_READ_WRONG;
               goto bail1;
          }
          gzsize += tempsize;
     }
     fclose(fp_pk);

     return gzsize;

bail1:
     fclose(fp_pk);
bail:
     return 0;
}

static int localExtUpdateProcess(void)
{
     FILE *fp_pk, *fp_w;
     off_t pos;
     int i, tempcrc, status;
     int fd_mem;
     char *mem;
     size_t mem_len=UPTOMEM_MAX_SIZE + PACK_OFFSET;  //16MB+4KB
     off_t mem_start=UPTOMEM_START_ADDR;
     ulong gzsize = 0;

     if((fp_pk = fopen (upkLocation, "r")) == NULL)
     {
	  extUpdateStatus = EXT_PART_READ_WRONG;
	  goto bail;
     }
     if((fd_mem = open ("/dev/mem", O_RDWR)) < 0)
     {
	  upStatus = EXT_PART_UNABLE_ACCESS_SDRAM;
	  goto bail1;
     }
     mem = mmap (0,mem_len , PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, mem_start);
     if(mem == MAP_FAILED)
     {
	  upStatus = EXT_PART_UNABLE_MMAP_SDRAM;
	  goto bail2;
     }

     if(access(EXTEND_LINK, W_OK) != 0)
     {
	  extUpdateStatus = EXT_PART_NO_EXTCARD;
	  goto bail2;
     }
     else 
     {    
	  CoolRunCommand("rm", "-rf", EXTEND_DIRECTORY, NULL);
          mkdir(EXTEND_DIRECTORY, 0777);
          mkdir(EXTEND_SCRATCH_AREA, 0777);
          gzsize = getLocalGzSize();
          if(gzsize == 0) goto bail2;
          if( getFreeSpaceSize() < (gzsize + SZ_16M))/* sz_16M is used to save the tmp.gz */
          {     
               WARNLOG("free space size 1 is %lu\n", getFreeSpaceSize());
               WARNLOG("need space size is %lu\n", (gzsize+SZ_16M) );
               CoolRunCommand("rm", "-rf", EXTEND_USERS_AREA, NULL);
               if( getFreeSpaceSize() < (gzsize + SZ_16M) )
               {
                    WARNLOG("free space size 2 is %lu\n", getFreeSpaceSize());
                    extUpdateStatus = EXT_PART_NO_SPACE;
                    goto bail2;
               }
          }
     }

     for(i=0; i<upkHeader.p_imagenum; i++)
     {
	  pos = upkHw.hw_len + sizeof(package_header_t) + i*sizeof(image_info_t);
	  fseek(fp_pk, pos, 0);
	  if(fread((char *)&upkImageInfo, sizeof(image_info_t), 1, fp_pk) != 1)
	  {
	       extUpdateStatus = EXT_PART_READ_WRONG;
	       goto bail2;
	  }
	  print_image_info(upkImageInfo);
	  if(upkImageInfo.i_type != IH_TYPE_COMPRESS) continue;
	  pos = upkHw.hw_len + upkImageInfo.i_startaddr_p;
	  fseek(fp_pk, pos, 0);
	  WARNLOG("read update image from storage\n");

	  {
	       char *pmem, buffer[BUFFER_LEN];
	       int nread; 

	       extCurSize = 0;
	       extTotalSize = upkImageInfo.i_imagesize;
	       extUpdateStatus = EXT_PART_GET_GZ;
	       pmem = mem+PACK_OFFSET;
	       do{
		    nread = fread(buffer, 1, sizeof(buffer), fp_pk);
                    if( (nread != sizeof(buffer)) && !feof(fp_pk) )
                    {
                         extUpdateStatus = EXT_PART_READ_WRONG;
                         goto bail2;
                    }
		    memcpy(pmem, buffer, nread);
		    pmem += nread;
		    extCurSize += nread;
	       }while(extCurSize < upkImageInfo.i_imagesize);
	  }
	  memcpy(&tempcrc, mem+PACK_OFFSET+upkImageInfo.i_imagesize-sizeof(int), sizeof(int));
	  if(CoolCrc32(0, mem+PACK_OFFSET, upkImageInfo.i_imagesize-sizeof(int)) != tempcrc)
	  {
	       int ttcrc;
	       ttcrc = CoolCrc32(0, mem+PACK_OFFSET, upkImageInfo.i_imagesize-sizeof(int));
	       DBGMSG("tempcrc = %x, ttcrc = %x\n", tempcrc, ttcrc);
	       extUpdateStatus = EXT_PART_READ_WRONG;
	       goto bail2;
	  }
	  if( (fp_w = fopen(EXTEND_SCRATCH_AREA"/tmp.gz", "wb+")) == NULL)
	  {
	       DBGMSG("can't open /.osd-extend/scratch/tmp.gz \n");
	       extUpdateStatus = EXT_PART_NO_EXTCARD;
	       goto bail2;
	  }
	  if(fwrite(mem+PACK_OFFSET, upkImageInfo.i_imagesize-sizeof(int), 1, fp_w) != 1)
	  {
	       extUpdateStatus = EXT_PART_NO_EXTCARD;
	       goto bail3;
	  }
	  fclose(fp_w);

          extCurSize = 0;
	  extUpdateStatus = EXT_PART_UNCOMPRESS;
	  WARNLOG("un-compress the image to %s\n", EXTEND_DIRECTORY);
	  status = CoolRunCommand("tar", "xzf", EXTEND_SCRATCH_AREA"/tmp.gz", 
                                  "-C", EXTEND_DIRECTORY, NULL);
          if(status != 0)
          {
               extUpdateStatus = EXT_PART_UNCOMPRESS_ERROR;
               CoolRunCommand("rm", "-rf", EXTEND_PROGRAMS_AREA, NULL);
               goto bail2;
          }
     }
     CoolRunCommand("rm", EXTEND_SCRATCH_AREA"/tmp.gz", NULL);
     CoolMemupkClearSize(mem);
     close(fd_mem);
     fclose (fp_pk); 

     if(access("/media/CF-card/OSD-update", F_OK) == 0)
     {
          CoolRunCommand("rm", "-rf", "/media/CF-card/OSD-update", NULL);
     }
     if(access("/mnt/tmpfs/mount_NAND/OSD-update", F_OK) == 0)
     {
          CoolRunCommand("rm", "-rf", "/mnt/tmpfs/mount_NAND/OSD-update", NULL);
     }

     CoolRunCommand("/sbin/ubootcmd", "set", "update_boot", NULL);
     extUpdateStatus = EXT_PART_UPDATE_COMPLETE;
     sleep(3);
     CoolRunCommand("reboot", NULL);
     
     return 0;

bail3:
     fclose(fp_w);
bail2:
     close(fd_mem);
bail1:
     fclose(fp_pk);
bail:
     return -1;
}

static int netExtUpdateProcess(void)
{
     int status;

     if( access(EXT_GZ_FILE1, F_OK) != 0 )
     {
          extUpdateStatus = EXT_PART_NO_EXIST;
          goto bail;
     }
     CoolRunCommand("rm", "-rf", EXTEND_DIRECTORY, NULL);
     mkdir(EXTEND_DIRECTORY, 0777);
     mkdir(EXTEND_SCRATCH_AREA, 0777);

     extUpdateStatus = EXT_PART_UNCOMPRESS;
     WARNLOG("un-compress the image to %s\n", EXTEND_DIRECTORY);

     status = CoolRunCommand("tar", "xzf", EXT_GZ_FILE1, 
                             "-C", EXTEND_DIRECTORY, NULL);
     if(status != 0)
     {
          extUpdateStatus = EXT_PART_UNCOMPRESS_ERROR;
          CoolRunCommand("rm", "-rf", EXTEND_PROGRAMS_AREA, NULL);
          goto bail;
     }
     CoolRunCommand("rm", EXT_GZ_FILE1, NULL);

     if(access(EXT_GZ_FILE2, F_OK) == 0)
     {
          status = CoolRunCommand("tar", "xzf", EXT_GZ_FILE2, 
                                  "-C", EXTEND_DIRECTORY, NULL);
          if(status != 0)
          {
               extUpdateStatus = EXT_PART_UNCOMPRESS_ERROR;
               CoolRunCommand("rm", "-rf", EXTEND_PROGRAMS_AREA, NULL);
               goto bail;
          }
          CoolRunCommand("rm", EXT_GZ_FILE2, NULL);
     }

     CoolRunCommand("/sbin/ubootcmd", "set", "update_boot", NULL);
     extUpdateStatus = EXT_PART_UPDATE_COMPLETE;
     sleep(2);
     CoolRunCommand("reboot", NULL);
     
     return 0;

bail:
     return -1;
}

/* read ext part and gunzip to specified directory */
static void * extUpdateLoop(void * arg)
{
     if(strncmp(upkLocation, "http://", strlen("http://")) == 0)
     {
	  if(netExtUpdateProcess()) 
	       goto bail;
     }
     else
     {
	  if(access(upkLocation, F_OK) == 0)
	  {
	       if(getLocalUpkHeader()) 
	       {
		    extUpdateStatus = EXT_PART_READ_WRONG;
		    goto bail;
	       }
	       if(localExtUpdateProcess()) 
	       {
		    rmdir(EXTEND_PROGRAMS_AREA); 
		    goto bail;
	       }
	  }
	  else
	  {
	       extUpdateStatus = EXT_PART_NO_PACKAGE;
	       goto bail;
	  }
     }
bail:
     return NULL;
}

int CoolUpgradeExtPart(void)
{
     if( getPositionFromEnv() )
     {
	  extUpdateStatus = EXT_PART_NO_PARAM;
	  return -1;
     }
     extUpdateStatus = EXT_PART_IN_PROGRESS;
     if(pthread_create(&extUpdateThread, NULL, extUpdateLoop, NULL))
     {
	  extUpdateStatus = EXT_PART_SYSTEM_ERROR;
	  return -1;
     }
     
     return 0;
}

/** Returns various ext part update status. */
int CoolUpgradeExtPartStatus(void)
{
     if( (extUpdateStatus != EXT_PART_IN_PROGRESS) && (!extUpdateThread) )
     {
	  pthread_join(extUpdateThread, NULL);
	  extUpdateThread = (pthread_t)NULL;
     }

    return extUpdateStatus;
}

/*****************************************************
 *  Check the CF card support
 ******************************************************/

#define FS_EXT2       0xEF51
#define FS_EXT2_EXT3  0xEF53
#define FS_NFS        0x6969
#define FS_YAFFS      0x5941ff53
#define CF_MOUNT_POINT   "/mnt/tmpfs/mount_CF-card"

static pthread_t  extFormatThread = (pthread_t)NULL;
static int extCheckCardStatus;

static int cfNeedBeClean(void)
{
     if(CoolRunCommand("/sbin/ubootcmd", "get", "clean_ext_area", NULL))
     {
          return 0;
     }
     else 
     {
          return 1;
     }
}

static int loadUpkToSram(void)
{
     int fd_pk, fd_mem, size;
     char *mem;
     size_t mem_len = MAX_UPK_SIZE_IN_CF;
     off_t mem_start = UPTOMEM_START_ADDR;
     struct stat statbuf;

     if(stat(upkLocation, &statbuf) < 0 )
     {
	  extCheckCardStatus = EXT_PART_NO_PACKAGE;
	  return -1;
     }
     if( (fd_pk = open(upkLocation, O_RDONLY)) < 0)
     {
	  extCheckCardStatus = EXT_PART_READ_WRONG;
	  return -1;
     }
     if( (fd_mem = open("/dev/mem", O_RDWR)) < 0 )
     {
	  close(fd_pk);
	  extCheckCardStatus = EXT_PART_UNABLE_ACCESS_SDRAM;
	  return -1;
     }
     mem = mmap (0,mem_len , PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, mem_start);
     if(mem == MAP_FAILED)
     {
	  close(fd_pk);
	  close(fd_mem); 
	  extCheckCardStatus = EXT_PART_UNABLE_MMAP_SDRAM;
	  return -1;
     }
     size = read(fd_pk, mem+PACK_OFFSET, statbuf.st_size);
     munmap(mem, mem_len);
     close(fd_pk);
     close(fd_mem);
     if(size != statbuf.st_size)
     {
	  extCheckCardStatus = EXT_PART_READ_WRONG;
	  return -1;
     }

     return 0;
}

static int copyUpkBackToCard(int size)
{
     FILE *fp_w; 
     int fd_mem;
     char *mem;
     size_t mem_len = MAX_UPK_SIZE_IN_CF;
     off_t mem_start = UPTOMEM_START_ADDR;

     CoolRunCommand("mkdir", "/media/CF-card/OSD-update", NULL);
     if( (fp_w = fopen("/media/CF-card/OSD-update/OSD.upk", "wb+")) == NULL)
     {
	  extCheckCardStatus = EXT_PART_SYSTEM_ERROR;
	  return -1;
     }

     if( (fd_mem = open("/dev/mem", O_RDWR)) < 0 )
     {
	  fclose(fp_w);
	  extCheckCardStatus = EXT_PART_UNABLE_ACCESS_SDRAM;
	  return -1;
     }

     mem = mmap (0,mem_len , PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, mem_start);
     if(mem == MAP_FAILED)
     {
	  fclose(fp_w);
	  close(fd_mem); 
	  extCheckCardStatus = EXT_PART_UNABLE_MMAP_SDRAM;
	  return -1;
     }

     if(fwrite(mem+PACK_OFFSET, size, 1, fp_w) != 1)
     {
	  fclose(fp_w);
	  munmap(mem, mem_len);
	  close(fd_mem); 
	  extCheckCardStatus = EXT_PART_SYSTEM_ERROR;
	  return -1;
     }
     munmap(mem, mem_len);
     fclose(fp_w);
     close(fd_mem);

     CoolRunCommand("/sbin/ubootcmd", "set", "package_dir", "/media/CF-card/OSD-update/OSD.upk", NULL);
     return 0;
}

static int getNodeFromProcMounts(char *devnode)
{
     FILE *fp_r;
     char linebuff[PATH_MAX];
     char *dir;
     
     if((fp_r =fopen("/proc/mounts", "r")) == NULL) 
	  return -1;

     while(1)
     {
	  if(feof(fp_r)) 
	  {
	       fclose(fp_r);
	       return -1;
	  }
	  if(!fgets(linebuff, PATH_MAX, fp_r))
	  {
	       fclose(fp_r);
	       return -1;
	  }
	  dir = strchr(linebuff, 0x20); /* find the first space */
	  if(dir == NULL) 
	  {
	       fclose(fp_r);
	       return -1;
	  }
	  if(strncmp(dir+1, CF_MOUNT_POINT, strlen(CF_MOUNT_POINT)) == 0)
	  {
	       strlcpy(devnode, linebuff, dir-linebuff+1);
	       WARNLOG("devnode = %s\n", devnode);
	       break;
	  }
     }

     fclose(fp_r);
     return 0;
}

static int partedCFCard(char *dev)
{
     char              devname[PATH_MAX];
     PedDevice         *peddev;
     PedDisk           *peddisk;
     PedPartition      *part;
     PedConstraint     *dev_constraint = NULL;
     int               i, flag = 0;

     strlcpy(devname, dev, NODE_LENGTH);
     WARNLOG("device name is = %s\n", devname);

     peddev = ped_device_get(devname);
     if(!peddev)
     {
          ped_exception_catch();
          WARNLOG("can't get device %s\n", devname);
          return -1;
     }
     if( !(ped_device_open(peddev)) )
     {
          WARNLOG("can't open device %s\n", devname);
          return -1;
     }
     /* retry 5 times, because sometimes some cards 
        can't get the disk information correctly at one time */
     for(i=0; i<5; i++) 
     {
          if( (peddisk = ped_disk_new(peddev)) )
          {
               flag = 1;
               break;
          }
          printf("ped_disk_new, retry times = %x\n", i);
     }
     if(flag == 0)
     {
          WARNLOG("no disk detect in device\n"); 
          goto error_device_close;
     }
     ped_disk_print(peddisk);

     if(ped_disk_get_last_partition_num(peddisk) > 1)
     {
          if( !(ped_disk_delete_all(peddisk)) )
          {
               WARNLOG("can't delete all the partions on disk\n");
               goto error_distroy_disk;
          }
          if( !(part = ped_partition_new(peddisk, PED_PARTITION_NORMAL, NULL, 0, peddev->length-1)) )
          {
               WARNLOG("can't create a new partition\n");
               goto error_distroy_disk;
          }
          dev_constraint  = ped_device_get_constraint(peddev);
          if( !(ped_disk_add_partition(peddisk, part, dev_constraint)) )
          {
               WARNLOG("can't add the partition into disk\n");
               goto error_remove_part;
          }
          if( !(ped_disk_commit(peddisk)) )
          {
               WARNLOG("can't commit to the disk\n");
               goto error_remove_part;
          }
          ped_constraint_destroy(dev_constraint);
     }

     ped_disk_destroy(peddisk);
     ped_device_close(peddev);

     return 0;

error_remove_part:
     ped_disk_remove_partition (peddisk, part);
     ped_partition_destroy(part);
     if(dev_constraint != NULL)
          ped_constraint_destroy(dev_constraint);

error_distroy_disk:

     ped_disk_destroy(peddisk);

error_device_close:
     ped_device_close(peddev);

     return -1;
}

static int performFormatAction(void)
{
     int status;
     char devnode[PATH_MAX];

     if(getNodeFromProcMounts(devnode)) 
     {
	  extCheckCardStatus = EXT_PART_SYSTEM_ERROR;
	  return -1;
     }
     extCheckCardStatus = EXT_FORMAT_IN_PROGRESS;
     CoolRunCommand("umount", EXTEND_LINK, NULL);
     system("umount -f /mnt/tmpfs/mount_CF-card*");

     if(cfNeedBeClean())
     {     /* delete all partitions and create a new one, using libparted */
          if( partedCFCard(devnode) )
          {
               extCheckCardStatus = EXT_PARTED_CARD_ERROR;
               return -1;
          }

          CoolRunCommand("/sbin/ubootcmd", "set", "clean_ext_area", NULL);
     }

     status = CoolRunCommand("mkfs.ext3", devnode, "-O", "dir_index,filetype,resize_inode,sparse_super", NULL);
     if(status != 0)
     {
	  extCheckCardStatus = EXT_FORMAT_FAILED;
	  return -1;
     }

     CoolRunCommand("mount", devnode, EXTEND_LINK, NULL);

     return 0;
}

static void * extFormatLoop(void * arg)
{
     int upkinmem = 0;
     struct stat statbuf;

     if( getPositionFromEnv() )
     {
	  extCheckCardStatus = EXT_PART_NO_PARAM;
	  return NULL;
     }
     if( strncmp(upkLocation, "/media/CF-card", strlen("/media/CF-card"))==0)
     {
	  if( (stat(upkLocation, &statbuf) < 0) || (statbuf.st_size > (MAX_UPK_SIZE_IN_CF)) )
	  {
	       extCheckCardStatus = EXT_UPK_TOO_LARGE;
	       return NULL;
	  }
	  /* copy the upk to memory */
	  extCheckCardStatus = EXT_PART_LOADING_UPK;
	  if(loadUpkToSram() != 0)
	       return NULL;
	  upkinmem = 1;
     }
 
     if(performFormatAction()) return NULL;

     if(upkinmem) 
	  copyUpkBackToCard(statbuf.st_size);

     extCheckCardStatus = EXT_FORMAT_SUCCESS;
     return NULL;
}

int CoolFormatExtStorage(void)
{
     if(pthread_create(&extFormatThread, NULL, extFormatLoop, NULL))
     {
	  extCheckCardStatus = EXT_PART_SYSTEM_ERROR;
	  return -1;
     }

     return 0;
}

int CoolGetFormatStatus(void)
{
     if( (extCheckCardStatus != EXT_FORMAT_IN_PROGRESS) && (!extFormatThread) )
     {
	  pthread_join(extFormatThread, NULL);
	  extFormatThread = (pthread_t)NULL;
     }
     
     return extCheckCardStatus;
}

int CoolExtStorageIsRdy(void)
{
     struct statfs statfsbuf;

     if(statfs(EXTEND_LINK, &statfsbuf) !=  0)
	  return EXT_STORAGE_NO_STORAGE;
     WARNLOG("statfsbuf.f_type = %x\n", statfsbuf.f_type);

     /* nfs or nand flash */
     if(statfsbuf.f_type == FS_NFS)     return EXT_STORAGE_IS_RDY;
     if(statfsbuf.f_type == FS_YAFFS)
     {
          if( cfNeedBeClean() && 
              (access("/mnt/tmpfs/mount_NAND/OSD-update/OSD.upk", F_OK) != 0) ) 
          {
               system("rm -rf /mnt/tmpfs/mount_NAND/* /mnt/tmpfs/mount_NAND/.*");
               CoolRunCommand("/sbin/ubootcmd", "set", "clean_ext_area", NULL);
          }
          return EXT_STORAGE_IS_RDY;
     }
     
     if( (!cfNeedBeClean()) && 
         (statfsbuf.f_type == FS_EXT2 || statfsbuf.f_type == FS_EXT2_EXT3) )
          return EXT_STORAGE_IS_RDY;
     else 
          return EXT_STORAGE_NEED_FORMAT;
}

/** read current version from enviroment in flash */
void CoolGetCurOsdVersion(char *version)
{
    const char * pc;
     FILE * fp_r;
     char * cptr, ch;

     if( !system("/sbin/ubootcmd -s get cur_rootfs > /tmp/env.curver") )
     {
          if( (fp_r = fopen("/tmp/env.curver", "r")) == NULL )
               version[5] = '\0';
          else
          {
               cptr = &version[5];
               while(!feof(fp_r))
               {
                    ch = fgetc(fp_r);
                    if( (ch == 0x0d) || (ch == 0x0a) ) ch = '\0';
                    *cptr++ = ch;
               }
               *(--cptr) = '\0';
               fclose(fp_r);
          }
     }
     else
          version[5] = '\0';
     CoolRunCommand("rm", "-r", "/tmp/env.curver", NULL);
    // update HW version.
    pc = CoolGetHWversion();
    if(pc != NULL)
    {
        pc++; // skip first character, as it is always zero.
        version[0] = *pc++;
        version[1] = *pc++;
        version[2] = *pc++;
        version[3] = *pc;
        version[4] = '-';
    }
}

int CoolExtGetTotalSize(void)
{
     return extTotalSize;
}

int CoolExtGetCurSize(void)
{
     return extCurSize;
}
