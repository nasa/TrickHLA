/*!
@file TrickHLA/TSyncCTETimeline.cpp
@ingroup TrickHLA
@brief The implementation of a TSync CTE timeline.

This is a baseline implementation based off of the TSync hardware clock.
It is intended to provide the definition of the CTE Timeline interface.

TSync Driver:
https://safran-navigation-timing.com/portal/public-downloads/latest-tsyncpcie-update-files/

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
@trick_link_dependency{TSyncCTETimeline.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2025, --, Initial implementation.}
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
#include "trick/message_proto.h"
#include "trick/realtimesync_proto.h"

// TrickHLA include files.
#include "TrickHLA/CTETimelineBase.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/TSyncCTETimeline.hh"
#include "TrickHLA/Timeline.hh"

#if !defined( SWIG )
#   if !defined( __linux__ )
#      error "The TSync Central Timing Equipment (CTE) card is only supported on Linux."
#   endif

extern "C" {
#   include "tsync.h" // cppcheck-suppress [missingInclude]
}
#endif

using namespace std;
using namespace Trick;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
TSyncCTETimeline::TSyncCTETimeline()
   : CTETimelineBase( exec_get_time_tic_value(),
                      "TrickHLA::TSyncCTETimeline - TSYNC" ),
     full_device_name(),
     board_handle()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
TSyncCTETimeline::~TSyncCTETimeline()
{
   return;
}

/*!
 * @details Get the minimum time resolution in seconds, which is the smallest
 * time representation for this timeline.
 */
double const TSyncCTETimeline::get_min_resolution()
{
   return 0.000000001;
}

/*!
 * @brief Update the clock tics per second resolution of this clock
 *  to match the Trick executive resolution.
 */
void TSyncCTETimeline::set_clock_tics_per_sec(
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
double const TSyncCTETimeline::get_time()
{
   TSYNC_HWTimeSecondsObj hw_time;

   // Send Get Seconds Time message
   TSYNC_ERROR err = TSYNC_HW_getTimeSec( board_handle, &hw_time );

   if ( err != TSYNC_SUCCESS ) {
      ostringstream errmsg;
      errmsg << "TSyncCTETimeline::get_time():" << __LINE__
             << " ERROR: '" << tsync_strerror( err ) << "'\n";
      message_publish( MSG_ERROR, errmsg.str().c_str() );
      return 0;
   }

   // Convert the TSync time based on clock tics per second resolution.
   return (double)hw_time.time.seconds + ( (double)hw_time.time.ns * 0.000000001 );
}

/*!
 * @details Set the global "the_clock" pointer to this instance.
 */
int TSyncCTETimeline::clock_init()
{
   // Make sure the CTE clock resolution is updated to meet
   // the time resolution needed by the Trick executive.
   set_clock_tics_per_sec( exec_get_time_tic_value() );

   TSYNC_ERROR err = TSYNC_open( &board_handle, full_device_name.c_str() );
   if ( err != TSYNC_SUCCESS ) {
      ostringstream errmsg;
      errmsg << "TSyncCTETimeline::clock_init():" << __LINE__
             << " ERROR: Could not open TSync CTE card '"
             << full_device_name << "' [" << tsync_strerror( err ) << "]\n";
      message_publish( MSG_ERROR, errmsg.str().c_str() );
      return 1;
   }

   set_global_clock();
   return 0;
}

/*!
 * @details Call the system clock_gettime to get the current real time with
 * a resolution set by Clock::clock_tics_per_sec.
 */
long long TSyncCTETimeline::wall_clock_time()
{
   TSYNC_HWTimeSecondsObj hw_time;

   // Get the time in seconds.
   TSYNC_ERROR err = TSYNC_HW_getTimeSec( board_handle, &hw_time );

   if ( err != TSYNC_SUCCESS ) {
      ostringstream errmsg;
      errmsg << "TSyncCTETimeline::wall_clock_time():" << __LINE__
             << " ERROR: '" << tsync_strerror( err ) << "'\n";
      message_publish( MSG_ERROR, errmsg.str().c_str() );
      return 0;
   }

   // Convert the TSync board time using the clock tics per second resolution.
   return ( hw_time.time.seconds * clock_tics_per_sec )
          + ( ( hw_time.time.ns * clock_tics_per_sec ) / 1000000000LL );
}

/*!
 * @details This function is empty
 */
int TSyncCTETimeline::clock_stop()
{
   int rc = TSYNC_close( board_handle );
   if ( rc != TSYNC_SUCCESS ) {
      ostringstream errmsg;
      errmsg << "TSyncCTETimeline::clock_stop():" << __LINE__
             << " ERROR: Could not close TSync CTE card '"
             << full_device_name << "' [" << rc << "]\n";
      message_publish( MSG_ERROR, errmsg.str().c_str() );
   }
   return 0;
}
