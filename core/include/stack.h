#ifndef STACK__H
#define STACK__H
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
 * Generic Stack routines.
 * Memory Managment of the stack is internal to the stack, user does not
 * modify coolstack_t memory directly.  Only one coolstack_t should ever be touched
 * and that is only to pass the stack pointer to the stack functions.
 *
 * REVISION:
 *
 *
 * 1) Initial creation. ----------------------------------- 2007-07-12 TOM
 *
 */

#include <stdlib.h>


typedef struct coolstack_t
{
	struct coolstack_t * next;  /* next item on stack */
	int level;		/* current depth of stack (size) */
	void* data;		/* data */

} coolstack_t;


int CoolStackPush(coolstack_t *top, void* data);
void * CoolStackPop(coolstack_t * stack);

const void * CoolStackPeek(coolstack_t * stack);

coolstack_t *  CoolStackNew(void);
int CoolStackFree(coolstack_t * stack);
int CoolStackSize(coolstack_t * stack); 


#endif

