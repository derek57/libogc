/*
 *  Implementation of hooks for the CYGNUS newlib libc
 *  These hooks set things up so that:
 *       + '_REENT' is switched at task switch time.
 *
 *  COPYRIGHT (c) 1994 by Division Incorporated
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id$
 *
 */

#include <stdlib.h>             /* for free() */
#include <string.h>             /* for memset() */

#include <sys/reent.h>          /* for extern of _REENT (aka _impure_ptr) */
#include "sysstate.h"
#include "lwp_threads.h"

int              libc_reentrant;        /* do we think we are reentrant? */
struct _reent    libc_global_reent;

extern void _wrapup_reent(struct _reent *);
extern void _reclaim_reent(struct _reent *);

int libc_create_hook(
  Thread_Control *current_task,
  Thread_Control *creating_task
)
{
  creating_task->libc_reent = NULL;
  return 1;
}

/*
 * Called for all user TASKS (system tasks are MPCI Receive Server and IDLE)
 */

int libc_start_hook(
  Thread_Control *current_task,
  Thread_Control *starting_task
)
{
  struct _reent *ptr;

  /*  NOTE: The RTEMS malloc is reentrant without a reent ptr since
   *        it is based on the Classic API Region Manager.
   */

  ptr = (struct _reent *)calloc(1, sizeof(struct _reent));

  if (!ptr)
    abort();

#ifdef __GNUC__
  /* GCC extension: structure constants */
  _REENT_INIT_PTR((ptr));
#else
  /* 
   *  WARNING: THIS IS VERY DEPENDENT ON NEWLIB!!! 
   *           Last visual check was against newlib 1.8.2 but last known
   *           use was against 1.7.0.  This is basically an exansion of
   *           REENT_INIT() in <sys/reent.h>.
   *  NOTE:    calloc() takes care of zeroing fields.
   */
  ptr->_stdin = &ptr->__sf[0];
  ptr->_stdout = &ptr->__sf[1];
  ptr->_stderr = &ptr->__sf[2];
  ptr->_current_locale = "C";
  ptr->_new._reent._rand_next = 1;
#endif

  starting_task->libc_reent = ptr;
  return 1;
}

/*
 *  Function:   libc_delete_hook
 *  Created:    94/12/10
 *
 *  Description:
 *      Called when a task is deleted.
 *      Must restore the new lib reentrancy state for the new current
 *      task.
 *
 *  Parameters:
 *
 *
 *  Returns:
 *
 *
 *  Side Effects:
 *
 *  Notes:
 *
 *
 *  Deficiencies/ToDo:
 *
 *
 */

int libc_delete_hook(
  Thread_Control *current_task,
  Thread_Control *deleted_task
)
{
  struct _reent *ptr;

  /*
   * The reentrancy structure was allocated by newlib using malloc()
   */

  if (current_task == deleted_task)
    ptr = _REENT;
  else
    ptr = (struct _reent *)deleted_task->libc_reent;

  /* if (ptr) */
  if (ptr && ptr != &libc_global_reent) {
    _wrapup_reent(ptr);
    _reclaim_reent(ptr);
    free(ptr);
  }

  deleted_task->libc_reent = 0;

  /*
   * Require the switch back to another task to install its own
   */

  if ( current_task == deleted_task )
    _REENT = 0;

  return 1;
}

/*
 *  Function:   libc_init
 *  Created:    94/12/10
 *
 *  Description:
 *      Init libc for CYGNUS newlib
 *      Set up _REENT to use our global libc_global_reent.
 *      (newlib provides a global of its own, but we prefer our
 *      own name for it)
 *
 *      If reentrancy is desired (which it should be), then
 *      we install the task extension hooks to maintain the
 *      newlib reentrancy global variable _REENT on task
 *      create, delete, switch, exit, etc.
 *
 *  Parameters:
 *      reentrant               non-zero if reentrant library desired.
 *
 *  Returns:
 *
 *  Side Effects:
 *      installs libc extensions if reentrant.
 *
 *  Notes:
 *
 *
 *  Deficiencies/ToDo:
 *
 */

void libc_init(
  int reentrant
)
{
  libc_global_reent = (struct _reent)_REENT_INIT((libc_global_reent));
  _REENT = &libc_global_reent;

  if (reentrant) {
    _Thread_Set_libc_reent((void *)&_REENT);
    libc_reentrant = reentrant;
  }
}

/*
 * CYGNUS newlib routine that does atexit() processing and flushes
 *      stdio streams
 *      undocumented
 */

void libc_wrapup(void)
{
  /*
   *  In case RTEMS is already down, don't do this.  It could be 
   *  dangerous.
   */

  if (!_System_state_Is_up(_System_state_Get()))
    return;

  /*
   *  This was already done if the user called exit() directly .
  _wrapup_reent(0);
   */

  if (_REENT != &libc_global_reent) {
    _wrapup_reent(&libc_global_reent);
#if 0
      /*  Don't reclaim this one, just in case we do printfs
       *  on the way out to ROM.
       */
      _reclaim_reent(&libc_global_reent);
#endif
    _REENT = &libc_global_reent;
  }
}


