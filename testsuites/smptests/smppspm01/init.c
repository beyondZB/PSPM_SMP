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

#include <inttypes.h>

const char rtems_test_name[] = "PSPM SMP 01";

uint32_t GCD(uint32_t a, uint32_t b){
    /* Assuming that a and b are greater than 0 */
    return b == 0 ? a : GCD(b, a%b);
}

uint32_t LCM(uint32_t a, uint32_t b){
    return a*b/GCD(a,b);
}

void Loop() {
  volatile int i;
  for (i=0; i<300000; i++);
}

void rtems_sucess(){
    /**
    *status = rtems_task_delete( RTEMS_SELF );
    *directive_failed( status, "rtems_task_delete of RTEMS_SELF" );
    */
    TEST_END();
    rtems_test_exit( 0 );
}

void pspm_smp_task_manager_initialize( uint32_t task_num, uint32_t quanta)
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
  pspm_smp_task_manager.array_length = 0;
  pspm_smp_task_manager.quantum_length = quanta; /* in number of ticks */
}


rtems_task Init(
  rtems_task_argument argument
)
{
  uint32_t           i;
  uint32_t           lcm = 1;
  char               ch;
  uint32_t           cpu_self;
  uint32_t           cpu_num;
  uint32_t           count;   /* The number of pspm tasks */
  rtems_id           id[TASK_NUM_MAX];        /* id is for rtems task */
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

  /* Output System information */
  printf(" Init Task run on CPU %" PRIu32 "\n", cpu_self);
  printf(" The number of CPU is %" PRIu32 "\n", cpu_num);

  /* Initialize the most important structure */
  pspm_smp_task_manager_initialize(TASK_NUM_MAX, QUANTUM_LENGTH);

  /* Interpretation the application designed with pspm_smp programming paradigm */
  main();

  ///* Obtaining the pspm task chain and the first task node in the chain */
  chain = &pspm_smp_task_manager.Task_Node_queue;
  node = rtems_chain_first(chain);

  /* The tail in the Chain is NULL, thus the tail node should not be processed */
  count = 0;
  while( !rtems_chain_is_tail(chain, node) ){
    task_node = RTEMS_CONTAINER_OF(node, Task_Node, Chain);

    /* Obtaining the pspm task id */
    task_id = task_node->id;
    /* Obtaining the LCM of task periods */
    lcm = LCM(lcm, task_node->period);

    Subtask_Node * snode = _Scheduler_EDF_SMP_Subtask_Chain_get_first(task_node);

    status = rtems_task_create(
      rtems_build_name( 'P', 'T', 'A', task_id+'0' ),
//      _pspm_smp_create_priority(task_node->utility, snode->d * pspm_smp_task_manager.quantum_length, snode->b, snode->g) >> 31,
      snode->d * pspm_smp_task_manager.quantum_length,
      RTEMS_MINIMUM_STACK_SIZE,
      RTEMS_TIMESLICE,
      RTEMS_DEFAULT_ATTRIBUTES,
      &id[count]
    );
    directive_failed( status, "task create" );

    /* API locked_printf will blocked, so we use function printf which is also thread safe */
    printf(" CPU %" PRIu32 " start periodic task TA%d\n", cpu_self, task_id);

    /* task_id is the argument for creating I-C-O Servants of a task, for more details, please refer to pspmimpl.c */
    status = rtems_task_start( id[count], _comp_servant_routine, task_id);
    directive_failed( status, "task start" );

    /* waiting for starting the next pspm task */
    Loop();
    node = rtems_chain_next(node);
    count ++;
  }

  rtems_cpu_usage_reset();

  /* The time here are !!!not!!! supposed to be set to the LCM of task periods.
   * Otherwise, the Allocator Error occurs leading to no statistic results of CPU usage.
   * */
  status = rtems_task_wake_after(1000 * lcm + 10);
  rtems_test_assert(RTEMS_SUCCESSFUL == status);

  for( ; count > 0; count--){
      status = rtems_task_suspend(id[count - 1]);
      rtems_test_assert(RTEMS_SUCCESSFUL == status);
  }

  rtems_cpu_usage_report_with_plugin(&rtems_test_printer);
  rtems_rate_monotonic_report_statistics_with_plugin(&rtems_test_printer);

  pspm_smp_print_count();
  /* APP finishes */
  rtems_sucess();
}



