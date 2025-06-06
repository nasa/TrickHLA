/*!
@file TrickHLA/encoding/CharRawDataEncoder.cpp
@ingroup TrickHLA
@brief This class represents the char array raw data/none encoder implementation.

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


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
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
#include "TrickHLA/Types.hh"
#include "TrickHLA/encoding/CharRawDataEncoder.hh"
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
#include "RTI/encoding/HLAfixedArray.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

CharRawDataEncoder::CharRawDataEncoder(
   void       *addr,
   ATTRIBUTES *attr )
   : EncoderBase( addr, attr )
{
   if ( ( this->type != TRICK_CHARACTER )
        && ( this->type != TRICK_UNSIGNED_CHARACTER ) ) {
      ostringstream errmsg;
      errmsg << "CharRawDataEncoder::CharRawDataEncoder():" << __LINE__
             << " ERROR: Trick type for the '" << this->name
             << "' simulation variable (type:"
             << trickTypeCharString( this->type, "UNKNOWN_TYPE" )
             << ") is not the expected type '"
             << trickTypeCharString( TRICK_CHARACTER, "TRICK_CHARACTER" )
             << "'." << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( !is_dynamic_array() ) {
      ostringstream errmsg;
      errmsg << "CharRawDataEncoder::CharRawDataEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << this->name
             << "' the variable must be a dynamic variable array!" << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }
}

CharRawDataEncoder::~CharRawDataEncoder()
{
   return;
}

VariableLengthData &CharRawDataEncoder::encode()
{
   // Since the Trick variable is dynamic (i.e. a pointer) its size
   // can change at any point so we need to refresh the counts.
   calculate_var_element_count();

   this->data.setDataPointer( *static_cast< void ** >( address ), var_element_count );
   return this->data;
}

bool const CharRawDataEncoder::decode(
   VariableLengthData const &encoded_data )
{
   resize_trick_var( encoded_data.size() );
   memcpy( *static_cast< void ** >( address ), encoded_data.data(), encoded_data.size() );
   return true;
}

string CharRawDataEncoder::to_string()
{
   return ( "CharRawDataEncoder[" + this->name + "]" );
}
