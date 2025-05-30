/*!
@file TrickHLA/encoding/EncoderFactory.cpp
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
@trick_link_dependency{BasicDataEncoders.cpp}
@trick_link_dependency{BasicDataFixedArrayEncoders.cpp}
@trick_link_dependency{BasicDataVariableArrayEncoders.cpp}
@trick_link_dependency{CharASCIIStringEncoder.cpp}
@trick_link_dependency{CharOpaqueDataEncoder.cpp}
@trick_link_dependency{CharUnicodeStringEncoder.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}
@trick_link_dependency{../Utilities.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/BasicDataEncoders.hh"
#include "TrickHLA/encoding/BasicDataFixedArrayEncoders.hh"
#include "TrickHLA/encoding/BasicDataVariableArrayEncoders.hh"
#include "TrickHLA/encoding/CharASCIIStringEncoder.hh"
#include "TrickHLA/encoding/CharOpaqueDataEncoder.hh"
#include "TrickHLA/encoding/CharUnicodeStringEncoder.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/EncoderFactory.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

EncoderFactory::EncoderFactory()
{
   return;
}

EncoderFactory::~EncoderFactory()
{
   return;
}

EncoderBase *EncoderFactory::create(
   string const      &trick_name,
   EncodingEnum const hla_encoding )
{
   // The individual encoders are responsible for deleting ref2.
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
         encoder = create_char_encoder( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_UNSIGNED_CHARACTER: {
         // (unsigned char)
#if defined( IEEE_1516_2025 )
         encoder = create_uchar_encoder( trick_name, hla_encoding, ref2 );
#else
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned char', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
#endif
         break;
      }
      case TRICK_STRING: {
         // (std::string)
         encoder = create_string_encoder( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_SHORT: {
         // (short)
         encoder = create_int16_encoder( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_UNSIGNED_SHORT: {
         // (unsigned short)
#if defined( IEEE_1516_2025 )
         encoder = create_uint16_encoder( trick_name, hla_encoding, ref2 );
#else
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned short', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
#endif
         break;
      }
      case TRICK_INTEGER: {
         // (int)
         encoder = create_int32_encoder( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_UNSIGNED_INTEGER: {
         // (unsigned int)
#if defined( IEEE_1516_2025 )
         encoder = create_uint32_encoder( trick_name, hla_encoding, ref2 );
#else
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned int', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
#endif
         break;
      }
      case TRICK_LONG: {
         // (long)
         switch ( sizeof( long ) ) {
            case 4: {
               encoder = create_int32_encoder( trick_name, hla_encoding, ref2 );
               break;
            }
            case 8:
            default: {
               encoder = create_int64_encoder( trick_name, hla_encoding, ref2 );
               break;
            }
         }
         break;
      }
      case TRICK_UNSIGNED_LONG: {
         // (unsigned long)
#if defined( IEEE_1516_2025 )
         switch ( sizeof( unsigned long ) ) {
            case 4: {
               encoder = create_uint32_encoder( trick_name, hla_encoding, ref2 );
               break;
            }
            case 8:
            default: {
               encoder = create_uint64_encoder( trick_name, hla_encoding, ref2 );
               break;
            }
         }
#else
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned long', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
#endif
         break;
      }
      case TRICK_FLOAT: {
         // (float)
         encoder = create_float32_encoder( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_DOUBLE: {
         // (double)
         encoder = create_float64_encoder( trick_name, hla_encoding, ref2 );
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
         encoder = create_int64_encoder( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_UNSIGNED_LONG_LONG: {
         // (unsigned long long)
#if defined( IEEE_1516_2025 )
         encoder = create_uint64_encoder( trick_name, hla_encoding, ref2 );
#else
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned long long', and is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
#endif
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
         encoder = create_bool_encoder( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_WCHAR: {
         // (wchar_t)
         encoder = create_wchar_encoder( trick_name, hla_encoding, ref2 );
         break;
      }
      case TRICK_WSTRING: {
         // std::wstring
         encoder = create_wstring_encoder( trick_name, hla_encoding, ref2 );
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

EncoderBase *EncoderFactory::create_char_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array         = ( ref2->attr->num_index > 0 );
   bool const is_static_array  = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );
   bool const is_dynamic_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 );

   switch ( hla_encoding ) {
      case ENCODING_NONE: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new ByteFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new ByteVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new ByteEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_ASCII_CHAR: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new ASCIICharFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new ASCIICharVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new ASCIICharEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_ASCII_STRING: {
         if ( is_dynamic_array ) {
            return new CharASCIIStringEncoder( trick_name, ref2 );
         } else {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create_char_encoder():" << __LINE__
                   << " ERROR: Trick ref-attributes for '" << trick_name
                   << "' the Trick variable is of type 'char' for the specified"
                   << " ENCODING_ASCII_STRING encoding and only a dynamic"
                   << " array of characters (i.e. char *) is supported!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }
      case ENCODING_UNICODE_STRING: {
         if ( is_dynamic_array ) {
            return new CharUnicodeStringEncoder( trick_name, ref2 );
         } else {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create_char_encoder():" << __LINE__
                   << " ERROR: Trick ref-attributes for '" << trick_name
                   << "' the Trick variable is of type 'char' for the specified"
                   << " ENCODING_UNICODE_STRING encoding and only a dynamic"
                   << " array of characters (i.e. char *) is supported!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }
      case ENCODING_OPAQUE_DATA: {
         if ( is_dynamic_array ) {
            return new CharOpaqueDataEncoder( trick_name, ref2 );
         } else {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create_char_encoder():" << __LINE__
                   << " ERROR: Trick ref-attributes for '" << trick_name
                   << "' the Trick variable is of type 'char' for the specified"
                   << " ENCODING_OPAQUE_DATA encoding and only a dynamic"
                   << " array of characters (i.e. char *) is supported!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_char_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'char', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_string_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_ASCII_STRING: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new ASCIIStringFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new ASCIIStringVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new ASCIIStringEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_string_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'std::string', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_wchar_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_UNICODE_CHAR: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UnicodeCharFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new UnicodeCharVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new UnicodeCharEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_wchar_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'wchar', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_wstring_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
#if defined( TRICK_WSTRING_MM_SUPPORT )
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_UNICODE_STRING: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UnicodeStringFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new UnicodeStringVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new UnicodeStringEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_wstring_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'std::wstring', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
#else
   ostringstream errmsg;
   errmsg << "EncoderFactory::create_wstring_encoder():" << __LINE__
          << " ERROR: Trick ref-attributes for '" << trick_name
          << "' the variable is of type 'std::wstring', the specified HLA-encoding ("
          << hla_encoding << ") is not supported.\n";
   DebugHandler::terminate_with_message( errmsg.str() );
#endif // TRICK_WSTRING_MM_SUPPORT

   return NULL;
}

EncoderBase *EncoderFactory::create_int16_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int16BEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Int16BEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Int16BEEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int16LEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Int16LEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Int16LEEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_int16_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'short', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_int32_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int32BEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Int32BEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Int32BEEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int32LEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Int32LEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Int32LEEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_int32_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'int', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_int64_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int64BEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Int64BEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Int64BEEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int64LEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Int64LEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Int64LEEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_int64_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'long long', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

#if defined( IEEE_1516_2025 )
EncoderBase *EncoderFactory::create_uint16_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt16BEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new UInt16BEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new UInt16BEEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt16LEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new UInt16LEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new UInt16LEEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_uint16_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned short', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_uint32_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt32BEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new UInt32BEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new UInt32BEEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt32LEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new UInt32LEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new UInt32LEEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_uint32_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned int', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_uint64_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt64BEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new UInt64BEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new UInt64BEEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt64LEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new UInt64LEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new UInt64LEEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_uint64_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'unsigned long long', the specified"
                << " hla_encoding (" << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}
#endif

EncoderBase *EncoderFactory::create_float32_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Float32BEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Float32BEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Float32BEEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Float32LEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Float32LEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Float32LEEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_float32_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'float', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_float64_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Float64BEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Float64BEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Float64BEEncoder( trick_name, ref2 );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Float64LEFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new Float64LEVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new Float64LEEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_float64_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'double', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_bool_encoder(
   string const      &trick_name,
   EncodingEnum const hla_encoding,
   REF2              *ref2 )
{
   bool const is_array        = ( ref2->attr->num_index > 0 );
   bool const is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BOOLEAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new BoolFixedArrayEncoder( trick_name, ref2 );
            } else {
               return new BoolVariableArrayEncoder( trick_name, ref2 );
            }
         } else {
            return new BoolEncoder( trick_name, ref2 );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_bool_encoder():" << __LINE__
                << " ERROR: Trick ref-attributes for '" << trick_name
                << "' the variable is of type 'bool', the specified HLA-encoding ("
                << hla_encoding << ") is not supported.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}
