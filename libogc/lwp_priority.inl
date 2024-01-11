#ifndef __LWP_PRIORITY_INL__
#define __LWP_PRIORITY_INL__

static __inline__ void _Priority_Initialize_information(Priority_Information *theprio,u32 prio)
{
	u32 major,minor,mask;
	
	major = prio/16;
	minor = prio%16;
	
	theprio->minor = &_Priority_Bit_map[major];
	
	mask = 0x80000000>>major;
	theprio->ready_major = mask;
	theprio->block_major = ~mask;
	
	mask = 0x80000000>>minor;
	theprio->ready_minor = mask;
	theprio->block_minor = ~mask;
#ifdef _LWPPRIO_DEBUG
	printf("_Priority_Initialize_information(%p,%d,%p,%d,%d,%d,%d)\n",theprio,prio,theprio->minor,theprio->ready_major,theprio->ready_minor,theprio->block_major,theprio->block_minor);
#endif
}

static __inline__ void _Priority_Add_to_bit_map(Priority_Information *theprio)
{
	*theprio->minor |= theprio->ready_minor;
	_Priority_Major_bit_map |= theprio->ready_major;
}

static __inline__ void _Priority_Remove_from_bit_map(Priority_Information *theprio)
{
	*theprio->minor &= theprio->block_minor;
	if(*theprio->minor==0)
		_Priority_Major_bit_map &= theprio->block_major;
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
