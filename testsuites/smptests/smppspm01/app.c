/*
 * This file implements the app interfaces defined in app.h
 * These functions are runnables for implementing app in multicore pspm
 *
 * */
#include "app.h"

void i_servant_0(void * data_isc, size_t *size_isc)
{
  * data_isc = 10;
  * size_isc = 1;
}

void i_servant_1(void * data_isc, size_t *size_isc)
{
  * data_isc = 20;
  * size_isc = 1;
}

void c_servant_0(
    tid_t source_id,
    void *data_cri,
    size_t size_cri,
    tid_t *target_id,
    int32_t *target_num,
    void *data_cso,
    size_t *size_cso
)
{
  int input_data, i;
  switch(source_id){
    /* Task ids start from zero and are sequential */
    case 0:
      for( i = 0; i < size_cri; ++i ){
        input_data = *(int *)data_cri;

        /* Do Something Here */
        input_data *= 2;

        /* Output the processing result */
        *data_cso = input_data;
        data_cso += 1;
      }

      *size_cso = i+1;
      *target_id = 0;
      break;
    case 1:
      for( i = 0; i < size_cri; ++i ){
        input_data = *(int *)data_cri;

        /* Do Something Here */
        input_data *= 4;

        /* Output the processing result */
        *data_cso = input_data;
        data_cso += 1;
      }

      *size_cso = i+1;
      *target_id = 0; /* send message to task 0, the message will be sent to OUT_QUEUE of task 0 */
      //*target_id = 1;  /* Otherwise, the message will be sent to COMP_QUEUE of task 1 */
      break;
    default:
      print("Error: No such kind of communication relationship defined\n");

  }
}

void c_servant_1(
    tid_t source_id,
    void *data_cri,
    size_t size_cri,
    tid_t *target_id,
    int32_t *target_num,
    void *data_cso,
    size_t *size_cso
)
{
  int input_data, i;
  *size_cso = 0;
  switch(source_id){
    /* Task ids start from zero and are sequential */
    case 1:
      for( i = 0; i < size_cri; ++i, ++data_cso, ++data_cri ){
        input_data = *(int *)data_cri;

        /* Do Something Here */
        input_data *= 10;

        /* Output the processing result */
        *data_cso = input_data;
      }

      *size_cso = i+1;

      /* Send message to task 0 and 1 */
      *target_id = 0;
      *(target_id + 1) = 1;
      break;
    default:
      print("Error: No such kind of communication relationship defined\n");
  }
}


void o_servant_0(void *data_orc, size_t size_orc)
{
  int i;
  printf("Task 0 Output:\n");
  for( i = 0; i < size_orc, ++i, ++data_orc){
    printf("No.%d %d\n", i+1, *(int *)data_orc);
  }
}

void o_servant_1(void *data_orc, size_t size_orc)
{
  int i;
  printf("Task 1 Output:\n");
  for( i = 0; i < size_orc, ++i, ++data_orc){
    printf("No.%d %d\n", i+1, *(int *)data_orc);
  }

}
