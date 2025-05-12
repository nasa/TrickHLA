/*!
@file TrickHLA/EncoderBase.cpp
@ingroup TrickHLA
@brief This class represents the base encoder implementation.

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

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/encoding/DataElement.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details The endianess of the computer is determined as part of the
 * EncoderBase construction process.
 * @job_class{initialization}
 */
EncoderBase::EncoderBase(
   string const      &trick_variable_name,
   EncodingEnum const hla_encoding,
   REF2              *r2 )
   : trick_name( trick_variable_name ),
     rti_encoding( hla_encoding ),
     ref2( r2 ),
     ref2_byte_count( 0 ),
     is_array( false ),
     is_1d_array( false ),
     is_static_array( false ),
     is_dynamic_array( false ),
     is_static_in_size( false ),
     buffer( NULL ),
     buffer_capacity( 0 ),
     buffer_size( 0 ),
     encoder( NULL )
{
   EncoderBase::initialize();
}

/*!
 * @details The buffer and ref2 values are freed and nulled.
 * @job_class{shutdown}
 */
EncoderBase::~EncoderBase()
{
   if ( ref2 != NULL ) {
      free( ref2 );
      ref2 = NULL;
   }
}

void EncoderBase::initialize()
{
   if ( ref2 == NULL ) {
      ref2 = ref_attributes( trick_name.c_str() );

      // Determine if we had an error getting the ref-attributes.
      if ( ref2 == NULL ) {
         ostringstream errmsg;
         errmsg << "EncoderBase::initialize():" << __LINE__
                << " ERROR: Error retrieving Trick ref-attributes for '"
                << trick_name << "'. Please check your input or modified-data"
                << " files to make sure the object attribute Trick name is"
                << " correctly specified. If '" << trick_name
                << "' is an inherited variable then make sure the base class"
                << " uses either the 'public' or 'protected' access level for"
                << " the variable.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }
   }

   // For now, we do not support more than a 1-D array that is dynamic
   // (i.e. a pointer such as char *). If the size of the last indexed
   // attribute is zero then it is a pointer and not static.
   is_array          = ( ref2->attr->num_index > 0 );
   is_1d_array       = ( ref2->attr->num_index == 1 );
   is_static_array   = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );
   is_dynamic_array  = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 );
   is_static_in_size = !is_array || is_static_array;

   ref2_byte_count = calculate_size_in_bytes();
   ensure_buffer_capacity( ref2_byte_count );
}

/*!
 * @brief Ensure the attribute buffer has at least the specified capacity.
 *  @param capacity Desired capacity of the buffer in bytes.
 */
void EncoderBase::ensure_buffer_capacity(
   int const capacity )
{
   if ( buffer == NULL ) {
      // Handle the case where the buffer has not been created yet and we
      // might have an invalid capacity specified.

      // Make sure the capacity is at least 1.
      buffer_capacity = ( capacity > 0 ) ? capacity : 1;

      buffer = static_cast< unsigned char * >( TMM_declare_var_1d( "unsigned char", buffer_capacity ) );

      if ( buffer == NULL ) {
         ostringstream errmsg;
         errmsg << "EncoderBase::ensure_buffer_capacity():" << __LINE__
                << " ERROR: Could not allocate memory for buffer for requested"
                << " capacity " << capacity << " for FOM-name '" << FOM_name
                << "' with Trick name '" << trick_name << "'!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   } else if ( capacity > buffer_capacity ) {
      // Only resize to a larger capacity.
      buffer_capacity = capacity;

      buffer = static_cast< unsigned char * >( TMM_resize_array_1d_a( buffer, buffer_capacity ) );

      if ( buffer == NULL ) {
         ostringstream errmsg;
         errmsg << "EncoderBase::ensure_buffer_capacity():" << __LINE__
                << " ERROR: Could not resize memory for buffer for requested"
                << " capacity " << capacity << " for FOM-name '" << FOM_name
                << "' with Trick name '" << trick_name << "'!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}

size_t const EncoderBase::calculate_size_in_bytes()
{
   size_t byte_count;

   if ( is_static_in_size && ( ref2_byte_count > 0 ) ) {
      byte_count = ref2_byte_count;
   } else {

      if ( is_dynamic_array ) {
         // We have a multi-dimension array that is a pointer and the
         // number of dimensions is ref2->attr->num_index

         // TODO: Need to refresh ref2 since the variable is dynamic.

         // Handle dynamic arrays of characters differently since we need to know the
         // length of each string.
         if ( ( ref2->attr->type == TRICK_CHARACTER )
              || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) ) {

            byte_count = 0;

            switch ( rti_encoding ) {
               case ENCODING_OPAQUE_DATA:
               case ENCODING_NONE: {
                  // Determine total number of bytes used by the Trick simulation
                  // variable, and the data can be binary and not just the printable
                  // ASCII characters.
                  size_t index = 0;
                  for ( int i = 0; i < ref2->attr->num_index; ++i ) {
                     for ( int k = 0; k < ref2->attr->index[i].size; ++k ) {
                        char *s = *( static_cast< char ** >( ref2->address ) + index );
                        ++index;
                        if ( s != NULL ) {
                           int size = get_size( s );
                           if ( size > 0 ) {
                              byte_count += size;
                           }
                        }
                     }
                  }
                  break;
               }
               default: {
                  // For the ENCODING_C_STRING, ENCODING_UNICODE_STRING,
                  // and ENCODING_ASCII_STRING encodings assume the string is
                  // terminated with a null character and determine the number of
                  // characters using strlen().
                  size_t index = 0;
                  for ( int i = 0; i < ref2->attr->num_index; ++i ) {
                     for ( int k = 0; k < ref2->attr->index[i].size; ++k ) {
                        char const *s = *( static_cast< char ** >( ref2->address ) + index );
                        if ( s != NULL ) {
                           byte_count += strlen( s );
                        }
                        ++index;
                     }
                  }
                  break;
               }
            }
         } else {
            // Handle other dynamic arrays for non-character types.
#if 1
            // get_size returns the number of elements in the dynamic array.
            int const num_items = get_size( *static_cast< void ** >( ref2->address ) );
            byte_count          = ( num_items > 0 ) ? ( ref2->attr->size * num_items ) : 0;
#else
            size_t num_items = 1;
            for ( int i = 0; i < ref2->attr->num_index; ++i ) {
               if ( ref2->attr->index[i].size > 0 ) {
                  num_items *= ref2->attr->index[i].size;
               } else {
                  void *ptr;
                  if ( num_items == 1 ) {
                     ptr = *static_cast< void ** >( ref2->address );
                  } else {
                     ptr = *( static_cast< void ** >( ref2->address )
                              + ( num_items * ref2->attr->size ) );
                  }
                  if ( ptr != NULL ) {
                     int length = get_size( ptr );
                     if ( length > 0 ) {
                        num_items *= length;
                     }
                  }
               }
            }
            byte_count = ref2->attr->size * num_items;
#endif
         }
      } else {
         // The user variable is either a primitive type or a static
         // multi-dimension array.
         size_t num_items = 1;
         for ( int i = 0; i < ref2->attr->num_index; ++i ) {
            if ( ref2->attr->index[i].size > 0 ) {
               num_items *= ref2->attr->index[i].size;
            }
         }
         byte_count = ref2->attr->size * num_items;
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      ostringstream msg;
      msg << "EncoderBase::calculate_size_in_bytes():" << __LINE__ << '\n'
          << "========================================================\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << trick_name << "'\n"
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  ref2->attr->name:'" << ref2->attr->name << "'\n"
          << "  ref2->attr->type_name:'" << ref2->attr->type_name << "'\n"
          << "  ref2->attr->type:" << ref2->attr->type << '\n'
          << "  ref2->attr->units:" << ref2->attr->units << '\n'
          << "  ref2_byte_count:" << ref2_byte_count << '\n'
          << "  byte_count:" << byte_count << '\n';
      if ( is_array ) {
         msg << "  get_size(*(void **)ref2->address):" << get_size( *static_cast< void ** >( ref2->address ) ) << '\n';
      } else {
         msg << "  get_size(ref2->address):" << get_size( ref2->address ) << '\n';
      }
      msg << "  ref2->attr->size:" << ref2->attr->size << '\n'
          << "  ref2->attr->num_index:" << ref2->attr->num_index << '\n';
      for ( int i = 0; i < ref2->attr->num_index; ++i ) {
         msg << "  ref2->attr->index[" << i << "].size:" << ref2->attr->index[i].size << '\n';
      }
      msg << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  buffer_size:" << buffer_size << '\n'
          << "  is_array:" << ( is_array ? "Yes" : "No" ) << '\n'
          << "  is_1d_array:" << ( is_1d_array ? "Yes" : "No" ) << '\n'
          << "  is_static_array:" << ( is_static_array ? "Yes" : "No" ) << '\n'
          << "  is_dynamic_array:" << ( is_dynamic_array ? "Yes" : "No" ) << '\n';
      if ( is_dynamic_array && ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( ref2->address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return byte_count;
}
