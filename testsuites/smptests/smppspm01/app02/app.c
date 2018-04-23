/*
 * This file implements the app interfaces defined in app.h
 * These functions are runnables for implementing app in multicore pspm
 *
 * */
#include "app.h"

static void my_delay(uint32_t ms)
{
  rtems_interval start, stop;
  start = rtems_clock_get_uptime_nanoseconds();
  do {
    stop = rtems_clock_get_uptime_nanoseconds();
  } while ( (stop - start) < ms);
}

/**************************/
/* Accelerator Pedal Task */
/**************************/

void accSensor(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;

  data_array = (uint32_t *)msg->address;

  data_array[0] = 1;
  data_array[1] = 2;
  msg->size = 2;
  printf("Task Accelerator obtains input: including %d messages\n", msg->size);
}

void accController(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;
  pspm_status_code status;

  data_array = (uint32_t *)msg->address;

  printf("C-Servant of task accelerator runs\n");

  /* Sum the message data from sensor, and send the results */
  data_array[0] += data_array[1] ;
  msg->size = 1;

  /* Send the updated message to the Throttle Controller */
  status = pspm_smp_message_queue_send(1, msg);
  if(SATISFIED == status){
      printf("Messages of Task 0 send successfully\n");
  }else{
      printf("Messages of Task 0 send failed\n");
  }
}

void accActuator(pspm_smp_message *msg)
{
  /* Do nothing */
}






/**************************/
/*     Throttle Task      */
/**************************/


void throttleSensor(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;

  data_array = (uint32_t *)msg->address;

  data_array[0] = 11;
  data_array[1] = 12;

  /* Sum the message data, and send the result */
  data_array[0] += data_array[1];
  msg->size = 1;

  printf("Task Throttle obtains input: including %d messages\n", msg->size);
}

void throttleController( pspm_smp_message * msg )
{
  int i;
  uint32_t   *data_array;
  pspm_smp_message message;
  pspm_status_code status;
  uint32_t * data_receive;
  uint32_t  data_port;

  data_array = (uint32_t *)msg->address;
  pspm_smp_message_initialize(&message);

  printf("C-Servant of Throttle Task runs\n");

  for(i = 0; i < msg->size; ++i){
      data_array[i] *=100;
  }

  while(1){
      status = pspm_smp_message_queue_receive(&message);
      if(UNSATISFIED == status){
          printf("Task controller has no message received\n");
          break;
      }
      data_receive = (uint32_t *)message.address;

      printf("Task controller receives messages from task %d\n", message.sender);

      /* Obtain the newest message data;
       * otherwise, the data_port variable have the latest message data already
       * */
      switch(message.sender){
        case 0:
          data_port = data_receive[0];
          printf("Message updated from Accelerator Task\n");
          break;
        default:
          printf("Wrong message from %d\n", message.sender);
      }
  }

  /* message data processing */
  for(i = 0; i < msg->size; ++i){
      data_array[i] += data_port;
  }
}

void throttleActuator(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;
  data_array = (uint32_t *)msg->address;

  printf("Task 1 output %d messages: ",msg->size);
  for( i = 0; i < msg->size; ++i){
    printf("%u\t",  data_array[i]);
  }
  printf("\n");
}



/**************************/
/*  Base Fuel Mass Task   */
/**************************/


void baseFMSensor(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;

  data_array = (uint32_t *)msg->address;

  data_array[0] = 11;

  msg->size = 1;

  printf("Task Base Fuel Mask obtains input: including %d messages\n", msg->size);
}

void baseFMController( pspm_smp_message * msg )
{
  int i;
  uint32_t   *data_array;
  pspm_status_code status;

  data_array = (uint32_t *)msg->address;

  printf("C-Servant of Base Fuel Mass Task runs\n");

  for(i = 0; i < msg->size; ++i){
      data_array[i] *=100;
  }

  /* send message to Transient Fuel Mass Task */
  status = pspm_smp_message_queue_send(3, msg);
  if(SATISFIED == status){
      printf("Task 2 send message to task 3 successfully\n");
  }else{
      printf("Task 2 send message to task 3 failed\n");
  }

  /* send message to Ignition Timing Task */
  status = pspm_smp_message_queue_send(4, msg);
  if(SATISFIED == status){
      printf("Task 2 send message to task 3 successfully\n");
  }else{
      printf("Task 2 send message to task 3 failed\n");
  }

}

void baseFMActuator(pspm_smp_message *msg)
{
  /* Do nothing */
}



/****************************************************/
/* Transient Fuel Mass Task and Total Fuel Mass Task*/
/****************************************************/


void transFMSensor(pspm_smp_message *msg)
{
  /* Do Nothing */
  int i;
  uint32_t   *data_array;
  data_array = (uint32_t *)msg->address;

  for(i = 0; i < MESSAGE_DATA_LENGTH; ++i ){
    data_array[i] = 0;
  }
  msg->size = 0;
}

void transFMController( pspm_smp_message * msg )
{
  int i;
  uint32_t   *data_array;
  pspm_status_code status;
  pspm_smp_message message;
  uint32_t * data_receive;
  uint32_t  data_port;

  data_array = (uint32_t *)msg->address;
  pspm_smp_message_initialize(&message);

  printf("C-Servant of Transient Fuel Mass Task runs\n");

  while(1){
      status = pspm_smp_message_queue_receive(&message);
      if(UNSATISFIED == status){
          printf("Task controller has no message received\n");
          break;
      }
      data_receive = (uint32_t *)message.address;

      printf("Task controller receives messages from task %d\n", message.sender);

      /* Obtain the newest message data;
       * otherwise, the data_port variable have the latest message data already
       * */
      switch(message.sender){
        case 2:
          data_port = data_receive[0];
          printf("Message updated from Base Fuel Mass Task\n");
          break;
        default:
          printf("Wrong message from %d\n", message.sender);
      }
  }

  /* message data processing */
  for(i = 0; i < msg->size; ++i){
      data_array[0] += data_port;
  }

}

void inJectionTimeActuator(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;
  data_array = (uint32_t *)msg->address;

  printf("Task 3 output %d messages: ",msg->size);
  for( i = 0; i < msg->size; ++i){
    printf("%u\t",  data_array[i]);
  }
  printf("\n");
}

/****************************************************/
/*            Ignition Timing Task                  */
/****************************************************/


void ignitionSensor(pspm_smp_message *msg)
{
  /* Do Nothing */
  int i;
  uint32_t   *data_array;
  data_array = (uint32_t *)msg->address;

  for(i = 0; i < MESSAGE_DATA_LENGTH; ++i ){
    data_array[i] = 0;
  }
  msg->size = 0;
}

void ignitionController( pspm_smp_message * msg )
{
  int i;
  uint32_t   *data_array;
  pspm_status_code status;
  pspm_smp_message message;
  uint32_t * data_receive;
  uint32_t  data_port;

  data_array = (uint32_t *)msg->address;
  pspm_smp_message_initialize(&message);

  printf("C-Servant of Ignition Timing Task runs\n");

  while(1){
      status = pspm_smp_message_queue_receive(&message);
      if(UNSATISFIED == status){
          printf("Task controller has no message received\n");
          break;
      }
      data_receive = (uint32_t *)message.address;

      printf("Task controller receives messages from task %d\n", message.sender);

      /* Obtain the newest message data;
       * otherwise, the data_port variable have the latest message data already
       * */
      switch(message.sender){
        case 2:
          data_port = data_receive[0];
          printf("Message updated from Base Fuel Mass Task\n");
          break;
        default:
          printf("Wrong message from %d\n", message.sender);
      }
  }

  /* message data processing */
  data_array[0] += data_port;
  msg->size = 1;

}

void ignitionActuator(pspm_smp_message *msg)
{
  int i;
  uint32_t   *data_array;
  data_array = (uint32_t *)msg->address;

  printf("Task 4 output %d messages: ",msg->size);
  for( i = 0; i < msg->size; ++i){
    printf("%u\t",  data_array[i]);
  }
  printf("\n");
}

