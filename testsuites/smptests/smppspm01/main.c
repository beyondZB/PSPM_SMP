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
  task_nodes[0] = pspm_smp_task_create(0, PERIOD_TASK, 5, 200);
  task_nodes[1] = pspm_smp_task_create(1, PERIOD_TASK, 6, 100);

  /* The creation of Servant in specific tasks */
  pspm_smp_servant_create(task_nodes[0], i_servant_0, c_servant_0, o_servant_0);
  pspm_smp_servant_create(task_nodes[1], i_servant_1, c_servant_1, o_servant_1);

}


