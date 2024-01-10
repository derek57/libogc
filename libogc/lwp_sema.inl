#ifndef __LWP_SEMA_INL__
#define __LWP_SEMA_INL__

static __inline__ u32 _CORE_semaphore_Is_priority(CORE_semaphore_Attributes *attr)
{
	return (attr->mode==LWP_SEMA_MODEPRIORITY);
}

static __inline__ void _CORE_semaphore_Seize_isr_disable(CORE_semaphore_Control *sema,u32 id,u32 wait,u32 *isrlevel)
{
	Thread_Control *exec;
	u32 level = *isrlevel;

	exec = _thr_executing;
	exec->wait.return_code = LWP_SEMA_SUCCESSFUL;
	if(sema->count!=0) {
		--sema->count;
		_CPU_ISR_Restore(level);
		return;
	}

	if(!wait) {
		_CPU_ISR_Restore(level);
		exec->wait.return_code = LWP_SEMA_UNSATISFIED_NOWAIT;
		return;
	}

	_Thread_Disable_dispatch();
	_Thread_queue_Enter_critical_section(&sema->wait_queue);
	exec->wait.queue = &sema->wait_queue;
	exec->wait.id = id;
	_CPU_ISR_Restore(level);

	_Thread_queue_Enqueue(&sema->wait_queue,0);
	_Thread_Enable_dispatch();
}

#endif
