/**
 *  @file
 *
 *  @brief Announce a Clock Tick
 *  @ingroup ClassicClock
 */

/*
 *  COPYRIGHT (c) 1989-2009.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/rtems/clock.h>
#include <rtems/score/timecounter.h>
#include <rtems/score/threadimpl.h>

rtems_status_code rtems_clock_tick( void )
{
  ISR_lock_Context lock_context;

  _Timecounter_Acquire( &lock_context );

  /* calculate the clock tick overheads */
  static uint64_t start, end, total;
  static uint32_t count;
  start = rtems_clock_get_uptime_nanoseconds();
  _Timecounter_Tick_simple(
    rtems_configuration_get_microseconds_per_tick(),
    0,
    &lock_context
  );
  end = rtems_clock_get_uptime_nanoseconds();
  total += end - start;
  count ++;
  printf("The clock tick overheads is %llf, the total overheads is %llu, the count is %u\n", (double)total/count, total, count);

  if ( _Thread_Dispatch_is_enabled() ) {
    _Thread_Dispatch();
  }

  return RTEMS_SUCCESSFUL;
}
