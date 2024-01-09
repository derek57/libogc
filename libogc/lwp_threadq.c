#include <stdio.h>
#include <lwp_watchdog.h>
#include "asm.h"
#include "lwp_threadq.h"

//#define _LWPTHRQ_DEBUG

static void _Thread_queue_Timeout(void *usr_data)
{
	lwp_cntrl *thethread;
	lwp_thrqueue *thequeue;
	
	_Thread_Disable_dispatch();
	thethread = (lwp_cntrl*)usr_data;
	thequeue = thethread->wait.queue;
	if(thequeue->sync_state!=LWP_THREADQ_SYNCHRONIZED && _Thread_Is_executing(thethread)) {
		if(thequeue->sync_state!=LWP_THREADQ_SATISFIED) thequeue->sync_state = LWP_THREADQ_TIMEOUT;
	} else {
		thethread->wait.ret_code = thethread->wait.queue->timeout_state;
		_Thread_queue_Extract(thethread->wait.queue,thethread);
	}
	_Thread_Unnest_dispatch();
}

lwp_cntrl* _Thread_queue_First_fifo(lwp_thrqueue *queue)
{
	if(!_Chain_Is_empty(&queue->queues.fifo))
		return (lwp_cntrl*)queue->queues.fifo.first;

	return NULL;
}

lwp_cntrl* _Thread_queue_First_priority(lwp_thrqueue *queue)
{
	u32 index;

	for(index=0;index<LWP_THREADQ_NUM_PRIOHEADERS;index++) {
		if(!_Chain_Is_empty(&queue->queues.priority[index]))
			return (lwp_cntrl*)queue->queues.priority[index].first;
	}
	return NULL;
}

void _Thread_queue_Enqueue_fifo(lwp_thrqueue *queue,lwp_cntrl *thethread,u64 timeout)
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
			_Chain_Append_unprotected(&queue->queues.fifo,&thethread->object.node);
			_CPU_ISR_Restore(level);
			return;
		case LWP_THREADQ_TIMEOUT:
			thethread->wait.ret_code = thethread->wait.queue->timeout_state;
			_CPU_ISR_Restore(level);
			break;
		case LWP_THREADQ_SATISFIED:
			if(_Watchdog_Is_active(&thethread->timer)) {
				_Watchdog_Deactivate(&thethread->timer);
				_CPU_ISR_Restore(level);
				_Watchdog_Remove_ticks(&thethread->timer);
			} else
				_CPU_ISR_Restore(level);

			break;
	}
	_Thread_Unblock(thethread);
}

lwp_cntrl* _Thread_queue_Dequeue_fifo(lwp_thrqueue *queue)
{
	u32 level;
	lwp_cntrl *ret;

	_CPU_ISR_Disable(level);
	if(!_Chain_Is_empty(&queue->queues.fifo)) {
		ret = (lwp_cntrl*)_Chain_Get_first_unprotected(&queue->queues.fifo);
		if(!_Watchdog_Is_active(&ret->timer)) {
			_CPU_ISR_Restore(level);
			_Thread_Unblock(ret);
		} else {
			_Watchdog_Deactivate(&ret->timer);
			_CPU_ISR_Restore(level);
			_Watchdog_Remove_ticks(&ret->timer);
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

void _Thread_queue_Enqueue_priority(lwp_thrqueue *queue,lwp_cntrl *thethread,u64 timeout)
{
	u32 level,search_prio,header_idx,prio,block_state,sync_state;
	lwp_cntrl *search_thread;
	lwp_queue *header;
	lwp_node *cur_node,*next_node,*prev_node,*search_node;

	_Chain_Initialize_empty(&thethread->wait.block2n);
	
	prio = thethread->cur_prio;
	header_idx = prio/LWP_THREADQ_PRIOPERHEADER;
	header = &queue->queues.priority[header_idx];
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
	search_thread = (lwp_cntrl*)header->first;
	while(!_Chain_Is_tail(header,(lwp_node*)search_thread)) {
		search_prio = search_thread->cur_prio;
		if(prio<=search_prio) break;
		_CPU_ISR_Flash(level);

		if(!_States_Are_set(search_thread->cur_state,block_state)) {
			_CPU_ISR_Restore(level);
			goto forward_search;
		}
		search_thread = (lwp_cntrl*)search_thread->object.node.next;
	}
	if(queue->sync_state!=LWP_THREADQ_NOTHINGHAPPEND) goto synchronize;
	queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
	if(prio==search_prio) goto equal_prio;

	search_node = (lwp_node*)search_thread;
	prev_node = search_node->prev;
	cur_node = (lwp_node*)thethread;
	
	cur_node->next = search_node;
	cur_node->prev = prev_node;
	prev_node->next = cur_node;
	search_node->prev = cur_node;
	_CPU_ISR_Restore(level);
	return;

reverse_search:
	search_prio = LWP_PRIO_MAX + 1;
	_CPU_ISR_Disable(level);
	search_thread = (lwp_cntrl*)header->last;
	while(!_Chain_Is_head(header,(lwp_node*)search_thread)) {
		search_prio = search_thread->cur_prio;
		if(prio>=search_prio) break;
		_CPU_ISR_Flash(level);

		if(!_States_Are_set(search_thread->cur_state,block_state)) {
			_CPU_ISR_Restore(level);
			goto reverse_search;
		}
		search_thread = (lwp_cntrl*)search_thread->object.node.prev;
	}
	if(queue->sync_state!=LWP_THREADQ_NOTHINGHAPPEND) goto synchronize;
	queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
	if(prio==search_prio) goto equal_prio;

	search_node = (lwp_node*)search_thread;
	next_node = search_node->next;
	cur_node = (lwp_node*)thethread;
	
	cur_node->next = next_node;
	cur_node->prev = search_node;
	search_node->next = cur_node;
	next_node->prev = cur_node;
	_CPU_ISR_Restore(level);
	return;

equal_prio:
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue_priority(%p,equal_prio)\n",thethread);
#endif
	search_node = _Chain_Tail(&search_thread->wait.block2n);
	prev_node = search_node->prev;
	cur_node = (lwp_node*)thethread;

	cur_node->next = search_node;
	cur_node->prev = prev_node;
	prev_node->next = cur_node;
	search_node->prev = cur_node;
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
			thethread->wait.ret_code = thethread->wait.queue->timeout_state;
			_CPU_ISR_Restore(level);
			break;
		case LWP_THREADQ_SATISFIED:
			if(_Watchdog_Is_active(&thethread->timer)) {
				_Watchdog_Deactivate(&thethread->timer);
				_CPU_ISR_Restore(level);
				_Watchdog_Remove_ticks(&thethread->timer);
			} else
				_CPU_ISR_Restore(level);
			break;
	}
	_Thread_Unblock(thethread);
}

lwp_cntrl* _Thread_queue_Dequeue_priority(lwp_thrqueue *queue)
{
	u32 level,idx;
	lwp_cntrl *newfirstthr,*ret = NULL;
	lwp_node *newfirstnode,*newsecnode,*last_node,*next_node,*prev_node;

	_CPU_ISR_Disable(level);
	for(idx=0;idx<LWP_THREADQ_NUM_PRIOHEADERS;idx++) {
		if(!_Chain_Is_empty(&queue->queues.priority[idx])) {
			ret	 = (lwp_cntrl*)queue->queues.priority[idx].first;
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
	newfirstnode = ret->wait.block2n.first;
	newfirstthr = (lwp_cntrl*)newfirstnode;
	next_node = ret->object.node.next;
	prev_node = ret->object.node.prev;
	if(!_Chain_Is_empty(&ret->wait.block2n)) {
		last_node = ret->wait.block2n.last;
		newsecnode = newfirstnode->next;
		prev_node->next = newfirstnode;
		next_node->prev = newfirstnode;
		newfirstnode->next = next_node;
		newfirstnode->prev = prev_node;
		
		if(!_Chain_Has_only_one_node(&ret->wait.block2n)) {
			newsecnode->prev = _Chain_Head(&newfirstthr->wait.block2n);
			newfirstthr->wait.block2n.first = newsecnode;
			newfirstthr->wait.block2n.last = last_node;
			last_node->next = _Chain_Tail(&newfirstthr->wait.block2n);
		}
	} else {
		prev_node->next = next_node;
		next_node->prev = prev_node;
	}

	if(!_Watchdog_Is_active(&ret->timer)) {
		_CPU_ISR_Restore(level);
		_Thread_Unblock(ret);
	} else {
		_Watchdog_Deactivate(&ret->timer);
		_CPU_ISR_Restore(level);
		_Watchdog_Remove_ticks(&ret->timer);
		_Thread_Unblock(ret);
	}
	return ret;
}

void _Thread_queue_Initialize(lwp_thrqueue *queue,u32 mode,u32 state,u32 timeout_state)
{
	u32 index;

	queue->state = state;
	queue->mode = mode;
	queue->timeout_state = timeout_state;
	queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Initialize(%p,%08x,%d,%d)\n",queue,state,timeout_state,mode);
#endif
	switch(mode) {
		case LWP_THREADQ_MODEFIFO:
			_Chain_Initialize_empty(&queue->queues.fifo);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			for(index=0;index<LWP_THREADQ_NUM_PRIOHEADERS;index++)
				_Chain_Initialize_empty(&queue->queues.priority[index]);
			break;
	}
}

lwp_cntrl* _Thread_queue_First(lwp_thrqueue *queue)
{
	lwp_cntrl *ret;

	switch(queue->mode) {
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

void _Thread_queue_Enqueue(lwp_thrqueue *queue,u64 timeout)
{
	lwp_cntrl *thethread;

	thethread = _thr_executing;
	_Thread_Set_state(thethread,queue->state);
	
	if(timeout) {
		_Watchdog_Initialize(&thethread->timer,_Thread_queue_Timeout,thethread->object.id,thethread);
		_Watchdog_Insert_ticks(&thethread->timer,timeout);
	}
	
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue(%p,%p,%d)\n",queue,thethread,queue->mode);
#endif
	switch(queue->mode) {
		case LWP_THREADQ_MODEFIFO:
			_Thread_queue_Enqueue_fifo(queue,thethread,timeout);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			_Thread_queue_Enqueue_priority(queue,thethread,timeout);
			break;
	}
}

lwp_cntrl* _Thread_queue_Dequeue(lwp_thrqueue *queue)
{
	lwp_cntrl *ret = NULL;

#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Dequeue(%p,%p,%d,%d)\n",queue,_thr_executing,queue->mode,queue->sync_state);
#endif
	switch(queue->mode) {
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

void _Thread_queue_Flush(lwp_thrqueue *queue,u32 status)
{
	lwp_cntrl *thethread;
	while((thethread=_Thread_queue_Dequeue(queue))) {
		thethread->wait.ret_code = status;
	}
}

void _Thread_queue_Extract(lwp_thrqueue *queue,lwp_cntrl *thethread)
{
	switch(queue->mode) {
		case LWP_THREADQ_MODEFIFO:
			_Thread_queue_Extract_fifo(queue,thethread);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			_Thread_queue_Extract_priority(queue,thethread);
			break;
	}

}

void _Thread_queue_Extract_fifo(lwp_thrqueue *queue,lwp_cntrl *thethread)
{
	u32 level;

	_CPU_ISR_Disable(level);
	if(!_States_Is_waiting_on_thread_queue(thethread->cur_state)) {
		_CPU_ISR_Restore(level);
		return;
	}
	
	_Chain_Extract_unprotected(&thethread->object.node);
	if(!_Watchdog_Is_active(&thethread->timer)) {
		_CPU_ISR_Restore(level);
	} else {
		_Watchdog_Deactivate(&thethread->timer);
		_CPU_ISR_Restore(level);
		_Watchdog_Remove_ticks(&thethread->timer);
	}
	_Thread_Unblock(thethread);
}

void _Thread_queue_Extract_priority(lwp_thrqueue *queue,lwp_cntrl *thethread)
{
	u32 level;
	lwp_cntrl *first;
	lwp_node *curr,*next,*prev,*new_first,*new_sec,*last;

	curr = (lwp_node*)thethread;

	_CPU_ISR_Disable(level);
	if(_States_Is_waiting_on_thread_queue(thethread->cur_state)) {
		next = curr->next;
		prev = curr->prev;
		
		if(!_Chain_Is_empty(&thethread->wait.block2n)) {
			new_first = thethread->wait.block2n.first;
			first = (lwp_cntrl*)new_first;
			last = thethread->wait.block2n.last;
			new_sec = new_first->next;

			prev->next = new_first;
			next->prev = new_first;
			new_first->next = next;
			new_first->prev = prev;

			if(!_Chain_Has_only_one_node(&thethread->wait.block2n)) {
				new_sec->prev = _Chain_Head(&first->wait.block2n);
				first->wait.block2n.first = new_sec;
				first->wait.block2n.last = last;
				last->next = _Chain_Tail(&first->wait.block2n);
			}
		} else {
			prev->next = next;
			next->prev = prev;
		}
		if(!_Watchdog_Is_active(&thethread->timer)) {
			_CPU_ISR_Restore(level);
			_Thread_Unblock(thethread);
		} else {
			_Watchdog_Deactivate(&thethread->timer);
			_CPU_ISR_Restore(level);
			_Watchdog_Remove_ticks(&thethread->timer);
			_Thread_Unblock(thethread);
		}
	} else
		_CPU_ISR_Restore(level);
}

u32 _Thread_queue_Extract_with_proxy(lwp_cntrl *thethread)
{
	u32 state;

	state = thethread->cur_state;
	if(_States_Is_waiting_on_thread_queue(state)) {
		_Thread_queue_Extract(thethread->wait.queue,thethread);
		return TRUE;
	}
	return FALSE;
}
