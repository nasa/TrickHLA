/*!
@file TrickHLA/encoding/StringUnicodeStringFixedArrayEncoder.cpp
@ingroup TrickHLA
@brief This class represents the std::string Unicode string fixed array
encoder implementation.

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
@trick_link_dependency{StringUnicodeStringFixedArrayEncoder.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{VariableArrayEncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, Sept 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstddef>
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/attributes.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/encoding/StringUnicodeStringFixedArrayEncoder.hh"
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
#include "RTI/encoding/HLAfixedArray.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

StringUnicodeStringFixedArrayEncoder::StringUnicodeStringFixedArrayEncoder(
   void         *addr,
   ATTRIBUTES   *attr,
   string const &name )
   : VariableArrayEncoderBase( addr, attr, name ),
     wstring_data( NULL )
{
   if ( this->type != TRICK_STRING ) {
      ostringstream errmsg;
      errmsg << "StringUnicodeStringFixedArrayEncoder::StringUnicodeStringFixedArrayEncoder():" << __LINE__
             << " ERROR: Trick type for the '" << data_name
             << "' simulation variable (type:"
             << trickTypeCharString( this->type, "UNSUPPORTED_TYPE" )
             << ") is not the expected type '"
             << trickTypeCharString( TRICK_STRING, "UNSUPPORTED_TYPE" )
             << "'." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( !is_static_array() ) {
      ostringstream errmsg;
      errmsg << "StringUnicodeStringFixedArrayEncoder::StringUnicodeStringFixedArrayEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << data_name
             << "' the variable must be a static array of 'std::string'!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   HLAfixedArray *array_encoder = new HLAfixedArray( HLAunicodeString(), var_element_count );
   this->data_encoder           = array_encoder;

   // Create the array of wstring's that holds the data that will be encoded.
   this->wstring_data = static_cast< std::wstring * >( malloc( var_element_count * sizeof( std::wstring ) ) );

   // Connect the users array data to the encoder array elements.
   if ( addr != NULL ) {
      string *array_data = static_cast< std::string * >( addr );
      for ( size_t i = 0; i < var_element_count; ++i ) {
         wstring_data[i].assign( array_data[i].begin(), array_data[i].end() );

         const_cast< HLAunicodeString & >( // NOLINT(bugprone-macro-parentheses)
            dynamic_cast< HLAunicodeString const & >(
               array_encoder->get( i ) ) )
            .setDataPointer( &wstring_data[i] );
      }
   }
}

StringUnicodeStringFixedArrayEncoder::~StringUnicodeStringFixedArrayEncoder()
{
   if ( wstring_data != NULL ) {
      free( wstring_data );
      wstring_data = NULL;
   }
}

void StringUnicodeStringFixedArrayEncoder::update_before_encode()
{
   // Convert the string into a wide-string.
   string *array_data = static_cast< std::string * >( address );
   for ( size_t i = 0; i < var_element_count; ++i ) {
      wstring_data[i].assign( array_data[i].begin(), array_data[i].end() );
   }
}

void StringUnicodeStringFixedArrayEncoder::update_after_decode()
{
   // Convert from wide-string to string.
   string *array_data = static_cast< std::string * >( address );
   for ( size_t i = 0; i < var_element_count; ++i ) {
      array_data[i].assign( wstring_data[i].begin(), wstring_data[i].end() );
   }
}

int const StringUnicodeStringFixedArrayEncoder::get_data_size()
{
   int byte_count = 0;
   if ( data_encoder != NULL ) {
      HLAfixedArray const *array_encoder = dynamic_cast< HLAfixedArray * >( data_encoder );
      int const            array_size    = array_encoder->size();
      for ( int i = 0; i < array_size; ++i ) {
         byte_count += ( sizeof( wchar_t ) * dynamic_cast< HLAunicodeString const & >( array_encoder->get( i ) ).get().size() );
      }
   }
   return byte_count;
}
