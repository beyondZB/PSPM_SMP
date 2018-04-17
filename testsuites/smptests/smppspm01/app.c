/*
 * This file implements the app interfaces defined in app.h
 * These functions are runnables for implementing app in multicore pspm
 *
 * */
#include "app.h"

void i_servant_0(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;

  data_array = (uint32_t *)msg->address;

  data_array[0] = 10;
  data_array[1] = 20;
  msg->size = 2;
  /* the message sender will be setted automatically by the runtime */

  printf("Task 0 Input:\n");
  for( i = 0; i < msg->size; ++i){
    printf("No.%d: %u\n", i+1, data_array[i]);
  }
}

void i_servant_1(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;

  data_array = (uint32_t *)msg->address;

  data_array[0] = 100;
  msg->size = 1;
  /* the message sender will be setted automatically by the runtime */

  printf("Task 1 Input:\n");
  for( i = 0; i < msg->size; ++i){
    printf("No.%d: %u\n", i+1, data_array[i]);
  }
}

void c_servant_0( pspm_smp_message * msg )
{
  int i;
  uint32_t   *data_array;

  data_array = (uint32_t *)msg->address;

  printf("Now is in c_servant_0\n");

  /* Obtaining message from IN_QUEUE and send them to OUT_QUEUE */
  for(i = 0; i < msg->size; ++i){
      data_array[i] *=100;
  }

  /* Send the updated message to the COMP_QUEUE of task 1 */
  pspm_smp_message_queue_send(1, msg);


}

void c_servant_1( pspm_smp_message * msg )
{
  int i;
  uint32_t   *data_array;
  pspm_smp_message message;
  pspm_status_code sc;
  data_array = (uint32_t *)msg->address;

  /* Initialize a local message, whose data can be used global */
  pspm_smp_message_initialize(&message);

  printf("Now is in c_servant_0\n");
  /* Obtaining message from IN_QUEUE and multiple 100 */
  for(i = 0; i < msg->size; ++i){
    data_array[i] *= 100;
  }

  /* Obtaining message from COMP_QUEUE */
  while(sc = pspm_smp_message_queue_receive(&message)){
      if(sc == UNSATISFIED){
          break;
      }
      uint32_t * data_receive;
      data_receive = (uint32_t *)message.address;

      printf("The message sender is %d\n", message.sender);

      /* Message from C-Servant 0 has two elements */
      if( message.sender == 0 ){
          for(i = 0; i < msg->size; ++i){
              data_array[i] = data_array[i] * data_receive[0] - data_receive[1];
          }
      }
  }
}


void o_servant_0(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;
  data_array = (uint32_t *)msg->address;

  printf("Task 0 Output:\n");
  for( i = 0; i < msg->size; ++i){
    printf("No.%d: %u\n", i+1, data_array[i]);
  }
}

void o_servant_1(pspm_smp_message * msg)
{
  int i;
  uint32_t   *data_array;
  data_array = (uint32_t *)msg->address;

  printf("Task 1 Output:\n");
  for( i = 0; i < msg->size; ++i){
    printf("No.%d: %u\n", i+1, data_array[i]);
  }
}
