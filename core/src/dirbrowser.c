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
 * directory tree routines.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2005-09-23 MG 
 *
 */

#include <unistd.h>
//#define OSD_DBG_MSG
#include "nc-err.h"
#include "dirtree.h"
#include "file-helper.h"
#include "stringutil.h"
#include "key-value-pair.h"

#define BLOCKSIZE 512
#define NETWORK_STR			"network"
#define UPNP_STR 				"upnpBrowser"
#define MEDIA_ROOT			"/media"
#define SYS_INIFILE    "/mnt/OSD/.syscfg"
#define GetSysKey(k,v)	CoolGetCharValue(SYS_INIFILE, "System",k, v, 200)

static const char *dirname;

static int Filter( int num, int * index, int * findex, 
				   struct dirent ** name, pf_dir_filter_t filter )
{
	int ii, jj= 0;

	if (num) 
	  {
		ii = 0;
		while (ii < num) 
		  {
			if ( filter(name[*index]->d_name) ) 
			  {
				jj++;
				*findex++ = *index;
			  }
			index++;
			ii++;
		  }
	  }
	return jj;
}

static int Compare(const void * d1, const void * d2)
{
	return strcasecmp((*(struct dirent **)d1)->d_name, (*(struct dirent **)d2)->d_name);
}

typedef struct
{
	dir_node_t * node;
	pf_dir_filter_t filter;
	int 			mode;
	
	pf_filter_t filterfp;
}mt_filter_param_t;

static void*
mtFilter(void *param)
{
	mt_filter_param_t *pm=(mt_filter_param_t *)param;
	dir_node_t * node=pm->node;
	pf_dir_filter_t filter=pm->filter;
	pf_filter_t filterfp=pm->filterfp;	

	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

	if(node->dirmt.sorttid)
		pthread_join(node->dirmt.sorttid,NULL);
	node->dirmt.mtstate=DMT_RUNNING;

	filterfp(node,filter,pm->mode);
	node->dirmt.filtertid=0;
	node->dirmt.mtstate=DMT_FINISHED;
//	pthread_detach(node->dirmt.sorttid);
	return NULL;
}

typedef struct
{
	dir_node_t * node;
	
	pf_sort_t sortfp;
}mt_sort_param_t;

static void*
mtSort(void *param)
{
	mt_sort_param_t *pm=(mt_sort_param_t *)param;
	dir_node_t * node=pm->node;
	pf_sort_t sortfp=pm->sortfp;	
	
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

	if(node->dirmt.mtstate!=DMT_RUNNING)
		return NULL; 

	sortfp(node);
	node->dirmt.sorttid=0;
	node->dirmt.mtstate=DMT_FINISHED;
//	pthread_detach(node->dirmt.sorttid);
	return NULL;
}

/**
 *	Specific compare function to place network and upnpBrowser string to end
 *	@param d1
 * 		pointer of node
 *	@param d2
 *		pointer of node
 *	@return
 *		> 0 : greater than, 
 *		= 0 : equal to,  
 *		< 0 : less than
 */
int
CoolRootDirCompare(const void * d1, const void * d2)
{
	int is_d1_upnp = strcmp((*(struct dirent **)d1)->d_name, UPNP_STR);
	int is_d2_net = strcmp((*(struct dirent **)d2)->d_name, NETWORK_STR);
	
	if (!is_d1_upnp && !is_d2_net)
		return Compare(d1, d2);
	else if (!is_d1_upnp)
		return 1;
	else if (!is_d2_net)
		return -1;
	else
	{
		int is_d1_net = strcmp((*(struct dirent **)d1)->d_name, NETWORK_STR);
		int is_d2_upnp = strcmp((*(struct dirent **)d2)->d_name, UPNP_STR);
		
		if (!is_d1_net && !is_d2_upnp)
			return Compare(d1, d2);
		else if (!is_d1_net)
			return 1;
		else if (!is_d2_upnp)
			return -1;
		else
			return Compare(d1, d2);
	}
}

/**
 * don't select  '.' and '..' when open directory.
 *
 * @param dt
 *        directory entry pointer.
 * @return
 *        1 seleted, 0 not select.
 */
int 
CoolDefSelectNoDotDot(const struct dirent *dt)
{
	return CoolDirFilterDotDot(dt->d_name);
}

/**
 * don't select  all hidden entry when open directory.
 *
 * @param dt
 *        directory entry pointer.
 * @return
 *        1 seleted, 0 not select.
 */
int 
CoolDefSelectNoHiddenEntry(const struct dirent *dt)
{
	return CoolDirFilterHiddenEntry(dt->d_name);
}

/**
 * Open the directory node, alphabetically sort the directory with
 * files coming after directories.
 *
 * @param path
 *        directory path.
 * @param node
 *        directory node pointer.
 * @param sortfp
 *	    sort function pointer
 * @param blockingsort
 *        block sort call or not
 * @return
 *        total number of entries found. -1 if error.
 */
int
CoolOpenDirectorySpecific( const char * path, dir_node_t * node,pf_def_select_t defselect, pf_sort_t sortfp,unsigned char blockingsort, pf_compare_t comp)
{
	
	int num,i;	
	unsigned int * index, * findex;

	dirname=path;
	node->scanned = FALSE;

	if (!comp)
	{
		if ((num = scandir(path, &node->namelist, defselect, Compare)) < 0) 
		{
			return -1;
		}
	}
	else
	{
		if ((num = scandir(path, &node->namelist, defselect, comp)) < 0) 
		{
			return -1;
		}
	}

	node->num = num;
	node->dnum = node->fdnum = 0;
	node->fnum = node->ffnum = num;	
	index= (unsigned int *)malloc(sizeof(unsigned int)*num);	  
	findex= (unsigned int *)malloc(sizeof(unsigned int)*num);	  

	node->dindex =node->findex =index;
	node->fdindex =node->ffindex	=findex;
	for(i=0;i<num;i++)
	{
		*index++ = i;
		*findex++ = i;		
	}
	node->path=strdup(path);
	node->scanned = TRUE;

	if(blockingsort)
	{
		node->dirmt.mtstate=DMT_SINGLE;
		if(sortfp)   sortfp(node);
	}
	else
	{
		node->dirmt.mtstate=DMT_RUNNING;
		node->dirmt.cindex=num-1;
		node->dirmt.sorttid=0;
		node->dirmt.filtertid=0;
		if(sortfp)
		{
			static mt_sort_param_t param;
			param.sortfp=sortfp;
			param.node=node;
			pthread_create(&node->dirmt.sorttid,NULL,mtSort,(void *)&param);
		}	
		else
			node->dirmt.mtstate=DMT_FINISHED;
	}
	return num;
}

typedef struct
{
	unsigned int * ofindex;
	unsigned int * offindex;
}sortdiv_data;

static void
freeSortDivFileDir(void *param)
{
	sortdiv_data *data = param;
	free(data->ofindex);
	free(data->offindex);
	free(data);
}

/**
 * Sort the directory node, divide file and directory,
 * directories will in top,then is files.
 *
 * @param node
 *        directory node pointer.
 * @return
 *        0 is success.
 */
int
CoolSortDivFileDir(dir_node_t *node)
{
	unsigned int num=node->num;
	if (num) 
	  {
		unsigned int * dindex, * fdindex, * findex, * ffindex;	  
		int ii;
		struct dirent ** name;
		struct stat st;
		char tmpstr[PATH_MAX];
		int oldcanceltype;
		sortdiv_data *data = malloc(sizeof(sortdiv_data));;

		pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldcanceltype); 
		data->ofindex = node->findex = findex = (unsigned int *)malloc(sizeof(unsigned int)*num);	  
		data->offindex = node->ffindex = ffindex = (unsigned int *)malloc(sizeof(unsigned int)*num);			
		pthread_cleanup_push( freeSortDivFileDir,data); 	

		memcpy(node->findex,node->dindex,sizeof(unsigned int) *num);
		memcpy(node->ffindex,node->fdindex,sizeof(unsigned int) *num);

		ii = 0;
		dindex = node->dindex;
		fdindex = node->fdindex;
		node->dnum = node->fdnum = 0;
		node->fnum = node->ffnum = 0;
		node->dirmt.cindex=0;
		name = node->namelist; 
		while (ii < node->num) 
		  {
			pthread_testcancel();
			CoolCatFilename2Path(tmpstr,PATH_MAX,node->path,(*name)->d_name);
			if ( 0 == stat(tmpstr, &st) ) 
			  {

				if ( S_ISDIR(st.st_mode) ) 
				  {  
				  	node->dirmt.cindex=node->dnum;
					*dindex++ = ii;
					*fdindex++ = ii;	
					node->dnum++;
					node->fdnum++;
					
				  }
				else
				{	
					*findex++ = ii;
					*ffindex++ = ii;					
					node->fnum++;
					node->ffnum++;				
				}
				node->dirmt.cindex++;
			  } 
			else 
			  { 
				WARNLOG("Directory status not available!\n");
				free(*name);
				*name = NULL;
			  }
			ii++;
			name++;
		  }
		memcpy(node->dindex+node->dnum,node->findex,sizeof(unsigned int)*node->fnum);
		node->findex=node->dindex+node->dnum;
		memcpy(node->fdindex+node->fdnum,node->ffindex,sizeof(unsigned int)*node->ffnum);
		node->ffindex=node->fdindex+node->fdnum;	
		node->dirmt.cindex=0;

		pthread_cleanup_pop(1); 	
		pthread_setcanceltype(oldcanceltype, NULL);		

	  }

	return 0;
}


static int
filterdir( dir_node_t * node, pf_dir_filter_t filter, int mode )
{
	if ( FALSE == node->scanned ) return -1;
	if ( 0 == mode ) 
	  {
		node->fdnum = Filter(node->dnum, node->dindex, 
							 node->fdindex,
							 node->namelist, filter);
		node->ffnum = Filter(node->fnum, node->findex, 
							 node->ffindex,
							 node->namelist, filter);
	  } 
	else 
	  {
		node->fdnum = Filter(node->fdnum, node->fdindex, 
							 node->fdindex,
							 node->namelist, filter);
		node->ffnum = Filter(node->ffnum, node->ffindex, 
							 node->ffindex,
							 node->namelist, filter);
	  }
	node->dirmt.cindex=0;
	return (node->fdnum+node->ffnum);
}

/**
 * Filter the directory node with given filter function.
 *
 * @param node
 *        directory node pointer.
 * @param filter
 *        filter function.
 * @param mode
 *        filter mode, 0 to apply on all, 1 to apply on current filtered out.
 * @return
 *       number of entries available after filtering, -1 if error occurs.
 */
int
CoolFilterDirectory( dir_node_t * node, pf_dir_filter_t filter, int mode )
{
	if(NULL==filter)
		return -1;

	if(node->dirmt.mtstate==DMT_SINGLE)
	{
		return filterdir( node, filter, mode);
	}
	else
	{

		static mt_filter_param_t param;
		param.filterfp=filterdir;
		param.filter=filter;
		param.node=node;
		param.mode=mode;
		pthread_create(&node->dirmt.filtertid,NULL,mtFilter,(void *)&param);		
		return (node->fdnum+node->ffnum);
	}
}


/**
 * Close the opened directory node.
 *
 * @param node
 *        directory node pointer.
 */
void
CoolCloseDirectory( dir_node_t * node )
{
	unsigned int num;
	struct dirent ** name;

	if ( FALSE == node->scanned ) return;
	if(node->dirmt.mtstate!=DMT_SINGLE)
	{
		if(node->dirmt.sorttid)
			pthread_cancel(node->dirmt.sorttid);	
		if(node->dirmt.filtertid)
			pthread_cancel(node->dirmt.filtertid);	
		pthread_join(node->dirmt.sorttid,NULL);	
		pthread_join(node->dirmt.filtertid,NULL);
	}
	if(node->path)
	{
		free(node->path);
		node->path=NULL;
	}
	num = node->num;
	if ( 0 == num ) return;

	name = node->namelist;
	if(name)
	{
		while ( num-- ) 
		{
			if(*name) free(*name);
			name++;
		}
	}
	if(node->dindex)
	{
		free(node->dindex);
		node->dindex=NULL;
	}
	if(node->fdindex)
	{
		free(node->fdindex);
		node->fdindex=NULL;
	}
	if(node->namelist)
	{
		free(node->namelist);
		node->namelist=NULL;
	}  
}

static int
FilterUpnp(const char * name)
{
	int ret=1;
	int show_upnp;
	char sdv[PATH_MAX];
	int b;
	if (!strcmp(dirname,MEDIA_ROOT))
	{
		sprintf(sdv, "%d", 0);
		b=GetSysKey("UpnpBrowser", sdv);
		show_upnp=atoi(sdv);   
		if ((!show_upnp)&&(!strcmp(name,UPNP_STR)))
		{
			ret=0;
		}
	}
	return ret;
}
/**
 * Filter all dot started entries.
 *
 * @param name
 *        directory/file name input.
 * @return
 *        1 if pass, 0 filtered.
 */
int
CoolDirFilterHiddenEntry( const char * name )
{
	if(!FilterUpnp(name))
		return 0;
	return !CoolIsFileHidden(name);
}

/**
 * Filter '.' and '..'.
 *
 * @param name
 *        directory/file name input.
 * @return
 *        1 if pass, 0 filtered.
 */
int
CoolDirFilterDotDot( const char * name )
{
	if(!FilterUpnp(name))
		return 0;
	int len = strlen(name);
	if ( (1==len && '.'==name[0]) || (2==len && '.'==name[0] && '.'==name[1]) ) 
		return 0;

	return 1;
}

/**
 * Get file size 
 *
 * @param fileName
 *		file name
 * @return 
 *		file size, unit byte			
 */
int 
CoolGetFileSize(const char *fileName)
{
	struct stat buf;
	if (0 == stat(fileName, &buf))
	{
		return buf.st_size;
	}
	return 0;
}

/**
 * Get directory size 
 *
 * @param fileName
 *		directory name
 * @return 
 *		directory size, unit byte			
 */
int
CoolGetDirectorySize(const char *dirname)    
{    
	char   pn[PATH_MAX];   
	DIR   *dp;
	int   f_size,d_size;
	int   i;
	struct   stat   sbuf;
	struct   direct   *dir;
	f_size=0;
	//DBGLOG("Current   directory   is   %s\n",name);
	for( i=0;i<=1;i++ )
	{
		if( (dp=opendir(dirname))==NULL )
		{
			//perror(dirname);
			return -1;
		}
		while( (dir=readdir(dp))!=NULL )
		{
			if(dir->d_ino==0)
				continue;

			strlcpy(pn, dirname, sizeof(PATH_MAX));
			strlcat(pn, "/", sizeof(PATH_MAX));
			strlcat(pn, dir->d_name, sizeof(PATH_MAX));
			if( lstat(pn,&sbuf)<0 )
			{
				//perror(pn);
				return -1;
			}
			if( ((sbuf.st_mode&S_IFMT)!=S_IFLNK)&&((sbuf.st_mode&S_IFMT)==S_IFDIR)&&(strcmp(dir->d_name,".")!=0)&&(strcmp(dir->d_name,"..")!=0) )
			{
				if( i==1 )
				{
					d_size=CoolGetDirectorySize(pn);
					if( d_size==-1 )
						return -1;
					f_size=f_size+d_size;
				}
			}
			else
			{
				if( i==0 )
				{
					f_size=f_size+sbuf.st_blocks*BLOCKSIZE;
				}
			}
		}
		closedir(dp);
	}
	return   f_size;
}
