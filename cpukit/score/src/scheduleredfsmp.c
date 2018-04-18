/**
 * @file
 *
 * @brief EDF SMP Scheduler Implementation
 *
 * @ingroup ScoreSchedulerSMPEDF
 */

/*
 * Copyright (c) 2017 embedded brains GmbH.
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
  #include "config.h"
#endif

#include <rtems/score/scheduleredfsmp.h>
#include <rtems/score/schedulersmpimpl.h>

/* Important !!!
 * @brief This is the core structure to connect PSPM SMP to rtems */
PSPM_SMP pspm_smp_task_manager;

uint64_t _pspm_smp_create_priority(double utility, uint32_t d, uint32_t b, uint32_t g)
{
    uint64_t priority = 0;
    uint32_t bit_mask_low_31 = 0x7fffffff;
    uint32_t dd = (-d) & bit_mask_low_31;
    uint32_t bb = (b & 1);
    if(utility < 0.5)
        bb = 0;
    uint32_t gg = g & bit_mask_low_31;

    priority |= dd;
    priority <<= 1;
    priority |= bb;
    priority <<= 31;
    priority |= gg;
//    priority <<= 1;

    return priority;
}

/* obtain the current subtask of a task node */
Subtask_Node * _Scheduler_EDF_SMP_Subtask_Chain_current(
    Scheduler_EDF_SMP_Node * scheduler_edf_smp_node
)
{
    /* return the current subtask_node */
    Subtask_Node * current = scheduler_edf_smp_node->current;

    /* Current points to the next subtask */
    Chain_Node * next = _Chain_Next(&current->Chain);
    scheduler_edf_smp_node->current = RTEMS_CONTAINER_OF(next, Subtask_Node, Chain);

    _Assert(!rtems_chain_is_tail(scheduler_edf_smp_node->task_node.Subtask_Node_queue, scheduler_edf_smp_node->current));

    return scheduler_edf_smp_node->current;
}

/* reset the current subtask of a task node, and return the first subtask node */
Subtask_Node * _Scheduler_EDF_SMP_Subtask_Chain_reset(
    Scheduler_EDF_SMP_Node * scheduler_edf_smp_node
)
{
    Task_Node * task_node = scheduler_edf_smp_node->task_node;
    Chain_Node * first_node = _Chain_First(& task_node->Subtask_Node_queue );

    /* Set the current pointer to the first subtask node in the task node */
    scheduler_edf_smp_node->current = RTEMS_CONTAINER_OF(first_node, Subtask_Node, Chain);

    return scheduler_edf_smp_node->current;
}

Subtask_Node * _Scheduler_EDF_SMP_Subtask_Chain_get_first(Task_Node * task_node)
{
    Chain_Node * first_node = _Chain_First(& task_node->Subtask_Node_queue );

    /* Set the current pointer to the first subtask node in the task node */
    Subtask_Node * current = RTEMS_CONTAINER_OF(first_node, Subtask_Node, Chain);

    return current;
}

Scheduler_EDF_SMP_Node *_get_Scheduler_EDF_SMP_Node(Thread_Control * the_thread)
{
    Scheduler_Node *scheduler_node = _Thread_Scheduler_get_home_node( the_thread);
    return (Scheduler_EDF_SMP_Node *)(scheduler_node);
}

static void apply_priority(
  Thread_Control *thread,
  Priority_Control new_priority
)
{
  Scheduler_Node *scheduler_node = _Thread_Scheduler_get_home_node( thread );
  scheduler_node->Priority.value = new_priority << 1;
}

static void change_priority(
  Thread_Control *thread,
  Priority_Control new_priority
)
{
  Per_CPU_Control *cpu_self;

  cpu_self = _Thread_Dispatch_disable();

  Thread_queue_Context queue_context;

  apply_priority(thread, new_priority);
  _Scheduler_Update_priority( thread );

  _Thread_Dispatch_enable( cpu_self );
}

void _Scheduler_PSPM_Tick(
  const Scheduler_Control *scheduler,
  Thread_Control          *executing
)
{
  (void) scheduler;

  /*
   *  If the thread is not preemptible or is not ready, then
   *  just return.
   */

  if ( !executing->is_preemptible )
    return;

  if ( !_States_Is_ready( executing->current_state ) )
    return;

  /*
   *  The cpu budget algorithm determines what happens next.
   */

  switch ( executing->budget_algorithm ) {
    case THREAD_CPU_BUDGET_ALGORITHM_NONE:
      break;

    case THREAD_CPU_BUDGET_ALGORITHM_RESET_TIMESLICE:
    #if defined(RTEMS_SCORE_THREAD_ENABLE_EXHAUST_TIMESLICE)
      case THREAD_CPU_BUDGET_ALGORITHM_EXHAUST_TIMESLICE:
    #endif
      if ( (int)(--executing->cpu_time_budget) <= 0 ) {
          Scheduler_EDF_SMP_Node * edf_smp_node = _get_Scheduler_EDF_SMP_Node(executing);
          Subtask_Node * subtask_node = _Scheduler_EDF_SMP_Subtask_Chain_current(edf_smp_node);
          uint64_t pd2prio = _pspm_smp_create_priority(edf_smp_node->task_node->utility, edf_smp_node->release_time + subtask_node->d * pspm_smp_task_manager.quantum_length, subtask_node->b, subtask_node->g);
//          uint64_t pd2prio = subtask_node->d;
          printf("%d +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ %llu \n", edf_smp_node->task_node->id, pd2prio<<1);
          change_priority(executing, pd2prio);

        /*
         *  A yield performs the ready chain mechanics needed when
         *  resetting a timeslice.  If no other thread's are ready
         *  at the priority of the currently executing thread, then the
         *  executing thread's timeslice is reset.  Otherwise, the
         *  currently executing thread is placed at the rear of the
         *  FIFO for this priority and a new heir is selected.
         */
        _Thread_Yield( executing );
        executing->cpu_time_budget =
          rtems_configuration_get_ticks_per_timeslice();
      }
      else/* if(executing->cpu_time_budget == rtems_configuration_get_ticks_per_timeslice() - 1)*/
      {
          change_priority(executing, 20);
      }
      break;

    #if defined(RTEMS_SCORE_THREAD_ENABLE_SCHEDULER_CALLOUT)
      case THREAD_CPU_BUDGET_ALGORITHM_CALLOUT:
	if ( --executing->cpu_time_budget == 0 )
	  (*executing->budget_callout)( executing );
	break;
    #endif
  }
}

void _Scheduler_PSPM_EDF_Release_job(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Priority_Node           *priority_node,
  uint64_t                 deadline,
  Thread_queue_Context    *queue_context
)
{
  (void) scheduler;

  _Thread_Wait_acquire_critical( the_thread, queue_context );

  Scheduler_EDF_SMP_Node * edf_smp_node = _get_Scheduler_EDF_SMP_Node(the_thread);
  Subtask_Node * subtask_node = _Scheduler_EDF_SMP_Subtask_Chain_reset(edf_smp_node);

  edf_smp_node->release_time = deadline - edf_smp_node->task_node->period;

  printf("maximum_priority ========== %llx\n", scheduler->maximum_priority);
  /*
   * There is no integer overflow problem here due to the
   * SCHEDULER_PRIORITY_MAP().  The deadline is in clock ticks.  With the
   * minimum clock tick interval of 1us, the uptime is limited to about 146235
   * years.
   */
  _Priority_Node_set_priority(
    priority_node,
//    SCHEDULER_PRIORITY_MAP( deadline )
    SCHEDULER_PRIORITY_MAP(_pspm_smp_create_priority(edf_smp_node->task_node->utility, edf_smp_node->release_time + subtask_node->d * pspm_smp_task_manager.quantum_length, subtask_node->b, subtask_node->g))
//    SCHEDULER_PRIORITY_MAP(subtask_node->d)
    );

  if ( _Priority_Node_is_active( priority_node ) ) {
    _Thread_Priority_changed(
      the_thread,
      priority_node,
      false,
      queue_context
    );
  } else {
    _Thread_Priority_add( the_thread, priority_node, queue_context );
  }

  _Thread_Wait_release_critical( the_thread, queue_context );
}

static inline Scheduler_EDF_SMP_Context *
_Scheduler_EDF_SMP_Get_context( const Scheduler_Control *scheduler )
{
  return (Scheduler_EDF_SMP_Context *) _Scheduler_Get_context( scheduler );
}

static inline Scheduler_EDF_SMP_Context *
_Scheduler_EDF_SMP_Get_self( Scheduler_Context *context )
{
  return (Scheduler_EDF_SMP_Context *) context;
}

static inline Scheduler_EDF_SMP_Node *
_Scheduler_EDF_SMP_Node_downcast( Scheduler_Node *node )
{
  return (Scheduler_EDF_SMP_Node *) node;
}

static inline bool _Scheduler_EDF_SMP_Priority_less_equal(
  const void        *left,
  const RBTree_Node *right
)
{
  const Priority_Control   *the_left;
  const Scheduler_SMP_Node *the_right;
  Priority_Control          prio_left;
  Priority_Control          prio_right;

  the_left = left;
  the_right = RTEMS_CONTAINER_OF( right, Scheduler_SMP_Node, Base.Node.RBTree );

  prio_left = *the_left;
  prio_right = the_right->priority;

  return prio_left <= prio_right;
}

void _Scheduler_EDF_SMP_Initialize( const Scheduler_Control *scheduler )
{
  Scheduler_EDF_SMP_Context *self =
    _Scheduler_EDF_SMP_Get_context( scheduler );

  _Scheduler_SMP_Initialize( &self->Base );
  _Chain_Initialize_empty( &self->Affine_queues );
  /* The ready queues are zero initialized and thus empty */
}

void _Scheduler_EDF_SMP_Node_initialize(
  const Scheduler_Control *scheduler,
  Scheduler_Node          *node,
  Thread_Control          *the_thread,
  Priority_Control         priority
)
{
  Scheduler_SMP_Node *smp_node;

  smp_node = _Scheduler_SMP_Node_downcast( node );
  _Scheduler_SMP_Node_initialize( scheduler, smp_node, the_thread, priority );
}

static inline void _Scheduler_EDF_SMP_Do_update(
  Scheduler_Context *context,
  Scheduler_Node    *node,
  Priority_Control   new_priority
)
{
  Scheduler_SMP_Node *smp_node;

  (void) context;

  smp_node = _Scheduler_SMP_Node_downcast( node );
  _Scheduler_SMP_Node_update_priority( smp_node, new_priority );
}

static inline bool _Scheduler_EDF_SMP_Has_ready( Scheduler_Context *context )
{
  Scheduler_EDF_SMP_Context *self = _Scheduler_EDF_SMP_Get_self( context );

  return !_RBTree_Is_empty( &self->Ready[ 0 ].Queue );
}

static inline bool _Scheduler_EDF_SMP_Overall_less(
  const Scheduler_EDF_SMP_Node *left,
  const Scheduler_EDF_SMP_Node *right
)
{
  Priority_Control lp;
  Priority_Control rp;

  lp = left->Base.priority;
  rp = right->Base.priority;

  return lp < rp || (lp == rp && left->generation < right->generation );
}

static inline Scheduler_EDF_SMP_Node *
_Scheduler_EDF_SMP_Challenge_highest_ready(
  Scheduler_EDF_SMP_Context *self,
  Scheduler_EDF_SMP_Node    *highest_ready,
  RBTree_Control            *ready_queue
)
{
  Scheduler_EDF_SMP_Node *other;

  other = (Scheduler_EDF_SMP_Node *) _RBTree_Minimum( ready_queue );
  _Assert( other != NULL );

  if ( _Scheduler_EDF_SMP_Overall_less( other, highest_ready ) ) {
    return other;
  }

  return highest_ready;
}

static inline Scheduler_Node *_Scheduler_EDF_SMP_Get_highest_ready(
  Scheduler_Context *context,
  Scheduler_Node    *filter
)
{
  Scheduler_EDF_SMP_Context *self;
  Scheduler_EDF_SMP_Node    *highest_ready;
  Scheduler_EDF_SMP_Node    *node;
  uint32_t                   rqi;
  const Chain_Node          *tail;
  Chain_Node                *next;

  self = _Scheduler_EDF_SMP_Get_self( context );
  highest_ready = (Scheduler_EDF_SMP_Node *)
    _RBTree_Minimum( &self->Ready[ 0 ].Queue );
  _Assert( highest_ready != NULL );

  /*
   * The filter node is a scheduled node which is no longer on the scheduled
   * chain.  In case this is an affine thread, then we have to check the
   * corresponding affine ready queue.
   */

  node = (Scheduler_EDF_SMP_Node *) filter;
  rqi = node->ready_queue_index;

  if ( rqi != 0 && !_RBTree_Is_empty( &self->Ready[ rqi ].Queue ) ) {
    highest_ready = _Scheduler_EDF_SMP_Challenge_highest_ready(
      self,
      highest_ready,
      &self->Ready[ rqi ].Queue
    );
  }

  tail = _Chain_Immutable_tail( &self->Affine_queues );
  next = _Chain_First( &self->Affine_queues );

  while ( next != tail ) {
    Scheduler_EDF_SMP_Ready_queue *ready_queue;

    ready_queue = (Scheduler_EDF_SMP_Ready_queue *) next;
    highest_ready = _Scheduler_EDF_SMP_Challenge_highest_ready(
      self,
      highest_ready,
      &ready_queue->Queue
    );

    next = _Chain_Next( next );
  }

  return &highest_ready->Base.Base;
}

static inline void _Scheduler_EDF_SMP_Set_scheduled(
  Scheduler_EDF_SMP_Context *self,
  Scheduler_EDF_SMP_Node    *scheduled,
  const Per_CPU_Control     *cpu
)
{
  self->Ready[ _Per_CPU_Get_index( cpu ) + 1 ].scheduled = scheduled;
}

static inline Scheduler_EDF_SMP_Node *_Scheduler_EDF_SMP_Get_scheduled(
  const Scheduler_EDF_SMP_Context *self,
  uint32_t                         rqi
)
{
  return self->Ready[ rqi ].scheduled;
}

static inline Scheduler_Node *_Scheduler_EDF_SMP_Get_lowest_scheduled(
  Scheduler_Context *context,
  Scheduler_Node    *filter_base
)
{
  Scheduler_EDF_SMP_Node *filter;
  uint32_t                rqi;

  filter = _Scheduler_EDF_SMP_Node_downcast( filter_base );
  rqi = filter->ready_queue_index;

  if ( rqi != 0 ) {
    Scheduler_EDF_SMP_Context *self;
    Scheduler_EDF_SMP_Node    *node;

    self = _Scheduler_EDF_SMP_Get_self( context );
    node = _Scheduler_EDF_SMP_Get_scheduled( self, rqi );

    if ( node->ready_queue_index > 0 ) {
      _Assert( node->ready_queue_index == rqi );
      return &node->Base.Base;
    }
  }

  return _Scheduler_SMP_Get_lowest_scheduled( context, filter_base );
}

static inline void _Scheduler_EDF_SMP_Insert_ready(
  Scheduler_Context *context,
  Scheduler_Node    *node_base,
  Priority_Control   insert_priority
)
{
  Scheduler_EDF_SMP_Context     *self;
  Scheduler_EDF_SMP_Node        *node;
  uint32_t                       rqi;
  Scheduler_EDF_SMP_Ready_queue *ready_queue;
  int                            generation_index;
  int                            increment;
  int64_t                        generation;

  self = _Scheduler_EDF_SMP_Get_self( context );
  node = _Scheduler_EDF_SMP_Node_downcast( node_base );
  rqi = node->ready_queue_index;
  generation_index = SCHEDULER_PRIORITY_IS_APPEND( insert_priority );
  increment = ( generation_index << 1 ) - 1;
  ready_queue = &self->Ready[ rqi ];

  generation = self->generations[ generation_index ];
  node->generation = generation;
  self->generations[ generation_index ] = generation + increment;

  _RBTree_Initialize_node( &node->Base.Base.Node.RBTree );
  _RBTree_Insert_inline(
    &ready_queue->Queue,
    &node->Base.Base.Node.RBTree,
    &insert_priority,
    _Scheduler_EDF_SMP_Priority_less_equal
  );

  if ( rqi != 0 && _Chain_Is_node_off_chain( &ready_queue->Node ) ) {
    Scheduler_EDF_SMP_Node *scheduled;

    scheduled = _Scheduler_EDF_SMP_Get_scheduled( self, rqi );

    if ( scheduled->ready_queue_index == 0 ) {
      _Chain_Append_unprotected( &self->Affine_queues, &ready_queue->Node );
    }
  }
}

static inline void _Scheduler_EDF_SMP_Extract_from_ready(
  Scheduler_Context *context,
  Scheduler_Node    *node_to_extract
)
{
  Scheduler_EDF_SMP_Context     *self;
  Scheduler_EDF_SMP_Node        *node;
  uint32_t                       rqi;
  Scheduler_EDF_SMP_Ready_queue *ready_queue;

  self = _Scheduler_EDF_SMP_Get_self( context );
  node = _Scheduler_EDF_SMP_Node_downcast( node_to_extract );
  rqi = node->ready_queue_index;
  ready_queue = &self->Ready[ rqi ];

  _RBTree_Extract( &ready_queue->Queue, &node->Base.Base.Node.RBTree );
  _Chain_Initialize_node( &node->Base.Base.Node.Chain );

  if (
    rqi != 0
      && _RBTree_Is_empty( &ready_queue->Queue )
      && !_Chain_Is_node_off_chain( &ready_queue->Node )
  ) {
    _Chain_Extract_unprotected( &ready_queue->Node );
    _Chain_Set_off_chain( &ready_queue->Node );
  }
}

static inline void _Scheduler_EDF_SMP_Move_from_scheduled_to_ready(
  Scheduler_Context *context,
  Scheduler_Node    *scheduled_to_ready
)
{
  Priority_Control insert_priority;

  _Chain_Extract_unprotected( &scheduled_to_ready->Node.Chain );
  insert_priority = _Scheduler_SMP_Node_priority( scheduled_to_ready );
  _Scheduler_EDF_SMP_Insert_ready(
    context,
    scheduled_to_ready,
    insert_priority
  );
}

static inline void _Scheduler_EDF_SMP_Move_from_ready_to_scheduled(
  Scheduler_Context *context,
  Scheduler_Node    *ready_to_scheduled
)
{
  Priority_Control insert_priority;

  _Scheduler_EDF_SMP_Extract_from_ready( context, ready_to_scheduled );
  insert_priority = _Scheduler_SMP_Node_priority( ready_to_scheduled );
  insert_priority = SCHEDULER_PRIORITY_APPEND( insert_priority );
  _Scheduler_SMP_Insert_scheduled(
    context,
    ready_to_scheduled,
    insert_priority
  );
}

static inline void _Scheduler_EDF_SMP_Allocate_processor(
  Scheduler_Context *context,
  Scheduler_Node    *scheduled_base,
  Scheduler_Node    *victim_base,
  Per_CPU_Control   *victim_cpu
)
{
  Scheduler_EDF_SMP_Context     *self;
  Scheduler_EDF_SMP_Node        *scheduled;
  uint32_t                       rqi;

  (void) victim_base;
  self = _Scheduler_EDF_SMP_Get_self( context );
  scheduled = _Scheduler_EDF_SMP_Node_downcast( scheduled_base );
  rqi = scheduled->ready_queue_index;

  if ( rqi != 0 ) {
    Scheduler_EDF_SMP_Ready_queue *ready_queue;
    Per_CPU_Control               *desired_cpu;

    ready_queue = &self->Ready[ rqi ];

    if ( !_Chain_Is_node_off_chain( &ready_queue->Node ) ) {
      _Chain_Extract_unprotected( &ready_queue->Node );
      _Chain_Set_off_chain( &ready_queue->Node );
    }

    desired_cpu = _Per_CPU_Get_by_index( rqi - 1 );

    if ( victim_cpu != desired_cpu ) {
      Scheduler_EDF_SMP_Node *node;

      node = _Scheduler_EDF_SMP_Get_scheduled( self, rqi );
      _Assert( node->ready_queue_index == 0 );
      _Scheduler_EDF_SMP_Set_scheduled( self, node, victim_cpu );
      _Scheduler_SMP_Allocate_processor_exact(
        context,
        &node->Base.Base,
        NULL,
        victim_cpu
      );
      victim_cpu = desired_cpu;
    }
  }

  _Scheduler_EDF_SMP_Set_scheduled( self, scheduled, victim_cpu );
  _Scheduler_SMP_Allocate_processor_exact(
    context,
    &scheduled->Base.Base,
    NULL,
    victim_cpu
  );
}

void _Scheduler_EDF_SMP_Block(
  const Scheduler_Control *scheduler,
  Thread_Control          *thread,
  Scheduler_Node          *node
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  _Scheduler_SMP_Block(
    context,
    thread,
    node,
    _Scheduler_EDF_SMP_Extract_from_ready,
    _Scheduler_EDF_SMP_Get_highest_ready,
    _Scheduler_EDF_SMP_Move_from_ready_to_scheduled,
    _Scheduler_EDF_SMP_Allocate_processor
  );
}

static inline bool _Scheduler_EDF_SMP_Enqueue(
  Scheduler_Context *context,
  Scheduler_Node    *node,
  Priority_Control   insert_priority
)
{
  return _Scheduler_SMP_Enqueue(
    context,
    node,
    insert_priority,
    _Scheduler_SMP_Priority_less_equal,
    _Scheduler_EDF_SMP_Insert_ready,
    _Scheduler_SMP_Insert_scheduled,
    _Scheduler_EDF_SMP_Move_from_scheduled_to_ready,
    _Scheduler_EDF_SMP_Get_lowest_scheduled,
    _Scheduler_EDF_SMP_Allocate_processor
  );
}

static inline bool _Scheduler_EDF_SMP_Enqueue_scheduled(
  Scheduler_Context *context,
  Scheduler_Node    *node,
  Priority_Control   insert_priority
)
{
  return _Scheduler_SMP_Enqueue_scheduled(
    context,
    node,
    insert_priority,
    _Scheduler_SMP_Priority_less_equal,
    _Scheduler_EDF_SMP_Extract_from_ready,
    _Scheduler_EDF_SMP_Get_highest_ready,
    _Scheduler_EDF_SMP_Insert_ready,
    _Scheduler_SMP_Insert_scheduled,
    _Scheduler_EDF_SMP_Move_from_ready_to_scheduled,
    _Scheduler_EDF_SMP_Allocate_processor
  );
}

void _Scheduler_EDF_SMP_Unblock(
  const Scheduler_Control *scheduler,
  Thread_Control          *thread,
  Scheduler_Node          *node
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  _Scheduler_SMP_Unblock(
    context,
    thread,
    node,
    _Scheduler_EDF_SMP_Do_update,
    _Scheduler_EDF_SMP_Enqueue
  );
}

static inline bool _Scheduler_EDF_SMP_Do_ask_for_help(
  Scheduler_Context *context,
  Thread_Control    *the_thread,
  Scheduler_Node    *node
)
{
  return _Scheduler_SMP_Ask_for_help(
    context,
    the_thread,
    node,
    _Scheduler_SMP_Priority_less_equal,
    _Scheduler_EDF_SMP_Insert_ready,
    _Scheduler_SMP_Insert_scheduled,
    _Scheduler_EDF_SMP_Move_from_scheduled_to_ready,
    _Scheduler_EDF_SMP_Get_lowest_scheduled,
    _Scheduler_EDF_SMP_Allocate_processor
  );
}

void _Scheduler_EDF_SMP_Update_priority(
  const Scheduler_Control *scheduler,
  Thread_Control          *thread,
  Scheduler_Node          *node
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  _Scheduler_SMP_Update_priority(
    context,
    thread,
    node,
    _Scheduler_EDF_SMP_Extract_from_ready,
    _Scheduler_EDF_SMP_Do_update,
    _Scheduler_EDF_SMP_Enqueue,
    _Scheduler_EDF_SMP_Enqueue_scheduled,
    _Scheduler_EDF_SMP_Do_ask_for_help
  );
}

bool _Scheduler_EDF_SMP_Ask_for_help(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Scheduler_Node          *node
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  return _Scheduler_EDF_SMP_Do_ask_for_help( context, the_thread, node );
}

void _Scheduler_EDF_SMP_Reconsider_help_request(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Scheduler_Node          *node
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  _Scheduler_SMP_Reconsider_help_request(
    context,
    the_thread,
    node,
    _Scheduler_EDF_SMP_Extract_from_ready
  );
}

void _Scheduler_EDF_SMP_Withdraw_node(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  Scheduler_Node          *node,
  Thread_Scheduler_state   next_state
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  _Scheduler_SMP_Withdraw_node(
    context,
    the_thread,
    node,
    next_state,
    _Scheduler_EDF_SMP_Extract_from_ready,
    _Scheduler_EDF_SMP_Get_highest_ready,
    _Scheduler_EDF_SMP_Move_from_ready_to_scheduled,
    _Scheduler_EDF_SMP_Allocate_processor
  );
}

static inline void _Scheduler_EDF_SMP_Register_idle(
  Scheduler_Context *context,
  Scheduler_Node    *idle_base,
  Per_CPU_Control   *cpu
)
{
  Scheduler_EDF_SMP_Context *self;
  Scheduler_EDF_SMP_Node    *idle;

  self = _Scheduler_EDF_SMP_Get_self( context );
  idle = _Scheduler_EDF_SMP_Node_downcast( idle_base );
  _Scheduler_EDF_SMP_Set_scheduled( self, idle, cpu );
}

void _Scheduler_EDF_SMP_Add_processor(
  const Scheduler_Control *scheduler,
  Thread_Control          *idle
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  _Scheduler_SMP_Add_processor(
    context,
    idle,
    _Scheduler_EDF_SMP_Has_ready,
    _Scheduler_EDF_SMP_Enqueue_scheduled,
    _Scheduler_EDF_SMP_Register_idle
  );
}

Thread_Control *_Scheduler_EDF_SMP_Remove_processor(
  const Scheduler_Control *scheduler,
  Per_CPU_Control         *cpu
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  return _Scheduler_SMP_Remove_processor(
    context,
    cpu,
    _Scheduler_EDF_SMP_Extract_from_ready,
    _Scheduler_EDF_SMP_Enqueue
  );
}

void _Scheduler_EDF_SMP_Yield(
  const Scheduler_Control *scheduler,
  Thread_Control          *thread,
  Scheduler_Node          *node
)
{
  Scheduler_Context *context = _Scheduler_Get_context( scheduler );

  _Scheduler_SMP_Yield(
    context,
    thread,
    node,
    _Scheduler_EDF_SMP_Extract_from_ready,
    _Scheduler_EDF_SMP_Enqueue,
    _Scheduler_EDF_SMP_Enqueue_scheduled
  );
}

static inline void _Scheduler_EDF_SMP_Do_set_affinity(
  Scheduler_Context *context,
  Scheduler_Node    *node_base,
  void              *arg
)
{
  Scheduler_EDF_SMP_Node *node;
  const uint32_t         *rqi;

  node = _Scheduler_EDF_SMP_Node_downcast( node_base );
  rqi = arg;
  node->ready_queue_index = *rqi;
}

void _Scheduler_EDF_SMP_Start_idle(
  const Scheduler_Control *scheduler,
  Thread_Control          *idle,
  Per_CPU_Control         *cpu
)
{
  Scheduler_Context *context;

  context = _Scheduler_Get_context( scheduler );

  _Scheduler_SMP_Do_start_idle(
    context,
    idle,
    cpu,
    _Scheduler_EDF_SMP_Register_idle
  );
}

bool _Scheduler_EDF_SMP_Set_affinity(
  const Scheduler_Control *scheduler,
  Thread_Control          *thread,
  Scheduler_Node          *node,
  const Processor_mask    *affinity
)
{
  Scheduler_Context *context;
  Processor_mask     a;
  uint32_t           count;
  uint32_t           rqi;

  context = _Scheduler_Get_context( scheduler );
  _Processor_mask_And( &a, &context->Processors, affinity );
  count = _Processor_mask_Count( &a );

  if ( count == 0 ) {
    return false;
  }

  if ( count == _SMP_Processor_count ) {
    rqi = 0;
  } else {
    rqi = _Processor_mask_Find_last_set( &a );
  }

  _Scheduler_SMP_Set_affinity(
    context,
    thread,
    node,
    &rqi,
    _Scheduler_EDF_SMP_Do_set_affinity,
    _Scheduler_EDF_SMP_Extract_from_ready,
    _Scheduler_EDF_SMP_Get_highest_ready,
    _Scheduler_EDF_SMP_Move_from_ready_to_scheduled,
    _Scheduler_EDF_SMP_Enqueue,
    _Scheduler_EDF_SMP_Allocate_processor
  );

  return true;
}
