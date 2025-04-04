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
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Int64Time.cpp}
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

// TrickHLA include files.
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/BasicDataElements.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
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
   LogicalTime const &value )
{
   set( value );
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   HLAinteger64Time const &value )
   : hla_time( value )
{
   return;
}

/*!
 * @job_class{initialization}
 */
Int64Time::Int64Time(
   Int64Time const &value )
   : hla_time( value.get_base_time() )
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

RTI1516_USERDATA Int64Time::encode() const
{
   return hla_time.encode();
}

void Int64Time::decode(
   RTI1516_USERDATA const &user_supplied_tag )
{
   hla_time.decode( user_supplied_tag );
}

int64_t Int64Time::get_seconds() const
{
   return ( (int64_t)( hla_time.getTime() / Int64BaseTime::get_base_time_multiplier() ) );
}

int64_t Int64Time::get_fractional_seconds() const
{
   return ( (int64_t)( hla_time.getTime() % Int64BaseTime::get_base_time_multiplier() ) );
}

int64_t Int64Time::get_base_time() const
{
   return ( hla_time.getTime() );
}

double Int64Time::get_time_in_seconds() const
{
   double const seconds    = (double)get_seconds();
   double const fractional = (double)get_fractional_seconds() / (double)Int64BaseTime::get_base_time_multiplier();
   return ( seconds + fractional );
}

string Int64Time::to_string() const
{
   ostringstream msg;
   msg << "Int64Time<" << get_time_in_seconds() << ">";
   return msg.str();
}

void Int64Time::set(
   int64_t const value )
{
   hla_time.setTime( value );
}

void Int64Time::set(
   double const value )
{
   this->hla_time = Int64BaseTime::to_base_time( value );
}

void Int64Time::set(
   LogicalTime const &value )
{
   HLAinteger64Time const &t = dynamic_cast< HLAinteger64Time const & >( value );

   this->hla_time = t.getTime();
}

void Int64Time::set(
   Int64Time const &value )
{
   this->hla_time = value.get_base_time();
}
