/*!
@file TrickHLA/encoding/BasicDataEncoders.cpp
@ingroup TrickHLA
@brief This class represents the basic data encoder implementation.

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
@trick_link_dependency{BasicDataEncoders.cpp}
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
#include <string>
#include <typeinfo>

// Trick include files.
#include "trick/attributes.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/encoding/BasicDataEncoders.hh"
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

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

#define DECLARE_BASIC_ENCODER_CLASS( EncoderClassName, EncodableDataType, SimpleDataType )                                            \
                                                                                                                                      \
   EncoderClassName::EncoderClassName(                                                                                                \
      void              *addr,                                                                                                        \
      ATTRIBUTES        *attr,                                                                                                        \
      std::string const &name )                                                                                                       \
      : EncoderBase( name )                                                                                                           \
   {                                                                                                                                  \
      this->data_encoder = new EncodableDataType( static_cast< SimpleDataType * >( addr ) ); /* NOLINT(bugprone-macro-parentheses) */ \
   }                                                                                                                                  \
                                                                                                                                      \
   EncoderClassName::~EncoderClassName()                                                                                              \
   {                                                                                                                                  \
      return;                                                                                                                         \
   }                                                                                                                                  \
                                                                                                                                      \
   int const EncoderClassName::get_data_size()                                                                                        \
   {                                                                                                                                  \
      if ( data_encoder != NULL ) {                                                                                                   \
         if ( typeid( SimpleDataType ) == typeid( std::string ) ) {                                                                   \
            return dynamic_cast< HLAASCIIstring * >( data_encoder )->get().size();                                                    \
         } else if ( typeid( SimpleDataType ) == typeid( std::wstring ) ) {                                                           \
            return ( sizeof( wchar_t ) * dynamic_cast< HLAunicodeString * >( data_encoder )->get().size() );                          \
         } else {                                                                                                                     \
            return sizeof( SimpleDataType );                                                                                          \
         }                                                                                                                            \
      }                                                                                                                               \
      return 0;                                                                                                                       \
   }

DECLARE_BASIC_ENCODER_CLASS( ASCIICharEncoder, HLAASCIIchar, char )
DECLARE_BASIC_ENCODER_CLASS( ASCIIStringEncoder, HLAASCIIstring, std::string )
DECLARE_BASIC_ENCODER_CLASS( BoolEncoder, HLAboolean, bool )
DECLARE_BASIC_ENCODER_CLASS( ByteEncoder, HLAbyte, Octet )
DECLARE_BASIC_ENCODER_CLASS( Float32BEEncoder, HLAfloat32BE, float )
DECLARE_BASIC_ENCODER_CLASS( Float32LEEncoder, HLAfloat32LE, float )
DECLARE_BASIC_ENCODER_CLASS( Float64BEEncoder, HLAfloat64BE, double )
DECLARE_BASIC_ENCODER_CLASS( Float64LEEncoder, HLAfloat64LE, double )
DECLARE_BASIC_ENCODER_CLASS( Int16BEEncoder, HLAinteger16BE, Integer16 )
DECLARE_BASIC_ENCODER_CLASS( Int16LEEncoder, HLAinteger16LE, Integer16 )
DECLARE_BASIC_ENCODER_CLASS( Int32BEEncoder, HLAinteger32BE, Integer32 )
DECLARE_BASIC_ENCODER_CLASS( Int32LEEncoder, HLAinteger32LE, Integer32 )
DECLARE_BASIC_ENCODER_CLASS( Int64BEEncoder, HLAinteger64BE, Integer64 )
DECLARE_BASIC_ENCODER_CLASS( Int64LEEncoder, HLAinteger64LE, Integer64 )

#if defined( IEEE_1516_2025 )
DECLARE_BASIC_ENCODER_CLASS( UInt16BEEncoder, HLAunsignedInteger16BE, UnsignedInteger16 )
DECLARE_BASIC_ENCODER_CLASS( UInt16LEEncoder, HLAunsignedInteger16LE, UnsignedInteger16 )
DECLARE_BASIC_ENCODER_CLASS( UInt32BEEncoder, HLAunsignedInteger32BE, UnsignedInteger32 )
DECLARE_BASIC_ENCODER_CLASS( UInt32LEEncoder, HLAunsignedInteger32LE, UnsignedInteger32 )
DECLARE_BASIC_ENCODER_CLASS( UInt64BEEncoder, HLAunsignedInteger64BE, UnsignedInteger64 )
DECLARE_BASIC_ENCODER_CLASS( UInt64LEEncoder, HLAunsignedInteger64LE, UnsignedInteger64 )
#endif // IEEE_1516_2025

DECLARE_BASIC_ENCODER_CLASS( UnicodeCharEncoder, HLAunicodeChar, wchar_t )

#if defined( TRICK_WSTRING_MM_SUPPORT )
DECLARE_BASIC_ENCODER_CLASS( UnicodeStringEncoder, HLAunicodeString, std::wstring )
#endif // TRICK_WSTRING_MM_SUPPORT
