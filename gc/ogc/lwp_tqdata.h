#ifndef __LWP_TQDATA_H__
#define __LWP_TQDATA_H__

#define TASK_QUEUE_DATA_NUMBER_OF_PRIORITY_HEADERS 4   
#define TASK_QUEUE_DATA_PRIORITIES_PER_HEADER      64
#define TASK_QUEUE_DATA_REVERSE_SEARCH_MASK        0x20 

typedef enum {
  THREAD_QUEUE_SYNCHRONIZED,
  THREAD_QUEUE_NOTHING_HAPPENED,
  THREAD_QUEUE_TIMEOUT,
  THREAD_QUEUE_SATISFIED
}  Thread_queue_States;

typedef enum {
  THREAD_QUEUE_DISCIPLINE_FIFO,     /* FIFO queue discipline */
  THREAD_QUEUE_DISCIPLINE_PRIORITY  /* PRIORITY queue discipline */
}   Thread_queue_Disciplines;

#ifdef __cplusplus
extern "C" {
#endif

#include "lwp_queue.h"
#include "lwp_priority.h"

typedef struct {
	union {
		Chain_Control Fifo;
		Chain_Control Priority[TASK_QUEUE_DATA_NUMBER_OF_PRIORITY_HEADERS];
	} Queues;
	u32 sync_state;
	u32 discipline;
	u32 state;
	u32 timeout_status;
} Thread_queue_Control;

#ifdef __cplusplus
	}
#endif

#endif
