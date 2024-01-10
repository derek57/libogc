#ifndef __LWP_OBJMGR_INL__
#define __LWP_OBJMGR_INL__

static __inline__ void _Objects_Set_local_object(Objects_Information *info,u32 idx,Objects_Control *object)
{
	if(idx<info->maximum) info->local_table[idx] = object;
}

static __inline__ void _Objects_Open(Objects_Information *info,Objects_Control *object)
{
	_Objects_Set_local_object(info,object->id,object);
}

static __inline__ void _Objects_Close(Objects_Information *info,Objects_Control *object)
{
	_Objects_Set_local_object(info,object->id,NULL);
}

#endif
