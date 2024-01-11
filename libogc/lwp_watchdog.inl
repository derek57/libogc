#ifndef __LWP_WATCHDOG_INL__
#define __LWP_WATCHDOG_INL__

static __inline__ void _Watchdog_Initialize(Watchdog_Control *wd,Watchdog_Service_routine_entry routine,u32 id,void *usr_data)
{
	wd->state = LWP_WD_INACTIVE;
	wd->id = id;
	wd->routine = routine;
	wd->user_data = usr_data;
}

static __inline__ Watchdog_Control* _Watchdog_First(Chain_Control *queue)
{
	return (Watchdog_Control*)queue->first;
}

static __inline__ Watchdog_Control* _Watchdog_Last(Chain_Control *queue)
{
	return (Watchdog_Control*)queue->last;
}

static __inline__ Watchdog_Control* _Watchdog_Next(Watchdog_Control *wd)
{
	return (Watchdog_Control*)wd->node.next;
}

static __inline__ Watchdog_Control* _Watchdog_Previous(Watchdog_Control *wd)
{
	return (Watchdog_Control*)wd->node.previous;
}

static __inline__ void _Watchdog_Activate(Watchdog_Control *wd)
{
	wd->state = LWP_WD_ACTIVE;
}

static __inline__ void _Watchdog_Deactivate(Watchdog_Control *wd)
{
	wd->state = LWP_WD_REMOVE;
}

static __inline__ u32 _Watchdog_Is_active(Watchdog_Control *wd)
{
	return (wd->state==LWP_WD_ACTIVE);
}

static __inline__ u64 _POSIX_Timespec_to_interval(const struct timespec *time)
{
	u64 ticks;

	ticks = secs_to_ticks(time->tv_sec);
	ticks += nanosecs_to_ticks(time->tv_nsec);

	return ticks;
}

static __inline__ void _Watchdog_Tickle_ticks()
{
	_Watchdog_Tickle(&_Watchdog_Ticks_chain);
}

static __inline__ void _Watchdog_Insert_ticks(Watchdog_Control *wd,s64 interval)
{
	wd->initial = gettime();
	wd->delta_interval = (wd->initial+LWP_WD_ABS(interval));
	_Watchdog_Insert(&_Watchdog_Ticks_chain,wd);
}

static __inline__ void _Watchdog_Adjust_ticks(u32 dir,s64 interval)
{
	_Watchdog_Adjust(&_Watchdog_Ticks_chain,dir,interval);
}

static __inline__ void _Watchdog_Remove_ticks(Watchdog_Control *wd)
{
	_Watchdog_Remove(&_Watchdog_Ticks_chain,wd);
}

static __inline__ void _Watchdog_Reset(Watchdog_Control *wd)
{
	_Watchdog_Remove(&_Watchdog_Ticks_chain,wd);
	_Watchdog_Insert(&_Watchdog_Ticks_chain,wd);
}
#endif
