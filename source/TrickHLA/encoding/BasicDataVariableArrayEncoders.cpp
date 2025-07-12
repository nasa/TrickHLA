/*!
@file TrickHLA/encoding/BasicDataVariableArrayEncoders.cpp
@ingroup TrickHLA
@brief This class represents the basic data variable array encoder implementation.

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
@trick_link_dependency{BasicDataVariableArrayEncoders.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{VariableArrayEncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <string>

// Trick include files.
#include "trick/attributes.h"

// TrickHLA include files.
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/encoding/BasicDataVariableArrayEncoders.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"

// HLA include files.
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/EncodingConfig.h"
#include "RTI/encoding/HLAvariableArray.h"

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

#define DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( EncoderClassName, EncodableDataType, SimpleDataType )                      \
                                                                                                                               \
   EncoderClassName::EncoderClassName(                                                                                         \
      void       *addr,                                                                                                        \
      ATTRIBUTES *attr )                                                                                                       \
      : VariableArrayEncoderBase( addr, attr )                                                                                 \
   {                                                                                                                           \
      this->data_encoder = new HLAvariableArray( EncodableDataType() );                                                        \
      resize_data_elements( var_element_count );                                                                               \
   }                                                                                                                           \
                                                                                                                               \
   EncoderClassName::~EncoderClassName()                                                                                       \
   {                                                                                                                           \
      return;                                                                                                                  \
   }                                                                                                                           \
                                                                                                                               \
   void EncoderClassName::update_before_encode()                                                                               \
   {                                                                                                                           \
      /* Since the Trick variable is dynamic (i.e. a pointer) its size */                                                      \
      /* can change at any point so we need to refresh the counts.     */                                                      \
      calculate_var_element_count();                                                                                           \
                                                                                                                               \
      /* Ensure the number of data elements matches the Trick variable */                                                      \
      resize_data_elements( var_element_count );                                                                               \
                                                                                                                               \
      HLAvariableArray *array_encoder = dynamic_cast< HLAvariableArray * >( data_encoder );                                    \
      SimpleDataType   *array_data    = *static_cast< SimpleDataType ** >( address ); /* NOLINT(bugprone-macro-parentheses) */ \
                                                                                                                               \
      /* Copy the Trick array values to the data elements to be encoded. */                                                    \
      for ( size_t i = 0; i < var_element_count; ++i ) {                                                                       \
         const_cast< EncodableDataType & >( /* NOLINT(bugprone-macro-parentheses) */                                           \
                                            dynamic_cast< EncodableDataType const & >(                                         \
                                               array_encoder->get( i ) ) )                                                     \
            .set( array_data[i] );                                                                                             \
      }                                                                                                                        \
   }                                                                                                                           \
                                                                                                                               \
   void EncoderClassName::update_after_decode()                                                                                \
   {                                                                                                                           \
      HLAvariableArray *array_encoder = dynamic_cast< HLAvariableArray * >( data_encoder );                                    \
                                                                                                                               \
      /* Resize Trick array variable to match the decoded data size. */                                                        \
      resize_trick_var( array_encoder->size() );                                                                               \
                                                                                                                               \
      SimpleDataType *array_data = *static_cast< SimpleDataType ** >( address ); /* NOLINT(bugprone-macro-parentheses) */      \
                                                                                                                               \
      /* Copy the decoded data element values to the Trick array. */                                                           \
      for ( size_t i = 0; i < var_element_count; ++i ) {                                                                       \
         array_data[i] = dynamic_cast< EncodableDataType const & >(                                                            \
                            array_encoder->get( i ) )                                                                          \
                            .get();                                                                                            \
      }                                                                                                                        \
   }                                                                                                                           \
                                                                                                                               \
   void EncoderClassName::resize_data_elements(                                                                                \
      size_t new_size )                                                                                                        \
   {                                                                                                                           \
      HLAvariableArray *array_encoder = dynamic_cast< HLAvariableArray * >( data_encoder );                                    \
      if ( new_size != array_encoder->size() ) {                                                                               \
         EncodableDataType const data_prototype;                                                                               \
                                                                                                                               \
         if ( new_size < array_encoder->size() ) {                                                                             \
            /* Because we can't resize the encoder to a smaller */                                                             \
            /* size we have to create a new one.                */                                                             \
            delete data_encoder;                                                                                               \
            array_encoder = new HLAvariableArray( data_prototype );                                                            \
            data_encoder  = array_encoder;                                                                                     \
         }                                                                                                                     \
                                                                                                                               \
         while ( array_encoder->size() < new_size ) {                                                                          \
            array_encoder->addElement( data_prototype );                                                                       \
         }                                                                                                                     \
      }                                                                                                                        \
   }

DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( ASCIICharVariableArrayEncoder, HLAASCIIchar, char )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( ASCIIStringVariableArrayEncoder, HLAASCIIstring, std::string )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( BoolVariableArrayEncoder, HLAboolean, bool )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( ByteVariableArrayEncoder, HLAbyte, Octet )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float32BEVariableArrayEncoder, HLAfloat32BE, float )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float32LEVariableArrayEncoder, HLAfloat32LE, float )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float64BEVariableArrayEncoder, HLAfloat64BE, double )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Float64LEVariableArrayEncoder, HLAfloat64LE, double )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int16BEVariableArrayEncoder, HLAinteger16BE, Integer16 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int16LEVariableArrayEncoder, HLAinteger16LE, Integer16 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int32BEVariableArrayEncoder, HLAinteger32BE, Integer32 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int32LEVariableArrayEncoder, HLAinteger32LE, Integer32 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int64BEVariableArrayEncoder, HLAinteger64BE, Integer64 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( Int64LEVariableArrayEncoder, HLAinteger64LE, Integer64 )

#if defined( IEEE_1516_2025 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt16BEVariableArrayEncoder, HLAunsignedInteger16BE, UnsignedInteger16 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt16LEVariableArrayEncoder, HLAunsignedInteger16LE, UnsignedInteger16 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt32BEVariableArrayEncoder, HLAunsignedInteger32BE, UnsignedInteger32 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt32LEVariableArrayEncoder, HLAunsignedInteger32LE, UnsignedInteger32 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt64BEVariableArrayEncoder, HLAunsignedInteger64BE, UnsignedInteger64 )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UInt64LEVariableArrayEncoder, HLAunsignedInteger64LE, UnsignedInteger64 )
#endif // IEEE_1516_2025

DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UnicodeCharVariableArrayEncoder, HLAunicodeChar, wchar_t )

#if defined( TRICK_WSTRING_MM_SUPPORT )
DECLARE_BASIC_VARIABLE_ARRAY_ENCODER_CLASS( UnicodeStringVariableArrayEncoder, HLAunicodeString, std::wstring )
#endif // TRICK_WSTRING_MM_SUPPORT

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic pop
