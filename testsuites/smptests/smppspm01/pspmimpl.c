/*
 * PSPM implementaion file
 * This file include the pspm.h file
 * For implementing pspm programming interfaces
 *
 * */
#include "pspm.h"

/* The length of message data, in number of word */
#define MESSAGE_DATA_LENGTH 2
/*One message is allowed to be sent to at most TARTGET_NUM_MAX tasks */
#define TARTGET_NUM_MAX 20

/* The size of message buffer in each message queue */
#define MESSAGE_BUFFER_LENGTH 5

typedef struct _Message{
  void * address; /*The start address of the Message*/
  size_t size; /*The length of Message */
  tid_t id; /* the task id of Message sender */
}Message_t;


/***********************************************************/
/*        Inner Function Declaration                       */
/***********************************************************/

/* @brief Obtain the global unique id of queue
 * Obtaining the id of message queue through task id and Queue type
 *
 * @retval If this method return -1, then no such a task is found.
 * */
rtems_id _get_message_queue(tid_t id, Queue_Type type);

/* @brief I-servants send message to C-servant in the same task
 * Can only be called by I-servants
 * @para id, the id of the function calling task
 * @para in, the start address of the sent message
 * @para size_t, the length of the message
 * */
static void _pspm_smp_message_queue_IsC(tid_t id, const void *in, size_t size_in);

/* @brief O-servants receive message from C-Servant in the same task
 * Can only be called by O-servants
 * @para qid, the id of the message queue, which is global unique
 * @para out , return the start address of the received message
 * @para size_t, return the length of the message
 * */
static void _pspm_smp_message_queue_OrC( tid_t id, void *out, size_t * size_out);

/* @brief C-servants receive message from I-servant in the same task
 * Can only be called by C-servants
 * @para in, return the start address of the received message
 * @para size_t, return the length of the message
 * */
static void _pspm_smp_message_queue_CrI( tid_t id, void * in, size_t * size_in);

/* @brief C-servants send message to O-servant in the same task
 * Can only be called by C-servants
 * @para out, the start address of the sent message
 * @para size_t, the length of the message
 * */
static void _pspm_smp_message_queue_CsO( tid_t id, const void * out, size_t size_in);

/* @brief PSPM SMP Message receive
 * This function can be used to communicating with other task
 * The message is received from the comp_queue of current task
 * @param[in] id is the task id of function caller
 * @param[out] source_id is the task id of message sender
 * @param[out] buff is the start address of the received message
 * @param[out] size is the length of message
 * */
static void _pspm_smp_message_queue_CrC( tid_t id, tid_t *source_id, void *buff, size_t *size);

/* @brief PSPM SMP Message send
 * This function can be used to communicating with other task
 * The message are always sent to the comp_queue of specific task
 * @param[in] id is the id of message receiver
 * @param[in] buff is the start address of sented message
 * @param[in] size is the length of message
 * */
static void _pspm_smp_message_queue_CsC( tid_t id, const void *buff, size_t size);


/* @brief Called by _pspm_smp_message_queue_operations, for sending message to qid queue
 * @para qid, the id of the message queue, which is global unique
 * @para in, the start address of the sent message
 * @para size_t, the length of the message
 * */
inline rtems_status_code _message_queue_send_message(rtems_id qid, const void *in, size_t size_in);
/* @brief Called by _pspm_smp_message_queue_operations, for sending message to qid queue
 * @para qid, the id of the message queue, which is global unique
 * @para out , return the start address of the received message
 * @para size_t, return the length of the message
 * */
inline rtems_status_code _message_queue_receive_message(rtems_id qid, void * out, size_t * size_out);


/* @brief Obtaining the runnable of Servant by task id and Queue type
 * if return -1, no such a task is found.
 * */
void * _get_runnable(tid_t id, Queue_Type type);

/* @brief allocate memory for message data
 *
 * Note: the length of memory for data is defined by MESSAGE_DATA_LENGTH
 * */
void _initialize_message( Message_t * message );

/* @brief constructing a I-servant routine for timer
 * This function act as a parameter of function rtems_timer_fire_after()
 * @param[in] id is the I-servant belonging task id
 * */
void _in_servant_routine( rtems_id timer_id, void * task_id );

/* @brief constructing a O-servant routine for timer
 * This function act as a parameter of function rtems_timer_fire_after()
 * @param[in] id is the O-servant belonging task id
 * */
void _out_servant_routine( rtems_id timer_id, void * task_id );


/***********************************************************/
/*        Inner Function Implementation                    */
/***********************************************************/



/* Obtaining the id of message queue through task id and Queue type
 * if return -1, no such a task is found.
 * */
rtems_id _get_message_queue(tid_t id, Queue_Type type)
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

/* @brief Obtaining scheduler node from the base node in RTEMS
 * This function is first introduced in file "Scheduleredfsmp.c"
 * */
static inline Scheduler_EDF_SMP_Node * _scheduler_edf_smp_node_downcast( Scheduler_Node *node  )
{
  return (Scheduler_EDF_SMP_Node *) node;
}

/* Math function ceil and floor implementation */
static float myfloor(float num)
{
  if(num < 0){
    int result = (int)num;
    return (float)(result-1);
  }else{
    return (float)(int32_t)num;
  }

}
static uint32_t myceil(float num)
{
  if(num < 0){
    int result = (int)num;
    return (float)(result);
  }else{
    int result = (int)num;
    return (float)(result+1);
  }
}


void * _get_runnable(tid_t id, Queue_Type type)
{
  void * runnable;
  /* No such a task exists */
  if( pspm_smp_task_manager.Task_Node_array[id] == NULL )
    return NULL;

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
      runnable = NULL;
  }
  return runnable;
}


void _initialize_message( Message_t * message )
{
  message->address = (int32_t *)malloc(MESSAGE_DATA_LENGTH * sizeof(int32_t));
  message->size = MESSAGE_DATA_LENGTH;
}


void _in_servant_routine( rtems_id timer_id, void * task_id )
{
  static Message_t message;
  rtems_id qid;
  rtems_id id = *(rtems_id *)task_id;
  IServantRunnable runnable;

  if((runnable = (IServantRunnable)_get_runnable(id, IN_QUEUE)) == NULL){
    printf("Error: No such a task (tid = %d) when creating a I-Servant\n", id);
  } else {
    qid = _get_message_queue(id, IN_QUEUE);
    _initialize_message(&message);
    runnable( & message.address, &message.size );

    if(message.size > MESSAGE_DATA_LENGTH){
      printf("Error: Message size in I-Servant exceed the MESSAGE_DATA_LENGTH\n");
      /* Warning: Resize the message size */
      message.size = MESSAGE_DATA_LENGTH;
    }

    /* Send message to the IN_QUEUE */
    message.id = id;
    _pspm_smp_message_queue_IsC( id, message.address, message.size);
  }
}

/* @brief constructing a O-servant routine for timer
 * This function act as a parameter of function rtems_timer_fire_after()
 * @param[in] id is the O-servant belonging task id
 * */
void _out_servant_routine( rtems_id timer_id, void * task_id )
{
  static Message_t message;
  rtems_id qid;
  rtems_id id = *(rtems_id *)task_id;
  OServantRunnable runnable;

  if((runnable = (OServantRunnable)_get_runnable(id, OUT_QUEUE)) == NULL){
    printf("Error: No such a task (tid = %d) when creating a O-Servant\n", id);
  } else {
    qid = _get_message_queue(id, OUT_QUEUE);

    /* Receive message from OUT_QUEUE. There may be multiple messages */
    while(1){
      _pspm_smp_message_queue_OrC( id, message.address, & message.size);

      if(message.size == 0){
        /*No more message should be received */
        break;
      }

      if(message.size > MESSAGE_DATA_LENGTH){
        printf("Error: Message size in I-Servant exceed the MESSAGE_DATA_LENGTH\n");
        /* Warning: Resize the message size */
        message.size = MESSAGE_DATA_LENGTH;
      }

      runnable( message.address, message.size );
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
  rtems_id source_id;
  rtems_id target_id;
  /* Reading data from in_queue, and writing data to out_queue*/
  rtems_id in_qid;
  rtems_id out_qid;

  /* timer for I-Servants and C-Servants */
  rtems_id in_timer_id;
  rtems_id out_timer_id;

  /* Used for I-C or C-O message passing */
  rtems_id target_id_array[TARTGET_NUM_MAX];
  rtems_id target_num;

  /* Input data address and size */
  void * in_buff;
  size_t  in_size;
  /* Output data address and size */
  Message_t message;

  /* Implementing the periodic semantics of rtems tasks */
  rtems_interval period;
  rtems_id rate_monotonic_id;

  CServantRunnable runnable;
  /* Some kernel including files must be included in this file,
   * Rather than including them in the pspm.h file */
  Thread_Control * executing;
  Scheduler_Node * base_node;
  Scheduler_EDF_SMP_Node * node;

  /* If runnable == NULL, no such a task exists */
  if((runnable = (CServantRunnable)_get_runnable(id, COMP_QUEUE)) == NULL){
    printf("Error: No such a task (tid = %d) when creating a C-Servant\n", id);
    return;
  }

  /* Connect pspm smp task with rtems task */
  /* rtems/score/Percpu.h */
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
  node->task_node = (pspm_smp_task_manager.Task_Node_array[id]);

  /* Start the message passing */
  in_qid = _get_message_queue(id, IN_QUEUE);
  out_qid = _get_message_queue(id, OUT_QUEUE);

  /* Allocate memory for message, its memory size is equal to the MESSAGE_DATA_LENGTH */
  _initialize_message(&message);

  /* Assuming that the rate monotonic object can always be created successfully */
  rtems_rate_monotonic_create( rtems_build_name( 'P', 'E', 'R', id  ), &rate_monotonic_id);
  period = pspm_smp_task_manager.Task_Node_array[id]->period;

  /* Creating timer for I-Servant and set the timer id of its Servant structure */
  rtems_timer_create( rtems_build_name('T', 'I', 'I', id), &in_timer_id );
  pspm_smp_task_manager.Task_Node_array[id]->i_servant.id = in_timer_id;
  rtems_timer_fire_after(in_timer_id, period, _in_servant_routine, &id);

  /* Creating timer for O-Servant and set the timer id of its Servant structure */
  rtems_timer_create( rtems_build_name('T', 'I', 'O', id), &out_timer_id );
  pspm_smp_task_manager.Task_Node_array[id]->o_servant.id = out_timer_id;
  rtems_timer_fire_after(out_timer_id, period, _out_servant_routine, &id);

  /* Entry the while loop of a periodic rtems task */
  while(1){
    rtems_rate_monotonic_period(rate_monotonic_id, period);

    /* I-C-O Servants in the same task communicating */
    while(1){
      _pspm_smp_message_queue_CrI(id, in_buff, &in_size);
      if( in_size == 0 ) {
        break;
      } else {
        runnable(id, in_buff, in_size, target_id_array, &target_num, message.address, &message.size);

        if( message.size == 0 ){
          printf("Warning: No message is sent from C-Servant to O-Servant, is that what you want?");
        }else if( message.size > MESSAGE_DATA_LENGTH ){
          printf("Warning: Message size is greater than defined, please confirm!");
        }else{
          /* Ensure that: Mesage from In_queue can only send to Out_queue, no matter which is the target that the programmer set */
          _pspm_smp_message_queue_CsO(id, message.address, message.size);
        }
      }
    }


    /* C-Servants communicate with other tasks */
    while(1){
      _pspm_smp_message_queue_CrC(id, &source_id, in_buff, &in_size);
      if(in_size == 0){
        /* No more Message needs to be received */
        break;
      }

      runnable(source_id, in_buff, in_size, target_id_array, &target_num, message.address, &message.size);

      if( message.size == 0 ){
        printf("Warning: No message will be sent to other tasks. Is that what you want?");
      }else if( message.size > MESSAGE_DATA_LENGTH ){
        printf("Warning: Message size is greater than defined, please confirm!");
        message.size = MESSAGE_DATA_LENGTH;
      }else{
        /* The message sender set to the current task */
        message.id = id;

        /* If the target id equal to that of current task, then the message is sent to the OUT_QUEUE of current task;
         * Otherwise, messages are sent to IN_QUEUE of other tasks, which is considered as communications among tasks.
         * */
        int32_t index;
        for( index = 0; index < target_num; index ++ ){
          target_id = target_id_array[index];
          if( target_id == id ){
            _pspm_smp_message_queue_CsO(target_id, message.address, message.size);
          }else{
            _pspm_smp_message_queue_CsC(target_id, message.address, message.size);
          }
        }
      }
    }/* End while */
  }
}


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
static void _pd2_subtasks_create(
    Task_Node *p_tnode
    )
{
  rtems_chain_initialize_empty( &p_tnode->Subtask_Node_queue );
  uint32_t min_group_deadline = p_tnode->period;
  for(int i = p_tnode->wcet; i >= 1; i--)
  {
    Subtask_Node *p_new_snode = (Subtask_Node *)malloc(sizeof(Subtask_Node));
    p_new_snode->r = myfloor((double)(i - 1) / p_tnode->utility);
    p_new_snode->d = myceil((double)i / p_tnode->utility);
    p_new_snode->b = myceil((double)i / p_tnode->utility) - myfloor((double)i / p_tnode->utility);
    /* the b(Ti) of the last subtask should be 0 */
    p_new_snode->b = (i == p_tnode->wcet) ? 0 : p_new_snode->b;
    _group_deadline_update(p_tnode->utility, p_new_snode, &min_group_deadline);

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
Task_Node_t  pspm_smp_task_create(
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
  p_tnode->wcet = RTEMS_MILLISECONDS_TO_TICKS(wcet);
  p_tnode->period = RTEMS_MILLISECONDS_TO_TICKS(period);

  /*Note that : the quantum length is presented in number of ticks */
  p_tnode->quant_wcet =  myceil((double)p_tnode->wcet / pspm_smp_task_manager.quantum_length);
  p_tnode->quant_period =  myceil((double)p_tnode->period / pspm_smp_task_manager.quantum_length);
  p_tnode->utility = (double)p_tnode->wcet / p_tnode->period;

  /* calculate the PD2 relative timing information for scheduling subtasks */
  _pd2_subtasks_create(p_tnode);

  /* insert the task node into the task node chain */
  rtems_chain_append_unprotected(&pspm_smp_task_manager.Task_Node_queue, &p_tnode->Chain);

  /* Important !!!
   * Insert the task node into the task node array for accelerating searching task node */
  if( pspm_smp_task_manager.Task_Node_array[task_id] != NULL){
    printf("Error: current task exists, please use unique task id\n");
  }else{
    pspm_smp_task_manager.Task_Node_array[task_id] = p_tnode;
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
    IServantRunnable i_runnable,
    CServantRunnable c_runnable,
    OServantRunnable o_runnable
)
{
  rtems_id qid;
  rtems_name name;
  rtems_status_code status;

  /* Set the Servant runnable of a Periodic task */
  ((Task_Node *)task)->i_servant.runnable = i_runnable;
  ((Task_Node *)task)->c_servant.runnable = c_runnable;
  ((Task_Node *)task)->o_servant.runnable = o_runnable;

  /* Create IN_QUEUE, COMP_QUEUE, and OUT_QUEUE in each periodic task
   * Notice: All message receive and send operations perform without blocking
   * This requires that RTEMS_NO_WAIT option in rtems message passing mechanism.
   * */

  /* rtems_status_code rtems_message_queue_create(rtems_name name, uint32_t count, size_t max_message_size, rtems_attribute attribute_set, rtems_id *id); */
  /**/
  name = rtems_build_name('Q','U','I',((Task_Node*)task)->id);
  status = rtems_message_queue_create(name, MESSAGE_BUFFER_LENGTH, MESSAGE_DATA_LENGTH, RTEMS_DEFAULT_ATTRIBUTES, &qid );
  if(status == RTEMS_SUCCESSFUL){
    ((Task_Node *)task)->i_queue_id= qid;
  }else{
    printf("Error: The creation of In QUEUE in task %d failed\n", ((Task_Node *)task)->id);
  }

  name = rtems_build_name('Q','U','C',((Task_Node*)task)->id);
  status = rtems_message_queue_create(name, MESSAGE_BUFFER_LENGTH, MESSAGE_DATA_LENGTH, RTEMS_DEFAULT_ATTRIBUTES, &qid );
  if(status == RTEMS_SUCCESSFUL){
    ((Task_Node *)task)->c_queue_id= qid;
  }else{
    printf("Error: The creation of In QUEUE in task %d failed\n", ((Task_Node *)task)->id);
  }

  name = rtems_build_name('Q','U','O',((Task_Node*)task)->id);
  status = rtems_message_queue_create(name, MESSAGE_BUFFER_LENGTH, MESSAGE_DATA_LENGTH, RTEMS_DEFAULT_ATTRIBUTES, &qid );
  if(status == RTEMS_SUCCESSFUL){
    ((Task_Node *)task)->o_queue_id= qid;
  }else{
    printf("Error: The creation of In QUEUE in task %d failed\n", ((Task_Node *)task)->id);
  }

}



/***********************************************************/
/*        Message Passing Interfaces Implementation        */
/***********************************************************/

inline rtems_status_code _message_queue_send_message(rtems_id qid, const void * in, size_t size_in)
{
  return rtems_message_queue_send(qid, in ,size_in);
}


inline rtems_status_code _message_queue_receive_message(rtems_id qid, void * out, size_t * size_out)
{
  /* No blocking receiving.
   * Since I-Servant has higher prior execution than C-Servants,
   * the In_message_queue of current task has at least one message.
   * */
  return rtems_message_queue_receive(qid, out, size_out, RTEMS_NO_WAIT, RTEMS_NO_TIMEOUT);
}





static void _pspm_smp_message_queue_CsC(
    tid_t id,
    const void *buff,
    size_t size
    )
{
  rtems_id comp_qid;

  if( buff == NULL ){
    printf("Error: The target address of message send is NULL\n");
    return;
  }

  if( size <= 0 || size > MESSAGE_DATA_LENGTH ){
    printf("Error: The specific size of message is out of defined length\n");
    return ;
  }

  comp_qid = _get_message_queue(id, COMP_QUEUE);
  if( comp_qid != -1 ){
    if(RTEMS_SUCCESSFUL == _message_queue_send_message(comp_qid, buff, size)){
      return;
    }else{
      printf("Error: Message Send Failed\n");
      return ;
    }
  } else{
    printf("Error: No such a task (tid = %d) is found\n", id);
    return ;
  }
}


static void _pspm_smp_message_queue_CrC(
    tid_t id,
    tid_t *source_id,
    void *buff,
    size_t *size
    )
{
  rtems_id comp_qid;

  /* get the id of comp_queue in current task */
  comp_qid = _get_message_queue(id, COMP_QUEUE);
  if( comp_qid != -1 ){
    if(RTEMS_SUCCESSFUL == _message_queue_receive_message(comp_qid, buff, size)){
      *source_id = ((Message_t *)buff)->id;
    }else{
      printf("Error: Message Received Failed\n");
      *size = 0;
    }
  }else{
    printf("Error: No such a task (tid = %d) is found\n", id);
    *size = 0;
  }
}

static void _pspm_smp_message_queue_IsC( tid_t id, const void *in, size_t size_in)
{
  rtems_id in_qid;

  in_qid = _get_message_queue(id, IN_QUEUE);
  if( in_qid != -1 ){
    if(RTEMS_SUCCESSFUL == _message_queue_send_message(in_qid, in, size_in)){
      return;
    }else{
      printf("Error: Message Send Failed\n");
    }
  }else{
    printf("Error: No such a task (tid = %d) is found\n", id);
  }

}



static void _pspm_smp_message_queue_OrC( tid_t id, void *out, size_t * size_out)
{
  rtems_id in_qid;

  in_qid = _get_message_queue(id, OUT_QUEUE);
  if( in_qid != -1 ){
    if(RTEMS_SUCCESSFUL == _message_queue_receive_message(in_qid, out, size_out)){
      return;
    }else{
      printf("Error: Message Received Failed\n");
      *size_out  = 0;
    }
  } else{
      printf("Error: No such a task (tid = %d) is found\n", id);
      *size_out  = 0;
  }


}


static void _pspm_smp_message_queue_CrI(tid_t id, void * in, size_t * size_in)
{
  rtems_id in_qid;

  in_qid = _get_message_queue(id, IN_QUEUE);
  if( in_qid != -1 ){
    if(RTEMS_SUCCESSFUL == _message_queue_receive_message(in_qid, in, size_in)){
      return;
    }else{
      printf("Error: Message Received Failed\n");
      *size_in = 0;
    }
  } else{
      printf("Error: No such a task (tid = %d) is found\n", id);
      *size_in = 0;
  }
}

static void _pspm_smp_message_queue_CsO(tid_t id, const void * out, size_t size_in)
{
  rtems_id out_qid;

  out_qid = _get_message_queue(id, OUT_QUEUE);
  if( out_qid != -1 ){
    if(RTEMS_SUCCESSFUL == _message_queue_send_message(out_qid, out, size_in)){
      return;
    }else{
      printf("Error: Message Send Failed\n");
    }
  }else{
    printf("Error: No such a task (tid = %d) is found\n", id);
  }

}



