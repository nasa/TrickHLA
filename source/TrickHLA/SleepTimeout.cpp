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
#include <math.h>
#include <time.h>

// TrickHLA include files.
#include "TrickHLA/SleepTimeout.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SleepTimeout::SleepTimeout()
{
   set( THLA_DEFAULT_SLEEP_TIMEOUT_IN_SECS, THLA_DEFAULT_SLEEP_WAIT_IN_NANOSEC );
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
   // Do a bounds check on the timeout time and calculate the timeout-count.
   if ( timeout_seconds < ( sleep_micros * ( (double)std::numeric_limits< unsigned long >::max() / 1000000.0 ) ) ) {
      this->timeout_count = (unsigned long)round( timeout_seconds * 1000000.0 / (double)sleep_micros );
   } else {
      this->timeout_count = std::numeric_limits< unsigned long >::max();
   }

   // Calculate the requested sleep-time.
   if ( sleep_micros >= 1000000 ) {
      req.tv_sec  = ( sleep_micros / 1000000 );
      req.tv_nsec = ( sleep_micros - ( req.tv_sec * 1000000 ) ) * 1000;
   } else {
      req.tv_sec  = 0;
      req.tv_nsec = sleep_micros * 1000;
   }

   // Make sure we do a reset now that the timeout and sleep values have changed.
   reset();
}

int SleepTimeout::sleep()
{
   if ( count < timeout_count ) {
      ++count;
   }
   return nanosleep( &req, NULL );
}
