#ifndef __LWP_MUTEX_H__
#define __LWP_MUTEX_H__

#include <gctypes.h>
#include <lwp_threadq.h>

#define CORE_MUTEX_UNLOCKED 1
#define CORE_MUTEX_LOCKED   0

typedef enum {
  CORE_MUTEX_NESTING_ACQUIRES,
  CORE_MUTEX_NESTING_IS_ERROR,
  CORE_MUTEX_NESTING_BLOCKS
}  CORE_mutex_Nesting_behaviors;

typedef enum {
  CORE_MUTEX_DISCIPLINES_FIFO,
  CORE_MUTEX_DISCIPLINES_PRIORITY,
  CORE_MUTEX_DISCIPLINES_PRIORITY_INHERIT,
  CORE_MUTEX_DISCIPLINES_PRIORITY_CEILING
}   CORE_mutex_Disciplines;

typedef enum {
  CORE_MUTEX_STATUS_SUCCESSFUL,
  CORE_MUTEX_STATUS_UNSATISFIED_NOWAIT,
  CORE_MUTEX_STATUS_NESTING_NOT_ALLOWED,
  CORE_MUTEX_STATUS_NOT_OWNER_OF_RESOURCE,
  CORE_MUTEX_WAS_DELETED,
  CORE_MUTEX_TIMEOUT,
  CORE_MUTEX_STATUS_CEILING_VIOLATED
}   CORE_mutex_Status;

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
				_Thread_Executing->Wait.return_code = CORE_MUTEX_STATUS_UNSATISFIED_NOWAIT; \
			} else { \
				_Thread_queue_Enter_critical_section(&(_mutex_t)->Wait_queue); \
				_Thread_Executing->Wait.queue = &(_mutex_t)->Wait_queue; \
				_Thread_Executing->Wait.id = _id; \
				_Thread_Disable_dispatch(); \
				_CPU_ISR_Restore(_level); \
				_CORE_mutex_Seize_interrupt_blocking(_mutex_t,(u64)_timeout); \
			} \
		} \
	} while(0)

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_mutex.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
