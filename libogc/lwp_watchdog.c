#include <stdlib.h>
#include <limits.h>
#include "asm.h"
#include "lwp_threads.h"
#include "lwp_watchdog.h"

//#define _LWPWD_DEBUG

#ifdef _LWPWD_DEBUG
#include <stdio.h>
#endif

vu32 _Watchdog_Sync_level;
vu32 _Watchdog_Sync_count;
u32 _Watchdog_Ticks_since_boot;

Chain_Control _Watchdog_Ticks_chain;

static void __lwp_wd_settimer(Watchdog_Control *wd)
{
	u64 now;
	s64 diff;
	union uulc {
		u64 ull;
		u32 ul[2];
	} v;

	now = gettime();
	v.ull = diff = diff_ticks(now,wd->delta_interval);
#ifdef _LWPWD_DEBUG
	printf("__lwp_wd_settimer(%p,%llu,%lld)\n",wd,wd->fire,diff);
#endif
	if(diff<=0) {
#ifdef _LWPWD_DEBUG
		printf(" __lwp_wd_settimer(0): %lld<=0\n",diff);
#endif
		wd->delta_interval = 0;
		mtdec(0);
	} else if(diff<0x0000000080000000LL) {
#ifdef _LWPWD_DEBUG
		printf("__lwp_wd_settimer(%d): %lld<0x0000000080000000LL\n",v.ul[1],diff);
#endif
		mtdec(v.ul[1]);
	} else {
#ifdef _LWPWD_DEBUG
		printf("__lwp_wd_settimer(0x7fffffff)\n");
#endif
		mtdec(0x7fffffff);
	}
}

void _Watchdog_Handler_initialization()
{
	_Watchdog_Sync_level = 0;
	_Watchdog_Sync_count = 0;
	_Watchdog_Ticks_since_boot = 0;

	_Chain_Initialize_empty(&_Watchdog_Ticks_chain);
}

void _Watchdog_Insert(Chain_Control *header,Watchdog_Control *the_watchdog)
{
	u32 level;
	u64 delta_interval;
	u32 insert_isr_nest_level;
	Watchdog_Control *after;
#ifdef _LWPWD_DEBUG
	printf("_Watchdog_Insert(%p,%llu,%llu)\n",the_watchdog,the_watchdog->start,the_watchdog->delta_interval);
#endif
	insert_isr_nest_level = _ISR_Is_in_progress();
	the_watchdog->state = WATCHDOG_BEING_INSERTED;

	_Watchdog_Sync_count++;
restart:
	_CPU_ISR_Disable(level);
	delta_interval = the_watchdog->delta_interval;
	for(after=_Watchdog_First(header);;after=_Watchdog_Next(after)) {
		if(delta_interval==0 || !_Watchdog_Next(after)) break;
		if(delta_interval<after->delta_interval) break;

		_CPU_ISR_Flash(level);
		if(the_watchdog->state!=WATCHDOG_BEING_INSERTED) goto exit_insert;
		if(_Watchdog_Sync_level>insert_isr_nest_level) {
			_Watchdog_Sync_level = insert_isr_nest_level;
			_CPU_ISR_Restore(level);
			goto restart;
		}
	}
	_Watchdog_Activate(the_watchdog);
	the_watchdog->delta_interval = delta_interval;
	_Chain_Insert_unprotected(after->node.previous,&the_watchdog->node);
	if(_Watchdog_First(header)==the_watchdog) __lwp_wd_settimer(the_watchdog);

exit_insert:
	_Watchdog_Sync_level = insert_isr_nest_level;
	_Watchdog_Sync_count--;
	_CPU_ISR_Restore(level);
	return;
}

u32 _Watchdog_Remove(Chain_Control *header,Watchdog_Control *the_watchdog)
{
	u32 level;
	u32 previous_state;
	Watchdog_Control *next_watchdog;
#ifdef _LWPWD_DEBUG
	printf("_Watchdog_Remove(%p)\n",the_watchdog);
#endif
	_CPU_ISR_Disable(level);
	previous_state = the_watchdog->state;
	switch(previous_state) {
		case WATCHDOG_INACTIVE:
			break;
		case  WATCHDOG_BEING_INSERTED:
			the_watchdog->state = WATCHDOG_INACTIVE;
			break;
		case WATCHDOG_ACTIVE:
		case WATCHDOG_REMOVE_IT:
			the_watchdog->state = WATCHDOG_INACTIVE;
			next_watchdog = _Watchdog_Next(the_watchdog);
			if(_Watchdog_Sync_count) _Watchdog_Sync_level = _ISR_Is_in_progress();
			_Chain_Extract_unprotected(&the_watchdog->node);
			if(!_Chain_Is_empty(header) && _Watchdog_First(header)==next_watchdog) __lwp_wd_settimer(next_watchdog);
			break;
	}
	_CPU_ISR_Restore(level);
	return previous_state;
}

void _Watchdog_Tickle(Chain_Control *header)
{
	Watchdog_Control *the_watchdog;
	u64 now;
	s64 diff;

	if(_Chain_Is_empty(header)) return;

	the_watchdog = _Watchdog_First(header);
	now = gettime();
	diff = diff_ticks(now,the_watchdog->delta_interval);
#ifdef _LWPWD_DEBUG
	printf("_Watchdog_Tickle(%p,%08x%08x,%08x%08x,%08x%08x,%08x%08x)\n",the_watchdog,(u32)(now>>32),(u32)now,(u32)(the_watchdog->start>>32),(u32)the_watchdog->start,(u32)(the_watchdog->fire>>32),(u32)the_watchdog->fire,(u32)(diff>>32),(u32)diff);
#endif
	if(diff<=0) {
		do {
			switch(_Watchdog_Remove(header,the_watchdog)) {
				case WATCHDOG_ACTIVE:	
					the_watchdog->routine(the_watchdog->user_data);
					break;
				case WATCHDOG_INACTIVE:
					break;
				case WATCHDOG_BEING_INSERTED:
					break;
				case WATCHDOG_REMOVE_IT:
					break;
			}
			the_watchdog = _Watchdog_First(header);
		} while(!_Chain_Is_empty(header) && the_watchdog->delta_interval==0);
	} else {
		_Watchdog_Reset(the_watchdog);
	}
}

void _Watchdog_Adjust(Chain_Control *header,u32 direction,s64 units)
{
	u32 level;
	u64 abs_int;

	_CPU_ISR_Disable(level);
	abs_int = gettime()+LWP_WD_ABS(units);
	if(!_Chain_Is_empty(header)) {
		switch(direction) {
			case WATCHDOG_BACKWARD:
				_Watchdog_First(header)->delta_interval += LWP_WD_ABS(units);
				break;
			case WATCHDOG_FORWARD:
				while(abs_int) {
					if(abs_int<_Watchdog_First(header)->delta_interval) {
						_Watchdog_First(header)->delta_interval -= LWP_WD_ABS(units);
						break;
					} else {
						abs_int -= _Watchdog_First(header)->delta_interval;
						_Watchdog_First(header)->delta_interval = gettime();
						_Watchdog_Tickle(header);
						if(_Chain_Is_empty(header)) break;
					}
				}
				break;
		}
	}
	_CPU_ISR_Restore(level);
}
