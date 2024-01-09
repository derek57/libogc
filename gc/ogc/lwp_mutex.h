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

typedef struct _lwpmutexattr {
	u32 mode;
	u32 nest_behavior;
	u8 prioceil,onlyownerrelease;
} lwp_mutex_attr;

typedef struct _lwpmutex {
	lwp_thrqueue wait_queue;
	lwp_mutex_attr atrrs;
	u32 lock,nest_cnt,blocked_cnt;
	lwp_cntrl *holder;
} lwp_mutex;

void _CORE_mutex_Initialize(lwp_mutex *mutex,lwp_mutex_attr *attrs,u32 init_lock);
u32 _CORE_mutex_Surrender(lwp_mutex *mutex);
void _CORE_mutex_Seize_interrupt_blocking(lwp_mutex *mutex,u64 timeout);
void _CORE_mutex_Flush(lwp_mutex *mutex,u32 status);

static __inline__ u32 _CORE_mutex_Seize_interrupt_trylock(lwp_mutex *mutex,u32 *isr_level);

#define _CORE_mutex_Seize(_mutex_t,_id,_wait,_timeout,_level) \
	do { \
		if(_CORE_mutex_Seize_interrupt_trylock(_mutex_t,&_level)) { \
			if(!_wait) {	\
				_CPU_ISR_Restore(_level); \
				_thr_executing->wait.ret_code = LWP_MUTEX_UNSATISFIED_NOWAIT; \
			} else { \
				_Thread_queue_Enter_critical_section(&(_mutex_t)->wait_queue); \
				_thr_executing->wait.queue = &(_mutex_t)->wait_queue; \
				_thr_executing->wait.id = _id; \
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
