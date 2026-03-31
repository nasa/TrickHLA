/*!
@file TrickHLA/time/Timeline.cpp
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

// System includes.
#include <cstdint>
#include <limits>

// TrickHLA includes.
#include "TrickHLA/time/Timeline.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Timeline::Timeline(
   double const t0 )
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

/*!
 * @brief Convert value to a time on the timeline with the minimum time resolution.
 * @return Returns the time in seconds on the timeline with the minimum resolution.
 * @param value The time value to convert.
 */
double Timeline::convert(
   double const value )
{
   double const min_res = get_min_resolution();
   if ( value <= ( (double)std::numeric_limits< long long >::min() * min_res ) ) {
      return (double)std::numeric_limits< long long >::min();
   } else if ( value >= ( (double)std::numeric_limits< long long >::max() * min_res ) ) {
      return (double)std::numeric_limits< long long >::max();
   }
   // Compute the time in tics, which truncates to a fixed-point number.
   int64_t const time_tics = (int64_t)( value / min_res );

   // Convert to a time in seconds with the minimum time resolution.
   return (double)( time_tics * min_res );
}
