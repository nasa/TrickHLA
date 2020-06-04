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

@revs_title
@revs_begin
@rev_entry{Robert G. Phillips, Titan Corp., DIS, October 2004, --, Initial implementation for ISS HTV Sim}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cmath>
#include <limits>
#include <stdint.h>
#include <stdio.h>

// TrickHLA include files.
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Types.hh"

namespace TrickHLA
{
extern const int64_t MICROS_MULTIPLIER        = 1000000;
extern const int64_t MAX_VALUE_IN_MICROS      = std::numeric_limits< int64_t >::max();
extern const double  MAX_LOGICAL_TIME_SECONDS = (double)MAX_VALUE_IN_MICROS / MICROS_MULTIPLIER;
} // namespace TrickHLA

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   int64_t value )
{
   setTo( value );
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   double value )
{
   setTo( value );
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   RTI1516_NAMESPACE::LogicalTimeInterval const &value )
{
   setTo( value );
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   RTI1516_NAMESPACE::HLAinteger64Interval const &value )
   : _HLAinterval( value )
{
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   Int64Interval const &value )
   : _HLAinterval( value.getTimeInMicros() )
{
}

/*!
 * @job_class{shutdown}
 */
Int64Interval::~Int64Interval()
{
}

long Int64Interval::getSeconds() const
{
   return ( (long)( _HLAinterval.getInterval() / MICROS_MULTIPLIER ) );
}

int Int64Interval::getMicros() const
{
   return ( (int)( _HLAinterval.getInterval() % MICROS_MULTIPLIER ) );
}

int64_t Int64Interval::getTimeInMicros() const
{
   return ( _HLAinterval.getInterval() );
}

double Int64Interval::getDoubleTime() const
{
   double t = (double)( (double)getMicros() / (double)MICROS_MULTIPLIER );
   t += (double)getSeconds();
   return ( t );
}

wstring Int64Interval::toString() const
{
   char buf[128];
   sprintf( buf, "Int64Interval<%0.06f>", getDoubleTime() );
   string  str( buf );
   wstring wstr;
   wstr.assign( str.begin(), str.end() );
   return wstr;
}

void Int64Interval::setTo(
   const int64_t value )
{
   _HLAinterval = value;
}

void Int64Interval::setTo(
   const double value )
{
   _HLAinterval = toMicroseconds( value );
}

void Int64Interval::setTo(
   RTI1516_NAMESPACE::LogicalTimeInterval const &value )
{
   const RTI1516_NAMESPACE::HLAinteger64Interval &p = dynamic_cast< const RTI1516_NAMESPACE::HLAinteger64Interval & >( value );
   _HLAinterval                                     = p.getInterval();
}

int64_t Int64Interval::toMicroseconds(
   const double value )
{
   // Do a range check on the double value.
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
