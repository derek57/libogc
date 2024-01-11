#include <stdlib.h>
#include "lwp_stack.h"
#include "lwp_wkspace.h"

u32 _Thread_Stack_Allocate(Thread_Control *the_thread,u32 stack_size)
{
	void *stack_addr = NULL;

	if(!_Stack_Is_enough(stack_size))
		stack_size = CPU_MINIMUM_STACK_SIZE;
	
	stack_size = _Stack_Adjust_size(stack_size);
	stack_addr = _Workspace_Allocate(stack_size);

	if(!stack_addr) stack_size = 0;

	the_thread->stack = stack_addr;
	return stack_size;
}

void _Thread_Stack_Free(Thread_Control *the_thread)
{
	if(!the_thread->core_allocated_stack)
		return;

	_Workspace_Free(the_thread->stack);
}
