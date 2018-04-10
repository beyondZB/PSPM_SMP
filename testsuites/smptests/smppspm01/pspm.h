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


typedef void (*ServantRunnable)(
  void *data_i2c,
  size_t size_i2c
);

typedef void (*CServantRunnable)(
  void *data_i2c,
  size_t size_i2c,
  void *data_c2o,
  size_t *size_c2o
);

typedef void (*OServantRunnable)(
  void *data_c2o,
  size_t size_c2o
);


struct _Servent{
  void * runnable;
  rtems_id id; /* timer id, only used in I-servant and O-servant*/
}Servant;

/* Type of subtasks
 * */
typedef struct _Subtask_Node{
  Chain_Node Chain; /* subtasks are managed with chain structure  */
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
typedef int32_t tid_t;

/* The structure for tasks in multi-core PSPM
 * It is one member of the scheduler node Scheduler_EDF_SMP_Node
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
 * */
extern PSPM_SMP pspm_smp_task_manager;

typedef struct _PSPM_SMP{
  Chain_Control Task_Node_queue; /* The queue of task node created by user for saving information */
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
Task_Node * pspm_smp_task_create(
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
  Task_Node *task,
  IServantRunnable i_runnable,
  CServantRunnable c_runnable,
  OServantRunnable o_runnable
);


/* @brief PSPM SMP Message send
 * User API, which can be used to communicating with other task
 * The message are always sent to the comp_queue of specific task
 * @param[in] task_id is the id of target task
 * @param[in] buff is the start address of sented message
 * @param[in] size is the length of message
 * */
void pspm_smp_message_send(
  tid_t task_id,
  const void *buff,
  size_t size
)

/* @brief PSPM SMP Message receive
 * User API, which can be used to communicating with other task
 * The message is received from the comp_queue of current task
 * @param[out] return the task id of message sender
 * @param[in] buff is the start address of sented message
 * @param[in] size is the length of message
 * */
void pspm_smp_message_receive(
  tid_t * task_id,
  void *buff,
  size_t *size
)



#endif


