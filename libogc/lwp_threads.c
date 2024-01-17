/*
 *  Thread Handler
 *
 *
 *  COPYRIGHT (c) 1989-1998.
 *  On-Line Applications Research Corporation (OAR).
 *  Copyright assigned to U.S. Government, 1994.
 *
 *  The license and distribution terms for this file may be
 *  found in found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "asm.h"
#include "processor.h"
#include "sys_state.h"
#include "lwp_stack.h"
#include "lwp_threads.h"
#include "lwp_threadq.h"
#include "lwp_watchdog.h"

#define LWP_MAXPRIORITIES		256

/* new one */
Context_Control _Thread_BSP_context;

Thread_Control *_thr_main = NULL;
Thread_Control *_Thread_Idle = NULL;

Thread_Control *_Thread_Executing = NULL;
Thread_Control *_Thread_Heir = NULL;

#if ( CPU_HARDWARE_FP == TRUE ) || ( CPU_SOFTWARE_FP == TRUE )
Thread_Control *_Thread_Allocated_fp = NULL;
#endif

volatile boolean _Context_Switch_necessary;
volatile unsigned32 _Thread_Dispatch_disable_level;

Watchdog_Control _lwp_wd_timeslice;
u32 _Thread_Ticks_per_timeslice = 0;
void **_Thread_libc_reent = NULL;
Chain_Control _Thread_Ready_chain[LWP_MAXPRIORITIES];

static void (*_lwp_exitfunc)(void);

extern void _CPU_Context_switch(void *,void *);
extern void _cpu_context_switch_ex(void *,void *);
extern void _CPU_Context_save(void *);
extern void _CPU_Context_restore(void *);
extern void _CPU_Context_save_fp_context(void *);
extern void _CPU_Context_restore_fp_context(void *);

extern int libc_create_hook(Thread_Control *,Thread_Control *);
extern int libc_start_hook(Thread_Control *,Thread_Control *);
extern int libc_delete_hook(Thread_Control *, Thread_Control *);

extern void kprintf(const char *str, ...);

#ifdef _LWPTHREADS_DEBUG
static void __lwp_dumpcontext(frame_context *ctx)
{
	kprintf("GPR00 %08x GPR08 %08x GPR16 %08x GPR24 %08x\n",ctx->GPR[0], ctx->GPR[8], ctx->GPR[16], ctx->GPR[24]);
	kprintf("GPR01 %08x GPR09 %08x GPR17 %08x GPR25 %08x\n",ctx->GPR[1], ctx->GPR[9], ctx->GPR[17], ctx->GPR[25]);
	kprintf("GPR02 %08x GPR10 %08x GPR18 %08x GPR26 %08x\n",ctx->GPR[2], ctx->GPR[10], ctx->GPR[18], ctx->GPR[26]);
	kprintf("GPR03 %08x GPR11 %08x GPR19 %08x GPR27 %08x\n",ctx->GPR[3], ctx->GPR[11], ctx->GPR[19], ctx->GPR[27]);
	kprintf("GPR04 %08x GPR12 %08x GPR20 %08x GPR28 %08x\n",ctx->GPR[4], ctx->GPR[12], ctx->GPR[20], ctx->GPR[28]);
	kprintf("GPR05 %08x GPR13 %08x GPR21 %08x GPR29 %08x\n",ctx->GPR[5], ctx->GPR[13], ctx->GPR[21], ctx->GPR[29]);
	kprintf("GPR06 %08x GPR14 %08x GPR22 %08x GPR30 %08x\n",ctx->GPR[6], ctx->GPR[14], ctx->GPR[22], ctx->GPR[30]);
	kprintf("GPR07 %08x GPR15 %08x GPR23 %08x GPR31 %08x\n",ctx->GPR[7], ctx->GPR[15], ctx->GPR[23], ctx->GPR[31]);
	kprintf("LR %08x SRR0 %08x SRR1 %08x MSR %08x\n\n", ctx->LR, ctx->SRR0, ctx->SRR1,ctx->MSR);
}

void __lwp_dumpcontext_fp(Thread_Control *thrA,Thread_Control *thrB)
{
	kprintf("_cpu_contextfp_dump(%p,%p)\n",thrA,thrB);
}
#endif

/*PAGE
 *
 *  This is the PowerPC specific implementation of the routine which
 *  returns TRUE if an interrupt is in progress.
 */

boolean _ISR_Is_in_progress( void )
{
  /* 
   *  Until the patch on PR288 is in all new exception BSPs, this is
   *  the safest thing to do.
   */
#ifdef mpc8260
  return (_ISR_Nest_level != 0);
#else
  register unsigned int isr_nesting_level;
  /*
   * Move from special purpose register 0 (mfspr SPRG0, r3)
   */
  isr_nesting_level = mfspr(272);
  return isr_nesting_level;
#endif
}

/*
 *  _ISR_Set_level
 *
 *  DESCRIPTION:
 *
 *  This routine sets the current interrupt level to that specified
 *  by new_level.  The new interrupt level is effective when the
 *  routine exits.
 */

static inline void _CPU_ISR_Set_level( unsigned32 level )
{
  register unsigned int msr;
  _CPU_MSR_GET(msr);
  if (!(level & CPU_MODES_INTERRUPT_MASK)) {
    msr |= MSR_EE;
  }
  else {
    msr &= ~MSR_EE;
  }
  _CPU_MSR_SET(msr);
}

/*
 *  _ISR_Get_level
 *
 *  DESCRIPTION:
 *
 *  This routine returns the current interrupt level.
 */

static inline unsigned32 _CPU_ISR_Get_level( void )
{
  register unsigned int msr;
  _CPU_MSR_GET(msr);
  if (msr & MSR_EE) return 0;
  else	return 1;
}

/*PAGE
 *
 *  _Thread_Delay_ended
 *
 *  This routine processes a thread whose delay period has ended.
 *  It is called by the watchdog handler.
 *
 *  Input parameters:
 *    id - thread id
 *
 *  Output parameters: NONE
 */

void _Thread_Delay_ended(
  void       *arg
)
{
  Thread_Control    *the_thread;

  the_thread = (Thread_Control *)arg;

  if(!the_thread)
    return;

  _Thread_Disable_dispatch();
  _Thread_Unblock( the_thread );
  _Thread_Unnest_dispatch();
}

/*PAGE
 *
 *  _Thread_Tickle_timeslice
 *
 *  This scheduler routine determines if timeslicing is enabled
 *  for the currently executing thread and, if so, updates the
 *  timeslice count and checks for timeslice expiration.
 *
 *  Input parameters:   NONE
 *
 *  Output parameters:  NONE
 */

void _Thread_Tickle_timeslice(
  void       *ignored
)
{
  s64 ticks;
  Thread_Control *executing;

  executing = _Thread_Executing;
  ticks = millisecs_to_ticks(1);

  _Thread_Disable_dispatch();

  if ( !executing->is_preemptible ) {
    _Watchdog_Insert_ticks( &_lwp_wd_timeslice, ticks );
    _Thread_Unnest_dispatch();
    return;
  }

  if ( !_States_Is_ready( executing->current_state ) ) {
    _Watchdog_Insert_ticks( &_lwp_wd_timeslice, ticks );
    _Thread_Unnest_dispatch();
    return;
  }

  switch ( executing->budget_algorithm ) {
    case THREAD_CPU_BUDGET_ALGORITHM_NONE:
      break;

    case THREAD_CPU_BUDGET_ALGORITHM_RESET_TIMESLICE:
      if ( --executing->cpu_time_budget == 0 ) {
        _Thread_Reset_timeslice();
        executing->cpu_time_budget = _Thread_Ticks_per_timeslice;
      }
      break;
  }

  _Watchdog_Insert_ticks( &_lwp_wd_timeslice, ticks );
  _Thread_Unnest_dispatch();
}

void __thread_dispatch_fp()
{
	u32 level;
	Thread_Control *executing;

	_ISR_Disable(level);
	executing = _Thread_Executing;
#ifdef _LWPTHREADS_DEBUG
	__lwp_dumpcontext_fp(executing,_Thread_Allocated_fp);
#endif
	if(!_Thread_Is_allocated_fp(executing)) {
		if(_Thread_Allocated_fp) _CPU_Context_save_fp_context(&_Thread_Allocated_fp->Registers);
		_CPU_Context_restore_fp_context(&executing->Registers);
		_Thread_Allocated_fp = executing;
	}
	_ISR_Enable(level);
}

/*PAGE
 *
 *  _Thread_Dispatch
 *
 *  This kernel routine determines if a dispatch is needed, and if so
 *  dispatches to the heir thread.  Once the heir is running an attempt
 *  is made to dispatch any ASRs.
 *
 *  ALTERNATE ENTRY POINTS:
 *    void _Thread_Enable_dispatch();
 *
 *  Input parameters:  NONE
 *
 *  Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    dispatch thread
 *    no dispatch thread
 */

void _Thread_Dispatch( void )
{
  ISR_Level         level;
  Thread_Control   *executing;
  Thread_Control   *heir;

  _ISR_Disable( level );
  executing   = _Thread_Executing;
  while ( _Context_Switch_necessary == TRUE ) {
    heir = _Thread_Heir;
    _Thread_Dispatch_disable_level = 1;
    _Context_Switch_necessary = FALSE;
    _Thread_Executing = heir;
    _ISR_Enable( level );

    if(_Thread_libc_reent) {
      executing->libc_reent = *_Thread_libc_reent;
      *_Thread_libc_reent = heir->libc_reent;
    }

    /*
     *  If the CPU has hardware floating point, then we must address saving
     *  and restoring it as part of the context switch.
     *
     *  The second conditional compilation section selects the algorithm used
     *  to context switch between floating point tasks.  The deferred algorithm
     *  can be significantly better in a system with few floating point tasks
     *  because it reduces the total number of save and restore FP context
     *  operations.  However, this algorithm can not be used on all CPUs due
     *  to unpredictable use of FP registers by some compilers for integer
     *  operations.
     */
/*
#if ( CPU_HARDWARE_FP == TRUE ) || ( CPU_SOFTWARE_FP == TRUE )
#if ( CPU_USE_DEFERRED_FP_SWITCH != TRUE )
    if ( executing->fp_context != NULL )
      _CPU_Context_save_fp_context( &executing->fp_context );
#endif
#endif
*/

#ifdef _DEBUG
    _cpu_context_switch_ex((void*)&executing->Registers,(void*)&heir->Registers);
#else
    _CPU_Context_switch((void*)&executing->Registers,(void*)&heir->Registers);
#endif

#if ( CPU_HARDWARE_FP == TRUE ) || ( CPU_SOFTWARE_FP == TRUE )
#if ( CPU_USE_DEFERRED_FP_SWITCH == TRUE )
    if ( (heir->fp_context != NULL) && !_Thread_Is_allocated_fp( heir ) ) {
      if ( _Thread_Allocated_fp != NULL )
        _CPU_Context_save_fp_context( &_Thread_Allocated_fp->fp_context );
      _CPU_Context_restore_fp_context( &heir->fp_context );
      _Thread_Allocated_fp = heir;
    }
/*
#else
    if ( executing->fp_context != NULL )
      _CPU_Context_restore_fp_context( &executing->fp_context );
*/
#endif
#endif

    executing = _Thread_Executing;

    _ISR_Disable( level );
  }

  _Thread_Dispatch_disable_level = 0;

  _ISR_Enable( level );
}

/*PAGE
 *
 *  _Thread_Handler
 *
 *  This routine is the default thread exitted error handler.  It is
 *  returned to when a thread exits.  The configured fatal error handler
 *  is invoked to process the exit.
 *
 *  Input parameters:   NONE
 *
 *  Output parameters:  NONE
 */

void _Thread_Handler( void )
{
  unsigned32      level;
  Thread_Control *executing;

  executing = _Thread_Executing;

  level = executing->isr_level;
  _CPU_ISR_Set_level(level);

  /*
   *  At this point, the dispatch disable level BETTER be 1.
   */

  _Thread_Enable_dispatch();
 
  executing->Wait.return_argument = executing->entry_point(executing->pointer_argument);

  pthread_exit(executing->Wait.return_argument);
}

/*PAGE
 *
 *  _Thread_Rotate_Ready_Queue
 *
 *  This kernel routine will rotate the ready queue.
 *  remove the running THREAD from the ready chain
 *  and place it immediatly at the rear of this chain.  Reset timeslice
 *  and yield the processor functions both use this routine, therefore if
 *  reset is TRUE and this is the only thread on the chain then the
 *  timeslice counter is reset.  The heir THREAD will be updated if the
 *  running is also the currently the heir.
 *
 *  Input parameters:  
 *      Priority of the queue we wish to modify.
 *
 *  Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    ready chain
 *    select heir
 */

void _Thread_Rotate_Ready_Queue( 
//  Priority_Control  priority
  unsigned32        priority
)
{
  ISR_Level       level;
  Thread_Control *executing;
  Chain_Control  *ready;
  Chain_Node     *node;

  ready     = &_Thread_Ready_chain[ priority ];
  executing = _Thread_Executing;

  if ( ready == executing->ready ) {
    _Thread_Yield_processor();
    return;
  }

  _ISR_Disable( level );

  if ( !_Chain_Is_empty( ready ) ) {
    if (!_Chain_Has_only_one_node( ready ) ) {
      node = _Chain_Get_first_unprotected( ready );
      _Chain_Append_unprotected( ready, node );
    }
  }

  _ISR_Flash( level );

  if ( _Thread_Heir->ready == ready )
    _Thread_Heir = (Thread_Control *) ready->first;

  if ( executing != _Thread_Heir )
    _Context_Switch_necessary = TRUE;

  _ISR_Enable( level );
}

/*PAGE
 *
 *  _Thread_Yield_processor
 *
 *  This kernel routine will remove the running THREAD from the ready chain
 *  and place it immediatly at the rear of this chain.  Reset timeslice
 *  and yield the processor functions both use this routine, therefore if
 *  reset is TRUE and this is the only thread on the chain then the
 *  timeslice counter is reset.  The heir THREAD will be updated if the
 *  running is also the currently the heir.
 *
 *  Input parameters:   NONE
 *
 *  Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    ready chain
 *    select heir
 */

void _Thread_Yield_processor( void )
{
  ISR_Level       level;
  Thread_Control *executing;
  Chain_Control  *ready;

  executing = _Thread_Executing;
  ready     = executing->ready;
  _ISR_Disable( level );
    if ( !_Chain_Has_only_one_node( ready ) ) {
      _Chain_Extract_unprotected( &executing->Object.Node );
      _Chain_Append_unprotected( ready, &executing->Object.Node );

      _ISR_Flash( level );

      if ( _Thread_Is_heir( executing ) )
        _Thread_Heir = (Thread_Control *) ready->first;
      _Context_Switch_necessary = TRUE;
    }
    else if ( !_Thread_Is_heir( executing ) )
      _Context_Switch_necessary = TRUE;

  _ISR_Enable( level );
}

/*PAGE
 *
 *  _Thread_Reset_timeslice
 *
 *  This routine will remove the running thread from the ready chain
 *  and place it immediately at the rear of this chain and then the
 *  timeslice counter is reset.  The heir THREAD will be updated if
 *  the running is also the currently the heir.
 *
 *  Input parameters:   NONE
 *
 *  Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    ready chain
 *    select heir
 */

void _Thread_Reset_timeslice( void )
{
  ISR_Level       level;
  Thread_Control *executing;
  Chain_Control  *ready;

  executing = _Thread_Executing;
  ready     = executing->ready;
  _ISR_Disable( level );
    if ( _Chain_Has_only_one_node( ready ) ) {
      _ISR_Enable( level );
      return;
    }
    _Chain_Extract_unprotected( &executing->Object.Node );
    _Chain_Append_unprotected( ready, &executing->Object.Node );

  _ISR_Flash( level );

    if ( _Thread_Is_heir( executing ) )
      _Thread_Heir = (Thread_Control *) ready->first;

    _Context_Switch_necessary = TRUE;

  _ISR_Enable( level );
}

/*PAGE
 *
 * _Thread_Set_state
 *
 * This kernel routine sets the requested state in the THREAD.  The
 * THREAD chain is adjusted if necessary.
 *
 * Input parameters:
 *   the_thread   - pointer to thread control block
 *   state - state to be set
 *
 * Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    ready chain
 *    select map
 */

void _Thread_Set_state(
  Thread_Control *the_thread,
  States_Control         state
)
{
  ISR_Level             level;
  Chain_Control *ready;

  ready = the_thread->ready;
  _ISR_Disable( level );
  if ( !_States_Is_ready( the_thread->current_state ) ) {
    the_thread->current_state =
       _States_Clear( the_thread->current_state, state );
    _ISR_Enable( level );
    return;
  }

  the_thread->current_state = state;

  if ( _Chain_Has_only_one_node( ready ) ) {

    _Chain_Initialize_empty( ready );
    _Priority_Remove_from_bit_map( &the_thread->Priority_map );

  } else
    _Chain_Extract_unprotected( &the_thread->Object.Node );

  _ISR_Flash( level );

  if ( _Thread_Is_heir( the_thread ) )
     _Thread_Calculate_heir();

  if ( _Thread_Is_executing( the_thread ) )
    _Context_Switch_necessary = TRUE;

  _ISR_Enable( level );
}


/*PAGE
 *
 *  _Thread_Clear_state
 *
 *  This kernel routine clears the appropriate states in the
 *  requested thread.  The thread ready chain is adjusted if
 *  necessary and the Heir thread is set accordingly.
 *
 *  Input parameters:
 *    the_thread - pointer to thread control block
 *    state      - state set to clear
 *
 *  Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    priority map
 *    select heir
 */


void _Thread_Clear_state(
  Thread_Control *the_thread,
  States_Control  state
)
{
  ISR_Level       level;
  States_Control  current_state;

  _ISR_Disable( level );
    current_state = the_thread->current_state;
    
    if ( current_state & state ) {
      current_state = 
      the_thread->current_state = _States_Clear( current_state, state );

      if ( _States_Is_ready( current_state ) ) {

        _Priority_Add_to_bit_map( &the_thread->Priority_map );

        _Chain_Append_unprotected(the_thread->ready, &the_thread->Object.Node);

        _ISR_Flash( level );

        if ( the_thread->current_priority < _Thread_Heir->current_priority ) {
          _Thread_Heir = the_thread;
          if ( _Thread_Executing->is_preemptible ||
               the_thread->current_priority == 0 )
            _Context_Switch_necessary = TRUE;
        }
      }
  }
  _ISR_Enable( level );
}

/*PAGE
 *
 *  _Thread_Evaluate_mode
 *
 *  XXX
 */

boolean _Thread_Evaluate_mode( void )
{
  Thread_Control     *executing;

  executing = _Thread_Executing;

  if ( !_States_Is_ready( executing->current_state ) ||
       ( !_Thread_Is_heir( executing ) && executing->is_preemptible ) ) {
    _Context_Switch_necessary = TRUE;
    return TRUE;
  }

  return FALSE;
}

/*PAGE
 *
 *  _Thread_Change_priority
 *
 *  This kernel routine changes the priority of the thread.  The
 *  thread chain is adjusted if necessary.
 *
 *  Input parameters:
 *    the_thread   - pointer to thread control block
 *    new_priority - ultimate priority
 *    prepend_it   - TRUE if the thread should be prepended to the chain
 *
 *  Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    ready chain
 *    select heir
 */

void _Thread_Change_priority(
  Thread_Control   *the_thread,
//  Priority_Control  new_priority,
  unsigned32        new_priority,
  boolean           prepend_it
)
{
  ISR_Level level;
  /* boolean   do_prepend = FALSE; */

  /*
   *  If this is a case where prepending the task to its priority is
   *  potentially desired, then we need to consider whether to do it.
   *  This usually occurs when a task lowers its priority implcitly as
   *  the result of losing inherited priority.  Normal explicit priority
   *  change calls (e.g. rtems_task_set_priority) should always do an
   *  append not a prepend.
   */
 
  /*
   *  Techically, the prepend should conditional on the thread lowering
   *  its priority but that does allow cxd2004 of the acvc 2.0.1 to 
   *  pass with rtems 4.0.0.  This should change when gnat redoes its
   *  priority scheme.
   */
/*
  if ( prepend_it &&
       _Thread_Is_executing( the_thread ) && 
       new_priority >= the_thread->current_priority )
    prepend_it = TRUE;
*/
                  
  _Thread_Set_transient( the_thread );

  if ( the_thread->current_priority != new_priority )
    _Thread_Set_priority( the_thread, new_priority );

  _ISR_Disable( level );

  the_thread->current_state =
    _States_Clear( the_thread->current_state, STATES_TRANSIENT );

  if ( ! _States_Is_ready( the_thread->current_state ) ) {
    /*
     *  XXX If a task is to be reordered while blocked on a priority
     *  XXX priority ordered thread queue, then this is where that
     *  XXX should occur.
     */
    _ISR_Enable( level );
    return;
  }

  _Priority_Add_to_bit_map( &the_thread->Priority_map );
  if ( prepend_it )
    _Chain_Prepend_unprotected( the_thread->ready, &the_thread->Object.Node );
  else
    _Chain_Append_unprotected( the_thread->ready, &the_thread->Object.Node );

  _ISR_Flash( level );

  _Thread_Calculate_heir();

  if ( !(_Thread_Executing == _Thread_Heir) &&
       _Thread_Executing->is_preemptible )
    _Context_Switch_necessary = TRUE;
  _ISR_Enable( level );
}

/*PAGE
 *
 * _Thread_Set_priority
 *
 * This directive enables and disables several modes of
 * execution for the requesting thread.
 *
 *  Input parameters:
 *    the_thread   - pointer to thread priority
 *    new_priority - new priority
 *
 *  Output: NONE
 */

void _Thread_Set_priority(
  Thread_Control   *the_thread,
//  Priority_Control  new_priority
  unsigned32        new_priority
)
{
  the_thread->current_priority = new_priority;
  the_thread->ready            = &_Thread_Ready_chain[ new_priority ];

  _Priority_Initialize_information( &the_thread->Priority_map, new_priority );
}

/*PAGE
 *
 * _Thread_Suspend
 *
 * This kernel routine sets the SUSPEND state in the THREAD.  The
 * THREAD chain and suspend count are adjusted if necessary.
 *
 * Input parameters:
 *   the_thread   - pointer to thread control block
 *
 * Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    ready chain
 *    select map
 */

void _Thread_Suspend(
  Thread_Control   *the_thread
)
{
  ISR_Level      level;
  Chain_Control *ready;

  ready = the_thread->ready;
  _ISR_Disable( level );
  the_thread->suspend_count++;
  if ( !_States_Is_ready( the_thread->current_state ) ) {
    the_thread->current_state =
       _States_Set( the_thread->current_state, STATES_SUSPENDED );
    _ISR_Enable( level );
    return;
  }

  the_thread->current_state = STATES_SUSPENDED;

  if ( _Chain_Has_only_one_node( ready ) ) {

    _Chain_Initialize_empty( ready );
    _Priority_Remove_from_bit_map( &the_thread->Priority_map );

  } else
    _Chain_Extract_unprotected( &the_thread->Object.Node );

  _ISR_Flash( level );

  if ( _Thread_Is_heir( the_thread ) )
     _Thread_Calculate_heir();

  if ( _Thread_Is_executing( the_thread ) )
    _Context_Switch_necessary = TRUE;

  _ISR_Enable( level );
}

/*PAGE
 *
 *  _Thread_Set_transient
 *
 *  This kernel routine places the requested thread in the transient state
 *  which will remove it from the ready queue, if necessary.  No
 *  rescheduling is necessary because it is assumed that the transient
 *  state will be cleared before dispatching is enabled.
 *
 *  Input parameters:
 *    the_thread - pointer to thread control block
 *
 *  Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    only case
 */

void _Thread_Set_transient(
  Thread_Control *the_thread
)
{
  ISR_Level             level;
  unsigned32            old_state;
  Chain_Control *ready;

  ready = the_thread->ready;
  _ISR_Disable( level );

  old_state = the_thread->current_state;
  the_thread->current_state = _States_Set( old_state, STATES_TRANSIENT );

  if ( _States_Is_ready( old_state ) ) {
    if ( _Chain_Has_only_one_node( ready ) ) {

      _Chain_Initialize_empty( ready );
      _Priority_Remove_from_bit_map( &the_thread->Priority_map );

    } else
      _Chain_Extract_unprotected( &the_thread->Object.Node );
  }

  _ISR_Enable( level );

}

/*PAGE
 *
 *  _Thread_Resume
 *
 *  This kernel routine clears the SUSPEND state if the suspend_count
 *  drops below one.  If the force parameter is set the suspend_count
 *  is forced back to zero. The thread ready chain is adjusted if
 *  necessary and the Heir thread is set accordingly.
 *
 *  Input parameters:
 *    the_thread - pointer to thread control block
 *    force      - force the suspend count back to 0
 *
 *  Output parameters:  NONE
 *
 *  INTERRUPT LATENCY:
 *    priority map
 *    select heir
 */


void _Thread_Resume(
  Thread_Control   *the_thread,
  boolean           force
)
{

  ISR_Level       level;
  States_Control  current_state;

  _ISR_Disable( level );
  
  if ( force == TRUE )
    the_thread->suspend_count = 0;
  else
    the_thread->suspend_count--;

  if ( the_thread->suspend_count > 0 ) {
    _ISR_Enable( level );
    return;
  }

  current_state = the_thread->current_state;
  if ( current_state & STATES_SUSPENDED ) {
    current_state = 
    the_thread->current_state = _States_Clear(the_thread->current_state, STATES_SUSPENDED);

    if ( _States_Is_ready( current_state ) ) {

      _Priority_Add_to_bit_map( &the_thread->Priority_map );

      _Chain_Append_unprotected(the_thread->ready, &the_thread->Object.Node);

      _ISR_Flash( level );

      if ( the_thread->current_priority < _Thread_Heir->current_priority ) {
        _Thread_Heir = the_thread;
        if ( _Thread_Executing->is_preemptible ||
             the_thread->current_priority == 0 )
          _Context_Switch_necessary = TRUE;
      }
    }
  }

  _ISR_Enable( level );
}

/*PAGE
 *
 *  _Thread_Load_environment
 *
 *  Load starting environment for another thread from its start area in the
 *  thread.  Only called from t_restart and t_start.
 *
 *  Input parameters:
 *    the_thread - thread control block pointer
 *
 *  Output parameters:  NONE
 */

void _Thread_Load_environment(
  Thread_Control *the_thread
)
{
  unsigned32 stackbase;
  unsigned32 sp;
  unsigned32 size;
  unsigned32 r2;
  unsigned32 r13;
  unsigned32 msr_value;

  the_thread->Registers.FPSCR = PPC_INIT_FPSCR;

  stackbase = (unsigned32)the_thread->stack;
  size      = the_thread->size;

  // tag both bottom & head of stack
  *((unsigned32 *)stackbase) = 0xDEADBABE;
  sp = stackbase + size - CPU_MINIMUM_STACK_FRAME_SIZE;
  sp &= ~(CPU_STACK_ALIGNMENT - 1);

  /* tag the bottom (T. Straumann 6/36/2001 <strauman@slac.stanford.edu>) */ 
  *((unsigned32 *)sp) = 0;

  the_thread->Registers.GPR[1] = sp;

  msr_value = (MSR_ME | MSR_IR | MSR_DR | MSR_RI);

  if ( !( the_thread->isr_level & CPU_MODES_INTERRUPT_MASK ) )
    msr_value |= MSR_EE;

  the_thread->Registers.MSR = msr_value;
  the_thread->Registers.LR  = (unsigned32)_Thread_Handler;

  __asm__ __volatile__ ("mr %0,2; mr %1,13" : "=r" ((r2)), "=r" ((r13)));

  the_thread->Registers.GPR[2]  = r2;
  the_thread->Registers.GPR[13] = r13;

#ifdef _LWPTHREADS_DEBUG
  kprintf( "_Thread_Load_environment(%p,%p,%d,%p)\n", the_thread, (void *)stackbase, size, (void *)sp );
#endif
}

/*PAGE
 *
 *  _Thread_Ready
 *
 *  This kernel routine readies the requested thread, the thread chain
 *  is adjusted.  A new heir thread may be selected.
 *
 *  Input parameters:
 *    the_thread - pointer to thread control block
 *
 *  Output parameters:  NONE
 *
 *  NOTE:  This routine uses the "blocking" heir selection mechanism.
 *         This insures the correct heir after a thread restart.
 *
 *  INTERRUPT LATENCY:
 *    ready chain
 *    select heir
 */

void _Thread_Ready(
  Thread_Control *the_thread
)
{
  ISR_Level              level;
  Thread_Control *heir;

  _ISR_Disable( level );

  the_thread->current_state = STATES_READY;

  _Priority_Add_to_bit_map( &the_thread->Priority_map );

  _Chain_Append_unprotected( the_thread->ready, &the_thread->Object.Node );

  _ISR_Flash( level );

  _Thread_Calculate_heir();

  heir = _Thread_Heir;

  if ( !_Thread_Is_executing( heir ) && _Thread_Executing->is_preemptible ) 
    _Context_Switch_necessary = TRUE;

  _ISR_Enable( level );
}

/*PAGE
 *
 *  _Thread_Initialize
 *
 *  This routine initializes the specified the thread.  It allocates
 *  all memory associated with this thread.  It completes by adding
 *  the thread to the local object table so operations on this
 *  thread id are allowed.
 */

boolean _Thread_Initialize(
  Thread_Control                       *the_thread,
  void                                 *stack_area,
  unsigned32                            stack_size,
//  Priority_Control                      priority,
  unsigned32                            priority,
  unsigned32                            isr_level,
  bool                                  is_preemptible
)
{
  unsigned32           actual_stack_size = 0;

  /*
   *  Allocate and Initialize the stack for this thread.
   */


  if ( !stack_area ) {
    if ( !_Stack_Is_enough( stack_size ) )
      actual_stack_size = STACK_MINIMUM_SIZE;
    else
      actual_stack_size = stack_size;

    actual_stack_size = _Thread_Stack_Allocate( the_thread, actual_stack_size );
 
    if ( !actual_stack_size ) 
      return FALSE;                     /* stack allocation failed */

    the_thread->core_allocated_stack = TRUE;
  } else {
    the_thread->stack = stack_area;
    actual_stack_size = stack_size;
    the_thread->core_allocated_stack = FALSE;
  } 

  the_thread->size = actual_stack_size;

  _Thread_queue_Initialize( &the_thread->Join_List, THREAD_QUEUE_DISCIPLINE_FIFO, STATES_WAITING_FOR_JOIN_AT_EXIT, 0 );

  memset( &the_thread->Registers, 0, sizeof(the_thread->Registers) );
  memset( &the_thread->Wait, 0, sizeof(the_thread->Wait) );

  /*
   *  General initialization
   */

  the_thread->budget_algorithm = ( priority < 128 ? THREAD_CPU_BUDGET_ALGORITHM_NONE : THREAD_CPU_BUDGET_ALGORITHM_RESET_TIMESLICE );
  the_thread->is_preemptible   = is_preemptible;
  the_thread->isr_level        = isr_level;
  the_thread->real_priority    = priority;
  the_thread->current_state    = STATES_DORMANT;
  the_thread->cpu_time_budget  = _Thread_Ticks_per_timeslice;
  the_thread->suspend_count    = 0;
  the_thread->resource_count   = 0;

  _Thread_Set_priority( the_thread, priority );

  libc_create_hook( _Thread_Executing, the_thread );

  return TRUE;
}

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
)
{
  ISR_Level             level;
  void                **value_ptr;
  Thread_Control       *p;

  _Thread_Set_state( the_thread, STATES_TRANSIENT );
 
  if ( !_Thread_queue_Extract_with_proxy( the_thread ) ) {
 
    if ( _Watchdog_Is_active( &the_thread->Timer ) )
      _Watchdog_Remove_ticks( &the_thread->Timer );
  }

  _ISR_Disable( level );
  value_ptr = (void **)the_thread->Wait.return_argument;

  while ( ( p = _Thread_queue_Dequeue( &the_thread->Join_List ) ) != NULL ) {
    *(void **)p->Wait.return_argument = value_ptr;
  }

  the_thread->cpu_time_budget = 0;
  the_thread->budget_algorithm = THREAD_CPU_BUDGET_ALGORITHM_NONE;
  _ISR_Enable( level );

  libc_delete_hook( _Thread_Executing, the_thread );

#if ( CPU_HARDWARE_FP == TRUE ) || ( CPU_SOFTWARE_FP == TRUE )
//#if ( CPU_USE_DEFERRED_FP_SWITCH == TRUE )
  if ( _Thread_Is_allocated_fp( the_thread ) )
    _Thread_Deallocate_fp();
/*
#endif
  the_thread->fp_context = NULL;

  if ( the_thread->Start.fp_context )
    (void) _Workspace_Free( the_thread->Start.fp_context );
*/
#endif

  _Thread_Stack_Free( the_thread ); 

  _Objects_Close( the_thread->Object.information, &the_thread->Object );
  _Objects_Free( the_thread->Object.information, &the_thread->Object );
}

void __lwp_thread_closeall( void )
{
  unsigned32      i;
  ISR_Level       level;
  Chain_Control  *header;
  Thread_Control *ptr;
  Thread_Control *next;

#ifdef _LWPTHREADS_DEBUG
  kprintf("__lwp_thread_closeall(enter)\n");
#endif

  _ISR_Disable( level );

  for ( i=0; i < LWP_MAXPRIORITIES; i++ ) {
    header = &_Thread_Ready_chain[ i ];
    ptr    = (Thread_Control *)header->first;

    while ( ptr != (Thread_Control *)_Chain_Tail( &_Thread_Ready_chain[ i ] ) ) {
      next = (Thread_Control *)ptr->Object.Node.next;

      if ( ptr != _Thread_Executing )
        _Thread_Close( ptr );

        ptr = next;
    }
  }

  _ISR_Enable( level );

#ifdef _LWPTHREADS_DEBUG
  kprintf( "__lwp_thread_closeall(leave)\n" );
#endif
}

void pthread_exit(
  void  *value_ptr
)
{
	_Thread_Disable_dispatch();

	_Thread_Executing->Wait.return_argument = (unsigned32 *)value_ptr;

	_Thread_Close( _Thread_Executing );

	_Thread_Enable_dispatch();
}

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
  Thread_Control       *the_thread,
  Thread_Entry          entry_point,
  void                 *pointer_argument
)
{
  if ( _States_Is_dormant( the_thread->current_state ) ) {
 
    the_thread->entry_point      = entry_point;
   
    the_thread->pointer_argument = pointer_argument;
 
    _Thread_Load_environment( the_thread );
 
    _Thread_Ready( the_thread );
 
    libc_start_hook( _Thread_Executing, the_thread );
 
    return TRUE;
  }
 
  return FALSE;
 
}

/*PAGE
 *
 *  _Thread_Start_multitasking
 *
 *  This kernel routine readies the requested thread, the thread chain
 *  is adjusted.  A new heir thread may be selected.
 *
 *  Input parameters:
 *    system_thread - pointer to system initialization thread control block
 *    idle_thread   - pointer to idle thread control block
 *
 *  Output parameters:  NONE
 *
 *  NOTE:  This routine uses the "blocking" heir selection mechanism.
 *         This insures the correct heir after a thread restart.
 *
 *  INTERRUPT LATENCY:
 *    ready chain
 *    select heir
 */

void _Thread_Start_multitasking( void )
{
  _lwp_exitfunc = NULL;

  _System_state_Set( SYSTEM_STATE_BEGIN_MULTITASKING );

  /*
   *  The system is now multitasking and completely initialized.  
   *  This system thread now either "goes away" in a single processor 
   *  system or "turns into" the server thread in an MP system.
   */

  _System_state_Set( SYSTEM_STATE_UP );

  _Context_Switch_necessary = FALSE;

  _Thread_Executing = _Thread_Heir;

   /*
    * Get the init task(s) running.
    *
    * Note: Thread_Dispatch() is normally used to dispatch threads.  As
    *       part of its work, Thread_Dispatch() restores floating point
    *       state for the heir task.
    *
    *       This code avoids Thread_Dispatch(), and so we have to restore
    *       (actually initialize) the floating point state "by hand".
    *
    *       Ignore the CPU_USE_DEFERRED_FP_SWITCH because we must always
    *       switch in the first thread if it is FP.
    */
 
#if 0
#if ( CPU_HARDWARE_FP == TRUE ) || ( CPU_SOFTWARE_FP == TRUE )
   /*
    *  don't need to worry about saving BSP's floating point state
    */

   if ( _Thread_Heir->fp_context != NULL )
     _Context_Restore_fp( &_Thread_Heir->fp_context );
#endif
#endif

  __lwp_thread_starttimeslice();
  _CPU_Context_switch( &_Thread_BSP_context, &_Thread_Heir->Registers );

  if ( _lwp_exitfunc )
    _lwp_exitfunc();
}

/*PAGE
 *
 *  _Thread_Stop_multitasking
 *
 *  This routine halts multitasking and returns control to
 *  the "thread" (i.e. the BSP) which initially invoked the
 *  routine which initialized the system. 
 */

void _Thread_Stop_multitasking(
  void (*exitfunc)()
)
{
  _lwp_exitfunc = exitfunc;

  if ( _System_state_Get() != SYSTEM_STATE_SHUTDOWN ) {
    __lwp_thread_stoptimeslice();
    _System_state_Set( SYSTEM_STATE_SHUTDOWN );
    _CPU_Context_switch( (void *)&_Thread_Executing->Registers, (void *)&_Thread_BSP_context );
  }
}

/*PAGE
 *
 *  _Thread_Handler_initialization
 *
 *  This routine initializes all thread manager related data structures.
 *
 *  Input parameters:
 *    maximum_tasks       - number of tasks to initialize
 *    ticks_per_timeslice - clock ticks per quantum
 *
 *  Output parameters:  NONE
 */

void _Thread_Handler_initialization( void )
{
  unsigned32 index;

#ifdef _LWPTHREADS_DEBUG
  kprintf( "_Thread_Handler_initialization()\n\n" );
#endif

  _Thread_Dispatch_initialization();
  __lwp_thread_inittimeslice();

  _Context_Switch_necessary = FALSE;
  _Thread_Executing         = NULL;
  _Thread_Heir              = NULL;
#if ( CPU_HARDWARE_FP == TRUE ) || ( CPU_SOFTWARE_FP == TRUE )
  _Thread_Allocated_fp      = NULL;
#endif

  _Thread_Ticks_per_timeslice = 10;

  memset( &_Thread_BSP_context, 0, sizeof(_Thread_BSP_context) );

  for ( index=0; index <= PRIORITY_MAXIMUM; index++ )
    _Chain_Initialize_empty( &_Thread_Ready_chain[ index ] );

  _System_state_Set( SYSTEM_STATE_BEFORE_MULTITASKING );
}
