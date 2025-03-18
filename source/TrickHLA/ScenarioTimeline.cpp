/*!
@file TrickHLA/ScenarioTimeline.cpp
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
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{ScenarioTimeline.cpp}
@trick_link_dependency{SimTimeline.cpp}
@trick_link_dependency{Timeline.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, January 2019, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cfloat>
#include <cstdint>

// Trick include files.
#include "trick/Executive.hh"
#include "trick/exec_proto.h"

// TrickHLA include files.
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/ScenarioTimeline.hh"
#include "TrickHLA/SimTimeline.hh"
#include "TrickHLA/Timeline.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ScenarioTimeline::ScenarioTimeline(
   SimTimeline &sim_timeline,
   double const tt0,
   double const st0 )
   : Timeline( tt0 ),
     sim_timeline( sim_timeline ),
     sim_offset( st0 ),
     hlt_offset( (int64_t)0 )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ScenarioTimeline::~ScenarioTimeline()
{
   return;
}

/*! @brief Compute a simulation time from a given scenario time.
 *  @return Simulation time in seconds.
 *  @param scenario_time Desired scenario time in seconds. */
double const ScenarioTimeline::compute_simulation_time(
   double const scenario_time )
{
   // Make sure to convert to a time on the sim-timeline for the minimum resolution.
   return ( sim_timeline.convert( scenario_time - ( epoch + sim_offset ) ) );
}

double const ScenarioTimeline::time_from_simulation_time(
   double const sim_time )
{
   return ( sim_time + ( epoch + sim_offset ) );
}

Int64Time const ScenarioTimeline::compute_HLT(
   double const scenario_time )
{
   Int64Time elapsed_time( scenario_time - epoch );
   return ( elapsed_time - hlt_offset );
}

double const ScenarioTimeline::time_from_HLT(
   Int64Time const &hlt )
{
   return ( hlt.get_time_in_seconds() + hlt_offset.get_time_in_seconds() + epoch );
}

/*!
 * @details Get the current scenario time.
 */
double const ScenarioTimeline::get_time()
{
   return ( epoch + sim_offset + sim_timeline.get_time() );
}

/*!
 * @details Get the minimum time resolution, which is the smallest time
 * representation for this timeline.
 */
double const ScenarioTimeline::get_min_resolution()
{
   return ( 1.0 / (double)Int64BaseTime::get_base_time_multiplier() );
}
