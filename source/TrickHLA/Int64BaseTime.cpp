/*!
@file TrickHLA/Timeline.cpp
@ingroup TrickHLA
@brief This class represents an integer time for a given base time units.

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
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2023, --, Base time for given time units.}
@revs_end

*/

// System include files.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>

// Trick include files.

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;

// Initialize the Int64BaseTime class variables.
HLABaseTimeEnum Int64BaseTime::base_units               = HLA_BASE_TIME_MICROSECONDS;
std::string     Int64BaseTime::units_string             = "microseconds";
int64_t         Int64BaseTime::base_time_multiplier     = 1000000LL;
double          Int64BaseTime::max_logical_time_seconds = ( (double)std::numeric_limits< int64_t >::max() / (double)1000000 );

/*!
 * @job_class{initialization}
 */
Int64BaseTime::Int64BaseTime()
{
   set( HLA_BASE_TIME_MICROSECONDS );
}

/*!
 * @job_class{initialization}
 */
Int64BaseTime::Int64BaseTime(
   HLABaseTimeEnum const units )
{
   set( units );
}

/*!
 * @job_class{shutdown}
 */
Int64BaseTime::~Int64BaseTime()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void Int64BaseTime::set(
   HLABaseTimeEnum const units )
{
   switch ( base_units ) {
      case HLA_BASE_TIME_SECONDS: // range: +/-292471208677.536 years with 1 second resolution
         base_time_multiplier = 1LL;
         units_string         = "seconds";
         break;
      case HLA_BASE_TIME_100_MILLISECONDS: // range: +/-29247120867.753 years with 100 millisecond resolution
         base_time_multiplier = 10LL;
         units_string         = "100-milliseconds";
         break;
      case HLA_BASE_TIME_10_MILLISECONDS: // range: +/-2924712086.775 years with 10 millisecond resolution
         base_time_multiplier = 100LL;
         units_string         = "10-milliseconds";
         break;
      case HLA_BASE_TIME_MILLISECONDS: // range: +/-292471208.677 years with 1 millisecond resolution
         base_time_multiplier = 1000LL;
         units_string         = "milliseconds";
         break;
      case HLA_BASE_TIME_100_MICROSECONDS: // range: +/-29247120.867 years with 100 microsecond resolution
         base_time_multiplier = 10000LL;
         units_string         = "100-microseconds";
         break;
      case HLA_BASE_TIME_10_MICROSECONDS: // range: +/-2924712.086 years with 10 microsecond resolution
         base_time_multiplier = 100000LL;
         units_string         = "10-microseconds";
         break;
      case HLA_BASE_TIME_MICROSECONDS: // range: +/-292471.208 years with 1 microsecond resolution
         base_time_multiplier = 1000000LL;
         units_string         = "microseconds";
         break;
      case HLA_BASE_TIME_100_NANOSECONDS: // range: +/-29247.120 years with 100 nanosecond resolution
         base_time_multiplier = 10000000LL;
         units_string         = "100-nanoseconds";
         break;
      case HLA_BASE_TIME_10_NANOSECONDS: // range: +/-2924.712 years with 10 nanosecond resolution
         base_time_multiplier = 100000000LL;
         units_string         = "10-nanoseconds";
         break;
      case HLA_BASE_TIME_NANOSECONDS: // range: +/-292.471 years with 1 nanosecond resolution
         base_time_multiplier = 1000000000LL;
         units_string         = "nanoseconds";
         break;
      case HLA_BASE_TIME_100_PICOSECONDS: // range: +/-29.247 years with 100 picosecond resolution
         base_time_multiplier = 10000000000LL;
         units_string         = "100-picoseconds";
         break;
      case HLA_BASE_TIME_10_PICOSECONDS: // range: +/-2.924 years with 10 picosecond resolution
         base_time_multiplier = 100000000000LL;
         units_string         = "10-picoseconds";
         break;
      case HLA_BASE_TIME_PICOSECONDS: // range: +/-2562.047 hours with 1 picosecond resolution
         base_time_multiplier = 1000000000000LL;
         units_string         = "picoseconds";
         break;
      case HLA_BASE_TIME_100_FEMTOSECONDS: // range: +/-256.204 hours with 100 femosecond resolution
         base_time_multiplier = 10000000000000LL;
         units_string         = "100-femtoseconds";
         break;
      case HLA_BASE_TIME_10_FEMTOSECONDS: // range: +/-25.620 hours with 10 femosecond resolution
         base_time_multiplier = 100000000000000LL;
         units_string         = "10-femtoseconds";
         break;
      case HLA_BASE_TIME_FEMTOSECONDS: // range: +/-9223.372 seconds with 1 femosecond resolution
         base_time_multiplier = 1000000000000000LL;
         units_string         = "femtoseconds";
         break;
      case HLA_BASE_TIME_100_ATTOSECONDS: // range: +/-922.337 seconds with 100 attosecond resolution
         base_time_multiplier = 10000000000000000LL;
         units_string         = "100-attoseconds";
         break;
      case HLA_BASE_TIME_10_ATTOSECONDS: // range: +/-92.233 seconds with 10 attosecond resolution
         base_time_multiplier = 100000000000000000LL;
         units_string         = "10-attoseconds";
         break;
      case HLA_BASE_TIME_ATTOSECONDS: // range: +/-9.223 seconds with 1 attosecond resolution
         base_time_multiplier = 1000000000000000000LL;
         units_string         = "attoseconds";
         break;
      default:
         ostringstream errmsg;
         errmsg << "Int64BaseTime::set():" << __LINE__
                << " ERROR: Unknown units:" << units << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
   }
   base_units               = units;
   max_logical_time_seconds = ( (double)std::numeric_limits< int64_t >::max() / (double)base_time_multiplier );
}

/*!
 * @job_class{initialization}
 */
string Int64BaseTime::get_units_that_exceeds(
   HLABaseTimeEnum const units )
{
   string msg = "";
   if ( units >= HLA_BASE_TIME_PICOSECONDS ) {
      msg = "Unknown";
   } else {
      int next_units = (int)units + 1;
      switch ( next_units ) {
         case HLA_BASE_TIME_SECONDS:
            msg += "HLA_BASE_TIME_SECONDS, ";
         case HLA_BASE_TIME_100_MILLISECONDS:
            msg += "HLA_BASE_TIME_100_MILLISECONDS, ";
         case HLA_BASE_TIME_10_MILLISECONDS:
            msg += "HLA_BASE_TIME_10_MILLISECONDS, ";
         case HLA_BASE_TIME_MILLISECONDS:
            msg += "HLA_BASE_TIME_MILLISECONDS, ";
         case HLA_BASE_TIME_100_MICROSECONDS:
            msg += "HLA_BASE_TIME_100_MICROSECONDS, ";
         case HLA_BASE_TIME_10_MICROSECONDS:
            msg += "HLA_BASE_TIME_10_MICROSECONDS, ";
         case HLA_BASE_TIME_MICROSECONDS:
            msg += "HLA_BASE_TIME_MICROSECONDS, ";
         case HLA_BASE_TIME_100_NANOSECONDS:
            msg += "HLA_BASE_TIME_100_NANOSECONDS, ";
         case HLA_BASE_TIME_10_NANOSECONDS:
            msg += "HLA_BASE_TIME_10_NANOSECONDS, ";
         case HLA_BASE_TIME_NANOSECONDS:
            msg += "HLA_BASE_TIME_NANOSECONDS, ";
         case HLA_BASE_TIME_100_PICOSECONDS:
            msg += "HLA_BASE_TIME_100_PICOSECONDS, ";
         case HLA_BASE_TIME_10_PICOSECONDS:
            msg += "HLA_BASE_TIME_10_PICOSECONDS, ";
         case HLA_BASE_TIME_PICOSECONDS:
            msg += "HLA_BASE_TIME_PICOSECONDS, ";
         case HLA_BASE_TIME_100_FEMTOSECONDS:
            msg += "HLA_BASE_TIME_100_FEMTOSECONDS, ";
         case HLA_BASE_TIME_10_FEMTOSECONDS:
            msg += "HLA_BASE_TIME_10_FEMTOSECONDS, ";
         case HLA_BASE_TIME_FEMTOSECONDS:
            msg += "HLA_BASE_TIME_FEMTOSECONDS, ";
         case HLA_BASE_TIME_100_ATTOSECONDS:
            msg += "HLA_BASE_TIME_100_ATTOSECONDS, ";
         case HLA_BASE_TIME_10_ATTOSECONDS:
            msg += "HLA_BASE_TIME_10_ATTOSECONDS, or ";
         case HLA_BASE_TIME_ATTOSECONDS:
         default:
            msg += "HLA_BASE_TIME_ATTOSECONDS";
            break;
      }
   }
   return msg;
}

/*!
 * @job_class{initialization}
 */
bool Int64BaseTime::exceeds_base_time_resolution(
   double const value )
{
   double seconds;
   return ( modf( value * base_time_multiplier, &seconds ) != 0.0 );
}

/*!
 * @job_class{initialization}
 */
bool Int64BaseTime::exceeds_base_time_resolution(
   double const value,
   long long    multiplier )
{
   double seconds;
   return ( modf( value * multiplier, &seconds ) != 0.0 );
}

/*!
 * @job_class{initialization}
 */
int64_t Int64BaseTime::to_base_time(
   double const value )
{
   // Do a range check on the double value in seconds.
   if ( value > max_logical_time_seconds ) {
      return std::numeric_limits< int64_t >::max();
   } else if ( value < -max_logical_time_seconds ) {
      return -std::numeric_limits< int64_t >::max();
   }

   // A more efficient way to calculate the time in the base units_string by avoiding fmod().
   double        seconds;
   int64_t const fractional = (int64_t)round( modf( value, &seconds ) * base_time_multiplier );
   return ( ( (int64_t)seconds * base_time_multiplier ) + fractional );

   // TODO: Benchmark the new code agaisnt this previous implementation to see
   // which is faster.
   // int64_t const seconds   = (int64_t)trunc( value );
   // int64_t const fractional = ( seconds >= 0 ) ? (int64_t)( fmod( value * MICROS_MULTIPLIER, MICROS_MULTIPLIER ) + 0.5 )
   //                                            : (int64_t)( fmod( value * MICROS_MULTIPLIER, MICROS_MULTIPLIER ) - 0.5 );
   // return ( ( seconds * MICROS_MULTIPLIER ) + fractional );
}

/*!
 * @job_class{initialization}
 */
double Int64BaseTime::to_seconds(
   int64_t const time_in_base_units )
{
   double const seconds    = (double)( time_in_base_units / base_time_multiplier );
   double const fractional = (double)( time_in_base_units % base_time_multiplier ) / (double)base_time_multiplier;
   return ( seconds + fractional );
}

/*!
 * @job_class{initialization}
 */
int64_t Int64BaseTime::to_whole_seconds(
   int64_t const time_in_base_units )
{
   return ( time_in_base_units / base_time_multiplier );
}
