/*!
@file TrickHLA/Parameter.cpp
@ingroup TrickHLA
@brief This class represents the HLA parameters of an interaction that is
managed by Trick.

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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Parameter.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Aug 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
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
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Parameter.hh"
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
 * @job_class{initialization}
 */
Parameter::Parameter()
   : trick_name( NULL ),
     FOM_name( NULL ),
     rti_encoding( ENCODING_UNKNOWN ),
     buffer( NULL ),
     buffer_capacity( 0 ),
     size_is_static( true ),
     size( 0 ),
     num_items( 0 ),
     value_changed( false ),
     byteswap( false ),
     address( NULL ),
     attr( NULL ),
     interaction_FOM_name( NULL )
{
   // The value is set based on the Endianness of this computer.
   // HLAtrue is a value of 1 on a Big Endian computer.
   HLAtrue = ( ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) ? 1 : 0x1000000 );
}

/*!
 * @details Frees the Trick allocated memory.
 * @job_class{shutdown}
 */
Parameter::~Parameter()
{
   if ( buffer != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( buffer ) ) ) {
         message_publish( MSG_WARNING, "Parameter::~Parameter():%d WARNING failed to delete Trick Memory for 'buffer'\n",
                          __LINE__ );
      }
      buffer          = NULL;
      buffer_capacity = 0;
   }

   if ( interaction_FOM_name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( interaction_FOM_name ) ) ) {
         message_publish( MSG_WARNING, "Parameter::~Parameter():%d WARNING failed to delete Trick Memory for 'interaction_FOM_name'\n",
                          __LINE__ );
      }
      interaction_FOM_name = NULL;
   }
}

/*!
 * @job_class{initialization}
 */
void Parameter::initialize(
   char const *interaction_fom_name,
   int const   interaction_index,
   int const   parameter_index )
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Make sure we have a valid parameter FOM name.
   if ( ( FOM_name == NULL ) || ( *FOM_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize():" << __LINE__
             << " ERROR: Interaction with FOM Name '"
             << interaction_fom_name << "' has a missing FOM name for the"
             << " parameter. Make sure 'THLA.manager.interactions["
             << interaction_index << "].parameters[" << parameter_index
             << "].FOM_name' in either your input.py file or modified-data files"
             << " is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make sure we have a valid parameter Trick-Name.
   if ( ( trick_name == NULL ) || ( *trick_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize():" << __LINE__
             << " ERROR: FOM Interaction Parameter '"
             << interaction_fom_name << "'->'" << FOM_name << "' has a missing"
             << " Trick name for the parameter. Make sure 'THLA.manager.interactions["
             << interaction_index << "].parameters[" << parameter_index
             << "].trick_name' in either your input.py file or modified-data files"
             << " is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Do a quick bounds check on the rti_encoding value.
   if ( ( rti_encoding < ENCODING_FIRST_VALUE ) || ( rti_encoding > ENCODING_LAST_VALUE ) ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize():" << __LINE__
             << " ERROR: FOM Interaction Parameter '"
             << interaction_fom_name << "'->'" << FOM_name << "' with Trick name '"
             << trick_name << "' has an 'rti_encoding' value of "
             << rti_encoding << " which is out of the valid range of "
             << ENCODING_FIRST_VALUE << " to " << ENCODING_LAST_VALUE
             << ". Please check your input or modified-data files to make sure"
             << " the value for the 'rti_encoding' is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the ref-attributes.
   REF2 *ref2 = ref_attributes( trick_name );

   // Determine if we had an error getting the ref-attributes.
   if ( ref2 == (REF2 *)NULL ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize():" << __LINE__
             << " ERROR: FOM Interaction Parameter '"
             << interaction_fom_name << "'->'" << FOM_name
             << "' Error retrieving Trick ref-attributes for '" << trick_name
             << "'. Please check your input or modified-data files to make sure"
             << " the interaction parameter Trick name is correctly specified."
             << " If '" << trick_name << "' is an inherited variable then make"
             << " sure the base class uses either the 'public' or 'protected'"
             << " access level for the variable.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      address              = ref2->address;
      attr                 = ref2->attr;
      interaction_FOM_name = trick_MM->mm_strdup( const_cast< char * >( interaction_fom_name ) );

      // Free the memory used by ref2.
      free( ref2 );
      ref2 = NULL;

      complete_initialization();
   }
}

/*!
 * @job_class{initialization}
 */
void Parameter::initialize(
   char const *interaction_fom_name,
   void       *in_addr,
   ATTRIBUTES *in_attr )
{
   this->address = in_addr;
   this->attr    = in_attr;

   if ( this->address == NULL ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize(char const *, void *, ATTRIBUTES *):" << __LINE__
             << " ERROR: For FOM Interaction Parameter '" << interaction_fom_name
             << "'. Unexpected NULL trick variable address.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   if ( this->attr == NULL ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize(char const *, void *, ATTRIBUTES *):" << __LINE__
             << " ERROR: For FOM Interaction Parameter '" << interaction_fom_name
             << "'. Unexpected NULL ATTRIBUTES pointer.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   interaction_FOM_name = trick_MM->mm_strdup( const_cast< char * >( interaction_fom_name ) );

   complete_initialization();
}

/*!
 * @job_class{initialization}
 */
void Parameter::complete_initialization()
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Verify that the rti_encoding value is valid given the ref-attributes type.
   switch ( attr->type ) {
      case TRICK_BOOLEAN: {
         if ( ( rti_encoding != ENCODING_BIG_ENDIAN )
              && ( rti_encoding != ENCODING_LITTLE_ENDIAN )
              && ( rti_encoding != ENCODING_BOOLEAN )
              && ( rti_encoding != ENCODING_NONE )
              && ( rti_encoding != ENCODING_UNKNOWN ) ) {
            ostringstream errmsg;
            errmsg << "Parameter::complete_initialization():" << __LINE__
                   << " ERROR: FOM Interaction Parameter '"
                   << interaction_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' must use either the ENCODING_BIG_ENDIAN, "
                   << " ENCODING_LITTLE_ENDIAN, ENCODING_BOOLEAN, ENCODING_NONE, or "
                   << "ENCODING_UNKNOWN value for the 'rti_encoding' when the "
                   << "parameter represents a 'bool' type. Please check your input "
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
            errmsg << "Parameter::complete_initialization():" << __LINE__
                   << " ERROR: FOM Interaction Parameter '"
                   << interaction_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' must use either the ENCODING_BIG_ENDIAN,"
                   << " ENCODING_LITTLE_ENDIAN, ENCODING_NONE, ENCODING_UNICODE_STRING,"
                   << " ENCODING_OPAQUE_DATA, or ENCODING_UNKNOWN, value for the"
                   << " 'rti_encoding' when the parameter  represents a 'char' or"
                   << " 'unsigned char' type. Please check your input or"
                   << " modified-data files to make sure the value for the"
                   << " 'rti_encoding' is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // For the ENCODING_UNICODE_STRING encoding, we only support a 1-D dynamic
         // array of characters.
         if ( ( rti_encoding == ENCODING_UNICODE_STRING )
              && ( ( attr->num_index != 1 ) || ( attr->index[attr->num_index - 1].size != 0 ) ) ) {
            ostringstream errmsg;
            errmsg << "Parameter::complete_initialization():" << __LINE__
                   << " ERROR: FOM Interaction Parameter '"
                   << interaction_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' and 'rti_encoding' of ENCODING_UNICODE_STRING must"
                   << " represent a one-dimensional array of characters (i.e."
                   << " 'char *' or 'unsigned char *'). Please check your input or"
                   << " modified-data files to make sure the value for the"
                   << " 'rti_encoding' is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // For the ENCODING_OPAQUE_DATA encoding, we only support a 1-D dynamic
         // array of characters.
         if ( ( rti_encoding == ENCODING_OPAQUE_DATA )
              && ( ( attr->num_index != 1 ) || ( attr->index[attr->num_index - 1].size != 0 ) ) ) {
            ostringstream errmsg;
            errmsg << "Parameter::complete_initialization():" << __LINE__
                   << " ERROR: FOM Interaction Parameter '"
                   << interaction_FOM_name << "'->'" << FOM_name
                   << "' with Trick name '" << trick_name
                   << "' and 'rti_encoding' of ENCODING_OPAQUE_DATA must"
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
         if ( ( rti_encoding != ENCODING_LOGICAL_TIME )
              && ( rti_encoding != ENCODING_BIG_ENDIAN )
              && ( rti_encoding != ENCODING_LITTLE_ENDIAN )
              && ( rti_encoding != ENCODING_NONE )
              && ( rti_encoding != ENCODING_UNKNOWN ) ) {
            ostringstream errmsg;
            errmsg << "Parameter::complete_initialization():" << __LINE__
                   << " ERROR: FOM Interaction Parameter '"
                   << interaction_FOM_name << "'->'" << FOM_name << "' with Trick name '"
                   << trick_name << "' must use either the ENCODING_LOGICAL_TIME,"
                   << " ENCODING_BIG_ENDIAN, ENCODING_LITTLE_ENDIAN, ENCODING_NONE, or "
                   << "ENCODING_UNKNOWN value for the 'rti_encoding' when the "
                   << "parameter represents a primitive type. Please check your "
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
            errmsg << "Parameter::complete_initialization():" << __LINE__
                   << " ERROR: FOM Interaction Parameter '"
                   << interaction_FOM_name << "'->'" << FOM_name
                   << "' with Trick name '" << trick_name
                   << "' must use either the ENCODING_C_STRING, ENCODING_UNICODE_STRING,"
                   << " ENCODING_ASCII_STRING, ENCODING_OPAQUE_DATA, ENCODING_NONE, or "
                   << "ENCODING_UNKNOWN value for the 'rti_encoding' when the "
                   << "parameter represents a String type (i.e. char *). Please "
                   << "check your input or modified-data files to make sure the "
                   << "value for the 'rti_encoding' is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // Only support an array of characters (i.e. char *) for ENCODING_NONE.
         if ( ( rti_encoding == ENCODING_NONE ) && ( attr->num_index != 0 ) ) {
            ostringstream errmsg;
            errmsg << "Parameter::complete_initialization():" << __LINE__
                   << " ERROR: FOM Interaction Parameter '"
                   << interaction_FOM_name << "'->'" << FOM_name
                   << "' with Trick name '"
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

   // Allow 1-D dynamic arrays: 'char *', 'unsigned char *' and 'bool *' etc.
   if ( attr->type != TRICK_STRING ) {
      // For now, we do not support more than a 1-D array that is dynamic
      // (i.e. a pointer such as char *). If the size of the last indexed
      // attribute is zero then it is a pointer.
      if ( ( attr->num_index > 1 ) && ( attr->index[attr->num_index - 1].size == 0 ) ) {
         ostringstream errmsg;
         errmsg << "Parameter::complete_initialization():" << __LINE__
                << " ERROR: FOM Interaction Parameter '"
                << interaction_FOM_name << "'->'" << FOM_name
                << "' with Trick name '" << trick_name << "' and type '"
                << attr->type_name << "' is a "
                << attr->num_index << "-dimensional dynamic array."
                << " Only one-dimensional dynamic arrays are supported for now.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   } else {
      // String type:
      // We only support static arrays for now so check for a pointer to an
      // array (i.e. non-static arrays) which is when we have an array
      // (i.e. num_index > 0) and the size of any of the dimensions is zero
      // (i.e. index[i].size == 0).
      if ( attr->num_index > 0 ) {
         // Make sure each dimension is statically defined (i.e. not zero).
         for ( int i = attr->num_index - 1; i >= 0; i-- ) {
            // If the size is zero then the array is dynamic in size (i.e. a pointer)
            if ( attr->index[i].size == 0 ) {
               ostringstream errmsg;
               errmsg << "Parameter::complete_initialization():" << __LINE__
                      << " ERROR: FOM Interaction Parameter '"
                      << interaction_FOM_name << "'->'" << FOM_name
                      << "' with Trick name '" << trick_name << "' is a "
                      << ( attr->num_index + 1 ) << "-dimensional dynamic array"
                      << " of strings. Only one-dimensional dynamic arrays are"
                      << " supported for now.\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }
      }
   }

   // Determine if the Interaction Parameter type is supported.
   if ( !is_supported_parameter_type() ) {
      ostringstream errmsg;
      errmsg << "Parameter::complete_initialization():" << __LINE__
             << " ERROR: Unsupported Type and/or"
             << " rti_encoding for FOM Interaction Parameter '"
             << interaction_FOM_name << "'->'" << FOM_name
             << "' with Trick name '" << trick_name
             << "' and rti_encoding = " << rti_encoding << ".\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // We do not support an array of primitive types for the logical
   // time encoding, otherwise we support everything else.
   if ( ( rti_encoding == ENCODING_LOGICAL_TIME ) && ( attr->num_index > 0 ) ) {
      ostringstream errmsg;
      errmsg << "Parameter::complete_initialization():" << __LINE__
             << " ERROR: FOM Interaction Parameter '"
             << interaction_FOM_name << "'->'" << FOM_name << "' with Trick name '"
             << trick_name << "' can not be an array when using ENCODING_LOGICAL_TIME"
             << " for the 'rti_encoding'. Please check your input or modified-data"
             << " files to make sure the value for the 'rti_encoding' is"
             << " correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // The units must be in seconds if the ENCODING_LOGICAL_TIME is used.
   if ( ( rti_encoding == ENCODING_LOGICAL_TIME ) && ( strcmp( "s", attr->units ) != 0 ) ) {
      ostringstream errmsg;
      errmsg << "Parameter::complete_initialization():" << __LINE__
             << " ERROR: FOM Interaction Parameter '"
             << interaction_FOM_name << "'->'" << FOM_name << "' with Trick name '"
             << trick_name << "' must have the units of 'seconds' when the"
             << " 'rti_encoding' is set to ENCODING_LOGICAL_TIME. Please check your"
             << " input or modified-data files to make sure the value for the"
             << " 'rti_encoding' is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine if we need to do a byteswap for data transmission.
   byteswap = Utilities::is_transmission_byteswap( rti_encoding );

   // Determine if the size of this parameter is static or dynamic.
   size_is_static = is_static_in_size();

   // Get the parameter size and number of items.
   calculate_size_and_number_of_items();

   // Ensure enough buffer capacity for the parameter.
   ensure_buffer_capacity( size );

   // Check to make sure the users simulation variable has memory allocated to
   // it. It could be that the users simulation variable happens to be pointing
   // to a null string.
   if ( ( size == 0 )
        && ( attr->type != TRICK_STRING )
        && !( ( attr->type == TRICK_CHARACTER ) && ( attr->num_index > 0 ) ) ) {
      ostringstream msg;
      msg << "Parameter::complete_initialization():" << __LINE__
          << " WARNING: FOM Interaction Parameter '" << interaction_FOM_name
          << "'->'" << FOM_name << "' with Trick name '" << trick_name
          << "' has an unexpected size of zero bytes! Make sure your simulation"
          << " variable is properly initialized before the initialize()"
          << " function is called.\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
      string param_handle_string;
      StringUtilities::to_string( param_handle_string, this->param_handle );
      ostringstream msg;
      msg << "Parameter::complete_initialization():" << __LINE__ << '\n'
          << "========================================================\n"
          << "  interaction_FOM_name:'" << interaction_FOM_name << "'\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
          << "  ParameterHandle:" << param_handle_string << '\n'
          << "  attr->name:'" << attr->name << "'\n"
          << "  attr->type_name:'" << attr->type_name << "'\n"
          << "  attr->type:" << attr->type << '\n'
          << "  attr->units:" << attr->units << '\n'
          << "  size:" << size << '\n'
          << "  num_items:" << num_items << '\n'
          // TODO: Figure out get_size_from_attributes() API in Trick 10.
          //<< "  get_size_from_attributes():" << get_size_from_attributes(attr, attr->name) << endl
          << "  attr->size:" << attr->size << '\n'
          << "  attr->num_index:" << attr->num_index << '\n'
          << "  attr->index[0].size:" << ( attr->num_index >= 1 ? attr->index[0].size : 0 ) << '\n'
          << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
          << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( attr->type == TRICK_STRING )
           || ( ( ( attr->type == TRICK_CHARACTER ) || ( attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( attr->num_index > 0 )
                && ( attr->index[attr->num_index - 1].size == 0 ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

VariableLengthData Parameter::get_encoded_parameter_value()
{
   // Pack the parameter buffer to encode the parameter.
   pack_parameter_buffer();
   if ( rti_encoding == ENCODING_BOOLEAN ) {
      // The size is the number of 1-byte bool values in c++ and we need to
      // map to a 4-byte HLAboolean type. The buffer already holds the
      // encoded HLAboolean type.
      return VariableLengthData( buffer, 4 * size );
   }
   return VariableLengthData( buffer, size );
}

bool Parameter::extract_data(
   int const            param_size,
   unsigned char const *param_data )
{
   // Make sure we actually have parameter data to process.
   if ( ( param_size == 0 ) || ( param_data == NULL ) ) {
      return false;
   }

   // Determine the number of bytes we expect to receive based on how much
   // memory the Trick simulation variable is using.
   int expected_byte_count = get_parameter_size();

   switch ( rti_encoding ) {
      case ENCODING_BOOLEAN: {
         if ( param_size != ( 4 * expected_byte_count ) ) {
            ostringstream errmsg;
            errmsg << "Parameter::extract_data():" << __LINE__
                   << " WARNING: For Parameter '" << interaction_FOM_name << "'->'"
                   << FOM_name << "' with Trick name '" << trick_name << "', the"
                   << " received FOM data size (" << param_size << " bytes) != Expected Trick"
                   << " simulation variable memory size (" << ( 4 * expected_byte_count )
                   << " bytes) for 'rti_encoding' of ENCODING_BOOLEAN. Make sure your"
                   << " simulation variable is the same size and type as what is"
                   << " defined in the FOM. If you are using Lag Compensation one"
                   << " possible cause of this problem is that your lag compensation"
                   << " variables are not the correct size or type.\n";
            message_publish( MSG_WARNING, errmsg.str().c_str() );

            // For now, we ignore this error by just returning here.
            return false;
         }

         // Ensure enough buffer capacity.
         ensure_buffer_capacity( param_size );

         // Note: We don't set the 'size' to the value of "param_size" since we
         // are mapping 4-byte HLAboolean types to 1-byte bool in c++.
         //
         // Copy the RTI parameter value into the buffer.
         memcpy( buffer, param_data, param_size );
         break;
      }
      case ENCODING_NONE: {
         // The byte counts must match between the received attribute and
         // the Trick simulation variable for ENCODING_NONE since this
         // RTI encoding only supports a fixed length array of characters.
         if ( param_size != expected_byte_count ) {
            ostringstream errmsg;
            errmsg << "Parameter::extract_data():"
                   << __LINE__ << " WARNING: For Parameter '" << interaction_FOM_name
                   << "'->'" << FOM_name << "' with Trick name '" << trick_name
                   << "', the received FOM data size (" << param_size << " bytes) != Expected"
                   << " Trick simulation variable memory size (" << expected_byte_count
                   << " bytes) for the rti_encoding of ENCODING_NONE. The"
                   << " ENCODING_NONE only supports a fixed length array of"
                   << " characters. Make sure your simulation variable is the same"
                   << " size and type as what is defined in the FOM. If you are"
                   << " using Lag Compensation one possible cause of this problem"
                   << " is that your lag compensation variables are not the correct"
                   << " size or type.\n";
            message_publish( MSG_WARNING, errmsg.str().c_str() );

            // Just return if we have a data size mismatch. This will allow us
            // to continue to run even though the other federate is sending us
            // data that is not correct in size.
            return false;
         }

         // Ensure enough buffer capacity.
         ensure_buffer_capacity( param_size );

         // Make sure the buffer size matches how much data we are putting in it.
         this->size = param_size;

         // Copy the RTI parameter value into the buffer.
         memcpy( buffer, param_data, size );
         break;
      }
      case ENCODING_LOGICAL_TIME: {
         // Sanity check: The attribute FOM size must not exceed that of the
         // Trick ref-attributes.
         //
         // TODO: Account for variable length types such as a byte buffer, but
         // for now we are doing a rigid bounds check regardless of type. DDexter
         //
         if ( param_size != 8 ) {
            ostringstream errmsg;
            errmsg << "Parameter::extract_data():"
                   << __LINE__ << " ERROR: For Parameter '" << interaction_FOM_name
                   << "'->'" << FOM_name << "' with Trick name '" << trick_name
                   << "', the received FOM data size (" << param_size << " bytes) != Expected"
                   << " Trick simulation variable memory size (8 bytes) for the"
                   << " ENCODING_LOGICAL_TIME rti_encoding. Make sure your simulation"
                   << " variable is the same size and type as what is defined in the"
                   << " FOM. If you are using Lag Compensation one possible cause of"
                   << " this problem is that your lag compensation variables are not"
                   << " the correct size or type.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         // Ensure enough buffer capacity.
         ensure_buffer_capacity( param_size );

         // Make sure the buffer size matches how much data we are putting in it.
         this->size = param_size;

         // Copy the RTI parameter value into the buffer.
         memcpy( buffer, param_data, size );
         break;
      }
      case ENCODING_OPAQUE_DATA: {
         if ( size_is_static && ( param_size != expected_byte_count ) ) {
            ostringstream errmsg;
            errmsg << "Parameter::extract_data():"
                   << __LINE__ << " WARNING: For Parameter '" << interaction_FOM_name
                   << "'->'" << FOM_name << "' with Trick name '" << trick_name
                   << "', the received FOM data size (" << param_size << " bytes) != Expected"
                   << " Trick simulation variable memory size (" << expected_byte_count
                   << " bytes) for the rti_encoding of ENCODING_OPAQUE_DATA. Your simulation"
                   << " variable is static in size and can not take variable length"
                   << " data. Make sure your simulation variable is the same size"
                   << " and type as what is defined in the FOM. If you are using Lag"
                   << " Compensation one possible cause of this problem is that your"
                   << " lag compensation variables are not the correct size or type.\n";
            message_publish( MSG_WARNING, errmsg.str().c_str() );

            // For now, we ignore this error by just returning here.
            return false;
         }

         // Ensure enough buffer capacity.
         ensure_buffer_capacity( param_size );

         // Make sure the buffer size matches how much data we are putting in it.
         this->size = param_size;

         // Copy the RTI parameter value into the buffer.
         memcpy( buffer, param_data, size );
         break;
      }
      default: {
         if ( ( attr->type != TRICK_STRING )
              && ( param_size != expected_byte_count )
              && ( rti_encoding != ENCODING_UNICODE_STRING ) ) {
            ostringstream errmsg;
            errmsg << "Parameter::extract_data():"
                   << __LINE__ << " WARNING: For Parameter '" << interaction_FOM_name
                   << "'->'" << FOM_name << "' with Trick name '" << trick_name
                   << "', the received FOM data size (" << param_size << " bytes) != Expected"
                   << " Trick simulation variable memory size (" << expected_byte_count
                   << " bytes). Make sure your simulation variable is the same size and"
                   << " type as what is defined in the FOM. If you are using Lag"
                   << " Compensation one possible cause of this problem is that your"
                   << " lag compensation variables are not the correct size or type.\n";
            message_publish( MSG_WARNING, errmsg.str().c_str() );

            // For now, we ignore this error by just returning here.
            return false;
         }

         // Ensure enough buffer capacity.
         ensure_buffer_capacity( param_size );

         // Make sure the buffer size matches how much data we are putting in it.
         this->size = param_size;

         // Copy the RTI parameter value into the buffer.
         memcpy( buffer, param_data, size );
         break;
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
      message_publish( MSG_NORMAL, "Parameter::extract_data():%d Decoded '%s' (trick_name '%s') \
from parameter map, buffer-size:%d, expected-byte-count:%d.\n",
                       __LINE__, FOM_name, trick_name, param_size, expected_byte_count );
   }
   if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
      print_buffer();
   }

   // Mark the parameter value as changed.
   mark_changed();

   return true;
}

void Parameter::ensure_buffer_capacity(
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
      errmsg << "Parameter::ensure_buffer_capacity():" << __LINE__
             << " ERROR: Could not allocate memory for buffer for requested"
             << " capacity " << capacity << " for parameter '" << FOM_name
             << "'!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

void Parameter::calculate_size_and_number_of_items()
{
   int num_bytes = 0;

   // Handle Strings differently since we need to know the length of each string.
   if ( ( attr->type == TRICK_STRING )
        || ( ( ( attr->type == TRICK_CHARACTER ) || ( attr->type == TRICK_UNSIGNED_CHARACTER ) )
             && ( attr->num_index > 0 )
             && ( attr->index[attr->num_index - 1].size == 0 ) ) ) {

      calculate_static_number_of_items();

      switch ( rti_encoding ) {
         case ENCODING_OPAQUE_DATA:
         case ENCODING_NONE: {
            // Determine total number of bytes used by the Trick simulation
            // variable, and the data can be binary and not just the printable
            // ASCII characters.
            for ( int i = 0; i < num_items; ++i ) {
               char *s = *( static_cast< char ** >( address ) + i );
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
            // For the ENCODING_C_STRING, ENCODING_UNICODE_STRING, and ENCODING_ASCII_STRING
            // encodings assume the string is NULL terminated and determine the
            // number of characters using strlen().
            for ( int i = 0; i < num_items; ++i ) {
               char const *s = *( static_cast< char ** >( address ) + i );
               if ( s != NULL ) {
                  num_bytes += strlen( s );
               }
            }
            break;
         }
      }
   } else {
      // Handle all the other primitive types.

      if ( ( attr->num_index > 0 ) && ( attr->index[attr->num_index - 1].size == 0 ) ) {
         // We have an multi-diminsion array that is a pointer and the
         // number of dimensions is num_index

         // TODO: Handle the case when attr->num_index > 1, right now we
         // only are handling the case when num_index == 1 below.
         // NOTE: For now we assume 1-D array.

         // get_size returns the number of elements in the array.
         num_bytes = ( get_size( *static_cast< char ** >( address ) ) * attr->size );

         // Since the users variable is a pointer, we need to recalculate
         // the number of items.
         if ( attr->size > 0 ) {
            this->num_items = num_bytes / attr->size;
         } else {
            // Punt and set the number of items to equal the number of bytes.
            this->num_items = num_bytes;
         }
      } else {
         // The user variable is either a primitive type or a static
         // multi-dimensional array.

         // Please not that get_size_from_attributes() and
         // nitems * attr->size should return the same number of bytes.
         // num_bytes = get_size_from_attributes( attr, attr->name );

         calculate_static_number_of_items();
         num_bytes = num_items * attr->size;
      }
   }

   this->size = num_bytes;

   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
      string param_handle_string;
      StringUtilities::to_string( param_handle_string, this->param_handle );
      ostringstream msg;
      msg << "Parameter::calculate_size_and_number_of_items():" << __LINE__ << '\n'
          << "========================================================\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
          << "  ParameterHandle:" << param_handle_string << '\n'
          << "  ref2->attr->name:'" << attr->name << "'\n"
          << "  ref2->attr->type_name:'" << attr->type_name << "'\n"
          << "  ref2->attr->type:" << attr->type << '\n'
          << "  ref2->attr->units:" << attr->units << '\n'
          << "  size:" << size << '\n'
          << "  num_items:" << num_items << '\n';
      // TODO: Figure out get_size_from_attributes() API in Trick 10.
      msg << "  ref2->attr->size:" << attr->size << '\n'
          << "  ref2->attr->num_index:" << attr->num_index << '\n'
          << "  ref2->attr->index[0].size:" << ( attr->num_index >= 1 ? attr->index[0].size : 0 ) << '\n'
          << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
          << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( attr->type == TRICK_STRING )
           || ( ( ( attr->type == TRICK_CHARACTER ) || ( attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( attr->num_index > 0 )
                && ( attr->index[attr->num_index - 1].size == 0 ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*!
 * @details If the parameter is static in size it uses a cached size value
 * otherwise the size is calculated.
 */
int Parameter::get_parameter_size()
{
   if ( !size_is_static ) {
      calculate_size_and_number_of_items();
   }
   return ( this->size );
}

bool Parameter::is_static_in_size() const
{
   if ( is_supported_parameter_type() ) {
      // If this is not an array (i.e. num_index == 0) or has static arrays then
      // this parameter is static in size.
      if ( attr->num_index > 0 ) {
         for ( int i = 0; i < attr->num_index; ++i ) {
            // Make sure each dimension is statically defined (i.e. not zero).
            if ( attr->index[i].size <= 0 ) {
               return false;
            }
         }
      }
      return true;
   }
   return false;
}

/*!
 * @details If the parameter is not for an array then a value of one is
 * returned. Otherwise the number of items in the static array are returned.
 * \par<b>Assumptions and Limitations:</b>
 * - Only static arrays are supported for now.
 * @job_class{initialization}
 */
void Parameter::calculate_static_number_of_items()
{
   int length = 1;

   // Determine the number of items this parameter has (i.e. items in array).
   if ( attr->num_index > 0 ) {
      for ( int i = 0; i < attr->num_index; ++i ) {
         if ( attr->index[i].size > 0 ) {
            length *= attr->index[i].size;
         }
      }
   }

   this->num_items = length;
}

void Parameter::pack_parameter_buffer()
{
   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
      string param_handle_string;
      StringUtilities::to_string( param_handle_string, this->param_handle );
      ostringstream msg;
      msg << "Parameter::pack_parameter_buffer():" << __LINE__ << '\n'
          << "======================== BEFORE PACK ================================\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
          << "  ParameterHandle:" << param_handle_string << '\n'
          << "  ref2->attr->name:'" << attr->name << "'\n"
          << "  ref2->attr->type_name:'" << attr->type_name << "'\n"
          << "  ref2->attr->type:" << attr->type << '\n'
          << "  ref2->attr->units:" << attr->units << '\n'
          << "  size:" << size << '\n'
          << "  num_items:" << num_items << '\n'
          // TODO: Figure out get_size_from_attributes() API in Trick 10.
          //<< "  get_size_from_attributes():" << get_size_from_attributes(attr, attr->name) << endl
          << "  ref2->attr->size:" << attr->size << '\n'
          << "  ref2->attr->num_index:" << attr->num_index << '\n'
          << "  ref2->attr->index[0].size:" << ( attr->num_index >= 1 ? attr->index[0].size : 0 ) << '\n'
          << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
          << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( attr->type == TRICK_STRING )
           || ( ( ( attr->type == TRICK_CHARACTER ) || ( attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( attr->num_index > 0 )
                && ( attr->index[attr->num_index - 1].size == 0 ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // TODO: Use a transcoder for each type to encode and decode depending on
   // the type specified in the FOM instead of the code below. Dan Dexter

   switch ( rti_encoding ) {
      case ENCODING_LOGICAL_TIME: {
         num_items = 1;
         size      = 8;

         // Ensure enough capacity in the buffer for the parameter and all its
         // items if it was an array.
         ensure_buffer_capacity( size );

         // Encode the logical time.
         encode_logical_time();
         break;
      }
      case ENCODING_BOOLEAN: {
         // Determine the number of items this parameter has (i.e. is it an array).
         if ( !size_is_static ) {
            calculate_size_and_number_of_items();
         }

         encode_boolean_to_buffer();

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
            ostringstream msg;
            msg << "Parameter::pack_parameter_buffer():" << __LINE__ << '\n'
                << "================== PARAMETER ENCODE ==================================\n"
                << " parameter '" << FOM_name << "' (trick name '" << trick_name
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

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
            ostringstream msg;
            msg << "Parameter::pack_parameter_buffer():" << __LINE__ << '\n'
                << "================== PARAMETER ENCODE ==================================\n"
                << " parameter '" << FOM_name << "' (trick name '" << trick_name
                << "')\n";
            message_publish( MSG_NORMAL, msg.str().c_str() );
            print_buffer();
         }
         break;
      }
      default: {
         // Must handle the string as a special case because of special encodings.
         if ( ( attr->type == TRICK_STRING )
              || ( ( ( attr->type == TRICK_CHARACTER ) || ( attr->type == TRICK_UNSIGNED_CHARACTER ) )
                   && ( attr->num_index > 0 )
                   && ( attr->index[attr->num_index - 1].size == 0 ) ) ) {

            // NOTE: For now we must calculate size everytime because on a receive,
            // the 'size' is adjusted to the number of bytes received and does not
            // reflect what we are sending. We only have this problem for variable
            // length types such as strings which is the only variable length type
            // we support right now. DDexter
            calculate_size_and_number_of_items();

            encode_string_to_buffer();

            if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
               ostringstream msg;
               msg << "Parameter::pack_parameter_buffer():" << __LINE__ << '\n'
                   << "================== PARAMETER ENCODE ==================================\n"
                   << " parameter '" << FOM_name << "' (trick name '" << trick_name
                   << "')\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
               print_buffer();
            }
         } else {
            // Determine the number of items this parameter has (i.e. is it an array).
            if ( !size_is_static ) {
               calculate_size_and_number_of_items();
            }

            // Ensure enough capacity in the buffer for the parameter and all its
            // items if it was an array.
            ensure_buffer_capacity( size );

            // Determine if the users variable is a pointer.
            if ( ( attr->num_index > 0 ) && ( attr->index[attr->num_index - 1].size == 0 ) ) {
               // It's a pointer

               // Byteswap if needed and copy the parameter to the buffer.
               byteswap_buffer_copy( buffer,
                                     *static_cast< char ** >( address ),
                                     attr->type,
                                     num_items,
                                     size );
            } else {
               // It's either a primitive type or a static array.

               // Byteswap if needed and copy the parameter to the buffer.
               byteswap_buffer_copy( buffer,
                                     address,
                                     attr->type,
                                     num_items,
                                     size );
            }

            if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
               ostringstream msg;
               msg << "Parameter::pack_parameter_buffer():" << __LINE__ << '\n'
                   << "================== PARAMETER ENCODE ==================================\n"
                   << " parameter '" << FOM_name << "' (trick name '" << trick_name
                   << "')\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
               print_buffer();
            }
         }
         break;
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
      string param_handle_string;
      StringUtilities::to_string( param_handle_string, this->param_handle );
      ostringstream msg;
      msg << "Parameter::pack_parameter_buffer():" << __LINE__ << '\n'
          << "======================== AFTER PACK ================================\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
          << "  ParameterHandle:" << param_handle_string << '\n'
          << "  ref2->attr->name:'" << attr->name << "'\n"
          << "  ref2->attr->type_name:'" << attr->type_name << "'\n"
          << "  ref2->attr->type:" << attr->type << '\n'
          << "  ref2->attr->units:" << attr->units << '\n'
          << "  size:" << size << '\n'
          << "  num_items:" << num_items << '\n'
          // TODO: Figure out get_size_from_attributes() API in Trick 10.
          //<< "  get_size_from_attributes():" << get_size_from_attributes(attr, attr->name) << endl
          << "  ref2->attr->size:" << attr->size << '\n'
          << "  ref2->attr->num_index:" << attr->num_index << '\n'
          << "  ref2->attr->index[0].size:" << ( attr->num_index >= 1 ? attr->index[0].size : 0 ) << '\n'
          << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
          << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( attr->type == TRICK_STRING )
           || ( ( ( attr->type == TRICK_CHARACTER ) || ( attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( attr->num_index > 0 )
                && ( attr->index[attr->num_index - 1].size == 0 ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

void Parameter::unpack_parameter_buffer()
{
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
         // Determine the number of items this parameter has (i.e. is it an array).
         if ( !size_is_static ) {
            calculate_size_and_number_of_items();
         }

         decode_boolean_from_buffer();

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
            ostringstream msg;
            msg << "Parameter::unpack_parameter_buffer():" << __LINE__ << '\n'
                << "================== PARAMETER DECODE ==================================\n"
                << " parameter '" << FOM_name << "' (trick name '" << trick_name
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

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
            ostringstream msg;
            msg << "Parameter::unpack_parameter_buffer():" << __LINE__ << '\n'
                << "================== PARAMETER DECODE ==================================\n"
                << " parameter '" << FOM_name << "' (trick name '" << trick_name
                << "')\n";
            message_publish( MSG_NORMAL, msg.str().c_str() );
            print_buffer();
         }
         break;
      }
      default: {
         // Must handle the string as a special case because of special encodings.
         if ( ( attr->type == TRICK_STRING )
              || ( ( ( attr->type == TRICK_CHARACTER ) || ( attr->type == TRICK_UNSIGNED_CHARACTER ) )
                   && ( attr->num_index > 0 )
                   && ( attr->index[attr->num_index - 1].size == 0 ) ) ) {

            // The size is the received size but recalculate the number of items.
            if ( !size_is_static ) {
               if ( attr->type == TRICK_STRING ) {
                  calculate_static_number_of_items();
               } else {
                  calculate_size_and_number_of_items();
               }
            }

            decode_string_from_buffer();

            if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
               ostringstream msg;
               msg << "Parameter::unpack_parameter_buffer():" << __LINE__ << '\n'
                   << "================ PARAMETER DECODE ================================\n"
                   << " parameter '" << FOM_name << "' (trick name '" << trick_name << "')"
                   << " value:\"" << ( *static_cast< char ** >( address ) ) << "\"\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
               print_buffer();
            }
         } else {

            // Determine the number of items this parameter has (i.e. is it an array).
            if ( !size_is_static ) {
               calculate_size_and_number_of_items();
            }

            // Determine if the users variable is a pointer.
            if ( ( attr->num_index > 0 ) && ( attr->index[attr->num_index - 1].size == 0 ) ) {
               // It's a pointer

               // Byteswap if needed and copy the buffer over to the parameter.
               byteswap_buffer_copy( *static_cast< char ** >( address ),
                                     buffer,
                                     attr->type,
                                     num_items,
                                     size );
            } else {
               // It's either a primitive type or a static array.

               // Byteswap if needed and copy the buffer over to the parameter.
               byteswap_buffer_copy( address,
                                     buffer,
                                     attr->type,
                                     num_items,
                                     size );

               if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
                  ostringstream msg;
                  msg << "Parameter::unpack_parameter_buffer():" << __LINE__ << '\n'
                      << "================== PARAMETER DECODE ================================\n"
                      << " parameter '" << FOM_name << "' (trick name '" << trick_name
                      << "')\n";
                  message_publish( MSG_NORMAL, msg.str().c_str() );
                  print_buffer();
               }
            }
         }
         break;
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
      string param_handle_string;
      StringUtilities::to_string( param_handle_string, this->param_handle );
      ostringstream msg;
      msg << "Parameter::unpack_parameter_buffer():" << __LINE__ << '\n'
          << "========================================================\n"
          << "  FOM_name:'" << ( ( FOM_name != NULL ) ? FOM_name : "NULL" ) << "'\n"
          << "  trick_name:'" << ( ( trick_name != NULL ) ? trick_name : "NULL" ) << "'\n"
          << "  ParameterHandle:" << param_handle_string << '\n'
          << "  ref2->attr->name:'" << attr->name << "'\n"
          << "  ref2->attr->type_name:'" << attr->type_name << "'\n"
          << "  ref2->attr->type:" << attr->type << '\n'
          << "  ref2->attr->units:" << attr->units << '\n'
          << "  size:" << size << '\n'
          << "  num_items:" << num_items << '\n'
          // TODO: Figure out get_size_from_attributes() API in Trick 10.
          //<< "  get_size_from_attributes():" << get_size_from_attributes(attr, attr->name) << endl
          << "  ref2->attr->size:" << attr->size << '\n'
          << "  ref2->attr->num_index:" << attr->num_index << '\n'
          << "  ref2->attr->index[0].size:" << ( attr->num_index >= 1 ? attr->index[0].size : 0 ) << '\n'
          << "  byteswap:" << ( is_byteswap() ? "Yes" : "No" ) << '\n'
          << "  buffer_capacity:" << buffer_capacity << '\n'
          << "  size_is_static:" << ( size_is_static ? "Yes" : "No" ) << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      if ( ( attr->type == TRICK_STRING )
           || ( ( ( attr->type == TRICK_CHARACTER ) || ( attr->type == TRICK_UNSIGNED_CHARACTER ) )
                && ( attr->num_index > 0 )
                && ( attr->index[attr->num_index - 1].size == 0 ) ) ) {
         msg << "  value:\"" << ( *static_cast< char ** >( address ) ) << "\"\n";
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

void Parameter::encode_boolean_to_buffer()
{
   bool *bool_src;

   // Determine if the users variable is a pointer.
   if ( ( attr->num_index > 0 ) && ( attr->index[attr->num_index - 1].size == 0 ) ) {
      // It's a pointer
      bool_src = reinterpret_cast< bool * >( *static_cast< char ** >( address ) );

   } else {
      // It's either a primitive type or a static array.
      bool_src = static_cast< bool * >( address );
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

void Parameter::decode_boolean_from_buffer() const
{
   bool *bool_dest;

   // Determine if the users variable is a pointer.
   if ( ( attr->num_index > 0 ) && ( attr->index[attr->num_index - 1].size == 0 ) ) {
      // It's a pointer
      bool_dest = reinterpret_cast< bool * >( *static_cast< char ** >( address ) );
   } else {
      // It's either a primitive type or a static array.
      bool_dest = static_cast< bool * >( address );
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

void Parameter::encode_logical_time() const
{
   // Integer representing time in the base HLA Logical Time representation.
   int64_t logical_time = 0;

   switch ( attr->type ) {
      case TRICK_DOUBLE: {
         double const *d_src = static_cast< double * >( address );
         logical_time        = Int64BaseTime::to_base_time( d_src[0] );
         break;
      }
      case TRICK_FLOAT: {
         float const *f_src = static_cast< float * >( address );
         logical_time       = Int64BaseTime::to_base_time( (double)f_src[0] );
         break;
      }
      case TRICK_SHORT: {
         short const *s_src = static_cast< short const * >( address );
         logical_time       = (int64_t)( Int64BaseTime::get_base_time_multiplier() * s_src[0] );
         break;
      }
      case TRICK_UNSIGNED_SHORT: {
         unsigned short const *us_src = static_cast< unsigned short const * >( address );
         logical_time                 = (int64_t)( Int64BaseTime::get_base_time_multiplier() * us_src[0] );
         break;
      }
      case TRICK_INTEGER: {
         int const *i_src = static_cast< int const * >( address );
         logical_time     = (int64_t)( Int64BaseTime::get_base_time_multiplier() * i_src[0] );
         break;
      }
      case TRICK_UNSIGNED_INTEGER: {
         unsigned int const *ui_src = static_cast< unsigned int const * >( address );
         logical_time               = (int64_t)( Int64BaseTime::get_base_time_multiplier() * ui_src[0] );
         break;
      }
      case TRICK_LONG: {
         long const *l_src = static_cast< long const * >( address );
         logical_time      = (int64_t)( Int64BaseTime::get_base_time_multiplier() * l_src[0] );
         break;
      }
      case TRICK_UNSIGNED_LONG: {
         unsigned long const *ul_src = static_cast< unsigned long const * >( address );
         logical_time                = (int64_t)( Int64BaseTime::get_base_time_multiplier() * ul_src[0] );
         break;
      }
      case TRICK_LONG_LONG: {
         long long const *ll_src = static_cast< long long const * >( address );
         logical_time            = (int64_t)( Int64BaseTime::get_base_time_multiplier() * ll_src[0] );
         break;
      }
      case TRICK_UNSIGNED_LONG_LONG: {
         unsigned long long const *ull_src = static_cast< unsigned long long const * >( address );
         logical_time                      = (int64_t)( Int64BaseTime::get_base_time_multiplier() * ull_src[0] );
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "Parameter::encode_logical_time():" << __LINE__
                << " ERROR: For Parameter '" << FOM_name << "' with Trick name '"
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

void Parameter::decode_logical_time()
{
   // Integer representing time in the base HLA Logical Time representation.
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

   switch ( attr->type ) {
      case TRICK_DOUBLE: {
         double *d_dest = static_cast< double * >( address );
         d_dest[0]      = Int64BaseTime::to_seconds( logical_time );
         break;
      }
      case TRICK_FLOAT: {
         float *f_dest = static_cast< float * >( address );
         f_dest[0]     = (float)Int64BaseTime::to_seconds( logical_time );
         break;
      }
      case TRICK_SHORT: {
         short  *s_dest = static_cast< short * >( address );
         int64_t value  = logical_time / Int64BaseTime::get_base_time_multiplier();
         s_dest[0]      = ( value > SHRT_MAX ) ? SHRT_MAX : (short)value;
         break;
      }
      case TRICK_UNSIGNED_SHORT: {
         unsigned short *us_dest = static_cast< unsigned short * >( address );
         int64_t         value   = logical_time / Int64BaseTime::get_base_time_multiplier();
         us_dest[0]              = ( value > USHRT_MAX ) ? USHRT_MAX : (unsigned short)value;
         break;
      }
      case TRICK_INTEGER: {
         int    *i_dest = static_cast< int * >( address );
         int64_t value  = logical_time / Int64BaseTime::get_base_time_multiplier();
         i_dest[0]      = ( value > INT_MAX ) ? INT_MAX : value;
         break;
      }
      case TRICK_UNSIGNED_INTEGER: {
         unsigned int *ui_dest = static_cast< unsigned int * >( address );
         int64_t       value   = logical_time / Int64BaseTime::get_base_time_multiplier();
         ui_dest[0]            = ( value > UINT_MAX ) ? UINT_MAX : (unsigned int)value;
         break;
      }
      case TRICK_LONG: {
         long   *l_dest = static_cast< long * >( address );
         int64_t value  = logical_time / Int64BaseTime::get_base_time_multiplier();
         l_dest[0]      = ( value > LONG_MAX ) ? LONG_MAX : (long)value;
         break;
      }
      case TRICK_UNSIGNED_LONG: {
         unsigned long *ul_dest = static_cast< unsigned long * >( address );
         int64_t        value   = logical_time / Int64BaseTime::get_base_time_multiplier();
         ul_dest[0]             = ( value > (int64_t)ULONG_MAX ) ? ULONG_MAX : (unsigned long)value;
         break;
      }
      case TRICK_LONG_LONG: {
         long long *ll_dest = static_cast< long long * >( address );
         int64_t    value   = logical_time / Int64BaseTime::get_base_time_multiplier();
         ll_dest[0]         = ( value > Int64BaseTime::get_max_base_time() ) ? Int64BaseTime::get_max_base_time() : (long long)value;
         break;
      }
      case TRICK_UNSIGNED_LONG_LONG: {
         unsigned long long *ull_dest = static_cast< unsigned long long * >( address );

         int64_t value = logical_time / Int64BaseTime::get_base_time_multiplier();
         ull_dest[0]   = ( value > Int64BaseTime::get_max_base_time() )
                            ? (unsigned long long)Int64BaseTime::get_max_base_time()
                            : (unsigned long long)value;
         break;
      }
      default: {
         ostringstream errmsg;
         errmsg << "Parameter::decode_logical_time():" << __LINE__
                << " ERROR: For Parameter '" << FOM_name << "' with Trick name '"
                << trick_name << "' the type is not supported for the"
                << " ENCODING_LOGICAL_TIME encoding.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         break;
      }
   }
}

void Parameter::encode_opaque_data_to_buffer()
{
   // Must handle the string as a special case because of special encodings.
   if ( attr->type == TRICK_STRING ) {
      encode_string_to_buffer();
   } else {
      // Handle the other primitive types.
      unsigned char *output;       // Cast the buffer to be a character array.
      int            num_elements; // Number of elements in the encoded string.
      char          *s;            // pointer to a string

      // HLAopaqueData format documented in IEEE Standard 1516.2-2000, which
      // will handle variable length binary data.

      // Determine if the users variable is a pointer.
      if ( ( attr->num_index > 0 ) && ( attr->index[attr->num_index - 1].size == 0 ) ) {
         // It's a pointer
         s = *( static_cast< char ** >( address ) );
      } else {
         // It's either a primitive type or a static array.
         s = static_cast< char * >( address );
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

void Parameter::decode_opaque_data_from_buffer()
{
   // Must handle the string as a special case because of special encodings.
   if ( attr->type == TRICK_STRING ) {
      decode_string_from_buffer();
   } else {
      // Handle the other primitive types.
      unsigned char *input;
      unsigned char *output;

      // HLAopaqueData format documented in IEEE Standard 1516.2-2000, which
      // will handle variable length binary data.

      input = buffer;

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
         message_publish( MSG_WARNING, "Parameter::decode_opaque_data_from_buffer():%d \
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
         message_publish( MSG_WARNING, "Parameter::decode_opaque_data_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA attribute '%s', decoded length %d > data buffer \
size %d, will use the data buffer size instead.\n",
                          __LINE__, FOM_name, length, data_buff_size );
         length = data_buff_size;
      }

      // Determine if the users variable is a pointer.
      if ( ( attr->num_index > 0 ) && ( attr->index[attr->num_index - 1].size == 0 ) ) {
         // It's a pointer
         output = *( static_cast< unsigned char ** >( address ) );

         if ( output != NULL ) {
            // The output array size must exactly match the incoming data size for opaque data.
            if ( length != get_size( output ) ) {
               // WORKAROUND: Trick 10 can't handle a length of zero so to work
               // around the memory manager problem use a size of 1 in the
               // allocation.
               *( static_cast< char ** >( address ) ) =
                  static_cast< char * >( TMM_resize_array_1d_a(
                     *( static_cast< char ** >( address ) ),
                     ( ( length > 0 ) ? length : 1 ) ) );

               output = *( static_cast< unsigned char ** >( address ) );
            }
         } else {
            // Allocate memory for the output array.
            // WORKAROUND: Trick 10 can't handle a length of zero so to work
            // around the memory manager problem use a size of 1 in the
            // allocation.
            *( static_cast< char ** >( address ) ) =
               static_cast< char * >( TMM_declare_var_1d( "char", ( ( length > 0 ) ? length : 1 ) ) );

            output = *( static_cast< unsigned char ** >( address ) );
         }
      } else {
         // It's either a primitive type or a static array.
         output = static_cast< unsigned char * >( address );

         if ( output != NULL ) {
            // The output array size must exactly match the incoming data size for opaque data.
            if ( length != get_size( output ) ) {
               // WORKAROUND: Trick 10 can't handle a length of zero so to work
               // around the memory manager problem use a size of 1 in the
               // allocation.
               address = static_cast< char * >( TMM_resize_array_1d_a(
                  static_cast< char * >( address ), ( ( length > 0 ) ? length : 1 ) ) );

               output = static_cast< unsigned char * >( address );
            }
         } else {
            // Allocate memory for the output array.
            // WORKAROUND: Trick 10 can't handle a length of zero so to work
            // around the memory manager problem use a size of 1 in the
            // allocation.
            address = static_cast< char * >( TMM_declare_var_1d( "char", ( ( length > 0 ) ? length : 1 ) ) );

            output = static_cast< unsigned char * >( address );
         }
      }

      if ( output == NULL ) {
         ostringstream errmsg;
         errmsg << "Parameter::decode_opaque_data_from_buffer():" << __LINE__
                << " ERROR: Could not allocate memory for ENCODING_OPAQUE_DATA Parameter '"
                << FOM_name << "' with Trick name '" << trick_name
                << "' and length " << length << "!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // Copy the characters over.
      if ( length > 0 ) {
         memcpy( output, input, length );
      }
   }
}

void Parameter::encode_string_to_buffer()
{
   unsigned char *output; // Cast the buffer to be a character array.
   char          *s;      // pointer to a string

   switch ( rti_encoding ) {
      case ENCODING_UNICODE_STRING: {
         // HLAunicodeString format documented in IEEE
         // Standard 1516.2-2000, Section 4.12.6
         if ( num_items == 1 ) {

            // UTF-16 character encoding of the string.
            s = *( static_cast< char ** >( address ) );

            // Number of elements to be encoded (number of characters).
            int num_elements = ( ( size > 0 ) && ( s != NULL ) ) ? size : 0;

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

               s = *( static_cast< char ** >( address ) + i );

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
                  byte_count += 2 * length;
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

            // UTF-16 character encoding of the string.
            s = *( static_cast< char ** >( address ) );

            // Number of elements to be encoded (number of characters).
            int num_elements = ( ( size > 0 ) && ( s != NULL ) ) ? size : 0;

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

               s = *( static_cast< char ** >( address ) + i );

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
                  // Copy the characters to the output buffer.
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

            s = *( static_cast< char ** >( address ) );

            int num_elements;
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
            int num_elements = 0;
            for ( int i = 0; i < num_items; ++i ) {
               s = *( static_cast< char ** >( address ) + i );
               if ( s != NULL ) {
                  int length = get_size( s );
                  if ( length > 0 ) {
                     num_elements += length;
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
               s = *( static_cast< char ** >( address ) + i );

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

            s = *( static_cast< char ** >( address ) + i );

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
            errmsg << "Parameter::encode_string_to_buffer():" << __LINE__
                   << " ERROR: For ENCODING_NONE, Parameter '" << FOM_name
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

            s = *( static_cast< char ** >( address ) + i );

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

void Parameter::decode_string_from_buffer()
{
   unsigned char *input;
   unsigned char *output;

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
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING parameter '%s', decoded length %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, decoded_count );
               length = 0;
            } else {
               length = decoded_count;
            }

            // If the users Trick simulation is static in size then we need to
            // do a bounds check so that we don't overflow the users variable.
            if ( size_is_static ) {
               int data_buff_size;
               if ( size > 4 ) {
                  data_buff_size = ( size - 4 ) / 2;
               } else {
                  data_buff_size = 0;
               }
               if ( length > data_buff_size ) {
                  message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING parameter '%s', decoded length %d > data buffer \
size %d, will use the data buffer size instead.\n",
                                   __LINE__, FOM_name, length,
                                   data_buff_size );
                  length = data_buff_size;
               }
            }

            // UTF-16 character encoding of the string.
            output = *( static_cast< unsigned char ** >( address ) );

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

                  *( static_cast< char ** >( address ) ) =
                     static_cast< char * >( TMM_resize_array_1d_a(
                        *( static_cast< char ** >( address ) ), array_size ) );

                  output = *( static_cast< unsigned char ** >( address ) );
               }
            } else {
               // Allocate memory for the sim string and include room for the
               // terminating null character and add a few more bytes to give
               // us a little more space for next time.
               int array_size = Utilities::next_positive_multiple_of_8( length );

               *( static_cast< char ** >( address ) ) =
                  static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );

               output = *( static_cast< unsigned char ** >( address ) );
            }

            if ( output == NULL ) {
               ostringstream errmsg;
               errmsg << "Parameter::decode_string_from_buffer():" << __LINE__
                      << " ERROR: Could not allocate memory for ENCODING_UNICODE_STRING"
                      << " parameter '" << FOM_name << "' and length "
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
            int num_elements;
            if ( decoded_count < 0 ) {
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING parmeter '%s', decoded element count %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, decoded_count );
               num_elements = 0;
            } else {
               num_elements = decoded_count;
            }

            // Handle the situation where more strings are in the input encoding
            // than what exist in the ref-attributes.
            if ( num_elements > num_items ) {
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: Truncating array of ENCODING_UNICODE_STRING from %d to %d elements for parameter '%s'!\n",
                                __LINE__, num_elements, num_items, FOM_name );
               num_elements = num_items;
            }

            // Calculate the number of UTF-16 characters which is the number of
            // bytes in the buffer minus the encoded length fields divided by 2.
            // data_buff_size = (size - 4 - 4 * num_elements)/2
            // TODO: IS THIS NEEDED INSTEAD?
            //            int data_buff_size = size;
            //            if ( attr->type == TRICK_STRING ) {
            //               data_buff_size = (size - (4 * (num_elements + 1))) / 2;
            //            }
            int data_buff_size;
            if ( size > ( 4 * ( num_elements + 1 ) ) ) {
               data_buff_size = ( size - ( 4 * ( num_elements + 1 ) ) ) / 2;
            } else {
               data_buff_size = 0;
            }

            // Decode each of the HLAunicodeString elements.
            for ( int i = 0; i < num_elements; ++i ) {

               // Decode the length of the string which is an HLAinteger32BE (Big Endian).
               int decoded_inner_count = 0;
               if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               } else {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               }

               // Do a sanity check on the decoded length, it should not be negative.
               int length;
               if ( decoded_inner_count < 0 ) {
                  message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING array element %d of %d, parameter '%s', the decoded \
length %d < 0, will use 0 instead.\n",
                                   __LINE__, ( i + 1 ), num_elements, FOM_name, decoded_inner_count );
                  length = 0;
               } else {
                  length = decoded_inner_count;
               }

               // Do a sanity check on the decoded length as compared to how much
               // data remains in the buffer.
               if ( length > data_buff_size ) {
                  message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_UNICODE_STRING array element %d of %d, parameter '%s', the decoded \
length %d > data buffer size %d, will use the data buffer size instead.\n",
                                   __LINE__, ( i + 1 ), num_elements, FOM_name, length, data_buff_size );
                  length = data_buff_size;
               }

               // Adjust the amount of data left in the buffer.
               if ( data_buff_size > length ) {
                  data_buff_size -= length;
               } else {
                  data_buff_size = 0;
               }

               // UTF-16 character encoding of the string.
               output = *( static_cast< unsigned char ** >( address ) + i );

               if ( output != NULL ) {

                  // Determine if we need to allocate more memory for the sim string.
                  // If it is larger than the existing string and larger than the
                  // memory allocated for the string then reallocate more memory.
                  if ( length >= get_size( output ) ) {

                     // Make sure to make room for the terminating null character,
                     // and add a few more bytes to give us a little more space
                     // for next time.
                     int array_size = Utilities::next_positive_multiple_of_8( length );

                     *( static_cast< char ** >( address ) + i ) =
                        static_cast< char * >( TMM_resize_array_1d_a(
                           *( static_cast< char ** >( address ) + i ), array_size ) );

                     output = *( static_cast< unsigned char ** >( address ) + i );
                  }
               } else {
                  // Allocate memory for the sim string and include room for the
                  // terminating null character and add a few more bytes to give
                  // us a little more space for next time.
                  int array_size = Utilities::next_positive_multiple_of_8( length );

                  *( static_cast< char ** >( address ) + i ) =
                     static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );

                  output = *( static_cast< unsigned char ** >( address ) + i );
               }

               if ( output == NULL ) {
                  ostringstream errmsg;
                  errmsg << "Parameter::decode_string_from_buffer():" << __LINE__
                         << " ERROR: Could not allocate memory for ENCODING_UNICODE_STRING"
                         << " parameter '" << FOM_name << "' and length "
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
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING parmeter '%s', decoded length %d < 0, will use 0 instead.\n",
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
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING parameter '%s', decoded length %d > data buffer size \
%d, will use the data buffer size instead.\n",
                                __LINE__, FOM_name, length, data_buff_size );
               length = data_buff_size;
            }

            // UTF-16 character encoding of the string.
            output = *( static_cast< unsigned char ** >( address ) );

            if ( output != NULL ) {

               // Determine if we need to allocate more memory for the sim string.
               // If it is larger than the existing string and larger than the
               // memory allocated for the string then reallocate more memory.
               if ( length >= get_size( output ) ) {

                  // Make sure to make room for the terminating null character,
                  // and add a few more bytes to give us a little more space
                  // for next time.
                  int array_size = Utilities::next_positive_multiple_of_8( length );

                  *( static_cast< char ** >( address ) ) =
                     static_cast< char * >( TMM_resize_array_1d_a(
                        *( static_cast< char ** >( address ) ), array_size ) );

                  output = *( static_cast< unsigned char ** >( address ) );
               }
            } else {
               // Allocate memory for the sim string and include room for the
               // terminating null character and add a few more bytes to give
               // us a little more space for next time.
               int array_size = Utilities::next_positive_multiple_of_8( length );

               *( static_cast< char ** >( address ) ) =
                  static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );

               output = *( static_cast< unsigned char ** >( address ) );
            }

            if ( output == NULL ) {
               ostringstream errmsg;
               errmsg << "Parameter::decode_string_from_buffer():" << __LINE__
                      << " ERROR: Could not allocate memory for ENCODING_ASCII_STRING"
                      << " parameter '" << FOM_name << "' and length "
                      << Utilities::next_positive_multiple_of_8( length )
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
            int num_elements;
            if ( decoded_count < 0 ) {
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING parameter '%s', decoded element count %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, decoded_count );
               num_elements = 0;
            } else {
               num_elements = decoded_count;
            }

            // Handle the situation where more strings are in the input encoding
            // than what exist in the ref-attributes.
            if ( num_elements > num_items ) {
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: Truncating array of ENCODING_ASCII_STRING from %d to %d elements for parameter '%s'!\n",
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
               int decoded_inner_count = 0;
               if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               } else {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               }

               // Do a sanity check on the decoded length, it should not be negative.
               int length;
               if ( decoded_inner_count < 0 ) {
                  message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING array element %d, parameter '%s', the decoded \
length %d < 0, will use 0 instead.\n",
                                   __LINE__, i, FOM_name, decoded_inner_count );
                  length = 0;
               } else {
                  length = decoded_inner_count;
               }

               // Do a sanity check on the decoded length as compared to how much
               // data remains in the buffer.
               if ( length > data_buff_size ) {
                  message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_ASCII_STRING array element %d, parameter '%s', the decoded \
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
               output = *( static_cast< unsigned char ** >( address ) + i );

               if ( output != NULL ) {

                  // Determine if we need to allocate more memory for the sim string.
                  // If it is larger than the existing string and larger than the
                  // memory allocated for the string then reallocate more memory.
                  if ( length >= get_size( output ) ) {

                     // Make sure to make room for the terminating null character,
                     // and add a few more bytes to give us a little more space
                     // for next time.
                     int array_size = Utilities::next_positive_multiple_of_8( length );

                     *( static_cast< char ** >( address ) + i ) =
                        static_cast< char * >( TMM_resize_array_1d_a(
                           *( static_cast< char ** >( address ) + i ), array_size ) );

                     output = *( static_cast< unsigned char ** >( address ) + i );
                  }
               } else {
                  // Allocate memory for the sim string and include room for the
                  // terminating null character and add a few more bytes to give
                  // us a little more space for next time.
                  int array_size = Utilities::next_positive_multiple_of_8( length );

                  *( static_cast< char ** >( address ) + i ) =
                     static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );

                  output = *( static_cast< unsigned char ** >( address ) + i );
               }

               if ( output == NULL ) {
                  ostringstream errmsg;
                  errmsg << "Parameter::decode_string_from_buffer():" << __LINE__
                         << " ERROR: Could not allocate memory for ENCODING_ASCII_STRING"
                         << " parameter '" << FOM_name << "' and length "
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
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA parameter '%s', decoded length %d < 0, will use 0 instead.\n",
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
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA parameter '%s', decoded length %d > data buffer size \
%d, will use the data buffer size instead.\n",
                                __LINE__, FOM_name, length, data_buff_size );
               length = data_buff_size;
            }

            // Get a pointer to the output.
            output = *( static_cast< unsigned char ** >( address ) );

            if ( output != NULL ) {
               // The output array size must exactly match the incoming data
               // size for opaque data.
               if ( length != get_size( output ) ) {
                  // WORKAROUND: Trick 10 can't handle a length of zero so to
                  // workaround the memory manager problem use a size of 1 in
                  // the allocation.
                  *( static_cast< char ** >( address ) ) =
                     static_cast< char * >( TMM_resize_array_1d_a(
                        *( static_cast< char ** >( address ) ),
                        ( ( length > 0 ) ? length : 1 ) ) );

                  output = *( static_cast< unsigned char ** >( address ) );
               }
            } else {
               // Allocate memory for the output array.
               // WORKAROUND: Trick 10 can't handle a length of zero so to
               // workaround the memory manager problem use a size of 1 in
               // the allocation.
               *( static_cast< char ** >( address ) ) =
                  static_cast< char * >( TMM_declare_var_1d( "char", ( ( length > 0 ) ? length : 1 ) ) );

               output = *( static_cast< unsigned char ** >( address ) );
            }

            if ( output == NULL ) {
               ostringstream errmsg;
               errmsg << "Parameter::decode_string_from_buffer():" << __LINE__
                      << " ERROR: Could not allocate memory for ENCODING_OPAQUE_DATA"
                      << " parameter '" << FOM_name << "' and length "
                      << length << "!\n";
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
            int num_elements;
            if ( decoded_count < 0 ) {
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA parameter '%s', decoded element count %d < 0, will use 0 instead.\n",
                                __LINE__, FOM_name, decoded_count );
               num_elements = 0;
            } else {
               num_elements = decoded_count;
            }

            // Handle the situation where more strings are in the input encoding
            // than what exist in the ref-attributes.
            if ( num_elements > num_items ) {
               message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: Truncating array of ENCODING_OPAQUE_DATA from %d to %d elements for parameter '%s'!\n",
                                __LINE__, num_elements, num_items, FOM_name );
               num_elements = num_items;
            }

            // Calculate the size of the data minus the encoded length fields.
            // data_buff_size = size - 4 - 4 * num_elements
            int data_buff_size;
            if ( size > ( 4 * ( 1 + num_elements ) ) ) {
               data_buff_size = size - ( 4 * ( 1 + num_elements ) );
            } else {
               data_buff_size = 0;
            }

            // Decode each of the HLAASCIIstring elements.
            for ( int i = 0; i < num_elements; ++i ) {

               // Decode the length of the string which is an HLAinteger32BE (Big Endian).
               int decoded_inner_count = 0;
               if ( Utilities::get_endianness() == TRICK_BIG_ENDIAN ) {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
               } else {
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 3 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 2 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 1 ) = *( static_cast< unsigned char * >( input++ ) );
                  *( ( reinterpret_cast< unsigned char * >( &decoded_inner_count ) ) + 0 ) = *( static_cast< unsigned char * >( input++ ) );
               }

               // Do a sanity check on the decoded length, it should not be negative.
               int length;
               if ( decoded_inner_count < 0 ) {
                  message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA array element %d, parameter '%s', the decoded \
length %d < 0, will use 0 instead.\n",
                                   __LINE__, i, FOM_name, decoded_inner_count );
                  length = 0;
               } else {
                  length = decoded_inner_count;
               }

               // Do a sanity check on the decoded length as compared to how much
               // data remains in the buffer.
               if ( length > data_buff_size ) {
                  message_publish( MSG_WARNING, "Parameter::decode_string_from_buffer():%d \
WARNING: For ENCODING_OPAQUE_DATA array element %d, parameter '%s', the decoded \
length %d > data buffer size %d, will use the data buffer size instead.\n",
                                   __LINE__, i, FOM_name, length, data_buff_size );
                  length = data_buff_size;
               }

               // Adjust the amount of data left in the buffer.
               if ( data_buff_size >= length ) {
                  data_buff_size -= length;
               } else {
                  data_buff_size = 0;
               }

               // 8-big characters.
               output = *( static_cast< unsigned char ** >( address ) + i );

               if ( output != NULL ) {
                  // The output array size must exactly match the incoming data
                  // size for opaque data.
                  if ( length != get_size( output ) ) {
                     // WORKAROUND: Trick 10 can't handle a length of zero so to
                     // workaround the memory manager problem use a size of 1 in
                     // the allocation.
                     *( static_cast< char ** >( address ) + i ) =
                        static_cast< char * >( TMM_resize_array_1d_a(
                           *( static_cast< char ** >( address ) + i ),
                           ( ( length > 0 ) ? length : 1 ) ) );

                     output = *( static_cast< unsigned char ** >( address ) + i );
                  }
               } else {
                  // Allocate memory for the output array.
                  // WORKAROUND: Trick 10 can't handle a length of zero so to
                  // workaround the memory manager problem use a size of 1 in
                  // the allocation.
                  *( static_cast< char ** >( address ) + i ) =
                     static_cast< char * >( TMM_declare_var_1d( "char", ( ( length > 0 ) ? length : 1 ) ) );

                  output = *( static_cast< unsigned char ** >( address ) + i );
               }

               if ( output == NULL ) {
                  ostringstream errmsg;
                  errmsg << "Parameter::decode_string_from_buffer():" << __LINE__
                         << " ERROR: Could not allocate memory for ENCODING_OPAQUE_DATA"
                         << " parameter '" << FOM_name << "' and length "
                         << length << "!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }

               // Copy the characters over.
               if ( length > 0 ) {
                  memcpy( output, input, length );
                  input += length;
               }

               // Skip the padding which was added to keep the data on a 4 byte
               // boundary. The last element gets no padding.
               int pad = ( 4 + length ) % 4;
               if ( ( i < ( num_items - 1 ) ) && ( pad != 0 ) ) {
                  input += ( 4 - pad );

                  // Adjust the amount of data left in the buffer.
                  if ( data_buff_size >= ( 4 - pad ) ) {
                     data_buff_size -= ( 4 - pad );
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
         output = *( static_cast< unsigned char ** >( address ) );

         if ( output == NULL ) {
            ostringstream errmsg;
            errmsg << "Parameter::decode_string_from_buffer():" << __LINE__
                   << " ERROR: For ENCODING_NONE, Parameter '" << FOM_name
                   << "' with Trick name '" << trick_name << "' is NULL!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // The existing output "char *" variable size must exactly match the
         // incoming data size for no-encoding of the data.
         if ( size != get_size( output ) ) {
            ostringstream errmsg;
            errmsg << "Parameter::decode_string_from_buffer():" << __LINE__
                   << " ERROR: For ENCODING_NONE, Parameter '" << FOM_name
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
         for ( int i = 0; i < num_items; ++i ) {

            // Find the end of the encoded string which is the null character.
            while ( *( input + end_index ) != '\0' ) {
               ++end_index;
            }

            int length = ( end_index - start_index ) + 1;

            if ( *( static_cast< char ** >( address ) + i ) != NULL ) {

               // Determine if we need to allocate more memory for the sim string.
               if ( length >= get_size( *( static_cast< char ** >( address ) + i ) ) ) {
                  int array_size = Utilities::next_positive_multiple_of_8( length );

                  *( static_cast< char ** >( address ) + i ) =
                     static_cast< char * >( TMM_resize_array_1d_a( *( static_cast< char ** >( address ) + i ), array_size ) );
               }
            } else {
               // Allocate memory for the sim string.
               int array_size = Utilities::next_positive_multiple_of_8( length );

               *( static_cast< char ** >( address ) + i ) = static_cast< char * >( TMM_declare_var_1d( "char", array_size ) );
            }

            if ( *( static_cast< char ** >( address ) + i ) == NULL ) {
               ostringstream errmsg;
               errmsg << "Parameter::decode_string_from_buffer():" << __LINE__
                      << " ERROR: Could not allocate memory for ENCODING_C_STRING"
                      << " parameter '" << FOM_name << "' and length "
                      << Utilities::next_positive_multiple_of_8( length )
                      << "!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
               return;
            }

            memcpy( *( static_cast< char ** >( address ) + i ),
                    ( input + start_index ),
                    length );

            // Move to the next encoded string in the input.
            ++end_index;
            start_index = end_index;
         }
         break;
      }
   }
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - The destination must be large enough to hold num_bytes of data.
 * - Only primitive types and static arrays of primitive type are supported
 * for now.
 */
void Parameter::byteswap_buffer_copy(
   void       *dest,
   void const *src,
   int const   type,
   int const   length,
   int const   num_bytes ) const
{
   if ( num_bytes == 0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
         ostringstream msg;
         msg << "Parameter::byteswap_buffer_copy():" << __LINE__
             << " WARNING: FOM Parameter '" << FOM_name << "' with Trick name '"
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

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Only primitive types and static arrays of primitive type are supported
 * for now.
 */
bool Parameter::is_supported_parameter_type() const
{
   if ( attr == NULL ) {
      return false;
   }

   // Allow 1-D dynamic arrays: 'char *', 'unsigned char *' and 'bool *' etc.
   if ( attr->type != TRICK_STRING ) {
      // For now, we do not support more than a 1-D array that is dynamic
      // (i.e. a pointer such as char *). If the size of the last indexed
      // attribute is zero then it is a pointer.
      if ( ( attr->num_index > 1 ) && ( attr->index[attr->num_index - 1].size == 0 ) ) {
         return false;
      }
   } else {
      // String type:
      // We only support static arrays for now so check for a pointer to an
      // array (i.e. non-static arrays) which is when we have an array
      // (i.e. num_index > 0) and the size of any of the dimensions is zero
      // (i.e. index[i].size == 0).
      if ( attr->num_index > 0 ) {
         // Make sure each dimension is statically defined (i.e. not zero).
         for ( int i = attr->num_index - 1; i >= 0; i-- ) {
            // If the size is zero then the array is dynamic in size (i.e. a pointer)
            if ( attr->index[i].size == 0 ) {
               return false;
            }
         }
      }
   }

   switch ( attr->type ) {
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
         if ( ( rti_encoding == ENCODING_NONE ) && ( attr->num_index != 0 ) ) {
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
         if ( ( rti_encoding == ENCODING_LOGICAL_TIME ) && ( attr->num_index > 0 ) ) {
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

void Parameter::print_buffer() const
{
   ostringstream msg;
   msg << "Parameter::print_buffer():" << __LINE__ << '\n'
       << " FOM-name:'" << get_FOM_name() << "'"
       << " type:" << attr->type
       << " byteswap:" << ( is_byteswap() ? "Yes" : "No" )
       << " num_items:" << num_items
       << " size:" << size
       << '\n';

   // For now we only support an parameter of type double for printing. DDexter
   if ( attr->type == TRICK_DOUBLE ) {

      double const *dbl_array = reinterpret_cast< double const * >( buffer ); // cppcheck-suppress [invalidPointerCast]

      if ( is_byteswap() ) {
         for ( int i = 0; i < num_items; ++i ) {
            // undo the byteswap for display
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
