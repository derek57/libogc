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

typedef struct {
	u32 id;
	u32 count;
	void *return_argument;
	void *return_argument_1;
	u32 option;
	u32 return_code;
	Chain_Control Block2n;
	Thread_queue_Control *queue;
} Thread_Wait_information;

typedef struct Thread_Control_struct Thread_Control; 

struct Thread_Control_struct {
	Objects_Control Object;
	u8  current_priority,real_priority;
	u32 suspend_count,resource_count;
	u32 isr_level;
	u32 current_state;
	u32 cpu_time_budget;
	lwp_cpu_budget_algorithms budget_algorithm;
	bool is_preemptible;
	Thread_Wait_information Wait;
	Priority_Information Priority_map;
	Watchdog_Control Timer;

	void* (*entry_point)(void *);
	void *pointer_argument;
	void *stack;
	u32 size;
	u8 core_allocated_stack;
	Chain_Control *ready;
	Thread_queue_Control join_list;
	Context_Control Registers;		//16
	void *libc_reent;
};

extern Thread_Control *_thr_main;
extern Thread_Control *_thr_idle;
extern Thread_Control *_thr_executing;
extern Thread_Control *_thr_heir;
extern Thread_Control *_thr_allocated_fp;
extern vu32 _context_switch_want;
extern vu32 _thread_dispatch_disable_level;

extern Watchdog_Control _lwp_wd_timeslice;
extern void **__lwp_thr_libc_reent;
extern Chain_Control _lwp_thr_ready[];

void _Thread_Dispatch();
void _Thread_Yield_processor();
void __lwp_thread_closeall();
void _Thread_Set_state(Thread_Control *,u32);
void _Thread_Clear_state(Thread_Control *,u32);
void _Thread_Change_priority(Thread_Control *,u32,u32);
void _Thread_Set_priority(Thread_Control *,u32);
void _Thread_Set_transient(Thread_Control *);
void _Thread_Suspend(Thread_Control *);
void _Thread_Resume(Thread_Control *,u32);
void _Thread_Load_environment(Thread_Control *);
void _Thread_Ready(Thread_Control *);
u32 _Thread_Initialize(Thread_Control *,void *,u32,u32,u32,bool);
u32 _Thread_Start(Thread_Control *,void* (*)(void*),void *);
void pthread_exit(void *);
void _Thread_Close(Thread_Control *);
void _Thread_Start_multitasking();
void _Thread_Stop_multitasking(void (*exitfunc)());
Objects_Control* _Thread_Get(Thread_Control *);
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
