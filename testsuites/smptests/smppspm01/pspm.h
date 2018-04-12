/*
 * This is head file of multicore PSPM programming model
 * Contents in this file contains data structures and programming interfaces
 * */

#ifndef __PSPM_H__
#define __PSPM_H__
#include "rtems/Chain.h"
#include "rtems.h"
#include "rtems/score/Percpu.h"
#include "rtems/score/Threadimpl.h"


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
  int32_t *target_num; /* There could multiple target */
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


struct _Servent{
  void * runnable;
  rtems_id id; /* timer id, only used in I-servant and O-servant*/
}Servant;


/* This function must be called in timeslice function to obtain the subsequent subtasks timing information */
tid_t _get_task_id()
{
  /* Some kernel including files must be included in this file,
   * Rather than including them in the pspm.h file */
  Thread_Control * executing;
  Scheduler_Node * base_node;
  Scheduler_EDF_SMP_Node * node;
  /* rtems/score/Percpu.h */
  executing = __Thread_Executing;
  /* rtems/score/Threadimpl.h */
  base_node = _Thread_Scheduler_get_home_node( executing );
  node = _Scheduler_EDF_SMP_node_downcast( base_node );

  return node->task_node.id;
}

/* Type of subtasks
 * This structure must be one of the structure in SMP_EDF scheduler
 * */
typedef struct _Subtask_Node{
  rtems_chain_node Chain; /* subtasks are managed with chain structure  */
  /* following parameters are timing infos of subtasks:
   * b(Ti), r(Ti), d(Ti), group deadline(Ti) in PD2*/
  uint32_t b;
  uint32_t r;
  uint32_t d;
  uint32_t g;
}Subtask_Node;

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

/* Type of task id */
typedef int64_t tid_t;

/* The structure for tasks in multi-core PSPM
 * It is one member of the scheduler node Scheduler_EDF_SMP_Node
 * This structure must be one of the structure in SMP_EDF scheduler
 * */
typedef struct _Task_Node{
  rtems_chain_node Chain;     /* chain for tasks */
  tid_t id;             /* the global unique task id */
  Task_Type type;
  uint32_t period;      /* period of task presented by number of ticks */
  uint32_t wcet;        /* wcet of task presented by number of ticks */
  uint32_t quant_period;/* period of task presented by number of quantum */
  uint32_t quant_wcet;  /* wcet of task presented by number of quantum */
  double utility;       /* Utilization of task */
  Servant  i_servant;
  Servant  c_servant;
  Servant  o_servant;
  rtems_id i_queue_id; /* id of queue for passing message between I and C servant in the same task */
  rtems_id c_queue_id; /* id of queue for communicating with other tasks, used by C-servant */
  rtems_id o_queue_id; /* id of queue for passing message between C and O servant in the same task */
  rtems_chain_control Subtask_Node_queue; /* chain for subtasks */
}Task_Node;

typedef struct _Message{
  void * address; /*The start address of the Message*/
  size_t size; /*The length of Message */
  tid_t id; /* the task id of Message sender */
}Message_t;

/* One global data structure for saving task information
 * This variable must be in the including file of kernel
 * For being invoked in default_tick function
 * */
typedef (void *) Task_Node_t;

/* This type must be in the same file pspm_smp_task_manager */
typedef struct _PSPM_SMP{
  rtems_chain_control Task_Node_queue; /* The queue of task node created by user for saving information */
  Task_Node ** Task_Node_array;  /* The array of task node created by user for searching node info*/
  uint32_t array_length;  /* The length of task node array */
  uint32_t quantum_length;  /* The quantum length setted by developer */
}PSPM_SMP;


/* @brief Task Creation API
 * User API, which is used to create the task
 * @param[in] task_id is the user defined task id, which is required to be unique and sequential.
 * @param[in] task_type defines the type of task, including periodic, aperiodic, sporadic
 * @param[in] wcet is worst case execution time of this task, the wcet is assumed to be provided by programmers
 * @param[in] period is the period if task type is periodic, otherwise is the minimal arrival interval
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

#endif


