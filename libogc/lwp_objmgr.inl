#ifndef __LWP_OBJMGR_INL__
#define __LWP_OBJMGR_INL__

static __inline__ void _Objects_Set_local_object(lwp_objinfo *info,u32 idx,lwp_obj *object)
{
	if(idx<info->max_nodes) info->local_table[idx] = object;
}

static __inline__ void _Objects_Open(lwp_objinfo *info,lwp_obj *object)
{
	_Objects_Set_local_object(info,object->id,object);
}

static __inline__ void _Objects_Close(lwp_objinfo *info,lwp_obj *object)
{
	_Objects_Set_local_object(info,object->id,NULL);
}

#endif
