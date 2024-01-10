#ifndef __LWP_PRIORITY_H__
#define __LWP_PRIORITY_H__

#include <gctypes.h>
#include "machine/processor.h"

#define LWP_PRIO_MIN				0
#define LWP_PRIO_MAX				255

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	u32 *minor;
	u32 ready_minor,ready_major;
	u32 block_minor,block_major;
} Priority_Information;

extern vu32 _prio_major_bitmap;
extern u32 _prio_bitmap[];

void _Priority_Handler_initialization();

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_priority.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
