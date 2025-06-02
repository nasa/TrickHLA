/*!
@file TrickHLA/encoding/CharASCIIStringEncoder.cpp
@ingroup TrickHLA
@brief This class represents the char array ASCII string encoder implementation.

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
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}
@trick_link_dependency{../Utilities.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstddef>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/attributes.h"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/CharASCIIStringEncoder.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

CharASCIIStringEncoder::CharASCIIStringEncoder(
   void       *addr,
   ATTRIBUTES *attr )
   : EncoderBase( addr, attr ),
     string_data()
{
   if ( ( attr->type != TRICK_CHARACTER )
        && ( attr->type != TRICK_UNSIGNED_CHARACTER ) ) {
      ostringstream errmsg;
      errmsg << "CharASCIIStringEncoder::CharASCIIStringEncoder():" << __LINE__
             << " ERROR: Trick type for the '" << this->name
             << "' simulation variable (type:"
             << Utilities::get_trick_type_string( attr->type )
             << ") is not the expected type '"
             << Utilities::get_trick_type_string( TRICK_CHARACTER ) << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( !is_dynamic_array() ) {
      ostringstream errmsg;
      errmsg << "CharASCIIStringEncoder::CharASCIIStringEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << this->name
             << "' the variable must be a dynamic variable array!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Automatically encode and decode into the string_data.
   this->encoder = new HLAASCIIstring( &string_data );
}

CharASCIIStringEncoder::~CharASCIIStringEncoder()
{
   return;
}

VariableLengthData &CharASCIIStringEncoder::encode()
{
   /* Convert the char * string into a std::string. */
   string_data = *static_cast< char ** >( address );

   return EncoderBase::encode();
}

bool const CharASCIIStringEncoder::decode(
   VariableLengthData const &encoded_data )
{
   if ( EncoderBase::decode( encoded_data ) ) {

      /* Convert from the std::string to a char * string. */
      if ( string_data.size() > 0 ) {
         *static_cast< char ** >( address ) = StringUtilities::ip_strdup_string( string_data );
      } else {
         char empty = '\0';

         *static_cast< char ** >( address ) = TMM_strdup( &empty );
      }
      return true;
   }
   return false;
}

string CharASCIIStringEncoder::to_string()
{
   return ( "CharASCIIStringEncoder[" + this->name + "]" );
}
