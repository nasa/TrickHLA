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
#include <limits>
#include <sstream>
#include <stdint.h>
#include <stdio.h>

// TrickHLA include files.
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Types.hh"

namespace TrickHLA
{
extern const int64_t MICROS_MULTIPLIER        = 1000000;
extern const int64_t MAX_VALUE_IN_MICROS      = std::numeric_limits< int64_t >::max();
extern const double  MAX_LOGICAL_TIME_SECONDS = ( (double)MAX_VALUE_IN_MICROS / (double)MICROS_MULTIPLIER );
} // namespace TrickHLA

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   int64_t value )
{
   set( value );
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   double value )
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
   return ( ( int64_t )( hla_interval.getInterval() / MICROS_MULTIPLIER ) );
}

int32_t Int64Interval::get_micros() const
{
   return ( ( int32_t )( hla_interval.getInterval() % MICROS_MULTIPLIER ) );
}

int64_t Int64Interval::get_time_in_micros() const
{
   return ( hla_interval.getInterval() );
}

double Int64Interval::get_time_in_seconds() const
{
   double seconds = (double)get_seconds();
   double micros  = (double)get_micros() / (double)MICROS_MULTIPLIER;
   return ( seconds + micros );
}

wstring Int64Interval::to_string() const
{
   ostringstream msg;
   msg << "Int64Interval<" << get_time_in_seconds() << ">";
   wstring wstr;
   wstr.assign( msg.str().begin(), msg.str().end() );
   return wstr;
}

void Int64Interval::set(
   const int64_t value )
{
   hla_interval = value;
}

void Int64Interval::set(
   const double value )
{
   hla_interval = to_microseconds( value );
}

void Int64Interval::set(
   RTI1516_NAMESPACE::LogicalTimeInterval const &value )
{
   const RTI1516_NAMESPACE::HLAinteger64Interval &p = dynamic_cast< const RTI1516_NAMESPACE::HLAinteger64Interval & >( value );

   hla_interval = p.getInterval();
}

int64_t Int64Interval::to_microseconds(
   const double value )
{
   // Do a range check on the double value in seconds.
   if ( value > MAX_LOGICAL_TIME_SECONDS ) {
      return MAX_VALUE_IN_MICROS;
   } else if ( value < -MAX_LOGICAL_TIME_SECONDS ) {
      return -MAX_VALUE_IN_MICROS;
   }
   int64_t seconds = (int64_t)trunc( value );
   int64_t micros  = ( seconds >= 0 ) ? ( int64_t )( fmod( value * MICROS_MULTIPLIER, MICROS_MULTIPLIER ) + 0.5 )
                                      : ( int64_t )( fmod( value * MICROS_MULTIPLIER, MICROS_MULTIPLIER ) - 0.5 );
   return ( ( seconds * MICROS_MULTIPLIER ) + micros );
}

double Int64Interval::to_seconds(
   const int64_t usec )
{
   double seconds = (double)( usec / (int64_t)MICROS_MULTIPLIER );
   double micros  = (double)( usec % (int64_t)MICROS_MULTIPLIER ) / (double)MICROS_MULTIPLIER;
   return ( seconds + micros );
}
