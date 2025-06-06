/*!
@file TrickHLA/encoding/EncoderBase.cpp
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
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/attributes.h"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/EncodingExceptions.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

EncoderBase::EncoderBase(
   void       *addr,
   ATTRIBUTES *attr )
   : address( addr ),
     var_element_count( 0 ),
     data(),
     data_elements(),
     encoder( NULL )
{
   if ( attr == NULL ) {
      ostringstream errmsg;
      errmsg << "EncoderBase::EncoderBase():" << __LINE__
             << " ERROR: Unexpected NULL Trick attributes. Please make sure the"
             << " variable is allocated memory by the Trick Memory Manager."
             << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Set the variable values now that the attribute is not NULL.
   this->name = ( attr->name != NULL ) ? attr->name : "";
   this->type = attr->type;

   // Set the persistent flags about the trick variable.
   this->is_array_flag         = ( attr->num_index > 0 );
   this->is_1d_array_flag      = ( attr->num_index == 1 );
   this->is_static_array_flag  = is_array() && ( attr->index[attr->num_index - 1].size != 0 );
   this->is_dynamic_array_flag = is_array() && ( attr->index[attr->num_index - 1].size == 0 );

   if ( address == NULL ) {
      ostringstream errmsg;
      errmsg << "EncoderBase::EncoderBase():" << __LINE__
             << " ERROR: The variable address is NULL for variable '"
             << this->name << "'. Please make sure the Trick variable"
             << " is allocated memory by the Trick Memory Manager."
             << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( is_static_in_size() ) {
      // The user variable is either a primitive type or a static
      // multi-dimension array.
      this->var_element_count = 1;
      for ( int i = 0; i < attr->num_index; ++i ) {
         if ( attr->index[i].size > 0 ) {
            this->var_element_count *= attr->index[i].size;
         }
      }
   } else {
      // Dynamic variable array size calculation.
      calculate_var_element_count();
   }
}

EncoderBase::~EncoderBase()
{
   while ( !data_elements.empty() ) {
      delete data_elements.back();
      data_elements.back() = NULL;
      data_elements.pop_back();
   }

   if ( encoder != NULL ) {
      delete encoder;
      encoder = NULL;
   }
}

VariableLengthData &EncoderBase::encode()
{
   try {
      encoder->encode( this->data );
   } catch ( EncoderException &e ) {
      string err_details;
      StringUtilities::to_string( err_details, e.what() );
      ostringstream errmsg;
      errmsg << "EncoderBase::encode():" << __LINE__
             << " ERROR: Unexpected error encoding HLA data for Trick variable '"
             << this->name << "' using encoder " << this->to_string()
             << " with error: " << err_details << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream errmsg;
      errmsg << "EncoderBase::encode():" << __LINE__
             << " Trick variable '" << this->name
             << "' with encoded length " << encoder->getEncodedLength()
             << " using encoder " << this->to_string() << std::endl;
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
   }

   return this->data;
}

bool const EncoderBase::decode(
   VariableLengthData const &encoded_data )
{
   try {
      encoder->decode( encoded_data );
   } catch ( EncoderException &e ) {
      string err_details;
      StringUtilities::to_string( err_details, e.what() );
      ostringstream errmsg;
      errmsg << "EncoderBase::decode():" << __LINE__
             << " WARNING: Unexpected error decoding HLA data for Trick variable '"
             << this->name << "' with encoded length "
             << encoder->getEncodedLength()
             << " using encoder " << this->to_string()
             << " with error: " << err_details << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      return false;
   }
   return true;
}

string EncoderBase::to_string()
{
   return ( "EncoderBase[" + this->name + "]" );
}

void EncoderBase::calculate_var_element_count()
{
   if ( is_dynamic_array() ) {
      // Dynamic array that is a pointer so check for NULL.
      if ( *static_cast< void ** >( address ) == NULL ) {
         this->var_element_count = 0;
      } else {
         // get_size returns the number of elements in the dynamic array.
         int const num_items     = get_size( *static_cast< void ** >( address ) );
         this->var_element_count = ( num_items > 0 ) ? num_items : 0;
      }
   }
}

void EncoderBase::resize_trick_var(
   size_t const new_size )
{
   // Trick array variable size does not match the new size.
   if ( is_dynamic_array()
        && ( ( new_size != var_element_count )
             || ( *( static_cast< void ** >( address ) ) == NULL ) ) ) {

      if ( this->type == TRICK_STRING ) {
         // TMM_resize_array_1d_a does not support STL strings.
         if ( *( static_cast< void ** >( address ) ) != NULL ) {
            TMM_delete_var_a( *( static_cast< void ** >( address ) ) );
         }
         *( static_cast< void ** >( address ) ) =
            static_cast< void * >( TMM_declare_var_1d( "std::string", new_size ) );
      } else {
         if ( *( static_cast< void ** >( address ) ) == NULL ) {
            *( static_cast< void ** >( address ) ) =
               static_cast< void * >( TMM_declare_var_1d(
                  trickTypeCharString( this->type, "UNKNOWN_TYPE" ), new_size ) );
         } else {
            *( static_cast< void ** >( address ) ) =
               static_cast< void * >( TMM_resize_array_1d_a(
                  *( static_cast< void ** >( address ) ), new_size ) );
         }
      }

      // Update the element count to the new size.
      this->var_element_count = new_size;

      if ( *static_cast< void ** >( address ) == NULL ) {
         ostringstream errmsg;
         errmsg << "EncoderBase::resize_trick_var():" << __LINE__
                << " ERROR: Could not allocate memory for Trick variable"
                << " with name '" << this->name << "' and type '"
                << trickTypeCharString( this->type, "UNKNOWN_TYPE" )
                << "' for " << new_size << " elements!" << std::endl;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}
