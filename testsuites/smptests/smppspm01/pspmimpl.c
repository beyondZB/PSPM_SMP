/*
 * PSPM implementaion file
 * This file include the pspm.h file
 * For implementing pspm programming interfaces
 *
 * */
#include "pspm.h"

chain_Control Task_Node_queue;

PSPM_SMP pspm_smp_task_manager;

/* @brief I-servants send message to C-servant in the same task
 * Can only be called by I-servants
 * @para id, the id of the function calling task
 * @para in, the start address of the sent message
 * @para size_t, the length of the message
 * */
static void _pspm_smp_message_queue_IsC(tid_t id, const void *in, size_t size_in);

/* @brief O-servants recieve message from C-Servant in the same task
 * Can only be called by O-servants
 * @para qid, the id of the message queue, which is global unique
 * @para out , return the start address of the received message
 * @para size_t, return the length of the message
 * */
static void _pspm_smp_message_queue_OrC( tid_t id, const void *out, size_t * size_out);

/* @brief C-servants receive message from I-servant in the same task
 * Can only be called by C-servants
 * @para in, return the start address of the received message
 * @para size_t, return the length of the message
 * */
static void _pspm_smp_message_queue_CrI( void * in, size_t * size_in);

/* @brief C-servants send message to O-servant in the same task
 * Can only be called by C-servants
 * @para out, the start address of the sent message
 * @para size_t, the length of the message
 * */
static void _pspm_smp_message_queue_CsO( void * out, size_t size_in);

/* Called by C-Servant routine for obtained the task id of its beloning task */
static inline tid_t _get_task_id();

/* @brief Obtain the global unique id of queue
 * Obtaining the id of message queue through task id and Queue type
 *
 * @retval If this method return -1, then no such a task is found.
 * */
inline rtems_id _message_queue_get_id(tid_t id, Queue_Type type);

/* @brief Called by _pspm_smp_message_queue_operations, for sending message to qid queue
 * @para qid, the id of the message queue, which is global unique
 * @para in, the start address of the sent message
 * @para size_t, the length of the message
 * */
static inline void _message_queue_send_message(rtems_id qid, const void *in, size_t size_in);
/* @brief Called by _pspm_smp_message_queue_operations, for sending message to qid queue
 * @para qid, the id of the message queue, which is global unique
 * @para out , return the start address of the received message
 * @para size_t, return the length of the message
 * */
static inline void _message_queue_recieve_message(rtems_id qid, void * out, size_t * size_out);

/* @brief Obtaining scheduler node from the base node in RTEMS
 * This function is first introduced in file "Scheduleredfsmp.c"
 * */
static inline Scheduler_EDF_SMP_Node * _Scheduler_EDF_SMP_Node_downcast( Scheduler_Node *node  )
{
  return (Scheduler_EDF_SMP_Node *) node;
}

/* @brief obtain the group deadline of a subtask
 * Group deadline is one of the timing information for sorting the execution of subtasks
 * For more details, please refer to the PD2 relative research paper
 * */
static void _update_min_group_deadline(
  double task_utility,
  Subtask_Node *p_snode,
  uint32_t *p_min_group_deadline
)
{
  if(task_utility < 0.5){
    p_snode->g = 0;
    *p_min_group_deadline = 0;
    return;
  }else{
    p_snode->g = *p_min_group_deadline;
    if(p_snode->b == 0 && p_snode->d < *p_min_group_deadline){
      *p_min_group_deadline = p_snode->d;
      p_snode->g = p_snode->d;
    }
    if(p_snode->d - p_snode->r == 3 && p_snode->d - 1 < *p_min_group_deadline)
      *p_min_group_deadline = p_snode->d - 1;
  }
}

/* @brief initialize subtasks timing information in a task required by PD2
 * @param[in] p_tnode is the point of task node to be initialized
 * */
static void _create_pd2_subtasks(
  Task_Node *p_tnode
)
{
  rtems_chain_initialize_empty( &p_tnode->Subtask_Node_queue );
  uint32_t min_group_deadline = p_tnode->period;
  for(int i = p_tnode->wcet; i >= 1; i--)
  {
    Subtask_Node *p_new_snode = (Subtask_Node *)malloc(sizeof(Subtask_Node));
    p_new_snode->r = floor((double)(i - 1) / p_tnode->utility);
    p_new_snode->d = ceil((double)i / p_tnode->utility);
    p_new_snode->b = ceil((double)i / p_tnode->utility) - floor((double)i / p_tnode->utility);
    /* the b(Ti) of the last subtask should be 0 */
    p_new_snode->b = (i == p_tnode->wcet) ? 0 : p_new_snode->b;
    _update_min_group_deadline(p_tnode->utility, p_new_snode, &min_group_deadline);

    /* prepend the subtask into subtask node queue */
    rtems_chain_prepend_unprotected( &p_tnode->Subtask_Node_queue, &p_new_snode->Chain );
  }
}

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
)
{
  /* Allocate memory for the task node */
  Task_Node *p_tnode = (Task_Node *)malloc(sizeof(Task_Node));
  /* Initialize the elements in task node */
  p_tnode->id = task_id;
  p_tnode->type = task_type;
  p_tnode->wcet = ceil((double)wcet / pspm_smp_task_manager.quantum_length);
  p_tnode->period = ceil((double)period / pspm_smp_task_manager.quantum_length);
  p_tnode->utility = (double)p_tnode->wcet / p_tnode->period;

  /* calculate the PD2 relative timing information for scheduling subtasks */
  _create_pd2_subtasks(p_tnode);
  /* fifth step: insert the task node into the task node chain */
  rtems_chain_append_unprotected(&pspm_smp_task_manager.Task_Node_queue, &p_tnode->Chain);
  return p_tnode;
}

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
)
{
  task->i_servant.runnable = i_runnable;
  task->c_servant.runnable = c_runnable;
  task->o_servant.runnable = o_runnable;
}

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
{
  rtems_id comp_qid;

  comp_qid = _message_queue_get_id(id, COMP_QUEUE);
  if( comp_qid != -1 )
    _message_queue_send_message(comp_qid, buff, size);
  else
    print("Error: No such a task (tid = %d) is found\n", id);
}

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
{
  rtems_id comp_qid;
  tid_t task_id;

  /* get the id of current task */
  task_id = _get_task_id();
  comp_qid = _message_queue_get_id(id, COMP_QUEUE);
  if( comp_qid != -1 ){
    _message_queue_recieve_message(comp_qid, buff, size);
    *task_id = ((Message_t *)buff)->id;
  }
  else
    print("Error: No such a task (tid = %d) is found\n", id);
}


inline tid_t _get_task_id()
{
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

inline void _message_queue_send_message(rtems_id qid, const void * in, size_t size_in)
{
  rtems_message_queue_send(qid, in ,size_in);
}

inline void _message_queue_recieve_message(rtems_id qid, void * out, size_t * size_out)
{
  /* No blocking receiving.
   * Since I-Servant has higher prior execution than C-Servants,
   * the In_message_queue of current task has at least one message.
   * */
  rtems_message_queue_receive(qid, out, size_out, RTEMS_NO_WAIT, RTEMS_NO_TIMEOUT)
}

/* Obtaining the id of message queue through task id and Queue type
 * if return -1, no such a task is found.
 * */
inline rtems_id _message_queue_get_id(tid_t id, Queue_Type type)
{
  rtems_id qid;
  /* No such a task exists */
  if( pspm_smp_task_manager.Task_Node_array[id] == NULL )
    return -1;

  switch(type){
    case IN_QUEUE:
      qid = pspm_smp_task_manager.Task_Node_array[id]->i_queue_id;
      break;
    case COMP_QUEUE:
      qid = pspm_smp_task_manager.Task_Node_array[id]->c_queue_id;
      break;
    case OUT_QUEUE:
      qid = pspm_smp_task_manager.Task_Node_array[id]->o_queue_id;
      break;
    default:
      qid = -1;
  }
  return qid;
}

void _pspm_smp_message_queue_IsC( tid_t id, const void *in, size_t size_in)
{
  rtems_id in_qid;

  in_qid = _message_queue_get_id(id, IN_QUEUE);
  if( in_qid != -1 )
    _message_queue_send_message(in_qid, in, size_in);
  else
    print("Error: No such a task (tid = %d) is found\n", id);
}

void _pspm_smp_message_queue_OrC( tid_t id, void *out, size_t * size_out)
{
  rtems_id in_qid;

  in_qid = _message_queue_get_id(id, OUT_QUEUE);
  if( in_qid != -1 )
    _message_queue_recieve_message(out_qid, out, size_out);
  else
    print("Error: No such a task (tid = %d) is found\n", id);

}


void _pspm_smp_message_queue_CrI(void * in, size_t * size_in)
{
  rtems_id in_qid;
  tid_t id;

  id =  _get_task_id();
  in_qid = rtems_message_queue_id_get(id, IN_QUEUE);
  if( in_qid != -1 )
    _message_queue_recieve_message(in_qid, in, size_in);
  else
    print("Error: No such a task (tid = %d) is found\n", id);
}

void _pspm_smp_message_queue_CsO(const void * out, size_t size_in)
{
  rtems_id out_qid;
  tid_t id;

  id = _get_task_id();
  out_qid = rtems_message_queue_id_get(id, OUT);
  if( in_qid != -1 )
    _message_queue_send_message(out_qid, out, size_out);
  else
    print("Error: No such a task (tid = %d) is found\n", id);

}



