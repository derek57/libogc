#ifndef __LWP_STACK_INL__
#define __LWP_STACK_INL__

RTEMS_INLINE_ROUTINE u32 _Stack_Is_enough(u32 size)
{
	return (size>=CPU_MINIMUM_STACK_SIZE);
}

RTEMS_INLINE_ROUTINE u32 _Stack_Adjust_size(u32 size)
{
	return (size+CPU_STACK_ALIGNMENT);
}

#endif
