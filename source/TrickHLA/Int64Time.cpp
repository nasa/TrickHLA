/*!
@file TrickHLA/Timeline.cpp
@ingroup TrickHLA
@brief This class represents the HLA time.

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
#include <cstdio>

// Trick include files.

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/Constants.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   int64_t value )
{
   setTo( value );
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   double value )
{
   setTo( value );
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   RTI1516_NAMESPACE::LogicalTime const &value )
{
   setTo( value );
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   RTI1516_NAMESPACE::HLAinteger64Time const &value )
   : _HLAtime( value )
{
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   Int64Time const &value )
   : _HLAtime( value.getTimeInMicros() )
{
}

/*!
 * @job_class{shutdown}
 */
Int64Time::~Int64Time()
{
}

void Int64Time::decode(
   RTI1516_USERDATA const &user_supplied_tag )
{
   _HLAtime.decode( user_supplied_tag );
}

long Int64Time::getSeconds() const
{
   return ( (long)( _HLAtime.getTime() / MICROS_MULTIPLIER ) );
}

int Int64Time::getMicros() const
{
   return ( (int)( _HLAtime.getTime() % MICROS_MULTIPLIER ) );
}

int64_t Int64Time::getTimeInMicros() const
{
   return ( _HLAtime.getTime() );
}

double Int64Time::getDoubleTime() const
{
   double t = (double)( (double)getMicros() / (double)MICROS_MULTIPLIER );
   t += (double)getSeconds();
   return ( t );
}

wstring Int64Time::toString() const
{
   char buf[128];
   sprintf( buf, "Int64Time<%0.06f>", getDoubleTime() );
   string  str( buf );
   wstring wstr;
   wstr.assign( str.begin(), str.end() );
   return ( wstr );
}

void Int64Time::setTo(
   const int64_t value )
{
   _HLAtime.setTime( value );
}

void Int64Time::setTo(
   const double value )
{
   _HLAtime = Int64Interval::toMicroseconds( value );
}

void Int64Time::setTo(
   RTI1516_NAMESPACE::LogicalTime const &value )
{
   const RTI1516_NAMESPACE::HLAinteger64Time &p = dynamic_cast< const RTI1516_NAMESPACE::HLAinteger64Time & >( value );
   _HLAtime                                     = p.getTime();
}

void Int64Time::setTo(
   Int64Time const &value )
{
   _HLAtime = value.getTimeInMicros();
}
