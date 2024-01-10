#ifndef __LWP_WATCHDOG_INL__
#define __LWP_WATCHDOG_INL__

static __inline__ void _Watchdog_Initialize(Watchdog_Control *wd,wd_service_routine routine,u32 id,void *usr_data)
{
	wd->state = LWP_WD_INACTIVE;
	wd->id = id;
	wd->routine = routine;
	wd->usr_data = usr_data;
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
	return (Watchdog_Control*)wd->node.prev;
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
	_Watchdog_Tickle(&_wd_ticks_queue);
}

static __inline__ void _Watchdog_Insert_ticks(Watchdog_Control *wd,s64 interval)
{
	wd->start = gettime();
	wd->fire = (wd->start+LWP_WD_ABS(interval));
	_Watchdog_Insert(&_wd_ticks_queue,wd);
}

static __inline__ void _Watchdog_Adjust_ticks(u32 dir,s64 interval)
{
	_Watchdog_Adjust(&_wd_ticks_queue,dir,interval);
}

static __inline__ void _Watchdog_Remove_ticks(Watchdog_Control *wd)
{
	_Watchdog_Remove(&_wd_ticks_queue,wd);
}

static __inline__ void _Watchdog_Reset(Watchdog_Control *wd)
{
	_Watchdog_Remove(&_wd_ticks_queue,wd);
	_Watchdog_Insert(&_wd_ticks_queue,wd);
}
#endif
