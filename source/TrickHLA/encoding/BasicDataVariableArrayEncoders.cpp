/*!
@file TrickHLA/encoding/BasicDataVariableArrayEncoders.cpp
@ingroup TrickHLA
@brief This class represents the basic data variable array encoder implementation.

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
#include <cstddef>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
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
      bool valid = ( ref2->attr->type == TrickTypeEnum )                                                                 \
                   || ( ( ( ref2->attr->type == TRICK_LONG )                                                             \
                          || ( ref2->attr->type == TRICK_UNSIGNED_LONG ) )                                               \
                        && ( sizeof( long ) == sizeof( SimpleDataType ) ) );                                             \
      if ( !valid ) {                                                                                                    \
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
      resize_data_elements( ref2_element_count );                                                                        \
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
      /* Ensure the number of data elements matches the Trick variable */                                                \
      resize_data_elements( ref2_element_count );                                                                        \
                                                                                                                         \
      HLAvariableArray const *array_encoder = dynamic_cast< HLAvariableArray * >( encoder );                             \
      SimpleDataType         *array_data    = *static_cast< SimpleDataType ** >( ref2->address );                        \
                                                                                                                         \
      /* Copy the Trick array values to the data elements to be encoded. */                                              \
      for ( size_t i = 0; i < ref2_element_count; ++i ) {                                                                \
         const_cast< EncodableDataType & >(                                                                              \
            dynamic_cast< EncodableDataType const & >(                                                                   \
               array_encoder->get( i ) ) )                                                                               \
            .set( array_data[i] );                                                                                       \
      }                                                                                                                  \
                                                                                                                         \
      return EncoderBase::encode();                                                                                      \
   }                                                                                                                     \
                                                                                                                         \
   void EncoderClassName::decode(                                                                                        \
      VariableLengthData const &encoded_data )                                                                           \
   {                                                                                                                     \
      EncoderBase::decode( encoded_data );                                                                               \
                                                                                                                         \
      HLAvariableArray const *array_encoder = dynamic_cast< HLAvariableArray * >( encoder );                             \
                                                                                                                         \
      /* Trick variable is dynamic (i.e. a pointer) so we need to refresh ref2. */                                       \
      update_ref2();                                                                                                     \
                                                                                                                         \
      /* Resize Trick array variable to match the decoded data size. */                                                  \
      resize_trick_var( array_encoder->size() );                                                                         \
                                                                                                                         \
      SimpleDataType *array_data = *static_cast< SimpleDataType ** >( ref2->address );                                   \
                                                                                                                         \
      /* Copy the decoded data element values to the Trick array. */                                                     \
      for ( size_t i = 0; i < ref2_element_count; ++i ) {                                                                \
         array_data[i] = dynamic_cast< EncodableDataType const & >( array_encoder->get( i ) ).get();                     \
      }                                                                                                                  \
   }                                                                                                                     \
                                                                                                                         \
   string EncoderClassName::to_string()                                                                                  \
   {                                                                                                                     \
      ostringstream msg;                                                                                                 \
      msg << #EncoderClassName << "[trick_name:'" << trick_name                                                          \
          << "' rti_encoding:" << rti_encoding << "]";                                                                   \
      return msg.str();                                                                                                  \
   }                                                                                                                     \
                                                                                                                         \
   void EncoderClassName::resize_trick_var(                                                                              \
      size_t const new_size )                                                                                            \
   {                                                                                                                     \
      /* Trick array variable size does not match the new size. */                                                       \
      if ( ref2_element_count != new_size ) {                                                                            \
         if ( ref2->attr->type == TRICK_STRING ) {                                                                       \
            /* TMM_resize_array_1d_a does not support STL strings. */                                                    \
            if ( *( static_cast< void ** >( ref2->address ) ) != NULL ) {                                                \
               TMM_delete_var_a( *( static_cast< void ** >( ref2->address ) ) );                                         \
            }                                                                                                            \
            *( static_cast< void ** >( ref2->address ) ) =                                                               \
               static_cast< void * >( TMM_declare_var_1d( "std::string", new_size ) );                                   \
         } else {                                                                                                        \
            if ( *( static_cast< void ** >( ref2->address ) ) == NULL ) {                                                \
               *( static_cast< void ** >( ref2->address ) ) =                                                            \
                  static_cast< void * >( TMM_declare_var_1d( #SimpleDataType, new_size ) );                              \
            } else {                                                                                                     \
               *( static_cast< void ** >( ref2->address ) ) =                                                            \
                  static_cast< void * >( TMM_resize_array_1d_a(                                                          \
                     *( static_cast< void ** >( ref2->address ) ), new_size ) );                                         \
            }                                                                                                            \
         }                                                                                                               \
      }                                                                                                                  \
                                                                                                                         \
      /* Update the element count to the new size. */                                                                    \
      ref2_element_count = new_size;                                                                                     \
                                                                                                                         \
      if ( *static_cast< SimpleDataType ** >( ref2->address ) == NULL ) {                                                \
         ostringstream errmsg;                                                                                           \
         errmsg << #EncoderClassName << "::resize_trick_var():" << __LINE__                                              \
                << " ERROR: Could not allocate memory for Trick variable"                                                \
                << " with name '" << trick_name << "' with " << new_size                                                 \
                << " elements!\n";                                                                                       \
         DebugHandler::terminate_with_message( errmsg.str() );                                                           \
      }                                                                                                                  \
   }                                                                                                                     \
                                                                                                                         \
   void EncoderClassName::resize_data_elements(                                                                          \
      size_t const new_size )                                                                                            \
   {                                                                                                                     \
      HLAvariableArray *array_encoder = dynamic_cast< HLAvariableArray * >( encoder );                                   \
                                                                                                                         \
      if ( array_encoder->size() != new_size ) {                                                                         \
         EncodableDataType data_prototype;                                                                               \
                                                                                                                         \
         if ( new_size < array_encoder->size() ) {                                                                       \
            /* Because we can't resize the encoder to a smaller */                                                       \
            /* size we have to create a new one.                */                                                       \
            delete this->encoder;                                                                                        \
            array_encoder = new HLAvariableArray( data_prototype );                                                      \
            this->encoder = array_encoder;                                                                               \
         }                                                                                                               \
                                                                                                                         \
         while ( array_encoder->size() < new_size ) {                                                                    \
            array_encoder->addElement( data_prototype );                                                                 \
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
#endif // IEEE_1516_2025

DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UnicodeCharVariableArrayEncoder, HLAunicodeChar, wchar_t, TRICK_WCHAR )

#if defined( TRICK_WSTRING_MM_SUPPORT )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UnicodeStringVariableArrayEncoder, HLAunicodeString, std::wstring, TRICK_WSTRING )
#endif // TRICK_WSTRING_MM_SUPPORT
