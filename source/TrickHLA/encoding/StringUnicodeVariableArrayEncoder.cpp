/*!
@file TrickHLA/encoding/StringUnicodeVariableArrayEncoder.cpp
@ingroup TrickHLA
@brief This class represents the std::string Unicode string variable array
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
@trick_link_dependency{StringUnicodeVariableArrayEncoder.cpp}
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
#include <ostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/attributes.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/encoding/StringUnicodeVariableArrayEncoder.hh"
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
#include "RTI/encoding/HLAvariableArray.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

StringUnicodeVariableArrayEncoder::StringUnicodeVariableArrayEncoder(
   void         *addr,
   ATTRIBUTES   *attr,
   string const &name )
   : VariableArrayEncoderBase( addr, attr, name )
{
   if ( this->type != TRICK_STRING ) {
      ostringstream errmsg;
      errmsg << "StringUnicodeVariableArrayEncoder::StringUnicodeVariableArrayEncoder():" << __LINE__
             << " ERROR: Trick type for the '" << data_name
             << "' simulation variable (type:"
             << trickTypeCharString( this->type, "UNSUPPORTED_TYPE" )
             << ") is not the expected type '"
             << trickTypeCharString( TRICK_STRING, "UNSUPPORTED_TYPE" )
             << "'." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( !is_dynamic_array() ) {
      ostringstream errmsg;
      errmsg << "StringUnicodeVariableArrayEncoder::StringUnicodeVariableArrayEncoder():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << data_name
             << "' the variable must be a dynamic array of 'std::string'!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   this->data_encoder = new HLAvariableArray( HLAunicodeString() );
   resize_data_elements( var_element_count );
}

StringUnicodeVariableArrayEncoder::~StringUnicodeVariableArrayEncoder()
{
   return;
}

void StringUnicodeVariableArrayEncoder::update_before_encode()
{
   // Since the Trick variable is dynamic (i.e. a pointer) its size
   // can change at any point so we need to refresh the counts.
   calculate_var_element_count();

   // Ensure the number of data elements matches the Trick variable
   resize_data_elements( var_element_count );

   HLAvariableArray const *array_encoder = dynamic_cast< HLAvariableArray * >( data_encoder );
   string                 *array_data    = *static_cast< std::string ** >( address ); // NOLINT(bugprone-macro-parentheses)

   // Copy the Trick array values to the data elements to be encoded.
   for ( size_t i = 0; i < var_element_count; ++i ) {

      // Convert from string to wide-string.
      wstring wstring_data;
      wstring_data.assign( array_data[i].begin(), array_data[i].end() );

      const_cast< HLAunicodeString & >( // NOLINT(bugprone-macro-parentheses)
         dynamic_cast< HLAunicodeString const & >(
            array_encoder->get( i ) ) )
         .set( wstring_data );
   }
}

void StringUnicodeVariableArrayEncoder::update_after_decode()
{
   HLAvariableArray const *array_encoder = dynamic_cast< HLAvariableArray * >( data_encoder );

   // Resize Trick array variable to match the decoded data size.
   resize_trick_var( array_encoder->size() );

   string *array_data = *static_cast< std::string ** >( address ); // NOLINT(bugprone-macro-parentheses)

   // Copy the decoded data element values to the Trick array.
   for ( size_t i = 0; i < var_element_count; ++i ) {
      wstring wstring_data;
      wstring_data = dynamic_cast< HLAunicodeString const & >( array_encoder->get( i ) ).get();

      // Convert from wide-string to string.
      array_data[i].assign( wstring_data.begin(), wstring_data.end() );
   }
}

int const StringUnicodeVariableArrayEncoder::get_data_size()
{
   int byte_count = 0;
   if ( data_encoder != NULL ) {
      HLAvariableArray const *array_encoder = dynamic_cast< HLAvariableArray * >( data_encoder );
      int const               array_size    = array_encoder->size();
      for ( int i = 0; i < array_size; ++i ) {
         byte_count += ( sizeof( wchar_t ) * dynamic_cast< HLAunicodeString const & >( array_encoder->get( i ) ).get().size() );
      }
   }
   return byte_count;
}

void StringUnicodeVariableArrayEncoder::resize_data_elements(
   size_t const new_size )
{
   HLAvariableArray *array_encoder = dynamic_cast< HLAvariableArray * >( data_encoder );
   if ( new_size != array_encoder->size() ) {
      HLAunicodeString const data_prototype;

      if ( new_size < array_encoder->size() ) {
         // Because we can't resize the encoder to a smaller
         // size we have to create a new one.
         delete data_encoder;
         array_encoder = new HLAvariableArray( data_prototype );
         data_encoder  = array_encoder;
      }

      while ( array_encoder->size() < new_size ) {
         array_encoder->addElement( data_prototype );
      }
   }
}
