/*
 * PSPM implementaion file
 * This file include the pspm.h file
 * For implementing pspm programming interfaces
 *
 * */
#include "pspm.h"
//#include <rtems/score/percpu.h>

/***********************************************************/
/*        Inner Function Declaration                       */
/***********************************************************/

/* @brief create message queue for each task
 * This function must be invoked after task creations.
 * message queue creation function will create three kinds of message queue in a task.
 * Including I-queue, C-queue, O-queue
 * @param[in] id The id is the pspm task id defined in task node
 * */
void _message_queue_create(Task_Node * task);

/* @brief Obtain the global unique id of queue
 * Obtaining the id of message queue through task id and Queue type
 * @param[in]id The id of task the message queue belongs to
 * @param[in] Queue_Type: IN_QUEUE, COMP_QUEUE, OUT_QUEUE
 * @retval If this method return -1, then no such a task is found.
 * */
rtems_id _get_message_queue(tid_t id, Queue_type type);

/* @brief I-servants send message to C-servant in the same task
 * Can only be called by I-servants
 * @para id, the id of the function calling task
 * @para in, the start address of the sent message
 * @para size_t, the length of the message
 * */
void _pspm_smp_message_queue_IsC(tid_t id, pspm_smp_message *msg);

/* @brief O-servants receive message from C-Servant in the same task
 * Can only be called by O-servants
 * @para qid, the id of the message queue, which is global unique
 * @para out , return the start address of the received message
 * @para size_t, return the length of the message
 * */
rtems_status_code _pspm_smp_message_queue_OrC( tid_t id, pspm_smp_message *msg);

/* @brief C-servants receive message from I-servant in the same task
 * Can only be called by C-servants
 * @para in, return the start address of the received message
 * @para size_t, return the length of the message
 * */
rtems_status_code _pspm_smp_message_queue_CrI( tid_t id, pspm_smp_message *msg);

/* @brief C-servants send message to O-servant in the same task
 * Can only be called by C-servants
 * @para out, the start address of the sent message
 * @para size_t, the length of the message
 * */
void _pspm_smp_message_queue_CsO( tid_t id, pspm_smp_message *msg);

/* @brief Message receive among task, called by pspm smp message receive API
 * This function can be used to communicating with other task
 * The message is received from the comp_queue of current task
 * @param[in] id is the task id of function caller
 * @param[out] source_id is the task id of message sender
 * @param[out] buff is the start address of the received message
 * @param[out] size is the length of message
 * */
rtems_status_code _pspm_smp_message_queue_CrC( tid_t id, pspm_smp_message *msg);

/* @brief Message send among task, called by pspm smp message send API
 * This function can be used to communicating with other task
 * The message are always sent to the comp_queue of specific task
 * @param[in] id is the id of message receiver
 * @param[in] buff is the start address of sented message
 * @param[in] size is the length of message
 * */
rtems_status_code _pspm_smp_message_queue_CsC( tid_t id, pspm_smp_message *msg);


/* @brief Called by _pspm_smp_message_queue_operations, for sending message to qid queue
 * @param[in] qid, the id of the message queue, which is global unique
 * @param[in] msg, the address of the sent message
 * */
rtems_status_code _message_queue_send_message(rtems_id qid, pspm_smp_message * msg);

/* @brief Called by _pspm_smp_message_queue_operations, for receiving message from qid queue
 * @param[in] qid, the id of the message queue
 * @param[in out] msg, return the address of the received message
 * */
rtems_status_code _message_queue_receive_message(rtems_id qid, pspm_smp_message *msg);

/* @brief Obtaining the task id of current executing task
 * @param[out] return the task id of executing.
 * */
tid_t _get_current_task_id();

/* @brief Obtaining the runnable of Servant by task id and Queue type
 * if return -1, no such a task is found.
 * */
void * _get_runnable(tid_t id, Queue_type type);

/* Obtaining the id of message queue through task id and Queue type
 * if return -1, no such a task is found.
 * */
rtems_id _get_message_queue(tid_t id, Queue_type type);

/* @brief constructing a I-servant routine for timer
 * This function act as a parameter of function rtems_timer_fire_after()
 * @param[in] id is the I-servant belonging task id
 * */
void _in_servant_routine( rtems_id null_id, void * task_node);

/* @brief constructing a O-servant routine for timer
 * This function act as a parameter of function rtems_timer_fire_after()
 * @param[in] id is the O-servant belonging task id
 * */
void _out_servant_routine( rtems_id null_id, void * task_node);


/***********************************************************/
/*        Inner Function Implementation                    */
/***********************************************************/

/* @brief Obtaining scheduler node from the base node in RTEMS
 * This function is first introduced in file "Scheduleredfsmp.c"
 * */
static inline Scheduler_EDF_SMP_Node * _scheduler_edf_smp_node_downcast( Scheduler_Node *node  )
{
  return (Scheduler_EDF_SMP_Node *) node;
}

/* @brief Obtaining the task id of current executing task
 * @param[out] return the task id of executing.
 * */
tid_t _get_current_task_id()
{
    /* Since this function don't know current task, we have to obtain it */
  Thread_Control * executing;
  Scheduler_Node * base_node;
  Scheduler_EDF_SMP_Node * node;

  /* Connect pspm smp task with rtems task */
  /* rtems/score/Percpu.h _Per_CPU_Get_executing(_Per_CPU_Get())*/
  executing = _Thread_Executing;
  /* rtems/score/Threadimpl.h */
  base_node = _Thread_Scheduler_get_home_node( executing );
  node = _scheduler_edf_smp_node_downcast( base_node );

  return node->task_node->id;
}


/* Obtaining the id of message queue through task id and Queue type
 * if return -1, no such a task is found.
 * */
rtems_id _get_message_queue(tid_t id, Queue_type type)
{
  rtems_id qid;
  /* No such a task exists */
  if( NULL == pspm_smp_task_manager.Task_Node_array[id])
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



/* Math function ceil and floor implementation */
static double myfloor(double num)
{
  if(num < 0){
    int64_t result = (int64_t)num;
    return (double)(result-1);
  }else{
    return (double)(int64_t)num;
  }

}
static uint64_t myceil(float num)
{
  if(num < 0){
    return (double)(int64_t)num;
  }else{
    int64_t result = (int64_t)num;
    return (double)(result+1);
  }
}


void * _get_runnable(tid_t id, Queue_type type)
{
  void * runnable;
  /* No such a task exists */
  if( NULL == pspm_smp_task_manager.Task_Node_array[id]){
    printf("Error: task %d is not created\n", id);
    return NULL;
  }

  switch(type){
    case IN_QUEUE:
      runnable = pspm_smp_task_manager.Task_Node_array[id]->i_servant.runnable;
      break;
    case COMP_QUEUE:
      runnable = pspm_smp_task_manager.Task_Node_array[id]->c_servant.runnable;
      break;
    case OUT_QUEUE:
      runnable = pspm_smp_task_manager.Task_Node_array[id]->o_servant.runnable;
      break;
    default:
      printf("Error: No runnable is found\n");
      runnable = NULL;
  }
  return runnable;
}


/***********************************************************/
/*        Servant Implementation                           */
/***********************************************************/

void _in_servant_routine(rtems_id null_id, void * task_node)
{
  pspm_smp_message message;
  rtems_id timer_id;
  rtems_interval period;
  rtems_status_code status;
  Task_Node * node= (Task_Node *)task_node;
  ServantRunnable runnable;

  /* Obtaining servant timer id */
  timer_id = node->i_servant.id;
  /* Set the subsequent timer */
  rtems_timer_reset(timer_id);

  if( NULL == (runnable = (ServantRunnable)(node->i_servant.runnable)) ){
    printf("Error: No such a task (tid = %d) when creating a I-Servant\n", node->id);
  } else {
    message.address = node->in_message;
    message.size = 0;
    printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< t%d i-servant\n", node->id);
    runnable( &message);
    printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n", node->id);
    message.sender = node->id;
    _pspm_smp_message_queue_IsC( node->id, &message);
  }
}

/* @brief constructing a O-servant routine for timer
 * This function act as a parameter of function rtems_timer_fire_after()
 * @param[in] id is the O-servant belonging task id
 * */
void _out_servant_routine( rtems_id null_id, void * task_node)
{
  pspm_smp_message message;
  rtems_id timer_id;
  rtems_interval period;
  Task_Node * node= (Task_Node *)task_node;
  rtems_status_code status;
  ServantRunnable runnable;

  /* Obtaining servant timer id */
  timer_id = node->o_servant.id;
  /* Set the subsequent timer */
  rtems_timer_reset(timer_id);

  if( NULL == (runnable = (ServantRunnable)node->o_servant.runnable)){
    printf("Error: No such a task (tid = %d) when creating a O-Servant\n", node->id);
  } else {
    message.address = node->out_message;
    message.size = 0;
    /* Receive message from OUT_QUEUE. There may be multiple messages */
    status = _pspm_smp_message_queue_OrC( node->id, &message);
    if(RTEMS_UNSATISFIED == status){
        printf("No message should output in task %d\n", node->id);
    }else{
        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>   t%d o-servant\n", node->id);
        runnable( &message);
        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>\n\n", node->id);
    }
  }
}

/* @brief constructing a rtems periodic task entry
 * This function act as a parameter of function rtems_task_start()
 * This function is declared in the file system.h
 * */
rtems_task _comp_servant_routine(rtems_task_argument argument)
{
  rtems_id id = argument;

  /* timer for I-Servants and C-Servants */
  rtems_id in_timer_id;
  rtems_id out_timer_id;

  /* Output data address and size */
  pspm_smp_message msg;

  /* Message queue status */
  rtems_status_code status;
  rtems_name name;

  /* Implementing the periodic semantics of rtems tasks */
  rtems_interval period;
  rtems_id rate_monotonic_id;

  ServantRunnable runnable;
  /* If runnable == NULL, no such a task exists */
  if( NULL == (runnable = (ServantRunnable)_get_runnable(id, COMP_QUEUE))){
    printf("Error: No such a task (tid = %d) when creating a C-Servant\n", id);
    return;
  }

  pspm_smp_message_initialize(&msg);
  /******************************************************/
  /*         Initialize the task node structure         */
  /******************************************************/
  /* Some kernel including files must be included in this file,
   * Rather than including them in the pspm.h file */
  Thread_Control * executing;
  Scheduler_Node * base_node;
  Scheduler_EDF_SMP_Node * node;

  /* Connect pspm smp task with rtems task */
  /* rtems/score/Percpu.h _Per_CPU_Get_executing(_Per_CPU_Get())*/
  executing = _Thread_Executing;
  /* rtems/score/Threadimpl.h */
  base_node = _Thread_Scheduler_get_home_node( executing );
  node = _scheduler_edf_smp_node_downcast( base_node );
  /* Important!!!
   * The scheduler EDF SMP node adds one pointer,
   * which corresponds to the task node .
   * Here, initialize the task node element in the scheduler EDF SMP node
   * This operation connects the rtems kernel with multicore PSPM
   * */
  node->task_node = pspm_smp_task_manager.Task_Node_array[id];


  /******************************************************/
  /*          The creation of I-Servant and O-Servant   */
  /******************************************************/
  period = pspm_smp_task_manager.Task_Node_array[id]->period;
  status = rtems_object_get_classic_name(rtems_task_self(), &name);
  directive_failed(status, "rtems_get_name");

  /* Creating timer for O-Servant and set the timer id of its Servant structure */
  status = rtems_timer_create( name, &out_timer_id );
  directive_failed(status, "rtems_timer_create");
  pspm_smp_task_manager.Task_Node_array[id]->o_servant.id = out_timer_id;
  rtems_timer_fire_after(out_timer_id, period, _out_servant_routine, pspm_smp_task_manager.Task_Node_array[id]);

  /* Creating timer for I-Servant and set the timer id of its Servant structure */
  status = rtems_timer_create( name, &in_timer_id );
  directive_failed(status, "rtems_timer_create");
  pspm_smp_task_manager.Task_Node_array[id]->i_servant.id = in_timer_id;
  rtems_timer_fire_after(in_timer_id, period, _in_servant_routine, pspm_smp_task_manager.Task_Node_array[id]);

  /******************************************************/
  /*        The creation of C-Servant                   */
  /******************************************************/
  /* Assuming that the rate monotonic object can always be created successfully */
  status = rtems_rate_monotonic_create( name, &rate_monotonic_id);
  directive_failed(status, "rtems rate monotonic create");

  /* Entry the while loop of a periodic rtems task */
  while(1){
    /*rtems_test_busy_cpu_usage(0, 90000);  busy for 90000 ns */

    /* In rtems, periodic tasks are created by rate monotonic mechanism */
    status = rtems_rate_monotonic_period(rate_monotonic_id, period);
    directive_failed(status, "rtems_rate_monotonic_period");

    /* Receive message from IN_QUEUE of current task */
    status = _pspm_smp_message_queue_CrI(id, &msg);

    if(RTEMS_UNSATISFIED == status) continue;

    printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++t%d c-servant start\n\n", id);
    printf("C-Servant in Task %d has receive message from I-Servant\n", id);
    runnable(&msg);
    printf("\n-------------------------------------------------------------------------t%d c-servant finished\n\n", id);
    /* set the message sender as current task */
    msg.sender = id;
    /* Send message to OUT_QUEUE of current task */
    _pspm_smp_message_queue_CsO(id, &msg);
  }
}/* End Out_servant */


/***********************************************************/
/*        Programming Interface Implementation             */
/***********************************************************/


/* @brief obtain the group deadline of a subtask
 * Group deadline is one of the timing information for sorting the execution of subtasks
 * For more details, please refer to the PD2 relative research paper
 * */
static void _group_deadline_update(
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
    if(0 == p_snode->b && p_snode->d < *p_min_group_deadline){
      *p_min_group_deadline = p_snode->d;
      p_snode->g = p_snode->d;
    }
    if(3 == (p_snode->d - p_snode->r) && (p_snode->d - 1) < *p_min_group_deadline)
      *p_min_group_deadline = p_snode->d - 1;
  }
}

/* @brief initialize subtasks timing information in a task required by PD2
 * @param[in] p_tnode is the point of task node to be initialized
 * */
static void _pd2_subtasks_create(
    Task_Node *p_tnode
    )
{
  rtems_chain_initialize_empty( &p_tnode->Subtask_Node_queue );
  uint32_t min_group_deadline = p_tnode->quant_period;
  for(int i = p_tnode->quant_wcet; i >= 1; i--)
  {
    Subtask_Node *p_new_snode = (Subtask_Node *)malloc(sizeof(Subtask_Node));
    p_new_snode->subtask_no = i;
    p_new_snode->r = myfloor((double)(i - 1) / p_tnode->utility);
    p_new_snode->d = myceil((double)i / p_tnode->utility);

    if(p_tnode->utility < 0.5)
        p_new_snode->b = 0;
    else{
        p_new_snode->b = myceil((double)i / p_tnode->utility) - myfloor((double)i / p_tnode->utility);
        /* the b(Ti) of the last subtask should be 0 */
        p_new_snode->b = (i == p_tnode->quant_wcet) ? 0 : p_new_snode->b;
    }

    _group_deadline_update(p_tnode->utility, p_new_snode, &min_group_deadline);

    /* prepend the subtask into subtask node queue */
    rtems_chain_prepend_unprotected( &p_tnode->Subtask_Node_queue, &p_new_snode->Chain );
    printf("task%d-%d\td = %u\tb = %u\tg = %u\n", p_tnode->id, i, p_new_snode->d, p_new_snode->b, p_new_snode->g);
  }
}

/* @brief Task Creation API
 * User API, which is used to create the task
 * @param[in] task_id is the user defined task id, which is required to be unique and sequential.
 * @param[in] task_type defines the type of task, including periodic, aperiodic, sporadic
 * @param[in] wcet is worst case execution time of this task, the wcet is assumed to be provided by programmers
 * @param[in] period is the period if task type is periodic, otherwise is the minimal arrival interval
 * */
Task_Node_t  pspm_smp_task_create(
    tid_t task_id,
    Task_type task_type,
    int64_t wcet,
    int64_t period
)
{
    /* Allocate memory for the task node */
    Task_Node *p_tnode = (Task_Node *)malloc(sizeof(Task_Node));
    /* Initialize the elements in task node */
    p_tnode->id = task_id;
    p_tnode->type = task_type;
    p_tnode->wcet = RTEMS_MILLISECONDS_TO_TICKS(wcet);
    p_tnode->period = RTEMS_MILLISECONDS_TO_TICKS(period);
    p_tnode->in_message = (void *)malloc(MESSAGE_DATA_LENGTH * sizeof(uint32_t));
    p_tnode->out_message = (void *)malloc(MESSAGE_DATA_LENGTH * sizeof(uint32_t));

    /*Note that : the quantum length is presented in number of ticks */
    p_tnode->quant_wcet =  myceil((double)p_tnode->wcet / pspm_smp_task_manager.quantum_length);
    p_tnode->quant_period =  myceil((double)p_tnode->period / pspm_smp_task_manager.quantum_length);
    p_tnode->utility = (double)p_tnode->quant_wcet / p_tnode->quant_period;

    /* calculate the PD2 relative timing information for scheduling subtasks */
    _pd2_subtasks_create(p_tnode);

    /* insert the task node into the task node chain */
    rtems_chain_append(&(pspm_smp_task_manager.Task_Node_queue), &(p_tnode->Chain));

    /* Important !!!
    * Insert the task node into the task node array for accelerating searching task node */
    if( pspm_smp_task_manager.Task_Node_array[task_id] != NULL){
        printf("Error: current task exists, please use unique task id\n");
    }else{
        pspm_smp_task_manager.Task_Node_array[task_id] = p_tnode;
        pspm_smp_task_manager.array_length ++;
    }

    return (Task_Node_t)p_tnode;
}

/* @brief Task Creation API
 * User API, which is used to create a servant
 * @param[in] task is the belonging task of current servant
 * @param[in] i_runnable is the defined runnable function of I-servant
 * @param[in] c_runnable is the defined runnable function of C-servant
 * @param[in] o_runnable is the defined runnable function of O-servant
 * */
void pspm_smp_servant_create(
    Task_Node_t task,
    ServantRunnable i_runnable,
    ServantRunnable c_runnable,
    ServantRunnable o_runnable
)
{
  /* Set the Servant runnable of a Periodic task */
  ((Task_Node *)task)->i_servant.runnable = i_runnable;
  ((Task_Node *)task)->c_servant.runnable = c_runnable;
  ((Task_Node *)task)->o_servant.runnable = o_runnable;

  /* Creating the task before obtaining message queue */
  _message_queue_create((Task_Node *)task);
}




/***********************************************************/
/*        Message Passing Interfaces Implementation        */
/************************************************************/
/* @brief C-Servant message initialization API
 * Note: this function can only be used in C-Servant runnable
 * */
void pspm_smp_message_initialize(pspm_smp_message *msg)
{
    msg->address = (void *)malloc(MESSAGE_DATA_LENGTH * sizeof(uint32_t));
    msg->size = 0;
}

static void * _pspm_smp_memcpy(void *dst, const void *src, unsigned int n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    while(n-- > 0) *d++ = *s++;
    return dst;
}

void _message_queue_create(Task_Node * task)
{
  /* Create IN_QUEUE, COMP_QUEUE, and OUT_QUEUE in each periodic task
   * Important: message queue function must be created after task being created
   * Notice: All message receive and send operations perform without blocking
   * This requires that RTEMS_NO_WAIT option in rtems message passing mechanism.
   * */
  rtems_id qid;
  rtems_name name;
  rtems_status_code status;

  /* rtems_status_code rtems_message_queue_create(rtems_name name, uint32_t count, size_t max_message_size, rtems_attribute attribute_set, rtems_id *id); */

  name = rtems_build_name('Q','U','I',task->id + '0');
  status = rtems_message_queue_create(name, MESSAGE_BUFFER_LENGTH, sizeof(pspm_smp_message), RTEMS_DEFAULT_ATTRIBUTES, &qid );
  if(RTEMS_SUCCESSFUL == status){
    task->i_queue_id = qid;
  }
  directive_failed(status, "message queue in queue create");

  name = rtems_build_name('Q','U','C',task->id + '0');
  status = rtems_message_queue_create(name, MESSAGE_BUFFER_LENGTH, sizeof(pspm_smp_message), RTEMS_DEFAULT_ATTRIBUTES, &qid );
  if(RTEMS_SUCCESSFUL == status){
    task->o_queue_id = qid;
  }
  directive_failed(status, "message queue comp queue create");

  name = rtems_build_name('Q','U','O',task->id + '0');
  status = rtems_message_queue_create(name, MESSAGE_BUFFER_LENGTH, sizeof(pspm_smp_message), RTEMS_DEFAULT_ATTRIBUTES, &qid );
  if(RTEMS_SUCCESSFUL == status){
    task->c_queue_id = qid;
  }
  directive_failed(status, "message queue out queue create");

}



pspm_status_code pspm_smp_message_queue_send(tid_t id, pspm_smp_message * msg)
{
  rtems_status_code status;

  msg->sender = _get_current_task_id();
  status = _pspm_smp_message_queue_CsC(id, msg);
  if(RTEMS_UNSATISFIED == status){
      printf("Error: Message send in CsC is UNSATISFIED\n");
      return UNSATISFIED;
  }
  return SATISFIED;
}

pspm_status_code pspm_smp_message_queue_receive(pspm_smp_message * msg)
{
  rtems_status_code status;
  status = _pspm_smp_message_queue_CrC(_get_current_task_id(), msg);
  if(RTEMS_UNSATISFIED == status){
      return UNSATISFIED;
  }
  return SATISFIED;
}


rtems_status_code _message_queue_send_message(rtems_id qid, pspm_smp_message * msg)
{
  return rtems_message_queue_send(qid, (const void *) msg, sizeof(pspm_smp_message));
}


rtems_status_code _message_queue_receive_message(rtems_id qid, pspm_smp_message *msg)
{
  /* No blocking receiving.
   * Since I-Servant has higher prior execution than C-Servants,
   * the In_message_queue of current task has at least one message.
   * */
  pspm_smp_message message;
  rtems_status_code status;
  int size;

  status  = rtems_message_queue_receive(qid, &message, &size, RTEMS_NO_WAIT, RTEMS_NO_TIMEOUT);
  if(RTEMS_UNSATISFIED == status){
    return status;
  }
  directive_failed(status, "rtems message queue receive");

  msg->size = message.size;
  msg->sender = message.sender;

  /* A simple memcpy function implemented here, there may be some bugs like overflow.
   * So being careful to use this function.
   * */
  _pspm_smp_memcpy(msg->address, message.address, message.size*sizeof(uint32_t));

  return status;
}

rtems_status_code _pspm_smp_message_queue_CsC( tid_t id, pspm_smp_message *msg)
{
  rtems_id comp_qid;
  rtems_status_code status;

  comp_qid = _get_message_queue(id, COMP_QUEUE);
  if( comp_qid != -1 ){

    if( msg->size > MESSAGE_DATA_LENGTH ){
      printf("Warning: Message size is greater than defined, please confirm!");
      msg->size = MESSAGE_DATA_LENGTH;
    }

    status = _message_queue_send_message(comp_qid, msg);
    if(RTEMS_UNSATISFIED == status){
        return status;
    }
    directive_failed(status, "message queue send CsC");
  } else{
    printf("Error: No such a task (tid = %d) is found\n", id);
    rtems_test_exit(0);
  }
}


rtems_status_code _pspm_smp_message_queue_CrC( tid_t id, pspm_smp_message *msg)
{
  rtems_id comp_qid;
  rtems_status_code status;

  /* get the id of comp_queue in current task */
  comp_qid = _get_message_queue(id, COMP_QUEUE);
  if( comp_qid != -1 ){
    status = _message_queue_receive_message(comp_qid, msg);
    if(RTEMS_UNSATISFIED == status){
        return status;
    }
    directive_failed(status, "message queue receive CrC");

    if( msg->size > MESSAGE_DATA_LENGTH ){
      printf("Warning: Message size is greater than defined, please confirm!");
      msg->size = MESSAGE_DATA_LENGTH;
    }
  }else{
    printf("Error: No such a task (tid = %d) is found\n", id);
    rtems_test_exit(0);
  }
}

rtems_status_code _pspm_smp_message_queue_CrI(tid_t id, pspm_smp_message *msg)
{
  rtems_id in_qid;
  rtems_status_code status;

  in_qid = _get_message_queue(id, IN_QUEUE);
  if( in_qid != -1 ){
    status = _message_queue_receive_message(in_qid, msg);
    if(RTEMS_UNSATISFIED == status){
        return status;
    }
    directive_failed(status, "message queue receive CrI");

    if( msg->size > MESSAGE_DATA_LENGTH ){
      printf("Warning: Message size is greater than defined, please confirm!");
      msg->size = MESSAGE_DATA_LENGTH;
    }
  } else{
    printf("Error: No such a task (tid = %d) is found\n", id);
    rtems_test_exit(0);
  }
}

void _pspm_smp_message_queue_CsO(tid_t id, pspm_smp_message *msg)
{
  rtems_id out_qid;
  rtems_status_code status;

  out_qid = _get_message_queue(id, OUT_QUEUE);

  if( out_qid != -1 ){

    if( msg->size > MESSAGE_DATA_LENGTH ){
      printf("Warning: Message size in CsO the MESSAGE_DATA_LENGTH!");
      msg->size = MESSAGE_DATA_LENGTH;
    }
    status = _message_queue_send_message(out_qid, msg);
    directive_failed(status, "message queue send CsO");

  }else{

    printf("Error: No such a task (tid = %d) is found\n", id);
    rtems_test_exit(0);

  }
}

void _pspm_smp_message_queue_IsC( tid_t id, pspm_smp_message * msg)
{
  rtems_id in_qid;
  rtems_status_code status;

  in_qid = _get_message_queue(id, IN_QUEUE);
  if( in_qid != -1 ){
    if(msg->size > MESSAGE_DATA_LENGTH){
      printf("Error: Message size in IsC exceed the MESSAGE_DATA_LENGTH\n");
      /* Warning: Resize the message size */
      msg->size = MESSAGE_DATA_LENGTH;
    }
    /* Send message to the IN_QUEUE */
    status = _message_queue_send_message(in_qid, msg);
    directive_failed(status, "message queue send IsC");

  }else{
    printf("Error: No such a task (tid = %d) is found\n", id);
    rtems_test_exit(0);
  }
}



rtems_status_code _pspm_smp_message_queue_OrC( tid_t id, pspm_smp_message *msg)
{
  rtems_id in_qid;
  rtems_status_code status;

  in_qid = _get_message_queue(id, OUT_QUEUE);
  if( in_qid != -1 ){
    status = _message_queue_receive_message(in_qid, msg);
    if(status == RTEMS_UNSATISFIED){
        return status;
    }
    directive_failed(status, "Out_servant message queue receive");

    if(msg->size > MESSAGE_DATA_LENGTH){
      printf("Error: Message size in I-Servant exceed the MESSAGE_DATA_LENGTH\n");
      /* Warning: Resize the message size */
      msg->size = MESSAGE_DATA_LENGTH;
    }
  } else{
      printf("Error: No such a task (tid = %d) is found\n", id);
      rtems_test_exit(0);
  }
}




/* configuration information */

  /* Output data address and size */

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_SIMPLE_CONSOLE_DRIVER
#define CONFIGURE_MAXIMUM_PERIODS 10
#define CONFIGURE_MAXIMUM_TIMERS 20
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 30

/* 1000 us == 1 tick, not used here */
#define CONFIGURE_MICROSECONDS_PER_TICK 1000

/* 1 timeslice == 50 ticks */
#define CONFIGURE_TICKS_PER_TIMESLICE QUANTUM_LENGTH
#define CONFIGURE_MAXIMUM_PROCESSORS 1


#define CONFIGURE_MAXIMUM_TASKS     10
#define CONFIGURE_MAXIMUM_SEMAPHORES 10

//#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION
//
//#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_SCHEDULER_EDF_SMP

#include <rtems/scheduler.h>

//RTEMS_SCHEDULER_EDF_SMP(a, CONFIGURE_MAXIMUM_PROCESSORS);
//
//#define CONFIGURE_SCHEDULER_TABLE_ENTRIES \
//  RTEMS_SCHEDULER_TABLE_EDF_SMP(a, rtems_build_name('E', 'D', 'F', ' '))
//
//#define CONFIGURE_SCHEDULER_ASSIGNMENTS \
//  RTEMS_SCHEDULER_ASSIGN(0, RTEMS_SCHEDULER_ASSIGN_PROCESSOR_MANDATORY)

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

//#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_INIT

#include <rtems/confdefs.h>

