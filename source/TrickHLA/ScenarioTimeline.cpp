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
@trick_link_dependency{Timeline.cpp}
@trick_link_dependency{ScenarioTimeline.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, January 2019, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.

// Trick include files.

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/ScenarioTimeline.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ScenarioTimeline::ScenarioTimeline(
   SimTimeline &sim_timeline,
   double       tt0,
   double       st0 )
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

double ScenarioTimeline::compute_simulation_time(
   double scenario_time )
{
   return ( scenario_time - ( epoch + sim_offset ) );
}

double ScenarioTimeline::time_from_simulation_time(
   double sim_time )
{
   return ( sim_time + ( epoch + sim_offset ) );
}

Int64Time ScenarioTimeline::compute_HLT(
   double scenario_time )
{
   Int64Time elapsed_time( scenario_time - epoch );
   return ( elapsed_time - hlt_offset );
}

double ScenarioTimeline::time_from_HLT(
   Int64Time hlt )
{
   return ( hlt.get_double_time() + hlt_offset.get_double_time() + epoch );
}

double ScenarioTimeline::get_time()
{
   return ( epoch + sim_offset + sim_timeline.get_time() );
}
