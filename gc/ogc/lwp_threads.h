#ifndef __LWP_THREADS_H__
#define __LWP_THREADS_H__

#include <gctypes.h>
#include <stdlib.h>
#include "lwp_states.h"
#include "lwp_tqdata.h"
#include "lwp_watchdog.h"
#include "lwp_objmgr.h"
#include "context.h"

//#define _LWPTHREADS_DEBUG
#define LWP_TIMESLICE_TIMER_ID			0x00070040

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	LWP_CPU_BUDGET_ALGO_NONE = 0,
	LWP_CPU_BUDGET_ALGO_TIMESLICE	
} lwp_cpu_budget_algorithms;

typedef struct _lwpwaitinfo {
	u32 id;
	u32 cnt;
	void *ret_arg;
	void *ret_arg_1;
	u32 option;
	u32 ret_code;
	lwp_queue block2n;
	lwp_thrqueue *queue;
} lwp_waitinfo;

typedef struct _lwpcntrl {
	lwp_obj object;
	u8  cur_prio,real_prio;
	u32 suspendcnt,res_cnt;
	u32 isr_level;
	u32 cur_state;
	u32 cpu_time_budget;
	lwp_cpu_budget_algorithms budget_algo;
	bool is_preemptible;
	lwp_waitinfo wait;
	prio_cntrl priomap;
	wd_cntrl timer;

	void* (*entry)(void *);
	void *arg;
	void *stack;
	u32 stack_size;
	u8 stack_allocated;
	lwp_queue *ready;
	lwp_thrqueue join_list;
	frame_context context;		//16
	void *libc_reent;
} lwp_cntrl, *lwp_cntrl_t;

extern lwp_cntrl *_thr_main;
extern lwp_cntrl *_thr_idle;
extern lwp_cntrl *_thr_executing;
extern lwp_cntrl *_thr_heir;
extern lwp_cntrl *_thr_allocated_fp;
extern vu32 _context_switch_want;
extern vu32 _thread_dispatch_disable_level;

extern wd_cntrl _lwp_wd_timeslice;
extern void **__lwp_thr_libc_reent;
extern lwp_queue _lwp_thr_ready[];

void _Thread_Dispatch();
void _Thread_Yield_processor();
void __lwp_thread_closeall();
void _Thread_Set_state(lwp_cntrl *,u32);
void _Thread_Clear_state(lwp_cntrl *,u32);
void _Thread_Change_priority(lwp_cntrl *,u32,u32);
void _Thread_Set_priority(lwp_cntrl *,u32);
void _Thread_Set_transient(lwp_cntrl *);
void _Thread_Suspend(lwp_cntrl *);
void _Thread_Resume(lwp_cntrl *,u32);
void _Thread_Load_environment(lwp_cntrl *);
void _Thread_Ready(lwp_cntrl *);
u32 _Thread_Initialize(lwp_cntrl *,void *,u32,u32,u32,bool);
u32 _Thread_Start(lwp_cntrl *,void* (*)(void*),void *);
void pthread_exit(void *);
void _Thread_Close(lwp_cntrl *);
void _Thread_Start_multitasking();
void _Thread_Stop_multitasking(void (*exitfunc)());
lwp_obj* _Thread_Get(lwp_cntrl *);
u32 _Thread_Evaluate_mode();

u32 _ISR_Is_in_progress();
void _Thread_Reset_timeslice();
void _Thread_Rotate_Ready_Queue(u32);
void _Thread_Delay_ended(void *);
void _Thread_Tickle_timeslice(void *);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_threads.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
