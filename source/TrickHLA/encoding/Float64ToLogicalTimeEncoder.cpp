/*!
@file TrickHLA/encoding/Float64ToLogicalTimeEncoder.cpp
@ingroup TrickHLA
@brief This class represents the char array ASCII string encoder implementation.

@copyright Copyright 2025 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{Float64ToLogicalTimeEncoder.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{VariableArrayEncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}
@trick_link_dependency{../time/Int64BaseTime.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/attributes.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/encoding/Float64ToLogicalTimeEncoder.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"
#include "TrickHLA/time/Int64BaseTime.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/encoding/BasicDataElements.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

Float64ToLogicalTimeEncoder::Float64ToLogicalTimeEncoder(
   void         *addr,
   ATTRIBUTES   *attr,
   string const &name )
   : VariableArrayEncoderBase( addr, attr, name ),
     time_data( 0LL )
{
   if ( this->type != TRICK_DOUBLE ) {
      ostringstream errmsg;
      errmsg << "Float64ToLogicalTimeEncoder::Float64ToLogicalTimeEncoder():" << __LINE__
             << " ERROR: Trick type for the '" << data_name
             << "' simulation variable (type:"
             << trickTypeCharString( this->type, "UNSUPPORTED_TYPE" )
             << ") is not the expected type '"
             << trickTypeCharString( TRICK_DOUBLE, "UNSUPPORTED_TYPE" )
             << "'." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( is_array() ) {
      ostringstream errmsg;
      errmsg << "Float64ToLogicalTimeEncoder::Float64ToLogicalTimeEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << data_name
             << "' the variable must be a 'double' primitive type!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   this->data_encoder = new HLAinteger64BE( &time_data );
}

Float64ToLogicalTimeEncoder::~Float64ToLogicalTimeEncoder()
{
   return;
}

void Float64ToLogicalTimeEncoder::update_before_encode()
{
   // Convert double time in seconds to the base time.
   this->time_data = Int64BaseTime::to_base_time( *static_cast< double * >( address ) );
}

void Float64ToLogicalTimeEncoder::update_after_decode()
{
   // Convert 64-bit integer time to time in seconds.
   *static_cast< double * >( address ) = Int64BaseTime::to_seconds( this->time_data );
}
