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
#include <cstdint>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/Executive.hh"
#include "trick/clock_proto.h"
#include "trick/exec_proto.h"

// TrickHLA include files.
#include "TrickHLA/ElapsedTimeStats.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ElapsedTimeStats::ElapsedTimeStats()
   : first_pass( true ),
     prev_time( 0 ),
     count( 0 ),
     elapsed_time( 0.0 ),
     min( 0.0 ),
     max( 0.0 ),
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
   // Number of time tics per second.
   int const time_tic_value = exec_get_time_tic_value();

   // Get the integer time in microseconds.
   int64_t const time = ( time_tic_value == 1000000 )
                           ? clock_wall_time()
                           : ( ( clock_wall_time() * 1000000 ) / time_tic_value );
   if ( first_pass ) {
      first_pass = false;
   } else {
      elapsed_time = ( time - prev_time ) * 0.001; // milliseconds
      if ( count == 0 ) {
         max = elapsed_time; // milliseconds
         min = elapsed_time; // milliseconds
      } else {
         if ( elapsed_time > max ) {
            max = elapsed_time;
         } else if ( elapsed_time < min ) {
            min = elapsed_time;
         }
      }
      time_sum += elapsed_time;                        // milliseconds
      time_squared_sum += elapsed_time * elapsed_time; // milliseconds^2
      ++count;
   }
   prev_time = time;
}

/*!
 * @job_class{scheduled}
 */
double const ElapsedTimeStats::confidence_to_Z(
   double &confidence )
{
   // The confidence level to Z values used below can be found here:
   // https://www.calculator.net/confidence-interval-calculator.html
   double Z;
   if ( confidence >= 0.99999 ) {
      confidence = 0.99999;
      Z          = 4.417;
   } else if ( confidence >= 0.9999 ) {
      confidence = 0.9999;
      Z          = 3.891;
   } else if ( confidence >= 0.999 ) {
      confidence = 0.999;
      Z          = 3.291;
   } else if ( confidence >= 0.995 ) {
      confidence = 0.995;
      Z          = 2.807;
   } else if ( confidence >= 0.99 ) {
      confidence = 0.99;
      Z          = 2.576;
   } else if ( confidence >= 0.98 ) {
      confidence = 0.98;
      Z          = 2.326;
   } else if ( confidence >= 0.95 ) {
      confidence = 0.95;
      Z          = 1.960;
   } else if ( confidence >= 0.90 ) {
      confidence = 0.90;
      Z          = 1.645;
   } else {
      confidence = 0.80;
      Z          = 1.282;
   }
   return Z;
}

/*!
 * @job_class{scheduled}
 */
std::string const ElapsedTimeStats::to_string()
{
   stringstream msg;
   msg << "ElapsedTimeStats::to_string():" << __LINE__ << '\n';

   if ( count > 0 ) {
      double mean = time_sum / (double)count; // milliseconds

      // Calculate the corrected sample standard deviation from the unbiased
      // sample variance.
      // See https://en.wikipedia.org/wiki/Standard_deviation
      // See https://en.wikipedia.org/wiki/Bessel%27s_correction
      //
      double variance = ( time_squared_sum / (double)count ) - ( mean * mean );
      if ( count > 1 ) {
         variance *= (double)count / (double)( count - 1 );
      }
      double std_dev = sqrt( abs( variance ) ); // milliseconds

      // Determine the number of samples for statistical significance.
      // http://www.itl.nist.gov/div898//handbook/prc/section2/prc222.htm
      // https://www.isixsigma.com/tools-templates/sampling-data/how-determine-sample-size-determining-sample-size/
      // N >= ((Z * std_dev)/M)^2 for 99.9% confidence level with a margin of
      // error of M (i.e. mean +/- M).
      //
      double confidence = 0.999;
      double Z          = confidence_to_Z( confidence );

      // Goal: To estimate the average elapsed time between reads to within
      // some percent (? milliseconds) of the mean (margin of error) with a
      // 99.9% confidence level we need at least N samples based on the
      // statistics.
      //
      // Use a Margin of Error (M) that is 0.25% within the mean value.
      double M_percent = 0.0025;
      double M         = mean * M_percent; // milliseconds

      // √N >= (Z * std_dev) / M
      double sqrt_N = Z * std_dev / M;

      // N >= ((Z * std_dev) / M)^2
      int64_t min_sample_size = (int64_t)ceil( sqrt_N * sqrt_N );

      // Calculate the margin of error based on the statistics.
      // M = (Z * std_dev) / √N
      double moe         = ( Z * std_dev ) / sqrt( count ); // milliseconds
      double moe_percent = moe / mean;

      // We have to double escape the % sign so that message_publish() will
      // print the percent character '%' correctly and not as a c-string
      // formating code.
      msg << "    sample-count: " << count << '\n'
          << "             min: " << min << " milliseconds\n"
          << "             max: " << max << " milliseconds\n"
          << "            mean: " << mean << " milliseconds\n"
          << "  sample-std-dev: " << std_dev << " milliseconds\n"
          << " margin-of-error: " << ( moe_percent * 100.0 )
          << "%%%% (" << moe << " milliseconds) with "
          << ( confidence * 100.0 ) << "%%%% confidence\n"
          << " min-sample-size: " << min_sample_size << '\n'
          << "        guidance: To estimate the average elapsed time between reads to within a "
          << ( M_percent * 100.0 ) << "%%%% ("
          << M << " milliseconds) margin of error with a "
          << ( confidence * 100.0 ) << "%%%% confidence level we need at least "
          << min_sample_size << " samples based on the statistics.";
   } else {
      msg << "    sample-count: " << count << '\n'
          << "             min: N/A\n"
          << "             max: N/A\n"
          << "            mean: N/A\n"
          << "  sample-std-dev: N/A";
   }
   return msg.str();
}
