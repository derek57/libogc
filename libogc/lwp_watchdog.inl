#ifndef __LWP_WATCHDOG_INL__
#define __LWP_WATCHDOG_INL__

static __inline__ void _Watchdog_Initialize(wd_cntrl *wd,wd_service_routine routine,u32 id,void *usr_data)
{
	wd->state = LWP_WD_INACTIVE;
	wd->id = id;
	wd->routine = routine;
	wd->usr_data = usr_data;
}

static __inline__ wd_cntrl* _Watchdog_First(lwp_queue *queue)
{
	return (wd_cntrl*)queue->first;
}

static __inline__ wd_cntrl* _Watchdog_Last(lwp_queue *queue)
{
	return (wd_cntrl*)queue->last;
}

static __inline__ wd_cntrl* _Watchdog_Next(wd_cntrl *wd)
{
	return (wd_cntrl*)wd->node.next;
}

static __inline__ wd_cntrl* _Watchdog_Previous(wd_cntrl *wd)
{
	return (wd_cntrl*)wd->node.prev;
}

static __inline__ void _Watchdog_Activate(wd_cntrl *wd)
{
	wd->state = LWP_WD_ACTIVE;
}

static __inline__ void _Watchdog_Deactivate(wd_cntrl *wd)
{
	wd->state = LWP_WD_REMOVE;
}

static __inline__ u32 _Watchdog_Is_active(wd_cntrl *wd)
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

static __inline__ void _Watchdog_Insert_ticks(wd_cntrl *wd,s64 interval)
{
	wd->start = gettime();
	wd->fire = (wd->start+LWP_WD_ABS(interval));
	_Watchdog_Insert(&_wd_ticks_queue,wd);
}

static __inline__ void _Watchdog_Adjust_ticks(u32 dir,s64 interval)
{
	_Watchdog_Adjust(&_wd_ticks_queue,dir,interval);
}

static __inline__ void _Watchdog_Remove_ticks(wd_cntrl *wd)
{
	_Watchdog_Remove(&_wd_ticks_queue,wd);
}

static __inline__ void _Watchdog_Reset(wd_cntrl *wd)
{
	_Watchdog_Remove(&_wd_ticks_queue,wd);
	_Watchdog_Insert(&_wd_ticks_queue,wd);
}
#endif
