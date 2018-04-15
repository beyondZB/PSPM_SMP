/*
 * This file contains all the names of app runnables
 * And they are implemented in the app.c
 *
 * */

#ifndef __APP_H__
#define __APP_H__

#include "pspm.h"

/* Used in main.c */
#define PERIOD_TASK_NUM 2

/* @brief I-Servant runnable
 * Reading data from environment device(e.g. pin), and writing to the data_sic.
 * This runnable will be invoked when timer fires.
 * @param[out] data_isc is the start address of sending message
 * @param[out] size_isc is the length of the message
 * */

//typedef void (*IServantRunnable)(
//  void *data_isc,
//  size_t *size_isc
//);
//
void i_servant_0(void * data_isc, size_t *size_isc);
void i_servant_1(void * data_isc, size_t *size_isc);

/* @brief C-Servant runnable
 * Reading data from data_cri, and writing to the data_cso.
 * This runnable will be invoked when C-servant is scheduled
 * @param[in] source_id is the task id of message sender
 * @param[in] data_cri is the start address of received message
 * @param[in] size_cri is the length of received message
 * @param[out] target_id is the task id of message receiver
 * @param[out] target_num is the number of message receiver
 * @param[out] data_cso is the start address of sent message
 * @param[out] size_cso is the length of sent message
 * */

//typedef void (*CServantRunnable)(
//    tid_t source_id,  /* The message sender, there is only one sender*/
//  void *data_cri,
//  size_t size_cri,
//  tid_t *target_id,  /* The array of target tasks */
//  int32_t *target_num; /* There could multiple target */
//  void *data_cso,
//  size_t *size_cso
//);
void c_servant_0(tid_t source_id, void *data_cri, size_t size_cri, tid_t *target_id, int32_t *target_num, void *data_cso, size_t *size_cso);
void c_servant_1(tid_t source_id, void *data_cri, size_t size_cri, tid_t *target_id, int32_t *target_num, void *data_cso, size_t *size_cso);

/* @brief O-Servant runnable
 * Reading data from data_orc, and writing to the environment device.
 * This runnable will be invoked when timer fires
 * @param[in] data_orc is the start address of received message
 * @param[in] size_orc is the length of received message
 * */
//typedef void (*OServantRunnable)(
//  void *data_orc,
//  size_t size_orc
//);
void o_servant_0(void *data_orc, size_t size_orc);
void o_servant_1(void *data_orc, size_t size_orc);


#endif

