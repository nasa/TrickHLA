/*!
@file TrickHLA/utils/ElapsedTimeStats.cpp
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

// System includes.
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/clock_proto.h"

// TrickHLA includes.
#include "TrickHLA/utils/ElapsedTimeStats.hh"

using namespace std;
using namespace TrickHLA;

ElapsedTimeStats::ElapsedTimeStats()
   : ElapsedTimeStats( true )
{
   return;
}

ElapsedTimeStats::ElapsedTimeStats(
   bool const enable )
   : ElapsedTimeStats( enable, "" )
{
   return;
}

ElapsedTimeStats::ElapsedTimeStats(
   bool const    enable,
   string const &message )
   : enabled( enable ),
     description( message ),
     start_time( 0 ),
     count( 0 ),
     elapsed_time( 0.0 ),
     elapsed_time_prev( 0.0 ),
     elapsed_time_avg( 0.0 ),
     elapsed_time_min( 0.0 ),
     elapsed_time_max( 0.0 ),
     elapsed_time_sum( 0.0 ),
     elapsed_time_sq_sum( 0.0 ),
     jitter( 0.0 ),
     jitter_avg( 0.0 ),
     jitter_min( 0.0 ),
     jitter_max( 0.0 ),
     jitter_sum( 0.0 )
{
   return;
}

ElapsedTimeStats::~ElapsedTimeStats()
{
   return;
}

void ElapsedTimeStats::start_timer()
{
   if ( enabled ) {
      start_time = clock_wall_time(); // integer in microseconds
   }
}

void ElapsedTimeStats::measure()
{
   if ( enabled ) {
      if ( start_time == 0 ) {
         start_timer();
      } else {
         ++count;

         int64_t const time = clock_wall_time(); // integer in microseconds

         elapsed_time = ( time - start_time ) * 0.001;       // milliseconds
         elapsed_time_sum += elapsed_time;                   // milliseconds
         elapsed_time_sq_sum += elapsed_time * elapsed_time; // milliseconds^2
         elapsed_time_avg = elapsed_time_sum / count;        // milliseconds

         if ( count > 1 ) {
            if ( elapsed_time > elapsed_time_max ) {
               elapsed_time_max = elapsed_time;
            } else if ( elapsed_time < elapsed_time_min ) {
               elapsed_time_min = elapsed_time;
            }
         } else {
            elapsed_time_max = elapsed_time; // milliseconds
            elapsed_time_min = elapsed_time; // milliseconds
         }

         // Cannot calculate the jitter until the second measured elapsed time
         // because the jitter is a difference between consecutive elapsed times
         // so we need at least 2 measurements.
         if ( count > 2 ) {
            jitter = abs( elapsed_time - elapsed_time_prev );
            jitter_sum += jitter;

            // Jitter count is always 1 less than elapsed time count.
            jitter_avg = jitter_sum / ( count - 1 );

            if ( jitter > jitter_max ) {
               jitter_max = jitter;
            } else if ( jitter < jitter_min ) {
               jitter_min = jitter;
            }
         } else if ( count == 2 ) {
            jitter     = abs( elapsed_time - elapsed_time_prev );
            jitter_sum = jitter;
            jitter_avg = jitter;
            jitter_max = jitter; // milliseconds
            jitter_min = jitter; // milliseconds
         }

         elapsed_time_prev = elapsed_time;
         start_time        = time;
      }
   }
}

double ElapsedTimeStats::confidence_to_Z(
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

std::string const ElapsedTimeStats::to_string()
{
   stringstream msg;
   msg << "TrickHLA::ElapsedTimeStats::to_string():" << __LINE__ << "\n";

   if ( !description.empty() ) {
      msg << description << "\n";
   }

   if ( !any_measurements() ) {
      msg << "No measurements taken.\n";
   } else {
      double const mean = elapsed_time_sum / (double)count; // milliseconds

      // Determine the number of samples for statistical significance.
      // http://www.itl.nist.gov/div898//handbook/prc/section2/prc222.htm
      // https://www.isixsigma.com/tools-templates/sampling-data/how-determine-sample-size-determining-sample-size/
      // N >= ((Z * std_dev)/M)^2 for 99.9% confidence level with a margin of
      // error of M (i.e. mean +/- M).
      //
      double       confidence = 0.999;
      double const Z          = confidence_to_Z( confidence );

      // Calculate the corrected sample standard deviation from the unbiased
      // sample variance.
      // See https://en.wikipedia.org/wiki/Standard_deviation
      // See https://en.wikipedia.org/wiki/Bessel%27s_correction
      //
      double variance = ( elapsed_time_sq_sum / count ) - ( mean * mean );
      if ( count > 1 ) {
         variance *= (double)count / (double)( count - 1 );
      }
      double const std_dev = sqrt( abs( variance ) ); // milliseconds

      // Goal: To estimate the average elapsed time between reads to within
      // some percent (? milliseconds) of the mean (margin of error) with a
      // 99.9% confidence level we need at least N samples based on the
      // statistics.
      //
      // Use a Margin of Error (M) that is 0.25% within the mean value.
      double const M_percent = 0.0025;
      double const M         = mean * M_percent; // milliseconds

      int64_t min_sample_size;
      if ( std_dev != 0.0 ) {
         if ( M != 0.0 ) {
            // √N >= (Z * std_dev) / M
            double const sqrt_N = ( Z * std_dev ) / M;

            // N >= ((Z * std_dev) / M)^2
            min_sample_size = (int64_t)ceil( sqrt_N * sqrt_N );
         } else {
            min_sample_size = INT64_MAX;
         }
      } else {
         min_sample_size = 1;
      }

      // Calculate the margin of error based on the statistics.
      // M = (Z * std_dev) / √N
      double const moe = ( Z * std_dev ) / sqrt( count ); // milliseconds

      double moe_percent;
      if ( moe != 0.0 ) {
         if ( mean != 0.0 ) {
            moe_percent = moe / mean;
         } else {
            moe_percent = std::numeric_limits< double >::max();
         }
      } else {
         moe_percent = 0.0;
      }

      // We have to double escape the % sign so the Trick message_publish will
      // print the percent character '%' correctly and not as a c-string
      // formating code.
      msg << "    sample-count: " << count << "\n"
          << "             min: " << elapsed_time_min << " milliseconds\n"
          << "             max: " << elapsed_time_max << " milliseconds\n"
          << "            mean: " << mean << " milliseconds\n"
          << "  sample-std-dev: " << std_dev << " milliseconds\n"
          << " margin-of-error: " << ( moe_percent * 100.0 ) << "%% (" << moe
          << " milliseconds) with " << ( confidence * 100.0 ) << "%% confidence\n"
          << " min-sample-size: " << min_sample_size << "\n"
          << "    jitter-count: " << ( count - 1 ) << "\n"
          << "     jitter-mean: " << jitter_avg << " milliseconds\n"
          << "      jitter-min: " << jitter_min << " milliseconds\n"
          << "      jitter-max: " << jitter_max << " milliseconds\n"
          << "        guidance: To estimate the average elapsed time between measurements to within a "
          << ( M_percent * 100.0 ) << "%% (" << M << " milliseconds) margin of error with a "
          << ( confidence * 100.0 ) << "%% confidence, at least "
          << min_sample_size << " samples are needed based on the statistics.\n";
   }
   return msg.str();
}
