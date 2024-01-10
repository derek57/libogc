#include <stdlib.h>
#include "lwp_stack.h"
#include "lwp_wkspace.h"

u32 _Thread_Stack_Allocate(Thread_Control *thethread,u32 size)
{
	void *stack_addr = NULL;

	if(!_Stack_Is_enough(size))
		size = CPU_MINIMUM_STACK_SIZE;
	
	size = _Stack_Adjust_size(size);
	stack_addr = _Workspace_Allocate(size);

	if(!stack_addr) size = 0;

	thethread->stack = stack_addr;
	return size;
}

void _Thread_Stack_Free(Thread_Control *thethread)
{
	if(!thethread->core_allocated_stack)
		return;

	_Workspace_Free(thethread->stack);
}
