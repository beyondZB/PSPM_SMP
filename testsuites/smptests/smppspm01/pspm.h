/*
 * This is head file of multicore PSPM programming model
 * Contents in this file contains data structures and programming interfaces
 * */

#ifndef __PSPM_H__
#define __PSPM_H__
#include "rtems/score/threadimpl.h"
#include "rtems/score/scheduleredfsmp.h"

#include "tmacros.h"
#include "test_support.h"

#define QUANTUM_LENGTH 50 /* In number of ticks */
#define TASK_NUM_MAX 20

/* functions */
void pspm_smp_task_manager_initialize( int task_num, int quanta);

rtems_task Init( rtems_task_argument argument);

rtems_task _comp_servant_routine( rtems_task_argument argument );

void Loop( void );

/*
 *  Handy macros and static inline functions
 */

/* end of include file */

/* @brief I-Servant runnable
 * This runnable will be invoked when timer fires
 * @param[out] data_isc is the start address of sending message
 * @param[out] size_isc is the length of the message
 * */
typedef void (*IServantRunnable)(
  void *data_isc,
  size_t *size_isc
);

/* @brief C-Servant runnable
 * This runnable will be invoked when C-servant is scheduled
 * @param[in] source_id is the task id of message sender
 * @param[in] data_cri is the start address of received message
 * @param[in] size_cri is the length of received message
 * @param[out] target_id is the task id of message receiver
 * @param[out] data_cso is the start address of sent message
 * @param[out] size_cso is the length of sent message
 * */
typedef void (*CServantRunnable)(
  tid_t source_id,  /* The message sender, there is only one sender*/
  void *data_cri,
  size_t size_cri,
  tid_t *target_id,  /* The array of target tasks */
  int32_t *target_num, /* There could multiple target */
  void *data_cso,
  size_t *size_cso
);

/* @brief O-Servant runnable
 * This runnable will be invoked when timer fires
 * @param[in] data_orc is the start address of received message
 * @param[in] size_orc is the length of received message
 * */
typedef void (*OServantRunnable)(
  void *data_orc,
  size_t size_orc
);

/* This function must be called in timeslice function to obtain the subsequent subtasks timing information */
//tid_t _get_task_id(Thread_Control * executing)
//{
//  /* Some kernel including files must be included in this file,
//   * Rather than including them in the pspm.h file */
//  Scheduler_Node * base_node;
//  Scheduler_EDF_SMP_Node * node;
//  /* rtems/score/Threadimpl.h */
//  base_node = _Thread_Scheduler_get_home_node( executing );
//  node = _Scheduler_EDF_SMP_node_downcast( base_node );
//
//  return node->task_node->id;
//}

/* Type of Tasks in multicore PSPM
 * Currently, only the periodic task can be created. 2018/4/10
 * */
#define PERIOD_TASK    1001
#define APERIODIC_TASK 1002
#define SPORADIC_TASK  1003

/* Type of Queue in multicore PSPM
 * */
#define  IN_QUEUE     2001
#define  COMP_QUEUE   2002
#define  OUT_QUEUE    2003

typedef uint32_t Queue_Type;

typedef void* Task_Node_t;


/* @brief Task Creation API
 * User API, which is used to create the task
 * @param[in] task_id is the user defined task id, which is required to be unique and sequential.
 * @param[in] task_type defines the type of task, including periodic, aperiodic, sporadic
 * @param[in] wcet is worst case execution time (in number of ms) of this task
 * @param[in] period is the period (in number of ms) if task type is periodic, otherwise is the minimal arrival interval
 * */
Task_Node_t pspm_smp_task_create(
  tid_t task_id,
  Task_Type task_type,
  int64_t wcet,
  int64_t period
);

/* @brief Task Creation API
 * User API, which is used to create a servant
 * @param[in] task is the belonging task of current servant
 * @param[in] i_runnable is the defined runnable function of I-servant
 * @param[in] c_runnable is the defined runnable function of C-servant
 * @param[in] o_runnable is the defined runnable function of O-servant
 * */
void pspm_smp_servant_create(
  Task_Node_t task,
  IServantRunnable i_runnable,
  CServantRunnable c_runnable,
  OServantRunnable o_runnable
);

/* PSPM SMP programs entry
 * */
void main();

/* configuration information */

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_SIMPLE_CONSOLE_DRIVER
#define CONFIGURE_MAXIMUM_TIMERS 10

/* 1 ms == 1 tick*/
#define CONFIGURE_MILLISECONDES_PER_TICK 1

/* 1 timeslice == 50 ticks */
#define CONFIGURE_TICKS_PER_TIMESLICE 50

#define CONFIGURE_MAXIMUM_PROCESSORS   4

#define CONFIGURE_MAXIMUM_TASKS     (1 + TASK_NUM_MAX)
#define CONFIGURE_MAXIMUM_SEMAPHORES 1

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#include <rtems/confdefs.h>

#endif


