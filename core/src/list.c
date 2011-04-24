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
 * Generic list routines.
 *
 * REVISION:
 * 
 * 2) FIX:CoolSlistTail should return NULL if list is empty. -- 2005-10-26 MG
 * 1) Initial creation. ----------------------------------- 2005-08-29 MG 
 *
 */

#include "list.h"

/**
 * Function creates a new list item.
 *
 * @param data
 *        list data.
 * @return
 *        pointer to new list item if successful, otherwise NULL.
 */
slist_t *
CoolSlistNew( void * data )
{
	slist_t * new = (slist_t*)malloc(sizeof(slist_t));
	
	if (NULL == new) return NULL;

	new->next = NULL;
	new->data = data;
	
	return new;
}

/**
 * Insert new item right after the current item.
 *
 * @param curItem
 *        current item.
 * @param newItem
 *        inserting item.
 * @return
 *        pointer to newItem after addition.
 */
slist_t *
CoolSlistInsert( slist_t * curItem, slist_t * newItem )
{
  	slist_t * item;
	item = curItem->next;
	curItem->next = newItem;
	newItem->next = item;

	return newItem;	
}

/**
 * Remove specified item.
 *
 * @param head
 *        head item.
 * @param item
 *        removing item.
 * @return
 *        current list head, NULL if list becomes empty.        
 */
slist_t *
CoolSlistRemove( slist_t * head, slist_t * item )
{
  	slist_t * cur;
	slist_t * pre;

	if (head == item)
	  {
		head = head->next;
	  }
	else
	  {
		cur = head;
		do
		  {
			pre = cur;
			cur = cur->next;
			if (NULL == cur) return head;
		  } while (cur != item);
		
		pre->next = item->next;
	  }

	free(item);
	return head;
}

/**
 * Return current list tail.
 *
 * @param head
 *        head item.
 * @return
 *        list tail.
 */
slist_t *
CoolSlistTail( slist_t * head )
{
  	slist_t * cur = head;
	if (cur)
	  {
		while (cur->next) cur = cur->next;
	  }

	return cur;
}

/**
 * destory current list to tail.
 *
 * @param head
 *        head item.
 */
void
CoolSlistFree( slist_t * head )
{
  	slist_t * cur = head;
	slist_t * tmp;

	while (cur) 
	{
		tmp=cur;
		cur = cur->next;
		free(tmp->data);
		free(tmp);
	}
	

	return ;
}

// -------------------- d-list ----------------------------


/**
 * Function creates a new D-list item.
 *
 * @param data
 *        list data.
 * @return
 *        pointer to new D-list item if successful, otherwise NULL.
 */
dlist_t *
CoolDlistNew(void * data)
{
	dlist_t * new = (dlist_t*)malloc(sizeof(dlist_t));
	
	if (NULL == new) return NULL;

	new->next = NULL;
	new->prev = NULL;
	new->data = data;
	
	return new;
}

/**
 * Insert new item either after or before the current item.
 *
 * @param curItem
 *        current item.
 * @param newItem
 *        inserting item.
 * @param insertAfter
 *        DLIP_AFTER to add newItem after curItem
 *        DPLI_BEFORE to add it before that
 * @return
 *        pointer to newItem after addition.
 */
dlist_t *
CoolDlistInsert(dlist_t * curItem, dlist_t * newItem, DLIST_INSERT_POS position)
{
	dlist_t * item;
	if (position == DLIP_AFTER) 
	{
		item = curItem->next;
		if (item) 
		{
			item->prev = newItem;
			newItem->next = item;
		}
		newItem->prev = curItem;
		curItem->next = newItem;
	} 
   else 
   {
		item = curItem->prev;
		if (item)
		{
			item->next = newItem;
			newItem->prev = item;
		}
		curItem->prev = newItem;
		newItem->next = curItem;
	}
	return newItem;
}

/**
 * Find a item in the list by index.
 * 
 * @param start 
 *        the start item begin to search
 * @param index
 *        the index of the item be searched
 * @retrun
 *        pointer of the item, if no found return NULL.
 */
dlist_t *
CoolDlistIndex(dlist_t * start, int index)
{
    dlist_t * item;
    item = start;
    if(index == 0) return item;
	else if(index>0)
	{
		while (index)
		{
			if (item->next == NULL)
				return NULL;
			item = item->next;
			index--;
		}
	}
	else
	{
		while (index)
		{
			if (item->prev == NULL )
				return NULL;
			item = item->prev;
			index++;
		}
	}
    return item;
}

/**
 * Remove specified item. The item is free'd, but not the data it owned.
 *
 * @param item
 *        item to be removed.
 * @return
 *        the item after the one removed.
 */
dlist_t *
CoolDlistRemove(dlist_t * item)
{
	dlist_t *ret;
	ret = NULL;

	if (item->prev != NULL) 
	{
		ret = item->prev;
		ret->next = item->next;
	}
	if (item->next != NULL) 
	{
		if (ret == NULL) ret = item->next;
		item->next->prev = item->prev;
	}

	item->data = NULL;
	item->next = NULL;
	item->prev = NULL;

	free(item);
	return ret;
}

/**
 * Iterate the list following the next elements util tail is found. 
 * The callback function will be called once for each element.
 * You should return 1 from the callback if you want to stop the iteration. 
 *
 * @param start
 *        Where to start iterating. the callback will be called for this item too.
 * @param direction
 *        DLED_FORWARDS to follow the next links when iterating.
 *        DLED_BACKWARDS to follow the prev links when iterating.
 * @param cb
 *        The callback function that will be called.
 * @param data
 *        A pointer to arbitrary user data. It will be passed to the callback at each iteration.
 * @return
 *        The last node that was visited.
 */
dlist_t* CoolDlistIterateNext (dlist_t * start, DlistIterateCallback cb, void* userdata)
{
   while (start)
   {
      if (cb) if (cb(start, userdata) == 1) return start;
      if (start->next == NULL) return start;
      start = start->next;
   }
   return NULL;
}

/**
 * Iterate the list following the prev elements util head is found. 
 * The callback function will be called once for each element.
 * You should return 1 from the callback if you want to stop the iteration. 
 *
 * @param start
 *        Where to start iterating. the callback will be called for this item too.
 * @param direction
 *        DLED_FORWARDS to follow the next links when iterating.
 *        DLED_BACKWARDS to follow the prev links when iterating.
 * @param cb
 *        The callback function that will be called.
 * @param data
 *        A pointer to arbitrary user data. It will be passed to the callback at each iteration.
 * @return
 *        The last node that was visited.
 */
dlist_t* CoolDlistIteratePrev (dlist_t * start, DlistIterateCallback cb, void* userdata)
{
   while (start)
   {
      if (cb) if (cb(start, userdata) == 1) return start;
      if (start->prev == NULL) return start;
      start = start->prev;
   }
   return NULL;
}

/**
 * Destory current list to tail. Current item is destroyed too.
 * Pass head if you want to destroy whole list.
 *
 * @param item
 *        any item.
 * @param freeData
 *        1 to call free() on the data owned by items, otherwise 
 *        memory for them will remain allocated.
 */
void
CoolDlistFree(dlist_t * item, int freeData)
{
	while (item) 
	{
		if (freeData) free(item->data);
		item = CoolDlistRemove(item);
	}
}
