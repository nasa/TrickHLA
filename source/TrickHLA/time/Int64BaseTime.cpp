/*!
@file TrickHLA/time/Timeline.cpp
@ingroup TrickHLA
@brief This class represents an integer time for a given base time unit.

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
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2023, --, Base time for given time unit.}
@revs_end

*/

// System includes.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64BaseTime.hh"

using namespace std;
using namespace TrickHLA;

// Initialize the Int64BaseTime class variables.
HLABaseTimeEnum Int64BaseTime::base_unit            = HLA_BASE_TIME_MICROSECONDS;
std::string     Int64BaseTime::base_unit_string     = "microseconds";
int64_t         Int64BaseTime::base_time_multiplier = 1000000LL;

/*!
 * @brief Default constructor with microsecond base base_unit_string.
 */
Int64BaseTime::Int64BaseTime()
{
   set( HLA_BASE_TIME_MICROSECONDS );
}

/*!
 * @brief Constructor with base time multiplier specified.
 * @param multiplier The base time multiplier to use.
 */
Int64BaseTime::Int64BaseTime(
   int64_t const multiplier )
{
   set( multiplier );
}

/*!
 * @brief Constructor with base base_unit_string specified.
 * @param unit The base time unit to use.
 */
Int64BaseTime::Int64BaseTime(
   HLABaseTimeEnum const unit )
{
   set( unit );
}

/*!
 * @brief Destructor for the TrickHLA Int64BaseTime class.
 */
Int64BaseTime::~Int64BaseTime()
{
   return;
}

/*! @brief Set the base time resolution multiplier.
 *  @param multiplier The base time multiplier to use. */
void Int64BaseTime::set(
   int64_t const multiplier )
{
   if ( multiplier < 1LL ) {
      ostringstream errmsg;
      errmsg << "Int64BaseTime::set():" << __LINE__
             << " ERROR: The base-time multiplier specified (" << multiplier
             << ") must be greater than or equal to 1!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   base_time_multiplier = multiplier;

   update_unit_for_multiplier();
}

/*!
 * @brief Set the base time resolution for the enum value.
 * @param unit The base time unit to use.
 */
void Int64BaseTime::set(
   HLABaseTimeEnum unit )
{
   base_unit = unit;

   switch ( unit ) {
      case HLA_BASE_TIME_SECONDS: { // range: +/-292471208677.536 years with 1 second resolution
         base_time_multiplier = 1LL;
         base_unit_string     = "seconds";
         break;
      }
      case HLA_BASE_TIME_100_MILLISECONDS: { // range: +/-29247120867.753 years with 100 millisecond resolution
         base_time_multiplier = 10LL;
         base_unit_string     = "100-milliseconds";
         break;
      }
      case HLA_BASE_TIME_10_MILLISECONDS: { // range: +/-2924712086.775 years with 10 millisecond resolution
         base_time_multiplier = 100LL;
         base_unit_string     = "10-milliseconds";
         break;
      }
      case HLA_BASE_TIME_MILLISECONDS: { // range: +/-292471208.677 years with 1 millisecond resolution
         base_time_multiplier = 1000LL;
         base_unit_string     = "milliseconds";
         break;
      }
      case HLA_BASE_TIME_100_MICROSECONDS: { // range: +/-29247120.867 years with 100 microsecond resolution
         base_time_multiplier = 10000LL;
         base_unit_string     = "100-microseconds";
         break;
      }
      case HLA_BASE_TIME_10_MICROSECONDS: { // range: +/-2924712.086 years with 10 microsecond resolution
         base_time_multiplier = 100000LL;
         base_unit_string     = "10-microseconds";
         break;
      }
      case HLA_BASE_TIME_MICROSECONDS: { // range: +/-292471.208 years with 1 microsecond resolution
         base_time_multiplier = 1000000LL;
         base_unit_string     = "microseconds";
         break;
      }
      case HLA_BASE_TIME_100_NANOSECONDS: { // range: +/-29247.120 years with 100 nanosecond resolution
         base_time_multiplier = 10000000LL;
         base_unit_string     = "100-nanoseconds";
         break;
      }
      case HLA_BASE_TIME_10_NANOSECONDS: { // range: +/-2924.712 years with 10 nanosecond resolution
         base_time_multiplier = 100000000LL;
         base_unit_string     = "10-nanoseconds";
         break;
      }
      case HLA_BASE_TIME_NANOSECONDS: { // range: +/-292.471 years with 1 nanosecond resolution
         base_time_multiplier = 1000000000LL;
         base_unit_string     = "nanoseconds";
         break;
      }
      case HLA_BASE_TIME_100_PICOSECONDS: { // range: +/-29.247 years with 100 picosecond resolution
         base_time_multiplier = 10000000000LL;
         base_unit_string     = "100-picoseconds";
         break;
      }
      case HLA_BASE_TIME_10_PICOSECONDS: { // range: +/-2.924 years with 10 picosecond resolution
         base_time_multiplier = 100000000000LL;
         base_unit_string     = "10-picoseconds";
         break;
      }
      case HLA_BASE_TIME_PICOSECONDS: { // range: +/-2562.047 hours with 1 picosecond resolution
         base_time_multiplier = 1000000000000LL;
         base_unit_string     = "picoseconds";
         break;
      }
      case HLA_BASE_TIME_100_FEMTOSECONDS: { // range: +/-256.204 hours with 100 femosecond resolution
         base_time_multiplier = 10000000000000LL;
         base_unit_string     = "100-femtoseconds";
         break;
      }
      case HLA_BASE_TIME_10_FEMTOSECONDS: { // range: +/-25.620 hours with 10 femosecond resolution
         base_time_multiplier = 100000000000000LL;
         base_unit_string     = "10-femtoseconds";
         break;
      }
      case HLA_BASE_TIME_FEMTOSECONDS: { // range: +/-9223.372 seconds with 1 femosecond resolution
         base_time_multiplier = 1000000000000000LL;
         base_unit_string     = "femtoseconds";
         break;
      }
      case HLA_BASE_TIME_100_ATTOSECONDS: { // range: +/-922.337 seconds with 100 attosecond resolution
         base_time_multiplier = 10000000000000000LL;
         base_unit_string     = "100-attoseconds";
         break;
      }
      case HLA_BASE_TIME_10_ATTOSECONDS: { // range: +/-92.233 seconds with 10 attosecond resolution
         base_time_multiplier = 100000000000000000LL;
         base_unit_string     = "10-attoseconds";
         break;
      }
      case HLA_BASE_TIME_ATTOSECONDS: { // range: +/-9.223 seconds with 1 attosecond resolution
         base_time_multiplier = 1000000000000000000LL;
         base_unit_string     = "attoseconds";
         break;
      }
      case HLA_BASE_TIME_NOT_DEFINED: { // Not a pre-defined enum value.
         update_unit_for_multiplier();
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "Int64BaseTime::set():" << __LINE__
                << " ERROR: Unknown unit:" << unit << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
}

void Int64BaseTime::update_unit_for_multiplier()
{
   switch ( base_time_multiplier ) {
      case 1LL: {
         base_unit        = HLA_BASE_TIME_SECONDS;
         base_unit_string = "seconds";
         break;
      }
      case 10LL: {
         base_unit        = HLA_BASE_TIME_100_MILLISECONDS;
         base_unit_string = "100-milliseconds";
         break;
      }
      case 100LL: {
         base_unit        = HLA_BASE_TIME_10_MILLISECONDS;
         base_unit_string = "10-milliseconds";
         break;
      }
      case 1000LL: {
         base_unit        = HLA_BASE_TIME_MILLISECONDS;
         base_unit_string = "milliseconds";
         break;
      }
      case 10000LL: {
         base_unit        = HLA_BASE_TIME_100_MICROSECONDS;
         base_unit_string = "100-microseconds";
         break;
      }
      case 100000LL: {
         base_unit        = HLA_BASE_TIME_10_MICROSECONDS;
         base_unit_string = "10-microseconds";
         break;
      }
      case 1000000LL: {
         base_unit        = HLA_BASE_TIME_MICROSECONDS;
         base_unit_string = "microseconds";
         break;
      }
      case 10000000LL: {
         base_unit        = HLA_BASE_TIME_100_NANOSECONDS;
         base_unit_string = "100-nanoseconds";
         break;
      }
      case 100000000LL: {
         base_unit        = HLA_BASE_TIME_10_NANOSECONDS;
         base_unit_string = "10-nanoseconds";
         break;
      }
      case 1000000000LL: {
         base_unit        = HLA_BASE_TIME_NANOSECONDS;
         base_unit_string = "nanoseconds";
         break;
      }
      case 10000000000LL: {
         base_unit        = HLA_BASE_TIME_100_PICOSECONDS;
         base_unit_string = "100-picoseconds";
         break;
      }
      case 100000000000LL: {
         base_unit        = HLA_BASE_TIME_10_PICOSECONDS;
         base_unit_string = "10-picoseconds";
         break;
      }
      case 1000000000000LL: {
         base_unit        = HLA_BASE_TIME_PICOSECONDS;
         base_unit_string = "picoseconds";
         break;
      }
      case 10000000000000LL: {
         base_unit        = HLA_BASE_TIME_100_FEMTOSECONDS;
         base_unit_string = "100-femtoseconds";
         break;
      }
      case 100000000000000LL: {
         base_unit        = HLA_BASE_TIME_10_FEMTOSECONDS;
         base_unit_string = "10-femtoseconds";
         break;
      }
      case 1000000000000000LL: {
         base_unit        = HLA_BASE_TIME_FEMTOSECONDS;
         base_unit_string = "femtoseconds";
         break;
      }
      case 10000000000000000LL: {
         base_unit        = HLA_BASE_TIME_100_ATTOSECONDS;
         base_unit_string = "100-attoseconds";
         break;
      }
      case 100000000000000000LL: {
         base_unit        = HLA_BASE_TIME_10_ATTOSECONDS;
         base_unit_string = "10-attoseconds";
         break;
      }
      case 1000000000000000000LL: {
         base_unit        = HLA_BASE_TIME_ATTOSECONDS;
         base_unit_string = "attoseconds";
         break;
      }
      default: {
         base_unit        = HLA_BASE_TIME_NOT_DEFINED;
         base_unit_string = std::to_string( base_time_multiplier ) + " Ticks/second";
         break;
      }
   }
}

/*!
 * @brief A string representing the specified unit.
 * @param unit The base time unit.
 * @return A string representing the specified unit.
 */
std::string const Int64BaseTime::get_base_unit_enum_string(
   HLABaseTimeEnum const unit )
{
   switch ( unit ) {
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
      case HLA_BASE_TIME_ATTOSECONDS: {
         return "HLA_BASE_TIME_ATTOSECONDS";
      }
      case HLA_BASE_TIME_NOT_DEFINED:
      default: {
         return "HLA_BASE_TIME_NOT_DEFINED";
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
   for ( int64_t multiplier = 1;
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
   int64_t      multiplier )
{
   double seconds;
   return ( fabs( modf( value * multiplier, &seconds ) ) >= 1e-10 );
}

/*!
 * @brief Converts the given floating point time to an integer representing
 *  the time in the HLA Logical base time.
 * @return Time value in the HLA Logical base time.
 * @param time The time in seconds as a floating point double.
 */
int64_t const Int64BaseTime::to_base_time(
   double const time )
{
   return llround( time * base_time_multiplier );
}

/*!
 * @brief Converts the given integer time to an floating-point time representing seconds.
 * @return Time value in seconds.
 * @param time_in_base_unit Time value as a 64-bit integer in the base_unit_string specified for this class.
 */
double const Int64BaseTime::to_seconds(
   int64_t const time_in_base_unit )
{
   double const seconds    = (double)( time_in_base_unit / base_time_multiplier );
   double const fractional = (double)( time_in_base_unit % base_time_multiplier ) / (double)base_time_multiplier;
   return ( seconds + fractional );
}

/*!
 * @brief Converts the given integer time to an integer time representing whole seconds.
 * @return Time value in whole seconds.
 * @param time_in_base_unit Time value as a 64-bit integer in the base_unit_string specified for this class.
 */
int64_t const Int64BaseTime::to_whole_seconds(
   int64_t const time_in_base_unit )
{
   return ( time_in_base_unit / base_time_multiplier );
}
