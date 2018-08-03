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
#include <rtems/score/overhead_measurement.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULER_EDF_SMP_MAXIMUM_PRIORITY (0x7fffffffffffffff)

// PSPM_SMP+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/* Type of task id */
typedef int32_t tid_t;
/* Type of task defined in pspm.h */

/**
 * Type of subtasks
 * This structure must be one of the structure in SMP_EDF scheduler
 */
typedef struct _Subtask_Node{
  /**
   * @brief the no of subtask in a job.
   */
  uint32_t subtask_no;
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
 **/
typedef struct _Task_Node{
  Chain_Node Chain;     /* chain for tasks */
  tid_t id;             /* the global unique task id */
  uint32_t type;        /* Task_type define in pspm.h */
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
  uint8_t is_pspm_node;
  uint8_t is_preemptable;
}Task_Node;

/*
 * @brief Global PSPM SMP structure for saving pspm tasks informations
 *
 *  Task_Node_queue is the chain of tasks nodes (type is declared in scheduleredfsmp.h)
 *  Task_Node_arary is the array of same tasks nodes, only different in implementation method for accelerating searching.
 *  array_length is the length of array above
 *  quantum_length is the developer defined quanta, which will be initialized in the init.c
 * */
typedef struct _PSPM_SMP{
  Chain_Control Task_Node_queue; /* The queue of task node created by user for saving information */
  Task_Node ** Task_Node_array;  /* The array of task node created by user for searching node info*/
  uint32_t array_length;  /* The length of task node array */
  uint32_t quantum_length;  /* The quantum length (in number of ticks) defined in system.h */
}PSPM_SMP;

/**
 * @brief global variable which contains context of pspm_smp
 */
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

  /**
   * @brief current subtask_node ofthe task
   */
  Subtask_Node * current;

  /*
   * @brief current job release time
   */
  uint64_t release_time;

} Scheduler_EDF_SMP_Node;
// PSPM_SMP-------------------------------------------------------------------------------------

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
    _Scheduler_EDF_SMP_Rlease_job, \
    _Scheduler_EDF_Cancel_job, \
    _Scheduler_EDF_SMP_Tick, \
    _Scheduler_EDF_SMP_Start_idle, \
    _Scheduler_EDF_SMP_Set_affinity \
  }

// PSPM_SMP+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/**
 * @brief Obtaining the current subtask of a task node,
 *   this function may be used in default tick function
 *
 * @param[in] the Scheduler_EDF_SMP_Node of the task
 */
Subtask_Node * _Scheduler_EDF_SMP_Subtask_Chain_current(
        Scheduler_EDF_SMP_Node * scheduler_edf_smp_node
);

/**
 * @brief Set the current subtask to the first one in a task node,
 *   and return the first subtask node.
 * This function may be used in release job function in Rate management mechanism
 *
 * @param[in] the Scheduler_EDF_SMP_Node of the task
 */
Subtask_Node * _Scheduler_EDF_SMP_Subtask_Chain_reset(
        Scheduler_EDF_SMP_Node * scheduler_edf_smp_node
);

/**
 * @brief get the first subtask_node of task_node
 *
 * @param[in] the Task_Node of the task
 */
Subtask_Node * _Scheduler_EDF_SMP_Subtask_Chain_get_first(
        Task_Node * task_node
);

/**
 * @brief get the Scheduler_EDF_SMP_Node of the_thread
 *
 * @param[in] the Thread_Control of the task
 */
Scheduler_EDF_SMP_Node *_get_Scheduler_EDF_SMP_Node(
        Thread_Control * the_thread
);

/**
 * @beif create the pd2 priority
 *
 * @param[in] utility is the utility of the task
 * @param[in] d is the windows deadline, d(Ti) in pd2
 * @param[in] b is the b(Ti) in pd2
 * @param[in] g is the group deadline, D(Ti) in pd2
 *
 */
uint64_t _Scheduler_EDF_SMP_priority_map(
        double utility,
        uint32_t d,
        uint32_t b,
        uint32_t g
);

/**
 * @brief Performs tick operations depending on the CPU budget algorithm for
 * each executing thread.
 *
 * This routine is invoked as part of processing each clock tick.
 *
 * @param[in] scheduler The scheduler.
 * @param[in] executing An executing thread.
 */
void _Scheduler_EDF_SMP_Tick(
  const Scheduler_Control *scheduler,
  Thread_Control          *executing
);

/**
 * @brief based on Scheduler_EDF_release_job.
 *   Will chane priority of tasks as the rule of pspm_smp
 */
void _Scheduler_EDF_SMP_Rlease_job(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Priority_Node           *priority_node,
  uint64_t                 deadline,
  Thread_queue_Context    *queue_context
);
// PSPM_SMP-------------------------------------------------------------------------------------

void _Scheduler_EDF_SMP_Initialize(
        const Scheduler_Control *scheduler
);

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
