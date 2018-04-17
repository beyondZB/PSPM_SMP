/**
 * @file
 *
 * @brief EDF SMP Scheduler API
 *
 * @ingroup ScoreSchedulerSMPEDF
 */

/*
 * Copyright (c) 2017 embedded brains GmbH.
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_SCHEDULEREDFSMP_H
#define _RTEMS_SCORE_SCHEDULEREDFSMP_H

#include <rtems/score/scheduler.h>
#include <rtems/score/scheduleredf.h>
#include <rtems/score/schedulersmp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Type of task id */
typedef int32_t tid_t;
/* Type of task defined in pspm.h */

/* Type of subtasks
 * This structure must be one of the structure in SMP_EDF scheduler
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

typedef struct _Servent{
  void * runnable;
  uint32_t id; /* timer id, only used in I-servant and O-servant*/
}Servant;

/* The structure for tasks in multi-core PSPM
 * It is one member of the scheduler node Scheduler_EDF_SMP_Node
 * This structure must be one of the structure in SMP_EDF scheduler
 * */
typedef struct _Task_Node{
  Chain_Node Chain;     /* chain for tasks */
  tid_t id;             /* the global unique task id */
  uint32_t type;       /* Task_type define in pspm.h */
  uint32_t period;      /* period of task presented by number of ms */
  uint32_t wcet;        /* wcet of task presented by number of ms */
  uint32_t quant_period;/* period of task presented by number of quantum */
  uint32_t quant_wcet;  /* wcet of task presented by number of quantum */
  double utility;       /* Utilization of task */
  Servant  i_servant;
  Servant  c_servant;
  Servant  o_servant;
  void * in_message;   /* message for IsC */
  void * out_message;  /* message for OrC */
  uint32_t i_queue_id; /* id of queue for passing message between I and C servant in the same task */
  uint32_t c_queue_id; /* id of queue for communicating with other tasks, used by C-servant */
  uint32_t o_queue_id; /* id of queue for passing message between C and O servant in the same task */
  Chain_Control Subtask_Node_queue; /* chain for subtasks */
}Task_Node;

/*
 * @brief Global PSPM SMP structure for saving pspm tasks informations
 *
 * @param Task_Node_queue is the chain of tasks nodes (type is declared in scheduleredfsmp.h)
 * @param Task_Node_arary is the array of same tasks nodes, only different in implementation method for accelerating searching.
 * @param array_length is the length of array above
 * @param quantum_length is the developer defined quanta, which will be initialized in the init.c
 * */
typedef struct _PSPM_SMP{
  Chain_Control Task_Node_queue; /* The queue of task node created by user for saving information */
  Task_Node ** Task_Node_array;  /* The array of task node created by user for searching node info*/
  uint32_t array_length;  /* The length of task node array */
  uint32_t quantum_length;  /* The quantum length (in number of ticks) defined in system.h */
}PSPM_SMP;

extern PSPM_SMP pspm_smp_task_manager;

/**
 * @defgroup ScoreSchedulerSMPEDF EDF Priority SMP Scheduler
 *
 * @ingroup ScoreSchedulerSMP
 *
 * @{
 */

typedef struct {
  Scheduler_SMP_Node Base;

  /**
   * @brief Generation number to ensure FIFO/LIFO order for threads of the same
   * priority across different ready queues.
   */
  int64_t generation;

  /**
   * @brief The ready queue index depending on the processor affinity of the thread.
   *
   * The ready queue index zero is used for threads with a one-to-all thread
   * processor affinity.  Threads with a one-to-one processor affinity use the
   * processor index plus one as the ready queue index.
   */
  uint32_t ready_queue_index;

  /**
   * @brief Connection with multicore pspm task
   */
  Task_Node * task_node;

  Subtask_Node * current;

} Scheduler_EDF_SMP_Node;


/* Obtaining the current subtask of a task node, this function may be used in default tick function  */
Subtask_Node * _Scheduler_EDF_SMP_Subtask_Chain_current( Scheduler_EDF_SMP_Node * scheduler_edf_smp_node);

/* Set the current subtask to the first one in a task node, and return the first subtask node.
 * This function may be used in release job function in Rate management mechanism */
Subtask_Node * _Scheduler_EDF_SMP_Subtask_Chain_reset( Scheduler_EDF_SMP_Node * scheduler_edf_smp_node);


typedef struct {
  /**
   * @brief Chain node for Scheduler_SMP_Context::Affine_queues.
   */
  Chain_Node Node;

  /**
   * @brief The ready threads of the corresponding affinity.
   */
  RBTree_Control Queue;

  /**
   * @brief The scheduled thread of the corresponding processor.
   */
  Scheduler_EDF_SMP_Node *scheduled;
} Scheduler_EDF_SMP_Ready_queue;

typedef struct {
  Scheduler_SMP_Context Base;

  /**
   * @brief Current generation for LIFO (index 0) and FIFO (index 1) ordering.
   */
  int64_t generations[ 2 ];

  /**
   * @brief Chain of ready queues with affine threads to determine the highest
   * priority ready thread.
   */
  Chain_Control Affine_queues;

  /**
   * @brief A table with ready queues.
   *
   * The index zero queue is used for threads with a one-to-all processor
   * affinity.  Index one corresponds to processor index zero, and so on.
   */
  Scheduler_EDF_SMP_Ready_queue Ready[ RTEMS_ZERO_LENGTH_ARRAY ];
} Scheduler_EDF_SMP_Context;

#define SCHEDULER_EDF_SMP_ENTRY_POINTS \
  { \
    _Scheduler_EDF_SMP_Initialize, \
    _Scheduler_default_Schedule, \
    _Scheduler_EDF_SMP_Yield, \
    _Scheduler_EDF_SMP_Block, \
    _Scheduler_EDF_SMP_Unblock, \
    _Scheduler_EDF_SMP_Update_priority, \
    _Scheduler_EDF_Map_priority, \
    _Scheduler_EDF_Unmap_priority, \
    _Scheduler_EDF_SMP_Ask_for_help, \
    _Scheduler_EDF_SMP_Reconsider_help_request, \
    _Scheduler_EDF_SMP_Withdraw_node, \
    _Scheduler_EDF_SMP_Add_processor, \
    _Scheduler_EDF_SMP_Remove_processor, \
    _Scheduler_EDF_SMP_Node_initialize, \
    _Scheduler_default_Node_destroy, \
    _Scheduler_EDF_Release_job, \
    _Scheduler_EDF_Cancel_job, \
    _Scheduler_default_Tick, \
    _Scheduler_EDF_SMP_Start_idle, \
    _Scheduler_EDF_SMP_Set_affinity \
  }

void _Scheduler_EDF_SMP_Initialize( const Scheduler_Control *scheduler );

void _Scheduler_EDF_SMP_Node_initialize(
  const Scheduler_Control *scheduler,
  Scheduler_Node          *node,
  Thread_Control          *the_thread,
  Priority_Control         priority
);

void _Scheduler_EDF_SMP_Block(
  const Scheduler_Control *scheduler,
  Thread_Control          *thread,
  Scheduler_Node          *node
);

void _Scheduler_EDF_SMP_Unblock(
  const Scheduler_Control *scheduler,
  Thread_Control          *thread,
  Scheduler_Node          *node
);

void _Scheduler_EDF_SMP_Update_priority(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Scheduler_Node          *node
);

bool _Scheduler_EDF_SMP_Ask_for_help(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Scheduler_Node          *node
);

void _Scheduler_EDF_SMP_Reconsider_help_request(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Scheduler_Node          *node
);

void _Scheduler_EDF_SMP_Withdraw_node(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Scheduler_Node          *node,
  Thread_Scheduler_state   next_state
);

void _Scheduler_EDF_SMP_Add_processor(
  const Scheduler_Control *scheduler,
  Thread_Control          *idle
);

Thread_Control *_Scheduler_EDF_SMP_Remove_processor(
  const Scheduler_Control *scheduler,
  struct Per_CPU_Control  *cpu
);

void _Scheduler_EDF_SMP_Yield(
  const Scheduler_Control *scheduler,
  Thread_Control          *thread,
  Scheduler_Node          *node
);

void _Scheduler_EDF_SMP_Start_idle(
  const Scheduler_Control *scheduler,
  Thread_Control          *idle,
  struct Per_CPU_Control  *cpu
);

bool _Scheduler_EDF_SMP_Set_affinity(
  const Scheduler_Control *scheduler,
  Thread_Control          *thread,
  Scheduler_Node          *node,
  const Processor_mask    *affinity
);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _RTEMS_SCORE_SCHEDULEREDFSMP_H */
