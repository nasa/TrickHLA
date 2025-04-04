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
@trick_link_dependency{Int64BaseTime.cpp}
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
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
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
   LogicalTimeInterval const &value )
{
   set( value );
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   HLAinteger64Interval const &value )
   : hla_interval( value )
{
   return;
}

/*!
 * @job_class{initialization}
 */
Int64Interval::Int64Interval(
   Int64Interval const &value )
   : hla_interval( value.get_base_time() )
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
   return ( (int64_t)( hla_interval.getInterval() / Int64BaseTime::get_base_time_multiplier() ) );
}

int64_t Int64Interval::get_fractional_seconds() const
{
   return ( (int64_t)( hla_interval.getInterval() % Int64BaseTime::get_base_time_multiplier() ) );
}

int64_t Int64Interval::get_base_time() const
{
   return ( hla_interval.getInterval() );
}

double Int64Interval::get_time_in_seconds() const
{
   double const seconds    = (double)get_seconds();
   double const fractional = (double)get_fractional_seconds() / (double)Int64BaseTime::get_base_time_multiplier();
   return ( seconds + fractional );
}

bool Int64Interval::is_zero() const
{
   return hla_interval.isZero();
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
   this->hla_interval = Int64BaseTime::to_base_time( value );
}

void Int64Interval::set(
   LogicalTimeInterval const &value )
{
   HLAinteger64Interval const &t = dynamic_cast< HLAinteger64Interval const & >( value );

   this->hla_interval = t.getInterval();
}
