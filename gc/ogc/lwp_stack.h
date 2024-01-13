#ifndef __LWP_STACK_H__
#define __LWP_STACK_H__

#include <gctypes.h>
#include <lwp_threads.h>

#define CPU_STACK_ALIGNMENT				8
#define CPU_MINIMUM_STACK_SIZE			1024*8
#define CPU_MINIMUM_STACK_FRAME_SIZE	16
#define CPU_MODES_INTERRUPT_MASK		0x00000001 /* interrupt level in mode */

#ifdef __cplusplus
extern "C" {
#endif

u32 _Thread_Stack_Allocate(Thread_Control *,u32);
void _Thread_Stack_Free(Thread_Control *);

#ifdef __RTEMS_APPLICATION__
#include <libogc/lwp_stack.inl>
#endif
	
#ifdef __cplusplus
	}
#endif

#endif
