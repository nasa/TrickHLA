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
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

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
#include "RTI/encoding/HLAvariableArray.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

CharASCIIStringEncoder::CharASCIIStringEncoder(
   string const &trick_variable_name,
   REF2         *r2 )
   : EncoderBase( trick_variable_name,
                  r2 )
{
   if ( ref2 == NULL ) {
      update_ref2();
   }

   if ( ref2->attr->type != TRICK_CHARACTER ) {
      ostringstream errmsg;
      errmsg << "CharASCIIStringEncoder::CharASCIIStringEncoder():" << __LINE__
             << " ERROR: Trick type for the '" << trick_name
             << "' simulation variable (type:"
             << Utilities::get_trick_type_string( ref2->attr->type )
             << ") is not the expected type '"
             << Utilities::get_trick_type_string( TRICK_CHARACTER ) << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( !is_dynamic_array() ) {
      ostringstream errmsg;
      errmsg << "CharASCIIStringEncoder::CharASCIIStringEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << trick_name
             << "' the variable must be a dynamic variable array!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   HLAASCIIstring    data_prototype;
   HLAvariableArray *array_encoder = new HLAvariableArray( data_prototype );
   array_encoder->addElement( data_prototype );

   this->encoder = array_encoder;
}

CharASCIIStringEncoder::~CharASCIIStringEncoder()
{
   if ( encoder != NULL ) {
      delete encoder;
      encoder = NULL;
   }
}

VariableLengthData &CharASCIIStringEncoder::encode()
{
   /* Since the Trick variable is dynamic (i.e. a pointer) its */
   /* size can change at any point so we need to refresh ref2. */
   update_ref2();

   /* Convert the char * string into a wide string. */
   string data = *static_cast< char ** >( ref2->address );

   HLAvariableArray const *array_encoder = dynamic_cast< HLAvariableArray * >( encoder );

   const_cast< HLAASCIIstring & >(
      dynamic_cast< HLAASCIIstring const & >(
         array_encoder->get( 0 ) ) )
      .set( data );

   return EncoderBase::encode();
}

void CharASCIIStringEncoder::decode(
   VariableLengthData const &encoded_data )
{
   EncoderBase::decode( encoded_data );

   HLAvariableArray const *array_encoder = dynamic_cast< HLAvariableArray * >( encoder );

   /* Trick variable is dynamic (i.e. a pointer) so we need to refresh ref2. */
   update_ref2();

   /* Convert from the wide-string to a char * string. */
   *static_cast< char ** >( ref2->address ) = StringUtilities::ip_strdup_string(
      dynamic_cast< HLAASCIIstring const & >( array_encoder->get( 0 ) ).get() );
}

string CharASCIIStringEncoder::to_string()
{
   ostringstream msg;
   msg << "CharASCIIStringEncoder[trick_name:" << trick_name << "]";
   return msg.str();
}
