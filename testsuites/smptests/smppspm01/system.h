/*
 *  COPYRIGHT (c) 1989-2011.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#include "tmacros.h"
#include "test_support.h"

#define QUANTUM_LENGTH 50 /* In number of ticks */
#define TASK_NUM_MAX 20

/* functions */
void pspm_smp_task_manager_initialize( int task_num, int quanta);

rtems_task Init(
  rtems_task_argument argument
);

rtems_task _comp_servant_routine()(
    rtems_task_argument argument
);

/* configuration information */

#define CONFIGURE_APPLICATION_DOES_NOT_NEED_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_SIMPLE_CONSOLE_DRIVER

#define CONFIGURE_MAXIMUM_PROCESSORS   4

#define CONFIGURE_MAXIMUM_TASKS     (1 + CONFIGURE_MAXIMUM_PROCESSORS)
#define CONFIGURE_MAXIMUM_SEMAPHORES 1

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#include <rtems/confdefs.h>

/* global variables */

/*
 *  Keep the names and IDs in global variables so another task can use them.
 */

TEST_EXTERN volatile bool TaskRan[ CONFIGURE_MAXIMUM_PROCESSORS ];

void Loop( void );

/*
 *  Handy macros and static inline functions
 */

/* end of include file */
