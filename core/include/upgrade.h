#ifndef UPGRADE__H
#define UPGRADE__H
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
 * OSD upgrade management header.
 *
 * REVISION:
 * 
 * 
 * 2) Add API . ------------------------------------------- 2006-09-11 JC
 * 1) Initial creation. ----------------------------------- 2006-07-30 MG 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <asm/sizes.h>

#define UPK_DOES_NOT_EXIST              1
#define UPK_CHECKSUM_ERROR              2
#define UPK_UNABLE_TO_OPEN              3
#define UPK_UNABLE_TO_MMAP_SDRAM        4
#define UPK_UNABLE_TO_ACCESS_SDRAM      5
#define UPK_READ_ERROR                  6
#define UPK_UPGRADE_READY               7
#define UPK_SYSTEM_ERROR                8
#define UPK_IN_PROGRESS                 9
#define UPK_INVALID                     10

// various net package status definitions.
#define NP_CONNECTION_ERROR             1
#define NP_SYSTEM_ERROR                 2
#define NP_GET_PKG_IN_PROGRESS          3
#define NP_PKG_DOWNLOADED               4
#define NP_PKG_DOWNLOADED_FAIL          5

/* update extra part status definitions */
#define EXT_PART_IN_PROGRESS            1
#define EXT_PART_GET_UPK_INFO           2
#define EXT_PART_GET_GZ                 3
#define EXT_PART_UNCOMPRESS             4
#define EXT_PART_NO_PARAM               5
#define EXT_PART_NO_PACKAGE             6
#define EXT_PART_NO_EXTCARD             7
#define EXT_PART_READ_WRONG             8
#define EXT_PART_NET_WRONG              9
#define EXT_PART_CHECKSUM_ERROR         10
#define EXT_PART_UNCOMPRESS_ERROR       11
#define EXT_PART_UNABLE_ACCESS_SDRAM    12
#define EXT_PART_UNABLE_MMAP_SDRAM      13
#define EXT_PART_SYSTEM_ERROR           14
#define EXT_PART_UPDATE_COMPLETE        15
/* check extend storage and format status definitions */
#define EXT_PART_LOADING_UPK            16
#define EXT_STORAGE_IS_RDY              17
#define EXT_STORAGE_NO_STORAGE          18
#define EXT_STORAGE_NEED_FORMAT         19
#define EXT_FORMAT_IN_PROGRESS          20
#define EXT_FORMAT_FAILED               21
#define EXT_FORMAT_SUCCESS              22
#define EXT_UPK_TOO_LARGE               23
#define EXT_PART_NO_SPACE               24
#define EXT_PARTED_CARD_ERROR           25

#define EXT_DOWNLOAD_HEADER_SUCCESS     26
#define EXT_GET_PKG_HEADER_IN_PROGRESS  27
#define EXT_NEED_BE_CLEAN               28
#define EXT_FREE_SIZE_ENOUGH            29
#define EXT_UPK_ERROR                   30
#define EXT_PART_NO_EXIST               31

#define EXT_VERSION_SAME                0
#define EXT_VERSION_DIFF                1

#define UPTOMEM_START_ADDR   (0x100000+CFG_KERNEL_MEM_RESERVE*SZ_1M+PHYS_FLASH_SIZE*CFG_MAX_FLASH_BANKS)
#define UPTOMEM_MAX_SIZE     0x1000000  //16MB
#define PACK_OFFSET          0x1000

/* the value same as the kernel/linux/include/asm/arch-ntosd-dm320/hardware.h */
#define FB_SIZE              ((720*576*2)/SZ_4K+1)*SZ_4K 
#define MAX_UPK_SIZE_IN_CF   30*SZ_1M-PACK_OFFSET-FB_SIZE

#define NAMELEN        32
#define VERLEN         20  
#define DESCLEN        40  
#define SIGNATURELEN   52

typedef struct packet_header{
  unsigned long    p_headsize;       /* package header size         */
  unsigned long    p_reserve;        /* Bit[1:0] use to judging 8M or 16M flash package */
  unsigned long    p_headcrc;        /* package header crc checksum */
  unsigned long    p_datasize;       /* package data size           */
  unsigned long    p_datacrc;        /* package data crc checksum   */
  unsigned char    p_name[NAMELEN];  /* package name                */
  unsigned char    p_vuboot[VERLEN]; /* version of uboot which depend on */
  unsigned char    p_vkernel[VERLEN];/* version of kernel which depend on*/
  unsigned char    p_vrootfs[VERLEN];/* version of rootfs which depend on*/
  unsigned long    p_imagenum;       /* num of the images in package*/
                                /* follow is image info */
}package_header_t;

typedef struct image_info{
  unsigned long    i_type;           /* image type, uboot?        */
  unsigned long    i_imagesize;      /* size of image             */
  unsigned long    i_startaddr_p;    /* start address in packeage */
  unsigned long    i_startaddr_f;    /* start address in flash    */
  unsigned long    i_endaddr_f;      /* end address in flash      */
  unsigned char    i_name[NAMELEN];  /* image name                */
  unsigned char    i_version[VERLEN];/* image version             */
}image_info_t;

#define PACKAGE_ID "neuros-osd"
typedef struct version_struct
{
    unsigned char  upk_desc[DESCLEN];
    unsigned char  pack_id[NAMELEN];
    unsigned char  hw1_ver[VERLEN];
    unsigned char  hw2_ver[VERLEN];
    unsigned char  os_ver [VERLEN];
    unsigned char  app_ver[VERLEN];
}pack_info;

#define HW_FLAG 0x55AAAA55
typedef struct hw_struct
{
    unsigned long hw_flag;
    unsigned long hw_len;
}hw_info;

#define NEUROS_UPK_SIGNATURE "Neuros Technology International LLC"
typedef struct signature_struct{
    unsigned char   string[SIGNATURELEN];
    unsigned long   strcrc;
}signature_t;

char *  CoolGetUpkVersion(char * ver, const char * pkname);
char *  CoolGetUpkDesc(char * ver, const char * pkname);
void    CoolUpgradeFromPackage(const char *pkname);
int     CoolUpgradeStatus(void);

int     CoolExtStorageIsRdy(void);
int     CoolFormatExtStorage(void);
int     CoolGetFormatStatus(void);
int     CoolCompareExtVersion(const char *pkname);
int     CoolUpgradeExtPart(void);
int     CoolUpgradeExtPartStatus(void);

int     CoolNetUrlGetPkgHeader(const char *pkgname);
int     CoolNetUrlGetPkgHeaderStatus(void);
int     CoolCheckExtFreeSize(const char *pkgname);
void    CoolCleanExtStorage(void);
int     CoolNetUrlGetPkgExt(const char *pkgname);
int     CoolNetUrlGetPkgExtStatus(void);
int     CoolNetUrlGetPkgExtDownloaded(void);
int     CoolNetUrlGetPkgExtSize(void);

int     CoolNetUrlGetPkg(char *url);
int     CoolNetGetPkgStatus(void);
int     CoolNetGetPkgSize(void);
int     CoolNetGetPkgDownloaded(void);
void    CoolStopNetDownload(void);
void    CoolGetCurOsdVersion(char *version);
int     CoolExtGetTotalSize(void);
int     CoolExtGetCurSize(void);
void    CoolStopDownloadExt(void);

char*   CoolMemupkOpen(void);
void    CoolMemupkClose(char *pMemupk);
int     CoolMemupkGetSize(char *pMemupk);
int     CoolMemupkSetSize(char *pMemupk,int upksize);
int     CoolMemupkClearSize(char *pMemupk);

#endif /* UPGRADE__H */
