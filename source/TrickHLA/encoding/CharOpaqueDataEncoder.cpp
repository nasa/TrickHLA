/*!
@file TrickHLA/encoding/CharOpaqueDataEncoder.cpp
@ingroup TrickHLA
@brief This class represents the char array Opaque data encoder implementation.

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
#include "TrickHLA/encoding/CharOpaqueDataEncoder.hh"
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
#include "RTI/encoding/HLAopaqueData.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

CharOpaqueDataEncoder::CharOpaqueDataEncoder(
   void       *addr,
   ATTRIBUTES *attr )
   : EncoderBase( addr, attr )
{
   if ( ( this->type != TRICK_CHARACTER )
        && ( this->type != TRICK_UNSIGNED_CHARACTER ) ) {
      ostringstream errmsg;
      errmsg << "CharOpaqueDataEncoder::CharOpaqueDataEncoder():" << __LINE__
             << " ERROR: Trick type for the '" << this->name
             << "' simulation variable (type:"
             << Utilities::get_trick_type_string( this->type )
             << ") is not the expected type '"
             << Utilities::get_trick_type_string( TRICK_CHARACTER )
             << "'." << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( !is_dynamic_array() ) {
      ostringstream errmsg;
      errmsg << "CharOpaqueDataEncoder::CharOpaqueDataEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << this->name
             << "' the variable must be a dynamic variable array!" << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   this->encoder = new HLAopaqueData();
}

CharOpaqueDataEncoder::~CharOpaqueDataEncoder()
{
   return;
}

VariableLengthData &CharOpaqueDataEncoder::encode()
{
   Octet *byte_data = *static_cast< Octet ** >( address );

   HLAopaqueData *opaque_encoder = dynamic_cast< HLAopaqueData * >( encoder );

   opaque_encoder->set( const_cast< Octet const * >( byte_data ), get_size( byte_data ) );

   return EncoderBase::encode();
}

bool const CharOpaqueDataEncoder::decode(
   VariableLengthData const &encoded_data )
{
   if ( EncoderBase::decode( encoded_data ) ) {

      HLAopaqueData const *opaque_encoder = dynamic_cast< HLAopaqueData * >( encoder );

      resize_trick_var( opaque_encoder->dataLength() );

      Octet *byte_data = *static_cast< Octet ** >( address );
      if ( byte_data != NULL ) {
         memcpy( byte_data, opaque_encoder->get(), opaque_encoder->dataLength() );
      }

      return true;
   }
   return false;
}

string CharOpaqueDataEncoder::to_string()
{
   return ( "CharOpaqueDataEncoder[" + this->name + "]" );
}
