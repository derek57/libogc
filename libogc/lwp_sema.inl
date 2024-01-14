#ifndef __LWP_SEMA_INL__
#define __LWP_SEMA_INL__

RTEMS_INLINE_ROUTINE u32 _CORE_semaphore_Is_priority(CORE_semaphore_Attributes *the_attribute)
{
	return (the_attribute->discipline==CORE_SEMAPHORE_DISCIPLINES_PRIORITY);
}

RTEMS_INLINE_ROUTINE void _CORE_semaphore_Seize_isr_disable(CORE_semaphore_Control *the_semaphore,u32 id,u32 wait,u32 *level_p)
{
	Thread_Control *executing;
	u32 level = *level_p;

	executing = _Thread_Executing;
	executing->Wait.return_code = CORE_SEMAPHORE_STATUS_SUCCESSFUL;
	if(the_semaphore->count!=0) {
		--the_semaphore->count;
		_ISR_Enable(level);
		return;
	}

	if(!wait) {
		_ISR_Enable(level);
		executing->Wait.return_code = CORE_SEMAPHORE_STATUS_UNSATISFIED_NOWAIT;
		return;
	}

	_Thread_Disable_dispatch();
	_Thread_queue_Enter_critical_section(&the_semaphore->Wait_queue);
	executing->Wait.queue = &the_semaphore->Wait_queue;
	executing->Wait.id = id;
	_ISR_Enable(level);

	_Thread_queue_Enqueue(&the_semaphore->Wait_queue,0);
	_Thread_Enable_dispatch();
}

#endif
