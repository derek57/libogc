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

#include "asm.h"

#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES      64

#define CONFIGURE_MAXIMUM_POSIX_MUTEXES             64

#define CONFIGURE_MAXIMUM_POSIX_THREADS             16

#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES          64

#define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES 64

#define CONFIGURE_MAXIMUM_POSIX_QUEUED_SIGNALS      64

#define CONFIGURE_MAXIMUM_POSIX_TIMERS              64

/*
 *  This section defines the basic types for this processor.
 */

typedef unsigned char       unsigned8;  /* unsigned 8-bit  integer */
typedef unsigned short      unsigned16; /* unsigned 16-bit integer */
typedef unsigned int        unsigned32; /* unsigned 32-bit integer */
typedef unsigned long long  unsigned64; /* unsigned 64-bit integer */ 

typedef unsigned32          Priority_Bit_map_control;

typedef signed char         signed8;    /* 8-bit  signed integer */
typedef signed short        signed16;   /* 16-bit signed integer */
typedef signed int          signed32;   /* 32-bit signed integer */
typedef signed long long    signed64;   /* 64 bit signed integer */ 

typedef unsigned32          boolean;    /* Boolean value   */

typedef float               single_precision;     /* single precision float */
typedef double              double_precision;     /* double precision float */ 

/*
 *  The following type defines the control block used to manage a
 *  thread's state.
 */

typedef unsigned32          States_Control;

/*
 *  The following type defines the control block used to manage
 *  intervals.
 */

typedef uint64_t            Watchdog_Interval;

/*
 *  The following ensures that all data is declared in the space
 *  of the initialization routine for either the Initialization Manager
 *  or the initialization file for the appropriate API.  It is 
 *  referenced as "external" in every other file.
 */

#ifdef SCORE_INIT
#undef  SCORE_EXTERN
#define SCORE_EXTERN
#else
#undef  SCORE_EXTERN
#define SCORE_EXTERN        extern
#endif

#ifdef POSIX_API_INIT
#undef  POSIX_EXTERN
#define POSIX_EXTERN
#else
#undef  POSIX_EXTERN
#define POSIX_EXTERN        extern
#endif

/**
 *  The following type defines the control block used to manage
 *  object IDs.  The format is as follows (0=LSB):
 *
 *     Bits  0 ..  7    = index  (up to 254 objects of a type)
 *     Bits  8 .. 10    = API    (up to 7 API classes)
 *     Bits 11 .. 15    = class  (up to 31 object types per API)
 */

typedef unsigned32          Objects_Id;

/**
 * This type is used to store the maximum number of allowed objects
 * of each type.
 */

typedef unsigned32          Objects_Maximum; 

typedef unsigned32          CORE_message_queue_Submit_types; 

/*
 *  The following type defines the control block used to manage
 *  thread priorities.
 *
 *  NOTE: Priority 0 is reserved for internal threads only.
 */

typedef unsigned32          Priority_Control;

/*
 *  The following type defines the control block used to manage
 *  the interrupt level portion of the status register.
 */

typedef unsigned32          ISR_Level;

/*
 *  Application binary interfaces.
 *
 *  PPC_ABI MUST be defined as one of these.
 *  Only PPC_ABI_POWEROPEN is currently fully supported.
 *  Only EABI will be supported in the end when
 *  the tools are there.
 *  Only big endian is currently supported.
 */

/*
 *  PowerOpen ABI.  This is Andy's hack of the
 *  PowerOpen ABI to ELF.  ELF rather than a
 *  XCOFF assembler is used.  This may work
 *  if PPC_ASM == PPC_ASM_XCOFF is defined.
 */
#define PPC_ABI_POWEROPEN	0

/*
 *  GCC 2.7.0 munched version of EABI, with
 *  PowerOpen calling convention and stack frames,
 *  but EABI style indirect function calls.
 */
#define PPC_ABI_GCC27		1

/*
 *  SVR4 ABI
 */
#define PPC_ABI_SVR4		2

/*
 *  Embedded ABI
 */
#define PPC_ABI_EABI		3

/*
 *  Default to the EABI used by current GNU tools
 */

#ifndef PPC_ABI
#define PPC_ABI PPC_ABI_EABI
#endif

#if (PPC_ABI == PPC_ABI_POWEROPEN)
#define PPC_STACK_ALIGNMENT	8
#elif (PPC_ABI == PPC_ABI_GCC27)
#define PPC_STACK_ALIGNMENT	8
#elif (PPC_ABI == PPC_ABI_SVR4)
#define PPC_STACK_ALIGNMENT	16
#elif (PPC_ABI == PPC_ABI_EABI)
#define PPC_STACK_ALIGNMENT	8
#else
#error  "PPC_ABI is not properly defined"
#endif
#ifndef PPC_ABI
#error  "PPC_ABI is not properly defined"
#endif

/*
 *  Bitfield handler macros
 *
 *  These macros perform the following functions:
 *     + scan for the highest numbered (MSB) set in a 16 bit bitfield
 *
 *  NOTE:
 *
 *    It appears that on the M68020 bitfield are always 32 bits wide
 *    when in a register.  This code forces the bitfield to be in
 *    memory (it really always is anyway). This allows us to
 *    have a real 16 bit wide bitfield which operates "correctly."
 */

#define CPU_USE_GENERIC_BITFIELD_CODE   FALSE
#define CPU_USE_GENERIC_BITFIELD_DATA   FALSE

/*
 *  _Bitfield_Find_first_bit
 *
 *  DESCRIPTION:
 *
 *  This routine returns the bit_number of the first bit set
 *  in the specified value.  The correspondence between bit_number
 *  and actual bit position is processor dependent.  The search for
 *  the first bit set may run from most to least significant bit
 *  or vice-versa.
 *
 *  NOTE:
 *
 *  This routine is used when the executing thread is removed
 *  from the ready state and, as a result, its performance has a
 *  significant impact on the performance of the executive as a whole.
 */

#if ( CPU_USE_GENERIC_BITFIELD_DATA == TRUE )

#ifndef SCORE_INIT
extern const unsigned char __log2table[256];
#else
const unsigned char __log2table[256] = {
    7, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
#endif
 
#endif 

/* Bitfield handler macros */

/*
 *  This routine sets _output to the bit number of the first bit
 *  set in _value.  _value is of CPU dependent type Priority_Bit_map_control.
 *  This type may be either 16 or 32 bits wide although only the 16
 *  least significant bits will be used.
 *
 *  There are a number of variables in using a "find first bit" type
 *  instruction.
 *
 *    (1) What happens when run on a value of zero?
 *    (2) Bits may be numbered from MSB to LSB or vice-versa.
 *    (3) The numbering may be zero or one based.
 *    (4) The "find first bit" instruction may search from MSB or LSB.
 *
 *  RTEMS guarantees that (1) will never happen so it is not a concern.
 *  (2),(3), (4) are handled by the macros _CPU_Priority_mask() and
 *  _CPU_Priority_Bits_index().  These three form a set of routines
 *  which must logically operate together.  Bits in the _value are
 *  set and cleared based on masks built by _CPU_Priority_mask().
 *  The basic major and minor values calculated by _Priority_Major()
 *  and _Priority_Minor() are "massaged" by _CPU_Priority_Bits_index()
 *  to properly range between the values returned by the "find first bit"
 *  instruction.  This makes it possible for _Priority_Get_highest() to
 *  calculate the major and directly index into the minor table.
 *  This mapping is necessary to ensure that 0 (a high priority major/minor)
 *  is the first bit found.
 *
 *  This entire "find first bit" and mapping process depends heavily
 *  on the manner in which a priority is broken into a major and minor
 *  components with the major being the 4 MSB of a priority and minor
 *  the 4 LSB.  Thus (0 << 4) + 0 corresponds to priority 0 -- the highest
 *  priority.  And (15 << 4) + 14 corresponds to priority 254 -- the next
 *  to the lowest priority.
 *
 *  If your CPU does not have a "find first bit" instruction, then
 *  there are ways to make do without it.  Here are a handful of ways
 *  to implement this in software:
 *
 *    - a series of 16 bit test instructions
 *    - a "binary search using if's"
 *    - _number = 0
 *      if _value > 0x00ff
 *        _value >>=8
 *        _number = 8;
 *
 *      if _value > 0x0000f
 *        _value >=8
 *        _number += 4
 *
 *      _number += bit_set_table[ _value ]
 *
 *    where bit_set_table[ 16 ] has values which indicate the first
 *      bit set
 */

#define _CPU_Bitfield_Find_first_bit( _value, _output ) \
  { \
    asm volatile ("cntlzw %0, %1" : "=r" ((_output)), "=r" ((_value)) : \
		  "1" ((_value))); \
  }

/* end of Bitfield handler macros */

#if ( CPU_USE_GENERIC_BITFIELD_CODE == FALSE )

#define _Bitfield_Find_first_bit( _value, _bit_number ) \
        _CPU_Bitfield_Find_first_bit( _value, _bit_number )

/*
 *  This routine builds the mask which corresponds to the bit fields
 *  as searched by _CPU_Bitfield_Find_first_bit().  See the discussion
 *  for that routine.
 */

#define _CPU_Priority_Mask( _bit_number ) \
  ( 0x80000000 >> (_bit_number) )

/*
 *  This routine translates the bit numbers returned by
 *  _CPU_Bitfield_Find_first_bit() into something suitable for use as
 *  a major or minor component of a priority.  See the discussion
 *  for that routine.
 */

#define _CPU_Priority_bits_index( _priority ) \
  (_priority)

/* end of Priority handler macros */

#define _Priority_Mask( _bit_number ) \
  _CPU_Priority_Mask( _bit_number )
 
#define _Priority_Bits_index( _priority ) \
  _CPU_Priority_bits_index( _priority )

#else

/*
 *  The following must be a macro because if a CPU specific version
 *  is used it will most likely use inline assembly.
 */

#define _Bitfield_Find_first_bit( _value, _bit_number ) \
  { \
    register unsigned32 __value = (unsigned32) (_value); \
    register const unsigned char *__p = __log2table; \
    \
    if ( __value < 0x100 ) \
      (_bit_number) = __p[ __value ] + 8; \
    else \
      (_bit_number) = __p[ __value >> 8 ]; \
  }

#endif

#define RTEMS_POSIX_API
#define USE_INLINES
#define STATIC              static
#define INLINE              __inline__

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
 *  to choose between the use of RTEMS_INLINE_ROUTINE functions and macro
 *  functions.   The RTEMS_INLINE_ROUTINE implementation allows better
 *  type checking with no cost in code size or execution speed.
 */

/*
 *  The following is the variable attribute used to force alignment
 *  of critical RTEMS structures.  On some processors it may make
 *  sense to have these aligned on tighter boundaries than
 *  the minimum requirements of the compiler in order to have as
 *  much of the critical data area as possible in a cache line.
 *
 *  The placement of this macro in the declaration of the variables
 *  is based on the syntactically requirements of the GNU C
 *  "__attribute__" extension.  For example with GNU C, use
 *  the following to force a structures to a 32 byte boundary.
 *
 *      __attribute__ ((aligned (32)))
 *
 *  NOTE:  Currently only the Priority Bit Map table uses this feature.
 *         To benefit from using this, the data must be heavily
 *         used so it will stay in the cache and used frequently enough
 *         in the executive to justify turning this on.
 */

#define CPU_STRUCTURE_ALIGNMENT \
  __attribute__ ((aligned (PPC_CACHE_ALIGNMENT)))

/* conditional compilation parameters */

/*
 *  Should the calls to _Thread_Enable_dispatch be inlined?
 *
 *  If TRUE, then they are inlined.
 *  If FALSE, then a subroutine call is made.
 *
 *  Basically this is an example of the classic trade-off of size
 *  versus speed.  Inlining the call (TRUE) typically increases the
 *  size of RTEMS while speeding up the enabling of dispatching.
 *  [NOTE: In general, the _Thread_Dispatch_disable_level will
 *  only be 0 or 1 unless you are in an interrupt handler and that
 *  interrupt handler invokes the executive.]  When not inlined
 *  something calls _Thread_Enable_dispatch which in turns calls
 *  _Thread_Dispatch.  If the enable dispatch is inlined, then
 *  one subroutine call is avoided entirely.]
 */

#define CPU_INLINE_ENABLE_DISPATCH  TRUE

/*
 *  Unless specified above, then assume the model has FP support.
 */

#ifndef PPC_HAS_FPU
#define PPC_HAS_FPU         1
#endif

/*
 *  Does the CPU have hardware floating point?
 *
 *  If TRUE, then the RTEMS_FLOATING_POINT task attribute is supported.
 *  If FALSE, then the RTEMS_FLOATING_POINT task attribute is ignored.
 *
 *  If there is a FP coprocessor such as the i387 or mc68881, then
 *  the answer is TRUE.
 *
 *  The macro name "PPC_HAS_FPU" should be made CPU specific.
 *  It indicates whether or not this CPU model has FP support.  For
 *  example, it would be possible to have an i386_nofp CPU model
 *  which set this to false to indicate that you have an i386 without
 *  an i387 and wish to leave floating point support out of RTEMS.
 */

#if ( PPC_HAS_FPU == 1 )
#define CPU_HARDWARE_FP     TRUE
#else
#define CPU_HARDWARE_FP     FALSE
#endif

/*
 *  Initial value for the FPSCR register
 */

#define PPC_INIT_FPSCR		0x000000f8

/*
 *  The following (in conjunction with compiler arguments) are used
 *  to choose between the use of RTEMS_INLINE_ROUTINE functions and macro
 *  functions.   The RTEMS_INLINE_ROUTINE implementation allows better
 *  type checking with no cost in code size or execution speed.
 */

#ifdef USE_INLINES
# ifdef __GNUC__
#  define RTEMS_INLINE_ROUTINE  static __inline__
# else
#  define RTEMS_INLINE_ROUTINE  RTEMS_INLINE_ROUTINE
# endif
#else
# define RTEMS_INLINE_ROUTINE
#endif

#endif
