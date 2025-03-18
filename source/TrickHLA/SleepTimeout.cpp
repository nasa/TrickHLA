/*!
@file TrickHLA/SleepTimeout.cpp
@ingroup TrickHLA
@brief TrickHLA sleep timer for use in spin locks to detect a timeout.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{SleepTimeout.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, July 2020, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstdint>
#include <limits>
#include <time.h>

// Trick include files.
#include "trick/Executive.hh"
#include "trick/clock_proto.h"
#include "trick/exec_proto.h"

// TrickHLA include files.
#include "TrickHLA/SleepTimeout.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SleepTimeout::SleepTimeout()
{
   set( THLA_DEFAULT_SLEEP_TIMEOUT_IN_SEC, THLA_DEFAULT_SLEEP_WAIT_IN_MICROS );
}

SleepTimeout::SleepTimeout(
   double const timeout_seconds )
{
   set( timeout_seconds, THLA_DEFAULT_SLEEP_WAIT_IN_MICROS );
}

SleepTimeout::SleepTimeout(
   long const sleep_micros )
{
   set( THLA_DEFAULT_SLEEP_TIMEOUT_IN_SEC, sleep_micros );
}

SleepTimeout::SleepTimeout(
   double const timeout_seconds,
   long const   sleep_micros )
{
   set( timeout_seconds, sleep_micros );
}

SleepTimeout::~SleepTimeout()
{
   return;
}

void SleepTimeout::set(
   double const timeout_seconds,
   long const   sleep_micros )
{
   // Do a bounds check on the timeout in seconds and convert it to microseconds.
   if ( timeout_seconds <= 0.0 ) {
      this->timeout_time = 0;
   } else if ( timeout_seconds >= ( INT64_MAX / 1000000 ) ) {
      this->timeout_time = INT64_MAX;
   } else {
      this->timeout_time = timeout_seconds * 1000000; // in microseconds
   }

   // Calculate the requested sleep-time.
   if ( sleep_micros >= 1000000 ) {
      sleep_time.tv_sec  = ( sleep_micros / 1000000 );
      sleep_time.tv_nsec = ( sleep_micros - ( sleep_time.tv_sec * 1000000 ) ) * 1000;
   } else if ( sleep_micros > 0 ) {
      sleep_time.tv_sec  = 0;
      sleep_time.tv_nsec = sleep_micros * 1000;
   } else {
      sleep_time.tv_sec  = 0;
      sleep_time.tv_nsec = 0;
   }

   // Make sure we do a reset now that the timeout and sleep values are set.
   reset();
}

int const SleepTimeout::sleep() const
{
   return nanosleep( &sleep_time, NULL );
}

// Current time as an integer in microseconds.
int64_t const SleepTimeout::time() const
{
   // Make sure the time is always an integer in microseconds.
   int const time_tic_value = exec_get_time_tic_value();
   return ( time_tic_value == 1000000 )
             ? clock_wall_time()
             : ( ( clock_wall_time() * 1000000 ) / time_tic_value );
}

bool const SleepTimeout::timeout() const
{
   return timeout( time() );
}

bool const SleepTimeout::timeout(
   int64_t const time_in_micros ) const
{
   return ( time_in_micros >= this->timeout_clock_time );
}

/*! @brief Reset the internal timeout time. */
void SleepTimeout::reset()
{
   int64_t const t = time();
   if ( t < ( INT64_MAX - this->timeout_time ) ) {
      this->timeout_clock_time = t + this->timeout_time;
   } else {
      this->timeout_clock_time = INT64_MAX;
   }
}
