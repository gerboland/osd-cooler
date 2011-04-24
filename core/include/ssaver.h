#ifndef SSAVER__H
#define SSAVER__H
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
 * Screen saver support module header.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2006-06-07  EY
 */

#include <stdio.h>
#include <stdlib.h>


typedef struct 
{
  const char * name;
  void (*ShowScrSaver)(void *data);
  void (*HideScrSaver)(void);
  const char ** description;
} ssaver_plugin_t;


typedef enum 
{
	spsPause,
	spsResume,
}SSAVER_PR_STATUS;

#define SSAVER_NAME_MAX_LEN  20

int     	SSaverInit(void);
int		SSaverGetName(int index,char *name);
void		SSaverStop(void);
int		NeuxSSaverGetName(int index,const char **name);
int		NeuxSSaverSetTimeout(int timeout);
int		NeuxSSaverChoose(const char *name);
int		NeuxSSaverPauseResume(SSAVER_PR_STATUS pr);
int		NeuxSSaverPreview(const char *name);
int    	SSaverChoose(const char *name);
void 	SSaverClose(void);

#endif /* SSAVER__H */

