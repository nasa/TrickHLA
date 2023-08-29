/*!
@file TrickHLA/Timeline.cpp
@ingroup TrickHLA
@brief This class is the abstract base class for representing timelines.

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

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, April 2016, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end
*/

// TrickHLA include files.
#include "TrickHLA/Timeline.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Timeline::Timeline(
   double t0 )
   : epoch( t0 )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
Timeline::~Timeline()
{
   return;
}
