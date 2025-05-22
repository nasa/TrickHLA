/*!
@file TrickHLA/BasicDataVariableArrayEncoders.cpp
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
@trick_link_dependency{BasicDataVariableArrayEncoders.cpp}
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

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/BasicDataVariableArrayEncoders.hh"
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

#define DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( EncoderClassName, EncodableDataType, SimpleDataType, TrickTypeEnum ) \
                                                                                                                         \
   EncoderClassName::EncoderClassName(                                                                                   \
      string const      &trick_variable_name,                                                                            \
      EncodingEnum const hla_encoding,                                                                                   \
      REF2              *r2 )                                                                                            \
      : EncoderBase( trick_variable_name,                                                                                \
                     hla_encoding,                                                                                       \
                     r2 )                                                                                                \
   {                                                                                                                     \
      if ( ref2 == NULL ) {                                                                                              \
         update_ref2();                                                                                                  \
      }                                                                                                                  \
                                                                                                                         \
      if ( ( rti_encoding != ENCODING_LITTLE_ENDIAN )                                                                    \
           && ( rti_encoding != ENCODING_BIG_ENDIAN ) ) {                                                                \
         ostringstream errmsg;                                                                                           \
         errmsg << #EncoderClassName << "::" #EncoderClassName << "():" << __LINE__                                      \
                << " ERROR: Trick ref-attributes for '" << trick_name                                                    \
                << "' the HLA encoding specified (" << rti_encoding                                                      \
                << ") must be either ENCODING_LITTLE_ENDIAN or"                                                          \
                << " ENCODING_BIG_ENDIAN!\n";                                                                            \
         DebugHandler::terminate_with_message( errmsg.str() );                                                           \
         return;                                                                                                         \
      }                                                                                                                  \
                                                                                                                         \
      if ( ref2->attr->type != TrickTypeEnum ) {                                                                         \
         ostringstream errmsg;                                                                                           \
         errmsg << #EncoderClassName << "::" #EncoderClassName << "():" << __LINE__                                      \
                << " ERROR: Trick type for the '" << trick_name                                                          \
                << "' simulation variable (type:"                                                                        \
                << Utilities::get_trick_type_string( ref2->attr->type )                                                  \
                << ") is not the expected type '"                                                                        \
                << Utilities::get_trick_type_string( TrickTypeEnum ) << "'.\n";                                          \
         DebugHandler::terminate_with_message( errmsg.str() );                                                           \
         return;                                                                                                         \
      }                                                                                                                  \
                                                                                                                         \
      /* This encoder is only for a dynamic variable array. */                                                           \
      if ( !is_dynamic_array ) {                                                                                         \
         ostringstream errmsg;                                                                                           \
         errmsg << #EncoderClassName << "::" #EncoderClassName << "():" << __LINE__                                      \
                << " ERROR: Trick ref-attributes for '" << trick_name                                                    \
                << "' the variable must be a dynamic variable array!\n";                                                 \
         DebugHandler::terminate_with_message( errmsg.str() );                                                           \
         return;                                                                                                         \
      }                                                                                                                  \
                                                                                                                         \
      EncodableDataType data_prototype;                                                                                  \
      this->encoder = new HLAvariableArray( data_prototype );                                                            \
                                                                                                                         \
      if ( !EncoderClassName::resize( ref2_element_count ) ) {                                                           \
         refresh_data_elements();                                                                                        \
      }                                                                                                                  \
   }                                                                                                                     \
                                                                                                                         \
   EncoderClassName::~EncoderClassName()                                                                                 \
   {                                                                                                                     \
      if ( encoder != NULL ) {                                                                                           \
         delete encoder;                                                                                                 \
         encoder = NULL;                                                                                                 \
      }                                                                                                                  \
   }                                                                                                                     \
                                                                                                                         \
   VariableLengthData &EncoderClassName::encode()                                                                        \
   {                                                                                                                     \
      /* Since the Trick variable is dynamic (i.e. a pointer) its */                                                     \
      /* size can change at any point so we need to refresh ref2. */                                                     \
      update_ref2();                                                                                                     \
                                                                                                                         \
      /* Resize data elements and the array if needed which will also  */                                                \
      /* update the data elements. Otherwise, update the data elements */                                                \
      /* before we encode.                                             */                                                \
      if ( !resize( ref2_element_count ) ) {                                                                             \
         refresh_data_elements();                                                                                        \
      }                                                                                                                  \
                                                                                                                         \
      return EncoderBase::encode();                                                                                      \
   }                                                                                                                     \
                                                                                                                         \
   void EncoderClassName::decode(                                                                                        \
      VariableLengthData const &encoded_data )                                                                           \
   {                                                                                                                     \
      update_ref2();                                                                                                     \
                                                                                                                         \
      /* Resize data elements and the array if needed which will also  */                                                \
      /* update the data elements. Otherwise, update the data elements */                                                \
      /* before we decode.                                             */                                                \
      if ( !resize( ref2_element_count ) ) {                                                                             \
         refresh_data_elements();                                                                                        \
      }                                                                                                                  \
                                                                                                                         \
      EncoderBase::decode( encoded_data );                                                                               \
                                                                                                                         \
      HLAvariableArray const *array_encoder =                                                                            \
         static_cast< HLAvariableArray * >( encoder );                                                                   \
                                                                                                                         \
      /* If the size of the decoded data does not match the simulation */                                                \
      /* array variable size, then resize and try decoding again.      */                                                \
      if ( ref2_element_count != array_encoder->size() ) {                                                               \
                                                                                                                         \
         /* Resize data elements and the array if needed which will also */                                              \
         /* update the data elements. Otherwise, refresh data elements   */                                              \
         /* before we decode.                                            */                                              \
         if ( !resize( array_encoder->size() ) ) {                                                                       \
            refresh_data_elements();                                                                                     \
         }                                                                                                               \
                                                                                                                         \
         /* Decode again now that we have the proper elements connected */                                               \
         /* to the Trick array data elements.                           */                                               \
         EncoderBase::decode( encoded_data );                                                                            \
      }                                                                                                                  \
   }                                                                                                                     \
                                                                                                                         \
   string EncoderClassName::to_string()                                                                                  \
   {                                                                                                                     \
      ostringstream msg;                                                                                                 \
      msg << #EncoderClassName                                                                                           \
          << "[trick_name:'" << trick_name                                                                               \
          << "' rti_encoding:" << rti_encoding << "]";                                                                   \
      return msg.str();                                                                                                  \
   }                                                                                                                     \
                                                                                                                         \
   bool EncoderClassName::resize(                                                                                        \
      size_t const new_size )                                                                                            \
   {                                                                                                                     \
      /* Determine if we need to resize the Trick array variable */                                                      \
      /* and data elements.                                      */                                                      \
      if ( ( new_size == ref2_element_count ) && ( new_size == data_elements.size() ) ) {                                \
         return false;                                                                                                   \
      }                                                                                                                  \
                                                                                                                         \
      /* Trick variable array size does not match the new size. */                                                       \
      if ( ref2_element_count != new_size ) {                                                                            \
                                                                                                                         \
         /* Resize the Trick array variable to match the incoming data size. */                                          \
         *( static_cast< void ** >( ref2->address ) ) =                                                                  \
            static_cast< void * >( TMM_resize_array_1d_a(                                                                \
               *( static_cast< void ** >( ref2->address ) ), new_size ) );                                               \
                                                                                                                         \
         /* Update the element count to the new size. */                                                                 \
         ref2_element_count = new_size;                                                                                  \
                                                                                                                         \
         if ( *static_cast< SimpleDataType ** >( ref2->address ) == NULL ) {                                             \
            ostringstream errmsg;                                                                                        \
            errmsg << #EncoderClassName << "::resize():" << __LINE__                                                     \
                   << " ERROR: Could not allocate memory for Trick variable"                                             \
                   << " with name '" << trick_name << "' with " << new_size                                              \
                   << " elements!\n";                                                                                    \
            DebugHandler::terminate_with_message( errmsg.str() );                                                        \
         }                                                                                                               \
      }                                                                                                                  \
                                                                                                                         \
      /* Remove the extra elements if the new_size reduces the element count. */                                         \
      size_t const orig_element_count = data_elements.size();                                                            \
      if ( orig_element_count > new_size ) {                                                                             \
         for ( size_t i = new_size; i < orig_element_count; ++i ) {                                                      \
            delete data_elements[i];                                                                                     \
         }                                                                                                               \
         data_elements.resize( new_size );                                                                               \
                                                                                                                         \
      } else if ( orig_element_count < new_size ) {                                                                      \
         /* Reserve enough capacity for the new elements for the larger size. */                                         \
         data_elements.reserve( new_size );                                                                              \
      }                                                                                                                  \
                                                                                                                         \
      HLAvariableArray *array_encoder = static_cast< HLAvariableArray * >( encoder );                                    \
      SimpleDataType   *array_data    = *static_cast< SimpleDataType ** >( ref2->address );                              \
                                                                                                                         \
      /* Because we can't resize the encoder to a smaller size we */                                                     \
      /* have to create a new one.                                */                                                     \
      if ( new_size < array_encoder->size() ) {                                                                          \
         if ( encoder != NULL ) {                                                                                        \
            delete encoder;                                                                                              \
         }                                                                                                               \
         EncodableDataType data_prototype;                                                                               \
         array_encoder = new HLAvariableArray( data_prototype );                                                         \
         this->encoder = array_encoder;                                                                                  \
      }                                                                                                                  \
                                                                                                                         \
      /* Connect the array data to the encoder array elements. */                                                        \
      EncodableDataType *element;                                                                                        \
      for ( size_t i = 0; i < new_size; ++i ) {                                                                          \
         if ( i < orig_element_count ) {                                                                                 \
            element = static_cast< EncodableDataType * >( data_elements[i] );                                            \
            element->setDataPointer( &array_data[i] );                                                                   \
         } else {                                                                                                        \
            element = new EncodableDataType( &array_data[i] );                                                           \
            data_elements.push_back( element );                                                                          \
         }                                                                                                               \
                                                                                                                         \
         if ( i < array_encoder->size() ) {                                                                              \
            array_encoder->setElementPointer( i, element );                                                              \
         } else {                                                                                                        \
            array_encoder->addElementPointer( element );                                                                 \
         }                                                                                                               \
      }                                                                                                                  \
      return true;                                                                                                       \
   }                                                                                                                     \
                                                                                                                         \
   void EncoderClassName::refresh_data_elements()                                                                        \
   {                                                                                                                     \
      HLAvariableArray *array_encoder = static_cast< HLAvariableArray * >( encoder );                                    \
                                                                                                                         \
      if ( ( data_elements.size() != ref2_element_count )                                                                \
           || ( data_elements.size() != array_encoder->size() ) ) {                                                      \
         ostringstream errmsg;                                                                                           \
         errmsg << #EncoderClassName << "::refresh_data_elements():" << __LINE__                                         \
                << " ERROR: For Trick variable with name '" << trick_name                                                \
                << "' the number of elements don't agree with the encoder!\n";                                           \
         DebugHandler::terminate_with_message( errmsg.str() );                                                           \
      }                                                                                                                  \
                                                                                                                         \
      SimpleDataType *array_data = *static_cast< SimpleDataType ** >( ref2->address );                                   \
                                                                                                                         \
      for ( size_t i = 0; i < data_elements.size(); ++i ) {                                                              \
         EncodableDataType *element = static_cast< EncodableDataType * >( data_elements[i] );                            \
         element->setDataPointer( &array_data[i] );                                                                      \
         if ( static_cast< void * >( element )                                                                           \
              != static_cast< void * >( &array_encoder[i] ) ) {                                                          \
            array_encoder->setElementPointer( i, element );                                                              \
         }                                                                                                               \
      }                                                                                                                  \
   }

DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( ASCIICharVariableArrayEncoder, HLAASCIIchar, char, TRICK_CHARACTER )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( ASCIIStringVariableArrayEncoder, HLAASCIIstring, std::string, TRICK_STRING )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( BoolVariableArrayEncoder, HLAboolean, bool, TRICK_BOOLEAN )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( ByteVariableArrayEncoder, HLAbyte, Octet, TRICK_CHARACTER )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float32BEVariableArrayEncoder, HLAfloat32BE, float, TRICK_FLOAT )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float32LEVariableArrayEncoder, HLAfloat32LE, float, TRICK_FLOAT )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float64BEVariableArrayEncoder, HLAfloat64BE, double, TRICK_DOUBLE )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float64LEVariableArrayEncoder, HLAfloat64LE, double, TRICK_DOUBLE )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int16BEVariableArrayEncoder, HLAinteger16BE, Integer16, TRICK_SHORT )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int16LEVariableArrayEncoder, HLAinteger16LE, Integer16, TRICK_SHORT )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int32BEVariableArrayEncoder, HLAinteger32BE, Integer32, TRICK_INTEGER )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int32LEVariableArrayEncoder, HLAinteger32LE, Integer32, TRICK_INTEGER )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int64BEVariableArrayEncoder, HLAinteger64BE, Integer64, TRICK_LONG_LONG )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int64LEVariableArrayEncoder, HLAinteger64LE, Integer64, TRICK_LONG_LONG )
#if defined( IEEE_1516_2025 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt16BEVariableArrayEncoder, HLAunsignedInteger16BE, UnsignedInteger16, TRICK_UNSIGNED_SHORT )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt16LEVariableArrayEncoder, HLAunsignedInteger16LE, UnsignedInteger16, TRICK_UNSIGNED_SHORT )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt32BEVariableArrayEncoder, HLAunsignedInteger32BE, UnsignedInteger32, TRICK_UNSIGNED_INTEGER )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt32LEVariableArrayEncoder, HLAunsignedInteger32LE, UnsignedInteger32, TRICK_UNSIGNED_INTEGER )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt64BEVariableArrayEncoder, HLAunsignedInteger64BE, UnsignedInteger64, TRICK_UNSIGNED_LONG_LONG )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt64LEVariableArrayEncoder, HLAunsignedInteger64LE, UnsignedInteger64, TRICK_UNSIGNED_LONG_LONG )
#endif
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UnicodeCharVariableArrayEncoder, HLAunicodeChar, wchar_t, TRICK_WCHAR )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UnicodeStringVariableArrayEncoder, HLAunicodeString, std::wstring, TRICK_WSTRING )
