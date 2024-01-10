#ifndef __LWP_OBJMGR_H__
#define __LWP_OBJMGR_H__

#include <gctypes.h>
#include "lwp_queue.h"

#define LWP_OBJMASKTYPE(type)		((type)<<16)
#define LWP_OBJMASKID(id)			((id)&0xffff)
#define LWP_OBJTYPE(id)				((id)>>16)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Objects_Control Objects_Information;

typedef struct {
	Chain_Node Node;
	s32 id;
	Objects_Information *information;
} Objects_Control;

struct _Objects_Control {
	u32 minimum_id;
	u32 maximum_id;
	u32 maximum;
	u32 size;
	Objects_Control **local_table;
	void *object_blocks;
	Chain_Control Inactive;
	u32 inactive;
};

void _Objects_Initialize_information(Objects_Information *info,u32 max_nodes,u32 node_size);
void _Objects_Free(Objects_Information *info,Objects_Control *object);
Objects_Control* _Objects_Allocate(Objects_Information *info);
Objects_Control* _Objects_Get(Objects_Information *info,u32 id);
Objects_Control* _Objects_Get_isr_disable(Objects_Information *info,u32 id,u32 *p_level);
Objects_Control* _Objects_Get_no_protection(Objects_Information *info,u32 id);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_objmgr.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
