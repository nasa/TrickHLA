/*!
@file TrickHLA/encoding/StringUnicodeStringEncoder.cpp
@ingroup TrickHLA
@brief This class represents the std::string Unicode string encoder implementation.

\par<b>Assumptions and Limitations:</b>
- Only primitive types and static arrays of primitive type are supported for now.

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
@trick_link_dependency{StringUnicodeStringEncoder.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{VariableArrayEncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstdio>
#include <ostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/attributes.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/encoding/StringUnicodeStringEncoder.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include "RTI/encoding/BasicDataElements.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

StringUnicodeStringEncoder::StringUnicodeStringEncoder(
   void       *addr,
   ATTRIBUTES *attr )
   : VariableArrayEncoderBase( addr, attr ),
     wstring_data()
{
   if ( this->type != TRICK_STRING ) {
      ostringstream errmsg;
      errmsg << "StringUnicodeStringEncoder::StringUnicodeStringEncoder():" << __LINE__
             << " ERROR: Trick type for the '"
             << ( ( ( attr != NULL ) && ( attr->name != NULL ) ) ? attr->name : "" )
             << "' simulation variable (type:"
             << trickTypeCharString( this->type, "UNSUPPORTED_TYPE" )
             << ") is not the expected type '"
             << trickTypeCharString( TRICK_STRING, "UNSUPPORTED_TYPE" )
             << "'." << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( is_array() ) {
      ostringstream errmsg;
      errmsg << "StringUnicodeStringEncoder::StringUnicodeStringEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '"
             << ( ( ( attr != NULL ) && ( attr->name != NULL ) ) ? attr->name : "" )
             << "' the variable must be a std::string and not an array!" << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   this->data_encoder = new HLAunicodeString( &wstring_data );
}

StringUnicodeStringEncoder::~StringUnicodeStringEncoder()
{
   return;
}

void StringUnicodeStringEncoder::update_before_encode()
{
   // Convert the string into a wide-string.
   string &str_data = *static_cast< std::string * >( address );
   this->wstring_data.assign( str_data.begin(), str_data.end() );
}

void StringUnicodeStringEncoder::update_after_decode()
{
   // Convert from wide-string to string.
   ( *static_cast< std::string * >( address ) ).assign( wstring_data.begin(), wstring_data.end() );
}
