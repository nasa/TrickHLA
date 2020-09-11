/*!
@file TrickHLA/ElapsedTimeStats.cpp
@ingroup TrickHLA
@brief This class gathers statistics on the elapsed time between calls to the
measure function.

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
@trick_link_dependency{ElapsedTimeStats.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, Sept 2020, --, Initial version.}
@revs_end

*/

// System include files.
#include <cmath>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/clock_proto.h"

// TrickHLA include files.
#include "TrickHLA/ElapsedTimeStats.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ElapsedTimeStats::ElapsedTimeStats()
   : first_pass( true ),
     elapsed_time( 0.0 ),
     prev_time( 0 ),
     count( 0 ),
     min( 0 ),
     max( 0 ),
     time_sum( 0.0 ),
     time_squared_sum( 0.0 )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ElapsedTimeStats::~ElapsedTimeStats()
{
   return;
}

/*!
 * @job_class{scheduled}
 */
void ElapsedTimeStats::measure()
{
   long long time = clock_wall_time(); // in microseconds
   if ( first_pass ) {
      first_pass = false;
   } else {
      elapsed_time = time - prev_time;
      if ( count == 0 ) {
         max = elapsed_time;
         min = elapsed_time;
      } else {
         if ( elapsed_time > max ) {
            max = elapsed_time;
         } else if ( elapsed_time < min ) {
            min = elapsed_time;
         }
      }
      time_sum += elapsed_time;
      time_squared_sum += elapsed_time * elapsed_time;
      ++count;
   }
   prev_time = time;
}

/*!
 * @job_class{scheduled}
 */
std::string ElapsedTimeStats::to_string()
{
   stringstream msg;
   msg << "ElapsedTimeStats::to_string():" << __LINE__
       << " count:" << count;

   if ( count > 0 ) {
      double mean = time_sum / count;

      // Calculate the corrected sample standard deviation from the unbiased
      // sample variance.
      // See https://en.wikipedia.org/wiki/Standard_deviation
      // See https://en.wikipedia.org/wiki/Bessel%27s_correction
      double variance = ( ( time_squared_sum / count ) - ( mean * mean ) );
      if ( count > 1 ) {
         variance *= (double)count / (double)( count - 1 );
      }
      double time_std_dev = sqrt( abs( variance ) );

      msg << " min:" << ( min * 0.001 ) << " milliseconds"
          << " max:" << ( max * 0.001 ) << " milliseconds"
          << " mean:" << ( mean * 0.001 ) << " milliseconds"
          << " sample-std-dev:" << ( time_std_dev * 0.001 ) << " milliseconds";
   } else {
      msg << " min:N/A"
          << " max:N/A"
          << " mean:N/A"
          << " sample-std-dev:N/A";
   }
   return msg.str();
}
