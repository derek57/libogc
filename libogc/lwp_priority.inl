#ifndef __LWP_PRIORITY_INL__
#define __LWP_PRIORITY_INL__

static __inline__ void _Priority_Initialize_information(Priority_Information *the_priority_map,u32 new_priority)
{
	u32 major,minor,mask;
	
	major = new_priority/16;
	minor = new_priority%16;
	
	the_priority_map->minor = &_Priority_Bit_map[major];
	
	mask = 0x80000000>>major;
	the_priority_map->ready_major = mask;
	the_priority_map->block_major = ~mask;
	
	mask = 0x80000000>>minor;
	the_priority_map->ready_minor = mask;
	the_priority_map->block_minor = ~mask;
#ifdef _LWPPRIO_DEBUG
	printf("_Priority_Initialize_information(%p,%d,%p,%d,%d,%d,%d)\n",the_priority_map,new_priority,the_priority_map->minor,the_priority_map->ready_major,the_priority_map->ready_minor,the_priority_map->block_major,the_priority_map->block_minor);
#endif
}

static __inline__ void _Priority_Add_to_bit_map(Priority_Information *the_priority_map)
{
	*the_priority_map->minor |= the_priority_map->ready_minor;
	_Priority_Major_bit_map |= the_priority_map->ready_major;
}

static __inline__ void _Priority_Remove_from_bit_map(Priority_Information *the_priority_map)
{
	*the_priority_map->minor &= the_priority_map->block_minor;
	if(*the_priority_map->minor==0)
		_Priority_Major_bit_map &= the_priority_map->block_major;
}

static __inline__ u32 _Priority_Get_highest()
{
	u32 major,minor;
	major = cntlzw(_Priority_Major_bit_map);
	minor = cntlzw(_Priority_Bit_map[major]);
#ifdef _LWPPRIO_DEBUG
	printf("_Priority_Get_highest(%d)\n",((major<<4)+minor));
#endif
	return ((major<<4)+minor);
}

#endif
