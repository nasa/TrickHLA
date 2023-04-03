/*!
@file TrickHLA/CTETimelineBase.cpp
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
synchronized using an external mechanism like NTP.

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
@trick_link_dependency{CTETimelineBase.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2016, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System includes

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/CTETimelineBase.hh"

using namespace Trick;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
CTETimelineBase::CTETimelineBase()
   : Clock( 1000000, "GetTimeOfDay - CLOCK_REALTIME" ),
     clk_id( CLOCK_REALTIME )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
CTETimelineBase::~CTETimelineBase()
{
   return;
}

/*!
 * @details Set the global "the_clock" pointer to this instance.
 */
int CTETimelineBase::clock_init()
{
   set_global_clock();
   return 0;
}

/*!
 * @details Get the global time base on the CTE.
 */
double CTETimelineBase::get_time()
{
   struct timespec ts;
   clock_gettime( CLOCK_REALTIME, &ts );
   return ( (double)ts.tv_sec + ( (double)ts.tv_nsec * 0.000000001 ) );
}

/*!
 * @details Call the system clock_gettime to get the current real time.
 */
int64_t CTETimelineBase::wall_clock_time()
{
   struct timespec tp;
   clock_gettime( clk_id, &tp );
   return (int64_t)tp.tv_sec * 1000000LL + (int64_t)( ( tp.tv_nsec ) / 1000 );
}

/*!
 * @details This function is empty
 */
int CTETimelineBase::clock_stop()
{
   return 0;
}

void CTETimelineBase::set_clock_ID( clockid_t id )
{
   switch ( id ) {
      case CLOCK_REALTIME:
         name = "GetTimeOfDay - CLOCK_REALTIME";
         break;
      case CLOCK_MONOTONIC:
         name = "GetTimeOfDay - CLOCK_MONOTONIC";
         break;
      case CLOCK_MONOTONIC_RAW:
         name = "GetTimeOfDay - CLOCK_MONOTONIC_RAW";
         break;
      default:
         name = "GetTimeOfDay - other";
         break;
   }
}

clockid_t CTETimelineBase::get_clock_ID()
{
   return clk_id;
}
