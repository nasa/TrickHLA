/*!
@file TrickHLA/encoding/StringUnicodeFixedArrayEncoder.cpp
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
@trick_link_dependency{StringUnicodeFixedArrayEncoder.cpp}
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
#include "TrickHLA/encoding/StringUnicodeFixedArrayEncoder.hh"
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

#define USE_DATA_ELEMENTS

StringUnicodeFixedArrayEncoder::StringUnicodeFixedArrayEncoder(
   void         *addr,
   ATTRIBUTES   *attr,
   string const &name )
   : VariableArrayEncoderBase( addr, attr, name )
{
   if ( this->type != TRICK_STRING ) {
      ostringstream errmsg;
      errmsg << "StringUnicodeFixedArrayEncoder::StringUnicodeFixedArrayEncoder():" << __LINE__
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
      errmsg << "StringUnicodeFixedArrayEncoder::StringUnicodeFixedArrayEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << data_name
             << "' the variable must be a static array of 'std::string'!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   HLAfixedArray *array_encoder = new HLAfixedArray( HLAunicodeString(), var_element_count );
   this->data_encoder           = array_encoder;
   string const *array_data     = static_cast< std::string * >( addr );

   std::wstring wstr;
   for ( size_t i = 0; i < var_element_count; ++i ) {
      // Convert from string to wide-string.
      wstr.assign( array_data[i].begin(), array_data[i].end() );
      array_encoder->set( i, HLAunicodeString( wstr ) );
   }
}

StringUnicodeFixedArrayEncoder::~StringUnicodeFixedArrayEncoder()
{
   return;
}

void StringUnicodeFixedArrayEncoder::update_before_encode()
{
   HLAfixedArray *array_encoder = dynamic_cast< HLAfixedArray * >( data_encoder );
   string        *array_data    = static_cast< std::string * >( address ); // NOLINT(bugprone-macro-parentheses)
   std::wstring   wstr;

   for ( size_t i = 0; i < var_element_count; ++i ) {
      // Convert from string to wide-string.
      wstr.assign( array_data[i].begin(), array_data[i].end() );

      const_cast< HLAunicodeString & >( // NOLINT(bugprone-macro-parentheses)
         dynamic_cast< HLAunicodeString const & >(
            array_encoder->get( i ) ) )
         .set( wstr );
   }
}

void StringUnicodeFixedArrayEncoder::update_after_decode()
{
   HLAfixedArray const *array_encoder = dynamic_cast< HLAfixedArray * >( data_encoder );
   string              *array_data    = static_cast< std::string * >( address ); // NOLINT(bugprone-macro-parentheses)

   // Copy the decoded data element values to the Trick array.
   for ( size_t i = 0; i < var_element_count; ++i ) {
      wstring wstr = dynamic_cast< HLAunicodeString const & >( array_encoder->get( i ) ).get();

      // Convert from wide-string to string.
      array_data[i].assign( wstr.begin(), wstr.end() );
   }
}

int const StringUnicodeFixedArrayEncoder::get_data_size()
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
