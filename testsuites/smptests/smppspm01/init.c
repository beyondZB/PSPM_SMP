/*
 *  COPYRIGHT (c) 1989-2011.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define CONFIGURE_INIT
#include "pspm.h"

/* PSPM  relative contents */
extern PSPM_SMP pspm_smp_task_manager;

#include <inttypes.h>

const char rtems_test_name[] = "PSPM SMP 01";

void Loop() {
  volatile int i;

  for (i=0; i<300000; i++);
}

void pspm_smp_task_manager_initialize( int task_num, int quanta)
{
  int index;

  /* Initialize the Chain control, which is required */
  rtems_chain_initialize_empty(&pspm_smp_task_manager.Task_Node_queue);

  /* Initialize the task node array */
  pspm_smp_task_manager.Task_Node_array = (Task_Node **)malloc(task_num * sizeof(Task_Node *));
  for(index = 0; index < task_num; ++index){
    pspm_smp_task_manager.Task_Node_array[index] = NULL;
  }
  /* Initialize the array length and quantum length */
  pspm_smp_task_manager.array_length = task_num;
  pspm_smp_task_manager.quantum_length = quanta; /* in number of ticks */
}

rtems_task Init(
  rtems_task_argument argument
)
{
  uint32_t           i;
  char               ch;
  uint32_t           cpu_self;
  uint32_t           cpu_num;
  rtems_id           id;        /* id is for rtems task */
  rtems_id           task_id;   /* task_id is for smp pspm task */
  rtems_status_code  status;
  rtems_chain_node   *node;
  rtems_chain_control *chain;
  Task_Node           *task_node;


  cpu_self = rtems_get_current_processor();
  cpu_num  = rtems_get_processor_count();

  /* XXX - Delay a bit to allow debug messages from
   * startup to print.  This may need to go away when
   * debug messages go away.
   */
  Loop();
  TEST_BEGIN();
  locked_print_initialize();

  /* Output System information */
  locked_printf(" Init Task run on CPU %" PRIu32 "\n", cpu_self);
  locked_printf(" The number of CPU is %" PRIu32 "\n", cpu_num);

  /* Initialize the most important structure */
  pspm_smp_task_manager_initialize(TASK_NUM_MAX, QUANTUM_LENGTH);

  /* Interpretation the application designed with pspm_smp programming paradigm */
  main();

  ///* Obtaining the first task node of PSPM task chain */
  chain = &pspm_smp_task_manager.Task_Node_queue;
  node = rtems_chain_first(chain);

  /* The tail in the Chain is NULL, thus the tail node should not be processed */
  while( !rtems_chain_is_tail(chain, node) ){
    task_node = RTEMS_CONTAINER_OF(node, Task_Node, Chain);
    task_id = task_node->id;
    status = rtems_task_create(
      rtems_build_name( 'P', 'T', 'A', task_id ),
      2,
      RTEMS_MINIMUM_STACK_SIZE,
      RTEMS_TIMESLICE | RTEMS_PREEMPT,
      RTEMS_DEFAULT_ATTRIBUTES,
      &id
    );
    directive_failed( status, "task create" );

    locked_printf(" CPU %" PRIu32 " start periodic task TA%d\n", cpu_self, task_id);
    /* task_id is the argument for creating I-C-O Servants of a task, for more details, please refer to pspmimpl.c */
    status = rtems_task_start( id, _comp_servant_routine, task_id);
    directive_failed( status, "task start" );
    Loop();
    node = rtems_chain_next(node);
  }

  /* Wait on the all tasks to run */
  TEST_END();
  //rtems_test_exit( 0 );
  status = rtems_task_delete( RTEMS_SELF );
  directive_failed( status, "rtems_task_delete of RTEMS_SELF" );



}
