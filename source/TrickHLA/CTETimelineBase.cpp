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
@trick_link_dependency{CTETimelineBase.cpp}
@trick_link_dependency{Timeline.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2016, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2025, --, Made into base class.}
@revs_end

*/

// System include files.
#include <sstream>
#include <string>
#include <time.h>

// Trick include files.
#include "trick/Clock.hh"
#include "trick/RealtimeSync.hh"
#include "trick/realtimesync_proto.h"

// TrickHLA include files.
#include "TrickHLA/CTETimelineBase.hh"

using namespace std;
using namespace Trick;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
CTETimelineBase::CTETimelineBase(
   unsigned long long const clock_tics_per_sec,
   string const            &clock_name )
   : Clock( clock_tics_per_sec, clock_name )
{
   // Change the Trick real time clock to this clock.
   real_time_change_clock( this );
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
   // Use this CTE timeline as the global Trick clock.
   set_global_clock();

   return 0;
}
