#include <stdlib.h>
#include <string.h>
#include <sys/reent.h>
#include "sys_state.h"
#include "lwp_threads.h"

int libc_reentrant;
struct _reent libc_global_reent;

extern void _wrapup_reent(struct _reent *);
extern void _reclaim_reent(struct _reent *);

int libc_create_hook(Thread_Control *current_task,Thread_Control *creating_task)
{
	creating_task->libc_reent = NULL;
	return 1;
}

int libc_start_hook(Thread_Control *current_task,Thread_Control *starting_task)
{
	struct _reent *ptr;

	ptr = (struct _reent*)calloc(1,sizeof(struct _reent));
	if(!ptr) abort();

#ifdef __GNUC__
	_REENT_INIT_PTR((ptr));
#endif
	
	starting_task->libc_reent = ptr;
	return 1;
}

int libc_delete_hook(Thread_Control *current_task, Thread_Control *deleted_task)
{
	struct _reent *ptr;

	if(current_task==deleted_task)
		ptr = _REENT;
	else
		ptr = (struct _reent*)deleted_task->libc_reent;
	
	if(ptr && ptr!=&libc_global_reent) {
		_wrapup_reent(ptr);
		_reclaim_reent(ptr);
		free(ptr);
	}
	deleted_task->libc_reent = 0;

	if(current_task==deleted_task) _REENT = 0;

	return 1;
}

void libc_init(int reentrant)
{
	libc_global_reent = (struct _reent)_REENT_INIT((libc_global_reent));
	_REENT = &libc_global_reent;

	if(reentrant) {
		_Thread_Set_libc_reent((void*)&_REENT);
		libc_reentrant = reentrant;
	}
}

void libc_wrapup()
{
	if(!_System_state_Is_up(_System_state_Get())) return;
	if(_REENT!=&libc_global_reent) {
		_wrapup_reent(&libc_global_reent);
		_REENT = &libc_global_reent;
	}
}


