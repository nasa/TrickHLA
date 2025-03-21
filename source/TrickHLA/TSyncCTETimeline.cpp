/*!
@file TrickHLA/TSyncCTETimeline.cpp
@ingroup TrickHLA
@brief The implementation of a TSync CTE timeline.

This is a baseline implementation based off of the TSync hardware clock.
It is intended to provide the definition of the CTE Timeline interface.

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
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CTETimelineBase.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/TSyncCTETimeline.hh"
#include "TrickHLA/Timeline.hh"

#if defined( TSYNC_CTE )
extern "C" {
#   include "tsync.h"
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
                      "TrickHLA::TSyncCTETimeline - TSYNC" )
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
void TSyncCTETimeline::update_clock_resolution()
{
   // Make sure resolution of this clock can meet the time resolution
   // needed by the Trick executive. Limit the resolution to 1-nanosecond,
   // which this clock supports.
   this->clock_tics_per_sec = ( exec_get_time_tic_value() <= 1000000000LL )
                                 ? exec_get_time_tic_value()
                                 : 1000000000ULL;

   // Update the sim time ratio given the Trick executive time resolution
   // and the updated clock_tics_per_sec may have changed.
   calc_sim_time_ratio( exec_get_time_tic_value() );
}

/*!
 * @details Get the global time in seconds based on the CTE.
 */
double const TSyncCTETimeline::get_time()
{
#if defined( TSYNC_CTE )

   TSYNC_HWTimeSecondsObj hwTime;

   // Send Get Seconds Time message
   TSYNC_ERROR err = TSYNC_HW_getTimeSec( pBoard, &hwTime );

   if ( err != TSYNC_SUCCESS ) {
      ostringstream errmsg;
      errmsg << "TSyncCTETimeline::get_time():" << __LINE__
             << " ERROR: '" << tsync_strerror( err ) << "'\n";
      send_hs( stdout, errmsg.str().c_str() );
      return 0;
   }

   // Convert the TSync time based on clock tics per second resolution.
   return (double)hwTime.time.seconds + ( (double)hwTime.time.ns * 0.000000001 );
#else
   ostringstream errmsg;
   errmsg << "TSyncCTETimeline::get_time():" << __LINE__
          << " ERROR: Not configured for TSync CTE card.\n";
   send_hs( stdout, errmsg.str().c_str() );
   return 0;
#endif
}

/*!
 * @details Set the global "the_clock" pointer to this instance.
 */
int TSyncCTETimeline::clock_init()
{
#if defined( TSYNC_CTE )

   int rc = TSYNC_open( &pBoard, (char *)dev_name.c_str() );

   if ( rc != TSYNC_SUCCESS ) {
      ostringstream errmsg;
      errmsg << "TSyncCTETimeline::clock_init():" << __LINE__
             << " ERROR: Could Not Open TSync Board '"
             << dev_name.c_str() << "' [" << rc << "]\n";
      send_hs( stdout, errmsg.str().c_str() );
      return ( 1 );
   }

   set_global_clock();
   return 0;
#else
   ostringstream errmsg;
   errmsg << "TSyncCTETimeline::clock_init():" << __LINE__
          << " ERROR: Not configured for TSync CTE card.\n";
   send_hs( stdout, errmsg.str().c_str() );
   return -1;
#endif
}

/*!
 * @details Call the system clock_gettime to get the current real time with
 * a resolution set by Clock::clock_tics_per_sec.
 */
long long TSyncCTETimeline::wall_clock_time()
{
#if defined( TSYNC_CTE )

   TSYNC_HWTimeSecondsObj hwTime;

   // Send Get Seconds Time message
   TSYNC_ERROR err = TSYNC_HW_getTimeSec( pBoard, &hwTime );

   if ( err != TSYNC_SUCCESS ) {
      ostringstream errmsg;
      errmsg << "TSyncCTETimeline::wall_clock_time():" << __LINE__
             << " ERROR: '" << tsync_strerror( err ) << "'\n";
      send_hs( stdout, errmsg.str().c_str() );
      return 0;
   }

   // Convert the TSync board time based on clock tics per second resolution.
   return ( hwTime.time.seconds * clock_tics_per_sec )
          + ( ( hwTime.time.ns * clock_tics_per_sec ) / 1000000000LL );
#else
   ostringstream errmsg;
   errmsg << "TSyncCTETimeline::wall_clock_time():" << __LINE__
          << " ERROR: Not configured for TSync CTE card.\n";
   send_hs( stdout, errmsg.str().c_str() );
   return 0;
#endif
}

/*!
 * @details This function is empty
 */
int TSyncCTETimeline::clock_stop()
{
#if defined( TSYNC_CTE )
   int rc = TSYNC_close( pBoard );
   if ( rc != TSYNC_SUCCESS ) {
      ostringstream errmsg;
      errmsg << "TSyncCTETimeline::clock_stop():" << __LINE__
             << " ERROR: Could close TSync Board '"
             << dev_name.c_str() << "' [" << rc << "]\n";
      send_hs( stdout, errmsg.str().c_str() );
   }
#endif
   return 0;
}
