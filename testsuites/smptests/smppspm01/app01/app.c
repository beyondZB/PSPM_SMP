/*
 * This file implements the app interfaces defined in app.h
 * These functions are runnables for implementing app in multicore pspm
 *
 * */
#include "app.h"

static void my_delay(int ticks)
{
  rtems_interval start, stop;
  start = rtems_clock_get_ticks_since_boot();
  do {
    stop = rtems_clock_get_ticks_since_boot();
  } while ( (stop - start) < ticks  );
}

void i_servant_0(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;

  data_array = (uint32_t *)msg->address;

  data_array[0] = 10;
  data_array[1] = 20;
  msg->size = 2;
  printf("Task 0 obtains input: including %d messages\n", msg->size);
  /* the message sender will be setted automatically by the runtime */
  for( i = 0; i < msg->size; ++i){
    printf("%u\t",  data_array[i]);
  }
}

void i_servant_1(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;

  data_array = (uint32_t *)msg->address;

  data_array[0] = 100;
  msg->size = 1;
  printf("Task 1 obtains input: including %d messages\n", msg->size);
  /* the message sender will be setted automatically by the runtime */
  for( i = 0; i < msg->size; ++i){
    printf("%u\t", data_array[i]);
  }
}

void c_servant_0( pspm_smp_message * msg )
{
  int i;
  uint32_t   *data_array;
  pspm_status_code status;

  data_array = (uint32_t *)msg->address;

  printf("C-Servant of Task 0 runs\n");

  /* Obtaining message from IN_QUEUE and send them to OUT_QUEUE */
  for(i = 0; i < msg->size; ++i){
      data_array[i] *=100;
  }

  /* Send the updated message to the COMP_QUEUE of task 1 */
//  status = pspm_smp_message_queue_send(1, msg);
//  if(SATISFIED == status){
//      printf("Messages of Task 0 send successfully\n");
//  }else{
//      printf("Messages of Task 0 send failed\n");
//  }

  for(int j = 0; j < 65; j++)
  {
      rtems_test_busy_cpu_usage(0, 1e6);  //busy for 90000 ns
//      my_delay(1);
      printf("&");
  }
}

void c_servant_1( pspm_smp_message * msg )
{
  int i;
  uint32_t   *data_array;
  pspm_smp_message message;
  pspm_status_code status;
  data_array = (uint32_t *)msg->address;

  /* Initialize a local message, whose data can be used global */
  pspm_smp_message_initialize(&message);

  /* Obtaining message from IN_QUEUE and multiple 100 */
  for(i = 0; i < msg->size; ++i){
    data_array[i] *= 100;
  }

  /* Obtaining message from COMP_QUEUE */
//  while(1){
//      status = pspm_smp_message_queue_receive(&message);
//      if(UNSATISFIED == status){
//          printf("Task 1 has no message received\n");
//          break;
//      }
//      uint32_t * data_receive;
//      data_receive = (uint32_t *)message.address;
//
//      printf("Task 1 receives messages, and the sender is %d\n", message.sender);
//
//      /* Message from C-Servant 0 has two elements */
//      if( message.sender == 0 ){
//          for(i = 0; i < msg->size; ++i){
//              data_array[i] = data_array[i] * data_receive[0] - data_receive[1];
//          }
//      }
//  }

  for(int j = 0; j < 50; j++)
  {
      rtems_test_busy_cpu_usage(0, 1e6);  //busy for 90000 ns
//      my_delay(1);
      printf("@");
  }
}


void o_servant_0(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;
  data_array = (uint32_t *)msg->address;

  printf("Task 0 output: including %d messages\n",msg->size);
  for( i = 0; i < msg->size; ++i){
    printf("%u\t",  data_array[i]);
  }
}

void o_servant_1(pspm_smp_message * msg)
{
  int i;
  uint32_t   *data_array;
  data_array = (uint32_t *)msg->address;

  printf("Task 1 output: including %d messages\n",msg->size);
  for( i = 0; i < msg->size; ++i){
    printf("%u\t",  data_array[i]);
  }
}

