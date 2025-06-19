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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2023, --, Base time for given time unit.}
@revs_end

*/

// System includes.
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Int64BaseTime.hh"

using namespace std;
using namespace TrickHLA;

// Initialize the Int64BaseTime class variables.
HLABaseTimeEnum Int64BaseTime::base_units               = HLA_BASE_TIME_MICROSECONDS;
std::string     Int64BaseTime::units_string             = "microseconds";
int64_t         Int64BaseTime::base_time_multiplier     = 1000000LL;
double          Int64BaseTime::max_logical_time_seconds = ( (double)INT64_MAX / (double)Int64BaseTime::base_time_multiplier );

/*!
 * @brief Default constructor with microsecond base units_string.
 */
Int64BaseTime::Int64BaseTime()
{
   set( HLA_BASE_TIME_MICROSECONDS );
}

/*!
 * @brief Constructor with base units_string specified.
 * @param units The base time units to use.
 */
Int64BaseTime::Int64BaseTime(
   HLABaseTimeEnum const units )
{
   set( units );
}

/*!
 * @brief Destructor for the TrickHLA Int64BaseTime class.
 */
Int64BaseTime::~Int64BaseTime()
{
   return;
}

/*!
 * @brief Determine if the specified value exceeds the base time resolution.
 * @param units The base time units to use.
 */
void Int64BaseTime::set(
   HLABaseTimeEnum const units )
{
   switch ( units ) {
      case HLA_BASE_TIME_SECONDS: { // range: +/-292471208677.536 years with 1 second resolution
         base_time_multiplier = 1LL;
         units_string         = "seconds";
         break;
      }
      case HLA_BASE_TIME_100_MILLISECONDS: { // range: +/-29247120867.753 years with 100 millisecond resolution
         base_time_multiplier = 10LL;
         units_string         = "100-milliseconds";
         break;
      }
      case HLA_BASE_TIME_10_MILLISECONDS: { // range: +/-2924712086.775 years with 10 millisecond resolution
         base_time_multiplier = 100LL;
         units_string         = "10-milliseconds";
         break;
      }
      case HLA_BASE_TIME_MILLISECONDS: { // range: +/-292471208.677 years with 1 millisecond resolution
         base_time_multiplier = 1000LL;
         units_string         = "milliseconds";
         break;
      }
      case HLA_BASE_TIME_100_MICROSECONDS: { // range: +/-29247120.867 years with 100 microsecond resolution
         base_time_multiplier = 10000LL;
         units_string         = "100-microseconds";
         break;
      }
      case HLA_BASE_TIME_10_MICROSECONDS: { // range: +/-2924712.086 years with 10 microsecond resolution
         base_time_multiplier = 100000LL;
         units_string         = "10-microseconds";
         break;
      }
      case HLA_BASE_TIME_MICROSECONDS: { // range: +/-292471.208 years with 1 microsecond resolution
         base_time_multiplier = 1000000LL;
         units_string         = "microseconds";
         break;
      }
      case HLA_BASE_TIME_100_NANOSECONDS: { // range: +/-29247.120 years with 100 nanosecond resolution
         base_time_multiplier = 10000000LL;
         units_string         = "100-nanoseconds";
         break;
      }
      case HLA_BASE_TIME_10_NANOSECONDS: { // range: +/-2924.712 years with 10 nanosecond resolution
         base_time_multiplier = 100000000LL;
         units_string         = "10-nanoseconds";
         break;
      }
      case HLA_BASE_TIME_NANOSECONDS: { // range: +/-292.471 years with 1 nanosecond resolution
         base_time_multiplier = 1000000000LL;
         units_string         = "nanoseconds";
         break;
      }
      case HLA_BASE_TIME_100_PICOSECONDS: { // range: +/-29.247 years with 100 picosecond resolution
         base_time_multiplier = 10000000000LL;
         units_string         = "100-picoseconds";
         break;
      }
      case HLA_BASE_TIME_10_PICOSECONDS: { // range: +/-2.924 years with 10 picosecond resolution
         base_time_multiplier = 100000000000LL;
         units_string         = "10-picoseconds";
         break;
      }
      case HLA_BASE_TIME_PICOSECONDS: { // range: +/-2562.047 hours with 1 picosecond resolution
         base_time_multiplier = 1000000000000LL;
         units_string         = "picoseconds";
         break;
      }
      case HLA_BASE_TIME_100_FEMTOSECONDS: { // range: +/-256.204 hours with 100 femosecond resolution
         base_time_multiplier = 10000000000000LL;
         units_string         = "100-femtoseconds";
         break;
      }
      case HLA_BASE_TIME_10_FEMTOSECONDS: { // range: +/-25.620 hours with 10 femosecond resolution
         base_time_multiplier = 100000000000000LL;
         units_string         = "10-femtoseconds";
         break;
      }
      case HLA_BASE_TIME_FEMTOSECONDS: { // range: +/-9223.372 seconds with 1 femosecond resolution
         base_time_multiplier = 1000000000000000LL;
         units_string         = "femtoseconds";
         break;
      }
      case HLA_BASE_TIME_100_ATTOSECONDS: { // range: +/-922.337 seconds with 100 attosecond resolution
         base_time_multiplier = 10000000000000000LL;
         units_string         = "100-attoseconds";
         break;
      }
      case HLA_BASE_TIME_10_ATTOSECONDS: { // range: +/-92.233 seconds with 10 attosecond resolution
         base_time_multiplier = 100000000000000000LL;
         units_string         = "10-attoseconds";
         break;
      }
      case HLA_BASE_TIME_ATTOSECONDS: { // range: +/-9.223 seconds with 1 attosecond resolution
         base_time_multiplier = 1000000000000000000LL;
         units_string         = "attoseconds";
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "Int64BaseTime::set():" << __LINE__
                << " ERROR: Unknown units:" << units << '\n';
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   base_units               = units;
   max_logical_time_seconds = ( (double)INT64_MAX / (double)base_time_multiplier );
}

/*!
 * @brief A string representing the specified units.
 * @param units The base time units.
 * @return A string representing the specified units.
 */
std::string const Int64BaseTime::get_units_string(
   HLABaseTimeEnum const units )
{
   switch ( units ) {
      case HLA_BASE_TIME_SECONDS: {
         return "HLA_BASE_TIME_SECONDS";
      }
      case HLA_BASE_TIME_100_MILLISECONDS: {
         return "HLA_BASE_TIME_100_MILLISECONDS";
      }
      case HLA_BASE_TIME_10_MILLISECONDS: {
         return "HLA_BASE_TIME_10_MILLISECONDS";
      }
      case HLA_BASE_TIME_MILLISECONDS: {
         return "HLA_BASE_TIME_MILLISECONDS";
      }
      case HLA_BASE_TIME_100_MICROSECONDS: {
         return "HLA_BASE_TIME_100_MICROSECONDS";
      }
      case HLA_BASE_TIME_10_MICROSECONDS: {
         return "HLA_BASE_TIME_10_MICROSECONDS";
      }
      case HLA_BASE_TIME_MICROSECONDS: {
         return "HLA_BASE_TIME_MICROSECONDS";
      }
      case HLA_BASE_TIME_100_NANOSECONDS: {
         return "HLA_BASE_TIME_100_NANOSECONDS";
      }
      case HLA_BASE_TIME_10_NANOSECONDS: {
         return "HLA_BASE_TIME_10_NANOSECONDS";
      }
      case HLA_BASE_TIME_NANOSECONDS: {
         return "HLA_BASE_TIME_NANOSECONDS";
      }
      case HLA_BASE_TIME_100_PICOSECONDS: {
         return "HLA_BASE_TIME_100_PICOSECONDS";
      }
      case HLA_BASE_TIME_10_PICOSECONDS: {
         return "HLA_BASE_TIME_10_PICOSECONDS";
      }
      case HLA_BASE_TIME_PICOSECONDS: {
         return "HLA_BASE_TIME_PICOSECONDS";
      }
      case HLA_BASE_TIME_100_FEMTOSECONDS: {
         return "HLA_BASE_TIME_100_FEMTOSECONDS";
      }
      case HLA_BASE_TIME_10_FEMTOSECONDS: {
         return "HLA_BASE_TIME_10_FEMTOSECONDS";
      }
      case HLA_BASE_TIME_FEMTOSECONDS: {
         return "HLA_BASE_TIME_FEMTOSECONDS";
      }
      case HLA_BASE_TIME_100_ATTOSECONDS: {
         return "HLA_BASE_TIME_100_ATTOSECONDS";
      }
      case HLA_BASE_TIME_10_ATTOSECONDS: {
         return "HLA_BASE_TIME_10_ATTOSECONDS";
      }
      case HLA_BASE_TIME_ATTOSECONDS:
      default: {
         return "HLA_BASE_TIME_ATTOSECONDS";
      }
   }
}

/*!
 * @brief Determine the best supporting base time resolution for the value.
 * @return The best supporting base time enum value.
 * @param value Time value as a floating point double in seconds.
 */
HLABaseTimeEnum const Int64BaseTime::best_base_time_resolution(
   double const value )
{
   int resolution = (int)HLA_BASE_TIME_SECONDS;
   for ( long long multiplier = 1;
         ( resolution < (int)HLA_BASE_TIME_ATTOSECONDS )
         && exceeds_base_time_resolution( value, multiplier );
         ++resolution, multiplier *= 10LL ) {
      // Do nothing
   }
   return (HLABaseTimeEnum)resolution;
}

/*!
 * @brief Determine if the specified value exceeds the resolution of
 *  the base time (i.e. value is much smaller than base time resolution).
 *  @return True if the value exceeds the resolution of the base time.
 * @param value Time value as a floating point double in seconds.
 */
bool const Int64BaseTime::exceeds_base_time_resolution(
   double const value )
{
   return exceeds_base_time_resolution( value, base_time_multiplier );
}

/*!
 * @brief Determine if the specified value exceeds the resolution of
 *  a base time with the corresponding multiplier.
 * @return True if the value exceeds the resolution of the base time.
 * @param value Time value as a floating point double in seconds.
 * @param multiplier Base time multiplier.
 */
bool const Int64BaseTime::exceeds_base_time_resolution(
   double const value,
   long long    multiplier )
{
   double seconds;
   return ( modf( value * multiplier, &seconds ) != 0.0 );
}

/*!
 * @brief Converts the given floating point time to an integer representing
 *  the time in the HLA Logical base time.
 * @return Time value in the HLA Logical base time.
 * @param value Time value as a floating point double in seconds.
 */
int64_t const Int64BaseTime::to_base_time(
   double const value )
{
   // Do a range check on the double value in seconds.
   if ( value > max_logical_time_seconds ) {
      return INT64_MAX;
   } else if ( value < -max_logical_time_seconds ) {
      return -INT64_MAX;
   }

   // A more efficient way to calculate the time in the base units_string by avoiding fmod().
   double        seconds;
   int64_t const fractional = (int64_t)round( modf( value, &seconds ) * base_time_multiplier );
   return ( ( (int64_t)seconds * base_time_multiplier ) + fractional );

   // TODO: Benchmark the new code against this previous implementation to see
   // which is faster.
   // int64_t const seconds    = (int64_t)trunc( value );
   // int64_t const fractional = ( seconds >= 0 ) ? (int64_t)( fmod( value * MICROS_MULTIPLIER, MICROS_MULTIPLIER ) + 0.5 )
   //                                             : (int64_t)( fmod( value * MICROS_MULTIPLIER, MICROS_MULTIPLIER ) - 0.5 );
   // return ( ( seconds * MICROS_MULTIPLIER ) + fractional );
}

/*!
 * @brief Converts the given integer time to an floating-point time representing seconds.
 * @return Time value in seconds.
 * @param time_in_base_units Time value as a 64-bit integer in the units_string specified for this class.
 */
double const Int64BaseTime::to_seconds(
   int64_t const time_in_base_units )
{
   double const seconds    = (double)( time_in_base_units / base_time_multiplier );
   double const fractional = (double)( time_in_base_units % base_time_multiplier ) / (double)base_time_multiplier;
   return ( seconds + fractional );
}

/*!
 * @brief Converts the given integer time to an integer time representing whole seconds.
 * @return Time value in whole seconds.
 * @param time_in_base_units Time value as a 64-bit integer in the units_string specified for this class.
 */
int64_t const Int64BaseTime::to_whole_seconds(
   int64_t const time_in_base_units )
{
   return ( time_in_base_units / base_time_multiplier );
}
