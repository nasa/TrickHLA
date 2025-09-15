/*!
@file TrickHLA/encoding/VariableArrayEncoderBase.cpp
@ingroup TrickHLA
@brief This class represents the variable array base encoder implementation.

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
@trick_link_dependency{VariableArrayEncoderBase.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}
@trick_link_dependency{../Utilities.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <iostream>
#include <ostream>
#include <sstream>
#include <stddef.h>
#include <string>

// Trick include files.
#include "trick/attributes.h"
#include "trick/io_alloc.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

VariableArrayEncoderBase::VariableArrayEncoderBase(
   void         *addr,
   ATTRIBUTES   *attr,
   string const &name )
   : EncoderBase( name ),
     address( addr ),
     type( ( attr != NULL ) ? attr->type : TRICK_VOID ),
     var_element_count( 0 ),
     data_elements()
{
   if ( attr == NULL ) {
      ostringstream errmsg;
      errmsg << "VariableArrayEncoderBase::VariableArrayEncoderBase():" << __LINE__
             << " ERROR: Unexpected NULL Trick attributes. Please make sure the"
             << " variable is allocated memory by the Trick Memory Manager."
             << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Set the persistent flags about the trick variable.
   this->is_array_flag         = ( attr->num_index > 0 );
   this->is_1d_array_flag      = ( attr->num_index == 1 );
   this->is_static_array_flag  = is_array() && ( attr->index[attr->num_index - 1].size != 0 );
   this->is_dynamic_array_flag = is_array() && ( attr->index[attr->num_index - 1].size == 0 );

   if ( this->address == NULL ) {
      ostringstream errmsg;
      errmsg << "VariableArrayEncoderBase::VariableArrayEncoderBase():" << __LINE__
             << " ERROR: The variable address is NULL for variable '"
             << data_name << "'. Please make sure the Trick variable"
             << " is allocated memory by the Trick Memory Manager." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( is_static_in_size() ) {
      this->var_element_count = Utilities::get_static_var_element_count( attr );
   } else {
      // Dynamic variable array size calculation.
      calculate_var_element_count();
   }
}

VariableArrayEncoderBase::~VariableArrayEncoderBase()
{
   while ( !data_elements.empty() ) {
      delete data_elements.back();
      data_elements.back() = NULL;
      data_elements.pop_back();
   }
}

void VariableArrayEncoderBase::update_before_encode()
{
   return;
}

void VariableArrayEncoderBase::update_after_decode()
{
   return;
}

int const VariableArrayEncoderBase::get_data_size()
{
   int byte_count = 0;

   if ( address != NULL ) {
      calculate_var_element_count();

      switch ( type ) {
         case TRICK_STRING: {
            if ( is_dynamic_array() ) {
               string *str_array = *static_cast< std::string ** >( address );
               for ( size_t i = 0; i < var_element_count; ++i ) {
                  byte_count += str_array[i].size(); // cppcheck-suppress [useStlAlgorithm]
               }
            } else {
               byte_count = var_element_count;
            }
            break;
         }
         case TRICK_WSTRING: {
            if ( is_dynamic_array() ) {
               size_t   wchar_size = sizeof( wchar_t );
               wstring *wstr_array = *static_cast< std::wstring ** >( address );
               for ( size_t i = 0; i < var_element_count; ++i ) {
                  byte_count += ( wchar_size * wstr_array[i].size() ); // cppcheck-suppress [useStlAlgorithm]
               }
            } else {
               byte_count = var_element_count;
            }
            break;
         }
         default: {
            byte_count = var_element_count;
            break;
         }
      }
   }
   return byte_count;
}

void VariableArrayEncoderBase::calculate_var_element_count()
{
   if ( is_dynamic_array() ) {
      // Dynamic array that is a pointer so check for NULL.
      if ( *static_cast< void ** >( address ) != NULL ) {
         // get_size returns the number of elements in the dynamic array.
         int num_items = get_size( *static_cast< void ** >( address ) );
         if ( num_items <= 0 ) {
            ALLOC_INFO *alloc_info = get_alloc_info_of( *static_cast< void ** >( address ) );
            if ( alloc_info != NULL ) {
               num_items = alloc_info->num;
            }
         }
         this->var_element_count = ( num_items >= 0 ) ? num_items : 0;
      } else {
         this->var_element_count = 0;
      }
   }
}

void VariableArrayEncoderBase::resize_trick_var(
   size_t const new_size )
{
   // Trick array variable size does not match the new size.
   if ( is_dynamic_array()
        && ( new_size > 0 )
        && ( ( new_size != var_element_count )
             || ( *( static_cast< void ** >( address ) ) == NULL ) ) ) {

      if ( this->type == TRICK_STRING ) {
         // TMM_resize_array_1d_a does not support STL strings.
         if ( *( static_cast< void ** >( address ) ) != NULL ) {
            TMM_delete_var_a( *( static_cast< void ** >( address ) ) );
         }
         *( static_cast< void ** >( address ) ) =
            static_cast< void * >( TMM_declare_var_1d( "std::string", (int)new_size ) );
      } else {
         if ( *( static_cast< void ** >( address ) ) == NULL ) {
            *( static_cast< void ** >( address ) ) =
               static_cast< void * >( TMM_declare_var_1d(
                  trickTypeCharString( this->type, "UNSUPPORTED_TYPE" ), (int)new_size ) );
         } else {
            *( static_cast< void ** >( address ) ) =
               static_cast< void * >( TMM_resize_array_1d_a(
                  *( static_cast< void ** >( address ) ), (int)new_size ) );
         }
      }

      // Update the element count to the new size.
      this->var_element_count = new_size;

      if ( *static_cast< void ** >( address ) == NULL ) {
         ostringstream errmsg;
         errmsg << "VariableArrayEncoderBase::resize_trick_var():" << __LINE__
                << " ERROR: Could not allocate memory for Trick variable"
                << " '" << data_name << "' of type '"
                << trickTypeCharString( this->type, "UNSUPPORTED_TYPE" )
                << "' for " << new_size << " elements!" << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}
