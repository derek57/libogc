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

u32 __lwp_objmgr_memsize()
{
	return _lwp_objmgr_memsize;
}

void _Objects_Initialize_information(Objects_Information *information,u32 maximum,u32 size)
{
	u32 index,i,sz;
	Objects_Control *the_object;
	Chain_Control inactives;
	void **local_table;
	
	information->minimum_id = 0;
	information->maximum_id = 0;
	information->inactive = 0;
	information->size = size;
	information->maximum = maximum;
	information->object_blocks = NULL;
	information->local_table = &null_local_table;

	_Chain_Initialize_empty(&information->Inactive);

	sz = ((information->maximum*sizeof(Objects_Control*))+(information->maximum*information->size));
	local_table = (void**)_Workspace_Allocate(information->maximum*sizeof(Objects_Control*));
	if(!local_table) return;

	information->local_table = (Objects_Control**)local_table;
	for(i=0;i<information->maximum;i++) {
		local_table[i] = NULL;
	}

	information->object_blocks = _Workspace_Allocate(information->maximum*information->size);
	if(!information->object_blocks) {
		_Workspace_Free(local_table);
		return;
	}

	_Chain_Initialize(&inactives,information->object_blocks,information->maximum,information->size);

	index = information->minimum_id;
	while((the_object=(Objects_Control*)_Chain_Get(&inactives))!=NULL) {
		the_object->id = index;
		the_object->information = NULL;
		_Chain_Append(&information->Inactive,&the_object->Node);
		index++;
	}

	information->maximum_id += information->maximum;
	information->inactive += information->maximum;
	_lwp_objmgr_memsize += sz;
}

Objects_Control* _Objects_Get_isr_disable(Objects_Information *information,u32 id,u32 *level_p)
{
	u32 level;
	Objects_Control *the_object = NULL;

	_ISR_Disable(level);
	if(information->maximum_id>=id) {
		if((the_object=information->local_table[id])!=NULL) {
			*level_p = level;
			return the_object;
		}
	}
	_ISR_Enable(level);
	return NULL;
}

Objects_Control* _Objects_Get_no_protection(Objects_Information *information,u32 id)
{
	Objects_Control *the_object = NULL;

	if(information->maximum_id>=id) {
		if((the_object=information->local_table[id])!=NULL) return the_object;
	}
	return NULL;
}

Objects_Control* _Objects_Get(Objects_Information *information,u32 id)
{
	Objects_Control *the_object = NULL;

	if(information->maximum_id>=id) {
		_Thread_Disable_dispatch();
		if((the_object=information->local_table[id])!=NULL) return the_object;
		_Thread_Enable_dispatch();
	}
	return NULL;
}

Objects_Control* _Objects_Allocate(Objects_Information *information)
{
	u32 level;
	Objects_Control* the_object;

	_ISR_Disable(level);
	 the_object = (Objects_Control*)_Chain_Get_unprotected(&information->Inactive);
	 if(the_object) {
		 the_object->information = information;
		 information->inactive--;
	 }
	_ISR_Enable(level);

	return the_object;
}

void _Objects_Free(Objects_Information *information,Objects_Control *the_object)
{
	u32 level;

	_ISR_Disable(level);
	_Chain_Append_unprotected(&information->Inactive,&the_object->Node);
	the_object->information	= NULL;
	information->inactive++;
	_ISR_Enable(level);
}
