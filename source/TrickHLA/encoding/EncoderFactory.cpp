/*!
@file TrickHLA/EncoderFactory.cpp
@ingroup TrickHLA
@brief This class represents the encoder factory implementation.

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
@trick_link_dependency{EncoderFactory.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{Int16Encoder.cpp}
@trick_link_dependency{Int16FixedArrayEncoder.cpp}
@trick_link_dependency{Int16VariableArrayEncoder.cpp}
@trick_link_dependency{Int32Encoder.cpp}
@trick_link_dependency{Int32FixedArrayEncoder.cpp}
@trick_link_dependency{Int32VariableArrayEncoder.cpp}
@trick_link_dependency{Int64Encoder.cpp}
@trick_link_dependency{Int64FixedArrayEncoder.cpp}
@trick_link_dependency{Int64VariableArrayEncoder.cpp}
@trick_link_dependency{UInt16Encoder.cpp}
@trick_link_dependency{UInt16FixedArrayEncoder.cpp}
@trick_link_dependency{UInt16VariableArrayEncoder.cpp}
@trick_link_dependency{UInt32Encoder.cpp}
@trick_link_dependency{UInt32FixedArrayEncoder.cpp}
@trick_link_dependency{UInt32VariableArrayEncoder.cpp}
@trick_link_dependency{UInt64Encoder.cpp}
@trick_link_dependency{UInt64FixedArrayEncoder.cpp}
@trick_link_dependency{UInt64VariableArrayEncoder.cpp}
@trick_link_dependency{WstringEncoder.cpp}
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
#include "TrickHLA/encoding/EncoderFactory.hh"
#include "TrickHLA/encoding/Int16Encoder.hh"
#include "TrickHLA/encoding/Int16FixedArrayEncoder.hh"
#include "TrickHLA/encoding/Int16VariableArrayEncoder.hh"
#include "TrickHLA/encoding/Int32Encoder.hh"
#include "TrickHLA/encoding/Int32FixedArrayEncoder.hh"
#include "TrickHLA/encoding/Int32VariableArrayEncoder.hh"
#include "TrickHLA/encoding/Int64Encoder.hh"
#include "TrickHLA/encoding/Int64FixedArrayEncoder.hh"
#include "TrickHLA/encoding/Int64VariableArrayEncoder.hh"
#include "TrickHLA/encoding/UInt16Encoder.hh"
#include "TrickHLA/encoding/UInt16FixedArrayEncoder.hh"
#include "TrickHLA/encoding/UInt16VariableArrayEncoder.hh"
#include "TrickHLA/encoding/UInt32Encoder.hh"
#include "TrickHLA/encoding/UInt32FixedArrayEncoder.hh"
#include "TrickHLA/encoding/UInt32VariableArrayEncoder.hh"
#include "TrickHLA/encoding/UInt64Encoder.hh"
#include "TrickHLA/encoding/UInt64FixedArrayEncoder.hh"
#include "TrickHLA/encoding/UInt64VariableArrayEncoder.hh"
#include "TrickHLA/encoding/WstringEncoder.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details The endianess of the computer is determined as part of the
 * EncoderFactory construction process.
 * @job_class{initialization}
 */
EncoderFactory::EncoderFactory()
{
   return;
}

/*!
 * @details The buffer and ref2 values are freed and nulled.
 * @job_class{shutdown}
 */
EncoderFactory::~EncoderFactory()
{
   return;
}

EncoderBase *EncoderFactory::create(
   string const      &trick_name,
   EncodingEnum const hla_encoding )
{
   REF2 *ref2 = ref_attributes( trick_name.c_str() );

   // Determine if we had an error getting the ref-attributes.
   if ( ref2 == NULL ) {
      ostringstream errmsg;
      errmsg << "EncoderFactory::create():" << __LINE__
             << " ERROR: Could not retrieve Trick ref-attributes for '"
             << trick_name << "'. Please check your input or modified-data"
             << " files to make sure the object attribute Trick name is"
             << " correctly specified. If '" << trick_name
             << "' is an inherited variable then make sure the base class"
             << " uses either the 'public' or 'protected' access level for"
             << " the variable.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return NULL;
   }

#if 0
   // For now, we do not support more than a 1-D array that is dynamic
   // (i.e. a pointer such as char *). If the size of the last indexed
   // attribute is zero then it is a pointer and not static.
   bool const is_array    = ( ref2->attr->num_index > 0 );
   bool const is_1d_array = ( ref2->attr->num_index == 1 );

   // For now, only support 1-dimensional arrays.
   if ( is_array && !is_1d_array ) {
      ostringstream errmsg;
      errmsg << "EncoderFactory::create():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << trick_name
             << "' the variable is a multidimensional array and only"
             << " 1-dimensional arrays are supported for now!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return NULL;
   }
#endif

   EncoderBase *encoder = NULL;

   switch ( ref2->attr->type ) {
      case TRICK_VOID: {
         // No type, not supported.
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'void', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_CHARACTER: {
         // (char)
         break;
      }
      case TRICK_UNSIGNED_CHARACTER: {
         // (unsigned char)
         break;
      }
      case TRICK_STRING: {
         // string
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'string', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_SHORT: {
         // (short)
         switch ( sizeof( short ) ) {
            case 2: {
               encoder = create_int16( trick_name, hla_encoding, ref2 );
               break;
            }
            case 4:
            default: {
               encoder = create_int32( trick_name, hla_encoding, ref2 );
               break;
            }
         }
         break;
      }
      case TRICK_UNSIGNED_SHORT: {
         // (unsigned short)
         switch ( sizeof( unsigned short ) ) {
            case 2: {
               encoder = create_uint16( trick_name, hla_encoding, ref2 );
               break;
            }
            case 4:
            default: {
               encoder = create_uint32( trick_name, hla_encoding, ref2 );
               break;
            }
         }
         break;
      }
      case TRICK_INTEGER: {
         // (int)
         switch ( sizeof( int ) ) {
            case 2: {
               encoder = create_int16( trick_name, hla_encoding, ref2 );
               break;
            }
            case 4:
            default: {
               encoder = create_int32( trick_name, hla_encoding, ref2 );
               break;
            }
         }
         break;
      }
      case TRICK_UNSIGNED_INTEGER: {
         // (unsigned int)
         switch ( sizeof( unsigned int ) ) {
            case 2: {
               encoder = create_uint16( trick_name, hla_encoding, ref2 );
               break;
            }
            case 4:
            default: {
               encoder = create_uint32( trick_name, hla_encoding, ref2 );
               break;
            }
         }
         break;
      }
      case TRICK_LONG: {
         // (long)
         switch ( sizeof( long ) ) {
            case 4: {
               encoder = create_int32( trick_name, hla_encoding, ref2 );
               break;
            }
            case 8:
            default: {
               encoder = create_int64( trick_name, hla_encoding, ref2 );
               break;
            }
         }
         break;
      }
      case TRICK_UNSIGNED_LONG: {
         // (unsigned long)
         switch ( sizeof( unsigned long ) ) {
            case 4: {
               encoder = create_uint32( trick_name, hla_encoding, ref2 );
               break;
            }
            case 8:
            default: {
               encoder = create_uint64( trick_name, hla_encoding, ref2 );
               break;
            }
         }
         break;
      }
      case TRICK_FLOAT: {
         // (float)
         break;
      }
      case TRICK_DOUBLE: {
         // (double)
         break;
      }
      case TRICK_BITFIELD: {
         // (signed int : 1), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type bit-field 'int : 1', and is"
                << " not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_UNSIGNED_BITFIELD: {
         // (unsigned int : 1), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type bit-field 'unsigned int : 1',"
                << " and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_LONG_LONG: {
         // (long long)
         encoder = create_int64( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_UNSIGNED_LONG_LONG: {
         // (unsigned long long)
         encoder = create_uint64( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_FILE_PTR: {
         // (file *), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'file *', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_BOOLEAN: {
         // (bool)
         break;
      }
      case TRICK_WCHAR: {
         // (wchar_t)
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'wchar_t', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_WSTRING: {
         // std::wstring
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'std::wstring', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_VOID_PTR: {
         // An arbitrary address (void *), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'void *', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_ENUMERATED: {
         // User defined type (enumeration), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'enum', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_STRUCTURED: {
         // User defined type (struct/class), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'struct' or class, and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_OPAQUE_TYPE: {
         // User defined type (where type details are as yet unknown)
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'opaque', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_STL: {
         // stl::type, Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'stl::type', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      default: {
         // Unrecognized types are not supported.
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of unknown type ("
                << Utilities::get_trick_type_string( ref2->attr->type )
                << " = " << ref2->attr->type << "), and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }

   return encoder;
}

EncoderBase *EncoderFactory::create_int16(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN:
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int16FixedArrayEncoder( trick_name, hla_encoding, ref2 );
            } else {
               return new Int16VariableArrayEncoder( trick_name, hla_encoding, ref2 );
            }
         } else {
            return new Int16Encoder( trick_name, hla_encoding, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_int16():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'short', the specified hla_endoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_uint16(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN:
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt16FixedArrayEncoder( trick_name, hla_encoding, ref2 );
            } else {
               return new UInt16VariableArrayEncoder( trick_name, hla_encoding, ref2 );
            }
         } else {
            return new UInt16Encoder( trick_name, hla_encoding, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_uint16():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned short', the specified hla_endoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_int32(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN:
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int32FixedArrayEncoder( trick_name, hla_encoding, ref2 );
            } else {
               return new Int32VariableArrayEncoder( trick_name, hla_encoding, ref2 );
            }
         } else {
            return new Int32Encoder( trick_name, hla_encoding, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_int32():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'int', the specified hla_endoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_uint32(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN:
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt32FixedArrayEncoder( trick_name, hla_encoding, ref2 );
            } else {
               return new UInt32VariableArrayEncoder( trick_name, hla_encoding, ref2 );
            }
         } else {
            return new UInt32Encoder( trick_name, hla_encoding, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_uint32():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned int', the specified hla_endoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_int64(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN:
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int64FixedArrayEncoder( trick_name, hla_encoding, ref2 );
            } else {
               return new Int64VariableArrayEncoder( trick_name, hla_encoding, ref2 );
            }
         } else {
            return new Int64Encoder( trick_name, hla_encoding, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_int64():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'long long', the specified hla_endoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_uint64(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN:
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt64FixedArrayEncoder( trick_name, hla_encoding, ref2 );
            } else {
               return new UInt64VariableArrayEncoder( trick_name, hla_encoding, ref2 );
            }
         } else {
            return new UInt64Encoder( trick_name, hla_encoding, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_uint64():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned long long', the specified"
                << " hla_endoding (" << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}
