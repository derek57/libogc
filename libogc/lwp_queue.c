#include <stdlib.h>
#include "asm.h"
#include "processor.h"
#include "lwp_queue.h"

void _Chain_Initialize(Chain_Control *the_chain,void *starting_address,u32 number_nodes,u32 node_size)
{
	u32 count;
	Chain_Node *current;
	Chain_Node *next;

#ifdef _LWPQ_DEBUG
	printf("_Chain_Initialize(%p,%p,%d,%d)\n",the_chain,starting_address,number_nodes,node_size);
#endif
	count = number_nodes;
	current = _Chain_Head(the_chain);
	the_chain->permanent_null = NULL;
	next = (Chain_Node*)starting_address;
	
	while(count--) {
		current->next = next;
		next->previous = current;
		current = next;
		next = (Chain_Node*)(((void*)next)+node_size);
	}
	current->next = _Chain_Tail(the_chain);
	the_chain->last = current;
}

Chain_Node* _Chain_Get(Chain_Control *the_chain)
{
	u32 level;
	Chain_Node *return_node = NULL;
	
	_ISR_Disable(level);
	if(!_Chain_Is_empty(the_chain)) {
		return_node	 = _Chain_Get_first_unprotected(the_chain);
	}
	_ISR_Enable(level);
	return return_node;
}

void _Chain_Append(Chain_Control *the_chain,Chain_Node *node)
{
	u32 level;
	
	_ISR_Disable(level);
	_Chain_Append_unprotected(the_chain,node);
	_ISR_Enable(level);
}

void _Chain_Extract(Chain_Node *node)
{
	u32 level;
	
	_ISR_Disable(level);
	_Chain_Extract_unprotected(node);
	_ISR_Enable(level);
}

void _Chain_Insert(Chain_Node *after_node,Chain_Node *node)
{
	u32 level;
	
	_ISR_Disable(level);
	_Chain_Insert_unprotected(after_node,node);
	_ISR_Enable(level);
}
