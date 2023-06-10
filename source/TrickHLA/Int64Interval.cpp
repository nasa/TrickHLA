/*!
@file TrickHLA/Int64Interval.cpp
@ingroup TrickHLA
@brief This class represents the HLA Interval time.

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
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{Int64Interval.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Robert G. Phillips, Titan Corp., DIS, October 2004, --, Initial implementation for ISS HTV Sim}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2020, --, Changed function names to match TrickHLA coding style.}
@revs_end

*/

// System include files.
#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdio.h>
#include <string>

// TrickHLA include files.
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Types.hh"

namespace TrickHLA
{
extern int64_t const MICROS_MULTIPLIER        = 1000000;
extern int64_t const MAX_VALUE_IN_MICROS      = std::numeric_limits< int64_t >::max();
extern double const  MAX_LOGICAL_TIME_SECONDS = ( (double)MAX_VALUE_IN_MICROS / (double)MICROS_MULTIPLIER );
} // namespace TrickHLA

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   int64_t const value )
{
   set( value );
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   double const value )
{
   set( value );
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   RTI1516_NAMESPACE::LogicalTimeInterval const &value )
{
   set( value );
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   RTI1516_NAMESPACE::HLAinteger64Interval const &value )
   : hla_interval( value )
{
   return;
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   Int64Interval const &value )
   : hla_interval( value.get_time_in_micros() )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
Int64Interval::~Int64Interval()
{
   return;
}

int64_t Int64Interval::get_seconds() const
{
   return ( (int64_t)( this->hla_interval.getInterval() / MICROS_MULTIPLIER ) );
}

int32_t Int64Interval::get_micros() const
{
   return ( (int32_t)( this->hla_interval.getInterval() % MICROS_MULTIPLIER ) );
}

int64_t Int64Interval::get_time_in_micros() const
{
   return ( this->hla_interval.getInterval() );
}

double Int64Interval::get_time_in_seconds() const
{
   double const seconds = (double)get_seconds();
   double const micros  = (double)get_micros() / (double)MICROS_MULTIPLIER;
   return ( seconds + micros );
}

bool Int64Interval::is_zero() const
{
   return this->hla_interval.isZero();
}

wstring Int64Interval::to_wstring() const
{
   ostringstream msg;
   msg << "Int64Interval<" << get_time_in_seconds() << ">";
   wstring wstr;
   wstr.assign( msg.str().begin(), msg.str().end() );
   return wstr;
}

void Int64Interval::set(
   int64_t const value )
{
   this->hla_interval = value;
}

void Int64Interval::set(
   double const value )
{
   this->hla_interval = to_microseconds( value );
}

void Int64Interval::set(
   RTI1516_NAMESPACE::LogicalTimeInterval const &value )
{
   RTI1516_NAMESPACE::HLAinteger64Interval const &t = dynamic_cast< RTI1516_NAMESPACE::HLAinteger64Interval const & >( value );

   this->hla_interval = t.getInterval();
}

int64_t Int64Interval::to_microseconds(
   double const value )
{
   // Do a range check on the double value in seconds.
   if ( value > MAX_LOGICAL_TIME_SECONDS ) {
      return MAX_VALUE_IN_MICROS;
   } else if ( value < -MAX_LOGICAL_TIME_SECONDS ) {
      return -MAX_VALUE_IN_MICROS;
   }

   // A more efficient way to calculate the time in microseconds by avoiding fmod().
   double        seconds;
   int64_t const micros = (int64_t)round( modf( value, &seconds ) * MICROS_MULTIPLIER );
   return ( ( (int64_t)seconds * MICROS_MULTIPLIER ) + micros );

   //   int64_t const seconds = (int64_t)trunc( value );
   //   int64_t const micros  = ( seconds >= 0 ) ? (int64_t)( fmod( value * MICROS_MULTIPLIER, MICROS_MULTIPLIER ) + 0.5 )
   //                                            : (int64_t)( fmod( value * MICROS_MULTIPLIER, MICROS_MULTIPLIER ) - 0.5 );
   //   return ( ( seconds * MICROS_MULTIPLIER ) + micros );
}

double Int64Interval::to_seconds(
   int64_t const usec )
{
   double const seconds = (double)( usec / (int64_t)MICROS_MULTIPLIER );
   double const micros  = (double)( usec % (int64_t)MICROS_MULTIPLIER ) / (double)MICROS_MULTIPLIER;
   return ( seconds + micros );
}
