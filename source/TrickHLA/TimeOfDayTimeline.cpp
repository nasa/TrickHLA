/*!
@file TrickHLA/TimeOfDayTimeline.cpp
@ingroup TrickHLA
@brief This class represents the simulation timeline.

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
@trick_link_dependency{Timeline.cpp}
@trick_link_dependency{TimeOfDayTimeline.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, SpaceFOM, June 2016, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System includes
#include <time.h>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/TimeOfDayTimeline.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
TimeOfDayTimeline::TimeOfDayTimeline()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
TimeOfDayTimeline::~TimeOfDayTimeline()
{
   return;
}

/*!
 * @details Get the current time of day for this timeline.
 */
double const TimeOfDayTimeline::get_time()
{
   struct timespec ts;
   clock_gettime( CLOCK_REALTIME, &ts );
   return ( (double)ts.tv_sec + ( (double)ts.tv_nsec * 0.000000001 ) );
}

/*!
 * @details Get the minimum time resolution, which is the smallest time
 * representation for this timeline.
 */
double const TimeOfDayTimeline::get_min_resolution()
{
   struct timespec ts;
   clock_getres( CLOCK_REALTIME, &ts );
   return ( (double)ts.tv_sec + ( (double)ts.tv_nsec * 0.000000001 ) );
}
