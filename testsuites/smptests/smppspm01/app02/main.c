/*
 * This file is the entry for design application in PSPM programming model
 *
 * */

#include "app.h"
#include "pspm.h"

//Task_Node_t  pspm_smp_task_create( tid_t task_id, Task_Type task_type, int64_t wcet, int64_t period);

//void pspm_smp_servant_create( Task_Node_t task, IServantRunnable i_runnable, CServantRunnable c_runnable, OServantRunnable o_runnable);

void main()
{

  Task_Node_t task_nodes[PERIOD_TASK_NUM];

  /* The creation of periodic tasks */
  /* The task start from 0, since every task will be mapped into a element in a array for convenient search */
  /* The time parameters including wcet and period are in number of microseconds */
  task_nodes[0] = pspm_smp_task_create(0, PERIOD_TASK, 400,  10000);
  task_nodes[1] = pspm_smp_task_create(1, PERIOD_TASK, 3000, 10000);
  task_nodes[2] = pspm_smp_task_create(2, PERIOD_TASK, 1500, 10000);
  task_nodes[3] = pspm_smp_task_create(3, PERIOD_TASK, 2800, 10000);
  task_nodes[4] = pspm_smp_task_create(4, PERIOD_TASK, 2400, 10000);

  /* The creation of Servant in specific tasks */
  pspm_smp_servant_create(task_nodes[0], accSensor, accController, accActuator);
  pspm_smp_servant_create(task_nodes[1], throttleSensor, throttleController, throttleActuator);
  pspm_smp_servant_create(task_nodes[2], baseFMSensor, baseFMController, baseFMActuator);
  pspm_smp_servant_create(task_nodes[3], transFMSensor, transFMController, inJectionTimeActuator);
  pspm_smp_servant_create(task_nodes[4], ignitionSensor, ignitionController, ignitionActuator);

}


