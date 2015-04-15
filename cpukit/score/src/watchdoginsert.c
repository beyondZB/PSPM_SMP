/**
 * @file 
 *
 * @brief Watchdog Insert
 * @ingroup ScoreWatchdog
 */
 
/*
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/score/watchdogimpl.h>
#include <rtems/score/isrlevel.h>
#include <rtems/score/percpu.h>

void _Watchdog_Insert(
  Watchdog_Header       *header,
  Watchdog_Control      *the_watchdog
)
{
  ISR_lock_Context   lock_context;
  Watchdog_Control  *after;
  uint32_t           insert_isr_nest_level;
  Watchdog_Interval  delta_interval;


  insert_isr_nest_level   = _ISR_Nest_level;

  _Watchdog_Acquire( header, &lock_context );

  /*
   *  Check to see if the watchdog has just been inserted by a
   *  higher priority interrupt.  If so, abandon this insert.
   */

  if ( the_watchdog->state != WATCHDOG_INACTIVE ) {
    _Watchdog_Release( header, &lock_context );
    return;
  }

  the_watchdog->state = WATCHDOG_BEING_INSERTED;
  _Watchdog_Sync_count++;

restart:
  delta_interval = the_watchdog->initial;

  for ( after = _Watchdog_First( header ) ;
        ;
        after = _Watchdog_Next( after ) ) {

     if ( delta_interval == 0 || !_Watchdog_Next( after ) )
       break;

     if ( delta_interval < after->delta_interval ) {
       after->delta_interval -= delta_interval;
       break;
     }

     delta_interval -= after->delta_interval;

     _Watchdog_Flash( header, &lock_context );

     if ( the_watchdog->state != WATCHDOG_BEING_INSERTED ) {
       goto exit_insert;
     }

     if ( _Watchdog_Sync_level > insert_isr_nest_level ) {
       _Watchdog_Sync_level = insert_isr_nest_level;
       goto restart;
     }
  }

  _Watchdog_Activate( the_watchdog );

  the_watchdog->delta_interval = delta_interval;

  _Chain_Insert_unprotected( after->Node.previous, &the_watchdog->Node );

  the_watchdog->start_time = _Watchdog_Ticks_since_boot;

exit_insert:
  _Watchdog_Sync_level = insert_isr_nest_level;
  _Watchdog_Sync_count--;
  _Watchdog_Release( header, &lock_context );
}
