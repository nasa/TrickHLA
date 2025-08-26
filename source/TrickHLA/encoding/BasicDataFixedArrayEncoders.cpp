/*!
@file TrickHLA/encoding/BasicDataFixedArrayEncoders.cpp
@ingroup TrickHLA
@brief This class represents the basic data fixed array encoder implementation.

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
@trick_link_dependency{BasicDataFixedArrayEncoders.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstddef>
#include <cstdlib>
#include <string>
#include <typeinfo>

// Trick include files.
#include "trick/attributes.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/BasicDataFixedArrayEncoders.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/EncodingConfig.h"
#include "RTI/encoding/HLAfixedArray.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

#define DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( EncoderClassName, EncodableDataType, SimpleDataType )                                 \
                                                                                                                                       \
   EncoderClassName::EncoderClassName(                                                                                                 \
      void              *addr,                                                                                                         \
      ATTRIBUTES        *attr,                                                                                                         \
      std::string const &name )                                                                                                        \
      : EncoderBase( name )                                                                                                            \
   {                                                                                                                                   \
      size_t const   length        = Utilities::get_static_var_element_count( attr );                                                  \
      HLAfixedArray *array_encoder = new HLAfixedArray( EncodableDataType(), length );                                                 \
      this->data_encoder           = array_encoder;                                                                                    \
                                                                                                                                       \
      /* Connect the users array data to the encoder array elements. */                                                                \
      if ( addr != NULL ) {                                                                                                            \
         SimpleDataType *array_data = static_cast< SimpleDataType * >( addr );                                                         \
         for ( size_t i = 0; i < length; ++i ) {                                                                                       \
            const_cast< EncodableDataType & >( /* NOLINT(bugprone-macro-parentheses) */                                                \
                                               dynamic_cast< EncodableDataType const & >(                                              \
                                                  array_encoder->get( i ) ) )                                                          \
               .setDataPointer( &array_data[i] );                                                                                      \
         }                                                                                                                             \
      }                                                                                                                                \
   }                                                                                                                                   \
                                                                                                                                       \
   EncoderClassName::~EncoderClassName()                                                                                               \
   {                                                                                                                                   \
      return;                                                                                                                          \
   }                                                                                                                                   \
                                                                                                                                       \
   int const EncoderClassName::get_data_size()                                                                                         \
   {                                                                                                                                   \
      int byte_count = 0;                                                                                                              \
      if ( data_encoder != NULL ) {                                                                                                    \
         HLAfixedArray const *array_encoder = dynamic_cast< HLAfixedArray * >( data_encoder );                                         \
         int const            array_size    = array_encoder->size();                                                                   \
                                                                                                                                       \
         if ( typeid( SimpleDataType ) == typeid( std::string ) ) {                                                                    \
            for ( int i = 0; i < array_size; ++i ) {                                                                                   \
               byte_count += dynamic_cast< HLAASCIIstring const & >( array_encoder->get( i ) ).get().size();                           \
            }                                                                                                                          \
         } else if ( typeid( SimpleDataType ) == typeid( std::wstring ) ) {                                                            \
            for ( int i = 0; i < array_size; ++i ) {                                                                                   \
               byte_count += ( sizeof( wchar_t ) * dynamic_cast< HLAunicodeString const & >( array_encoder->get( i ) ).get().size() ); \
            }                                                                                                                          \
         } else {                                                                                                                      \
            byte_count = sizeof( SimpleDataType ) * array_size;                                                                        \
         }                                                                                                                             \
      }                                                                                                                                \
      return byte_count;                                                                                                               \
   }

DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ASCIICharFixedArrayEncoder, HLAASCIIchar, char )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ASCIIStringFixedArrayEncoder, HLAASCIIstring, std::string )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( BoolFixedArrayEncoder, HLAboolean, bool )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ByteFixedArrayEncoder, HLAbyte, Octet )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float32BEFixedArrayEncoder, HLAfloat32BE, float )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float32LEFixedArrayEncoder, HLAfloat32LE, float )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float64BEFixedArrayEncoder, HLAfloat64BE, double )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float64LEFixedArrayEncoder, HLAfloat64LE, double )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int16BEFixedArrayEncoder, HLAinteger16BE, Integer16 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int16LEFixedArrayEncoder, HLAinteger16LE, Integer16 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int32BEFixedArrayEncoder, HLAinteger32BE, Integer32 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int32LEFixedArrayEncoder, HLAinteger32LE, Integer32 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int64BEFixedArrayEncoder, HLAinteger64BE, Integer64 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int64LEFixedArrayEncoder, HLAinteger64LE, Integer64 )

#if defined( IEEE_1516_2025 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt16BEFixedArrayEncoder, HLAunsignedInteger16BE, UnsignedInteger16 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt16LEFixedArrayEncoder, HLAunsignedInteger16LE, UnsignedInteger16 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt32BEFixedArrayEncoder, HLAunsignedInteger32BE, UnsignedInteger32 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt32LEFixedArrayEncoder, HLAunsignedInteger32LE, UnsignedInteger32 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt64BEFixedArrayEncoder, HLAunsignedInteger64BE, UnsignedInteger64 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt64LEFixedArrayEncoder, HLAunsignedInteger64LE, UnsignedInteger64 )
#endif // IEEE_1516_2025

DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UnicodeCharFixedArrayEncoder, HLAunicodeChar, wchar_t )

#if defined( TRICK_WSTRING_MM_SUPPORT )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UnicodeStringFixedArrayEncoder, HLAunicodeString, std::wstring )
#endif // TRICK_WSTRING_MM_SUPPORT
