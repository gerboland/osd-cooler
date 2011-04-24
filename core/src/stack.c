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
 * Generic Stack routines.  All stack access is provided.
 * Memory Managment of the stack is internal to the stack, user does not
 * modify stack_t memory directly.  Only one stack_t should ever be touched
 * and that is only to pass the stack pointer to the stack functions.
 *
 * REVISION:
 *
 * 1) Initial creation. ----------------------------------- 2007-07-12 MG
 *
 *
 */

#include "stack.h"
#include <stdlib.h>

/**
 * Function creates a new item on the stack.
 *
 * @param stack
 *        pointer to the initialized stack
 * @param data
 *        list data.
 * @return
 *        New size of stack, 0 For error.
 */
int CoolStackPush(coolstack_t *stack, void* data)
{
	if (!stack) return 0;
	
	coolstack_t* newitem = malloc(sizeof(coolstack_t));

	newitem->next = stack->next;
	newitem->data = stack->data;
	newitem->level = stack->level;

	stack->next = newitem;
	stack->data = data;
	stack->level = stack->level + 1;

	return stack->level;

}

/**
 * Function returns data stored at top of stack without removing from stack
 *
 * @param stack
 *        pointer to the initialized stack
 * @return
 *        Data stored 
 */
const void* CoolStackPeek(coolstack_t * stack)
{
	if (!stack) return NULL;
	
	return stack->data;
}


/**
 * Function returns data stored at top of stack and removes data from stack
 *
 * @param stack
 *        pointer to the initialized stack
 * @return
 *        Data stored 
 */
void* CoolStackPop(coolstack_t * stack)
{
	if (!stack) return NULL;
	
	coolstack_t* temp = NULL;
	void* data = NULL;


	if(stack->level > 0)
	{
		data = stack->data;
		temp = stack->next;		

		stack->level = stack->next->level;
		stack->data = stack->next->data;
		stack->next = stack->next->next;

		free(temp);
		
	}
	else
	{
		return NULL;
	}

	
	return data;


}

/**
 * Function returns an initialized Stack
 *
 * @return
 *        pointer to stack initialized memory
 */
coolstack_t* CoolStackNew(void)
{

	coolstack_t* newstack = malloc(sizeof(coolstack_t));
	newstack->next = NULL;
	newstack->data = NULL;
	newstack->level = 0;

	return newstack;
}

/**
 * Function frees memory from an empty stack.
 *
 * @param stack
 *       stack to free
 * @return
 *        0 error, >0 success
 *
 *  DO NOT FREE A NON EMPTY STACK. Will FAIL and cause leak.
 *  Pop all data off stack and free first.
 */
int CoolStackFree(coolstack_t* stack)
{
	int success = 0;

	if (!stack) return success;
	
	if(stack->level > 0 || stack->next != NULL || stack->data != NULL)
	{
		success = 0;
	}
	else
	{
		free(stack);
		success = 1;	
	}

	return success;
		
}

/**
 * Function returns current size/depth of stack
 *
 * @param stack
 *       stack to get size of
 * @return
 *        stack size
 *
 */
int CoolStackSize(coolstack_t * stack)
{
	if (!stack) return 0;
	return stack->level;
}

