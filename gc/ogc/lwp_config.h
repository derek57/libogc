/*  system.h
 *
 *  This include file contains information that is included in every
 *  function in the executive.  This must be the first include file
 *  included in all internal RTEMS files.
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
 
#ifndef __LWP_CONFIG_H__
#define __LWP_CONFIG_H__


#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES      64

#define CONFIGURE_MAXIMUM_POSIX_MUTEXES             64

#define CONFIGURE_MAXIMUM_POSIX_THREADS             16

#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES          64

#define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES 64

#define CONFIGURE_MAXIMUM_POSIX_QUEUED_SIGNALS      64

#define CONFIGURE_MAXIMUM_POSIX_TIMERS              64

typedef unsigned int        unsigned32;
typedef unsigned short      unsigned16;
typedef unsigned char       unsigned8;

/*
 *  The following type defines the control block used to manage
 *  intervals.
 */

typedef uint64_t            Watchdog_Interval;

/*
 *  The following type defines the control block used to manage
 *  thread priorities.
 *
 *  NOTE: Priority 0 is reserved for internal threads only.
 */

typedef u8                  Priority_Control;

/*
 *  The following type defines the control block used to manage
 *  the interrupt level portion of the status register.
 */

typedef unsigned32          ISR_Level;

#define RTEMS_POSIX_API
#define USE_INLINES
#define STATIC              static
#define INLINE              __inline__

typedef unsigned int        boolean;

/*
 *  This number corresponds to the byte alignment requirement for the
 *  heap handler.  This alignment requirement may be stricter than that
 *  for the data types alignment specified by CPU_ALIGNMENT.  It is
 *  common for the heap to follow the same alignment requirement as
 *  CPU_ALIGNMENT.  If the CPU_ALIGNMENT is strict enough for the heap,
 *  then this should be set to CPU_ALIGNMENT.
 *
 *  NOTE:  This does not have to be a power of 2.  It does have to
 *         be greater or equal to than CPU_ALIGNMENT.
 */

#define CPU_HEAP_ALIGNMENT      PPC_ALIGNMENT

/*
 *  The following (in conjunction with compiler arguments) are used
 *  to choose between the use of static inline functions and macro
 *  functions.   The static inline implementation allows better
 *  type checking with no cost in code size or execution speed.
 */

#ifdef USE_INLINES
# ifdef __GNUC__
#  define RTEMS_INLINE_ROUTINE  static __inline__
# else
#  define RTEMS_INLINE_ROUTINE  static inline
# endif
#else
# define RTEMS_INLINE_ROUTINE
#endif

#endif
