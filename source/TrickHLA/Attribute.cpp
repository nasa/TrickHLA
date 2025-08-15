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
@trick_link_dependency{RecordElement.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}
@trick_link_dependency{encoding/EncoderBase.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2006, --, Initial version.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Encoder helpers rewrite.}
@revs_end

*/

// System includes.
#include <cmath>
#include <cstddef>
#include <limits>
#include <ostream>
#include <sstream>

// Trick includes.
#include <trick/message_proto.h>
#include <trick/message_type.h>

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/RecordElement.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

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
 * @details The endianess of the computer is determined as part of the
 * Attribute construction process.
 * @job_class{initialization}
 */
Attribute::Attribute()
   : RecordElement(),
     FOM_name(),
     config( CONFIG_NONE ),
     preferred_order( TRANSPORT_SPECIFIED_IN_FOM ),
     publish( false ),
     subscribe( false ),
     locally_owned( false ),
     cycle_time( -std::numeric_limits< double >::max() ),
     value_changed( false ),
     update_requested( false ),
     cycle_ratio( 1 ),
     cycle_cnt( 0 ),
     pull_requested( false ),
     push_requested( false ),
     divest_requested( false ),
     attr_handle()
{
   return;
}

/*!
 * @details The buffer and ref2 values are freed and nulled.
 * @job_class{shutdown}
 */
Attribute::~Attribute()
{
   return;
}

void Attribute::initialize(
   std::string const &obj_FOM_name,
   int const          object_index,
   int const          attribute_index )
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Make sure we have a valid Object FOM name.
   if ( obj_FOM_name.empty() ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: Unexpected NULL Object FOM-Name argument passed to this"
             << " function.";
      if ( !get_FOM_name().empty() ) {
         errmsg << " For FOM Attribute Named '" << get_FOM_name() << "'.";
      }
      errmsg << '\n';
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make sure we have a valid attribute FOM name.
   if ( get_FOM_name().empty() ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: Object with FOM Name '" << obj_FOM_name << "' has a missing"
             << " FOM name for the attribute. Make sure THLA.manager.objects["
             << object_index << "].attributes[" << attribute_index
             << "].FOM_name' in either your input.py file or modified-data files"
             << " is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( rti_encoding != ENCODING_FIXED_RECORD ) {
      // Make sure we have a valid attribute Trick-Name.
      if ( trick_name.empty() ) {
         ostringstream errmsg;
         errmsg << "Attribute::initialize():" << __LINE__
                << " ERROR: FOM Object Attribute '"
                << obj_FOM_name << "'->'" << get_FOM_name() << "' has a missing Trick name"
                << " for the attribute. Make sure THLA.manager.objects["
                << object_index << "].attributes[" << attribute_index
                << "].trick_name' in either your input.py file or modified-data files"
                << " is correctly specified.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // Do a quick bounds check on the 'rti_encoding' value.
   if ( ( rti_encoding < ENCODING_FIRST_VALUE ) || ( rti_encoding > ENCODING_LAST_VALUE ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '"
             << obj_FOM_name << "'->'" << get_FOM_name() << "' with Trick name '"
             << get_trick_name() << "' has an 'rti_encoding' value of " << rti_encoding
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
             << obj_FOM_name << "'->'" << get_FOM_name() << "' with Trick name '"
             << get_trick_name() << "' has an invalid 'preferred_order' and it must be"
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
             << obj_FOM_name << "'->'" << get_FOM_name() << "' with Trick name '"
             << get_trick_name() << "' has a 'config' value of " << config
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
                << obj_FOM_name << "'->'" << get_FOM_name() << "' with Trick name '"
                << get_trick_name() << "' has a 'config' value of CONFIG_TYPE_NONE.\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }

   // Do a bounds check on the 'cycle_time' value. Once we have a valid
   // job-cycle-time we will do another bounds check against that value.
   if ( ( this->cycle_time <= 0.0 ) && ( this->cycle_time > -std::numeric_limits< double >::max() ) ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: FOM Object Attribute '"
             << obj_FOM_name << "'->'" << get_FOM_name()
             << "' with Trick name '" << get_trick_name()
             << "' has a 'cycle_time' value of " << this->cycle_time
             << " seconds, which is not valid. The 'cycle_time' must be > 0."
             << " Please check your input or modified-data files to make sure"
             << " the value for the 'cycle_time' is correctly specified." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Initialize the element encoders including a fixed record encoder.
   initialize_element_encoder();

   if ( this->encoder == NULL ) {
      ostringstream errmsg;
      errmsg << "Attribute::initialize():" << __LINE__
             << " ERROR: Unexpected NULL encoder for Trick variable '"
             << get_trick_name() << "' with an 'rti_encoding' value of "
             << encoding_enum_to_string( rti_encoding ) << ".\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      ostringstream msg;
      msg << "Attribute::initialize():" << __LINE__ << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      string attr_handle_string;
      StringUtilities::to_string( attr_handle_string, this->attr_handle );
      ostringstream msg;
      msg << "Attribute::initialize():" << __LINE__ << '\n'
          << "========================================================\n"
          << "  FOM_name:'" << get_FOM_name() << "'\n"
          << "  trick_name:'" << get_trick_name() << "'\n"
          << "  AttributeHandle:" << attr_handle_string << '\n'
          << "  publish:" << publish << '\n'
          << "  subscribe:" << subscribe << '\n'
          << "  locally_owned:" << locally_owned << '\n'
          << "  rti_encoding:" << rti_encoding << '\n'
          << "  changed:" << ( is_changed() ? "Yes" : "No" ) << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

VariableLengthData &Attribute::encode()
{
   update_before_encode();

   try {
      encoder->encode( encoder->data );
   } catch ( EncoderException &e ) {
      string err_details;
      StringUtilities::to_string( err_details, e.what() );
      ostringstream errmsg;
      errmsg << "Attribute::encode():" << __LINE__
             << " ERROR: Unexpected error encoding HLA data for Trick variable '"
             << get_trick_name() << "' and FOM name '" << get_FOM_name()
             << "' with error: " << err_details << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return encoder->data;
}

bool const Attribute::decode(
   VariableLengthData const &encoded_data )
{
   try {
      encoder->decode( encoded_data );
   } catch ( EncoderException &e ) {
      string err_details;
      StringUtilities::to_string( err_details, e.what() );
      ostringstream errmsg;
      errmsg << "Attribute::decode():" << __LINE__
             << " ERROR: Unexpected error decoding HLA data for Trick variable '"
             << get_trick_name() << "' and FOM name '" << get_FOM_name()
             << "' with error: " << err_details << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   update_after_decode();

   if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      message_publish( MSG_NORMAL, "Attribute::decode():%d Decoded '%s' (trick_name '%s') from attribute map.\n",
                       __LINE__, get_FOM_name().c_str(), get_trick_name().c_str() );
   }

   // Mark the attribute value as changed.
   mark_changed();

   return true;
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
                << " ERROR: FOM Object Attribute '" << get_FOM_name()
                << "' with Trick name '" << get_trick_name()
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
                << " ERROR: FOM Object Attribute '" << get_FOM_name()
                << "' with Trick name '" << get_trick_name()
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
                << " ERROR: FOM Object Attribute '" << get_FOM_name()
                << "' with Trick name '" << get_trick_name()
                << "' has a 'cycle_time' value of " << this->cycle_time
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
             << "  FOM_name:'" << get_FOM_name() << "'\n"
             << "  trick_name:'" << get_trick_name() << "'\n"
             << "  core_job_cycle_time:" << core_job_cycle_time << " seconds\n"
             << "  cyle_time:" << this->cycle_time << " seconds\n"
             << "  cycle_ratio:" << this->cycle_ratio << '\n';
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }
}
