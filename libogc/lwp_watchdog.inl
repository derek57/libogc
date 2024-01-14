#ifndef __LWP_WATCHDOG_INL__
#define __LWP_WATCHDOG_INL__

RTEMS_INLINE_ROUTINE void _Watchdog_Initialize(Watchdog_Control *the_watchdog,Watchdog_Service_routine_entry routine,u32 id,void *user_data)
{
	the_watchdog->state = WATCHDOG_INACTIVE;
	the_watchdog->id = id;
	the_watchdog->routine = routine;
	the_watchdog->user_data = user_data;
}

RTEMS_INLINE_ROUTINE Watchdog_Control* _Watchdog_First(Chain_Control *queue)
{
	return (Watchdog_Control*)queue->first;
}

RTEMS_INLINE_ROUTINE Watchdog_Control* _Watchdog_Last(Chain_Control *queue)
{
	return (Watchdog_Control*)queue->last;
}

RTEMS_INLINE_ROUTINE Watchdog_Control* _Watchdog_Next(Watchdog_Control *the_watchdog)
{
	return (Watchdog_Control*)the_watchdog->node.next;
}

RTEMS_INLINE_ROUTINE Watchdog_Control* _Watchdog_Previous(Watchdog_Control *the_watchdog)
{
	return (Watchdog_Control*)the_watchdog->node.previous;
}

RTEMS_INLINE_ROUTINE void _Watchdog_Activate(Watchdog_Control *the_watchdog)
{
	the_watchdog->state = WATCHDOG_ACTIVE;
}

RTEMS_INLINE_ROUTINE void _Watchdog_Deactivate(Watchdog_Control *the_watchdog)
{
	the_watchdog->state = WATCHDOG_REMOVE_IT;
}

RTEMS_INLINE_ROUTINE u32 _Watchdog_Is_active(Watchdog_Control *the_watchdog)
{
	return (the_watchdog->state==WATCHDOG_ACTIVE);
}

RTEMS_INLINE_ROUTINE u64 _POSIX_Timespec_to_interval(const struct timespec *time)
{
	u64 ticks;

	ticks = secs_to_ticks(time->tv_sec);
	ticks += nanosecs_to_ticks(time->tv_nsec);

	return ticks;
}

RTEMS_INLINE_ROUTINE void _Watchdog_Tickle_ticks()
{
	_Watchdog_Tickle(&_Watchdog_Ticks_chain);
}

RTEMS_INLINE_ROUTINE void _Watchdog_Insert_ticks(Watchdog_Control *the_watchdog,s64 units)
{
	the_watchdog->initial = gettime();
	the_watchdog->delta_interval = (the_watchdog->initial+LWP_WD_ABS(units));
	_Watchdog_Insert(&_Watchdog_Ticks_chain,the_watchdog);
}

RTEMS_INLINE_ROUTINE void _Watchdog_Adjust_ticks(u32 dir,s64 units)
{
	_Watchdog_Adjust(&_Watchdog_Ticks_chain,dir,units);
}

RTEMS_INLINE_ROUTINE void _Watchdog_Remove_ticks(Watchdog_Control *the_watchdog)
{
	_Watchdog_Remove(&_Watchdog_Ticks_chain,the_watchdog);
}

RTEMS_INLINE_ROUTINE void _Watchdog_Reset(Watchdog_Control *the_watchdog)
{
	_Watchdog_Remove(&_Watchdog_Ticks_chain,the_watchdog);
	_Watchdog_Insert(&_Watchdog_Ticks_chain,the_watchdog);
}
#endif
