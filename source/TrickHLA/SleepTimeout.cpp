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
#include <limits>
#include <time.h>

// Trick include files.
#include "trick/clock_proto.h"

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
   double timeout_seconds )
{
   set( timeout_seconds, THLA_DEFAULT_SLEEP_WAIT_IN_MICROS );
}

SleepTimeout::SleepTimeout(
   long sleep_micros )
{
   set( THLA_DEFAULT_SLEEP_TIMEOUT_IN_SEC, sleep_micros );
}

SleepTimeout::SleepTimeout(
   double timeout_seconds,
   long   sleep_micros )
{
   set( timeout_seconds, sleep_micros );
}

SleepTimeout::~SleepTimeout()
{
   return;
}

void SleepTimeout::set(
   double timeout_seconds,
   long   sleep_micros )
{
   // Do a bounds check on the timeout in seconds and convert it to microseconds.
   if ( timeout_seconds <= 0.0 ) {
      this->timeout_time = 0;
   } else if ( ( timeout_seconds * 1000000 ) >= std::numeric_limits< long long >::max() ) {
      this->timeout_time = std::numeric_limits< long long >::max();
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

int SleepTimeout::sleep()
{
   return nanosleep( &sleep_time, NULL );
}

const bool SleepTimeout::timeout() const
{
   return ( clock_wall_time() >= this->timeout_clock_time );
}

/*! @brief Reset the internal timeout time. */
void SleepTimeout::reset()
{
   this->timeout_clock_time = clock_wall_time() + this->timeout_time;
}
