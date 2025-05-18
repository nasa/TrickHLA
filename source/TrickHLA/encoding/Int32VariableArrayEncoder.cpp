/*!
@file TrickHLA/Int32VariableArrayEncoder.cpp
@ingroup TrickHLA
@brief This class represents the base encoder implementation.

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
@trick_link_dependency{Int32VariableArrayEncoder.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/trick_byteswap.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/Int32VariableArrayEncoder.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/HLAvariableArray.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details The endianess of the computer is determined as part of the
 * Int32VariableArrayEncoder construction process.
 * @job_class{initialization}
 */
Int32VariableArrayEncoder::Int32VariableArrayEncoder(
   string const      &trick_variable_name,
   EncodingEnum const hla_encoding,
   REF2              *r2 )
   : EncoderBase( trick_variable_name,
                  hla_encoding,
                  r2 )
{
   Int32VariableArrayEncoder::initialize();
}

/*!
 * @details The buffer and ref2 values are freed and nulled.
 * @job_class{shutdown}
 */
Int32VariableArrayEncoder::~Int32VariableArrayEncoder()
{
   if ( encoder != NULL ) {
      delete encoder;
      encoder = NULL;
   }
}

void Int32VariableArrayEncoder::initialize()
{
   if ( ref2 == NULL ) {
      EncoderBase::initialize();
   }

   if ( ( rti_encoding != ENCODING_LITTLE_ENDIAN )
        && ( rti_encoding != ENCODING_BIG_ENDIAN ) ) {
      ostringstream errmsg;
      errmsg << "Int32VariableArrayEncoder::initialize():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << trick_name
             << "' the HLA encoding specified (" << rti_encoding
             << ") must be either ENCODING_LITTLE_ENDIAN or ENCODING_BIG_ENDIAN!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   bool const valid_type = ( ( ref2->attr->type == TRICK_INTEGER ) && ( sizeof( int ) == sizeof( Integer32 ) ) )
                           || ( ( ref2->attr->type == TRICK_LONG ) && ( sizeof( long ) == sizeof( Integer32 ) ) );
   if ( !valid_type ) {
      ostringstream errmsg;
      errmsg << "Int32VariableArrayEncoder::initialize():" << __LINE__
             << " ERROR: Trick type for the '" << trick_name
             << "' simulation variable (type:"
             << Utilities::get_trick_type_string( ref2->attr->type )
             << ") is not the expected type '"
             << Utilities::get_trick_type_string( TRICK_INTEGER ) << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // This encoder is only for a dynamic variable array.
   if ( !is_dynamic_array ) {
      ostringstream errmsg;
      errmsg << "Int32FixedArrayEncoder::initialize():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << trick_name
             << "' the variable must be a dynamic variable array!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   switch ( rti_encoding ) {
      case ENCODING_LITTLE_ENDIAN: {
         data_elements.reserve( ref2_element_count );
         this->encoder = new HLAvariableArray( HLAinteger32LE() );
         break;
      }
      case ENCODING_BIG_ENDIAN: {
         data_elements.reserve( ref2_element_count );
         this->encoder = new HLAvariableArray( HLAinteger32BE() );
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "Int32Encoder::initialize():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' and HLA encoding specified (" << rti_encoding
                << ") must be either ENCODING_LITTLE_ENDIAN or ENCODING_BIG_ENDIAN!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
}

VariableLengthData &Int32VariableArrayEncoder::encode()
{
   // Since the Trick variable is dynamic (i.e. a pointer) its size can
   // change at any point so we need to refresh ref2.
   update_ref2();

   Integer32   *array_data    = *static_cast< Integer32 ** >( ref2->address );
   size_t const element_count = data_elements.size();

   if ( element_count > ref2_element_count ) {
      // Encoder has more elements than the Trick variable array size so
      // we need to create a new encoder because we can't resize it smaller.
      HLAvariableArray *array_encoder;

      // Resize to truncate excess elements.
      data_elements.resize( ref2_element_count );

      switch ( rti_encoding ) {
         case ENCODING_LITTLE_ENDIAN: {
            array_encoder = new HLAvariableArray( HLAinteger32LE() );
            for ( size_t i = 0; i < ref2_element_count; ++i ) {
               HLAinteger32LE *element = static_cast< HLAinteger32LE * >( data_elements[i] );
               element->set( array_data[i] );
               array_encoder->addElementPointer( element );
            }
            break;
         }
         case ENCODING_BIG_ENDIAN:
         default: {
            array_encoder = new HLAvariableArray( HLAinteger32BE() );
            for ( size_t i = 0; i < ref2_element_count; ++i ) {
               HLAinteger32BE *element = static_cast< HLAinteger32BE * >( data_elements[i] );
               element->set( array_data[i] );
               array_encoder->addElementPointer( element );
            }
            break;
         }
      }

      if ( encoder != NULL ) {
         delete encoder;
      }
      this->encoder = array_encoder;

   } else if ( element_count < ref2_element_count ) {
      // Add elements to the encoder.
      HLAvariableArray *array_encoder = static_cast< HLAvariableArray * >( this->encoder );

      // Ensure capacity for the data elements.
      data_elements.reserve( ref2_element_count );

      switch ( rti_encoding ) {
         case ENCODING_LITTLE_ENDIAN: {
            for ( size_t i = 0; i < element_count; ++i ) {
               HLAinteger32LE *element = static_cast< HLAinteger32LE * >( data_elements[i] );
               element->set( array_data[i] );
            }
            for ( size_t i = element_count; i < ref2_element_count; ++i ) {
               HLAinteger32LE *element = new HLAinteger32LE( array_data[i] );
               data_elements.push_back( element );
               array_encoder->addElementPointer( element );
            }
            break;
         }
         case ENCODING_BIG_ENDIAN:
         default: {
            for ( size_t i = 0; i < element_count; ++i ) {
               HLAinteger32BE *element = static_cast< HLAinteger32BE * >( data_elements[i] );
               element->set( array_data[i] );
            }
            for ( size_t i = element_count; i < ref2_element_count; ++i ) {
               HLAinteger32BE *element = new HLAinteger32BE( array_data[i] );
               data_elements.push_back( element );
               array_encoder->addElementPointer( element );
            }
            break;
         }
      }
   } else {
      // Equal number of elements and Trick variable array size.

      // Update the data elements with the current Trick variable data values.
      switch ( rti_encoding ) {
         case ENCODING_LITTLE_ENDIAN: {
            for ( size_t i = 0; i < ref2_element_count; ++i ) {
               HLAinteger32LE *element = static_cast< HLAinteger32LE * >( data_elements[i] );
               element->set( array_data[i] );
            }
            break;
         }
         case ENCODING_BIG_ENDIAN:
         default: {
            for ( size_t i = 0; i < ref2_element_count; ++i ) {
               HLAinteger32BE *element = static_cast< HLAinteger32BE * >( data_elements[i] );
               element->set( array_data[i] );
            }
            break;
         }
      }
   }

   return EncoderBase::encode();
}

void Int32VariableArrayEncoder::decode(
   VariableLengthData const &encoded_data )
{
   EncoderBase::decode( encoded_data );

   HLAvariableArray *array_encoder = static_cast< HLAvariableArray * >( this->encoder );
   size_t const      encoder_size  = array_encoder->size();

   update_ref2();

   if ( encoder_size != ref2_element_count ) {
      // Reallocate the Trick variable array to match the number of
      // decoded elements.
   } else {
      // Equal
   }


   Integer32   *array_data    = *static_cast< Integer32 ** >( ref2->address );
   size_t const element_count = data_elements.size();

}
