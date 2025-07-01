/*!
@file TrickHLA/RecordElement.cpp
@ingroup TrickHLA
@brief This class represents the record element implementation.

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
@trick_link_dependency{RecordElement.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{encoding/EncoderBase.cpp}
@trick_link_dependency{encoding/EncoderFactory.cpp}
@trick_link_dependency{encoding/FixedRecordEncoder.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

// System includes.
#include <sstream>
#include <string>

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/RecordElement.hh"
#include "TrickHLA/encoding/EncoderFactory.hh"
#include "TrickHLA/encoding/FixedRecordEncoder.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/HLAfixedRecord.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

RecordElement::RecordElement()
   : trick_name(),
     rti_encoding( ENCODING_UNKNOWN ),
     element_count( 0 ),
     elements( NULL ),
     encoder( NULL )
{
   return;
}

RecordElement::~RecordElement()
{
   if ( elements != NULL ) {
      elements      = NULL;
      element_count = 0;
   }
   if ( encoder != NULL ) {
      delete encoder;
      encoder = NULL;
   }
}

string RecordElement::to_string()
{
   ostringstream msg;
   msg << "RecordElement[" << get_trick_name()
       << ", " << encoding_enum_to_string( rti_encoding )
       << ", element_count:" << element_count << "]";
   return msg.str();
}

void RecordElement::initialize_element_encoder()
{
   if ( rti_encoding == ENCODING_FIXED_RECORD ) {

      // If we have an element count but no elements then let the user know.
      if ( ( element_count > 0 ) && ( elements == NULL ) ) {
         ostringstream errmsg;
         errmsg << "RecordElement::initialize_element_encoder():" << __LINE__
                << " ERROR: For element with trick_name '" << get_trick_name()
                << "', the 'element_count' is " << element_count
                << " but no 'elements' are specified. Please check your input.py"
                << " or modified-data files to make sure the attributes are"
                << " correctly specified." << std::endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      // If we have elements but the element-count is invalid then let
      // the user know.
      if ( ( element_count <= 0 ) && ( elements != NULL ) ) {
         ostringstream errmsg;
         errmsg << "RecordElement::initialize_element_encoder():" << __LINE__
                << " ERROR: For element with trick_name '" << get_trick_name()
                << "', the 'element_count' is " << element_count
                << " but 'elements' are specified. Please check your input.py"
                << " or modified-data files to make sure the elements are"
                << " correctly specified." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      if ( elements == NULL ) {
         ostringstream errmsg;
         errmsg << "RecordElement::initialize_element_encoder():" << __LINE__
                << " ERROR: For element with trick_name '" << get_trick_name()
                << "', no 'elements' have been specified. Please check your"
                << " input.py or modified-data files to make sure the elements"
                << " are correctly specified." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      this->encoder                 = new FixedRecordEncoder();
      HLAfixedRecord *hla_fixed_rec = dynamic_cast< HLAfixedRecord * >( this->encoder->data_encoder );

      for ( int i = 0; i < element_count; ++i ) {
         elements[i].initialize_element_encoder();
         hla_fixed_rec->appendElementPointer( elements[i].encoder );
      }
   } else {
      if ( trick_name.empty() ) {
         ostringstream errmsg;
         errmsg << "RecordElement::initialize_element_encoder():" << __LINE__
                << " ERROR: The fixed record element has a missing Trick name."
                << " Make sure the trick_name is set in either your input.py"
                << " file or modified-data files is correctly specified."
                << std::endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      this->encoder = EncoderFactory::create( trick_name, rti_encoding );
   }
}

void RecordElement::initialize_element_encoder(
   void       *address,
   ATTRIBUTES *attr )
{
   if ( rti_encoding == ENCODING_FIXED_RECORD ) {
      ostringstream errmsg;
      errmsg << "RecordElement::initialize_element_encoder():" << __LINE__
             << " ERROR: For element with trick_name '" << get_trick_name()
             << "', this function does not support the ENCODING_FIXED_RECORD"
             << " encoding for a given address and ATTRIBUTES." << std::endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   this->encoder = EncoderFactory::create( address, attr, rti_encoding );
}

void RecordElement::update_before_encode()
{
   if ( rti_encoding == ENCODING_FIXED_RECORD ) {
      for ( int i = 0; i < element_count; ++i ) {
         elements[i].update_before_encode();
      }
   } else {
      encoder->update_before_encode();
   }
}

void RecordElement::update_after_decode()
{
   if ( rti_encoding == ENCODING_FIXED_RECORD ) {
      for ( int i = 0; i < element_count; ++i ) {
         elements[i].update_after_decode();
      }
   } else {
      encoder->update_after_decode();
   }
}
