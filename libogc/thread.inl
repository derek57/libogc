#ifndef __THREAD_inl
#define __THREAD_inl

/*PAGE
 *
 *  _Thread_Is_executing
 *
 *  DESCRIPTION:
 *
 *  This function returns TRUE if the_thread is the currently executing
 *  thread, and FALSE otherwise.
 */

RTEMS_INLINE_ROUTINE boolean _Thread_Is_executing (
  Thread_Control *the_thread
)
{
  return ( the_thread == _Thread_Executing );
}

/*PAGE
 *
 *  _Thread_Is_heir
 *
 *  DESCRIPTION:
 *
 *  This function returns TRUE if the_thread is the heir
 *  thread, and FALSE otherwise.
 */

RTEMS_INLINE_ROUTINE boolean _Thread_Is_heir (
  Thread_Control *the_thread
)
{
  return ( the_thread == _Thread_Heir );
}

/*PAGE
 *
 *  _Thread_Calculate_heir
 *
 *  DESCRIPTION:
 *
 *  This function returns a pointer to the highest priority
 *  ready thread.
 */

RTEMS_INLINE_ROUTINE void _Thread_Calculate_heir( void )
{
  _Thread_Heir = (Thread_Control *)
    _Thread_Ready_chain[ _Priority_Get_highest() ].first;
}

/*PAGE
 *
 *  _Thread_Is_allocated_fp
 *
 *  DESCRIPTION:
 *
 *  This function returns TRUE if the floating point context of
 *  the_thread is currently loaded in the floating point unit, and
 *  FALSE otherwise.
 */

#if ( CPU_HARDWARE_FP == TRUE ) || ( CPU_SOFTWARE_FP == TRUE )
RTEMS_INLINE_ROUTINE boolean _Thread_Is_allocated_fp (
  Thread_Control *the_thread
)
{
  return ( the_thread == _Thread_Allocated_fp );
}
#endif

/*PAGE
 *
 *  _Thread_Deallocate_fp
 *
 *  DESCRIPTION:
 *
 *  This routine is invoked when the currently loaded floating
 *  point context is now longer associated with an active thread.
 */

#if ( CPU_HARDWARE_FP == TRUE ) || ( CPU_SOFTWARE_FP == TRUE )
RTEMS_INLINE_ROUTINE void _Thread_Deallocate_fp( void )
{
  _Thread_Allocated_fp = NULL;
}
#endif

/*PAGE
 *
 *  _Thread_Dispatch_initialization
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the thread dispatching subsystem.
 */

RTEMS_INLINE_ROUTINE void _Thread_Dispatch_initialization( void )
{
  _Thread_Dispatch_disable_level = 1;
}

/*PAGE
 *
 *  _Thread_Enable_dispatch
 *
 *  DESCRIPTION:
 *
 *  This routine allows dispatching to occur again.  If this is
 *  the outer most dispatching critical section, then a dispatching
 *  operation will be performed and, if necessary, control of the
 *  processor will be transferred to the heir thread.
 */

#if ( CPU_INLINE_ENABLE_DISPATCH == TRUE )
RTEMS_INLINE_ROUTINE void _Thread_Enable_dispatch()
{
  if ( (--_Thread_Dispatch_disable_level) == 0 )
    _Thread_Dispatch();
}
#endif

/*PAGE
 *
 *  _Thread_Disable_dispatch
 *
 *  DESCRIPTION:
 *
 *  This routine prevents dispatching.
 */

RTEMS_INLINE_ROUTINE void _Thread_Disable_dispatch( void )
{
  _Thread_Dispatch_disable_level += 1;
}

/*PAGE
 *
 *  _Thread_Unnest_dispatch
 *
 *  DESCRIPTION:
 *
 *  This routine allows dispatching to occur again.  However,
 *  no dispatching operation is performed even if this is the outer
 *  most dispatching critical section.
 */

RTEMS_INLINE_ROUTINE void _Thread_Unnest_dispatch( void )
{
  _Thread_Dispatch_disable_level -= 1;
}

/*PAGE
 *
 *  _Thread_Unblock
 *
 *  DESCRIPTION:
 *
 *  This routine clears any blocking state for the_thread.  It performs
 *  any necessary scheduling operations including the selection of
 *  a new heir thread.
 */

RTEMS_INLINE_ROUTINE void _Thread_Unblock (
  Thread_Control *the_thread
)
{
  _Thread_Clear_state( the_thread, STATES_BLOCKED );
}

/*PAGE
 *
 *  _Thread_Get_libc_reent
 *
 *  DESCRIPTION:
 *
 *  This routine returns the C library re-enterant pointer.
 */
 
RTEMS_INLINE_ROUTINE void **_Thread_Get_libc_reent( void )
{
  return _Thread_libc_reent;
}

/*PAGE
 *
 *  _Thread_Set_libc_reent
 *
 *  DESCRIPTION:
 *
 *  This routine set the C library re-enterant pointer.
 */
 
RTEMS_INLINE_ROUTINE void _Thread_Set_libc_reent (
  void **libc_reent
)
{
  _Thread_libc_reent = libc_reent;
}

/*PAGE
 *
 *  _Thread_Is_context_switch_necessary
 *
 *  DESCRIPTION:
 *
 *  This function returns TRUE if dispatching is disabled, and FALSE
 *  otherwise.
 */

RTEMS_INLINE_ROUTINE boolean _Thread_Is_context_switch_necessary( void )
{
  return ( _Context_Switch_necessary );
}

/*PAGE
 *
 *  _Thread_Is_dispatching_enabled
 *
 *  DESCRIPTION:
 *
 *  This function returns TRUE if dispatching is disabled, and FALSE
 *  otherwise.
 */

RTEMS_INLINE_ROUTINE boolean _Thread_Is_dispatching_enabled( void )
{
  return ( _Thread_Dispatch_disable_level == 0 );
}

RTEMS_INLINE_ROUTINE void __lwp_thread_inittimeslice()
{
	_Watchdog_Initialize(&_lwp_wd_timeslice,_Thread_Tickle_timeslice,LWP_TIMESLICE_TIMER_ID,NULL);
}

RTEMS_INLINE_ROUTINE void __lwp_thread_starttimeslice()
{
	_Watchdog_Insert_ticks(&_lwp_wd_timeslice,millisecs_to_ticks(1));
}

RTEMS_INLINE_ROUTINE void __lwp_thread_stoptimeslice()
{
	_Watchdog_Remove_ticks(&_lwp_wd_timeslice);
}

#endif
