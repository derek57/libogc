/*  watchdog.h
 *
 *  This include file contains all the constants and structures associated
 *  with watchdog timers.   This Handler provides mechanisms which can be
 *   used to initialize and manipulate watchdog timers.
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

#ifndef __WATCHDOG_h
#define __WATCHDOG_h

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include "lwp_queue.h"
#include <time.h>

#if defined(HW_RVL)
	#define TB_BUS_CLOCK				243000000u
	#define TB_CORE_CLOCK				729000000u
#elif defined(HW_DOL)
	#define TB_BUS_CLOCK				162000000u
	#define TB_CORE_CLOCK				486000000u
#endif
#define TB_TIMER_CLOCK				(TB_BUS_CLOCK/4000)			//4th of the bus frequency

#define TB_SECSPERMIN				60
#define TB_MINSPERHR				60
#define TB_MONSPERYR				12
#define TB_DAYSPERYR				365
#define TB_HRSPERDAY				24
#define TB_SECSPERDAY				(TB_SECSPERMIN*TB_MINSPERHR*TB_HRSPERDAY)
#define TB_SECSPERNYR				(365*TB_SECSPERDAY)
								
#define TB_MSPERSEC					1000
#define TB_USPERSEC					1000000
#define TB_NSPERSEC					1000000000
#define TB_NSPERMS					1000000
#define TB_NSPERUS					1000
#define TB_USPERTICK				10000

#define ticks_to_cycles(ticks)		((((Watchdog_Interval)(ticks)*(Watchdog_Interval)((TB_CORE_CLOCK*2)/TB_TIMER_CLOCK))/2))
#define ticks_to_secs(ticks)		(((Watchdog_Interval)(ticks)/(Watchdog_Interval)(TB_TIMER_CLOCK*1000)))
#define ticks_to_millisecs(ticks)	(((Watchdog_Interval)(ticks)/(Watchdog_Interval)(TB_TIMER_CLOCK)))
#define ticks_to_microsecs(ticks)	((((Watchdog_Interval)(ticks)*8)/(Watchdog_Interval)(TB_TIMER_CLOCK/125)))
#define ticks_to_nanosecs(ticks)	((((Watchdog_Interval)(ticks)*8000)/(Watchdog_Interval)(TB_TIMER_CLOCK/125)))

#define tick_microsecs(ticks)		((((Watchdog_Interval)(ticks)*8)%(Watchdog_Interval)(TB_TIMER_CLOCK/125)))
#define tick_nanosecs(ticks)		((((Watchdog_Interval)(ticks)*8000)%(Watchdog_Interval)(TB_TIMER_CLOCK/125)))


#define secs_to_ticks(sec)			((Watchdog_Interval)(sec)*(TB_TIMER_CLOCK*1000))
#define millisecs_to_ticks(msec)	((Watchdog_Interval)(msec)*(TB_TIMER_CLOCK))
#define microsecs_to_ticks(usec)	(((Watchdog_Interval)(usec)*(TB_TIMER_CLOCK/125))/8)
#define nanosecs_to_ticks(nsec)		(((Watchdog_Interval)(nsec)*(TB_TIMER_CLOCK/125))/8000)

#define diff_ticks(tick0,tick1)		(((Watchdog_Interval)(tick1)<(Watchdog_Interval)(tick0))?((Watchdog_Interval)-1-(Watchdog_Interval)(tick0)+(Watchdog_Interval)(tick1)):((Watchdog_Interval)(tick1)-(Watchdog_Interval)(tick0)))

/*
 *  The following enumerated type lists the states in which a
 *  watchdog timer may be at any given time.
 */

typedef enum {
  WATCHDOG_INACTIVE,       /* off all chains */
  WATCHDOG_BEING_INSERTED, /* off all chains, searching for insertion point */
  WATCHDOG_ACTIVE,         /* on chain, allowed to fire */
  WATCHDOG_REMOVE_IT       /* on chain, remove without firing if expires */
} Watchdog_States;

/*
 *  The following enumerated type details the manner in which
 *  a watchdog chain may be adjusted by the Watchdog_Adjust
 *  routine.  The direction indicates a movement FORWARD
 *  or BACKWARD in time.
 */

typedef enum {
  WATCHDOG_FORWARD,      /* adjust delta value forward */
  WATCHDOG_BACKWARD      /* adjust delta value backward */
} Watchdog_Adjust_directions;

/*
 *  Constant for indefinite wait.  (actually an illegal interval)
 */

#define WATCHDOG_NO_TIMEOUT			0

#define LWP_WD_ABS(x)				((signed64)(x)>0?(signed64)(x):-((signed64)(x)))

/*
 *  The following are used for synchronization purposes
 *  during an insert on a watchdog delta chain.
 */

SCORE_EXTERN volatile unsigned32  _Watchdog_Sync_level;
SCORE_EXTERN volatile unsigned32  _Watchdog_Sync_count;

/*
 *  The following contains the number of ticks since the
 *  system was booted.
 */

SCORE_EXTERN unsigned32 _Watchdog_Ticks_since_boot;

/*
 *  The following defines the watchdog chains which are managed
 *  on ticks and second boundaries.
 */

SCORE_EXTERN Chain_Control _Watchdog_Ticks_chain;

extern unsigned32 gettick();
extern Watchdog_Interval gettime();
extern void settime(Watchdog_Interval);

unsigned32 diff_sec(Watchdog_Interval start,Watchdog_Interval end);
unsigned32 diff_msec(Watchdog_Interval start,Watchdog_Interval end);
unsigned32 diff_usec(Watchdog_Interval start,Watchdog_Interval end);
unsigned32 diff_nsec(Watchdog_Interval start,Watchdog_Interval end);

/*
 *  The following types define a pointer to a watchdog service routine.
 */

typedef void Watchdog_Service_routine;

typedef Watchdog_Service_routine ( *Watchdog_Service_routine_entry )(
                 void *
             );

/*
 *  The following record defines the control block used
 *  to manage each watchdog timer.
 */

typedef struct {
  /** This field is a Chain Node structure and allows this to be placed on
   *  chains for set management.
   */
  Chain_Node                      Node;
  /** This field is the initially requested interval. */
  Watchdog_Interval               initial;
  /** This field is the Id to pass as an argument to the routine. */
  Objects_Id                      id;
  /** This field is the state of the watchdog. */
  Watchdog_States                 state;
  /** This field is the remaining portion of the interval. */
  Watchdog_Interval               delta_interval;
  /** This field is the function to invoke. */
  Watchdog_Service_routine_entry  routine;
  /** This field is an untyped pointer to user data that is passed to the
   *  watchdog handler routine.
   */
  void                           *user_data;
}   Watchdog_Control;

/*
 *  _Watchdog_Handler_initialization
 *
 *  DESCRIPTION:
 *
 *  This routine initializes the watchdog handler.  The watchdog
 *  synchronization flag is initialized and the watchdog chains are
 *  initialized and emptied.
 */

void _Watchdog_Handler_initialization( void );

/*
 *  _Watchdog_Insert
 *
 *  DESCRIPTION:
 *
 *  This routine inserts THE_WATCHDOG into the HEADER watchdog chain
 *  for a time of UNITS.  The INSERT_MODE indicates whether
 *  THE_WATCHDOG is to be activated automatically or later, explicitly
 *  by the caller.
 *
 */

void _Watchdog_Insert (
  Chain_Control         *header,
  Watchdog_Control      *the_watchdog
);

/*
 *  _Watchdog_Remove
 *
 *  DESCRIPTION:
 *
 *  This routine removes THE_WATCHDOG from the watchdog chain on which
 *  it resides and returns the state THE_WATCHDOG timer was in.
 */

Watchdog_States _Watchdog_Remove (
  Chain_Control    *header,
  Watchdog_Control *the_watchdog
);

/*
 *  _Watchdog_Tickle
 *
 *  DESCRIPTION:
 *
 *  This routine is invoked at appropriate intervals to update
 *  the HEADER watchdog chain.
 */

void _Watchdog_Tickle (
  Chain_Control *header
);

/*
 *  _Watchdog_Adjust
 *
 *  DESCRIPTION:
 *
 *  This routine adjusts the HEADER watchdog chain in the forward
 *  or backward DIRECTION for UNITS ticks.
 */

void _Watchdog_Adjust (
  Chain_Control              *header,
  Watchdog_Adjust_directions  direction,
  signed64                    units
);

#ifdef __RTEMS_APPLICATION__
#include <libogc/watchdog.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
