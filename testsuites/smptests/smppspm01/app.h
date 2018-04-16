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
 * Reading data from environment, and send them as message to the C-servant
 * This runnable will be invoked when timer fires
 * @param[in out] msg The input message and output message
 * */
void i_servant_0(pspm_smp_message * msg);
void i_servant_1(pspm_smp_message * msg);

/* @brief C-Servant runnable
 * Reading data from msg, and updating the data in the msg then writing to same message
 * This runnable will be invoked by smp scheduler Pfair
 * @param[in out] msg The input message and output message
 * */
void c_servant_0(pspm_smp_message * msg);
void c_servant_1(pspm_smp_message * msg);

/* @brief O-Servant runnable
 * Reading data from msg, and update the actuator
 * This runnable will be invoked when timer fires
 * @param[in out] msg The input message and output message
 * */
void o_servant_0(pspm_smp_message * msg);
void o_servant_1(pspm_smp_message * msg);


#endif

