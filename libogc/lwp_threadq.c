#include <stdio.h>
#include <lwp_watchdog.h>
#include "asm.h"
#include "lwp_threadq.h"

//#define _LWPTHRQ_DEBUG

static void _Thread_queue_Timeout(void *usr_data)
{
	Thread_Control *thethread;
	Thread_queue_Control *thequeue;
	
	_Thread_Disable_dispatch();
	thethread = (Thread_Control*)usr_data;
	thequeue = thethread->Wait.queue;
	if(thequeue->sync_state!=LWP_THREADQ_SYNCHRONIZED && _Thread_Is_executing(thethread)) {
		if(thequeue->sync_state!=LWP_THREADQ_SATISFIED) thequeue->sync_state = LWP_THREADQ_TIMEOUT;
	} else {
		thethread->Wait.return_code = thethread->Wait.queue->timeout_status;
		_Thread_queue_Extract(thethread->Wait.queue,thethread);
	}
	_Thread_Unnest_dispatch();
}

Thread_Control* _Thread_queue_First_fifo(Thread_queue_Control *queue)
{
	if(!_Chain_Is_empty(&queue->Queues.Fifo))
		return (Thread_Control*)queue->Queues.Fifo.first;

	return NULL;
}

Thread_Control* _Thread_queue_First_priority(Thread_queue_Control *queue)
{
	u32 index;

	for(index=0;index<LWP_THREADQ_NUM_PRIOHEADERS;index++) {
		if(!_Chain_Is_empty(&queue->Queues.Priority[index]))
			return (Thread_Control*)queue->Queues.Priority[index].first;
	}
	return NULL;
}

void _Thread_queue_Enqueue_fifo(Thread_queue_Control *queue,Thread_Control *thethread,u64 timeout)
{
	u32 level,sync_state;

	_CPU_ISR_Disable(level);
	
	sync_state = queue->sync_state;
	queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue_fifo(%p,%d)\n",thethread,sync_state);
#endif
	switch(sync_state) {
		case LWP_THREADQ_SYNCHRONIZED:
			break;
		case LWP_THREADQ_NOTHINGHAPPEND:
			_Chain_Append_unprotected(&queue->Queues.Fifo,&thethread->Object.Node);
			_CPU_ISR_Restore(level);
			return;
		case LWP_THREADQ_TIMEOUT:
			thethread->Wait.return_code = thethread->Wait.queue->timeout_status;
			_CPU_ISR_Restore(level);
			break;
		case LWP_THREADQ_SATISFIED:
			if(_Watchdog_Is_active(&thethread->Timer)) {
				_Watchdog_Deactivate(&thethread->Timer);
				_CPU_ISR_Restore(level);
				_Watchdog_Remove_ticks(&thethread->Timer);
			} else
				_CPU_ISR_Restore(level);

			break;
	}
	_Thread_Unblock(thethread);
}

Thread_Control* _Thread_queue_Dequeue_fifo(Thread_queue_Control *queue)
{
	u32 level;
	Thread_Control *ret;

	_CPU_ISR_Disable(level);
	if(!_Chain_Is_empty(&queue->Queues.Fifo)) {
		ret = (Thread_Control*)_Chain_Get_first_unprotected(&queue->Queues.Fifo);
		if(!_Watchdog_Is_active(&ret->Timer)) {
			_CPU_ISR_Restore(level);
			_Thread_Unblock(ret);
		} else {
			_Watchdog_Deactivate(&ret->Timer);
			_CPU_ISR_Restore(level);
			_Watchdog_Remove_ticks(&ret->Timer);
			_Thread_Unblock(ret);
		}
		return ret;
	}
	
	switch(queue->sync_state) {
		case LWP_THREADQ_SYNCHRONIZED:
		case LWP_THREADQ_SATISFIED:
			_CPU_ISR_Restore(level);
			return NULL;
		case LWP_THREADQ_NOTHINGHAPPEND:
		case LWP_THREADQ_TIMEOUT:
			queue->sync_state = LWP_THREADQ_SATISFIED;
			_CPU_ISR_Restore(level);
			return _thr_executing;
	}
	return NULL;
}

void _Thread_queue_Enqueue_priority(Thread_queue_Control *queue,Thread_Control *thethread,u64 timeout)
{
	u32 level,search_prio,header_idx,prio,block_state,sync_state;
	Thread_Control *search_thread;
	Chain_Control *header;
	Chain_Node *cur_node,*next_node,*prev_node,*search_node;

	_Chain_Initialize_empty(&thethread->Wait.Block2n);
	
	prio = thethread->current_priority;
	header_idx = prio/LWP_THREADQ_PRIOPERHEADER;
	header = &queue->Queues.Priority[header_idx];
	block_state = queue->state;

	if(prio&LWP_THREADQ_REVERSESEARCHMASK) {
#ifdef _LWPTHRQ_DEBUG
		printf("_Thread_queue_Enqueue_priority(%p,reverse_search)\n",thethread);
#endif
		goto reverse_search;
	}

#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue_priority(%p,forward_search)\n",thethread);
#endif
forward_search:
	search_prio = LWP_PRIO_MIN - 1;
	_CPU_ISR_Disable(level);
	search_thread = (Thread_Control*)header->first;
	while(!_Chain_Is_tail(header,(Chain_Node*)search_thread)) {
		search_prio = search_thread->current_priority;
		if(prio<=search_prio) break;
		_CPU_ISR_Flash(level);

		if(!_States_Are_set(search_thread->current_state,block_state)) {
			_CPU_ISR_Restore(level);
			goto forward_search;
		}
		search_thread = (Thread_Control*)search_thread->Object.Node.next;
	}
	if(queue->sync_state!=LWP_THREADQ_NOTHINGHAPPEND) goto synchronize;
	queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
	if(prio==search_prio) goto equal_prio;

	search_node = (Chain_Node*)search_thread;
	prev_node = search_node->previous;
	cur_node = (Chain_Node*)thethread;
	
	cur_node->next = search_node;
	cur_node->previous = prev_node;
	prev_node->next = cur_node;
	search_node->previous = cur_node;
	_CPU_ISR_Restore(level);
	return;

reverse_search:
	search_prio = LWP_PRIO_MAX + 1;
	_CPU_ISR_Disable(level);
	search_thread = (Thread_Control*)header->last;
	while(!_Chain_Is_head(header,(Chain_Node*)search_thread)) {
		search_prio = search_thread->current_priority;
		if(prio>=search_prio) break;
		_CPU_ISR_Flash(level);

		if(!_States_Are_set(search_thread->current_state,block_state)) {
			_CPU_ISR_Restore(level);
			goto reverse_search;
		}
		search_thread = (Thread_Control*)search_thread->Object.Node.previous;
	}
	if(queue->sync_state!=LWP_THREADQ_NOTHINGHAPPEND) goto synchronize;
	queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
	if(prio==search_prio) goto equal_prio;

	search_node = (Chain_Node*)search_thread;
	next_node = search_node->next;
	cur_node = (Chain_Node*)thethread;
	
	cur_node->next = next_node;
	cur_node->previous = search_node;
	search_node->next = cur_node;
	next_node->previous = cur_node;
	_CPU_ISR_Restore(level);
	return;

equal_prio:
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue_priority(%p,equal_prio)\n",thethread);
#endif
	search_node = _Chain_Tail(&search_thread->Wait.Block2n);
	prev_node = search_node->previous;
	cur_node = (Chain_Node*)thethread;

	cur_node->next = search_node;
	cur_node->previous = prev_node;
	prev_node->next = cur_node;
	search_node->previous = cur_node;
	_CPU_ISR_Restore(level);
	return;

synchronize:
	sync_state = queue->sync_state;
	queue->sync_state = LWP_THREADQ_SYNCHRONIZED;

#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue_priority(%p,sync_state = %d)\n",thethread,sync_state);
#endif
	switch(sync_state) {
		case LWP_THREADQ_SYNCHRONIZED:
			break;
		case LWP_THREADQ_NOTHINGHAPPEND:
			break;
		case LWP_THREADQ_TIMEOUT:
			thethread->Wait.return_code = thethread->Wait.queue->timeout_status;
			_CPU_ISR_Restore(level);
			break;
		case LWP_THREADQ_SATISFIED:
			if(_Watchdog_Is_active(&thethread->Timer)) {
				_Watchdog_Deactivate(&thethread->Timer);
				_CPU_ISR_Restore(level);
				_Watchdog_Remove_ticks(&thethread->Timer);
			} else
				_CPU_ISR_Restore(level);
			break;
	}
	_Thread_Unblock(thethread);
}

Thread_Control* _Thread_queue_Dequeue_priority(Thread_queue_Control *queue)
{
	u32 level,idx;
	Thread_Control *newfirstthr,*ret = NULL;
	Chain_Node *newfirstnode,*newsecnode,*last_node,*next_node,*prev_node;

	_CPU_ISR_Disable(level);
	for(idx=0;idx<LWP_THREADQ_NUM_PRIOHEADERS;idx++) {
		if(!_Chain_Is_empty(&queue->Queues.Priority[idx])) {
			ret	 = (Thread_Control*)queue->Queues.Priority[idx].first;
			goto dequeue;
		}
	}

#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Dequeue_priority(%p,sync_state = %d)\n",ret,queue->sync_state);
#endif
	switch(queue->sync_state) {
		case LWP_THREADQ_SYNCHRONIZED:
		case LWP_THREADQ_SATISFIED:
			_CPU_ISR_Restore(level);
			return NULL;
		case LWP_THREADQ_NOTHINGHAPPEND:
		case LWP_THREADQ_TIMEOUT:
			queue->sync_state = LWP_THREADQ_SATISFIED;
			_CPU_ISR_Restore(level);
			return _thr_executing;
	}

dequeue:
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Dequeue_priority(%p,dequeue)\n",ret);
#endif
	newfirstnode = ret->Wait.Block2n.first;
	newfirstthr = (Thread_Control*)newfirstnode;
	next_node = ret->Object.Node.next;
	prev_node = ret->Object.Node.previous;
	if(!_Chain_Is_empty(&ret->Wait.Block2n)) {
		last_node = ret->Wait.Block2n.last;
		newsecnode = newfirstnode->next;
		prev_node->next = newfirstnode;
		next_node->previous = newfirstnode;
		newfirstnode->next = next_node;
		newfirstnode->previous = prev_node;
		
		if(!_Chain_Has_only_one_node(&ret->Wait.Block2n)) {
			newsecnode->previous = _Chain_Head(&newfirstthr->Wait.Block2n);
			newfirstthr->Wait.Block2n.first = newsecnode;
			newfirstthr->Wait.Block2n.last = last_node;
			last_node->next = _Chain_Tail(&newfirstthr->Wait.Block2n);
		}
	} else {
		prev_node->next = next_node;
		next_node->previous = prev_node;
	}

	if(!_Watchdog_Is_active(&ret->Timer)) {
		_CPU_ISR_Restore(level);
		_Thread_Unblock(ret);
	} else {
		_Watchdog_Deactivate(&ret->Timer);
		_CPU_ISR_Restore(level);
		_Watchdog_Remove_ticks(&ret->Timer);
		_Thread_Unblock(ret);
	}
	return ret;
}

void _Thread_queue_Initialize(Thread_queue_Control *queue,u32 mode,u32 state,u32 timeout_state)
{
	u32 index;

	queue->state = state;
	queue->discipline = mode;
	queue->timeout_status = timeout_state;
	queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Initialize(%p,%08x,%d,%d)\n",queue,state,timeout_state,mode);
#endif
	switch(mode) {
		case LWP_THREADQ_MODEFIFO:
			_Chain_Initialize_empty(&queue->Queues.Fifo);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			for(index=0;index<LWP_THREADQ_NUM_PRIOHEADERS;index++)
				_Chain_Initialize_empty(&queue->Queues.Priority[index]);
			break;
	}
}

Thread_Control* _Thread_queue_First(Thread_queue_Control *queue)
{
	Thread_Control *ret;

	switch(queue->discipline) {
		case LWP_THREADQ_MODEFIFO:
			ret = _Thread_queue_First_fifo(queue);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			ret = _Thread_queue_First_priority(queue);
			break;
		default:
			ret = NULL;
			break;
	}

	return ret;
}

void _Thread_queue_Enqueue(Thread_queue_Control *queue,u64 timeout)
{
	Thread_Control *thethread;

	thethread = _thr_executing;
	_Thread_Set_state(thethread,queue->state);
	
	if(timeout) {
		_Watchdog_Initialize(&thethread->Timer,_Thread_queue_Timeout,thethread->Object.id,thethread);
		_Watchdog_Insert_ticks(&thethread->Timer,timeout);
	}
	
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue(%p,%p,%d)\n",queue,thethread,queue->mode);
#endif
	switch(queue->discipline) {
		case LWP_THREADQ_MODEFIFO:
			_Thread_queue_Enqueue_fifo(queue,thethread,timeout);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			_Thread_queue_Enqueue_priority(queue,thethread,timeout);
			break;
	}
}

Thread_Control* _Thread_queue_Dequeue(Thread_queue_Control *queue)
{
	Thread_Control *ret = NULL;

#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Dequeue(%p,%p,%d,%d)\n",queue,_thr_executing,queue->mode,queue->sync_state);
#endif
	switch(queue->discipline) {
		case LWP_THREADQ_MODEFIFO:
			ret = _Thread_queue_Dequeue_fifo(queue);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			ret = _Thread_queue_Dequeue_priority(queue);
			break;
		default:
			ret = NULL;
			break;
	}
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Dequeue(%p,%p,%d,%d)\n",queue,ret,queue->mode,queue->sync_state);
#endif
	return ret;
}

void _Thread_queue_Flush(Thread_queue_Control *queue,u32 status)
{
	Thread_Control *thethread;
	while((thethread=_Thread_queue_Dequeue(queue))) {
		thethread->Wait.return_code = status;
	}
}

void _Thread_queue_Extract(Thread_queue_Control *queue,Thread_Control *thethread)
{
	switch(queue->discipline) {
		case LWP_THREADQ_MODEFIFO:
			_Thread_queue_Extract_fifo(queue,thethread);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			_Thread_queue_Extract_priority(queue,thethread);
			break;
	}

}

void _Thread_queue_Extract_fifo(Thread_queue_Control *queue,Thread_Control *thethread)
{
	u32 level;

	_CPU_ISR_Disable(level);
	if(!_States_Is_waiting_on_thread_queue(thethread->current_state)) {
		_CPU_ISR_Restore(level);
		return;
	}
	
	_Chain_Extract_unprotected(&thethread->Object.Node);
	if(!_Watchdog_Is_active(&thethread->Timer)) {
		_CPU_ISR_Restore(level);
	} else {
		_Watchdog_Deactivate(&thethread->Timer);
		_CPU_ISR_Restore(level);
		_Watchdog_Remove_ticks(&thethread->Timer);
	}
	_Thread_Unblock(thethread);
}

void _Thread_queue_Extract_priority(Thread_queue_Control *queue,Thread_Control *thethread)
{
	u32 level;
	Thread_Control *first;
	Chain_Node *curr,*next,*prev,*new_first,*new_sec,*last;

	curr = (Chain_Node*)thethread;

	_CPU_ISR_Disable(level);
	if(_States_Is_waiting_on_thread_queue(thethread->current_state)) {
		next = curr->next;
		prev = curr->previous;
		
		if(!_Chain_Is_empty(&thethread->Wait.Block2n)) {
			new_first = thethread->Wait.Block2n.first;
			first = (Thread_Control*)new_first;
			last = thethread->Wait.Block2n.last;
			new_sec = new_first->next;

			prev->next = new_first;
			next->previous = new_first;
			new_first->next = next;
			new_first->previous = prev;

			if(!_Chain_Has_only_one_node(&thethread->Wait.Block2n)) {
				new_sec->previous = _Chain_Head(&first->Wait.Block2n);
				first->Wait.Block2n.first = new_sec;
				first->Wait.Block2n.last = last;
				last->next = _Chain_Tail(&first->Wait.Block2n);
			}
		} else {
			prev->next = next;
			next->previous = prev;
		}
		if(!_Watchdog_Is_active(&thethread->Timer)) {
			_CPU_ISR_Restore(level);
			_Thread_Unblock(thethread);
		} else {
			_Watchdog_Deactivate(&thethread->Timer);
			_CPU_ISR_Restore(level);
			_Watchdog_Remove_ticks(&thethread->Timer);
			_Thread_Unblock(thethread);
		}
	} else
		_CPU_ISR_Restore(level);
}

u32 _Thread_queue_Extract_with_proxy(Thread_Control *thethread)
{
	u32 state;

	state = thethread->current_state;
	if(_States_Is_waiting_on_thread_queue(state)) {
		_Thread_queue_Extract(thethread->Wait.queue,thethread);
		return TRUE;
	}
	return FALSE;
}
