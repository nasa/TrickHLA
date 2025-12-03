/*!
@file TrickHLA/encoding/EncoderFactory.cpp
@ingroup TrickHLA
@brief This class represents the encoder factory implementation.

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
@trick_link_dependency{BasicDataEncoders.cpp}
@trick_link_dependency{BasicDataFixedArrayEncoders.cpp}
@trick_link_dependency{BasicDataVariableArrayEncoders.cpp}
@trick_link_dependency{CharASCIIStringEncoder.cpp}
@trick_link_dependency{CharOpaqueDataEncoder.cpp}
@trick_link_dependency{CharRawDataEncoder.cpp}
@trick_link_dependency{CharUnicodeStringEncoder.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{Float64ToLogicalTimeEncoder.cpp}
@trick_link_dependency{StringUnicodeEncoder.cpp}
@trick_link_dependency{StringUnicodeFixedArrayEncoder.cpp}
@trick_link_dependency{StringUnicodeVariableArrayEncoder.cpp}
@trick_link_dependency{VariableArrayEncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}
@trick_link_dependency{../Utilities.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/attributes.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/parameter_types.h"
#include "trick/reference.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/encoding/BasicDataEncoders.hh"
#include "TrickHLA/encoding/BasicDataFixedArrayEncoders.hh"
#include "TrickHLA/encoding/BasicDataVariableArrayEncoders.hh"
#include "TrickHLA/encoding/CharASCIIStringEncoder.hh"
#include "TrickHLA/encoding/CharOpaqueDataEncoder.hh"
#include "TrickHLA/encoding/CharRawDataEncoder.hh"
#include "TrickHLA/encoding/CharUnicodeStringEncoder.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/EncoderFactory.hh"
#include "TrickHLA/encoding/Float64ToLogicalTimeEncoder.hh"
#include "TrickHLA/encoding/StringUnicodeEncoder.hh"
#include "TrickHLA/encoding/StringUnicodeFixedArrayEncoder.hh"
#include "TrickHLA/encoding/StringUnicodeVariableArrayEncoder.hh"

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

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
             << " the variable." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return NULL;
   }

   EncoderBase *encoder = create( ref2->address,
                                  ref2->attr,
                                  hla_encoding,
                                  trick_name );
   free( ref2 );

   return encoder;
}

EncoderBase *EncoderFactory::create(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &trick_name )
{
   if ( attr == NULL ) {
      ostringstream errmsg;
      errmsg << "EncoderFactory::create():" << __LINE__
             << " ERROR: Unexpected NULL Trick attributes. Please make sure the"
             << " variable is allocated memory by the Trick Memory Manager."
             << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return NULL;
   }

   string data_name;
   if ( !trick_name.empty() ) {
      // Fully qualified Trick variable name of the form 'SimObjName.var_name'.
      data_name = trick_name;
   } else if ( attr->name != NULL ) {
      // Short variable name of the form 'var_name'.
      data_name = attr->name;
   } else {
      data_name = "";
   }

   if ( address == NULL ) {
      ostringstream errmsg;
      errmsg << "EncoderFactory::create():" << __LINE__
             << " ERROR: The variable address is NULL for variable '"
             << data_name << "'. Please make sure the Trick variable"
             << " is allocated memory by the Trick Memory Manager." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return NULL;
   }

   EncoderBase *encoder = NULL;

   switch ( attr->type ) {
      case TRICK_VOID: {
         // No type, not supported.
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'void', and is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_CHARACTER: { // NOLINT(bugprone-branch-clone)
         // (char)
         encoder = create_char_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_UNSIGNED_CHARACTER: {
         // (unsigned char)
         encoder = create_char_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_STRING: {
         // (std::string)
         encoder = create_string_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_SHORT: {
         // (short)
         encoder = create_int16_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_UNSIGNED_SHORT: {
         // (unsigned short)
#if defined( IEEE_1516_2025 )
         encoder = create_uint16_encoder( address, attr, hla_encoding, data_name );
#else
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_HLA_ENCODERS ) ) {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create():" << __LINE__
                   << " WARNING: Trick attributes for the variable '" << data_name
                   << "' is of type 'unsigned short', but the IEEE 1516-2010"
                   << " standard does not support encoding unsigned integers."
                   << " Using int16 encoder instead." << endl;
            message_publish( MSG_WARNING, errmsg.str().c_str() );
         }
         encoder = create_int16_encoder( address, attr, hla_encoding, data_name );
#endif // IEEE_1516_2025
         break;
      }
      case TRICK_INTEGER: {
         // (int)
         encoder = create_int32_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_UNSIGNED_INTEGER: {
         // (unsigned int)
#if defined( IEEE_1516_2025 )
         encoder = create_uint32_encoder( address, attr, hla_encoding, data_name );
#else
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_HLA_ENCODERS ) ) {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create():" << __LINE__
                   << " WARNING: Trick attributes for the variable '" << data_name
                   << "' is of type 'unsigned int', but the IEEE 1516-2010"
                   << " standard does not support encoding unsigned integers."
                   << " Using int32 encoder instead." << endl;
            message_publish( MSG_WARNING, errmsg.str().c_str() );
         }
         encoder = create_int32_encoder( address, attr, hla_encoding, data_name );
#endif // IEEE_1516_2025
         break;
      }
      case TRICK_LONG: {
         // (long)
         switch ( sizeof( long ) ) {
            case 4: {
               encoder = create_int32_encoder( address, attr, hla_encoding, data_name );
               break;
            }
            case 8: {
               encoder = create_int64_encoder( address, attr, hla_encoding, data_name );
               break;
            }
            default: {
               ostringstream errmsg;
               errmsg << "EncoderFactory::create():" << __LINE__
                      << " ERROR: Trick attributes for the variable '"
                      << data_name << "' is of type 'long', but has"
                      << " an unrecognized size of " << sizeof( long )
                      << " bytes." << endl;
               DebugHandler::terminate_with_message( errmsg.str() );
               break;
            }
         }
         break;
      }
      case TRICK_UNSIGNED_LONG: {
         // (unsigned long)
         switch ( sizeof( unsigned long ) ) {
            case 4: {
#if defined( IEEE_1516_2025 )
               encoder = create_uint32_encoder( address, attr, hla_encoding, data_name );
#else
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_HLA_ENCODERS ) ) {
                  ostringstream errmsg;
                  errmsg << "EncoderFactory::create():" << __LINE__
                         << " WARNING: Trick attributes for the variable '" << data_name
                         << "' is of type 'unsigned long', but the IEEE 1516-2010"
                         << " standard does not support encoding unsigned integers."
                         << " Using int32 encoder instead." << endl;
                  message_publish( MSG_WARNING, errmsg.str().c_str() );
               }
               encoder = create_int32_encoder( address, attr, hla_encoding, data_name );
#endif // IEEE_1516_2025
               break;
            }
            case 8: {
#if defined( IEEE_1516_2025 )
               encoder = create_uint64_encoder( address, attr, hla_encoding, data_name );
#else
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_HLA_ENCODERS ) ) {
                  ostringstream errmsg;
                  errmsg << "EncoderFactory::create():" << __LINE__
                         << " WARNING: Trick attributes for the variable '" << data_name
                         << "' is of type 'unsigned long', but the IEEE 1516-2010"
                         << " standard does not support encoding unsigned integers."
                         << " Using int64 encoder instead." << endl;
                  message_publish( MSG_WARNING, errmsg.str().c_str() );
               }
               encoder = create_int64_encoder( address, attr, hla_encoding, data_name );
#endif // IEEE_1516_2025
               break;
            }
            default: {
               ostringstream errmsg;
               errmsg << "EncoderFactory::create():" << __LINE__
                      << " ERROR: Trick attributes for the variable '"
                      << data_name << "' is of type 'unsigned long', but has"
                      << " an unrecognized size of " << sizeof( unsigned long )
                      << " bytes." << endl;
               DebugHandler::terminate_with_message( errmsg.str() );
               break;
            }
         }
         break;
      }
      case TRICK_FLOAT: {
         // (float)
         encoder = create_float32_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_DOUBLE: {
         // (double)
         encoder = create_float64_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_BITFIELD: {
         // (signed int : 1), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type bit-field 'int : 1', and is"
                << " not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_UNSIGNED_BITFIELD: {
         // (unsigned int : 1), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type bit-field 'unsigned int : 1',"
                << " and is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_LONG_LONG: {
         // (long long)
         encoder = create_int64_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_UNSIGNED_LONG_LONG: {
         // (unsigned long long)
#if defined( IEEE_1516_2025 )
         encoder = create_uint64_encoder( address, attr, hla_encoding, data_name );
#else
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_HLA_ENCODERS ) ) {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create():" << __LINE__
                   << " WARNING: Trick attributes for the variable '" << data_name
                   << "' is of type 'unsigned long long', but the IEEE 1516-2010"
                   << " standard does not support encoding unsigned integers."
                   << " Using int64 encoder instead." << endl;
            message_publish( MSG_WARNING, errmsg.str().c_str() );
         }
         encoder = create_int64_encoder( address, attr, hla_encoding, data_name );
#endif // IEEE_1516_2025
         break;
      }
      case TRICK_FILE_PTR: {
         // (file *), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'file *', and is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_BOOLEAN: {
         // (bool)
         encoder = create_bool_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_WCHAR: {
         // (wchar_t)
         encoder = create_wchar_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_WSTRING: {
         // std::wstring
#if defined( TRICK_WSTRING_MM_SUPPORT )
         encoder = create_wstring_encoder( address, attr, hla_encoding, data_name );
#else
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'std::wstring', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
#endif // TRICK_WSTRING_MM_SUPPORT
         break;
      }
      case TRICK_VOID_PTR: {
         // An arbitrary address (void *), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'void *', and is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_ENUMERATED: {
         // User defined type (enumeration)
         encoder = create_enum_encoder( address, attr, hla_encoding, data_name );
         break;
      }
      case TRICK_STRUCTURED: {
         // User defined type (struct/class), Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'struct' or class, and is not supported."
                << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_OPAQUE_TYPE: {
         // User defined type (where type details are as yet unknown)
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'opaque', and is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      case TRICK_STL: {
         // stl::type, Not supported
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'stl::type', and is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
      default: {
         // Unrecognized types are not supported.
         ostringstream errmsg;
         errmsg << "EncoderFactory::create():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of unknown type ("
                << trickTypeCharString( attr->type, "UNSUPPORTED_TYPE" )
                << " = " << attr->type << "), and is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }

   return encoder;
}

EncoderBase *EncoderFactory::create_char_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array         = ( attr->num_index > 0 );
   bool const is_static_array  = is_array && ( attr->index[attr->num_index - 1].size != 0 );
   bool const is_dynamic_array = is_array && ( attr->index[attr->num_index - 1].size == 0 );

   switch ( hla_encoding ) {
      case ENCODING_UNICODE_STRING: {
         if ( is_dynamic_array ) {
            return new CharUnicodeStringEncoder( address, attr, data_name );
         } else {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create_char_encoder():" << __LINE__
                   << " ERROR: Trick attributes for the variable '" << data_name
                   << "' is of type 'char' for the specified"
                   << " ENCODING_UNICODE_STRING encoding and only a dynamic"
                   << " array of characters (i.e. char *) is supported!"
                   << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }
      case ENCODING_ASCII_STRING: {
         if ( is_dynamic_array ) {
            return new CharASCIIStringEncoder( address, attr, data_name );
         } else {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create_char_encoder():" << __LINE__
                   << " ERROR: Trick attributes for the variable '" << data_name
                   << "' is of type 'char' for the specified"
                   << " ENCODING_ASCII_STRING encoding and only a dynamic"
                   << " array of characters (i.e. char *) is supported!"
                   << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }
      case ENCODING_OPAQUE_DATA: {
         if ( is_dynamic_array ) {
            return new CharOpaqueDataEncoder( address, attr, data_name );
         } else {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create_char_encoder():" << __LINE__
                   << " ERROR: Trick attributes for the variable '" << data_name
                   << "' is of type 'char' for the specified"
                   << " ENCODING_OPAQUE_DATA encoding and only a dynamic"
                   << " array of characters (i.e. char *) is supported!"
                   << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }
      case ENCODING_NONE: {
         if ( is_dynamic_array ) {
            return new CharRawDataEncoder( address, attr, data_name );
         } else {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create_char_encoder():" << __LINE__
                   << " ERROR: Trick attributes for the variable '" << data_name
                   << "' is of type 'char' for the specified"
                   << " ENCODING_NONE encoding and only a dynamic"
                   << " array of characters (i.e. char *) is supported!"
                   << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }
      case ENCODING_ASCII_CHAR: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new ASCIICharFixedArrayEncoder( address, attr, data_name );
            } else {
               return new ASCIICharVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new ASCIICharEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_char_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'char', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_string_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_ASCII_STRING: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new ASCIIStringFixedArrayEncoder( address, attr, data_name );
            } else {
               return new ASCIIStringVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new ASCIIStringEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_UNICODE_STRING: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new StringUnicodeFixedArrayEncoder( address, attr, data_name );
            } else {
               return new StringUnicodeVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new StringUnicodeEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_string_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'std::string', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_wchar_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_UNICODE_CHAR: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UnicodeCharFixedArrayEncoder( address, attr, data_name );
            } else {
               return new UnicodeCharVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new UnicodeCharEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_wchar_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'wchar', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

#if defined( TRICK_WSTRING_MM_SUPPORT )
EncoderBase *EncoderFactory::create_wstring_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_UNICODE_STRING: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UnicodeStringFixedArrayEncoder( address, attr, data_name );
            } else {
               return new UnicodeStringVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new UnicodeStringEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_wstring_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'std::wstring', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}
#endif // TRICK_WSTRING_MM_SUPPORT

EncoderBase *EncoderFactory::create_int16_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int16BEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Int16BEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Int16BEEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int16LEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Int16LEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Int16LEEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_int16_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'short', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_int32_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int32BEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Int32BEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Int32BEEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int32LEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Int32LEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Int32LEEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_int32_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'int', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_int64_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int64BEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Int64BEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Int64BEEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Int64LEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Int64LEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Int64LEEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_int64_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'long long', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

#if defined( IEEE_1516_2025 )
EncoderBase *EncoderFactory::create_uint16_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt16BEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new UInt16BEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new UInt16BEEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt16LEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new UInt16LEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new UInt16LEEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_uint16_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'unsigned short', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_uint32_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt32BEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new UInt32BEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new UInt32BEEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt32LEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new UInt32LEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new UInt32LEEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_uint32_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'unsigned int', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_uint64_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt64BEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new UInt64BEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new UInt64BEEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new UInt64LEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new UInt64LEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new UInt64LEEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_uint64_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'unsigned long long', the specified"
                << " hla_encoding (" << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}
#endif // IEEE_1516_2025

EncoderBase *EncoderFactory::create_float32_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Float32BEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Float32BEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Float32BEEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Float32LEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Float32LEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Float32LEEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_float32_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'float', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_float64_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BIG_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Float64BEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Float64BEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Float64BEEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_LITTLE_ENDIAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new Float64LEFixedArrayEncoder( address, attr, data_name );
            } else {
               return new Float64LEVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new Float64LEEncoder( address, attr, data_name );
         }
         break;
      }
      case ENCODING_LOGICAL_TIME: {
         if ( !is_array ) {
            return new Float64ToLogicalTimeEncoder( address, attr, data_name );
         } else {
            ostringstream errmsg;
            errmsg << "EncoderFactory::create_float64_encoder():" << __LINE__
                   << " ERROR: Trick attributes for the variable '" << data_name
                   << "' is an array of type 'double', the specified HLA-encoding ("
                   << encoding_enum_to_string( hla_encoding )
                   << ") is only supported for a primitive double value"
                   << " (i.e. not an array)." << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_float64_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'double', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_bool_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   bool const is_array        = ( attr->num_index > 0 );
   bool const is_static_array = is_array && ( attr->index[attr->num_index - 1].size != 0 );

   switch ( hla_encoding ) {
      case ENCODING_BOOLEAN: {
         if ( is_array ) {
            if ( is_static_array ) {
               return new BoolFixedArrayEncoder( address, attr, data_name );
            } else {
               return new BoolVariableArrayEncoder( address, attr, data_name );
            }
         } else {
            return new BoolEncoder( address, attr, data_name );
         }
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_bool_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type 'bool', the specified HLA-encoding ("
                << encoding_enum_to_string( hla_encoding )
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
   return NULL;
}

EncoderBase *EncoderFactory::create_enum_encoder(
   void              *address,
   ATTRIBUTES        *attr,
   EncodingEnum const hla_encoding,
   string const      &data_name )
{
   switch ( attr->size ) {
      case 1: {
         return create_char_encoder( address, attr, ENCODING_ASCII_CHAR, data_name );
      }
      case 2: {
         return create_int16_encoder( address, attr, hla_encoding, data_name );
      }
      case 4: {
         return create_int32_encoder( address, attr, hla_encoding, data_name );
      }
      case 8: {
         return create_int64_encoder( address, attr, hla_encoding, data_name );
      }
      default: {
         ostringstream errmsg;
         errmsg << "EncoderFactory::create_enum_encoder():" << __LINE__
                << " ERROR: Trick attributes for the variable '" << data_name
                << "' is of type TRICK_ENUMERATED (" << attr->type_name
                << "), but the specified size (" << attr->size
                << ") is not supported." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
   return NULL;
}
