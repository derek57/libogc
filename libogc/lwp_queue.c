#include <stdlib.h>
#include "asm.h"
#include "processor.h"
#include "lwp_queue.h"

void _Chain_Initialize(Chain_Control *queue,void *start_addr,u32 num_nodes,u32 node_size)
{
	u32 count;
	Chain_Node *curr;
	Chain_Node *next;

#ifdef _LWPQ_DEBUG
	printf("_Chain_Initialize(%p,%p,%d,%d)\n",queue,start_addr,num_nodes,node_size);
#endif
	count = num_nodes;
	curr = _Chain_Head(queue);
	queue->perm_null = NULL;
	next = (Chain_Node*)start_addr;
	
	while(count--) {
		curr->next = next;
		next->prev = curr;
		curr = next;
		next = (Chain_Node*)(((void*)next)+node_size);
	}
	curr->next = _Chain_Tail(queue);
	queue->last = curr;
}

Chain_Node* _Chain_Get(Chain_Control *queue)
{
	u32 level;
	Chain_Node *ret = NULL;
	
	_CPU_ISR_Disable(level);
	if(!_Chain_Is_empty(queue)) {
		ret	 = _Chain_Get_first_unprotected(queue);
	}
	_CPU_ISR_Restore(level);
	return ret;
}

void _Chain_Append(Chain_Control *queue,Chain_Node *node)
{
	u32 level;
	
	_CPU_ISR_Disable(level);
	_Chain_Append_unprotected(queue,node);
	_CPU_ISR_Restore(level);
}

void _Chain_Extract(Chain_Node *node)
{
	u32 level;
	
	_CPU_ISR_Disable(level);
	_Chain_Extract_unprotected(node);
	_CPU_ISR_Restore(level);
}

void _Chain_Insert(Chain_Node *after,Chain_Node *node)
{
	u32 level;
	
	_CPU_ISR_Disable(level);
	_Chain_Insert_unprotected(after,node);
	_CPU_ISR_Restore(level);
}
