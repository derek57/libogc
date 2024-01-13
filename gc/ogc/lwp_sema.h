#ifndef __LWP_SEMA_H__
#define __LWP_SEMA_H__

#include <gctypes.h>
#include <lwp_threadq.h>

typedef enum {
  /** This specifies that threads will wait for the semaphore in FIFO order. */
  CORE_SEMAPHORE_DISCIPLINES_FIFO,
  /** This specifies that threads will wait for the semaphore in
   *  priority order.
   */
  CORE_SEMAPHORE_DISCIPLINES_PRIORITY
}   CORE_semaphore_Disciplines;

typedef enum {
  /** This status indicates that the operation completed successfully. */
  CORE_SEMAPHORE_STATUS_SUCCESSFUL,
  /** This status indicates that the calling task did not want to block
   *  and the operation was unable to complete immediately because the
   *  resource was unavailable.
   */
  CORE_SEMAPHORE_STATUS_UNSATISFIED_NOWAIT,
  /** This status indicates that the thread was blocked waiting for an
   *  operation to complete and the semaphore was deleted.
   */
  CORE_SEMAPHORE_WAS_DELETED,
  /** This status indicates that the calling task was willing to block
   *  but the operation was unable to complete within the time allotted
   *  because the resource never became available.
   */
  CORE_SEMAPHORE_TIMEOUT,
  /** This status indicates that an attempt was made to unlock the semaphore
   *  and this would have made its count greater than that allowed.
   */
  CORE_SEMAPHORE_MAXIMUM_COUNT_EXCEEDED
}   CORE_semaphore_Status;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	u32 maximum_count;
	u32 discipline;
} CORE_semaphore_Attributes;

typedef struct {
	Thread_queue_Control Wait_queue;
	CORE_semaphore_Attributes	Attributes;
	u32 count;
} CORE_semaphore_Control;

void CORE_semaphore_Initialize(CORE_semaphore_Control *sema,CORE_semaphore_Attributes *attrs,u32 init_count);
u32 _CORE_semaphore_Surrender(CORE_semaphore_Control *sema,u32 id);
u32 _CORE_semaphore_Seize(CORE_semaphore_Control *sema,u32 id,u32 wait,u64 timeout);
void _CORE_semaphore_Flush(CORE_semaphore_Control *sema,u32 status);

#ifdef LIBOGC_INTERNAL
#include <libogc/lwp_sema.inl>
#endif

#ifdef __cplusplus
	}
#endif

#endif
