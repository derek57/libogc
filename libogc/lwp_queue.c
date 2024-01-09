#include <stdlib.h>
#include "asm.h"
#include "processor.h"
#include "lwp_queue.h"

void _Chain_Initialize(lwp_queue *queue,void *start_addr,u32 num_nodes,u32 node_size)
{
	u32 count;
	lwp_node *curr;
	lwp_node *next;

#ifdef _LWPQ_DEBUG
	printf("_Chain_Initialize(%p,%p,%d,%d)\n",queue,start_addr,num_nodes,node_size);
#endif
	count = num_nodes;
	curr = _Chain_Head(queue);
	queue->perm_null = NULL;
	next = (lwp_node*)start_addr;
	
	while(count--) {
		curr->next = next;
		next->prev = curr;
		curr = next;
		next = (lwp_node*)(((void*)next)+node_size);
	}
	curr->next = _Chain_Tail(queue);
	queue->last = curr;
}

lwp_node* _Chain_Get(lwp_queue *queue)
{
	u32 level;
	lwp_node *ret = NULL;
	
	_CPU_ISR_Disable(level);
	if(!_Chain_Is_empty(queue)) {
		ret	 = _Chain_Get_first_unprotected(queue);
	}
	_CPU_ISR_Restore(level);
	return ret;
}

void _Chain_Append(lwp_queue *queue,lwp_node *node)
{
	u32 level;
	
	_CPU_ISR_Disable(level);
	_Chain_Append_unprotected(queue,node);
	_CPU_ISR_Restore(level);
}

void _Chain_Extract(lwp_node *node)
{
	u32 level;
	
	_CPU_ISR_Disable(level);
	_Chain_Extract_unprotected(node);
	_CPU_ISR_Restore(level);
}

void _Chain_Insert(lwp_node *after,lwp_node *node)
{
	u32 level;
	
	_CPU_ISR_Disable(level);
	_Chain_Insert_unprotected(after,node);
	_CPU_ISR_Restore(level);
}
