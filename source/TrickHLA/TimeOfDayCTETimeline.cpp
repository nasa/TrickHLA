/*!
@file TrickHLA/TimeOfDayCTETimeline.cpp
@ingroup TrickHLA
@brief A base implementation of a SRFOM CTE timeline.

This is a baseline implementation based off of the system clock. It is
intended to provide the definition of the CTE Timeline interface in the
form of a base class. It also provides a working implementation based on the
system clock (see <b>Assumptions and Limitations below</b>).

The expectation is that a hardware specific implementation will be developed
and provided for the CTE hardware used in a specific federation execution.

\par<b>Assumptions and Limitations:</b>
- This implementation uses the system clock which is assumed to be
synchronized using an external mechanism like PTP or NTP.

@copyright Copyright 2025 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{CTETimelineBase.cpp}
@trick_link_dependency{Timeline.cpp}
@trick_link_dependency{TimeOfDayCTETimeline.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2016, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2025, --, Rewrite of CTE Timeline into this class.}
@revs_end

*/

// System include files.
#include <sstream>
#include <string>
#include <time.h>

// Trick include files.
#include "trick/Clock.hh"
#include "trick/Executive.hh"
#include "trick/RealtimeSync.hh"
#include "trick/exec_proto.h"
#include "trick/realtimesync_proto.h"

// TrickHLA include files.
#include "TrickHLA/CTETimelineBase.hh"
#include "TrickHLA/TimeOfDayCTETimeline.hh"
#include "TrickHLA/Timeline.hh"

using namespace std;
using namespace Trick;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
TimeOfDayCTETimeline::TimeOfDayCTETimeline()
   : CTETimelineBase( exec_get_time_tic_value(),
                      "TrickHLA::TimeOfDayCTETimeline - CLOCK_MONOTONIC" ),
     clk_id( CLOCK_MONOTONIC )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
TimeOfDayCTETimeline::~TimeOfDayCTETimeline()
{
   return;
}

/*!
 * @details Get the minimum time resolution in seconds, which is the smallest
 * time representation for this timeline.
 */
double const TimeOfDayCTETimeline::get_min_resolution()
{
   struct timespec ts;
   clock_getres( clk_id, &ts );
   return (double)ts.tv_sec + ( (double)ts.tv_nsec * 0.000000001 );
}

/*!
 * @brief Update the clock tics per second resolution of this clock
 *  to match the Trick executive resolution.
 */
void TimeOfDayCTETimeline::set_clock_tics_per_sec(
   int const tics_per_sec )
{
   // Make sure resolution of this clock can meet the time resolution
   // needed by the Trick executive. Limit the resolution to 1-nanosecond,
   // which this clock supports.
   this->clock_tics_per_sec = ( tics_per_sec > 0 ) ? tics_per_sec : 1;

   // Update the sim time ratio given the Trick executive time resolution.
   calc_sim_time_ratio( tics_per_sec );
}

/*!
 * @details Get the global time in seconds based on the CTE.
 */
double const TimeOfDayCTETimeline::get_time()
{
   struct timespec ts;
   clock_gettime( clk_id, &ts );
   return (double)ts.tv_sec + ( (double)ts.tv_nsec * 0.000000001 );
}

/*!
 * @details Set the global "the_clock" pointer to this instance.
 */
int TimeOfDayCTETimeline::clock_init()
{
   // Make sure the CTE clock resolution is updated to meet
   // the time resolution needed by the Trick executive.
   set_clock_tics_per_sec( exec_get_time_tic_value() );

   // Use this CTE timeline as the global Trick clock.
   set_global_clock();

   return 0;
}

/*!
 * @details Call the system clock_gettime to get the current real time with
 * a resolution set by Clock::clock_tics_per_sec.
 */
long long TimeOfDayCTETimeline::wall_clock_time()
{
   struct timespec ts;
   clock_gettime( clk_id, &ts );
   return ( ts.tv_sec * clock_tics_per_sec )
          + ( ( ts.tv_nsec * clock_tics_per_sec ) / 1000000000LL );
}

/*!
 * @details This function is empty
 */
int TimeOfDayCTETimeline::clock_stop()
{
   return 0;
}

void TimeOfDayCTETimeline::set_clock_ID( clockid_t const id )
{
   this->clk_id = id;

   switch ( id ) {
      case CLOCK_REALTIME: {
         this->name = "TrickHLA::TimeOfDayCTETimeline - CLOCK_REALTIME";
         break;
      }
      case CLOCK_MONOTONIC: {
         this->name = "TrickHLA::TimeOfDayCTETimeline - CLOCK_MONOTONIC";
         break;
      }
      case CLOCK_MONOTONIC_RAW: {
         this->name = "TrickHLA::TimeOfDayCTETimeline - CLOCK_MONOTONIC_RAW";
         break;
      }
      default: {
         this->name = "TrickHLA::TimeOfDayCTETimeline - clock_id("
                      + std::to_string( id ) + ")";
         break;
      }
   }
}

clockid_t const TimeOfDayCTETimeline::get_clock_ID()
{
   return this->clk_id;
}
