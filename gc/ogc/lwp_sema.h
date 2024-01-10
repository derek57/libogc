#ifndef __LWP_SEMA_H__
#define __LWP_SEMA_H__

#include <gctypes.h>
#include <lwp_threadq.h>

#define LWP_SEMA_MODEFIFO				0
#define LWP_SEMA_MODEPRIORITY			1

#define LWP_SEMA_SUCCESSFUL				0
#define LWP_SEMA_UNSATISFIED_NOWAIT		1
#define LWP_SEMA_DELETED				2
#define LWP_SEMA_TIMEOUT				3
#define LWP_SEMA_MAXCNT_EXCEEDED		4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	u32 maximum_count;
	u32 discipline;
} CORE_semaphore_Attributes;

typedef struct {
	Thread_queue_Control Wait_queue;
	CORE_semaphore_Attributes	Attributes;
	u32 count;
} CORE_semaphore_Control;

void CORE_semaphore_Initialize(CORE_semaphore_Control *sema,CORE_semaphore_Attributes *attrs,u32 init_count);
u32 _CORE_semaphore_Surrender(CORE_semaphore_Control *sema,u32 id);
u32 _CORE_semaphore_Seize(CORE_semaphore_Control *sema,u32 id,u32 wait,u64 timeout);
void _CORE_semaphore_Flush(CORE_semaphore_Control *sema,u32 status);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_sema.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
