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
@trick_link_dependency{../Utilities.cpp}


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
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
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
   if ( ( attr->type != TRICK_CHARACTER )
        && ( attr->type != TRICK_UNSIGNED_CHARACTER ) ) {
      ostringstream errmsg;
      errmsg << "CharRawDataEncoder::CharRawDataEncoder():" << __LINE__
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
      errmsg << "CharRawDataEncoder::CharRawDataEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << this->name
             << "' the variable must be a dynamic variable array!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   HLAbyte data_prototype;
   this->encoder = new HLAfixedArray( data_prototype, var_element_count );
}

CharRawDataEncoder::~CharRawDataEncoder()
{
   return;
}

VariableLengthData &CharRawDataEncoder::encode()
{
   /* Since the Trick variable is dynamic (i.e. a pointer) its size */
   /* can change at any point so we need to refresh the counts.     */
   calculate_var_element_count();

   /* Ensure the number of data elements matches the Trick variable */
   resize_data_elements( var_element_count );

   HLAfixedArray const *array_encoder = dynamic_cast< HLAfixedArray * >( encoder );
   Octet               *array_data    = *static_cast< Octet ** >( address );

   /* Copy the Trick array values to the data elements to be encoded. */
   for ( size_t i = 0; i < var_element_count; ++i ) {
      const_cast< HLAbyte & >(
         dynamic_cast< HLAbyte const & >(
            array_encoder->get( i ) ) )
         .set( array_data[i] );
   }

   return EncoderBase::encode();
}

bool const CharRawDataEncoder::decode(
   VariableLengthData const &encoded_data )
{
   // Resize the data element array to match the incoming data size.
   resize_data_elements( encoded_data.size() );

   if ( EncoderBase::decode( encoded_data ) ) {

      HLAfixedArray const *array_encoder = dynamic_cast< HLAfixedArray * >( encoder );

      /* Resize Trick array variable to match the decoded data size. */
      resize_trick_var( array_encoder->size() );

      Octet *array_data = *static_cast< Octet ** >( address );

      /* Copy the decoded data element values to the Trick array. */
      for ( size_t i = 0; i < var_element_count; ++i ) {
         array_data[i] = dynamic_cast< HLAbyte const & >( array_encoder->get( i ) ).get();
      }
      return true;
   }
   return false;
}

string CharRawDataEncoder::to_string()
{
   return ( "CharRawDataEncoder[" + this->name + "]" );
}

void CharRawDataEncoder::resize_trick_var(
   size_t const new_size )
{
   /* Trick array variable size does not match the new size. */
   if ( ( new_size != var_element_count )
        || ( *( static_cast< void ** >( address ) ) == NULL ) ) {

      if ( *( static_cast< void ** >( address ) ) == NULL ) {
         *( static_cast< void ** >( address ) ) =
            static_cast< void * >( TMM_declare_var_1d( "char", new_size ) );
      } else {
         *( static_cast< void ** >( address ) ) =
            static_cast< void * >( TMM_resize_array_1d_a(
               *( static_cast< void ** >( address ) ), new_size ) );
      }

      /* Update the element count to the new size. */
      var_element_count = new_size;

      if ( *static_cast< void ** >( address ) == NULL ) {
         ostringstream errmsg;
         errmsg << "CharRawDataEncoder::resize_trick_var():" << __LINE__
                << " ERROR: Could not allocate memory for Trick variable"
                << " with name '" << this->name << "' with " << new_size
                << " elements!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}

void CharRawDataEncoder::resize_data_elements(
   size_t const new_size )
{
   HLAfixedArray const *array_encoder = dynamic_cast< HLAfixedArray * >( encoder );

   if ( new_size != array_encoder->size() ) {
      HLAbyte data_prototype;
      delete this->encoder;
      this->encoder = new HLAfixedArray( data_prototype, new_size );
   }
}
