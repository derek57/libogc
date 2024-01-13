#ifndef __SYS_STATE_H__
#define __SYS_STATE_H__

typedef enum {
  SYSTEM_STATE_BEFORE_INITIALIZATION,   /* start -> end of 1st init part */
  SYSTEM_STATE_BEFORE_MULTITASKING,     /* end of 1st -> beginning of 2nd */
  SYSTEM_STATE_BEGIN_MULTITASKING,      /* just before multitasking starts */
  SYSTEM_STATE_UP,                      /* normal operation */
  SYSTEM_STATE_SHUTDOWN,                /* shutdown */
  SYSTEM_STATE_FAILED                   /* fatal error occurred */
} System_state_Codes; 

#include <gctypes.h>

#ifdef __cplusplus
extern "C" {
#endif

extern u32 _System_state_Current;

#ifdef LIBOGC_INTERNAL
#include <libogc/sys_state.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
