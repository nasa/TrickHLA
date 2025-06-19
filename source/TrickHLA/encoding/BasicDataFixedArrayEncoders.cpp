/*!
@file TrickHLA/encoding/BasicDataFixedArrayEncoders.cpp
@ingroup TrickHLA
@brief This class represents the basic data fixed array encoder implementation.

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
#include <string>

// TrickHLA include files.
#include "TrickHLA/encoding/BasicDataFixedArrayEncoders.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/EncodingConfig.h"
#include "RTI/encoding/HLAfixedArray.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

#define DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( EncoderClassName, EncodableDataType, SimpleDataType ) \
                                                                                                       \
   EncoderClassName::EncoderClassName(                                                                 \
      SimpleDataType *array_data,                                                                      \
      size_t          length )                                                                         \
      : EncoderBase()                                                                                  \
   {                                                                                                   \
      HLAfixedArray *array_encoder = new HLAfixedArray( EncodableDataType(), length );                 \
      this->data_encoder           = array_encoder;                                                    \
                                                                                                       \
      /* Connect the users array data to the encoder array elements. */                                \
      for ( size_t i = 0; i < length; ++i ) {                                                          \
         const_cast< EncodableDataType & >(                                                            \
            dynamic_cast< EncodableDataType const & >(                                                 \
               array_encoder->get( i ) ) )                                                             \
            .setDataPointer( &array_data[i] );                                                         \
      }                                                                                                \
   }                                                                                                   \
                                                                                                       \
   EncoderClassName::~EncoderClassName()                                                               \
   {                                                                                                   \
      return;                                                                                          \
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
