/*  thread.h
 *
 *  This include file contains all constants and structures associated
 *  with the thread control block.
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id$
 */

#ifndef __THREAD_h
#define __THREAD_h

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <stdlib.h>
#include "states.h"
#include "tqdata.h"
#include "watchdog.h"
#include "object.h"
#include "context.h"

//#define _LWPTHREADS_DEBUG
#define LWP_TIMESLICE_TIMER_ID			0x00070040

/*
 *  The following lists the algorithms used to manage the thread cpu budget.
 *
 *  Reset Timeslice:   At each context switch, reset the time quantum.
 */

typedef enum
{
  THREAD_CPU_BUDGET_ALGORITHM_NONE = 0,
  THREAD_CPU_BUDGET_ALGORITHM_RESET_TIMESLICE	
} Thread_CPU_budget_algorithms;

/*
 *  The following structure contains the information necessary to manage
 *  a thread which it is  waiting for a resource.
 */

typedef struct {
  Objects_Id            id;              /* waiting on this object       */
  unsigned32            count;           /* "generic" fields to be used */
  void                 *return_argument; /*   when blocking */
  void                 *return_argument_1;
  unsigned32            option;

  /*
   *  NOTE: The following assumes that all API return codes can be
   *        treated as an unsigned32.  
   */
  unsigned32            return_code;     /* status for thread awakened   */

  Chain_Control         Block2n;         /* 2 - n priority blocked chain */
  Thread_queue_Control *queue;           /* pointer to thread queue      */
}   Thread_Wait_information;

/*
 *  The following defines the "return type" of a thread.
 *
 *  NOTE:  This cannot always be right.  Some APIs have void
 *         tasks/threads, others return pointers, others may 
 *         return a numeric value.  Hopefully a pointer is
 *         always at least as big as an unsigned32. :)
 */

typedef void *Thread;

typedef Thread ( *Thread_Entry )(Thread);   /* basic type */

/*
 *  The following record defines the control block used
 *  to manage each thread.
 *
 *  NOTE: It is critical that proxies and threads have identical
 *        memory images for the shared part.
 */

typedef struct Thread_Control_struct Thread_Control; 

struct Thread_Control_struct {
  /** This field is the object management structure for each thread. */
  Objects_Control               Object;
  /** This field is the current priority state of this thread. */
  unsigned8                     current_priority;
  /** This field is the base priority of this thread. */
  unsigned8                     real_priority;
  /** This field is the number of nested suspend calls. */
  unsigned32                    suspend_count;
  /** This field is the number of mutexes currently held by this thread. */
  unsigned32                    resource_count;
  /** This field is the initial ISR disable level of this thread. */
  unsigned32                    isr_level;
  /** This field is the current execution state of this thread. */
  States_Control                current_state;
  /** This field is the length of the time quantum that this thread is
   *  allowed to consume.  The algorithm used to manage limits on CPU usage
   *  is specified by budget_algorithm.
   */
  unsigned32                    cpu_time_budget;
  /** This field is the algorithm used to manage this thread's time
   *  quantum.  The algorithm may be specified as none which case,
   *  no limit is in place.
   */
  Thread_CPU_budget_algorithms  budget_algorithm;
  /** This field is true if the thread is preemptible. */
  bool                          is_preemptible;
  /** This field is the blocking information for this thread. */
  Thread_Wait_information       Wait;
  /** This field contains precalculated priority map indices. */
  Priority_Information          Priority_map;
  /** This field is the Watchdog used to manage thread delays and timeouts. */
  Watchdog_Control              Timer;
#if defined(RTEMS_MULTIPROCESSING)
  /** This field is the received response packet in an MP system. */
  MP_packet_Prefix             *receive_packet;
#endif
  /** This field is the starting address for the thread. */
  Thread_Entry                 entry_point;
  /** This field is the pointer argument passed at thread start. */
  void                         *pointer_argument;
  /** This field is the initial stack area address. */
  void                         *stack;
  /** This field is the stack area size. */
  unsigned32                    size;
  /** This field indicates whether the SuperCore allocated the stack. */
  unsigned8                     core_allocated_stack;
  /** This field points to the Ready FIFO for this priority. */
  Chain_Control                *ready;
  /** This field holds the list of threads waiting to join. */
  Thread_queue_Control          Join_List;
  /** This field contains the context of this thread. */
  Context_Control               Registers;
  /** This field points to the newlib reentrancy structure for this thread. */
  void                         *libc_reent;
};

/*
 *  The following define the thread control pointers used to access
 *  and manipulate the idle thread.
 */

SCORE_EXTERN Thread_Control *_Thread_Idle;

/*
 *  The following points to the thread which is currently executing.
 *  This thread is implicitly manipulated by numerous directives.
 */

SCORE_EXTERN Thread_Control *_Thread_Executing;

/*
 *  The following points to the highest priority ready thread
 *  in the system.  Unless the current thread is not preemptibl,
 *  then this thread will be context switched to when the next
 *  dispatch occurs.
 */
SCORE_EXTERN Thread_Control *_Thread_Heir;

/*
 *  The following points to the thread whose floating point
 *  context is currently loaded.
 */

#if ( CPU_HARDWARE_FP == TRUE ) || ( CPU_SOFTWARE_FP == TRUE )
SCORE_EXTERN Thread_Control *_Thread_Allocated_fp;
#endif 

/*
 *  The following variable is set to TRUE when a reschedule operation
 *  has determined that the processor should be taken away from the
 *  currently executing thread and given to the heir thread.
 */

SCORE_EXTERN volatile boolean _Context_Switch_necessary;

/*
 *  The following declares the dispatch critical section nesting
 *  counter which is used to prevent context switches at inopportune
 *  moments.
 */
SCORE_EXTERN volatile unsigned32 _Thread_Dispatch_disable_level;

/*
 * The C library re-enter-rant global pointer. Some C library implementations
 * such as newlib have a single global pointer that changed during a context
 * switch. The pointer points to that global pointer. The Thread control block
 * holds a pointer to the task specific data.
 */

SCORE_EXTERN void **_Thread_libc_reent;

/*
 *  The following points to the array of FIFOs used to manage the
 *  set of ready threads.
 */
SCORE_EXTERN Chain_Control _Thread_Ready_chain[];

SCORE_EXTERN Watchdog_Control _lwp_wd_timeslice;

SCORE_EXTERN Thread_Control *_thr_main;

/*
 *  _Thread_Dispatch
 *
 *  DESCRIPTION:
 *
 *  This routine is responsible for transferring control of the
 *  processor from the executing thread to the heir thread.  As part
 *  of this process, it is responsible for the following actions:
 *
 *     + saving the context of the executing thread
 *     + restoring the context of the heir thread
 *     + dispatching any signals for the resulting executing thread
 */

void _Thread_Dispatch( void );

/*
 *  _Thread_Yield_processor
 *
 *  DESCRIPTION:
 *
 *  This routine is invoked when a thread wishes to voluntarily
 *  transfer control of the processor to another thread of equal
 *  or greater priority.
 */

void _Thread_Yield_processor( void );

void __lwp_thread_closeall();

/*
 *  _Thread_Set_state
 *
 *  DESCRIPTION:
 *
 *  This routine sets the indicated states for the_thread.  It performs
 *  any necessary scheduling operations including the selection of
 *  a new heir thread.
 *
 */

void _Thread_Set_state(
  Thread_Control *the_thread,
  States_Control  state
);

/*
 *  _Thread_Clear_state
 *
 *  DESCRIPTION:
 *
 *  This routine clears the indicated STATES for the_thread.  It performs
 *  any necessary scheduling operations including the selection of
 *  a new heir thread.
 */

void _Thread_Clear_state(
  Thread_Control *the_thread,
  States_Control  state
);

/*
 *  _Thread_Change_priority
 *
 *  DESCRIPTION:
 *
 *  This routine changes the current priority of the_thread to
 *  new_priority.  It performs any necessary scheduling operations
 *  including the selection of a new heir thread.
 */

void _Thread_Change_priority (
  Thread_Control   *the_thread,
  Priority_Control  new_priority,
  boolean           prepend_it
);

/*
 *  _Thread_Set_priority
 *
 *  DESCRIPTION:
 *
 *  This routine updates the priority related fields in the_thread
 *  control block to indicate the current priority is now new_priority.
 */

void _Thread_Set_priority(
  Thread_Control   *the_thread,
  Priority_Control  new_priority
);

/*
 *  _Thread_Set_transient
 *
 *  DESCRIPTION:
 *
 *  This routine sets the TRANSIENT state for the_thread.  It performs
 *  any necessary scheduling operations including the selection of
 *  a new heir thread.
 */

void _Thread_Set_transient(
  Thread_Control *the_thread
);

/*
 *  _Thread_Suspend
 *
 *  DESCRIPTION:
 *
 *  This routine updates the related suspend fields in the_thread
 *  control block to indicate the current nested level.
 */

void _Thread_Suspend(
  Thread_Control   *the_thread
);

/*
 *  _Thread_Resume
 *
 *  DESCRIPTION:
 *
 *  This routine updates the related suspend fields in the_thread
 *  control block to indicate the current nested level.  A force
 *  parameter of TRUE will force a resume and clear the suspend count.
 */

void _Thread_Resume(
  Thread_Control   *the_thread,
  boolean           force
);

/*
 *  _Thread_Load_environment
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the context of the_thread to its
 *  appropriate starting state.
 */

void _Thread_Load_environment(
  Thread_Control *the_thread
);

/*
 *  _Thread_Ready
 *
 *  DESCRIPTION:
 *
 *  This routine removes any set states for the_thread.  It performs
 *  any necessary scheduling operations including the selection of
 *  a new heir thread.
 */

void _Thread_Ready(
  Thread_Control *the_thread
);

/*
 *  _Thread_Initialize
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the specified the thread.  It allocates
 *  all memory associated with this thread.  It completes by adding
 *  the thread to the local object table so operations on this
 *  thread id are allowed.
 *
 *  NOTES:
 *
 *  If stack_area is NULL, it is allocated from the workspace.
 *
 *  If the stack is allocated from the workspace, then it is guaranteed to be
 *  of at least minimum size.
 */

boolean _Thread_Initialize(
  Thread_Control                       *the_thread,
  void                                 *stack_area,
  unsigned32                            stack_size,
  Priority_Control                      priority,
  unsigned32                            isr_level,
  bool                                  is_preemptible
);

/*
 *  _Thread_Start
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the executable information for a thread
 *  and makes it ready to execute.  After this routine executes, the
 *  thread competes with all other threads for CPU time.
 */
 
boolean _Thread_Start(
  Thread_Control           *the_thread,
  Thread_Entry              entry_point,
  void                     *pointer_argument
);

void pthread_exit(void *);

/*
 *  _Thread_Close
 *
 *  DESCRIPTION:
 *
 *  This routine frees all memory associated with the specified
 *  thread and removes it from the local object table so no further
 *  operations on this thread are allowed.
 */
 
void _Thread_Close(
  Thread_Control       *the_thread
);

/*
 *  _Thread_Start_multitasking
 *
 *  DESCRIPTION:
 *
 *  This routine initiates multitasking.  It is invoked only as
 *  part of initialization and its invocation is the last act of
 *  the non-multitasking part of the system initialization.
 */

void _Thread_Start_multitasking( void );

/*
 *  _Thread_Stop_multitasking
 *
 *  DESCRIPTION:
 *
 *  This routine halts multitasking and returns control to
 *  the "thread" (i.e. the BSP) which initially invoked the
 *  routine which initialized the system. 
 */

void _Thread_Stop_multitasking(void (*exitfunc)());

/*
 *  _Thread_Evaluate_mode
 *
 *  DESCRIPTION:
 *
 *  This routine evaluates the current scheduling information for the
 *  system and determines if a context switch is required.  This 
 *  is usually called after changing an execution mode such as preemptability
 *  for a thread.
 */

boolean _Thread_Evaluate_mode( void );

/*
 *  _ISR_Is_in_progress
 *
 *  DESCRIPTION:
 *
 *  Checks if an ISR in progress.
 *
 *  This function returns true if the processor is currently servicing
 *  and interrupt and false otherwise.   A return value of true indicates
 *  that the caller is an interrupt service routine, NOT a thread.
 *
 *  This methods returns true when called from an ISR.
 */

unsigned32 _ISR_Is_in_progress(void);

/*
 *  _Thread_Reset_timeslice
 *
 *  DESCRIPTION:
 *
 *  This routine is invoked upon expiration of the currently
 *  executing thread's timeslice.  If no other thread's are ready
 *  at the priority of the currently executing thread, then the
 *  executing thread's timeslice is reset.  Otherwise, the
 *  currently executing thread is placed at the rear of the
 *  FIFO for this priority and a new heir is selected.
 */

void _Thread_Reset_timeslice( void );

/*  
 *  _Thread_Rotate_Ready_Queue
 *  
 *  DESCRIPTION:
 *  
 *  This routine is invoked to rotate the ready queue for the
 *  given priority.  It can be used to yeild the processor
 *  by rotating the executing threads ready queue.
 */

void _Thread_Rotate_Ready_Queue(
  Priority_Control  priority
);

/*
 *  _Thread_Delay_ended
 *
 *  DESCRIPTION:
 *
 *  This routine is invoked when a thread must be unblocked at the
 *  end of a time based delay (i.e. wake after or wake when).
 */

void _Thread_Delay_ended(
  void       *ignored
);

/*
 *  _Thread_Tickle_timeslice
 *
 *  DESCRIPTION:
 *
 *  This routine is invoked as part of processing each clock tick.
 *  It is responsible for determining if the current thread allows
 *  timeslicing and, if so, when its timeslice expires.
 */

void _Thread_Tickle_timeslice(
  void       *ignored
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/thread.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
