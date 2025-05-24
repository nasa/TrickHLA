/*!
@file TrickHLA/BasicDataEncoders.cpp
@ingroup TrickHLA
@brief This class represents the basic data encoder implementation.

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
@trick_link_dependency{BasicDataEncoders.cpp}
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
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/BasicDataEncoders.hh"
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
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

#define DECLARE_BASIC_ENCODER_CLASS( EncoderClassName, EncodableDataType, SimpleDataType, TrickTypeEnum ) \
                                                                                                          \
   EncoderClassName::EncoderClassName(                                                                    \
      string const      &trick_variable_name,                                                             \
      EncodingEnum const hla_encoding,                                                                    \
      REF2              *r2 )                                                                             \
      : EncoderBase( trick_variable_name,                                                                 \
                     hla_encoding,                                                                        \
                     r2 )                                                                                 \
   {                                                                                                      \
      if ( ref2 == NULL ) {                                                                               \
         update_ref2();                                                                                   \
      }                                                                                                   \
                                                                                                          \
      bool valid = ( ref2->attr->type == TrickTypeEnum )                                                  \
                   || ( ( ( ref2->attr->type == TRICK_LONG )                                              \
                          || ( ref2->attr->type == TRICK_UNSIGNED_LONG ) )                                \
                        && ( sizeof( long ) == sizeof( SimpleDataType ) ) );                              \
      if ( !valid ) {                                                                                     \
         ostringstream errmsg;                                                                            \
         errmsg << #EncoderClassName << "::" << #EncoderClassName << "():" << __LINE__                    \
                << " ERROR: Trick type for the '" << trick_name                                           \
                << "' simulation variable (type:"                                                         \
                << Utilities::get_trick_type_string( ref2->attr->type )                                   \
                << ") is not the expected type '"                                                         \
                << Utilities::get_trick_type_string( TrickTypeEnum ) << "'.\n";                           \
         DebugHandler::terminate_with_message( errmsg.str() );                                            \
         return;                                                                                          \
      }                                                                                                   \
                                                                                                          \
      if ( !is_valid_encoding_for_type( hla_encoding, ref2->attr->type ) ) {                              \
         ostringstream errmsg;                                                                            \
         errmsg << #EncoderClassName << "::" #EncoderClassName << "():" << __LINE__                       \
                << " ERROR: Trick type for the '" << trick_name                                           \
                << "' simulation variable (type:"                                                         \
                << Utilities::get_trick_type_string( ref2->attr->type )                                   \
                << ") does not support the specified HLA encoding ("                                      \
                << rti_encoding << ")!\n";                                                                \
         DebugHandler::terminate_with_message( errmsg.str() );                                            \
         return;                                                                                          \
      }                                                                                                   \
                                                                                                          \
      /* This encoder is only for a primitive type. */                                                    \
      if ( is_array ) {                                                                                   \
         ostringstream errmsg;                                                                            \
         errmsg << #EncoderClassName << "::" << #EncoderClassName << "():" << __LINE__                    \
                << " ERROR: Trick ref-attributes for the '" << trick_name                                 \
                << "' variable must be a primitive and not an array!\n";                                  \
         DebugHandler::terminate_with_message( errmsg.str() );                                            \
         return;                                                                                          \
      }                                                                                                   \
                                                                                                          \
      this->encoder = new EncodableDataType(                                                              \
         static_cast< SimpleDataType * >( ref2->address ) );                                              \
   }                                                                                                      \
                                                                                                          \
   EncoderClassName::~EncoderClassName()                                                                  \
   {                                                                                                      \
      return;                                                                                             \
   }                                                                                                      \
                                                                                                          \
   string EncoderClassName::to_string()                                                                   \
   {                                                                                                      \
      ostringstream msg;                                                                                  \
      msg << #EncoderClassName                                                                            \
          << "[trick_name:'" << trick_name                                                                \
          << "' rti_encoding:" << rti_encoding << "]";                                                    \
      return msg.str();                                                                                   \
   }

DECLARE_BASIC_ENCODER_CLASS( ASCIICharEncoder, HLAASCIIchar, char, TRICK_CHARACTER )
DECLARE_BASIC_ENCODER_CLASS( ASCIIStringEncoder, HLAASCIIstring, std::string, TRICK_STRING )
DECLARE_BASIC_ENCODER_CLASS( BoolEncoder, HLAboolean, bool, TRICK_BOOLEAN )
DECLARE_BASIC_ENCODER_CLASS( ByteEncoder, HLAbyte, Octet, TRICK_CHARACTER )
DECLARE_BASIC_ENCODER_CLASS( Float32BEEncoder, HLAfloat32BE, float, TRICK_FLOAT )
DECLARE_BASIC_ENCODER_CLASS( Float32LEEncoder, HLAfloat32LE, float, TRICK_FLOAT )
DECLARE_BASIC_ENCODER_CLASS( Float64BEEncoder, HLAfloat64BE, double, TRICK_DOUBLE )
DECLARE_BASIC_ENCODER_CLASS( Float64LEEncoder, HLAfloat64LE, double, TRICK_DOUBLE )
DECLARE_BASIC_ENCODER_CLASS( Int16BEEncoder, HLAinteger16BE, Integer16, TRICK_SHORT )
DECLARE_BASIC_ENCODER_CLASS( Int16LEEncoder, HLAinteger16LE, Integer16, TRICK_SHORT )
DECLARE_BASIC_ENCODER_CLASS( Int32BEEncoder, HLAinteger32BE, Integer32, TRICK_INTEGER )
DECLARE_BASIC_ENCODER_CLASS( Int32LEEncoder, HLAinteger32LE, Integer32, TRICK_INTEGER )
DECLARE_BASIC_ENCODER_CLASS( Int64BEEncoder, HLAinteger64BE, Integer64, TRICK_LONG_LONG )
DECLARE_BASIC_ENCODER_CLASS( Int64LEEncoder, HLAinteger64LE, Integer64, TRICK_LONG_LONG )
#if defined( IEEE_1516_2025 )
DECLARE_BASIC_ENCODER_CLASS( UInt16BEEncoder, HLAunsignedInteger16BE, UnsignedInteger16, TRICK_UNSIGNED_SHORT )
DECLARE_BASIC_ENCODER_CLASS( UInt16LEEncoder, HLAunsignedInteger16LE, UnsignedInteger16, TRICK_UNSIGNED_SHORT )
DECLARE_BASIC_ENCODER_CLASS( UInt32BEEncoder, HLAunsignedInteger32BE, UnsignedInteger32, TRICK_UNSIGNED_INTEGER )
DECLARE_BASIC_ENCODER_CLASS( UInt32LEEncoder, HLAunsignedInteger32LE, UnsignedInteger32, TRICK_UNSIGNED_INTEGER )
DECLARE_BASIC_ENCODER_CLASS( UInt64BEEncoder, HLAunsignedInteger64BE, UnsignedInteger64, TRICK_UNSIGNED_LONG_LONG )
DECLARE_BASIC_ENCODER_CLASS( UInt64LEEncoder, HLAunsignedInteger64LE, UnsignedInteger64, TRICK_UNSIGNED_LONG_LONG )
#endif
DECLARE_BASIC_ENCODER_CLASS( UnicodeCharEncoder, HLAunicodeChar, wchar_t, TRICK_WCHAR )
DECLARE_BASIC_ENCODER_CLASS( UnicodeStringEncoder, HLAunicodeString, std::wstring, TRICK_WSTRING )
