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
@trick_link_dependency{MTRInteractionHandler.cpp}
@trick_link_dependency{ExecutionControl.cpp}
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

using namespace std;
using namespace SpaceFOM;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
MTRInteractionHandler::MTRInteractionHandler(
   Federate *fed )
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
   if ( this->name != (char *)NULL ) {
      trick_MM->delete_var( (void *)this->name );
      this->name = (char *)NULL;
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
         trick_MM->delete_var( (void *)this->name );
      }
      this->name = NULL;
   }
   this->name = trick_MM->mm_strdup( (char *)new_name );
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
             << " ERROR: Unexpected NULL Interaction!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the ExecutionControl object and cast it to an SpaceFOM::ExecutionControl.
   ExecutionControlBase const *exco_base = interaction->get_federate()->get_execution_control();

   // Set the requested mode.
   mtr_mode     = mode_request;
   mtr_mode_int = mtr_enum_to_int16( mode_request );

   // Create a User Supplied Tag based off the name in this example.
   RTI1516_USERDATA user_supplied_tag;
   if ( name != NULL ) {
      user_supplied_tag = RTI1516_USERDATA( name, strlen( name ) );
   } else {
      user_supplied_tag = RTI1516_USERDATA( 0, 0 );
   }

   // Get the current time line values.
   scenario_time = this->get_scenario_time();
   sim_time      = this->get_sim_time();
   if ( exco_base->does_cte_timeline_exist() ) {
      cte_time = this->get_cte_time();
   }
   granted_time = this->interaction->get_federate()->get_granted_time().get_time_in_seconds();

   // Notify the parent interaction handler to send the interaction using
   // Receive Order (RO).
   bool was_sent = this->InteractionHandler::send_interaction( user_supplied_tag );

   if ( was_sent ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {

         string user_supplied_tag_string;
         StringUtilities::to_string( user_supplied_tag_string, user_supplied_tag );

         cout << "++++SENDING++++ MTRInteractionHandler::send_interaction("
              << "Receive Order):" << __LINE__ << endl
              << "  name: '" << ( ( name != NULL ) ? name : "NULL" ) << "'" << endl
              << "  user-supplied-tag: '" << user_supplied_tag_string << "'" << endl
              << "  user-supplied-tag-size: " << user_supplied_tag.size() << endl
              << "  mode request: " << mtr_enum_to_string( mtr_mode ) << endl
              << "  Scenario time: " << scenario_time << endl
              << "  Simulation time: " << sim_time << endl;
         if ( exco_base->does_cte_timeline_exist() ) {
            cout << "  CTE time: " << cte_time << endl;
         }
         cout << "  HLA grant time: " << granted_time << " ("
              << Int64BaseTime::to_base_time( granted_time ) << " "
              << Int64BaseTime::get_units() << ")" << endl
              << "  send_cnt:" << ( send_cnt + 1 ) << endl;
      }

      // Update the send count, which is just used for the message in this example.
      send_cnt++;
   } else {
      // Use the inherited debug-handler to allow debug comments to be turned
      // on and off from a setting in the input.py file. Use a higher debug level.
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {

         // Get the current time line values.
         scenario_time = this->get_scenario_time();
         sim_time      = this->get_sim_time();
         if ( exco_base->does_cte_timeline_exist() ) {
            cte_time = this->get_cte_time();
         }
         granted_time = interaction->get_federate()->get_granted_time().get_time_in_seconds();

         // The interaction was Not sent.
         cout << "+-+-NOT SENT-+-+ MTRInteractionHandler::send_sine_interaction():"
              << __LINE__ << endl
              << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'" << endl
              << "  Scenario time: " << scenario_time << endl
              << "  Simulation time: " << sim_time << endl;
         if ( exco_base->does_cte_timeline_exist() ) {
            cout << "  CTE time: " << cte_time << endl;
         }
         cout << "  HLA grant time: " << granted_time << " ("
              << Int64BaseTime::to_base_time( granted_time ) << " "
              << Int64BaseTime::get_units() << ")" << endl;
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
             << " ERROR: Unexpected NULL Interaction!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the ExecutionControl object and cast it to an SpaceFOM::ExecutionControl.
   SpaceFOM::ExecutionControl *exco = dynamic_cast< ExecutionControl * >( interaction->get_federate()->get_execution_control() );
   if ( exco == static_cast< ExecutionControl * >( NULL ) ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::MTRInteractionHandler::receive_interaction():" << __LINE__
             << "  ERROR: Unexpected NULL SpaceFOM::ExecutionControl!" << THLA_ENDL;
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
   this->scenario_time = this->get_scenario_time();
   this->sim_time      = this->get_sim_time();
   if ( exco->does_cte_timeline_exist() ) {
      this->cte_time = this->get_cte_time();
   }
   this->granted_time = interaction->get_federate()->get_granted_time().get_time_in_seconds();

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input.py file.
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {

      string user_supplied_tag_string;
      StringUtilities::to_string( user_supplied_tag_string, the_user_supplied_tag );
      cout << "++++RECEIVING++++ SpaceFOM::MTRInteractionHandler::receive_interaction():"
           << __LINE__ << endl
           << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'" << endl
           << "  user-supplied-tag: '" << user_supplied_tag_string << "'" << endl
           << "  user-supplied-tag-size: " << the_user_supplied_tag.size() << endl
           << "  mode request: " << mtr_enum_to_string( this->mtr_mode ) << endl
           << "  Scenario time: " << this->scenario_time << endl
           << "  Simulation time: " << this->sim_time << endl;
      if ( exco->does_cte_timeline_exist() ) {
         cout << "  CTE time: " << this->cte_time << endl;
      }
      cout << "  HLA grant time: " << this->granted_time << " ("
           << Int64BaseTime::to_base_time( this->granted_time ) << " "
           << Int64BaseTime::get_units() << ")" << endl
           << "  receive_cnt:" << ( receive_cnt + 1 ) << endl;
   }

   receive_cnt++;
}
