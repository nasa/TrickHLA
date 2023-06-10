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
#include <cstdio>
#include <sstream>
#include <string>

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
   int64_t const value )
{
   set( value );
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   double const value )
{
   set( value );
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   RTI1516_NAMESPACE::LogicalTime const &value )
{
   set( value );
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   RTI1516_NAMESPACE::HLAinteger64Time const &value )
   : hla_time( value )
{
   return;
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   Int64Time const &value )
   : hla_time( value.get_time_in_micros() )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
Int64Time::~Int64Time()
{
   return;
}

void Int64Time::decode(
   RTI1516_USERDATA const &user_supplied_tag )
{
   this->hla_time.decode( user_supplied_tag );
}

int64_t Int64Time::get_seconds() const
{
   return ( (int64_t)( this->hla_time.getTime() / MICROS_MULTIPLIER ) );
}

int32_t Int64Time::get_micros() const
{
   return ( (int32_t)( this->hla_time.getTime() % MICROS_MULTIPLIER ) );
}

int64_t Int64Time::get_time_in_micros() const
{
   return ( this->hla_time.getTime() );
}

double Int64Time::get_time_in_seconds() const
{
   double const seconds = (double)get_seconds();
   double const micros  = (double)get_micros() / (double)MICROS_MULTIPLIER;
   return ( seconds + micros );
}

wstring Int64Time::to_wstring() const
{
   ostringstream msg;
   msg << "Int64Time<" << get_time_in_seconds() << ">";
   wstring wstr;
   wstr.assign( msg.str().begin(), msg.str().end() );
   return wstr;
}

void Int64Time::set(
   int64_t const value )
{
   this->hla_time.setTime( value );
}

void Int64Time::set(
   double const value )
{
   this->hla_time = Int64Interval::to_microseconds( value );
}

void Int64Time::set(
   RTI1516_NAMESPACE::LogicalTime const &value )
{
   RTI1516_NAMESPACE::HLAinteger64Time const &t = dynamic_cast< RTI1516_NAMESPACE::HLAinteger64Time const & >( value );

   this->hla_time = t.getTime();
}

void Int64Time::set(
   Int64Time const &value )
{
   this->hla_time = value.get_time_in_micros();
}
