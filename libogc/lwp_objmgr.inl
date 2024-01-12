#ifndef __LWP_OBJMGR_INL__
#define __LWP_OBJMGR_INL__

static __inline__ void _Objects_Set_local_object(Objects_Information *information,u32 index,Objects_Control *the_object)
{
	if(index<information->maximum) information->local_table[index] = the_object;
}

static __inline__ void _Objects_Open(Objects_Information *information,Objects_Control *the_object)
{
	_Objects_Set_local_object(information,the_object->id,the_object);
}

static __inline__ void _Objects_Close(Objects_Information *information,Objects_Control *the_object)
{
	_Objects_Set_local_object(information,the_object->id,NULL);
}

#endif
