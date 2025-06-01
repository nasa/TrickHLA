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
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{BasicDataFixedArrayEncoders.cpp}
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
#include "TrickHLA/encoding/BasicDataFixedArrayEncoders.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/HLAfixedArray.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

#define DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( EncoderClassName, EncodableDataType, SimpleDataType, TrickTypeEnum ) \
                                                                                                                      \
   EncoderClassName::EncoderClassName(                                                                                \
      void       *var_address,                                                                                        \
      ATTRIBUTES *var_attr )                                                                                          \
      : EncoderBase( var_address, var_attr )                                                                          \
   {                                                                                                                  \
      bool valid = ( attr->type == TrickTypeEnum )                                                                    \
                   || ( ( ( attr->type == TRICK_LONG )                                                                \
                          || ( attr->type == TRICK_UNSIGNED_LONG ) )                                                  \
                        && ( sizeof( long ) == sizeof( SimpleDataType ) ) )                                           \
                   || ( attr->type == TRICK_UNSIGNED_CHARACTER );                                                     \
      if ( !valid ) {                                                                                                 \
         ostringstream errmsg;                                                                                        \
         errmsg << #EncoderClassName << "::" #EncoderClassName << "():" << __LINE__                                   \
                << " ERROR: Trick type for the '" << attr->name                                                       \
                << "' simulation variable (type:"                                                                     \
                << Utilities::get_trick_type_string( attr->type )                                                     \
                << ") is not the expected type '"                                                                     \
                << Utilities::get_trick_type_string( TrickTypeEnum ) << "'.\n";                                       \
         DebugHandler::terminate_with_message( errmsg.str() );                                                        \
         return;                                                                                                      \
      }                                                                                                               \
                                                                                                                      \
      /* This encoder is only for a static array. */                                                                  \
      if ( !is_static_array() ) {                                                                                     \
         ostringstream errmsg;                                                                                        \
         errmsg << #EncoderClassName << "::" #EncoderClassName << "():" << __LINE__                                   \
                << " ERROR: Trick ref-attributes for '" << attr->name                                                 \
                << "' the variable must be a static array!\n";                                                        \
         DebugHandler::terminate_with_message( errmsg.str() );                                                        \
         return;                                                                                                      \
      }                                                                                                               \
                                                                                                                      \
      EncodableDataType data_prototype;                                                                               \
      HLAfixedArray    *array_encoder = new HLAfixedArray( data_prototype, attr_element_count );                      \
                                                                                                                      \
      this->encoder = array_encoder;                                                                                  \
                                                                                                                      \
      data_elements.reserve( attr_element_count );                                                                    \
      SimpleDataType *array_data = static_cast< SimpleDataType * >( address );                                        \
                                                                                                                      \
      /* Connect the users array data to the encoder array elements. */                                               \
      for ( size_t i = 0; i < attr_element_count; ++i ) {                                                             \
         EncodableDataType *element = new EncodableDataType( &array_data[i] );                                        \
         data_elements.push_back( element );                                                                          \
         array_encoder->setElementPointer( i, element );                                                              \
      }                                                                                                               \
   }                                                                                                                  \
                                                                                                                      \
   EncoderClassName::~EncoderClassName()                                                                              \
   {                                                                                                                  \
      return;                                                                                                         \
   }                                                                                                                  \
                                                                                                                      \
   string EncoderClassName::to_string()                                                                               \
   {                                                                                                                  \
      ostringstream msg;                                                                                              \
      msg << #EncoderClassName << "[" << string( attr->name ) << "]";                                                 \
      return msg.str();                                                                                               \
   }

DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ASCIICharFixedArrayEncoder, HLAASCIIchar, char, TRICK_CHARACTER )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ASCIIStringFixedArrayEncoder, HLAASCIIstring, std::string, TRICK_STRING )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( BoolFixedArrayEncoder, HLAboolean, bool, TRICK_BOOLEAN )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( ByteFixedArrayEncoder, HLAbyte, Octet, TRICK_CHARACTER )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float32BEFixedArrayEncoder, HLAfloat32BE, float, TRICK_FLOAT )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float32LEFixedArrayEncoder, HLAfloat32LE, float, TRICK_FLOAT )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float64BEFixedArrayEncoder, HLAfloat64BE, double, TRICK_DOUBLE )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Float64LEFixedArrayEncoder, HLAfloat64LE, double, TRICK_DOUBLE )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int16BEFixedArrayEncoder, HLAinteger16BE, Integer16, TRICK_SHORT )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int16LEFixedArrayEncoder, HLAinteger16LE, Integer16, TRICK_SHORT )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int32BEFixedArrayEncoder, HLAinteger32BE, Integer32, TRICK_INTEGER )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int32LEFixedArrayEncoder, HLAinteger32LE, Integer32, TRICK_INTEGER )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int64BEFixedArrayEncoder, HLAinteger64BE, Integer64, TRICK_LONG_LONG )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( Int64LEFixedArrayEncoder, HLAinteger64LE, Integer64, TRICK_LONG_LONG )

#if defined( IEEE_1516_2025 )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt16BEFixedArrayEncoder, HLAunsignedInteger16BE, UnsignedInteger16, TRICK_UNSIGNED_SHORT )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt16LEFixedArrayEncoder, HLAunsignedInteger16LE, UnsignedInteger16, TRICK_UNSIGNED_SHORT )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt32BEFixedArrayEncoder, HLAunsignedInteger32BE, UnsignedInteger32, TRICK_UNSIGNED_INTEGER )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt32LEFixedArrayEncoder, HLAunsignedInteger32LE, UnsignedInteger32, TRICK_UNSIGNED_INTEGER )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt64BEFixedArrayEncoder, HLAunsignedInteger64BE, UnsignedInteger64, TRICK_UNSIGNED_LONG_LONG )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UInt64LEFixedArrayEncoder, HLAunsignedInteger64LE, UnsignedInteger64, TRICK_UNSIGNED_LONG_LONG )
#endif // IEEE_1516_2025

DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UnicodeCharFixedArrayEncoder, HLAunicodeChar, wchar_t, TRICK_WCHAR )

#if defined( TRICK_WSTRING_MM_SUPPORT )
DECLARE_BASIC_FIXED_ARRAY_ENCODER_CLASS( UnicodeStringFixedArrayEncoder, HLAunicodeString, std::wstring, TRICK_WSTRING )
#endif // TRICK_WSTRING_MM_SUPPORT
