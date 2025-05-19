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
      update_ref2();
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
         Integer32        *array_data    = static_cast< Integer32 * >( ref2->address );
         HLAvariableArray *array_encoder = new HLAvariableArray( HLAinteger32LE() );

         data_elements.reserve( ref2_element_count );

         // Connect the users array data to the encoder array elements.
         for ( size_t i = 0; i < ref2_element_count; ++i ) {
            HLAinteger32LE *element = new HLAinteger32LE( &array_data[i] );
            data_elements.push_back( element );
            array_encoder->setElementPointer( i, element );
         }

         this->encoder = array_encoder;
         break;
      }
      case ENCODING_BIG_ENDIAN: {
         Integer32        *array_data    = static_cast< Integer32 * >( ref2->address );
         HLAvariableArray *array_encoder = new HLAvariableArray( HLAinteger32BE() );

         data_elements.reserve( ref2_element_count );

         // Connect the users array data to the encoder array elements.
         for ( size_t i = 0; i < ref2_element_count; ++i ) {
            HLAinteger32BE *element = new HLAinteger32BE( &array_data[i] );
            data_elements.push_back( element );
            array_encoder->setElementPointer( i, element );
         }

         this->encoder = array_encoder;
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

bool Int32VariableArrayEncoder::resize(
   size_t const new_size )
{
   // Determine if we need to resize the Trick array variable and data elements.
   if ( ( new_size == ref2_element_count ) && ( new_size == data_elements.size() ) ) {
      return false;
   }

   // Trick variable array size does not match the new size.
   if ( ref2_element_count != new_size ) {

      // Reallocate the Trick variable array to the new size.
      ref2_element_count  = new_size;
      int const num_bytes = ref2_element_count * ref2->attr->size;

      // Resize the Trick array variable to match the incoming data size.
      *( static_cast< char ** >( ref2->address ) ) =
         static_cast< char * >( TMM_resize_array_1d_a(
            *( static_cast< char ** >( ref2->address ) ), num_bytes ) );

      if ( *static_cast< Integer32 ** >( ref2->address ) == NULL ) {
         ostringstream errmsg;
         errmsg << "Int32VariableArrayEncoder::resize():" << __LINE__
                << " ERROR: Could not allocate memory for Trick variable with name '"
                << trick_name << "' with number of bytes " << num_bytes << "!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // Remove the extra elements if the new_size reduces the element count.
   size_t const orig_element_count = data_elements.size();
   if ( orig_element_count > new_size ) {
      for ( size_t i = new_size; i < orig_element_count; ++i ) {
         delete data_elements[i];
      }
      data_elements.resize( new_size );

   } else if ( orig_element_count < new_size ) {
      // Reserve enough capacity for the new elements for the larger size.
      data_elements.reserve( new_size );
   }

   HLAvariableArray *array_encoder = static_cast< HLAvariableArray * >( encoder );
   Integer32        *array_data    = *static_cast< Integer32 ** >( ref2->address );

   switch ( rti_encoding ) {
      case ENCODING_LITTLE_ENDIAN: {
         // Because we can't resize the encoder to a smaller size we have to
         // create a new one.
         if ( new_size < array_encoder->size() ) {
            if ( encoder != NULL ) {
               delete encoder;
            }
            array_encoder = new HLAvariableArray( HLAinteger32LE() );
            this->encoder = array_encoder;
         }

         // Connect the array data to the encoder array elements.
         HLAinteger32LE *element;
         for ( size_t i = 0; i < new_size; ++i ) {
            if ( i < orig_element_count ) {
               element = static_cast< HLAinteger32LE * >( data_elements[i] );
               element->setDataPointer( &array_data[i] );
            } else {
               element = new HLAinteger32LE( &array_data[i] );
               data_elements.push_back( element );
            }

            if ( i < array_encoder->size() ) {
               array_encoder->setElementPointer( i, element );
            } else {
               array_encoder->addElementPointer( element );
            }
         }
         break;
      }
      case ENCODING_BIG_ENDIAN:
      default: {
         // Because we can't resize the encoder to a smaller size we have to
         // create a new one.
         if ( new_size < array_encoder->size() ) {
            if ( encoder != NULL ) {
               delete encoder;
            }
            array_encoder = new HLAvariableArray( HLAinteger32BE() );
            this->encoder = array_encoder;
         }

         // Connect the array data to the encoder array elements.
         HLAinteger32BE *element;
         for ( size_t i = 0; i < new_size; ++i ) {
            if ( i < orig_element_count ) {
               element = static_cast< HLAinteger32BE * >( data_elements[i] );
               element->setDataPointer( &array_data[i] );
            } else {
               element = new HLAinteger32BE( &array_data[i] );
               data_elements.push_back( element );
            }

            if ( i < array_encoder->size() ) {
               array_encoder->setElementPointer( i, element );
            } else {
               array_encoder->addElementPointer( element );
            }
         }
         break;
      }
   }

   return true;
}

void Int32VariableArrayEncoder::refresh_data_elements()
{
   HLAvariableArray *array_encoder = static_cast< HLAvariableArray * >( encoder );

   if ( ( data_elements.size() != ref2_element_count )
        || ( data_elements.size() != array_encoder->size() ) ) {
      ostringstream errmsg;
      errmsg << "Int32VariableArrayEncoder::refresh_data_elements():" << __LINE__
             << " ERROR: For Trick variable with name '" << trick_name
             << "' the number of elements don't agree with the encoder!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   Integer32 *array_data = *static_cast< Integer32 ** >( ref2->address );

   switch ( rti_encoding ) {
      case ENCODING_LITTLE_ENDIAN: {
         for ( size_t i = 0; i < data_elements.size(); ++i ) {
            HLAinteger32LE *element = static_cast< HLAinteger32LE * >( data_elements[i] );
            element->setDataPointer( &array_data[i] );
            if ( static_cast< void * >( element ) != static_cast< void * >( &array_encoder[i] ) ) {
               array_encoder->setElementPointer( i, element );
            }
         }
         break;
      }
      case ENCODING_BIG_ENDIAN:
      default: {
         for ( size_t i = 0; i < data_elements.size(); ++i ) {
            HLAinteger32BE *element = static_cast< HLAinteger32BE * >( data_elements[i] );
            element->setDataPointer( &array_data[i] );
            if ( static_cast< void * >( element ) != static_cast< void * >( &array_encoder[i] ) ) {
               array_encoder->setElementPointer( i, element );
            }
         }
         break;
      }
   }
}

VariableLengthData &Int32VariableArrayEncoder::encode()
{
   // Since the Trick variable is dynamic (i.e. a pointer) its size can
   // change at any point so we need to refresh ref2.
   update_ref2();

   // Resize data elements and the array if needed which will also update
   // the data elements. Otherwise, update the data elements before we encode.
   if ( !resize( ref2_element_count ) ) {
      refresh_data_elements();
   }

   return EncoderBase::encode();
}

void Int32VariableArrayEncoder::decode(
   VariableLengthData const &encoded_data )
{
   update_ref2();

   // Resize data elements and the array if needed which will also update
   // the data elements. Otherwise, update the data elements before we decode.
   if ( !resize( ref2_element_count ) ) {
      refresh_data_elements();
   }

   EncoderBase::decode( encoded_data );

   HLAvariableArray const *array_encoder = static_cast< HLAvariableArray * >( encoder );

   // If the size of the decoded data does not match the simulation array
   // variable size, then resize and try decoding again.
   if ( ref2_element_count != array_encoder->size() ) {

      // Resize data elements and the array if needed which will also update
      // the data elements. Otherwise, update the data elements before we decode.
      if ( !resize( array_encoder->size() ) ) {
         refresh_data_elements();
      }

      // Decode again now that we have the proper elements connected to
      // the Trick array data elements.
      EncoderBase::decode( encoded_data );
   }
}
