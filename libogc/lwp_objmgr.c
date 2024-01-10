#include <processor.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lwp_threads.h>
#include <lwp_wkspace.h>
#include <lwp_config.h>

#include "lwp_objmgr.h"

static u32 _lwp_objmgr_memsize = 0;
static Objects_Control *null_local_table = NULL;

void _Objects_Initialize_information(Objects_Information *info,u32 max_nodes,u32 node_size)
{
	u32 idx,i,size;
	Objects_Control *object;
	Chain_Control inactives;
	void **local_table;
	
	info->minimum_id = 0;
	info->maximum_id = 0;
	info->inactive = 0;
	info->size = node_size;
	info->maximum = max_nodes;
	info->object_blocks = NULL;
	info->local_table = &null_local_table;

	_Chain_Initialize_empty(&info->Inactive);

	size = ((info->maximum*sizeof(Objects_Control*))+(info->maximum*info->size));
	local_table = (void**)_Workspace_Allocate(info->maximum*sizeof(Objects_Control*));
	if(!local_table) return;

	info->local_table = (Objects_Control**)local_table;
	for(i=0;i<info->maximum;i++) {
		local_table[i] = NULL;
	}

	info->object_blocks = _Workspace_Allocate(info->maximum*info->size);
	if(!info->object_blocks) {
		_Workspace_Free(local_table);
		return;
	}

	_Chain_Initialize(&inactives,info->object_blocks,info->maximum,info->size);

	idx = info->minimum_id;
	while((object=(Objects_Control*)_Chain_Get(&inactives))!=NULL) {
		object->id = idx;
		object->information = NULL;
		_Chain_Append(&info->Inactive,&object->Node);
		idx++;
	}

	info->maximum_id += info->maximum;
	info->inactive += info->maximum;
	_lwp_objmgr_memsize += size;
}

Objects_Control* _Objects_Get_isr_disable(Objects_Information *info,u32 id,u32 *p_level)
{
	u32 level;
	Objects_Control *object = NULL;

	_CPU_ISR_Disable(level);
	if(info->maximum_id>=id) {
		if((object=info->local_table[id])!=NULL) {
			*p_level = level;
			return object;
		}
	}
	_CPU_ISR_Restore(level);
	return NULL;
}

Objects_Control* _Objects_Get_no_protection(Objects_Information *info,u32 id)
{
	Objects_Control *object = NULL;

	if(info->maximum_id>=id) {
		if((object=info->local_table[id])!=NULL) return object;
	}
	return NULL;
}

Objects_Control* _Objects_Get(Objects_Information *info,u32 id)
{
	Objects_Control *object = NULL;

	if(info->maximum_id>=id) {
		_Thread_Disable_dispatch();
		if((object=info->local_table[id])!=NULL) return object;
		_Thread_Enable_dispatch();
	}
	return NULL;
}

Objects_Control* _Objects_Allocate(Objects_Information *info)
{
	u32 level;
	Objects_Control* object;

	_CPU_ISR_Disable(level);
	 object = (Objects_Control*)_Chain_Get_unprotected(&info->Inactive);
	 if(object) {
		 object->information = info;
		 info->inactive--;
	 }
	_CPU_ISR_Restore(level);

	return object;
}

void _Objects_Free(Objects_Information *info,Objects_Control *object)
{
	u32 level;

	_CPU_ISR_Disable(level);
	_Chain_Append_unprotected(&info->Inactive,&object->Node);
	object->information	= NULL;
	info->inactive++;
	_CPU_ISR_Restore(level);
}
