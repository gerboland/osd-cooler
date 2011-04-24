#ifndef LIST__H
#define LIST__H
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
 * Generic list module header.
 *
 * REVISION:
 * 
 * 
 * 1) Initial creation. ----------------------------------- 2005-08-29 MG 
 *
 */

#include <stdio.h>
#include <stdlib.h>

typedef struct slist_t 
{
	struct slist_t * next;    /* next item */
	void *           data;    /* data */
} slist_t;

typedef struct dlist_t 
{
	struct dlist_t * next;    /* next item */
	struct dlist_t * prev;    /* previous item */
	void *           data;    /* data */
} dlist_t;

typedef enum {
	DLIP_AFTER,
	DLIP_BEFORE
} DLIST_INSERT_POS;

typedef int (*DlistIterateCallback)(dlist_t * item, void * userdata);

slist_t * CoolSlistNew   (void * data);
slist_t * CoolSlistInsert(slist_t * curItem, slist_t * newItem);
slist_t * CoolSlistRemove(slist_t * head, slist_t * item);
slist_t * CoolSlistTail  (slist_t * head);
void  CoolSlistFree  (slist_t * head);

dlist_t * CoolDlistIndex(dlist_t * start, int index);
dlist_t * CoolDlistNew   (void * data);
dlist_t * CoolDlistInsert(dlist_t * curItem, dlist_t * newItem, DLIST_INSERT_POS position);
dlist_t * CoolDlistRemove(dlist_t * item);
dlist_t * CoolDlistIteratePrev (dlist_t * start, DlistIterateCallback cb, void* userdata);
dlist_t * CoolDlistIterateNext (dlist_t * start, DlistIterateCallback cb, void* userdata);
void CoolDlistFree       (dlist_t * item, int freeData);

#endif /* LIST__H */

