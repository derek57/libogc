#ifndef __LWP_MUTEX_H__
#define __LWP_MUTEX_H__

#include <gctypes.h>
#include <lwp_threadq.h>

#define LWP_MUTEX_LOCKED				0
#define LWP_MUTEX_UNLOCKED				1

#define LWP_MUTEX_NEST_ACQUIRE			0
#define LWP_MUTEX_NEST_ERROR			1
#define LWP_MUTEX_NEST_BLOCK			2

#define LWP_MUTEX_FIFO					0
#define LWP_MUTEX_PRIORITY				1
#define LWP_MUTEX_INHERITPRIO			2
#define LWP_MUTEX_PRIORITYCEIL			3

#define LWP_MUTEX_SUCCESSFUL			0
#define LWP_MUTEX_UNSATISFIED_NOWAIT	1
#define LWP_MUTEX_NEST_NOTALLOWED		2
#define LWP_MUTEX_NOTOWNER				3
#define LWP_MUTEX_DELETED				4	
#define LWP_MUTEX_TIMEOUT				5
#define LWP_MUTEX_CEILINGVIOL			6

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	u32 discipline;
	u32 lock_nesting_behavior;
	u8 priority_ceiling,only_owner_release;
} CORE_mutex_Attributes;

typedef struct {
	Thread_queue_Control Wait_queue;
	CORE_mutex_Attributes Attributes;
	u32 lock,nest_count,blocked_count;
	Thread_Control *holder;
} CORE_mutex_Control;

void _CORE_mutex_Initialize(CORE_mutex_Control *mutex,CORE_mutex_Attributes *attrs,u32 init_lock);
u32 _CORE_mutex_Surrender(CORE_mutex_Control *mutex);
void _CORE_mutex_Seize_interrupt_blocking(CORE_mutex_Control *mutex,u64 timeout);
void _CORE_mutex_Flush(CORE_mutex_Control *mutex,u32 status);

static __inline__ u32 _CORE_mutex_Seize_interrupt_trylock(CORE_mutex_Control *mutex,u32 *isr_level);

#define _CORE_mutex_Seize(_mutex_t,_id,_wait,_timeout,_level) \
	do { \
		if(_CORE_mutex_Seize_interrupt_trylock(_mutex_t,&_level)) { \
			if(!_wait) {	\
				_CPU_ISR_Restore(_level); \
				_thr_executing->Wait.return_code = LWP_MUTEX_UNSATISFIED_NOWAIT; \
			} else { \
				_Thread_queue_Enter_critical_section(&(_mutex_t)->Wait_queue); \
				_thr_executing->Wait.queue = &(_mutex_t)->Wait_queue; \
				_thr_executing->Wait.id = _id; \
				_Thread_Disable_dispatch(); \
				_CPU_ISR_Restore(_level); \
				_CORE_mutex_Seize_interrupt_blocking(_mutex_t,(u64)_timeout); \
			} \
		} \
	} while(0)

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_mutex.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
