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
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

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
   string const      &trick_variable_name,
   EncodingEnum const hla_encoding,
   REF2              *r2 )
   : EncoderBase( trick_variable_name,
                  hla_encoding,
                  r2 )
{
   if ( ref2 == NULL ) {
      update_ref2();
   }

   if ( ref2->attr->type != TRICK_CHARACTER ) {
      ostringstream errmsg;
      errmsg << "CharOpaqueDataEncoder::CharOpaqueDataEncoder():" << __LINE__
             << " ERROR: Trick type for the '" << trick_name
             << "' simulation variable (type:"
             << Utilities::get_trick_type_string( ref2->attr->type )
             << ") is not the expected type '"
             << Utilities::get_trick_type_string( TRICK_CHARACTER ) << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( !is_dynamic_array ) {
      ostringstream errmsg;
      errmsg << "CharOpaqueDataEncoder::CharOpaqueDataEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << trick_name
             << "' the variable must be a dynamic variable array!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   HLAopaqueData *opaque_encoder = new HLAopaqueData();

   this->encoder = opaque_encoder;
}

CharOpaqueDataEncoder::~CharOpaqueDataEncoder()
{
   if ( encoder != NULL ) {
      delete encoder;
      encoder = NULL;
   }
}

VariableLengthData &CharOpaqueDataEncoder::encode()
{
   /* Since the Trick variable is dynamic (i.e. a pointer) its */
   /* size can change at any point so we need to refresh ref2. */
   update_ref2();
   Octet *data = *static_cast< Octet ** >( ref2->address );

   HLAopaqueData *opaque_encoder = dynamic_cast< HLAopaqueData * >( encoder );

   opaque_encoder->set( const_cast< Octet const * >( data ), get_size( data ) );

   return EncoderBase::encode();
}

void CharOpaqueDataEncoder::decode(
   VariableLengthData const &encoded_data )
{
   EncoderBase::decode( encoded_data );

   HLAopaqueData const *opaque_encoder = dynamic_cast< HLAopaqueData * >( encoder );

   /* Trick variable is dynamic (i.e. a pointer) so we need to refresh ref2. */
   update_ref2();
   resize_trick_var( opaque_encoder->dataLength() );

   memcpy( *static_cast< Octet ** >( ref2->address ),
           opaque_encoder->get(),
           opaque_encoder->dataLength() );
}

string CharOpaqueDataEncoder::to_string()
{
   ostringstream msg;
   msg << "CharOpaqueDataEncoder[trick_name:'" << trick_name
       << "' rti_encoding:" << rti_encoding << "]";
   return msg.str();
}

void CharOpaqueDataEncoder::resize_trick_var(
   size_t const new_size )
{
   /* Trick array variable size does not match the new size. */
   if ( ref2_element_count != new_size ) {
      if ( *( static_cast< void ** >( ref2->address ) ) == NULL ) {
         *( static_cast< void ** >( ref2->address ) ) =
            static_cast< void * >( TMM_declare_var_1d( "char", new_size ) );
      } else {
         *( static_cast< void ** >( ref2->address ) ) =
            static_cast< void * >( TMM_resize_array_1d_a(
               *( static_cast< void ** >( ref2->address ) ), new_size ) );
      }

      /* Update the element count to the new size. */
      ref2_element_count = new_size;

      if ( *static_cast< void ** >( ref2->address ) == NULL ) {
         ostringstream errmsg;
         errmsg << "CharOpaqueDataEncoder::resize_trick_var():" << __LINE__
                << " ERROR: Could not allocate memory for Trick variable"
                << " with name '" << trick_name << "' with " << new_size
                << " elements!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}
