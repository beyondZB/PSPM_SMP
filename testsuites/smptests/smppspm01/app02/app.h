/*
 * This file contains all the names of app runnables
 * And they are implemented in the app.c
 *
 * */

#ifndef __APP_H__
#define __APP_H__

#include "pspm.h"

/* Used in main.c */
#define PERIOD_TASK_NUM 5

/**************************/
/* Accelerator Pedal Task */
/**************************/
void accSensor(pspm_smp_message *msg);
void accController(pspm_smp_message *msg);
void accActuator(pspm_smp_message *msg);

/**************************/
/*     Throttle Task      */
/**************************/
void throttleSensor(pspm_smp_message *msg);
void throttleController( pspm_smp_message * msg );
void throttleActuator(pspm_smp_message *msg);

/**************************/
/*  Base Fuel Mass Task   */
/**************************/
void baseFMSensor(pspm_smp_message *msg);
void baseFMController( pspm_smp_message * msg );
void baseFMActuator(pspm_smp_message *msg);

/****************************************************/
/* Transient Fuel Mass Task and Total Fuel Mass Task*/
/****************************************************/
void transFMSensor(pspm_smp_message *msg);
void transFMController( pspm_smp_message * msg );
void inJectionTimeActuator(pspm_smp_message *msg);

/****************************************************/
/*            Ignition Timing Task                  */
/****************************************************/
void ignitionSensor(pspm_smp_message *msg);
void ignitionController( pspm_smp_message * msg );
void ignitionActuator(pspm_smp_message *msg);


#endif

