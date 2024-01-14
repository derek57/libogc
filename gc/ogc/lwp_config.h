#ifndef __LWP_CONFIG_H__
#define __LWP_CONFIG_H__


#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES				64

#define CONFIGURE_MAXIMUM_POSIX_MUTEXES				64

#define CONFIGURE_MAXIMUM_POSIX_THREADS				16

#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES				64

#define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES			64

#define CONFIGURE_MAXIMUM_POSIX_QUEUED_SIGNALS				64

#define CONFIGURE_MAXIMUM_POSIX_TIMERS			64

#define unsigned32				u32
#define STATIC					static
#define INLINE					__inline__
#define boolean					BOOL

#define USE_INLINES

#define CPU_HEAP_ALIGNMENT      PPC_ALIGNMENT

#ifdef USE_INLINES
# ifdef __GNUC__
#  define RTEMS_INLINE_ROUTINE  static __inline__
# else
#  define RTEMS_INLINE_ROUTINE  static inline
# endif
#else
# define RTEMS_INLINE_ROUTINE
#endif

#include "address.inl"

#endif
