/*!
@file SpaceFOM/MTRInteractionHandler.cpp
@ingroup SpaceFOM
@brief This class handles the HLA interactions for Space Reference FOM (SpaceFOM)
Mode Transition Request (MTR) interaction.

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
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../TrickHLA/Federate.cpp}
@trick_link_dependency{../TrickHLA/Int64BaseTime.cpp}
@trick_link_dependency{../TrickHLA/InteractionHandler.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{ExecutionControl.cpp}
@trick_link_dependency{MTRInteractionHandler.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/InteractionHandler.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/ExecutionControl.hh"
#include "SpaceFOM/MTRInteractionHandler.hh"
#include "SpaceFOM/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace std;
using namespace SpaceFOM;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
MTRInteractionHandler::MTRInteractionHandler(
   Federate const *fed )
   : name( NULL ),
     mtr_mode( MTR_UNINITIALIZED ),
     mtr_mode_int( 0 ),
     scenario_time( 0.0 ),
     sim_time( 0.0 ),
     cte_time( 0.0 ),
     granted_time( 0.0 ),
     send_cnt( 0 ),
     receive_cnt( 0 )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
MTRInteractionHandler::~MTRInteractionHandler() // RETURN: -- None.
{
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::MTRInteractionHandler::~MTRInteractionHandler():%d WARNING failed to delete Trick Memory for 'this->name'\n",
                          __LINE__ );
      }
      this->name = NULL;
   }
   return;
}

/*!
 * @job_class{default_data}
 */
void MTRInteractionHandler::set_name(
   char const *new_name )
{
   if ( this->name != NULL ) {
      if ( trick_MM->is_alloced( this->name ) ) {
         if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
            message_publish( MSG_WARNING, "SpaceFOM::MTRInteractionHandler::set_name():%d WARNING failed to delete Trick Memory for 'this->name'\n",
                             __LINE__ );
         }
      }
      this->name = NULL;
   }
   this->name = trick_MM->mm_strdup( new_name );
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Timestamp Order or Receive Order is determined at compile time.
 */
void MTRInteractionHandler::send_interaction(
   MTREnum mode_request )
{
   // Make sure that the interaction reference has been set.
   if ( this->interaction == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::MTRInteractionHandler::send_interaction():" << __LINE__
             << " ERROR: Unexpected NULL Interaction!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the ExecutionControl object and cast it to an SpaceFOM::ExecutionControl.
   ExecutionControlBase const *exco_base = interaction->get_federate()->get_execution_control();

   // Set the requested mode.
   mtr_mode     = mode_request;
   mtr_mode_int = mtr_enum_to_int16( mode_request );

   // Create a User Supplied Tag based off the name in this example.
   RTI1516_USERDATA rti_user_supplied_tag;
   if ( name != NULL ) {
      rti_user_supplied_tag = RTI1516_USERDATA( name, strlen( name ) );
   } else {
      rti_user_supplied_tag = RTI1516_USERDATA( 0, 0 );
   }

   // Get the current time line values.
   scenario_time = get_scenario_time();
   sim_time      = get_sim_time();
   if ( exco_base->does_cte_timeline_exist() ) {
      cte_time = get_cte_time();
   }
   granted_time = interaction->get_federate()->get_granted_time().get_time_in_seconds();

   // Notify the parent interaction handler to send the interaction using
   // Receive Order (RO).
   bool was_sent = InteractionHandler::send_interaction( rti_user_supplied_tag );

   if ( was_sent ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {

         string rti_user_supplied_tag_string;
         StringUtilities::to_string( rti_user_supplied_tag_string, rti_user_supplied_tag );
         ostringstream msg;

         msg << "++++SENDING++++ MTRInteractionHandler::send_interaction("
             << "Receive Order):" << __LINE__ << '\n'
             << "  name: '" << ( ( name != NULL ) ? name : "NULL" ) << "'\n"
             << "  user-supplied-tag: '" << rti_user_supplied_tag_string << "'\n"
             << "  user-supplied-tag-size: " << rti_user_supplied_tag.size() << '\n'
             << "  mode request: " << mtr_enum_to_string( mtr_mode ) << '\n'
             << "  Scenario time: " << scenario_time << '\n'
             << "  Simulation time: " << sim_time << '\n';
         if ( exco_base->does_cte_timeline_exist() ) {
            msg << "  CTE time: " << cte_time << '\n';
         }
         msg << "  HLA grant time: " << granted_time << " ("
             << Int64BaseTime::to_base_time( granted_time ) << " "
             << Int64BaseTime::get_units() << ")\n"
             << "  send_cnt:" << ( send_cnt + 1 ) << '\n';
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      // Update the send count, which is just used for the message in this example.
      ++send_cnt;
   } else {
      // Use the inherited debug-handler to allow debug comments to be turned
      // on and off from a setting in the input.py file. Use a higher debug level.
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {

         // Get the current time line values.
         scenario_time = get_scenario_time();
         sim_time      = get_sim_time();
         if ( exco_base->does_cte_timeline_exist() ) {
            cte_time = get_cte_time();
         }
         granted_time = interaction->get_federate()->get_granted_time().get_time_in_seconds();

         // The interaction was Not sent.
         ostringstream msg;
         msg << "+-+-NOT SENT-+-+ MTRInteractionHandler::send_sine_interaction():"
             << __LINE__ << '\n'
             << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'\n"
             << "  Scenario time: " << scenario_time << '\n'
             << "  Simulation time: " << sim_time << '\n';
         if ( exco_base->does_cte_timeline_exist() ) {
            msg << "  CTE time: " << cte_time << '\n';
         }
         msg << "  HLA grant time: " << granted_time << " ("
             << Int64BaseTime::to_base_time( granted_time ) << " "
             << Int64BaseTime::get_units() << ")\n";
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }
}

void MTRInteractionHandler::receive_interaction(
   RTI1516_USERDATA const &the_user_supplied_tag )
{
   // Make sure that the federate reference has been set.
   if ( this->interaction == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::MTRInteractionHandler::receive_interaction():" << __LINE__
             << " ERROR: Unexpected NULL Interaction!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the ExecutionControl object and cast it to an SpaceFOM::ExecutionControl.
   SpaceFOM::ExecutionControl *exco = dynamic_cast< ExecutionControl * >( interaction->get_federate()->get_execution_control() );
   if ( exco == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::MTRInteractionHandler::receive_interaction():" << __LINE__
             << "  ERROR: Unexpected NULL SpaceFOM::ExecutionControl!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      exit( 1 );
   }

   // Set MTR.
   this->mtr_mode = mtr_int16_to_enum( this->mtr_mode_int );

   // Check to see if this is a valid Mode Change Request (MTR)?
   if ( exco->is_mtr_valid( this->mtr_mode ) ) {

      // Set the Mode Manger mode flags and mode request value.
      exco->set_mode_transition_requested();
      exco->set_pending_mtr( this->mtr_mode );
   }

   // Convert the HLA User Supplied Tag back into a string we can use.
   string user_tag_string;
   StringUtilities::to_string( user_tag_string, the_user_supplied_tag );

   // Get the current time line values.
   this->scenario_time = get_scenario_time();
   this->sim_time      = get_sim_time();
   if ( exco->does_cte_timeline_exist() ) {
      this->cte_time = get_cte_time();
   }
   this->granted_time = interaction->get_federate()->get_granted_time().get_time_in_seconds();

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input.py file.
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {

      string user_supplied_tag_string;
      StringUtilities::to_string( user_supplied_tag_string, the_user_supplied_tag );

      ostringstream msg;
      msg << "++++RECEIVING++++ SpaceFOM::MTRInteractionHandler::receive_interaction():"
          << __LINE__ << '\n'
          << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'\n"
          << "  user-supplied-tag: '" << user_supplied_tag_string << "'\n"
          << "  user-supplied-tag-size: " << the_user_supplied_tag.size() << '\n'
          << "  mode request: " << mtr_enum_to_string( this->mtr_mode ) << '\n'
          << "  Scenario time: " << this->scenario_time << '\n'
          << "  Simulation time: " << this->sim_time << '\n';
      if ( exco->does_cte_timeline_exist() ) {
         msg << "  CTE time: " << this->cte_time << '\n';
      }
      msg << "  HLA grant time: " << this->granted_time << " ("
          << Int64BaseTime::to_base_time( this->granted_time ) << " "
          << Int64BaseTime::get_units() << ")\n"
          << "  receive_cnt:" << ( receive_cnt + 1 ) << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   ++receive_cnt;
}
