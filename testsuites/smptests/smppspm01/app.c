/*
 * This file implements the app interfaces defined in app.h
 * These functions are runnables for implementing app in multicore pspm
 *
 * */
#include "app.h"

void i_servant_0(pspm_smp_message *msg)
{
  int i;

  msg->address[0] = 10;
  msg->address[1] = 20;
  msg->size = 2;
  /* the message sender will be setted automatically by the runtime */

  printf("Task 0 Input:\n");
  for( i = 0; i < msg->size; ++i){
    printf("No.%d: %u\n", i+1, msg->address[i]);
  }
}

void i_servant_1(pspm_smp_message *msg)
{
  int i;

  msg->address[0] = 100;
  msg->size = 1;
  /* the message sender will be setted automatically by the runtime */

  printf("Task 1 Input:\n");
  for( i = 0; i < msg->size; ++i){
    printf("No.%d: %u\n", i+1, msg->address[i]);
  }
}

void c_servant_0( pspm_smp_message * msg )
{
  int i;
  printf("Now is in c_servant_0\n");

  /* Obtaining message from IN_QUEUE and send them to OUT_QUEUE */
  for(i = 0; i < msg->size; ++i){
      msg->address[i] *=100;
  }

  /* Send the updated message to the COMP_QUEUE of task 1 */
  pspm_smp_message_queue_send(1, msg);


}

void c_servant_1( pspm_smp_message * msg )
{
  int i;
  pspm_smp_message message;
  pspm_status_code sc;
  printf("Now is in c_servant_0\n");

  /* Obtaining message from IN_QUEUE and send them to OUT_QUEUE */
  for(i = 0; i < msg->size; ++i){
    msg->address[i] *=100;
  }

  while(sc = pspm_smp_message_queue_receive(&message)){
      if(sc == UNSATISFIED){
          break;
      }
      printf("The message sender is %d\n", message.sender);
      for(i = 0; i < msg->size; ++i){
          msg->address[i] = msg->address[i] * message.address[0] - message.address[1];
      }
  }

}


void o_servant_0(pspm_smp_message *msg)
{
  int i;
  printf("Task 0 Output:\n");
  for( i = 0; i < msg->size; ++i){
    printf("No.%d: %u\n", i+1, msg->address[i]);
  }
}

void o_servant_1(pspm_smp_message * msg)
{
  int i;
  printf("Task 1 Output:\n");
  for( i = 0; i < msg->size; ++i){
    printf("No.%d: %u\n", i+1, msg->address[i]);
  }
}
