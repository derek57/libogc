#include <stdio.h>
#include <lwp_watchdog.h>
#include "asm.h"
#include "lwp_threadq.h"

//#define _LWPTHRQ_DEBUG

static void _Thread_queue_Timeout(void *usr_data)
{
	Thread_Control *the_thread;
	Thread_queue_Control *the_thread_queue;
	
	_Thread_Disable_dispatch();
	the_thread = (Thread_Control*)usr_data;
	the_thread_queue = the_thread->Wait.queue;
	if(the_thread_queue->sync_state!=LWP_THREADQ_SYNCHRONIZED && _Thread_Is_executing(the_thread)) {
		if(the_thread_queue->sync_state!=LWP_THREADQ_SATISFIED) the_thread_queue->sync_state = LWP_THREADQ_TIMEOUT;
	} else {
		the_thread->Wait.return_code = the_thread->Wait.queue->timeout_status;
		_Thread_queue_Extract(the_thread->Wait.queue,the_thread);
	}
	_Thread_Unnest_dispatch();
}

Thread_Control* _Thread_queue_First_fifo(Thread_queue_Control *the_thread_queue)
{
	if(!_Chain_Is_empty(&the_thread_queue->Queues.Fifo))
		return (Thread_Control*)the_thread_queue->Queues.Fifo.first;

	return NULL;
}

Thread_Control* _Thread_queue_First_priority(Thread_queue_Control *the_thread_queue)
{
	u32 index;

	for(index=0;index<LWP_THREADQ_NUM_PRIOHEADERS;index++) {
		if(!_Chain_Is_empty(&the_thread_queue->Queues.Priority[index]))
			return (Thread_Control*)the_thread_queue->Queues.Priority[index].first;
	}
	return NULL;
}

void _Thread_queue_Enqueue_fifo(Thread_queue_Control *the_thread_queue,Thread_Control *the_thread,u64 timeout)
{
	u32 level,sync_state;

	_CPU_ISR_Disable(level);
	
	sync_state = the_thread_queue->sync_state;
	the_thread_queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue_fifo(%p,%d)\n",the_thread,sync_state);
#endif
	switch(sync_state) {
		case LWP_THREADQ_SYNCHRONIZED:
			break;
		case LWP_THREADQ_NOTHINGHAPPEND:
			_Chain_Append_unprotected(&the_thread_queue->Queues.Fifo,&the_thread->Object.Node);
			_CPU_ISR_Restore(level);
			return;
		case LWP_THREADQ_TIMEOUT:
			the_thread->Wait.return_code = the_thread->Wait.queue->timeout_status;
			_CPU_ISR_Restore(level);
			break;
		case LWP_THREADQ_SATISFIED:
			if(_Watchdog_Is_active(&the_thread->Timer)) {
				_Watchdog_Deactivate(&the_thread->Timer);
				_CPU_ISR_Restore(level);
				_Watchdog_Remove_ticks(&the_thread->Timer);
			} else
				_CPU_ISR_Restore(level);

			break;
	}
	_Thread_Unblock(the_thread);
}

Thread_Control* _Thread_queue_Dequeue_fifo(Thread_queue_Control *the_thread_queue)
{
	u32 level;
	Thread_Control *the_thread;

	_CPU_ISR_Disable(level);
	if(!_Chain_Is_empty(&the_thread_queue->Queues.Fifo)) {
		the_thread = (Thread_Control*)_Chain_Get_first_unprotected(&the_thread_queue->Queues.Fifo);
		if(!_Watchdog_Is_active(&the_thread->Timer)) {
			_CPU_ISR_Restore(level);
			_Thread_Unblock(the_thread);
		} else {
			_Watchdog_Deactivate(&the_thread->Timer);
			_CPU_ISR_Restore(level);
			_Watchdog_Remove_ticks(&the_thread->Timer);
			_Thread_Unblock(the_thread);
		}
		return the_thread;
	}
	
	switch(the_thread_queue->sync_state) {
		case LWP_THREADQ_SYNCHRONIZED:
		case LWP_THREADQ_SATISFIED:
			_CPU_ISR_Restore(level);
			return NULL;
		case LWP_THREADQ_NOTHINGHAPPEND:
		case LWP_THREADQ_TIMEOUT:
			the_thread_queue->sync_state = LWP_THREADQ_SATISFIED;
			_CPU_ISR_Restore(level);
			return _Thread_Executing;
	}
	return NULL;
}

void _Thread_queue_Enqueue_priority(Thread_queue_Control *the_thread_queue,Thread_Control *the_thread,u64 timeout)
{
	u32 level,search_priority,header_index,priority,block_state,sync_state;
	Thread_Control *search_thread;
	Chain_Control *header;
	Chain_Node *the_node,*next_node,*previous_node,*search_node;

	_Chain_Initialize_empty(&the_thread->Wait.Block2n);
	
	priority = the_thread->current_priority;
	header_index = priority/LWP_THREADQ_PRIOPERHEADER;
	header = &the_thread_queue->Queues.Priority[header_index];
	block_state = the_thread_queue->state;

	if(priority&LWP_THREADQ_REVERSESEARCHMASK) {
#ifdef _LWPTHRQ_DEBUG
		printf("_Thread_queue_Enqueue_priority(%p,reverse_search)\n",the_thread);
#endif
		goto reverse_search;
	}

#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue_priority(%p,forward_search)\n",the_thread);
#endif
forward_search:
	search_priority = LWP_PRIO_MIN - 1;
	_CPU_ISR_Disable(level);
	search_thread = (Thread_Control*)header->first;
	while(!_Chain_Is_tail(header,(Chain_Node*)search_thread)) {
		search_priority = search_thread->current_priority;
		if(priority<=search_priority) break;
		_CPU_ISR_Flash(level);

		if(!_States_Are_set(search_thread->current_state,block_state)) {
			_CPU_ISR_Restore(level);
			goto forward_search;
		}
		search_thread = (Thread_Control*)search_thread->Object.Node.next;
	}
	if(the_thread_queue->sync_state!=LWP_THREADQ_NOTHINGHAPPEND) goto synchronize;
	the_thread_queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
	if(priority==search_priority) goto equal_prio;

	search_node = (Chain_Node*)search_thread;
	previous_node = search_node->previous;
	the_node = (Chain_Node*)the_thread;
	
	the_node->next = search_node;
	the_node->previous = previous_node;
	previous_node->next = the_node;
	search_node->previous = the_node;
	_CPU_ISR_Restore(level);
	return;

reverse_search:
	search_priority = LWP_PRIO_MAX + 1;
	_CPU_ISR_Disable(level);
	search_thread = (Thread_Control*)header->last;
	while(!_Chain_Is_head(header,(Chain_Node*)search_thread)) {
		search_priority = search_thread->current_priority;
		if(priority>=search_priority) break;
		_CPU_ISR_Flash(level);

		if(!_States_Are_set(search_thread->current_state,block_state)) {
			_CPU_ISR_Restore(level);
			goto reverse_search;
		}
		search_thread = (Thread_Control*)search_thread->Object.Node.previous;
	}
	if(the_thread_queue->sync_state!=LWP_THREADQ_NOTHINGHAPPEND) goto synchronize;
	the_thread_queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
	if(priority==search_priority) goto equal_prio;

	search_node = (Chain_Node*)search_thread;
	next_node = search_node->next;
	the_node = (Chain_Node*)the_thread;
	
	the_node->next = next_node;
	the_node->previous = search_node;
	search_node->next = the_node;
	next_node->previous = the_node;
	_CPU_ISR_Restore(level);
	return;

equal_prio:
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue_priority(%p,equal_prio)\n",the_thread);
#endif
	search_node = _Chain_Tail(&search_thread->Wait.Block2n);
	previous_node = search_node->previous;
	the_node = (Chain_Node*)the_thread;

	the_node->next = search_node;
	the_node->previous = previous_node;
	previous_node->next = the_node;
	search_node->previous = the_node;
	_CPU_ISR_Restore(level);
	return;

synchronize:
	sync_state = the_thread_queue->sync_state;
	the_thread_queue->sync_state = LWP_THREADQ_SYNCHRONIZED;

#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue_priority(%p,sync_state = %d)\n",the_thread,sync_state);
#endif
	switch(sync_state) {
		case LWP_THREADQ_SYNCHRONIZED:
			break;
		case LWP_THREADQ_NOTHINGHAPPEND:
			break;
		case LWP_THREADQ_TIMEOUT:
			the_thread->Wait.return_code = the_thread->Wait.queue->timeout_status;
			_CPU_ISR_Restore(level);
			break;
		case LWP_THREADQ_SATISFIED:
			if(_Watchdog_Is_active(&the_thread->Timer)) {
				_Watchdog_Deactivate(&the_thread->Timer);
				_CPU_ISR_Restore(level);
				_Watchdog_Remove_ticks(&the_thread->Timer);
			} else
				_CPU_ISR_Restore(level);
			break;
	}
	_Thread_Unblock(the_thread);
}

Thread_Control* _Thread_queue_Dequeue_priority(Thread_queue_Control *the_thread_queue)
{
	u32 level,index;
	Thread_Control *new_first_thread,*the_thread = NULL;
	Chain_Node *new_first_node,*new_second_node,*last_node,*next_node,*previous_node;

	_CPU_ISR_Disable(level);
	for(index=0;index<LWP_THREADQ_NUM_PRIOHEADERS;index++) {
		if(!_Chain_Is_empty(&the_thread_queue->Queues.Priority[index])) {
			the_thread	 = (Thread_Control*)the_thread_queue->Queues.Priority[index].first;
			goto dequeue;
		}
	}

#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Dequeue_priority(%p,sync_state = %d)\n",the_thread,the_thread_queue->sync_state);
#endif
	switch(the_thread_queue->sync_state) {
		case LWP_THREADQ_SYNCHRONIZED:
		case LWP_THREADQ_SATISFIED:
			_CPU_ISR_Restore(level);
			return NULL;
		case LWP_THREADQ_NOTHINGHAPPEND:
		case LWP_THREADQ_TIMEOUT:
			the_thread_queue->sync_state = LWP_THREADQ_SATISFIED;
			_CPU_ISR_Restore(level);
			return _Thread_Executing;
	}

dequeue:
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Dequeue_priority(%p,dequeue)\n",the_thread);
#endif
	new_first_node = the_thread->Wait.Block2n.first;
	new_first_thread = (Thread_Control*)new_first_node;
	next_node = the_thread->Object.Node.next;
	previous_node = the_thread->Object.Node.previous;
	if(!_Chain_Is_empty(&the_thread->Wait.Block2n)) {
		last_node = the_thread->Wait.Block2n.last;
		new_second_node = new_first_node->next;
		previous_node->next = new_first_node;
		next_node->previous = new_first_node;
		new_first_node->next = next_node;
		new_first_node->previous = previous_node;
		
		if(!_Chain_Has_only_one_node(&the_thread->Wait.Block2n)) {
			new_second_node->previous = _Chain_Head(&new_first_thread->Wait.Block2n);
			new_first_thread->Wait.Block2n.first = new_second_node;
			new_first_thread->Wait.Block2n.last = last_node;
			last_node->next = _Chain_Tail(&new_first_thread->Wait.Block2n);
		}
	} else {
		previous_node->next = next_node;
		next_node->previous = previous_node;
	}

	if(!_Watchdog_Is_active(&the_thread->Timer)) {
		_CPU_ISR_Restore(level);
		_Thread_Unblock(the_thread);
	} else {
		_Watchdog_Deactivate(&the_thread->Timer);
		_CPU_ISR_Restore(level);
		_Watchdog_Remove_ticks(&the_thread->Timer);
		_Thread_Unblock(the_thread);
	}
	return the_thread;
}

void _Thread_queue_Initialize(Thread_queue_Control *the_thread_queue,u32 the_discipline,u32 state,u32 timeout_status)
{
	u32 index;

	the_thread_queue->state = state;
	the_thread_queue->discipline = the_discipline;
	the_thread_queue->timeout_status = timeout_status;
	the_thread_queue->sync_state = LWP_THREADQ_SYNCHRONIZED;
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Initialize(%p,%08x,%d,%d)\n",the_thread_queue,state,timeout_status,the_discipline);
#endif
	switch(the_discipline) {
		case LWP_THREADQ_MODEFIFO:
			_Chain_Initialize_empty(&the_thread_queue->Queues.Fifo);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			for(index=0;index<LWP_THREADQ_NUM_PRIOHEADERS;index++)
				_Chain_Initialize_empty(&the_thread_queue->Queues.Priority[index]);
			break;
	}
}

Thread_Control* _Thread_queue_First(Thread_queue_Control *the_thread_queue)
{
	Thread_Control *the_thread;

	switch(the_thread_queue->discipline) {
		case LWP_THREADQ_MODEFIFO:
			the_thread = _Thread_queue_First_fifo(the_thread_queue);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			the_thread = _Thread_queue_First_priority(the_thread_queue);
			break;
		default:
			the_thread = NULL;
			break;
	}

	return the_thread;
}

void _Thread_queue_Enqueue(Thread_queue_Control *the_thread_queue,u64 timeout)
{
	Thread_Control *the_thread;

	the_thread = _Thread_Executing;
	_Thread_Set_state(the_thread,the_thread_queue->state);
	
	if(timeout) {
		_Watchdog_Initialize(&the_thread->Timer,_Thread_queue_Timeout,the_thread->Object.id,the_thread);
		_Watchdog_Insert_ticks(&the_thread->Timer,timeout);
	}
	
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Enqueue(%p,%p,%d)\n",the_thread_queue,the_thread,the_thread_queue->mode);
#endif
	switch(the_thread_queue->discipline) {
		case LWP_THREADQ_MODEFIFO:
			_Thread_queue_Enqueue_fifo(the_thread_queue,the_thread,timeout);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			_Thread_queue_Enqueue_priority(the_thread_queue,the_thread,timeout);
			break;
	}
}

Thread_Control* _Thread_queue_Dequeue(Thread_queue_Control *the_thread_queue)
{
	Thread_Control *the_thread = NULL;

#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Dequeue(%p,%p,%d,%d)\n",the_thread_queue,_Thread_Executing,the_thread_queue->mode,the_thread_queue->sync_state);
#endif
	switch(the_thread_queue->discipline) {
		case LWP_THREADQ_MODEFIFO:
			the_thread = _Thread_queue_Dequeue_fifo(the_thread_queue);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			the_thread = _Thread_queue_Dequeue_priority(the_thread_queue);
			break;
		default:
			the_thread = NULL;
			break;
	}
#ifdef _LWPTHRQ_DEBUG
	printf("_Thread_queue_Dequeue(%p,%p,%d,%d)\n",the_thread_queue,the_thread,the_thread_queue->mode,the_thread_queue->sync_state);
#endif
	return the_thread;
}

void _Thread_queue_Flush(Thread_queue_Control *the_thread_queue,u32 status)
{
	Thread_Control *the_thread;
	while((the_thread=_Thread_queue_Dequeue(the_thread_queue))) {
		the_thread->Wait.return_code = status;
	}
}

void _Thread_queue_Extract(Thread_queue_Control *the_thread_queue,Thread_Control *the_thread)
{
	switch(the_thread_queue->discipline) {
		case LWP_THREADQ_MODEFIFO:
			_Thread_queue_Extract_fifo(the_thread_queue,the_thread);
			break;
		case LWP_THREADQ_MODEPRIORITY:
			_Thread_queue_Extract_priority(the_thread_queue,the_thread);
			break;
	}

}

void _Thread_queue_Extract_fifo(Thread_queue_Control *the_thread_queue,Thread_Control *the_thread)
{
	u32 level;

	_CPU_ISR_Disable(level);
	if(!_States_Is_waiting_on_thread_queue(the_thread->current_state)) {
		_CPU_ISR_Restore(level);
		return;
	}
	
	_Chain_Extract_unprotected(&the_thread->Object.Node);
	if(!_Watchdog_Is_active(&the_thread->Timer)) {
		_CPU_ISR_Restore(level);
	} else {
		_Watchdog_Deactivate(&the_thread->Timer);
		_CPU_ISR_Restore(level);
		_Watchdog_Remove_ticks(&the_thread->Timer);
	}
	_Thread_Unblock(the_thread);
}

void _Thread_queue_Extract_priority(Thread_queue_Control *the_thread_queue,Thread_Control *the_thread)
{
	u32 level;
	Thread_Control *new_first_thread;
	Chain_Node *the_node,*next_node,*previous_node,*new_first_node,*new_second_node,*last_node;

	the_node = (Chain_Node*)the_thread;

	_CPU_ISR_Disable(level);
	if(_States_Is_waiting_on_thread_queue(the_thread->current_state)) {
		next_node = the_node->next;
		previous_node = the_node->previous;
		
		if(!_Chain_Is_empty(&the_thread->Wait.Block2n)) {
			new_first_node = the_thread->Wait.Block2n.first;
			new_first_thread = (Thread_Control*)new_first_node;
			last_node = the_thread->Wait.Block2n.last;
			new_second_node = new_first_node->next;

			previous_node->next = new_first_node;
			next_node->previous = new_first_node;
			new_first_node->next = next_node;
			new_first_node->previous = previous_node;

			if(!_Chain_Has_only_one_node(&the_thread->Wait.Block2n)) {
				new_second_node->previous = _Chain_Head(&new_first_thread->Wait.Block2n);
				new_first_thread->Wait.Block2n.first = new_second_node;
				new_first_thread->Wait.Block2n.last = last_node;
				last_node->next = _Chain_Tail(&new_first_thread->Wait.Block2n);
			}
		} else {
			previous_node->next = next_node;
			next_node->previous = previous_node;
		}
		if(!_Watchdog_Is_active(&the_thread->Timer)) {
			_CPU_ISR_Restore(level);
			_Thread_Unblock(the_thread);
		} else {
			_Watchdog_Deactivate(&the_thread->Timer);
			_CPU_ISR_Restore(level);
			_Watchdog_Remove_ticks(&the_thread->Timer);
			_Thread_Unblock(the_thread);
		}
	} else
		_CPU_ISR_Restore(level);
}

u32 _Thread_queue_Extract_with_proxy(Thread_Control *the_thread)
{
	u32 state;

	state = the_thread->current_state;
	if(_States_Is_waiting_on_thread_queue(state)) {
		_Thread_queue_Extract(the_thread->Wait.queue,the_thread);
		return TRUE;
	}
	return FALSE;
}
