/*!
@file TrickHLA/encoding/CharRawDataEncoder.cpp
@ingroup TrickHLA
@brief This class represents the char array raw data/none encoder implementation.

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
@trick_link_dependency{CharRawDataEncoder.cpp}
@trick_link_dependency{VariableArrayEncoderBase.cpp}

@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// System include files.
#include <cstddef>
#include <cstring>
#include <ostream>
#include <sstream>
#include <vector>

// Trick include files.
#include "trick/attributes.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/encoding/CharRawDataEncoder.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"

// HLA include files.
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/EncodingConfig.h"

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

CharRawDataEncoder::CharRawDataEncoder(
   void       *addr,
   ATTRIBUTES *attr )
   : VariableArrayEncoderBase( addr, attr )
{
   if ( ( this->type != TRICK_CHARACTER )
        && ( this->type != TRICK_UNSIGNED_CHARACTER ) ) {
      ostringstream errmsg;
      errmsg << "CharRawDataEncoder::CharRawDataEncoder():" << __LINE__
             << " ERROR: Trick type for the '"
             << ( ( ( attr != NULL ) && ( attr->name != NULL ) ) ? attr->name : "" )
             << "' simulation variable (type:"
             << trickTypeCharString( this->type, "UNSUPPORTED_TYPE" )
             << ") is not the expected type '"
             << trickTypeCharString( TRICK_CHARACTER, "UNSUPPORTED_TYPE" )
             << "'." << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( !is_dynamic_array() ) {
      ostringstream errmsg;
      errmsg << "CharRawDataEncoder::CharRawDataEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '"
             << ( ( ( attr != NULL ) && ( attr->name != NULL ) ) ? attr->name : "" )
             << "' the variable must be a dynamic variable array!" << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

#if defined( IEEE_1516_2025 )
   this->data_encoder = new HLAASCIIchar(); // TEMP
#endif
}

CharRawDataEncoder::~CharRawDataEncoder()
{
   return;
}

void CharRawDataEncoder::update_before_encode()
{
   // Since the Trick variable is dynamic (i.e. a pointer) its size
   // can change at any point so we need to refresh the counts.
   calculate_var_element_count();

   this->data.setDataPointer( *static_cast< void ** >( address ), var_element_count );
}

void CharRawDataEncoder::update_after_decode()
{
   return;
}

#if defined( IEEE_1516_2025 )
DataElement &CharRawDataEncoder::decode( VariableLengthData const &inData )
{
   // Resize Trick array variable to match the decoded data size.
   resize_trick_var( inData.size() );
   memcpy( *static_cast< void ** >( address ), inData.data(), inData.size() ); // flawfinder: ignore

   // TODO: Return a valid data element, which may not be possible for raw data.
   return *data_encoder; // TEMP
}
#else
void CharRawDataEncoder::decode( VariableLengthData const &inData ) throw( EncoderException )
{
   // Resize Trick array variable to match the decoded data size.
   resize_trick_var( inData.size() );
   memcpy( *static_cast< void ** >( address ), inData.data(), inData.size() ); // flawfinder: ignore
}
#endif // IEEE_1516_2025

size_t CharRawDataEncoder::decodeFrom(
   vector< Octet > const &buffer,
   size_t                 index )
#if defined( IEEE_1516_2010 )
   throw( EncoderException )
#endif
{
   ostringstream errmsg;
   errmsg << "CharRawDataEncoder::decodeFrom():" << __LINE__
          << " ERROR: Encoding not supported for a field of an HLA Record!"
          << std::endl;
   DebugHandler::terminate_with_message( errmsg.str() );
   return index;
}

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif
