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
@trick_link_dependency{RecordElement.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}
@trick_link_dependency{encoding/EncoderBase.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Aug 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Encoder helpers rewrite.}
@revs_end

*/

// System includes.
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/attributes.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/RecordElement.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/EncodingExceptions.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Parameter::Parameter()
   : RecordElement(),
     FOM_name(),
     value_received( false ),
     interaction_FOM_name(),
     param_handle()
{
   return;
}

/*!
 * @details Frees the Trick allocated memory.
 * @job_class{shutdown}
 */
Parameter::~Parameter()
{
   if ( encoder != NULL ) {
      delete encoder;
      encoder = NULL;
   }
}

/*!
 * @job_class{initialization}
 */
void Parameter::initialize(
   string const &interaction_fom_name,
   int const     interaction_index,
   int const     parameter_index )
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Make sure we have a valid parameter FOM name.
   if ( FOM_name.empty() ) {
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

   if ( rti_encoding != ENCODING_FIXED_RECORD ) {
      // Make sure we have a valid parameter Trick-Name.
      if ( trick_name.empty() ) {
         ostringstream errmsg;
         errmsg << "Parameter::initialize():" << __LINE__
                << " ERROR: FOM Interaction Parameter '"
                << interaction_fom_name << "'->'" << get_FOM_name() << "' has a missing"
                << " Trick name for the parameter. Make sure 'THLA.manager.interactions["
                << interaction_index << "].parameters[" << parameter_index
                << "].trick_name' in either your input.py file or modified-data files"
                << " is correctly specified.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // Do a quick bounds check on the rti_encoding value.
   if ( ( rti_encoding < ENCODING_FIRST_VALUE ) || ( rti_encoding > ENCODING_LAST_VALUE ) ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize():" << __LINE__
             << " ERROR: FOM Interaction Parameter '"
             << interaction_fom_name << "'->'" << get_FOM_name() << "' with Trick name '"
             << get_trick_name() << "' has an 'rti_encoding' value of "
             << rti_encoding << " which is out of the valid range of "
             << ENCODING_FIRST_VALUE << " to " << ENCODING_LAST_VALUE
             << ". Please check your input or modified-data files to make sure"
             << " the value for the 'rti_encoding' is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Initialize the element encoders including a fixed record encoder.
   initialize_element_encoder();

   if ( this->encoder == NULL ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize():" << __LINE__
             << " ERROR: Unexpected NULL encoder for Trick variable '"
             << get_trick_name() << "' with an 'rti_encoding' value of "
             << rti_encoding << ".\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make a copy.
   this->interaction_FOM_name = string( interaction_fom_name );

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
      string param_handle_string;
      StringUtilities::to_string( param_handle_string, this->param_handle );
      ostringstream msg;
      msg << "Parameter::initialize():" << __LINE__ << '\n'
          << "========================================================\n"
          //        << "  Encoder:" << this->encoder->to_string() << "\n"
          << "  interaction_FOM_name:'" << interaction_FOM_name << "'\n"
          << "  FOM_name:'" << get_FOM_name() << "'\n"
          << "  trick_name:'" << get_trick_name() << "'\n"
          << "  ParameterHandle:" << param_handle_string << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  received:" << ( is_received() ? "Yes" : "No" ) << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*!
 * @job_class{initialization}
 */
void Parameter::initialize(
   string const &interaction_fom_name,
   void         *address,
   ATTRIBUTES   *attr )
{
   if ( address == NULL ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize(char const *, void *, ATTRIBUTES *):" << __LINE__
             << " ERROR: For FOM Interaction Parameter '" << interaction_fom_name
             << "'. Unexpected NULL trick variable address.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   if ( attr == NULL ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize(char const *, void *, ATTRIBUTES *):" << __LINE__
             << " ERROR: For FOM Interaction Parameter '" << interaction_fom_name
             << "'. Unexpected NULL ATTRIBUTES pointer.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make a copy.
   this->interaction_FOM_name = string( interaction_fom_name );

   // Initialize the element encoders including a fixed record encoder.
   initialize_element_encoder( address, attr );

   if ( this->encoder == NULL ) {
      ostringstream errmsg;
      errmsg << "Parameter::initialize():" << __LINE__
             << " ERROR: Unexpected NULL encoder for Trick variable '"
             << attr->name << "' with an 'rti_encoding' value of "
             << rti_encoding << ".\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_PARAMETER ) ) {
      string param_handle_string;
      StringUtilities::to_string( param_handle_string, this->param_handle );
      ostringstream msg;
      msg << "Parameter::initialize():" << __LINE__ << '\n'
          << "========================================================\n"
          << "  Encoder:\n"
          << "  interaction_FOM_name:'" << interaction_FOM_name << "'\n"
          << "  FOM_name:'" << get_FOM_name() << "'\n"
          << "  trick_name:'" << get_trick_name() << "'\n"
          << "  ParameterHandle:" << param_handle_string << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  received:" << ( is_received() ? "Yes" : "No" ) << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

VariableLengthData &Parameter::encode()
{
   update_before_encode();

   try {
      encoder->encode( encoder->data );
   } catch ( EncoderException &e ) {
      string err_details;
      StringUtilities::to_string( err_details, e.what() );
      ostringstream errmsg;
      errmsg << "Parameter::encode():" << __LINE__
             << " ERROR: Unexpected error encoding HLA data for Trick variable '"
             << get_trick_name() << "' and FOM name '"
             << get_FOM_name() << "' with error: " << err_details << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return encoder->data;
}

bool const Parameter::decode(
   VariableLengthData const &encoded_data )
{
   try {
      encoder->decode( encoded_data );
   } catch ( EncoderException &e ) {
      string err_details;
      StringUtilities::to_string( err_details, e.what() );
      ostringstream errmsg;
      errmsg << "Parameter::decode():" << __LINE__
             << " ERROR: Unexpected error decoding HLA data for Trick variable '"
             << get_trick_name() << "' and FOM name '"
             << get_FOM_name() << "' with error: " << err_details << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   update_after_decode();

   if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      message_publish( MSG_NORMAL, "Parameter::decode():%d Decoded '%s' (trick_name '%s') from attribute map.\n",
                       __LINE__, get_FOM_name().c_str(), get_trick_name().c_str() );
   }

   // Mark the attribute value as received and changed.
   mark_received();

   return true;
}
