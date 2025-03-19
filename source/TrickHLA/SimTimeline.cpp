/*!
@file TrickHLA/SimTimeline.cpp
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
@trick_link_dependency{SimTimeline.cpp}
@trick_link_dependency{Timeline.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, NExSyS, April 2016, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// Trick include files.
#include "trick/exec_proto.h"

// TrickHLA include files.
#include "TrickHLA/SimTimeline.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SimTimeline::SimTimeline()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SimTimeline::~SimTimeline()
{
   return;
}

/*!
 * @details Get the current simulation time.
 */
double const SimTimeline::get_time()
{
   return exec_get_sim_time();
}

/*!
 * @details Get the minimum time resolution, which is the smallest time
 * representation for this timeline.
 */
double const SimTimeline::get_min_resolution()
{
   // Time resolution for the Trick Simulation Environment.
   return ( 1.0 / (double)exec_get_time_tic_value() );
}
