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
	
	info->min_id = 0;
	info->max_id = 0;
	info->inactives_cnt = 0;
	info->node_size = node_size;
	info->max_nodes = max_nodes;
	info->obj_blocks = NULL;
	info->local_table = &null_local_table;

	_Chain_Initialize_empty(&info->inactives);

	size = ((info->max_nodes*sizeof(Objects_Control*))+(info->max_nodes*info->node_size));
	local_table = (void**)_Workspace_Allocate(info->max_nodes*sizeof(Objects_Control*));
	if(!local_table) return;

	info->local_table = (Objects_Control**)local_table;
	for(i=0;i<info->max_nodes;i++) {
		local_table[i] = NULL;
	}

	info->obj_blocks = _Workspace_Allocate(info->max_nodes*info->node_size);
	if(!info->obj_blocks) {
		_Workspace_Free(local_table);
		return;
	}

	_Chain_Initialize(&inactives,info->obj_blocks,info->max_nodes,info->node_size);

	idx = info->min_id;
	while((object=(Objects_Control*)_Chain_Get(&inactives))!=NULL) {
		object->id = idx;
		object->information = NULL;
		_Chain_Append(&info->inactives,&object->node);
		idx++;
	}

	info->max_id += info->max_nodes;
	info->inactives_cnt += info->max_nodes;
	_lwp_objmgr_memsize += size;
}

Objects_Control* _Objects_Get_isr_disable(Objects_Information *info,u32 id,u32 *p_level)
{
	u32 level;
	Objects_Control *object = NULL;

	_CPU_ISR_Disable(level);
	if(info->max_id>=id) {
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

	if(info->max_id>=id) {
		if((object=info->local_table[id])!=NULL) return object;
	}
	return NULL;
}

Objects_Control* _Objects_Get(Objects_Information *info,u32 id)
{
	Objects_Control *object = NULL;

	if(info->max_id>=id) {
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
	 object = (Objects_Control*)_Chain_Get_unprotected(&info->inactives);
	 if(object) {
		 object->information = info;
		 info->inactives_cnt--;
	 }
	_CPU_ISR_Restore(level);

	return object;
}

void _Objects_Free(Objects_Information *info,Objects_Control *object)
{
	u32 level;

	_CPU_ISR_Disable(level);
	_Chain_Append_unprotected(&info->inactives,&object->node);
	object->information	= NULL;
	info->inactives_cnt++;
	_CPU_ISR_Restore(level);
}
