/*!
@file TrickHLA/encoding/CharUnicodeStringEncoder.cpp
@ingroup TrickHLA
@brief This class represents the char array Unicode string encoder implementation.

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
@trick_link_dependency{CharUnicodeStringEncoder.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{VariableArrayEncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/attributes.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/encoding/CharUnicodeStringEncoder.hh"
#include "TrickHLA/encoding/VariableArrayEncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/encoding/BasicDataElements.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

CharUnicodeStringEncoder::CharUnicodeStringEncoder(
   void         *addr,
   ATTRIBUTES   *attr,
   string const &name )
   : VariableArrayEncoderBase( addr, attr, name ),
     wstring_data()
{
   if ( ( this->type != TRICK_CHARACTER )
        && ( this->type != TRICK_UNSIGNED_CHARACTER ) ) {
      ostringstream errmsg;
      errmsg << "CharUnicodeStringEncoder::CharUnicodeStringEncoder():" << __LINE__
             << " ERROR: Trick type for the '" << data_name
             << "' simulation variable (type:"
             << trickTypeCharString( this->type, "UNSUPPORTED_TYPE" )
             << ") is not the expected type '"
             << trickTypeCharString( TRICK_CHARACTER, "UNSUPPORTED_TYPE" )
             << "'." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( !is_dynamic_array() ) {
      ostringstream errmsg;
      errmsg << "CharUnicodeStringEncoder::CharUnicodeStringEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << data_name
             << "' the variable must be a dynamic variable array!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   this->data_encoder = new HLAunicodeString( &wstring_data );
}

CharUnicodeStringEncoder::~CharUnicodeStringEncoder()
{
   return;
}

void CharUnicodeStringEncoder::update_before_encode()
{
   // Convert the char * string into a wide string.
   StringUtilities::to_wstring( this->wstring_data, *static_cast< char ** >( address ) );
}

void CharUnicodeStringEncoder::update_after_decode()
{
   calculate_var_element_count();

   // Include the null terminating char in the size comparison.
   if ( ( wstring_data.size() + 1 ) <= var_element_count ) {
      // Convert from wstring to string.
      string str_data;
      str_data.assign( wstring_data.begin(), wstring_data.end() );

      // Copy value to existing Trick variable char* memory and include
      // the null terminating character in the c_str().
      memcpy( *static_cast< void ** >( address ), // flawfinder: ignore
              str_data.c_str(),
              str_data.size() + 1 );
   } else {
      // Need to make the Trick variable char* bigger by reallocating.

      // Don't leak memory by deleting the old char* string allocation.
      if ( *static_cast< void ** >( address ) != NULL ) {
         TMM_delete_var_a( *( static_cast< void ** >( address ) ) );
      }

      // Convert from the wide-string to a char* string.
      if ( wstring_data.size() > 0 ) {
         *static_cast< char ** >( address ) = StringUtilities::mm_strdup_wstring( wstring_data );
      } else {
         char empty = '\0';

         // Zero length so allocate and set the null terminating character.
         *static_cast< char ** >( address ) = TMM_strdup( &empty );
      }
   }
}
