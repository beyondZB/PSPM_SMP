/*
 * This is head file of multicore PSPM programming model
 * Contents in this file contains data structures and programming interfaces
 * */

#ifndef __PSPM_H__
#define __PSPM_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems.h>
#include <rtems/cpuuse.h>

#include "rtems/score/threadimpl.h"
#include "rtems/score/scheduleredfsmp.h"

#include "tmacros.h"
#include "test_support.h"


/* if define PSPM_DEBUG lots of debug information will be printed. */
//#define PSPM_DEBUG

/* 1000 us == 1 tick, not used here */
#define CONFIGURE_MICROSECONDS_PER_TICK 1000

/* user define the quantum length in ms */
#define QUANTUM_LENGTH_IN_MS 8
/* In number of ticks */
#define QUANTUM_LENGTH ((QUANTUM_LENGTH_IN_MS * 1000) / CONFIGURE_MICROSECONDS_PER_TICK )

#define TASK_NUM_MAX 10

/* The length of message data, in number of word */
#define MESSAGE_DATA_LENGTH sizeof(pspm_smp_message)

/*One message is allowed to be sent to at most TARGET_NUM_MAX tasks */
#define TARGET_NUM_MAX 20

/* The size of message buffer in each message queue */
#define MESSAGE_BUFFER_LENGTH 20

typedef struct _Message{
  void *address; /*The start address of the Message*/
  size_t size; /*The length of Message */
  tid_t sender; /* the task id of Message sender */
}pspm_smp_message;


/* functions used in init.c */
void pspm_smp_task_manager_initialize( uint32_t task_num, uint32_t quanta);

rtems_task Init( rtems_task_argument argument);

rtems_task _comp_servant_routine( rtems_task_argument argument );

uint32_t GCD(uint32_t a, uint32_t b);

uint32_t LCM(uint32_t a, uint32_t b);

void Loop( void );

void rtems_success( void );

/*
 *  Handy macros and static inline functions
 */
/* Type of Tasks in multicore PSPM
 * Currently, only the periodic task can be created. 2018/4/10
 * */
typedef enum{
    PERIOD_TASK    =1,
    APERIODIC_TASK =2,
    SPORADIC_TASK  =3
}Task_type;

/* Type of Queue in multicore PSPM
 * */
typedef enum{
    IN_QUEUE     =1,
    COMP_QUEUE   =2,
    OUT_QUEUE    =3
}Queue_type;

typedef enum{
    SATISFIED,
    UNSATISFIED
}pspm_status_code;

typedef void* Task_Node_t;


/* @brief Servant runnable
 * I-Servant: Reading data from environment, and send them as message to the C-servant
 * C-Servant: Reading data from msg, and updating the data in the msg then writing to same message
 * O-Servant: Reading data from msg, and update the actuator
 * This runnable will be invoked when timer fires
 * @param[in out] msg The input message and output message
 * */
typedef void (*ServantRunnable)(pspm_smp_message * msg);

/* @brief Message send API
 * send message address to the COMP_QUEUE of target task
 * @param[in] id The id of message receiver task
 * @param[out] msg The address of sent message
 * */
pspm_status_code pspm_smp_message_queue_send(tid_t id, pspm_smp_message * msg);

/* @brief Message Receive API
 * Receive message from the IN_QUEUE of current task
 * @param[in out] msg The address of received message
 * */
pspm_status_code pspm_smp_message_queue_receive(pspm_smp_message * msg);

void pspm_smp_message_initialize(pspm_smp_message *msg);


/* @brief Task Creation API
 * User API, which is used to create the task
 * @param[in] task_id is the user defined task id, which is required to be unique and sequential.
 * @param[in] task_type defines the type of task, including periodic, aperiodic, sporadic
 * @param[in] wcet is worst case execution time (in number of ms) of this task
 * @param[in] period is the period (in number of ms) if task type is periodic, otherwise is the minimal arrival interval
 * */
Task_Node_t pspm_smp_task_create(
  tid_t task_id,
  Task_type task_type,
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
  ServantRunnable i_runnable,
  ServantRunnable c_runnable,
  ServantRunnable o_runnable
);

/* PSPM SMP programs entry
 * */
void main();



#endif


