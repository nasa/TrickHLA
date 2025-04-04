/*!
@file TrickHLA/Attribute.cpp
@ingroup TrickHLA
@brief This class represents the HLA attributes of an object that is managed
by Trick.

\par<b>Assumptions and Limitations:</b>
- Only primitive types and static arrays of primitive type are supported for now.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{Attribute.cpp}
@trick_link_dependency{Conditional.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2006, --, Initial version.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
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
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

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
 * Attribute construction process.
 * @job_class{initialization}
 */
Attribute::Attribute()
   : trick_name( NULL ),
     FOM_name( NULL ),
     config( CONFIG_NONE ),
     preferred_order( TRANSPORT_SPECIFIED_IN_FOM ),
     publish( false ),
     subscribe( false ),
     locally_owned( false ),
     rti_encoding( ENCODING_UNKNOWN ),
     cycle_time( -std::numeric_limits< double >::max() ),
     buffer( NULL ),
     buffer_capacity( 0 ),
     size_is_static( true ),
     size( 0 ),
     num_items( 0 ),
     value_changed( false ),
     update_requested( false ),
     byteswap( false ),
     cycle_ratio( 1 ),
     cycle_cnt( 0 ),
     ref2( NULL ),
     pull_requested( false ),
     push_requested( false ),
     divest_requested( false ),
     initialized( false )
{
   // The value is set based on the Endianness of this computer.
   // HLAtrue is a value of 1 on a Big Endian computer.
   HLAtrue = ( ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) ? 1 : 0x1000000 );
}

/*!
 * @details The buffer and ref2 values are freed and nulled.
 * @job_class{shutdown}
 */
Attribute::~Attribute()
{
   if ( buffer != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( buffer ) ) ) {
         message_publish( MSG_WARNING, "Attribute::~Attribute():%d WARNING failed to delete Trick Memory for 'buffer'\n",
                          __LINE__ );
      }
      buffer          = NULL;
      buffer_capacity = 0;
   }

   if ( ref2 != NULL ) {
      free( ref2 );
      ref2 = NULL;
   }
}

void Attribute::initialize(
   char const *obj_FOM_name,
   int const   object_index,
   int const   attribute_index )
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Make sure we have a valid Object FOM name.
   if ( ( obj_FOM_name == NULL ) || ( *obj_FOM_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: Unexpected NULL Object FOM-Name argument passed to this"
             << " function.";
      if ( FOM_name != NULL ) {
         errmsg << " For FOM Attribute Named '" << FOM_name << "'.";
      }
      errmsg << '\n';
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make sure we have a valid attribute FOM name.
   if ( ( FOM_name == NULL ) || ( *FOM_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: Object with FOM Name '" << obj_FOM_name << "' has a missing"
             << " FOM name for the attribute. Make sure THLA.manager.objects["
             << object_index << "].attributes[" << attribute_index
             << "].FOM_name' in either your input.py file or modified-data files"
             << " is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make sure we have a valid attribute Trick-Name.
   if ( ( trick_name == NULL ) || ( *trick_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '"
             << obj_FOM_name << "'->'" << FOM_name << "' has a missing Trick name"
             << " for the attribute. Make sure THLA.manager.objects["
             << object_index << "].attributes[" << attribute_index
             << "].trick_name' in either your input.py file or modified-data files"
             << " is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Do a quick bounds check on the 'rti_encoding' value.
   if ( ( rti_encoding < ENCODING_FIRST_VALUE ) || ( rti_encoding > ENCODING_LAST_VALUE ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '"
             << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
             << trick_name << "' has an 'rti_encoding' value of " << rti_encoding
             << " which is out of the valid range of " << ENCODING_FIRST_VALUE
             << " to " << ENCODING_LAST_VALUE << ". Please check your input or"
             << " modified-data files to make sure the value for the 'rti_encoding'"
             << " is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Do a quick bounds check on the 'preferred_order' value.
   if ( ( preferred_order < TRANSPORT_FIRST_VALUE ) || ( preferred_order > TRANSPORT_LAST_VALUE ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '"
             << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
             << trick_name << "' has an invalid 'preferred_order' and it must be"
             << " one of TRANSPORT_TYPE_SPECIFIED_IN_FOM, THLA_TIMESTAMP_ORDER or"
             << " THLA_RECEIVE_ORDER. Please check your input or modified-data"
             << " files to make sure the 'preferred_order' is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Do a bounds check on the 'config' value.
   if ( ( config < CONFIG_NONE ) || ( config > CONFIG_MAX_VALUE ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '"
             << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
             << trick_name << "' has a 'config' value of " << config
             << " which is out of the valid range of " << CONFIG_NONE
             << " to " << CONFIG_MAX_VALUE << ". Please check your input or"
             << " modified-data files to make sure the value for 'config'"
             << " is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Warn the user if the object attribute has a CONFIG_TYPE_NONE configuration.
   if ( config == CONFIG_NONE ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
         ostringstream errmsg;
         errmsg << "Attribute::initialize():" << __LINE__
                << " WARNING: FOM Object Attribute '"
                << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                << trick_name << "' has a 'config' value of CONFIG_TYPE_NONE.\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }

   // Do a bounds check on the 'cycle_time' value. Once we have a valid
   // job-cycle-time we will do another bounds check against that value.
   if ( ( this->cycle_time <= 0.0 ) && ( this->cycle_time > -std::numeric_limits< double >::max() ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '" << obj_FOM_name << "'->'" << FOM_name
             << "' with Trick name '" << trick_name << "' has a 'cycle_time' value"
             << " of " << this->cycle_time << " seconds, which is not valid. The"
             << " 'cycle_time' must be > 0. Please check your input or"
             << " modified-data files to make sure the value for the 'cycle_time'"
             << " is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the ref-attributes for the given trick-name.
   if ( ref2 == NULL ) {
      ref2 = ref_attributes( trick_name );
   }

   // Determine if we had an error getting the ref-attributes.
   if ( ref2 == NULL ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '"
             << obj_FOM_name << "'->'" << FOM_name << "', Error retrieving Trick"
             << " ref-attributes for '" << trick_name << "'. Please check"
             << " your input or modified-data files to make sure the object"
             << " attribute Trick name is correctly specified. If '"
             << trick_name << "' is an inherited variable then make"
             << " sure the base class uses either the 'public' or 'protected'"
             << " access level for the variable.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify that the rti_encoding value is valid given the ref-attributes type.
   switch ( ref2->attr->type ) { // cppcheck-suppress [nullPointerRedundantCheck,unmatchedSuppression]
      case TRICK_BOOLEAN: {
         if ( ( rti_encoding != ENCODING_BIG_ENDIAN )
              && ( rti_encoding != ENCODING_LITTLE_ENDIAN )
              && ( rti_encoding != ENCODING_BOOLEAN )
              && ( rti_encoding != ENCODING_NONE )
              && ( rti_encoding != ENCODING_UNKNOWN ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::initialize():" << __LINE__
                   << " ERROR: FOM Object Attribute '"
                   << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' must use either the ENCODING_BIG_ENDIAN, "
                   << "ENCODING_LITTLE_ENDIAN, ENCODING_BOOLEAN, ENCODING_NONE, or "
                   << "ENCODING_UNKNOWN value for the 'rti_encoding' when the "
                   << "attribute represents a 'bool' type. Please check your input "
                   << "or modified-data files to make sure the value for the 'rti_"
                   << "encoding' is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }

      case TRICK_CHARACTER:
      case TRICK_UNSIGNED_CHARACTER: {
         if ( ( rti_encoding != ENCODING_BIG_ENDIAN )
              && ( rti_encoding != ENCODING_LITTLE_ENDIAN )
              && ( rti_encoding != ENCODING_NONE )
              && ( rti_encoding != ENCODING_UNICODE_STRING )
              && ( rti_encoding != ENCODING_OPAQUE_DATA )
              && ( rti_encoding != ENCODING_UNKNOWN ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::initialize():" << __LINE__
                   << " ERROR: FOM Object Attribute '"
                   << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' must use either the ENCODING_BIG_ENDIAN,"
                   << " ENCODING_LITTLE_ENDIAN, ENCODING_NONE, ENCODING_UNICODE_STRING,"
                   << " ENCODING_OPAQUE_DATA, or ENCODING_UNKNOWN value for the"
                   << " 'rti_encoding' when the attribute represents a 'char' or"
                   << " 'unsigned char' type. Please check  your input or"
                   << " modified-data files to make sure the value for the"
                   << " 'rti_encoding' is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // For the ENCODING_UNICODE_STRING encoding, we only support a 1-D dynamic
         // array of characters.
         if ( ( rti_encoding == ENCODING_UNICODE_STRING )
              && ( ( ref2->attr->num_index != 1 ) || ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 ) ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::initialize():" << __LINE__
                   << " ERROR: FOM Object Attribute '"
                   << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' and 'rti_encoding' of ENCODING_UNICODE_STRING must"
                   << " represent a one-dimensional array of characters (i.e."
                   << " 'char *' or 'unsigned char *'). Please check your input or"
                   << " modified-data files to make sure the value for the"
                   << " 'rti_encoding' is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // For the ENCODING_OPAQUE_DATA encoding, we only support a
         // 1-D dynamic array of characters.
         if ( ( rti_encoding == ENCODING_OPAQUE_DATA )
              && ( ( ref2->attr->num_index != 1 ) || ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 ) ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::initialize():" << __LINE__
                   << " ERROR: FOM Object Attribute '"
                   << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' and 'rti_encoding' of ENCODING_OPAQUE_DATA must"
                   << " represent a one-dimensional array of characters (i.e."
                   << " 'char *' or 'unsigned char *'). Please check your input or"
                   << " modified-data files to make sure the value for the"
                   << " 'rti_encoding' is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }

      case TRICK_DOUBLE:
      case TRICK_FLOAT:
      case TRICK_SHORT:
      case TRICK_UNSIGNED_SHORT:
      case TRICK_INTEGER:
      case TRICK_UNSIGNED_INTEGER:
      case TRICK_LONG:
      case TRICK_UNSIGNED_LONG:
      case TRICK_LONG_LONG:
      case TRICK_UNSIGNED_LONG_LONG: {
         if ( ( rti_encoding != ENCODING_BIG_ENDIAN )
              && ( rti_encoding != ENCODING_LITTLE_ENDIAN )
              && ( rti_encoding != ENCODING_LOGICAL_TIME )
              && ( rti_encoding != ENCODING_NONE )
              && ( rti_encoding != ENCODING_UNKNOWN ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::initialize():" << __LINE__
                   << " ERROR: FOM Object Attribute '"
                   << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' must use either the ENCODING_LOGICAL_TIME, "
                   << "ENCODING_BIG_ENDIAN, ENCODING_LITTLE_ENDIAN, ENCODING_NONE, or "
                   << "ENCODING_UNKNOWN value for the 'rti_encoding' when the "
                   << "attribute represents a primitive type. Please check your "
                   << "input or modified-data files to make sure the value for the "
                   << "'rti_encoding' is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }

      case TRICK_STRING: {
         if ( ( rti_encoding != ENCODING_C_STRING )
              && ( rti_encoding != ENCODING_UNICODE_STRING )
              && ( rti_encoding != ENCODING_ASCII_STRING )
              && ( rti_encoding != ENCODING_OPAQUE_DATA )
              && ( rti_encoding != ENCODING_NONE )
              && ( rti_encoding != ENCODING_UNKNOWN ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::initialize():" << __LINE__
                   << " ERROR: FOM Object Attribute '"
                   << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' must use either the ENCODING_C_STRING, "
                   << "ENCODING_UNICODE_STRING, ENCODING_ASCII_STRING, ENCODING_OPAQUE_DATA, "
                   << "ENCODING_NONE, or ENCODING_UNKNOWN value for the "
                   << "'rti_encoding' when the attribute represents a String type "
                   << "(i.e. char *). Please check your input or modified-data "
                   << "files to make sure the value for the 'rti_encoding' is "
                   << "correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // Only support an array of characters (i.e. char *) for ENCODING_NONE.
         if ( ( rti_encoding == ENCODING_NONE ) && ( ref2->attr->num_index != 0 ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::initialize():" << __LINE__
                   << " ERROR: FOM Object Attribute '"
                   << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' and 'rti_encoding' of ENCODING_NONE must"
                   << " represent a one-dimensional array of characters (i.e."
                   << " 'char *'). Please check your input or modified-data"
                   << " files to make sure the value for the 'rti_encoding' is"
                   << " correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         break;
      }
      default:
         // Do nothing for other Trick types.
         break;
   }

   // Allow 1-D dynamic arrays of primitive types such as 'char *',
   // 'unsigned char *' and 'bool *' etc.
   if ( ref2->attr->type != TRICK_STRING ) {
      // For now, we do not support more than a 1-D array that is dynamic
      // (i.e. a pointer such as char *). If the size of the last indexed
      // attribute is zero then it is a pointer.
      if ( ( ref2->attr->num_index > 1 ) && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) {
         ostringstream errmsg;
         errmsg << "Attribute::initialize():" << __LINE__
                << " ERROR: FOM Object Attribute '"
                << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                << trick_name << "' and type '" << ref2->attr->type_name
                << "' is a " << ref2->attr->num_index << "-dimensional dynamic array."
                << " Only one-dimensional dynamic arrays are supported for now.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   } else {
      // String type:
      // We only support static arrays for now so check for a pointer to an
      // array (i.e. non-static arrays) which is when we have an array
      // (i.e. num_index > 0) and the size of any of the dimensions is zero
      // (i.e. index[i].size == 0).
      if ( ref2->attr->num_index > 0 ) {
         // Make sure each dimension is statically defined (i.e. not zero).
         for ( int i = ref2->attr->num_index - 1; i >= 0; --i ) {
            // If the size is zero then the array is dynamic in size (i.e. a pointer)
            if ( ref2->attr->index[i].size == 0 ) {
               ostringstream errmsg;
               errmsg << "Attribute::initialize():" << __LINE__
                      << " ERROR: FOM Object Attribute '"
                      << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                      << trick_name << "' is a " << ( ref2->attr->num_index + 1 )
                      << "-dimensional dynamic array of strings. Only"
                      << " one-dimensional dynamic arrays are supported for now.\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }
      }
   }

   // Determine if the Object Attribute type is supported.
   if ( !is_supported_attribute_type() ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: Unsupported Type and/or"
             << " rti_encoding for FOM Object Attribute '" << obj_FOM_name
             << "'->'" << FOM_name << "' with Trick name '" << trick_name
             << "' and rti_encoding = " << rti_encoding << ".\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // We do not support an array of primitive types for the logical
   // time encoding, otherwise we support everything else.
   if ( ( rti_encoding == ENCODING_LOGICAL_TIME ) && ( ref2->attr->num_index > 0 ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '"
             << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
             << trick_name << "' can not be an array when using ENCODING_LOGICAL_TIME"
             << " for the 'rti_encoding'. Please check your input or modified-data"
             << " files to make sure the value for the 'rti_encoding' is"
             << " correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // The units must be in seconds if the ENCODING_LOGICAL_TIME is used.
   if ( ( rti_encoding == ENCODING_LOGICAL_TIME ) && ( strcmp( "s", ref2->attr->units ) != 0 ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '"
             << obj_FOM_name << "'->'" << FOM_name << "' with Trick name '"
             << trick_name << "' must have the units of 'seconds' when the"
             << " 'rti_encoding' is set to ENCODING_LOGICAL_TIME. Please check your"
             << " input or modified-data files to make sure the value for the"
             << " 'rti_encoding' is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine if we need to do a byteswap for data transmission.
   byteswap = Utilities::is_transmission_byteswap( rti_encoding );

   // Determine if the size of this attribute is static or dynamic.
   size_is_static = is_static_in_size();

   // Get the attribute size and number of items.
   // However, do not re-initialize an attribute which was loaded
   // from a checkpoint (already in an initialized state).
   if ( !this->initialized ) {
      calculate_size_and_number_of_items();
      this->initialized = true;
   }

   // Ensure enough buffer capacity for the attribute.
   ensure_buffer_capacity( size );

   // Check to make sure the users simulation variable has memory allocated to
   // it. It could be that the users simulation variable happens to be pointing
   // to a null string.
   if ( ( size == 0 )
        && ( ref2->attr->type != TRICK_STRING )
        && !( ( ref2->attr->type == TRICK_CHARACTER ) && ( ref2->attr->num_index > 0 ) ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
         ostringstream msg;
         msg << "Attribute::initialize():" << __LINE__
             << " WARNING: FOM Object Attribute '" << obj_FOM_name << "'->'"
             << FOM_name << "' with Trick name '" << trick_name
             << "' has an unexpected size of zero bytes! Make sure your simulation"
             << " variable is properly initialized before the initialize()"
             << " function is called.\n";
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      string attr_handle_string;
      StringUtilities::to_string( attr_handle_string, this->attr_handle );
      ostringstream msg;
      msg << "Attribute::initialize():" << __LINE__ << '\n'
          << "========================================================\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
          << "  AttributeHandle:" << attr_handle_string << '\n'
          << "  ref2->attr->name:'" << ref2->attr->name << "'\n"
          << "  ref2->attr->type_name:'" << ref2->attr->type_name << "'\n"
          << "  ref2->attr->type:" << ref2->attr->type << '\n'
          << "  ref2->attr->units:" << ref2->attr->units << '\n'
          << "  size:" << size << '\n'
          << "  num_items:" << num_items << '\n';
      // TODO: Figure out get_size_from_attributes() API in Trick 10.
      //   << "  get_size_from_attributes():" << get_size_from_attributes(ref2->attr, ref2->attr->name) << endl
      msg << "  ref2->attr->size:" << ref2->attr->size << '\n'
          << "  ref2->attr->num_index:" << ref2->attr->num_index << '\n'
          << "  ref2->attr->index[0].size:" << ( ref2->attr->num_index >= 1 ? ref2->attr->index[0].size : 0 ) << '\n'
          << "  publish:" << publish << '\n'
          << "  subscribe:" << subscribe << '\n'
          << "  locally_owned:" << locally_owned << '\n'
          << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
          << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( ref2->attr->type == TRICK_STRING )
           || ( ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( ref2->attr->num_index > 0 )
                && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( ref2->address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Attribute::determine_cycle_ratio(
   double const core_job_cycle_time )
{
   if ( this->cycle_time <= -std::numeric_limits< double >::max() ) {
      // User has not specified cycle-time for this attribute so assume the
      // cycle time for this attribute matches the core job cycle time.
      this->cycle_ratio = 1;
   } else {
      // Do a bounds check on the core job cycle time.
      if ( core_job_cycle_time <= 0.0 ) {
         ostringstream errmsg;
         errmsg << "Attribute::determine_cycle_ratio():" << __LINE__
                << " ERROR: FOM Object Attribute '" << this->FOM_name
                << "' with Trick name '" << this->trick_name
                << "'. The core job cycle time (" << core_job_cycle_time
                << " seconds) for the send_cyclic_and_requested_data() job"
                << " must be > 0. Please make sure your S_define and/or THLA.sm"
                << " simulation module specifies a valid cycle time for the"
                << " send_cyclic_and_requested_data() job.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // Do a bounds check on the 'cycle_time' value.
      if ( this->cycle_time < core_job_cycle_time ) {
         ostringstream errmsg;
         errmsg << "Attribute::determine_cycle_ratio():" << __LINE__
                << " ERROR: FOM Object Attribute '" << this->FOM_name
                << "' with Trick name '" << this->trick_name
                << "' has a 'cycle_time' value of " << this->cycle_time
                << " seconds, which is not valid. The attribute 'cycle_time'"
                << " must be >= " << core_job_cycle_time
                << " seconds (i.e. the core job cycle time for the"
                << " send_cyclic_and_requested_data() job). Please check your"
                << " input or modified-data files to make sure the value for"
                << " the attribute 'cycle_time' is specified correctly.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      this->cycle_ratio = round( this->cycle_time / core_job_cycle_time );

      // Make sure we are ready to send the data on the first check.
      this->cycle_cnt = this->cycle_ratio;

      // The cycle-ratio must be an integer ratio so check for any fractional part.
      if ( fmod( this->cycle_time, core_job_cycle_time ) != 0.0 ) {
         ostringstream errmsg;
         errmsg << "Attribute::determine_cycle_ratio():" << __LINE__
                << " ERROR: FOM Object Attribute '" << FOM_name << "' with Trick name '"
                << this->trick_name << "' has a 'cycle_time' value of "
                << this->cycle_time
                << " seconds, which is not an integer multiple of the core job"
                << " cycle time of " << core_job_cycle_time << " seconds for the"
                << " send_cyclic_and_requested_data() job. The ratio of the"
                << " attribute cycle_time to the core job cycle time is ("
                << this->cycle_ratio << " + " << fmod( this->cycle_time, core_job_cycle_time )
                << "), which is not an integer. Please check your input or"
                << " modified-data files to make sure the value for the attribute"
                << " 'cycle_time' is specified correctly.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
         ostringstream msg;
         msg << "Attribute::determine_cycle_ratio():" << __LINE__ << '\n'
             << "  FOM_name:'" << ( ( this->FOM_name != NULL ) ? this->FOM_name : "NULL" ) << "'\n"
             << "  trick_name:'" << ( ( this->trick_name != NULL ) ? this->trick_name : "NULL" ) << "'\n"
             << "  core_job_cycle_time:" << core_job_cycle_time << " seconds\n"
             << "  cyle_time:" << this->cycle_time << " seconds\n"
             << "  cycle_ratio:" << this->cycle_ratio << '\n';
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }
}

VariableLengthData Attribute::get_attribute_value()
{
   if ( rti_encoding == ENCODING_BOOLEAN ) {
      // The size is the number of 1-byte bool values in c++ and we need to
      // map to a 4-byte HLAboolean type. The buffer already holds the
      // encoded HLAboolean type.
      return VariableLengthData( buffer, ( 4 * size ) );
   }
   return VariableLengthData( buffer, size );
}

bool Attribute::extract_data(             // RETURN: -- True if data successfully extracted, false otherwise.
   VariableLengthData const *attr_value ) // IN: ** HLA attribute-value to get data from.
{
   if ( attr_value == NULL ) {
      return false;
   }

   // Keep track of the attribute FOM size and ensure enough buffer capacity.
   int attr_size = attr_value->size();

   // Determine the number of bytes we expect to receive.
   int expected_byte_count = get_attribute_size();

   switch ( rti_encoding ) {
      case ENCODING_BOOLEAN: {
         if ( attr_size != ( 4 * expected_byte_count ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::extract_data():" << __LINE__
                   << " WARNING: For Attribute '" << FOM_name << "' with Trick name '"
                   << trick_name << "', the received FOM data size (" << attr_size
                   << " bytes) != Expected Trick simulation variable memory size ("
                   << ( 4 * expected_byte_count ) << " bytes) for 'rti_encoding' of"
                   << " ENCODING_BOOLEAN. Make sure your simulation variable is the same"
                   << " size and type as what is defined in the FOM. If you are"
                   << " using Lag Compensation one possible cause of this problem"
                   << " is that your lag compensation variables are not the correct"
                   << " size or type.\n";
            message_publish( MSG_WARNING, errmsg.str().c_str() );

            // For now, we ignore this error by just returning here.
            return false;
         }

         // Ensure enough buffer capacity.
         ensure_buffer_capacity( attr_size );

         // Note: We don't set the 'size' to the value of "attr_size" since we
         // are mapping 4-byte HLAboolean types to 1-byte bool in C++.
         //
         // Copy the RTI attribute value into the buffer.
         memcpy( buffer, attr_value->data(), attr_size );
         break;
      }
      case ENCODING_NONE: {
         // The byte counts must match between the received attribute and
         // the Trick simulation variable for ENCODING_NONE since this
         // RTI encoding only supports a fixed length array of characters.
         if ( attr_size != expected_byte_count ) {
            ostringstream errmsg;
            errmsg << "Attribute::extract_data():" << __LINE__
                   << " WARNING: For Attribute '" << FOM_name << "' with Trick name '"
                   << trick_name << "', the received FOM data size (" << attr_size
                   << " bytes) != Expected Trick simulation variable memory size ("
                   << expected_byte_count << " bytes) for the rti_encoding of"
                   << " ENCODING_NONE. The ENCODING_NONE only supports a fixed"
                   << " length array of characters. Make sure your simulation"
                   << " variable is the same size and type as what is defined in the"
                   << " FOM. If you are using Lag Compensation one possible cause of"
                   << " this problem is that your lag compensation variables are not"
                   << " the correct size or type.\n";
            message_publish( MSG_WARNING, errmsg.str().c_str() );

            // Just return if we have a data size mismatch. This will allow us
            // to continue to run even though the other federate is sending us
            // data that is not correct in size.
            return false;
         }

         // Ensure enough buffer capacity.
         ensure_buffer_capacity( attr_size );

         // Make sure the buffer size matches how much data we are putting in it.
         this->size = attr_size;

         // Copy the RTI attribute value into the buffer.
         memcpy( buffer, attr_value->data(), size );
         break;
      }
      case ENCODING_LOGICAL_TIME: {
         // Sanity check: The attribute FOM size must not exceed that of the
         // Trick ref-attributes.
         //
         // TODO: Account for variable length types such as a byte buffer, but
         // for now we are doing a rigid bounds check regardless of type. DDexter
         //
         if ( attr_size != 8 ) {
            ostringstream errmsg;
            errmsg << "Attribute::extract_data():" << __LINE__
                   << " ERROR: For Attribute '" << FOM_name << "' with Trick name '"
                   << trick_name << "', the received FOM data size (" << attr_size
                   << " bytes) != Expected Trick simulation variable memory size (8 bytes) for"
                   << " the ENCODING_LOGICAL_TIME rti_encoding. Make sure your"
                   << " simulation variable is the same size and type as what is"
                   << " defined in the FOM. If you are using Lag Compensation one"
                   << " possible cause of this problem is that your lag compensation"
                   << " variables are not the correct size or type.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         // Ensure enough buffer capacity.
         ensure_buffer_capacity( attr_size );

         // Make sure the buffer size matches how much data we are putting in it.
         this->size = attr_size;

         // Copy the RTI attribute value into the buffer.
         memcpy( buffer, attr_value->data(), size );
         break;
      }
      case ENCODING_OPAQUE_DATA: {
         if ( size_is_static && ( attr_size != expected_byte_count ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::extract_data():" << __LINE__
                   << " WARNING: For Attribute '" << FOM_name << "' with Trick name '"
                   << trick_name << "', the received FOM data size (" << attr_size
                   << " bytes) != Expected Trick simulation variable memory size ("
                   << expected_byte_count << " bytes) for the rti_encoding of"
                   << " ENCODING_OPAQUE_DATA. Your simulation variable is static in size"
                   << " and can not take variable length data. Make sure your"
                   << " simulation variable is the same size and type as what is"
                   << " defined in the FOM. If you are using Lag Compensation one"
                   << " possible cause of this problem is that your lag compensation"
                   << " variables are not the correct size or type.\n";
            message_publish( MSG_WARNING, errmsg.str().c_str() );

            // For now, we ignore this error by just returning here.
            return false;
         }

         // Ensure enough buffer capacity.
         ensure_buffer_capacity( attr_size );

         // Make sure the buffer size matches how much data we are putting in it.
         this->size = attr_size;

         // Copy the RTI attribute value into the buffer.
         memcpy( buffer, attr_value->data(), size );
         break;
      }
      default: {
         if ( ( ref2->attr->type != TRICK_STRING )
              && ( attr_size != expected_byte_count )
              && ( rti_encoding != ENCODING_UNICODE_STRING ) ) {

            ostringstream errmsg;
            errmsg << "Attribute::extract_data():" << __LINE__
                   << " WARNING: For Attribute '" << FOM_name << "' with Trick name '"
                   << trick_name << "', the received FOM data size (" << attr_size
                   << " bytes) != Expected Trick simulation variable memory size ("
                   << expected_byte_count << " bytes). Make sure your simulation variable"
                   << " is the same size and type as what is defined in the FOM. If"
                   << " you are using Lag Compensation one possible cause of this"
                   << " problem is that your lag compensation variables are not the"
                   << " correct size or type.\n";
            message_publish( MSG_WARNING, errmsg.str().c_str() );

            // For now, we ignore this error by just returning here.
            return false;
         }

         // Ensure enough buffer capacity.
         ensure_buffer_capacity( attr_size );

         // Make sure the buffer size matches how much data we are putting in it.
         this->size = attr_size;

         // Copy the RTI attribute value into the buffer.
         memcpy( buffer, attr_value->data(), size );
         break;
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      message_publish( MSG_NORMAL, "Attribute::extract_data():%d Decoded '%s' (trick_name '%s') from attribute map.\n",
                       __LINE__, get_FOM_name(), get_trick_name() );
   }
   if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      print_buffer();
   }

   // Mark the attribute value as changed.
   mark_changed();

   return true;
}

void Attribute::ensure_buffer_capacity(
   int capacity )
{
   if ( capacity > buffer_capacity ) {
      buffer_capacity = capacity;
      if ( buffer == NULL ) {
         buffer = static_cast< unsigned char * >( TMM_declare_var_1d( "unsigned char", buffer_capacity ) );
      } else {
         buffer = static_cast< unsigned char * >( TMM_resize_array_1d_a( buffer, buffer_capacity ) );
      }
   } else if ( buffer == NULL ) {
      // Handle the case where the buffer has not been created yet and we
      // might have an invalid capacity specified.

      // Make sure the capacity is at least 1.
      buffer_capacity = ( capacity > 0 ) ? capacity : 1;

      buffer = static_cast< unsigned char * >( TMM_declare_var_1d( "unsigned char", buffer_capacity ) );
   }

   if ( buffer == NULL ) {
      ostringstream errmsg;
      errmsg << "Attribute::ensure_buffer_capacity():" << __LINE__
             << " ERROR: Could not allocate memory for buffer for requested"
             << " capacity " << capacity << " for Attribute '" << FOM_name
             << "' with Trick name '" << trick_name << "'!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

void Attribute::calculate_size_and_number_of_items()
{
   int num_bytes = 0;

   // Handle Strings differently since we need to know the length of each string.
   if ( ( ref2->attr->type == TRICK_STRING )
        || ( ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) )
             && ( ref2->attr->num_index > 0 )
             && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) ) {

      calculate_static_number_of_items();

      switch ( rti_encoding ) {
         case ENCODING_OPAQUE_DATA:
         case ENCODING_NONE: {
            // Determine total number of bytes used by the Trick simulation
            // variable, and the data can be binary and not just the printable
            // ASCII characters.
            for ( int i = 0; i < num_items; ++i ) {
               char *s = *( static_cast< char ** >( ref2->address ) + i );
               if ( s != NULL ) {
                  int length = get_size( s );
                  if ( length > 0 ) {
                     num_bytes += length;
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
            for ( int i = 0; i < num_items; ++i ) {
               char const *s = *( static_cast< char ** >( ref2->address ) + i );
               if ( s != NULL ) {
                  num_bytes += strlen( s );
               }
            }
            break;
         }
      }
   } else {
      // Handle all the other primitive types.

      if ( ( ref2->attr->num_index > 0 ) && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) {
         // We have a multi-dimension array that is a pointer and the
         // number of dimensions is num_index

         // TODO: Handle the case when ref2->attr->num_index > 1, right now we
         // only are handling the case when num_index == 1 below.
         // NOTE: For now we assume 1-D array.

         // get_size returns the number of elements in the array.
         int trick_size = get_size( *static_cast< char ** >( ref2->address ) ) * ref2->attr->size;
         num_bytes      = ( trick_size >= 0 ) ? trick_size : 0;

         // Since the users variable is a pointer, we need to recalculate
         // the number of items based on the item size.
         if ( ref2->attr->size > 0 ) {
            this->num_items = num_bytes / ref2->attr->size;
         } else {
            // Punt and set the number of items equal to the number of bytes.
            this->num_items = num_bytes;
         }
      } else {
         // The user variable is either a primitive type or a static
         // multi-dimensional array.

         // Please note that get_size_from_attributes() and
         // nitems * ref2->attr->size should return the same number of bytes.
         // num_bytes = get_size_from_attributes( ref2->attr, ref2->attr->name );

         calculate_static_number_of_items();
         num_bytes = num_items * ref2->attr->size;
      }
   }

   this->size = num_bytes;

   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      string attr_handle_string;
      StringUtilities::to_string( attr_handle_string, this->attr_handle );
      ostringstream msg;
      msg << "Attribute::calculate_size_and_number_of_items():" << __LINE__ << '\n'
          << "========================================================\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
          << "  AttributeHandle:" << attr_handle_string << '\n'
          << "  ref2->attr->name:'" << ref2->attr->name << "'\n"
          << "  ref2->attr->type_name:'" << ref2->attr->type_name << "'\n"
          << "  ref2->attr->type:" << ref2->attr->type << '\n'
          << "  ref2->attr->units:" << ref2->attr->units << '\n'
          << "  size:" << size << '\n'
          << "  num_items:" << num_items << '\n'
          // TODO: Figure out get_size_from_attributes() API in Trick 10.
          //   << "  get_size_from_attributes():" << get_size_from_attributes(ref2->attr, ref2->attr->name) << endl
          << "  ref2->attr->size:" << ref2->attr->size << '\n'
          << "  ref2->attr->num_index:" << ref2->attr->num_index << '\n'
          << "  ref2->attr->index[0].size:" << ( ref2->attr->num_index >= 1 ? ref2->attr->index[0].size : 0 ) << '\n'
          << "  publish:" << publish << '\n'
          << "  subscribe:" << subscribe << '\n'
          << "  locally_owned:" << locally_owned << '\n'
          << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
          << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( ref2->attr->type == TRICK_STRING )
           || ( ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( ref2->attr->num_index > 0 )
                && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( ref2->address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return;
}

/*! @details If the attribute is static in size it uses a cached size value
 * otherwise the size is calculated. */
int Attribute::get_attribute_size()
{
   if ( !size_is_static ) {
      calculate_size_and_number_of_items();
   }
   return ( this->size );
}

bool Attribute::is_static_in_size() const
{
   if ( is_supported_attribute_type() ) {
      // If this is not an array (i.e. num_index == 0) or has static arrays then
      // this attribute is static in size.
      if ( ref2->attr->num_index > 0 ) {
         for ( int i = 0; i < ref2->attr->num_index; ++i ) {
            // Make sure each dimension is statically defined (i.e. not zero).
            if ( ref2->attr->index[i].size <= 0 ) {
               return false;
            }
         }
      }
      return true;
   }
   return false;
}

/*! @details If the attribute is not for an array then a value of one is
 * set in the num_items variable. Otherwise the number of items in the static
 * array are set in the num_items variable.
 *
 * \par<b>Assumptions and Limitations:</b>
 * - Only static arrays are supported for now.
 */
void Attribute::calculate_static_number_of_items()
{
   int length = 1;

   // Determine the number of items this attribute has (i.e. items in array).
   if ( ref2->attr->num_index > 0 ) {
      for ( int i = 0; i < ref2->attr->num_index; ++i ) {
         if ( ref2->attr->index[i].size > 0 ) {
            length *= ref2->attr->index[i].size;
         }
      }
   }

   this->num_items = length;
}

void Attribute::pack_attribute_buffer()
{
   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      string attr_handle_string;
      StringUtilities::to_string( attr_handle_string, this->attr_handle );
      ostringstream msg;
      msg << "Attribute::pack_attribute_buffer():" << __LINE__ << '\n'
          << "================== BEFORE PACK ==================================\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
          << "  AttributeHandle:" << attr_handle_string << '\n'
          << "  ref2->attr->name:'" << ref2->attr->name << "'\n"
          << "  ref2->attr->type_name:'" << ref2->attr->type_name << "'\n"
          << "  ref2->attr->type:" << ref2->attr->type << '\n'
          << "  ref2->attr->units:" << ref2->attr->units << '\n'
          << "  size:" << size << '\n'
          << "  num_items:" << num_items << '\n'
          // TODO: Figure out get_size_from_attributes() API in Trick 10.
          //   << "  get_size_from_attributes():" << get_size_from_attributes(ref2->attr, ref2->attr->name) << endl
          << "  ref2->attr->size:" << ref2->attr->size << '\n'
          << "  ref2->attr->num_index:" << ref2->attr->num_index << '\n'
          << "  ref2->attr->index[0].size:" << ( ref2->attr->num_index >= 1 ? ref2->attr->index[0].size : 0 ) << '\n'
          << "  publish:" << publish << '\n'
          << "  subscribe:" << subscribe << '\n'
          << "  locally_owned:" << locally_owned << '\n'
          << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
          << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( ref2->attr->type == TRICK_STRING )
           || ( ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( ref2->attr->num_index > 0 )
                && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( ref2->address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Don't pack the buffer if the attribute is not locally owned. Otherwise this will
   // corrupt the buffer for the data we received for this attribute from another federate.
   if ( !is_locally_owned() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
         ostringstream msg;
         msg << "Attribute::pack_attribute_buffer():" << __LINE__ << '\n'
             << " FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
             << " trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
             << " Skipping pack because attribute is not locally owned!\n";
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
      return;
   }

   // TODO: Use a transcoder for each type to encode and decode depending on
   // the type specified in the FOM instead of the code below. Dan Dexter

   switch ( rti_encoding ) {
      case ENCODING_LOGICAL_TIME: {
         num_items = 1;
         size      = 8;

         // Ensure enough capacity in the buffer for the attribute and all its
         // items if it was an array.
         ensure_buffer_capacity( size );

         // Encode the logical time.
         encode_logical_time();
         break;
      }
      case ENCODING_BOOLEAN: {
         // Determine the number of items this attribute has (i.e. is it an array).
         if ( !size_is_static ) {
            calculate_size_and_number_of_items();
         }

         encode_boolean_to_buffer();

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
            ostringstream msg;
            msg << "Attribute::pack_attribute_buffer():" << __LINE__ << '\n'
                << "================== ATTRIBUTE ENCODE ==================================\n"
                << " attribute '" << FOM_name << "' (trick name '" << trick_name
                << "')\n";
            message_publish( MSG_NORMAL, msg.str().c_str() );
            print_buffer();
         }
         break;
      }
      case ENCODING_OPAQUE_DATA: {
         // NOTE: For now we must calculate size every time because on a
         // receive, the 'size' is adjusted to the number of bytes received
         // and does not reflect what we are sending. We only have this
         // problem for variable length types such as strings which is the
         // only variable length type we support right now. DDexter
         calculate_size_and_number_of_items();

         encode_opaque_data_to_buffer();

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
            ostringstream msg;
            msg << "Attribute::pack_attribute_buffer():" << __LINE__ << '\n'
                << "================== ATTRIBUTE ENCODE ==================================\n"
                << " attribute '" << FOM_name << "' (trick name '" << trick_name
                << "')\n";
            message_publish( MSG_NORMAL, msg.str().c_str() );
            print_buffer();
         }
         break;
      }
      default: {
         // Must handle the string as a special case because of special encodings.
         if ( ( ref2->attr->type == TRICK_STRING )
              || ( ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) )
                   && ( ref2->attr->num_index > 0 )
                   && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) ) {
            // NOTE: For now we must calculate size every time because on a
            // receive, the 'size' is adjusted to the number of bytes received
            // and does not reflect what we are sending. We only have this
            // problem for variable length types such as strings which is the
            // only variable length type we support right now. DDexter
            calculate_size_and_number_of_items();

            encode_string_to_buffer();

            if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
               ostringstream msg;
               msg << "Attribute::pack_attribute_buffer():" << __LINE__ << '\n'
                   << "================== ATTRIBUTE ENCODE ==================================\n"
                   << " attribute '" << FOM_name << "' (trick name '" << trick_name
                   << "')\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
               print_buffer();
            }
         } else {
            // Determine the number of items this attribute has (i.e. is it an array).
            if ( !size_is_static ) {
               calculate_size_and_number_of_items();
            }

            // Ensure enough capacity in the buffer for the attribute and all its
            // items if it was an array.
            ensure_buffer_capacity( size );

            // Determine if the users variable is a pointer.
            if ( ( ref2->attr->num_index > 0 )
                 && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) {
               // It's a pointer

               // Byteswap if needed and copy the attribute to the buffer.
               byteswap_buffer_copy( buffer,
                                     *static_cast< char ** >( ref2->address ),
                                     ref2->attr->type,
                                     num_items,
                                     size );
            } else {
               // It's either a primitive type or a static array.

               // Byteswap if needed and copy the attribute to the buffer.
               byteswap_buffer_copy( buffer,
                                     ref2->address,
                                     ref2->attr->type,
                                     num_items,
                                     size );
            }

            if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
               ostringstream msg;
               msg << "Attribute::pack_attribute_buffer():" << __LINE__ << '\n'
                   << "================== ATTRIBUTE ENCODE ==================================\n"
                   << " attribute '" << FOM_name << "' (trick name '" << trick_name
                   << "')\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
               print_buffer();
            }
         }
         break;
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      string attr_handle_string;
      StringUtilities::to_string( attr_handle_string, this->attr_handle );
      ostringstream msg2;
      msg2 << "Attribute::pack_attribute_buffer():" << __LINE__ << '\n'
           << "================== AFTER PACK ==================================\n"
           << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
           << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
           << "  AttributeHandle:" << attr_handle_string << '\n'
           << "  ref2->attr->name:'" << ref2->attr->name << "'\n"
           << "  ref2->attr->type_name:'" << ref2->attr->type_name << "'\n"
           << "  ref2->attr->type:" << ref2->attr->type << '\n'
           << "  ref2->attr->units:" << ref2->attr->units << '\n'
           << "  size:" << size << '\n'
           << "  num_items:" << num_items << '\n'
           // TODO: Figure out get_size_from_attributes() API in Trick 10.
           //    << "  get_size_from_attributes():" << get_size_from_attributes(ref2->attr, ref2->attr->name) << endl
           << "  ref2->attr->size:" << ref2->attr->size << '\n'
           << "  ref2->attr->num_index:" << ref2->attr->num_index << '\n'
           << "  ref2->attr->index[0].size:" << ( ref2->attr->num_index >= 1 ? ref2->attr->index[0].size : 0 ) << '\n'
           << "  publish:" << publish << '\n'
           << "  subscribe:" << subscribe << '\n'
           << "  locally_owned:" << locally_owned << '\n'
           << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
           << "  buffer_capacity:" << buffer_capacity << '\n'
           << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
           << "  rti_encoding:" << rti_encoding << '\n'
           << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( ref2->attr->type == TRICK_STRING )
           || ( ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( ref2->attr->num_index > 0 )
                && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) ) {
         msg2 << "  value:\"" << ( *static_cast< char ** >( ref2->address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }
}

void Attribute::unpack_attribute_buffer()
{
   // Don't unpack the attribute buffer if the attribute is locally owned, which
   // means we did not receive data from another federate for this attribute.
   if ( is_locally_owned() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
         ostringstream msg;
         msg << "Attribute::unpack_attribute_buffer():" << __LINE__ << '\n'
             << " FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
             << " trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
             << " Skipping unpack of attribute buffer because the attribute is locally owned.\n";
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
      return;
   }

   // TODO: Use a transcoder for each type to encode and decode depending on
   // the type specified in the FOM instead of the code below. Dan Dexter

   switch ( rti_encoding ) {
      case ENCODING_LOGICAL_TIME: {
         num_items = 1;
         size      = 8;

         // Decode the logical time.
         decode_logical_time();
         break;
      }
      case ENCODING_BOOLEAN: {
         // Determine the number of items this attribute has (i.e. is it an array).
         if ( !size_is_static ) {
            calculate_size_and_number_of_items();
         }

         decode_boolean_from_buffer();

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
            ostringstream msg;
            msg << "Attribute::unpack_attribute_buffer():" << __LINE__ << '\n'
                << "================== ATTRIBUTE DECODE ==================================\n"
                << " attribute '" << FOM_name << "' (trick name '" << trick_name
                << "')\n";
            message_publish( MSG_NORMAL, msg.str().c_str() );
            print_buffer();
         }
         break;
      }
      case ENCODING_OPAQUE_DATA: {
         // The size is the received size but recalculate the number of items.
         if ( !size_is_static ) {
            calculate_static_number_of_items();
         }

         decode_opaque_data_from_buffer();

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
            ostringstream msg;
            msg << "Attribute::unpack_attribute_buffer():" << __LINE__ << '\n'
                << "================== ATTRIBUTE DECODE =============================\n"
                << " attribute '" << FOM_name << "' (trick name '" << trick_name
                << "')\n";
            message_publish( MSG_NORMAL, msg.str().c_str() );
            print_buffer();
         }
         break;
      }
      default: {
         // Must handle the string as a special case because of special encodings.
         if ( ( ref2->attr->type == TRICK_STRING )
              || ( ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) )
                   && ( ref2->attr->num_index > 0 )
                   && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) ) {
            // The size is the received size but recalculate the number of items.
            if ( !size_is_static ) {
               if ( ref2->attr->type == TRICK_STRING ) {
                  calculate_static_number_of_items();
               } else {
                  calculate_size_and_number_of_items();
               }
            }

            decode_string_from_buffer();

            if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
               ostringstream msg;
               msg << "Attribute::unpack_attribute_buffer():" << __LINE__ << '\n'
                   << "================== ATTRIBUTE DECODE ==================================\n"
                   << " attribute '" << FOM_name << "' (trick name '" << trick_name << "')"
                   << " value:\"" << ( *static_cast< char ** >( ref2->address ) ) << "\"\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
               print_buffer();
            }

         } else {

            // Determine the number of items this attribute has (i.e. is it an array).
            if ( !size_is_static ) {
               calculate_size_and_number_of_items();
            }

            // Determine if the users variable is a pointer.
            if ( ( ref2->attr->num_index > 0 ) && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) {
               // It's a pointer

               // Byteswap if needed and copy the buffer over to the attribute.
               byteswap_buffer_copy( *static_cast< char ** >( ref2->address ),
                                     buffer,
                                     ref2->attr->type,
                                     num_items,
                                     size );
            } else {
               // It's either a primitive type or a static array.

               // Byteswap if needed and copy the buffer over to the attribute.
               byteswap_buffer_copy( ref2->address,
                                     buffer,
                                     ref2->attr->type,
                                     num_items,
                                     size );

               if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
                  ostringstream msg;
                  msg << "Attribute::unpack_attribute_buffer():" << __LINE__ << '\n'
                      << "================== ATTRIBUTE DECODE ==================================\n"
                      << " attribute '" << FOM_name << "' (trick name '" << trick_name
                      << "')\n";
                  message_publish( MSG_NORMAL, msg.str().c_str() );
                  print_buffer();
               }
            }
         }
         break;
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      string attr_handle_string;
      StringUtilities::to_string( attr_handle_string, this->attr_handle );
      ostringstream msg;
      msg << "Attribute::unpack_attribute_buffer():" << __LINE__ << '\n'
          << "========================================================\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
          << "  AttributeHandle:" << attr_handle_string << '\n'
          << "  ref2->attr->name:'" << ref2->attr->name << "'\n"
          << "  ref2->attr->type_name:'" << ref2->attr->type_name << "'\n"
          << "  ref2->attr->type:" << ref2->attr->type << '\n'
          << "  ref2->attr->units:" << ref2->attr->units << '\n'
          << "  size:" << size << '\n'
          << "  num_items:" << num_items << '\n'
          // TODO: Figure out get_size_from_attributes() API in Trick 10.
          //   << "  get_size_from_attributes():" << get_size_from_attributes(ref2->attr, ref2->attr->name) << endl
          << "  ref2->attr->size:" << ref2->attr->size << '\n'
          << "  ref2->attr->num_index:" << ref2->attr->num_index << '\n'
          << "  ref2->attr->index[0].size:" << ( ref2->attr->num_index >= 1 ? ref2->attr->index[0].size : 0 ) << '\n'
          << "  publish:" << publish << '\n'
          << "  subscribe:" << subscribe << '\n'
          << "  locally_owned:" << locally_owned << '\n'
          << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
          << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( ref2->attr->type == TRICK_STRING )
           || ( ( ( ref2->attr->type == TRICK_CHARACTER ) || ( ref2->attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( ref2->attr->num_index > 0 )
                && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( ref2->address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

void Attribute::encode_boolean_to_buffer() // RETURN: -- None.
{
   bool *bool_src;

   // Determine if the users variable is a pointer.
   if ( ( ref2->attr->num_index > 0 ) && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) {
      // It's a pointer
      bool_src = reinterpret_cast< bool * >( *static_cast< char ** >( ref2->address ) );

   } else {
      // It's either a primitive type or a static array.
      bool_src = static_cast< bool * >( ref2->address );
   }

   // Encoded size is the number of (32 bit Big Endian) elements.
   ensure_buffer_capacity( 4 * num_items );

   unsigned int *int_dest = reinterpret_cast< unsigned int * >( buffer );

   if ( num_items == 1 ) {
      int_dest[0] = ( bool_src[0] ? HLAtrue : 0 );
   } else {
      for ( int k = 0; k < num_items; ++k ) {
         int_dest[k] = ( bool_src[k] ? HLAtrue : 0 );
      }
   }
}

void Attribute::decode_boolean_from_buffer() const // RETURN: -- None.
{
   bool *bool_dest;

   // Determine if the users variable is a pointer.
   if ( ( ref2->attr->num_index > 0 ) && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) {
      // It's a pointer
      bool_dest = reinterpret_cast< bool * >( ( *static_cast< char ** >( ref2->address ) ) );
   } else {
      // It's either a primitive type or a static array.
      bool_dest = static_cast< bool * >( ref2->address );
   }

   unsigned int const *int_src = reinterpret_cast< unsigned int * >( buffer );

   if ( num_items == 1 ) {
      bool_dest[0] = ( int_src[0] != 0 );
   } else {
      for ( int k = 0; k < num_items; ++k ) {
         bool_dest[k] = ( int_src[k] != 0 );
      }
   }
}

void Attribute::encode_logical_time() const // RETURN: -- None.
{
   // Integer representing time in the HLA Logical Time base.
   int64_t logical_time = 0;

   switch ( ref2->attr->type ) {
      case TRICK_DOUBLE: {
         double const *d_src = reinterpret_cast< double * >( ref2->address );
         logical_time        = Int64BaseTime::to_base_time( d_src[0] );
         break;
      }
      case TRICK_FLOAT: {
         float const *f_src = reinterpret_cast< float * >( ref2->address );
         logical_time       = Int64BaseTime::to_base_time( (double)f_src[0] );
         break;
      }
      case TRICK_SHORT: {
         short const *s_src = static_cast< short * >( ref2->address );
         logical_time       = (int64_t)( Int64BaseTime::get_base_time_multiplier() * s_src[0] );
         break;
      }
      case TRICK_UNSIGNED_SHORT: {
         unsigned short const *us_src = static_cast< unsigned short * >( ref2->address );
         logical_time                 = (int64_t)( Int64BaseTime::get_base_time_multiplier() * us_src[0] );
         break;
      }
      case TRICK_INTEGER: {
         int const *i_src = static_cast< int * >( ref2->address );
         logical_time     = (int64_t)( Int64BaseTime::get_base_time_multiplier() * i_src[0] );
         break;
      }
      case TRICK_UNSIGNED_INTEGER: {
         unsigned int const *ui_src = static_cast< unsigned int * >( ref2->address );
         logical_time               = (int64_t)( Int64BaseTime::get_base_time_multiplier() * ui_src[0] );
         break;
      }
      case TRICK_LONG: {
         long const *l_src = static_cast< long * >( ref2->address );
         logical_time      = (int64_t)( Int64BaseTime::get_base_time_multiplier() * l_src[0] );
         break;
      }
      case TRICK_UNSIGNED_LONG: {
         unsigned long const *ul_src = static_cast< unsigned long * >( ref2->address );
         logical_time                = (int64_t)( Int64BaseTime::get_base_time_multiplier() * ul_src[0] );
         break;
      }
      case TRICK_LONG_LONG: {
         long long const *ll_src = static_cast< long long * >( ref2->address );
         logical_time            = (int64_t)( Int64BaseTime::get_base_time_multiplier() * ll_src[0] );
         break;
      }
      case TRICK_UNSIGNED_LONG_LONG: {
         unsigned long long const *ull_src = static_cast< unsigned long long * >( ref2->address );
         logical_time                      = (int64_t)( Int64BaseTime::get_base_time_multiplier() * ull_src[0] );
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "Attribute::encode_logical_time():" << __LINE__
                << " ERROR: For Attribute '" << FOM_name << "' with Trick name '"
                << trick_name << "' the type is not supported for the"
                << " ENCODING_LOGICAL_TIME encoding.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }

   // Now that the buffer has been possibly resized, cast it to
   // something a little easier to use.
   unsigned char *output = buffer;

   output[0] = (unsigned char)( ( logical_time >> 56 ) & 0xFF );
   output[1] = (unsigned char)( ( logical_time >> 48 ) & 0xFF );
   output[2] = (unsigned char)( ( logical_time >> 40 ) & 0xFF );
   output[3] = (unsigned char)( ( logical_time >> 32 ) & 0xFF );
   output[4] = (unsigned char)( ( logical_time >> 24 ) & 0xFF );
   output[5] = (unsigned char)( ( logical_time >> 16 ) & 0xFF );
   output[6] = (unsigned char)( ( logical_time >> 8 ) & 0xFF );
   output[7] = (unsigned char)( ( logical_time >> 0 ) & 0xFF );
}

void Attribute::decode_logical_time() // RETURN: -- None.
{
   // Integer representing time in the HLA Logical Time base.
   int64_t              logical_time = 0;
   unsigned char const *src          = buffer;

   logical_time = ( logical_time << 8 ) | src[0]; // cppcheck-suppress [badBitmaskCheck]
   logical_time = ( logical_time << 8 ) | src[1];
   logical_time = ( logical_time << 8 ) | src[2];
   logical_time = ( logical_time << 8 ) | src[3];
   logical_time = ( logical_time << 8 ) | src[4];
   logical_time = ( logical_time << 8 ) | src[5];
   logical_time = ( logical_time << 8 ) | src[6];
   logical_time = ( logical_time << 8 ) | src[7];

   switch ( ref2->attr->type ) {
      case TRICK_DOUBLE: {
         double *d_dest = reinterpret_cast< double * >( ref2->address );
         d_dest[0]      = Int64BaseTime::to_seconds( logical_time );
         break;
      }
      case TRICK_FLOAT: {
         float *f_dest = reinterpret_cast< float * >( ref2->address );
         f_dest[0]     = (float)Int64BaseTime::to_seconds( logical_time );
         break;
      }
      case TRICK_SHORT: {
         short  *s_dest = static_cast< short * >( ref2->address );
         int64_t value  = logical_time / Int64BaseTime::get_base_time_multiplier();
         s_dest[0]      = ( value > SHRT_MAX ) ? SHRT_MAX : (short)value;
         break;
      }
      case TRICK_UNSIGNED_SHORT: {
         unsigned short *us_dest = static_cast< unsigned short * >( ref2->address );
         int64_t         value   = logical_time / Int64BaseTime::get_base_time_multiplier();
         us_dest[0]              = ( value > USHRT_MAX ) ? USHRT_MAX : (unsigned short)value;
         break;
      }
      case TRICK_INTEGER: {
         int    *i_dest = static_cast< int * >( ref2->address );
         int64_t value  = logical_time / Int64BaseTime::get_base_time_multiplier();
         i_dest[0]      = ( value > INT_MAX ) ? INT_MAX : value;
         break;
      }
      case TRICK_UNSIGNED_INTEGER: {
         unsigned int *ui_dest = static_cast< unsigned int * >( ref2->address );
         int64_t       value   = logical_time / Int64BaseTime::get_base_time_multiplier();
         ui_dest[0]            = ( value > UINT_MAX ) ? UINT_MAX : (unsigned int)value;
         break;
      }
      case TRICK_LONG: {
         long   *l_dest = static_cast< long * >( ref2->address );
         int64_t value  = logical_time / Int64BaseTime::get_base_time_multiplier();
         l_dest[0]      = ( value > LONG_MAX ) ? LONG_MAX : (long)value;
         break;
      }
      case TRICK_UNSIGNED_LONG: {
         unsigned long *ul_dest = static_cast< unsigned long * >( ref2->address );
         int64_t        value   = logical_time / Int64BaseTime::get_base_time_multiplier();
         ul_dest[0]             = ( value > (int64_t)ULONG_MAX ) ? ULONG_MAX : (unsigned long)value;
         break;
      }
      case TRICK_LONG_LONG: {
         long long *ll_dest = static_cast< long long * >( ref2->address );
         int64_t    value   = logical_time / Int64BaseTime::get_base_time_multiplier();
         ll_dest[0]         = ( value > Int64BaseTime::get_max_base_time() ) ? Int64BaseTime::get_max_base_time() : (long long)value;
         break;
      }
      case TRICK_UNSIGNED_LONG_LONG: {
         unsigned long long *ull_dest = static_cast< unsigned long long * >( ref2->address );

         int64_t value = logical_time / Int64BaseTime::get_base_time_multiplier();

         ull_dest[0] = ( value > Int64BaseTime::get_max_base_time() )
                          ? (unsigned long long)Int64BaseTime::get_max_base_time()
                          : (unsigned long long)value;
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "Attribute::decode_logical_time():" << __LINE__
                << " ERROR: For Attribute '" << FOM_name << "' with Trick name '"
                << trick_name << "' the type is not supported for the"
                << " ENCODING_LOGICAL_TIME encoding.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
}

void Attribute::encode_opaque_data_to_buffer() // RETURN: -- None.
{
   // Must handle the string as a special case because of special encodings.
   if ( ref2->attr->type == TRICK_STRING ) {
      encode_string_to_buffer();
   } else {
      // Handle the other primitive types.
      unsigned char *output;       // Cast the buffer to be a character array.
      int            num_elements; // Number of elements in the encoded string.
      char          *s;            // pointer to a string

      // HLAopaqueData format documented in IEEE Standard 1516.2-2000, which
      // will handle variable length binary data.

      // Determine if the users variable is a pointer.
      if ( ( ref2->attr->num_index > 0 ) && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) {
         // It's a pointer
         s = *( static_cast< char ** >( ref2->address ) );
      } else {
         // It's either a primitive type or a static array.
         s = static_cast< char * >( ref2->address );
      }

      if ( s != NULL ) {
         // Get the number of bytes allocated to this variable by Trick.
         num_elements = get_size( s );
         if ( num_elements < 0 ) {
            num_elements = 0;
         }

      } else {
         num_elements = 0;
      }

      // Encoded size is the number of elements (32 bit Big Endian)
      // followed by the data characters. Make sure we can hold the
      // encoded data.
      ensure_buffer_capacity( 4 + num_elements );

      // Now that the buffer has been possibly resized, cast it to
      // something a little easier to use.
      output = buffer;

      // Store the number of elements as an HLAinteger32BE (Big Endian).
      if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
         *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_elements ) ) + 0 );
         *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_elements ) ) + 1 );
         *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_elements ) ) + 2 );
         *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_elements ) ) + 3 );
      } else {
         *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_elements ) ) + 3 );
         *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_elements ) ) + 2 );
         *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_elements ) ) + 1 );
         *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_elements ) ) + 0 );
      }
      int byte_count = 4;

      // Copy the data to the output buffer.
      if ( ( s != NULL ) && ( num_elements > 0 ) ) {
         memcpy( output, s, num_elements );
         byte_count += num_elements;
      }

      // The amount of data in the buffer (i.e. size) is the encoded size.
      size = byte_count;
   }
}

void Attribute::decode_opaque_data_from_buffer() // RETURN: -- None.
{
   // Must handle the string as a special case because of special encodings.
   if ( ref2->attr->type == TRICK_STRING ) {
      decode_string_from_buffer();
   } else {
      // Handle the other primitive types.
      unsigned char *input;
      unsigned char *output;
      int            decoded_length = 0;

      // HLAopaqueData format documented in IEEE Standard 1516.2-2000, which
      // will handle variable length binary data.
      input = buffer;

      // Decode the number of elements which is an HLAinteger32BE (Big Endian).
      if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
         *( ( reinterpret_cast< unsigned char * >( &decoded_length ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
         *( ( reinterpret_cast< unsigned char * >( &decoded_length ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
         *( ( reinterpret_cast< unsigned char * >( &decoded_length ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
         *( ( reinterpret_cast< unsigned char * >( &decoded_length ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
      } else {
         *( ( reinterpret_cast< unsigned char * >( &decoded_length ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
         *( ( reinterpret_cast< unsigned char * >( &decoded_length ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
         *( ( reinterpret_cast< unsigned char * >( &decoded_length ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
         *( ( reinterpret_cast< unsigned char * >( &decoded_length ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
      }

      // Do a sanity check on the decoded length, it should not be negative.
      if ( decoded_length < 0 ) {
         message_publish( MSG_WARNING, "Attribute::decode_opaque_data_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA attribute '%s', decoded length %d < 0, will use 0 instead.\n",
                          __LINE__, FOM_name, decoded_length );
         decoded_length = 0;
      }

      // Do a sanity check on the decoded length as compared to how much
      // data is in the buffer, i.e. data_buff_size = size - 4.
      int data_buff_size;
      if ( size > 4 ) {
         data_buff_size = size - 4;
      } else {
         data_buff_size = 0;
      }
      if ( decoded_length > data_buff_size ) {
         message_publish( MSG_WARNING, "Attribute::decode_opaque_data_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA attribute '%s', decoded length %d > data buffer \
size %d, will use the data buffer size instead.\n",
                          __LINE__, FOM_name, decoded_length, data_buff_size );
         decoded_length = data_buff_size;
      }

      // Determine if the users variable is a pointer.
      if ( ( ref2->attr->num_index > 0 ) && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) {
         // It's a pointer
         output = *( static_cast< unsigned char ** >( ref2->address ) );

         if ( output != NULL ) {
            // The output array size must exactly match the incoming data size for opaque data.
            if ( decoded_length != get_size( output ) ) {
               *( static_cast< char ** >( ref2->address ) ) =
                  static_cast< char * >( TMM_resize_array_1d_a(
                     *( static_cast< char ** >( ref2->address ) ),
                     ( ( decoded_length > 0 ) ? decoded_length : 1 ) ) );

               output = *( static_cast< unsigned char ** >( ref2->address ) );
            }
         } else {
            // Allocate memory for the output array.
            *( static_cast< char ** >( ref2->address ) ) =
               static_cast< char * >( TMM_declare_var_1d(
                  "char", ( ( decoded_length > 0 ) ? decoded_length : 1 ) ) );

            output = *( static_cast< unsigned char ** >( ref2->address ) );
         }
      } else {
         // It's either a primitive type or a static array.
         output = static_cast< unsigned char * >( ref2->address );

         if ( output != NULL ) {
            // The output array size must exactly match the incoming data
            // size for opaque data.
            if ( decoded_length != get_size( output ) ) {
               // WORKAROUND: Trick 10 can't handle a length of zero so to
               // workaround the memory manager problem use a size of 1 in
               // the allocation.
               ref2->address = static_cast< char * >( TMM_resize_array_1d_a(
                  static_cast< char * >( ref2->address ),
                  ( ( decoded_length > 0 ) ? decoded_length : 1 ) ) );
               output        = static_cast< unsigned char * >( ref2->address );
            }
         } else {
            // Allocate memory for the output array.
            // WORKAROUND: Trick 10 can't handle a length of zero so to
            // workaround the memory manager problem use a size of 1 in
            // the allocation.
            ref2->address = static_cast< char * >( TMM_declare_var_1d( "char", ( ( decoded_length > 0 ) ? decoded_length : 1 ) ) );
            output        = static_cast< unsigned char * >( ref2->address );
         }
      }

      if ( output == NULL ) {
         ostringstream errmsg;
         errmsg << "Attribute::decode_opaque_data_from_buffer():" << __LINE__
                << " ERROR: Could not allocate memory for ENCODING_OPAQUE_DATA Attribute '"
                << FOM_name << "' with Trick name '" << trick_name << "' and length "
                << decoded_length << "!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // Copy the characters over.
      if ( decoded_length > 0 ) {
         memcpy( output, input, decoded_length );
      }
   }
}

void Attribute::encode_string_to_buffer() // RETURN: -- None.
{
   unsigned char *output;       // Cast the buffer to be a character array.
   int            num_elements; // Number of elements in the encoded string.
   char          *s;            // pointer to a string

   switch ( rti_encoding ) {
      case ENCODING_UNICODE_STRING: {
         // HLAunicodeString format documented in IEEE
         // Standard 1516.2-2000, Section 4.12.6
         if ( num_items == 1 ) {

            // Get a reference to the users string that is easier to use.
            s = *( static_cast< char ** >( ref2->address ) );

            // Number of elements to be encoded (number of characters).
            num_elements = ( ( size > 0 ) && ( s != NULL ) ) ? size : 0;

            // Encoded size is the number of elements (32 bit Big Endian)
            // followed by a UTF-16 (16 bit) encoding of the string characters.
            // Make sure we can hold the encoded data.
            ensure_buffer_capacity( 4 + ( 2 * num_elements ) );

            // Now that the buffer has been possibly resized, cast it to
            // something a little easier to use.
            output = buffer;

            // The encoded size is an HLAinteger32BE.
            int encoded_size = ( num_elements <= std::numeric_limits< int >::max() )
                                  ? num_elements
                                  : std::numeric_limits< int >::max();

            // Store the number of elements as an HLAinteger32BE (Big Endian).
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
            } else {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
            }
            int byte_count = 4;

            if ( s != NULL ) {
               // Get the length of the string.
               int length = strlen( s );

               // Encode as UTF-16 characters in Big Endian
               for ( int k = 0; k < length; ++k ) {
                  *( output++ ) = '\0';
                  *( output++ ) = (unsigned char)*( s++ );
               }
               byte_count += 2 * length;
            }

            // The amount of data in the buffer (i.e. size) is the encoded size.
            size = byte_count;

         } else if ( num_items > 1 ) {
            // We have more than one string to encode.

            // Number of HLAASCIIstring elements to be encoded in the
            // HLAvariableArray. The encoded size is an HLAinteger32BE.
            int num_outer_elements = ( num_items <= std::numeric_limits< int >::max() )
                                        ? num_items
                                        : std::numeric_limits< int >::max();

            // Encoded size is the number of outer elements (HLAuncodeString)
            // then a HLAuncodeString element followed by an optional pad then
            // the  next HLAuncodeString element and so on. Each HLAuncodeString
            // element has a 4 byte element count with 2 * length characters and
            // up to 2 pad characters. 4 + 2 * length + 2 = 6 + 2 * length
            // In case we have a NULL string it won't show up in the "size" so
            // add a characters per item to guard against that.
            // 4: encoded size,
            // 2 * (size + num_items): all string characters (UTF-16) + possible NULLs
            // 6 * num_items: 2 pad and 4 bytes for character count per element
            // encoded_size = 4 + 2 * (size + num_items) + 6 * num_items;
            // Make sure we can hold the encoded data.
            ensure_buffer_capacity( 4 + 2 * size + 8 * num_items );

            // Now that the buffer has been possibly resized, cast it to
            // something a little easier to use.
            output = buffer;

            // Store the number of outer elements as an HLAinteger32BE (Big Endian).
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 0 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 3 );
            } else {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 3 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 0 );
            }
            int byte_count = 4;

            // UTF-16 character encoding of the string separated by possible
            // Null character '\0' padding between strings to stay on a 4 byte
            // boundary to keep to the standard.
            for ( int i = 0; i < num_items; ++i ) {

               s = *( static_cast< char ** >( ref2->address ) + i );

               int length = ( s != NULL ) ? strlen( s ) : 0;

               // The encoded size is an HLAinteger32BE.
               int encoded_size = ( length <= std::numeric_limits< int >::max() )
                                     ? length
                                     : std::numeric_limits< int >::max();

               // Store the number of elements as an HLAinteger32BE (Big Endian).
               if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
               } else {
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
               }
               byte_count += 4;

               if ( s != NULL ) {
                  // Encode as UTF-16 characters in Big Endian
                  for ( int k = 0; k < length; ++k ) {
                     *( output++ ) = '\0';
                     *( output++ ) = (unsigned char)*( s++ );
                  }
                  byte_count += ( 2 * length );
               }

               // Separate the strings by a null UTF-16 character if padding
               // is needed to stay on a 4 byte boundary. The last element
               // gets no padding.
               if ( ( i < ( num_items - 1 ) ) && ( ( ( 4 + ( 2 * length ) ) % 4 ) != 0 ) ) {
                  *( output++ ) = '\0';
                  *( output++ ) = '\0';
                  byte_count += 2;
               }
            }

            // The amount of data in the buffer (i.e. size) is the number of
            // bytes we placed in it.
            size = byte_count;
         }
         break;
      }
      case ENCODING_ASCII_STRING: {
         // HLAASCIIstring format documented in IEEE
         // Standard 1516.2-2000, Section 4.12.6
         if ( num_items == 1 ) {

            // ASCII character encoding of the string.
            s = *( static_cast< char ** >( ref2->address ) );

            // Number of elements to be encoded (number of characters).
            num_elements = ( ( size > 0 ) && ( s != NULL ) ) ? size : 0;

            // Encoded size is the number of elements (32 bit Big Endian)
            // followed by the ASCII characters.
            // Make sure we can hold the encoded data.
            ensure_buffer_capacity( 4 + num_elements );

            // Now that the buffer has been possibly resized, cast it to
            // something a little easier to use.
            output = buffer;

            // The encoded size is an HLAinteger32BE.
            int encoded_size = ( num_elements <= std::numeric_limits< int >::max() )
                                  ? num_elements
                                  : std::numeric_limits< int >::max();

            // Store the number of elements as an HLAinteger32BE (Big Endian).
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
            } else {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
            }
            int byte_count = 4;

            if ( s != NULL ) {
               // Get the length of the string.
               int length = strlen( s );

               // Encode as ASCII characters.
               if ( length > 0 ) {
                  memcpy( output, s, length );
                  byte_count += length;
               }
            }

            // The amount of data in the buffer (i.e. size) is the encoded size.
            size = byte_count;

         } else if ( num_items > 1 ) {
            // We have more than one string to encode.

            // Number of HLAASCIIstring elements to be encoded in the
            // HLAvariableArray. The encoded size is an HLAinteger32BE.
            int num_outer_elements = ( num_items <= std::numeric_limits< int >::max() )
                                        ? num_items
                                        : std::numeric_limits< int >::max();

            // Encoded size is the number of outer elements (HLAASCIIstring)
            // then a HLAASCIIstring element followed by an optional pad then
            // the  next HLAASCIIstring element and so on. Each HLAASCIIstring
            // element has a 4 byte element count with length characters and
            // up to 3 pad characters. 4 + length + 3 = 7 + length
            // In case we have a NULL string it won't show up in the "size" so
            // add a character per item to guard against that.
            // 4: encoded size,
            // (size + num_items): all string characters + possible NULLs
            // 7 * num_items: 3 pad + 4 bytes for character count per element
            // encoded_size = 4 + (size + num_items) + 7 * num_items;
            // Make sure we can hold the encoded data.
            ensure_buffer_capacity( 4 + size + 8 * num_items );

            // Now that the buffer has been possibly resized, cast it to
            // something a little easier to use.
            output = buffer;

            // Store the number of outer elements as an HLAinteger32BE (Big Endian).
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 0 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 3 );
            } else {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 3 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 0 );
            }
            int byte_count = 4;

            // ASCII characters in the string separated by possible Null
            // character '\0' padding between strings to stay on a 4 byte
            // boundary to keep to the standard.
            for ( int i = 0; i < num_items; ++i ) {

               s = *( static_cast< char ** >( ref2->address ) + i );

               int length = ( s != NULL ) ? strlen( s ) : 0;

               // The encoded size is an HLAinteger32BE.
               int encoded_size = ( length <= std::numeric_limits< int >::max() )
                                     ? length
                                     : std::numeric_limits< int >::max();

               // Store the number of elements as an HLAinteger32BE (Big Endian).
               if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
               } else {
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
               }
               byte_count += 4;

               if ( ( s != NULL ) && ( length > 0 ) ) {
                  // Copy the ASCII characters to the output.
                  memcpy( output, s, length );
                  output += length;
                  byte_count += length;
               }

               // Separate the strings by padding with null characters.
               // Pad to stay on a 4 byte boundary. The last element
               // gets no padding.
               if ( ( i < ( num_items - 1 ) ) && ( ( ( 4 + length ) % 4 ) != 0 ) ) {
                  int pad_cnt = 4 - ( ( 4 + length ) % 4 );
                  for ( int k = 0; k < pad_cnt; ++k ) {
                     *( output++ ) = '\0';
                  }
                  byte_count += pad_cnt;
               }
            }

            // The amount of data in the buffer (i.e. size) is the number of
            // bytes we placed in it.
            size = byte_count;
         }
         break;
      }
      case ENCODING_OPAQUE_DATA: {
         // HLAopaqueData format documented in IEEE Standard 1516.2-2000, which
         // will handle variable length binary data.

         if ( num_items == 1 ) {

            s = *( static_cast< char ** >( ref2->address ) );

            if ( s != NULL ) {
               // Get the number of bytes allocated to this variable by Trick.
               int trick_size = get_size( s );
               num_elements   = ( trick_size >= 0 ) ? trick_size : 0;
            } else {
               num_elements = 0;
            }

            // Encoded size is the number of elements (32 bit Big Endian)
            // followed by the data characters. Make sure we can hold the
            // encoded data.
            ensure_buffer_capacity( 4 + num_elements );

            // Now that the buffer has been possibly resized, cast it to
            // something a little easier to use.
            output = buffer;

            // The encoded size is an HLAinteger32BE.
            int encoded_size = ( num_elements <= std::numeric_limits< int >::max() )
                                  ? num_elements
                                  : std::numeric_limits< int >::max();

            // Store the number of elements as an HLAinteger32BE (Big Endian).
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
            } else {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
            }
            int byte_count = 4;

            // Copy the data to the output buffer.
            if ( ( s != NULL ) && ( num_elements > 0 ) ) {
               memcpy( output, s, num_elements );
               byte_count += num_elements;
            }

            // The amount of data in the buffer (i.e. size) is the encoded size.
            size = byte_count;

         } else if ( num_items > 1 ) {
            // We have more than one string to encode.

            // Number of HLAASCIIstring elements to be encoded in the
            // HLAvariableArray. The encoded size is an HLAinteger32BE.
            int num_outer_elements = ( num_items <= std::numeric_limits< int >::max() )
                                        ? num_items
                                        : std::numeric_limits< int >::max();

            // We need to determine the total number of bytes of data.
            num_elements = 0;
            for ( int i = 0; i < num_items; ++i ) {
               s = *( static_cast< char ** >( ref2->address ) + i );
               if ( s != NULL ) {
                  int trick_size = get_size( s );
                  if ( trick_size > 0 ) {
                     num_elements += trick_size;
                  }
               }
            }

            // Encoded size is the number of outer elements (HLAopaqueData)
            // then a HLAopaqueData element followed by an optional pad then
            // the  next HLAopaqueData element and so on. Each HLAopaqueData
            // element has a 4 byte element count with length characters and
            // up to 3 pad characters. 4 + length + 3 = 7 + length
            // In case we have a NULL string it won't show up in the
            // "num_elements" so add a character per item to guard against that.
            // 4: encoded size
            // (num_elements + num_items): all string characters + possible NULLs
            // 7 * num_items: 3 pad + 4 bytes for character count per element
            // encoded_size = 4 + (num_elements + num_items) + 7 * num_items
            // Make sure we can hold the encoded data.
            ensure_buffer_capacity( 4 + num_elements + 8 * num_items );

            // Now that the buffer has been possibly resized, cast it to
            // something a little easier to use.
            output = buffer;

            // Store the number of outer elements as an HLAinteger32BE (Big Endian).
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 0 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 3 );
            } else {
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 3 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 2 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 1 );
               *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &num_outer_elements ) ) + 0 );
            }
            int byte_count = 4;

            // Buffer contains the characters in the string separated by
            // possible Null character '\0' padding between strings to stay
            // on a 4 byte boundary to keep to the standard.
            for ( int i = 0; i < num_items; ++i ) {

               // Determine the length of the "char *" for the given array index.
               s              = *( static_cast< char ** >( ref2->address ) + i );
               int trick_size = ( s != NULL ) ? get_size( s ) : 0;
               int length     = ( trick_size >= 0 ) ? trick_size : 0;

               // The encoded size is an HLAinteger32BE.
               int encoded_size = ( length <= std::numeric_limits< int >::max() )
                                     ? length
                                     : std::numeric_limits< int >::max();

               // Store the number of elements as an HLAinteger32BE (Big Endian).
               if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
               } else {
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 3 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 2 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 1 );
                  *( output++ ) = *( ( reinterpret_cast< unsigned char * >( &encoded_size ) ) + 0 );
               }
               byte_count += 4;

               // Copy the data to the output buffer.
               if ( ( s != NULL ) && ( length > 0 ) ) {
                  memcpy( output, s, length );
                  output += length;
                  byte_count += length;
               }

               // Separate the strings by padding with null characters.
               // Pad to stay on a 4 byte boundary. The last element
               // gets no padding.
               if ( ( i < ( num_items - 1 ) ) && ( ( ( 4 + length ) % 4 ) != 0 ) ) {
                  int pad_cnt = 4 - ( ( 4 + length ) % 4 );
                  for ( int k = 0; k < pad_cnt; ++k ) {
                     *( output++ ) = '\0';
                  }
                  byte_count += pad_cnt;
               }
            }

            // The amount of data in the buffer (i.e. size) is the number of
            // bytes we placed in it.
            size = byte_count;
         }
         break;
      }
      case ENCODING_NONE: {
         // No-encoding of the data, just send the bytes as is.

         // Make sure we can hold the encoded data in the buffer.
         ensure_buffer_capacity( size );

         // Now that the buffer has been possibly resized, cast it to
         // something a little easier to use.
         output = buffer;

         // Offset from the start of the output buffer.
         int byte_count = 0;

         // Send the data bytes as is.
         for ( int i = 0; i < num_items; ++i ) {

            s = *( static_cast< char ** >( ref2->address ) + i );

            if ( s != NULL ) {
               int length = get_size( s );
               if ( length > 0 ) {
                  memcpy( output + byte_count, s, length );
                  byte_count += length;
               }
            }
         }

         // Check for an unexpected number of data bytes.
         if ( byte_count != size ) {
            ostringstream errmsg;
            errmsg << "Attribute::encode_string_to_buffer():" << __LINE__
                   << " ERROR: For ENCODING_NONE, Attribute '" << FOM_name
                   << "' with Trick name '" << trick_name << "', actual data size"
                   << " (" << byte_count << ") != expected Trick simulation variable"
                   << " size (" << size << ")!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // The amount of data in the buffer (i.e. size) is the number of
         // bytes we placed in it.
         size = byte_count;
         break;
      }
      case ENCODING_C_STRING:
      default: {

         // We are also sending the terminating null character, so make sure
         // the size reflects that.
         // Make sure we can hold the encoded data in the buffer.
         ensure_buffer_capacity( size + num_items );

         // Now that the buffer has been possibly resized, cast it to
         // something a little easier to use.
         output = buffer;

         // Offset from the start of the output buffer.
         int byte_count = 0;

         // Box-car encode the strings.
         for ( int i = 0; i < num_items; ++i ) {

            s = *( static_cast< char ** >( ref2->address ) + i );

            if ( s != NULL ) {
               // Include the null character as well.
               int length = strlen( s ) + 1;

               memcpy( output + byte_count, s, length );

               byte_count += length;
            } else {
               // For a NULL string, encode it as a zero length string.
               *( output + byte_count ) = '\0';
               ++byte_count;
            }
         }

         // The amount of data in the buffer (i.e. size) is the number of
         // bytes we placed in it.
         size = byte_count;
         break;
      }
   }
}

void Attribute::decode_string_from_buffer() // RETURN: -- None.
{
   unsigned char *input;
   unsigned char *output;
   int            num_elements;

   switch ( rti_encoding ) {
      case ENCODING_UNICODE_STRING: {
         // HLAunicodeString format documented in IEEE
         // Standard 1516.2-2000, Section 4.12.6
         input = buffer;

         // Are we expecting only one string?
         if ( num_items == 1 ) {

            // Decode the number of elements which is an HLAinteger32BE (Big Endian).
            int decoded_count = 0;
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
            } else {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
            }

            // Do a sanity check on the decoded length, it should not be negative.
            int length;
            if ( decoded_count < 0 ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING attribute '%s' (trick_name: '%s'), decoded length %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, ( ( trick_name != NULL ) ? trick_name : "NULL" ),
                                decoded_count );
               length = 0;
            } else {
               length = decoded_count;
            }

            // If the users Trick simulation variable is static in size then we
            // need to do a bounds check so that we don't overflow the users
            // variable.
            if ( size_is_static ) {
               int data_buff_size;
               if ( ref2->attr->type == TRICK_STRING ) {
                  if ( size > 4 ) {
                     data_buff_size = ( size - 4 ) / 2;
                  } else {
                     data_buff_size = 0;
                  }
               } else {
                  // TODO: We need to redo this assignment, should it be
                  // data_buff_size = (size - 4) / 2; ?
                  data_buff_size = size;
               }
               if ( length > data_buff_size ) {
                  message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING parameter '%s', decoded length %d > data buffer \
size %d, will use the data buffer size instead.\n",
                                   __LINE__, FOM_name, length, data_buff_size );
                  length = data_buff_size;
               }
            }

            // UTF-16 character encoding of the string.
            output = *( static_cast< unsigned char ** >( ref2->address ) );

            if ( output != NULL ) {

               // TODO: If the users Trick simulation variable is static in size
               // should we be reallocating memory for it below?

               // Determine if we need to allocate more memory for the sim string.
               // If it is larger than the existing string and larger than the
               // memory allocated for the string then reallocate more memory.
               if ( length >= get_size( output ) ) {

                  // Make sure to make room for the terminating null character,
                  // and add a few more bytes to give us a little more space
                  // for next time.
                  int array_size = Utilities::next_positive_multiple_of_8( length );

                  *( static_cast< char ** >( ref2->address ) ) =
                     static_cast< char * >( TMM_resize_array_1d_a(
                        *( static_cast< char ** >( ref2->address ) ), array_size ) );

                  output = *( static_cast< unsigned char ** >( ref2->address ) );
               }
            } else {
               // Allocate memory for the sim string and include room for the
               // terminating null character and add a few more bytes to give
               // us a little more space for next time.
               int array_size = Utilities::next_positive_multiple_of_8( length );

               *( static_cast< char ** >( ref2->address ) ) =
                  static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );

               output = *( static_cast< unsigned char ** >( ref2->address ) );
            }

            if ( output == NULL ) {
               ostringstream errmsg;
               errmsg << "Attribute::decode_string_from_buffer():" << __LINE__
                      << " ERROR: Could not allocate memory for ENCODING_UNICODE_STRING Attribute '"
                      << FOM_name << "' with Trick name '" << trick_name
                      << "' and length " << Utilities::next_positive_multiple_of_8( length )
                      << "!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            } else {

               // Decode the UTF-16 characters.
               for ( int k = 0; k < length; ++k ) {
                  input++; // skip the high-character of the UTF-16 encoding
                  output[k] = *( input++ );
               }

               // Add the terminating null character '\0';
               output[length] = '\0';
            }
         } else if ( num_items > 1 ) {
            // Or is it an array of multiple strings?

            // Decode the number of elements which is an HLAinteger32BE (Big Endian).
            int decoded_count = 0;
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
            } else {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
            }

            // Sanity check, we should not get a negative element count.
            if ( decoded_count < 0 ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING attribute '%s', decoded element count %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, decoded_count );
               num_elements = 0;
            } else {
               num_elements = decoded_count;
            }

            // Handle the situation where more strings are in the input encoding
            // than what exist in the ref-attributes.
            if ( num_elements > num_items ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: Truncating array of ENCODING_UNICODE_STRING from %d to %d elements for attribute '%s'!\n",
                                __LINE__, num_elements, num_items, FOM_name );
               num_elements = num_items;
            }

            // Calculate the number of UTF-16 characters which is the number of
            // bytes in the buffer minus the encoded length fields divided by 2.
            // data_buff_size = (size - 4 - 4 * num_elements)/2
            int data_buff_size;
            if ( ref2->attr->type == TRICK_STRING ) {
               if ( size > ( 4 * ( num_elements + 1 ) ) ) {
                  data_buff_size = ( size - ( 4 * ( num_elements + 1 ) ) ) / 2;
               } else {
                  data_buff_size = 0;
               }
            } else {
               // TODO: We need to redo this assignment, should it be
               // data_buff_size = (size - (4 * (num_elements + 1))) / 2; ?
               data_buff_size = size;
            }

            // Decode each of the HLAunicodeString elements.
            for ( int i = 0; i < num_elements; ++i ) {

               // Decode the length of the string which is an HLAinteger32BE (Big Endian).
               if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               } else {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               }

               // Do a sanity check on the decoded length, it should not be negative.
               int length;
               if ( decoded_count < 0 ) {
                  message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING array element %d, attribute '%s', the decoded \
length %d < 0, will use 0 instead.\n",
                                   __LINE__, i, FOM_name, decoded_count );
                  length = 0;
               } else {
                  length = decoded_count;
               }

               // Do a sanity check on the decoded length as compared to how much
               // data remains in the buffer.
               if ( length > data_buff_size ) {
                  message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING array element %d, attribute '%s', the decoded \
length %d > data buffer size %d, will use the data buffer size instead.\n",
                                   __LINE__, i, FOM_name, length, data_buff_size );
                  length = data_buff_size;
               }

               // Adjust the amount of data left in the buffer.
               if ( data_buff_size > length ) {
                  data_buff_size -= length;
               } else {
                  data_buff_size = 0;
               }

               // UTF-16 character encoding of the string.
               output = *( static_cast< unsigned char ** >( ref2->address ) + i );

               if ( output != NULL ) {

                  // Determine if we need to allocate more memory for the sim string.
                  // If it is larger than the existing string and larger than the
                  // memory allocated for the string then reallocate more memory.
                  if ( length >= get_size( output ) ) {

                     // Make sure to make room for the terminating null character,
                     // and add a few more bytes to give us a little more space
                     // for next time.
                     int array_size = Utilities::next_positive_multiple_of_8( length );

                     *( static_cast< char ** >( ref2->address ) + i ) =
                        static_cast< char * >( TMM_resize_array_1d_a(
                           *( static_cast< char ** >( ref2->address ) + i ), array_size ) );

                     output = *( static_cast< unsigned char ** >( ref2->address ) + i );
                  }
               } else {
                  // Allocate memory for the sim string and include room for the
                  // terminating null character and add a few more bytes to give
                  // us a little more space for next time.
                  int array_size = Utilities::next_positive_multiple_of_8( length );

                  *( static_cast< char ** >( ref2->address ) + i ) =
                     static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );

                  output = *( static_cast< unsigned char ** >( ref2->address ) + i );
               }

               if ( output == NULL ) {
                  ostringstream errmsg;
                  errmsg << "Attribute::decode_string_from_buffer():" << __LINE__
                         << " ERROR: Could not allocate memory for ENCODING_UNICODE_STRING"
                         << " Attribute '" << FOM_name << "' with Trick name '"
                         << trick_name << "' and length "
                         << Utilities::next_positive_multiple_of_8( length )
                         << "!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               } else {

                  // Decode the UTF-16 characters.
                  for ( int k = 0; k < length; ++k ) {
                     input++; // skip the high-character of the UTF-16 encoding
                     output[k] = *( input++ );
                  }

                  // Add the terminating null character '\0';
                  output[length] = '\0';
               }

               // Skip any pad added between strings which is used to stay
               // on a 4 byte boundary. The last element has no padding.
               if ( ( i < ( num_items - 1 ) ) && ( ( ( 4 + ( 2 * length ) ) % 4 ) != 0 ) ) {
                  input += 2;

                  // Adjust the amount of data left in the buffer, and remember
                  // the data buff size is the number of two-byte elements so
                  // subtract one since we removed one pad element.
                  if ( data_buff_size > 0 ) {
                     --data_buff_size;
                  }
               }
            }
         }
         break;
      }
      case ENCODING_ASCII_STRING: {
         // HLAASCIIstring format documented in IEEE
         // Standard 1516.2-2000, Section 4.12.6

         input = buffer;

         // Are we expecting only one string?
         if ( num_items == 1 ) {

            // Decode the number of elements which is an HLAinteger32BE (Big Endian).
            int decoded_count = 0;
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
            } else {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
            }

            // Do a sanity check on the decoded length, it should not be negative.
            int length;
            if ( decoded_count < 0 ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING attribute '%s', decoded length %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, decoded_count );
               length = 0;
            } else {
               length = decoded_count;
            }

            // Do a sanity check on the decoded length as compared to how much
            // data is in the buffer, i.e. data_buff_size = size - 4.
            int data_buff_size;
            if ( size > 4 ) {
               data_buff_size = size - 4;
            } else {
               data_buff_size = 0;
            }

            if ( length > data_buff_size ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING attribute '%s', decoded length %d > data buffer size \
%d, will use the data buffer size instead.\n",
                                __LINE__, FOM_name, length, data_buff_size );
               length = data_buff_size;
            }

            // ASCII character encoding of the string.
            output = *( static_cast< unsigned char ** >( ref2->address ) );

            if ( output != NULL ) {

               // Determine if we need to allocate more memory for the sim string.
               // If it is larger than the existing string and larger than the
               // memory allocated for the string then reallocate more memory.
               if ( length >= get_size( output ) ) {

                  // Make sure to make room for the terminating null character,
                  // and add a few more bytes to give us a little more space
                  // for next time.
                  int array_size = Utilities::next_positive_multiple_of_8( length );

                  *( static_cast< char ** >( ref2->address ) ) =
                     static_cast< char * >( TMM_resize_array_1d_a(
                        *( static_cast< char ** >( ref2->address ) ), array_size ) );

                  output = *( static_cast< unsigned char ** >( ref2->address ) );
               }
            } else {
               // Allocate memory for the sim string and include room for the
               // terminating null character and add a few more bytes to give
               // us a little more space for next time.
               int array_size = Utilities::next_positive_multiple_of_8( length );

               *( static_cast< char ** >( ref2->address ) ) =
                  static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );

               output = *( static_cast< unsigned char ** >( ref2->address ) );
            }

            if ( output == NULL ) {
               ostringstream errmsg;
               errmsg << "Attribute::decode_string_from_buffer():" << __LINE__
                      << " ERROR: Could not allocate memory for ENCODING_ASCII_STRING Attribute '"
                      << FOM_name << "' with Trick name '" << trick_name
                      << "' and length " << Utilities::next_positive_multiple_of_8( length )
                      << "!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            } else {

               // Copy the ASCII characters over.
               if ( length > 0 ) {
                  memcpy( output, input, length );
               }

               // Add the terminating null character '\0';
               output[length] = '\0';
            }
         } else if ( num_items > 1 ) {
            // Or is it an array of multiple strings?

            // Decode the number of elements which is an HLAinteger32BE (Big Endian).
            int decoded_count = 0;
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
            } else {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
            }

            // Sanity check, we should not get a negative element count.
            if ( decoded_count < 0 ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING attribute '%s', decoded element count %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, decoded_count );
               num_elements = 0;
            } else {
               num_elements = decoded_count;
            }

            // Handle the situation where more strings are in the input encoding
            // than what exist in the ref-attributes.
            if ( num_elements > num_items ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: Truncating array of ENCODING_ASCII_STRING from %d to %d elements for attribute '%s'!\n",
                                __LINE__, num_elements, num_items, FOM_name );
               num_elements = num_items;
            }

            // Calculate the size of the data minus the encoded length fields.
            // data_buff_size = size - 4 - 4 * num_elements
            int data_buff_size;
            if ( size > ( 4 * ( num_elements + 1 ) ) ) {
               data_buff_size = size - ( 4 * ( num_elements + 1 ) );
            } else {
               data_buff_size = 0;
            }

            // Decode each of the HLAASCIIstring elements.
            for ( int i = 0; i < num_elements; ++i ) {

               // Decode the length of the string which is an HLAinteger32BE (Big Endian).
               if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               } else {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               }

               // Do a sanity check on the decoded length, it should not be negative.
               int length;
               if ( decoded_count < 0 ) {
                  message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING array element %d, attribute '%s', the decoded \
length %d < 0, will use 0 instead.\n",
                                   __LINE__, i, FOM_name, decoded_count );
                  length = 0;
               } else {
                  length = decoded_count;
               }

               // Do a sanity check on the decoded length as compared to how much
               // data remains in the buffer.
               if ( length > data_buff_size ) {
                  message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING array element %d, attribute '%s', the decoded \
length %d > data buffer size %d, will use the data buffer size instead.\n",
                                   __LINE__, i, FOM_name, length, data_buff_size );
                  length = data_buff_size;
               }

               // Adjust the amount of data left in the buffer.
               if ( data_buff_size > 0 ) {
                  data_buff_size -= length;
               }

               // ASCII character encoding of the string.
               output = *( static_cast< unsigned char ** >( ref2->address ) + i );

               if ( output != NULL ) {

                  // Determine if we need to allocate more memory for the sim string.
                  // If it is larger than the existing string and larger than the
                  // memory allocated for the string then reallocate more memory.
                  if ( length >= get_size( output ) ) {

                     // Make sure to make room for the terminating null character,
                     // and add a few more bytes to give us a little more space
                     // for next time.
                     int array_size = Utilities::next_positive_multiple_of_8( length );

                     *( static_cast< char ** >( ref2->address ) + i ) =
                        static_cast< char * >( TMM_resize_array_1d_a(
                           *( static_cast< char ** >( ref2->address ) + i ), array_size ) );

                     output = *( static_cast< unsigned char ** >( ref2->address ) + i );
                  }
               } else {
                  // Allocate memory for the sim string and include room for the
                  // terminating null character and add a few more bytes to give
                  // us a little more space for next time.
                  int array_size = Utilities::next_positive_multiple_of_8( length );

                  *( static_cast< char ** >( ref2->address ) + i ) =
                     static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );

                  output = *( static_cast< unsigned char ** >( ref2->address ) + i );
               }

               if ( output == NULL ) {
                  ostringstream errmsg;
                  errmsg << "Attribute::decode_string_from_buffer():" << __LINE__
                         << " ERROR: Could not allocate memory for ENCODING_ASCII_STRING"
                         << " Attribute '" << FOM_name << "' with Trick name '"
                         << trick_name << "' and length "
                         << Utilities::next_positive_multiple_of_8( length )
                         << "!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               } else {

                  // Copy the ASCII characters over.
                  if ( length > 0 ) {
                     memcpy( output, input, length );
                     input += length;
                  }

                  // Add the terminating null character '\0';
                  output[length] = '\0';
               }

               // Skip the padding which was added to keep the data on a 4 byte
               // boundary. The last element gets no padding.
               if ( ( i < ( num_items - 1 ) ) && ( ( ( 4 + length ) % 4 ) != 0 ) ) {
                  input += ( 4 - ( ( 4 + length ) % 4 ) );

                  // Adjust the amount of data left in the buffer.
                  if ( data_buff_size > ( 4 - ( ( 4 + length ) % 4 ) ) ) {
                     data_buff_size -= ( 4 - ( ( 4 + length ) % 4 ) );
                  } else {
                     data_buff_size = 0;
                  }
               }
            }
         }
         break;
      }
      case ENCODING_OPAQUE_DATA: {
         // HLAopaqueData format documented in IEEE Standard 1516.2-2000, which
         // will handle variable length binary data.

         input = buffer;

         // Are we expecting only one string?
         if ( num_items == 1 ) {

            // Decode the number of elements which is an HLAinteger32BE (Big Endian).
            int decoded_count = 0;
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
            } else {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
            }

            // Do a sanity check on the decoded length, it should not be negative.
            int length;
            if ( decoded_count < 0 ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA attribute '%s', decoded length %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, decoded_count );
               length = 0;
            } else {
               length = decoded_count;
            }

            // Do a sanity check on the decoded length as compared to how much
            // data is in the buffer, i.e. data_buff_size = size - 4.
            int data_buff_size;
            if ( size > 4 ) {
               data_buff_size = size - 4;
            } else {
               data_buff_size = 0;
            }
            if ( length > data_buff_size ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA attribute '%s', decoded length %d > data buffer size \
%d, will use the data buffer size instead.\n",
                                __LINE__, FOM_name, length, data_buff_size );
               length = data_buff_size;
            }

            // Get a pointer to the output.
            output = *( static_cast< unsigned char ** >( ref2->address ) );

            if ( output != NULL ) {
               // The output array size must exactly match the incoming data
               // size for opaque data.
               if ( length != get_size( output ) ) {
                  // WORKAROUND: Trick 10 can't handle a length of zero so to
                  // workaround the memory manager problem use a size of 1 in
                  // the allocation.
                  *( static_cast< char ** >( ref2->address ) ) =
                     static_cast< char * >( TMM_resize_array_1d_a( *( static_cast< char ** >( ref2->address ) ),
                                                                   ( ( length > 0 ) ? length : 1 ) ) );

                  output = *( static_cast< unsigned char ** >( ref2->address ) );
               }
            } else {
               // Allocate memory for the output array.
               // WORKAROUND: Trick 10 can't handle a length of zero so to
               // workaround the memory manager problem use a size of 1 in
               // the allocation.
               *( static_cast< char ** >( ref2->address ) ) =
                  static_cast< char * >( TMM_declare_var_1d( "char", ( ( length > 0 ) ? length : 1 ) ) );

               output = *( static_cast< unsigned char ** >( ref2->address ) );
            }

            if ( output == NULL ) {
               ostringstream errmsg;
               errmsg << "Attribute::decode_string_from_buffer():" << __LINE__
                      << " ERROR: Could not allocate memory for ENCODING_OPAQUE_DATA Attribute '"
                      << FOM_name << "' with Trick name '" << trick_name
                      << "' and length " << length << "!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }

            // Copy the characters over.
            if ( length > 0 ) {
               memcpy( output, input, length );
            }

         } else if ( num_items > 1 ) {
            // Or is it an array of multiple strings?

            // Decode the number of elements which is an HLAinteger32BE (Big Endian).
            int decoded_count = 0;
            if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
            } else {
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
               *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
            }

            // Sanity check, we should not get a negative element count.
            if ( decoded_count < 0 ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA attribute '%s', decoded element count %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, decoded_count );
               num_elements = 0;
            } else {
               num_elements = decoded_count;
            }

            // Handle the situation where more strings are in the input encoding
            // than what exist in the ref-attributes.
            if ( num_elements > num_items ) {
               message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: Truncating array of ENCODING_OPAQUE_DATA from %d to %d elements for attribute '%s'!\n",
                                __LINE__, num_elements, num_items, FOM_name );
               num_elements = num_items;
            }

            // Calculate the size of the data minus the encoded length fields.
            // data_buff_size = size - 4 - 4 * num_elements
            int data_buff_size;
            if ( size > ( 4 * ( num_elements + 1 ) ) ) {
               data_buff_size = size - ( 4 * ( num_elements + 1 ) );
            } else {
               data_buff_size = 0;
            }

            // Decode each of the HLAASCIIstring elements.
            for ( int i = 0; i < num_elements; ++i ) {

               // Decode the length of the string which is an HLAinteger32BE (Big Endian).
               if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               } else {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               }

               // Do a sanity check on the decoded length, it should not be negative.
               int length;
               if ( decoded_count < 0 ) {
                  message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA array element %d, attribute '%s', the decoded \
length %d < 0, will use 0 instead.\n",
                                   __LINE__, i, FOM_name, decoded_count );
                  length = 0;
               } else {
                  length = decoded_count;
               }

               // Do a sanity check on the decoded length as compared to how much
               // data remains in the buffer.
               if ( length > data_buff_size ) {
                  message_publish( MSG_WARNING, "Attribute::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA array element %d, attribute '%s', the decoded \
length %d > data buffer size %d, will use the data buffer size instead.\n",
                                   __LINE__, i, FOM_name, length, data_buff_size );
                  length = data_buff_size;
               }

               // Adjust the amount of data left in the buffer.
               if ( data_buff_size > length ) {
                  data_buff_size -= length;
               } else {
                  data_buff_size = 0;
               }

               // 8-bit characters
               output = *( static_cast< unsigned char ** >( ref2->address ) + i );

               if ( output != NULL ) {
                  // The output array size must exactly match the incoming data
                  // size for opaque data.
                  if ( length != get_size( output ) ) {
                     // WORKAROUND: Trick 10 can't handle a length of zero so to
                     // workaround the memory manager problem use a size of 1 in
                     // the allocation.
                     *( static_cast< char ** >( ref2->address ) + i ) =
                        static_cast< char * >( TMM_resize_array_1d_a(
                           *( static_cast< char ** >( ref2->address ) + i ),
                           ( ( length > 0 ) ? length : 1 ) ) );

                     output = *( static_cast< unsigned char ** >( ref2->address ) + i );
                  }
               } else {
                  // Allocate memory for the output array.
                  // WORKAROUND: Trick 10 can't handle a length of zero so to
                  // workaround the memory manager problem use a size of 1 in
                  // the allocation.
                  *( static_cast< char ** >( ref2->address ) + i ) = static_cast< char * >(
                     TMM_declare_var_1d( "char", ( ( length > 0 ) ? length : 1 ) ) );

                  output = *( static_cast< unsigned char ** >( ref2->address ) + i );
               }

               if ( output == NULL ) {
                  ostringstream errmsg;
                  errmsg << "Attribute::decode_string_from_buffer():" << __LINE__
                         << " ERROR: Could not allocate memory for ENCODING_OPAQUE_DATA"
                         << " Attribute '" << FOM_name << "' with Trick name '"
                         << trick_name << "' and length " << length << "!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }

               // Copy the characters over.
               if ( length > 0 ) {
                  memcpy( output, input, length );
                  input += length;
               }

               // Skip the padding which was added to keep the data on a 4 byte
               // boundary. The last element gets no padding.
               if ( ( i < ( (int)num_items - 1 ) ) && ( ( ( 4 + length ) % 4 ) != 0 ) ) {
                  input += ( 4 - ( ( 4 + length ) % 4 ) );

                  // Adjust the amount of data left in the buffer.
                  if ( data_buff_size > ( 4 - ( ( 4 + length ) % 4 ) ) ) {
                     data_buff_size -= ( 4 - ( ( 4 + length ) % 4 ) );
                  } else {
                     data_buff_size = 0;
                  }
               }
            }
         }
         break;
      }
      case ENCODING_NONE: {
         // No-Encoding of the data, just use it as is.

         input = buffer;

         // Get a pointer to the output.
         output = *( static_cast< unsigned char ** >( ref2->address ) );

         if ( output == NULL ) {
            ostringstream errmsg;
            errmsg << "Attribute::decode_string_from_buffer():" << __LINE__
                   << " ERROR: For ENCODING_NONE, Attribute '" << FOM_name
                   << "' with Trick name '" << trick_name << "' is NULL!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // The existing output "char *" variable size must exactly match the
         // incoming data size for no-encoding of the data.
         if ( (int)size != get_size( output ) ) {
            ostringstream errmsg;
            errmsg << "Attribute::decode_string_from_buffer():" << __LINE__
                   << " ERROR: For ENCODING_NONE, Attribute '" << FOM_name
                   << "' with Trick name '" << trick_name << "', received data"
                   << " size (" << size << ") != Trick simulation variable size ("
                   << get_size( output ) << ")!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // Copy the characters over.
         if ( size > 0 ) {
            memcpy( output, input, size );
         }
         break;
      }
      case ENCODING_C_STRING:
      default: {
         int start_index = 0;
         int end_index   = 0;

         input = buffer;

         // Decode the box-car encoded strings.
         for ( int i = 0; i < (int)num_items; ++i ) {

            // Find the end of the encoded string which is the null character.
            while ( *( input + end_index ) != '\0' ) {
               ++end_index;
            }

            int length = ( end_index - start_index ) + 1;

            output = *( static_cast< unsigned char ** >( ref2->address ) + i );

            if ( output != NULL ) {

               // Determine if we need to allocate more memory for the sim string.
               // TODO: Find a more efficient way to determine if we need to
               // reallocate memory for the string.
               if ( length >= get_size( output ) ) {
                  int array_size = Utilities::next_positive_multiple_of_8( length );

                  *( static_cast< char ** >( ref2->address ) + i ) = static_cast< char * >( TMM_resize_array_1d_a(
                     *( static_cast< char ** >( ref2->address ) + i ), array_size ) );

                  output = *( static_cast< unsigned char ** >( ref2->address ) + i );
               }
            } else {
               // Allocate memory for the sim string.
               int array_size = Utilities::next_positive_multiple_of_8( length );

               *( static_cast< char ** >( ref2->address ) + i ) = static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );

               output = *( static_cast< unsigned char ** >( ref2->address ) + i );
            }

            if ( output == NULL ) {
               ostringstream errmsg;
               errmsg << "Attribute::decode_string_from_buffer():" << __LINE__
                      << " ERROR: Could not allocate memory for ENCODING_C_STRING attribute '"
                      << FOM_name << "' with Trick name '" << trick_name
                      << "' and length " << Utilities::next_positive_multiple_of_8( length )
                      << "!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            } else {

               memcpy( output, ( input + start_index ), length );

               // Move to the next encoded string in the input.
               ++end_index;
               start_index = end_index;
            }
         }
         break;
      }
   }
}

/*! @details
 * \par<b>Assumptions and Limitations:</b>
 * - The destination must be large enough to hold num_bytes of data.
 * - Only primitive types and static arrays of primitive type are supported for now.
 */
void Attribute::byteswap_buffer_copy( // RETURN: -- None.
   void       *dest,                  // IN: -- Destination to copy data to.
   void const *src,                   // IN: -- Source of the data to byteswap and copy from.
   int const   type,                  // IN: -- The type of the data.
   int const   length,                // IN: -- The length/number of entries in the source array.
   int const   num_bytes ) const        // IN: -- The number of bytes in the source array.
{
   if ( num_bytes == 0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
         ostringstream msg;
         msg << "Attribute::byteswap_buffer_copy():" << __LINE__
             << " WARNING: FOM Attribute '" << FOM_name << "' with Trick name '"
             << trick_name << "' has an unexpected size of zero bytes! Make"
             << " sure your simulation variable is properly initialized before"
             << " this initialize() function is called.\n";
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
      return;
   }

   // Determine if we can just copy the data between the two buffers since
   // we don't need to byteswap or do any special encoding.
   if ( ( !byteswap ) || ( rti_encoding == ENCODING_NONE ) ) {

      // Copy the source into the destination since there is no byteswaping
      // or any special encoding.
      memcpy( dest, src, num_bytes );

   } else {

      // Do the byteswap based on the type.
      switch ( type ) {
         case TRICK_DOUBLE: {
            double const *d_src = static_cast< double const * >( src );

            double *d_dest = static_cast< double * >( dest );
            if ( length == 1 ) {
               d_dest[0] = Utilities::byteswap_double( d_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  d_dest[k] = Utilities::byteswap_double( d_src[k] );
               }
            }
            break;
         }
         case TRICK_FLOAT: {
            float const *f_src = static_cast< float const * >( src );

            float *f_dest = static_cast< float * >( dest );
            if ( length == 1 ) {
               f_dest[0] = Utilities::byteswap_float( f_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  f_dest[k] = Utilities::byteswap_float( f_src[k] );
               }
            }
            break;
         }
         case TRICK_CHARACTER:
         case TRICK_UNSIGNED_CHARACTER:
         case TRICK_BOOLEAN: {
            // No byteswap needed.
            memcpy( dest, src, num_bytes );
            break;
         }
         case TRICK_SHORT: {
            short const *s_src  = static_cast< short const * >( src );
            short       *s_dest = static_cast< short * >( dest );
            if ( length == 1 ) {
               s_dest[0] = Utilities::byteswap_short( s_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  s_dest[k] = Utilities::byteswap_short( s_src[k] );
               }
            }
            break;
         }
         case TRICK_UNSIGNED_SHORT: {
            unsigned short const *us_src  = static_cast< unsigned short const * >( src );
            unsigned short       *us_dest = static_cast< unsigned short * >( dest );
            if ( length == 1 ) {
               us_dest[0] = Utilities::byteswap_unsigned_short( us_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  us_dest[k] = Utilities::byteswap_unsigned_short( us_src[k] );
               }
            }
            break;
         }
         case TRICK_INTEGER: {
            int const *i_src  = static_cast< int const * >( src );
            int       *i_dest = static_cast< int * >( dest );
            if ( length == 1 ) {
               i_dest[0] = Utilities::byteswap_int( i_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  i_dest[k] = Utilities::byteswap_int( i_src[k] );
               }
            }
            break;
         }
         case TRICK_UNSIGNED_INTEGER: {
            unsigned int const *ui_src  = static_cast< unsigned int const * >( src );
            unsigned int       *ui_dest = static_cast< unsigned int * >( dest );
            if ( length == 1 ) {
               ui_dest[0] = Utilities::byteswap_unsigned_int( ui_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  ui_dest[k] = Utilities::byteswap_unsigned_int( ui_src[k] );
               }
            }
            break;
         }
         case TRICK_LONG: {
            long const *l_src  = static_cast< long const * >( src );
            long       *l_dest = static_cast< long * >( dest );
            if ( length == 1 ) {
               l_dest[0] = Utilities::byteswap_long( l_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  l_dest[k] = Utilities::byteswap_long( l_src[k] );
               }
            }
            break;
         }
         case TRICK_UNSIGNED_LONG: {
            unsigned long const *ul_src  = static_cast< unsigned long const * >( src );
            unsigned long       *ul_dest = static_cast< unsigned long * >( dest );
            if ( length == 1 ) {
               ul_dest[0] = Utilities::byteswap_unsigned_long( ul_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  ul_dest[k] = Utilities::byteswap_unsigned_long( ul_src[k] );
               }
            }
            break;
         }
         case TRICK_LONG_LONG: {
            long long const *ll_src  = static_cast< long long const * >( src );
            long long       *ll_dest = static_cast< long long * >( dest );
            if ( length == 1 ) {
               ll_dest[0] = Utilities::byteswap_long_long( ll_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  ll_dest[k] = Utilities::byteswap_long_long( ll_src[k] );
               }
            }
            break;
         }
         case TRICK_UNSIGNED_LONG_LONG: {
            unsigned long long const *ull_src  = static_cast< unsigned long long const * >( src );
            unsigned long long       *ull_dest = static_cast< unsigned long long * >( dest );
            if ( length == 1 ) {
               ull_dest[0] = Utilities::byteswap_unsigned_long_long( ull_src[0] );
            } else {
               for ( int k = 0; k < length; ++k ) {
                  ull_dest[k] = Utilities::byteswap_unsigned_long_long( ull_src[k] );
               }
            }
            break;
         }
         default: {
            // Default case is to treat the data as a byte array with NO byteswap.
            memcpy( dest, src, num_bytes );
            break;
         }
      }
   }
}

/*! @details
 * \par<b>Assumptions and Limitations:</b>
 * - Only primitive types and static arrays of primitive type are supported for now.
 */
bool Attribute::is_supported_attribute_type() const // RETURN: -- True if supported, false otherwise.
{
   if ( ref2->attr == NULL ) {
      return false;
   }

   // Allow 1-D dynamic arrays: 'char *', 'unsigned char *' and 'bool *' etc.
   if ( ref2->attr->type != TRICK_STRING ) {
      // For now, we do not support more than a 1-D array that is dynamic
      // (i.e. a pointer such as char *). If the size of the last indexed
      // attribute is zero then it is a pointer.
      if ( ( ref2->attr->num_index > 1 ) && ( ref2->attr->index[ref2->attr->num_index - 1].size == 0 ) ) {
         return false;
      }
   } else {
      // String type:
      // We only support static arrays for now so check for a pointer to an
      // array (i.e. non-static arrays) which is when we have an array
      // (i.e. num_index > 0) and the size of any of the dimensions is zero
      // (i.e. index[i].size == 0).
      if ( ref2->attr->num_index > 0 ) {
         // Make sure each dimension is statically defined (i.e. not zero).
         for ( int i = ref2->attr->num_index - 1; i >= 0; i-- ) {
            // If the size is zero then the array is dynamic in size (i.e. a pointer)
            if ( ref2->attr->index[i].size == 0 ) {
               return false;
            }
         }
      }
   }

   switch ( ref2->attr->type ) {
      case TRICK_BOOLEAN: {
         return ( ( rti_encoding == ENCODING_BIG_ENDIAN )
                  || ( rti_encoding == ENCODING_LITTLE_ENDIAN )
                  || ( rti_encoding == ENCODING_BOOLEAN )
                  || ( rti_encoding == ENCODING_UNKNOWN )
                  || ( rti_encoding == ENCODING_NONE ) );
      }
      case TRICK_CHARACTER:
      case TRICK_UNSIGNED_CHARACTER: {
         return ( ( rti_encoding == ENCODING_BIG_ENDIAN )
                  || ( rti_encoding == ENCODING_LITTLE_ENDIAN )
                  || ( rti_encoding == ENCODING_UNKNOWN )
                  || ( rti_encoding == ENCODING_UNICODE_STRING )
                  || ( rti_encoding == ENCODING_OPAQUE_DATA )
                  || ( rti_encoding == ENCODING_NONE ) );
      }
      case TRICK_STRING: {
         // Only support an 1-D array of characters (char *) for ENCODING_NONE.
         if ( ( rti_encoding == ENCODING_NONE ) && ( ref2->attr->num_index != 0 ) ) {
            return false;
         }
         return ( ( rti_encoding == ENCODING_C_STRING )
                  || ( rti_encoding == ENCODING_UNICODE_STRING )
                  || ( rti_encoding == ENCODING_ASCII_STRING )
                  || ( rti_encoding == ENCODING_OPAQUE_DATA )
                  || ( rti_encoding == ENCODING_UNKNOWN )
                  || ( rti_encoding == ENCODING_NONE ) );
      }
      case TRICK_DOUBLE:
      case TRICK_FLOAT:
      case TRICK_SHORT:
      case TRICK_UNSIGNED_SHORT:
      case TRICK_INTEGER:
      case TRICK_UNSIGNED_INTEGER:
      case TRICK_LONG:
      case TRICK_UNSIGNED_LONG:
      case TRICK_LONG_LONG:
      case TRICK_UNSIGNED_LONG_LONG: {
         // We do not support an array of primitive types for the logical
         // time encoding, otherwise we support everything else.
         if ( ( rti_encoding == ENCODING_LOGICAL_TIME ) && ( ref2->attr->num_index > 0 ) ) {
            return false;
         }
         return ( ( rti_encoding == ENCODING_BIG_ENDIAN )
                  || ( rti_encoding == ENCODING_LITTLE_ENDIAN )
                  || ( rti_encoding == ENCODING_LOGICAL_TIME )
                  || ( rti_encoding == ENCODING_UNKNOWN )
                  || ( rti_encoding == ENCODING_NONE ) );
      }
      default: {
         return false; // Type not supported
      }
   }
   return false; // If we made it to here then the type is not supported.
}

void Attribute::print_buffer() const
{
   ostringstream msg;
   msg << "Attribute::print_buffer():" << __LINE__ << '\n'
       << " FOM-name:'" << get_FOM_name() << "'"
       << " type:" << ref2->attr->type
       << " byteswap:" << ( is_byteswap() ? "Yes" : "No" )
       << " num_items:" << num_items
       << " size:" << size
       << '\n';

   // For now we only support an attribute of type double for printing. DDexter
   if ( ref2->attr->type == TRICK_DOUBLE ) {

      double const *dbl_array = reinterpret_cast< double const * >( buffer ); // cppcheck-suppress [invalidPointerCast]

      if ( is_byteswap() ) {
         for ( int i = 0; i < num_items; ++i ) {
            // Undo the byteswap for display
            double b_value = Utilities::byteswap_double( dbl_array[i] );
            msg << "\ti:" << i
                << " value:" << b_value
                << " byteswap-value:" << dbl_array[i] << '\n';
         }
      } else {
         for ( int i = 0; i < num_items; ++i ) {
            msg << " i:" << i << " " << dbl_array[i] << '\n';
         }
      }
   } else {

      // Else just treat the buffer as an array of characters.
      char const *char_array = reinterpret_cast< char * >( buffer );

      msg << "\tAttribute size:" << size << '\n'
          << "\tIndex\tValue\tCharacter\n";

      for ( int i = 0; i < size; ++i ) {
         int char_value = char_array[i];
         msg << "\t" << i << "\t" << char_value;
         if ( isgraph( char_array[i] ) ) {
            msg << "\t" << char_array[i];
         }
         msg << '\n';
      }
   }
   message_publish( MSG_NORMAL, msg.str().c_str() );
}
