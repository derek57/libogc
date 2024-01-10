#include <stdlib.h>
#include <limits.h>
#include "asm.h"
#include "lwp_threads.h"
#include "lwp_watchdog.h"

//#define _LWPWD_DEBUG

#ifdef _LWPWD_DEBUG
#include <stdio.h>
#endif

vu32 _wd_sync_level;
vu32 _wd_sync_count;
u32 _wd_ticks_since_boot;

Chain_Control _wd_ticks_queue;

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
	_wd_sync_level = 0;
	_wd_sync_count = 0;
	_wd_ticks_since_boot = 0;

	_Chain_Initialize_empty(&_wd_ticks_queue);
}

void _Watchdog_Insert(Chain_Control *header,Watchdog_Control *wd)
{
	u32 level;
	u64 fire;
	u32 isr_nest_level;
	Watchdog_Control *after;
#ifdef _LWPWD_DEBUG
	printf("_Watchdog_Insert(%p,%llu,%llu)\n",wd,wd->start,wd->fire);
#endif
	isr_nest_level = _ISR_Is_in_progress();
	wd->state = LWP_WD_INSERTED;

	_wd_sync_count++;
restart:
	_CPU_ISR_Disable(level);
	fire = wd->delta_interval;
	for(after=_Watchdog_First(header);;after=_Watchdog_Next(after)) {
		if(fire==0 || !_Watchdog_Next(after)) break;
		if(fire<after->delta_interval) break;

		_CPU_ISR_Flash(level);
		if(wd->state!=LWP_WD_INSERTED) goto exit_insert;
		if(_wd_sync_level>isr_nest_level) {
			_wd_sync_level = isr_nest_level;
			_CPU_ISR_Restore(level);
			goto restart;
		}
	}
	_Watchdog_Activate(wd);
	wd->delta_interval = fire;
	_Chain_Insert_unprotected(after->node.prev,&wd->node);
	if(_Watchdog_First(header)==wd) __lwp_wd_settimer(wd);

exit_insert:
	_wd_sync_level = isr_nest_level;
	_wd_sync_count--;
	_CPU_ISR_Restore(level);
	return;
}

u32 _Watchdog_Remove(Chain_Control *header,Watchdog_Control *wd)
{
	u32 level;
	u32 prev_state;
	Watchdog_Control *next;
#ifdef _LWPWD_DEBUG
	printf("_Watchdog_Remove(%p)\n",wd);
#endif
	_CPU_ISR_Disable(level);
	prev_state = wd->state;
	switch(prev_state) {
		case LWP_WD_INACTIVE:
			break;
		case  LWP_WD_INSERTED:
			wd->state = LWP_WD_INACTIVE;
			break;
		case LWP_WD_ACTIVE:
		case LWP_WD_REMOVE:
			wd->state = LWP_WD_INACTIVE;
			next = _Watchdog_Next(wd);
			if(_wd_sync_count) _wd_sync_level = _ISR_Is_in_progress();
			_Chain_Extract_unprotected(&wd->node);
			if(!_Chain_Is_empty(header) && _Watchdog_First(header)==next) __lwp_wd_settimer(next);
			break;
	}
	_CPU_ISR_Restore(level);
	return prev_state;
}

void _Watchdog_Tickle(Chain_Control *queue)
{
	Watchdog_Control *wd;
	u64 now;
	s64 diff;

	if(_Chain_Is_empty(queue)) return;

	wd = _Watchdog_First(queue);
	now = gettime();
	diff = diff_ticks(now,wd->delta_interval);
#ifdef _LWPWD_DEBUG
	printf("_Watchdog_Tickle(%p,%08x%08x,%08x%08x,%08x%08x,%08x%08x)\n",wd,(u32)(now>>32),(u32)now,(u32)(wd->start>>32),(u32)wd->start,(u32)(wd->fire>>32),(u32)wd->fire,(u32)(diff>>32),(u32)diff);
#endif
	if(diff<=0) {
		do {
			switch(_Watchdog_Remove(queue,wd)) {
				case LWP_WD_ACTIVE:	
					wd->routine(wd->user_data);
					break;
				case LWP_WD_INACTIVE:
					break;
				case LWP_WD_INSERTED:
					break;
				case LWP_WD_REMOVE:
					break;
			}
			wd = _Watchdog_First(queue);
		} while(!_Chain_Is_empty(queue) && wd->delta_interval==0);
	} else {
		_Watchdog_Reset(wd);
	}
}

void _Watchdog_Adjust(Chain_Control *queue,u32 dir,s64 interval)
{
	u32 level;
	u64 abs_int;

	_CPU_ISR_Disable(level);
	abs_int = gettime()+LWP_WD_ABS(interval);
	if(!_Chain_Is_empty(queue)) {
		switch(dir) {
			case LWP_WD_BACKWARD:
				_Watchdog_First(queue)->delta_interval += LWP_WD_ABS(interval);
				break;
			case LWP_WD_FORWARD:
				while(abs_int) {
					if(abs_int<_Watchdog_First(queue)->delta_interval) {
						_Watchdog_First(queue)->delta_interval -= LWP_WD_ABS(interval);
						break;
					} else {
						abs_int -= _Watchdog_First(queue)->delta_interval;
						_Watchdog_First(queue)->delta_interval = gettime();
						_Watchdog_Tickle(queue);
						if(_Chain_Is_empty(queue)) break;
					}
				}
				break;
		}
	}
	_CPU_ISR_Restore(level);
}
