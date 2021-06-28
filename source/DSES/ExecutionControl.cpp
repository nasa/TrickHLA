/*!
@file DSES/ExecutionControl.cpp
@ingroup DSES
@brief This class provides and abstract base class as the base implementation
for managing mode transitions.

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
@trick_link_dependency{../TrickHLA/SyncPntListBase.cpp}
@trick_link_dependency{../TrickHLA/SleepTimeout.cpp}
@trick_link_dependency{ExecutionControl.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, DSES support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iomanip>
#include <math.h>

// Trick includes.
#include "trick/Executive.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

// DSES include files.
#include "DSES/ExecutionConfiguration.hh"
#include "DSES/ExecutionControl.hh"

// IMSim file level declarations.
namespace DSES
{

// ExecutionControl type string.
const std::string ExecutionControl::type = "DSES";

// The DSES Multiphase initialization HLA synchronization-points.
static const std::wstring SIM_CONFIG_SYNC_POINT = L"sim_config";
static const std::wstring INITIALIZE_SYNC_POINT = L"initialize";
static const std::wstring STARTUP_SYNC_POINT    = L"startup";

// SISO SpaceFOM Mode Transition Request (MTR) synchronization-points.
static const std::wstring MTR_RUN_SYNC_POINT      = L"mtr_run";
static const std::wstring MTR_FREEZE_SYNC_POINT   = L"mtr_freeze";
static const std::wstring MTR_SHUTDOWN_SYNC_POINT = L"mtr_shutdown";

} // namespace DSES

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;
using namespace DSES;

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl()
   : time_padding( 5.0 ),
     mode_transition_requested( false ),
     pending_mtr( DSES::MTR_UNINITIALIZED ),
     requested_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     current_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     next_mode_scenario_time( -std::numeric_limits< double >::max() ),
     next_mode_cte_time( -std::numeric_limits< double >::max() ),
     simulation_freeze_time( 0.0 ),
     scenario_freeze_time( 0.0 ),
     late_joiner( false ),
     late_joiner_determined( false ),
     federate( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ExecutionControl::~ExecutionControl()
{
   clear_mode_values();
}

/*!
@details This routine will set a lot of the data in the TrickHLA::Federate that
is required for this execution control scheme. This should greatly simplify
input files and reduce input file setting errors.

@job_class{initialization}
*/
void ExecutionControl::initialize()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg;
      msg << "DSES::ExecutionControl::initialize():" << __LINE__
          << " Initialization-Scheme:'" << get_type()
          << "'" << THLA_ENDL;
      send_hs( stdout, (char *)msg.str().c_str() );
   }

   // Set the reference to the TrickHLA::Federate.
   this->federate = &fed;

   // Initialize the ExecutionControl so that it knows about the manager.
   this->execution_configuration->initialize( this->get_manager() );

   // There are things that must me set for the DSES initialization.
   this->use_preset_master = true;

   // If this is the Master federate, then it must support Time
   // Management and be both Time Regulating and Time Constrained.
   if ( this->is_master() ) {
      federate->time_management  = true;
      federate->time_regulating  = true;
      federate->time_constrained = true;

      // The software frame is set from the ExCO Least Common Time Step.
      // For the Master federate the Trick simulation software frame must
      // match the Least Common Time Step (LCTS).
      double software_frame_time = Int64Interval::to_seconds( least_common_time_step );
      exec_set_software_frame( software_frame_time );
   }

   // Add the Mode Transition Request synchronization points.
   this->add_sync_point( MTR_RUN_SYNC_POINT );
   this->add_sync_point( MTR_FREEZE_SYNC_POINT );
   this->add_sync_point( MTR_SHUTDOWN_SYNC_POINT );

   // Make sure we initialize the base class.
   TrickHLA::ExecutionControlBase::initialize();

   // Must use a preset master.
   if ( !this->is_master_preset() ) {
      ostringstream errmsg;
      errmsg << "DSES::ExecutionControl::initialize(): WARNING:" << __LINE__
             << " Only a preset master is supported. Make sure to set"
             << " 'THLA.federate.use_preset_master = true' in your input file."
             << " Setting use_preset_master to true!"
             << THLA_ENDL;
      send_hs( stdout, (char *)errmsg.str().c_str() );
      this->use_preset_master = true;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      if ( this->is_master() ) {
         send_hs( stdout, "DSES::ExecutionControl::initialize():%d\n    I AM THE PRESET MASTER%c",
                  __LINE__, THLA_NEWLINE );
      } else {
         send_hs( stdout, "DSES::ExecutionControl::initialize():%d\n    I AM NOT THE PRESET MASTER%c",
                  __LINE__, THLA_NEWLINE );
      }
   }

   // Call the DSES ExecutionControl pre-multi-phase initialization processes.
   pre_multi_phase_init_processes();
}

/*!
@details This routine implements the DSES Join Federation Process described
in section 7.2 and figure 7-3.

@job_class{initialization}
*/
void ExecutionControl::join_federation_process()
{
   // The base class implementation is good enough for now.
   TrickHLA::ExecutionControlBase::join_federation_process();
}

/*!
@details This routine implements the DSES pre multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::pre_multi_phase_init_processes()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      send_hs( stdout, "DSES::ExecutionControl::pre_multi_phase_init_processes():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Reset the sim-config required flag to make it required.
   execution_configuration->mark_required();

   // Reset the sim-config preferred-order for attributes to Receive-Order.
   execution_configuration->reset_preferred_order();

   // Reset the ownership flags and the attribute configuration flags for
   // the simulation configuration object.
   execution_configuration->reset_ownership_states();

   // Setup all the Trick Ref-Attributes for the user specified objects,
   // attributes, interactions and parameters.
   get_manager()->setup_all_ref_attributes();

   // Add the DSES multiphase initialization sync-points now that the
   // Simulation-Configuration has been initialized in the call to
   // the setup_all_ref_attributes() function. We do this here so
   // that we can handle the RTI callbacks that use them.
   this->add_multiphase_init_sync_points();

   // Create the RTI Ambassador and connect.
   federate->create_RTI_ambassador_and_connect();

   // Destroy the federation if it was orphaned from a previous simulation
   // run that did not shutdown cleanly.
   federate->destroy_orphaned_federation();

   // Create and then join the federation.
   federate->create_and_join_federation();

   // Don't forget to enable asynchronous delivery of messages.
   federate->enable_async_delivery();

   // Determine if this federate is the Master.
   determine_federation_master();

   // Setup all the RTI handles for the objects, attributes and interaction
   // parameters.
   get_manager()->setup_all_RTI_handles();

   // Call publish_and_subscribe AFTER we've initialized the manager,
   // federate, and FedAmb.
   get_manager()->publish_and_subscribe();

   // Reserve the RTI object instance names with the RTI, but only for
   // the objects that are locally owned.
   get_manager()->reserve_object_names_with_RTI();

   // Waits on the reservation of the RTI object instance names for the
   // locally owned objects. Calling this function will block until all
   // the object instances names for the locally owned objects have been
   // reserved.
   get_manager()->wait_for_reservation_of_object_names();

   // Creates an RTI object instance and registers it with the RTI, but
   // only for the objects that are locally owned.
   get_manager()->register_objects_with_RTI();

   // Setup the preferred order for all object attributes and interactions.
   get_manager()->setup_preferred_order_with_RTI();

   // Waits on the registration of all the required RTI object instances with
   // the RTI. Calling this function will block until all the required object
   // instances in the Federation have been registered.
   get_manager()->wait_for_registration_of_required_objects();

   // Initialize the MOM interface handles.
   federate->initialize_MOM_handles();

   // Perform the next few steps if we are the Master federate.
   if ( this->is_master() ) {
      // Make sure all required federates have joined the federation.
      (void)federate->wait_for_required_federates_to_join();

      // Register the Multi-phase initialization sync-points.
      this->register_all_sync_points( *federate->get_RTI_ambassador() );
   }

   // Wait for the "sim_config", "initialize", and "startup" sync-points
   // to be registered.
   this->wait_for_all_announcements( federate );

   // Achieve the "sim_config" sync-point and wait for the federation
   // to be synchronized on it.
   federate->achieve_and_wait_for_synchronization( DSES::SIM_CONFIG_SYNC_POINT );

   if ( this->is_master() ) {
      // Send the "Simulation Configuration".
      this->send_execution_configuration();
   } else {
      // Wait for the "Simulation Configuration" object attribute reflection.
      this->receive_execution_configuration();
   }

   // Achieve the "initialize" sync-point and wait for the federation
   // to be synchronized on it.
   federate->achieve_and_wait_for_synchronization( DSES::INITIALIZE_SYNC_POINT );
}

/*!
@details This routine implements the DSES post multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::post_multi_phase_init_process()
{
   // Setup HLA time management.
   federate->setup_time_management();

   // Achieve the "startup" sync-point and wait for the federation
   // to be synchronized on it.
   federate->achieve_and_wait_for_synchronization( DSES::STARTUP_SYNC_POINT );
}

/*!
 * @job_class{shutdown}
 */
void ExecutionControl::shutdown()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::setup_object_RTI_handles()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::setup_interaction_RTI_handles()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::add_multiphase_init_sync_points()
{
   // Call the base class method to add user defined multi-phase
   // initialization synchronization points.
   ExecutionControlBase::add_multiphase_init_sync_points();

   // Register initialization synchronization points used for startup regulation.
   this->add_sync_point( DSES::STARTUP_SYNC_POINT );
   this->add_sync_point( DSES::INITIALIZE_SYNC_POINT );
   this->add_sync_point( DSES::SIM_CONFIG_SYNC_POINT );
}

void ExecutionControl::announce_sync_point(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   wstring const &                   label,
   RTI1516_USERDATA const &          user_supplied_tag )
{
   if ( this->contains( label ) ) {
      // Mark init sync-point as announced.
      if ( this->mark_announced( label ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            send_hs( stdout, "DSES::ExecutionControl::announce_sync_point():%d DSES Multiphase Init Sync-Point:'%ls'%c",
                     __LINE__, label.c_str(), THLA_NEWLINE );
         }
      }

   } // By default, mark an unrecognized synchronization point as achieved.
   else {

      if ( DebugHandler::DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "DSES::ExecutionControl::announce_sync_point():%d Unrecognized synchronization point:'%ls', which will be achieved.%c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
      }

      // Unknown synchronization point so achieve it but don't wait for the
      // federation to by synchronized on it.
      this->achieve_sync_point( rti_ambassador, label );
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::achieve_all_multiphase_init_sync_points(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador )
{
   // Iterate through this ExecutionControl's synchronization point list.
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      // Achieve a synchronization point if it is not already achieved and is
      // not one of the predefined ExecutionControl synchronization points.
      if ( ( sp != NULL ) && sp->exists() && !sp->is_achieved()
           && ( sp->label.compare( DSES::STARTUP_SYNC_POINT ) != 0 )
           && ( sp->label.compare( DSES::INITIALIZE_SYNC_POINT ) != 0 )
           && ( sp->label.compare( DSES::SIM_CONFIG_SYNC_POINT ) != 0 ) ) {
         // Achieve the synchronization point.
         rti_ambassador.synchronizationPointAchieved( sp->label );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::wait_for_all_multiphase_init_sync_points()
{
   // Iterate through this ExecutionControl's synchronization point list.
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      // Wait for a synchronization point if it is not already achieved and is
      // not one of the predefined ExecutionControl synchronization points.
      if ( ( sp != NULL ) && sp->exists() && !sp->is_achieved()
           && ( sp->label.compare( DSES::STARTUP_SYNC_POINT ) != 0 )
           && ( sp->label.compare( DSES::INITIALIZE_SYNC_POINT ) != 0 )
           && ( sp->label.compare( DSES::SIM_CONFIG_SYNC_POINT ) != 0 ) ) {

         long long    wallclock_time;
         SleepTimeout print_timer( federate->wait_status_time );
         SleepTimeout sleep_timer;

         // Wait for the federation to synchronized on the sync-point.
         while ( !sp->is_achieved() ) {

            // Always check to see is a shutdown was received.
            federate->check_for_shutdown_with_termination();

            // Pause and release the processor for short sleep value.
            (void)sleep_timer.sleep();

            // Periodically check to make sure the federate is still part of
            // the federation exectuion.
            if ( !sp->is_achieved() ) {

               // To be more efficient, we get the time once and share it.
               wallclock_time = sleep_timer.time();

               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "DSES::ExecutionControl::wait_for_all_multiphase_init_sync_points:" << __LINE__
                            << " ERROR: Unexpectedly the Federate is no longer an execution"
                            << " member. This means we are either not connected to the"
                            << " RTI or we are no longer joined to the federation"
                            << " execution because someone forced our resignation at"
                            << " the Central RTI Component (CRC) level!"
                            << THLA_ENDL;
                     DebugHandler::terminate_with_message( errmsg.str() );
                  }
               }

               if ( print_timer.timeout( wallclock_time ) ) {
                  print_timer.reset();
                  send_hs( stdout, "DSES::ExecutionControl::wait_for_all_multiphase_init_sync_points():%d Waiting...%c",
                           __LINE__, THLA_NEWLINE );
               }
            }
         }
      }
   }
}

void ExecutionControl::publish()
{
   return;
}

void ExecutionControl::unpublish()
{
   return;
}

void ExecutionControl::subscribe()
{
   return;
}

void ExecutionControl::unsubscribe()
{

   return;
}

/*!
@job_class{initialization}
*/
bool ExecutionControlBase::object_instance_name_reservation_failed(
   std::wstring const &obj_instance_name )
{
   wstring ws_sim_config_name;
   StringUtilities::to_wstring( ws_sim_config_name, execution_configuration->get_name() );

   // For multiphase initialization version 1, we handle the SimConfig
   // instance name reservation failure to help determine the master.
   if ( obj_instance_name == ws_sim_config_name ) {

      // We are NOT the Master federate since we failed to reserve the
      // "SimConfig" name and only if the master was not preset.
      if ( !this->is_master_preset() ) {
         this->set_master( false );
      }

      // We failed to register the "SimConfig" name which means that another
      // federate has already registered it.
      execution_configuration->set_name_registered();

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "DSES::ExecutionControl::object_instance_name_reservation_failed():%d Name:'%ls'%c",
                  __LINE__, obj_instance_name.c_str(), THLA_NEWLINE );
      }

      // Return 'true' since we found a match.
      return ( true );
   }

   // Return 'false' since we did not find a match.
   return ( false );
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::determine_federation_master()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      send_hs( stdout, "DSES::ExecutionControl::determine_federation_master():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // If we are not supposed to use a preset master, then reserve the SimConfig
   // name so that we can determine the master. Also, if we are supposed to use
   // a preset master and we are that master then go ahead and register the
   // SimConfig name.
   if ( !this->is_master_preset() || this->is_master() ) {

      // Reserve "SimConfig" object instance name as a way of determining
      // who the master federate is.
      execution_configuration->reserve_object_name_with_RTI();

      // Wait for success or failure for the "SimConfig" name reservation.
      execution_configuration->wait_for_object_name_reservation();
   }

   // Setup the execution configuration object now that we know if we are the
   // "Master" federate or not.
   execution_configuration->set_master( this->is_master() );

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      if ( this->is_master() ) {
         send_hs( stdout, "DSES::ExecutionControl::determine_federation_master():%d\n    I AM THE MASTER%c",
                  __LINE__, THLA_NEWLINE );
      } else {
         send_hs( stdout, "DSES::ExecutionControl::determine_federation_master():%d\n    I AM NOT THE MASTER%c",
                  __LINE__, THLA_NEWLINE );
      }
   }
}

bool ExecutionControl::set_pending_mtr(
   MTREnum mtr_value )
{
   if ( this->is_mtr_valid( mtr_value ) ) {
      this->pending_mtr = mtr_value;
   }
   return false;
}

bool ExecutionControl::is_mtr_valid(
   MTREnum mtr_value )
{
   ExecutionConfiguration *ExCO = get_execution_configuration();

   switch ( mtr_value ) {
      case MTR_GOTO_RUN: {
         return ( ( ExCO->current_execution_mode == EXECUTION_MODE_INITIALIZING ) || ( ExCO->current_execution_mode == EXECUTION_MODE_FREEZE ) );
      }
      case MTR_GOTO_FREEZE: {
         return ( ( ExCO->current_execution_mode == EXECUTION_MODE_INITIALIZING ) || ( ExCO->current_execution_mode == EXECUTION_MODE_RUNNING ) );
      }
      case MTR_GOTO_SHUTDOWN: {
         return ( ExCO->current_execution_mode != EXECUTION_MODE_SHUTDOWN );
      }
      default: {
         return false;
      }
   }
   return false;
}

void ExecutionControl::set_mode_request_from_mtr(
   MTREnum mtr_value )
{
   switch ( mtr_value ) {
      case MTR_UNINITIALIZED:
         this->pending_mtr = MTR_UNINITIALIZED;
         set_next_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED );
         break;

      case MTR_INITIALIZING:
         this->pending_mtr = MTR_INITIALIZING;
         set_next_execution_control_mode( EXECUTION_CONTROL_INITIALIZING );
         break;

      case MTR_GOTO_RUN:
         this->pending_mtr = MTR_GOTO_RUN;
         set_next_execution_control_mode( EXECUTION_CONTROL_RUNNING );
         break;

      case MTR_GOTO_FREEZE:
         this->pending_mtr = MTR_GOTO_FREEZE;
         set_next_execution_control_mode( EXECUTION_CONTROL_FREEZE );
         break;

      case MTR_GOTO_SHUTDOWN:
         this->pending_mtr = MTR_GOTO_SHUTDOWN;
         set_next_execution_control_mode( EXECUTION_CONTROL_SHUTDOWN );
         break;

      default:
         this->pending_mtr = MTR_UNINITIALIZED;
         break;
   }
}

void ExecutionControl::set_next_execution_control_mode(
   TrickHLA::ExecutionControlEnum exec_control )
{
   // Reference the DSES Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // This should only be called by the Master federate.
   if ( !this->is_master() ) {
      ostringstream errmsg;
      errmsg << "DSES::ExecutionControl::set_next_execution_mode():" << __LINE__
             << " ERROR: This should only be called by the Master federate!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   switch ( exec_control ) {
      case EXECUTION_CONTROL_UNINITIALIZED:

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_UNINITIALIZED;
         ExCO->set_next_execution_mode( EXECUTION_MODE_UNINITIALIZED );

         // Set the next mode times.
         this->next_mode_scenario_time = this->get_scenario_time();      // Immediate
         ExCO->set_next_mode_scenario_time( this->get_scenario_time() ); // Immediate
         ExCO->set_next_mode_cte_time( this->get_cte_time() );           // Immediate

         break;

      case EXECUTION_CONTROL_INITIALIZING:

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_INITIALIZING;
         ExCO->set_next_execution_mode( EXECUTION_MODE_INITIALIZING );

         // Set the next mode times.
         ExCO->set_scenario_time_epoch( this->get_scenario_time() );     // Now.
         this->next_mode_scenario_time = this->get_scenario_time();      // Immediate
         ExCO->set_next_mode_scenario_time( this->get_scenario_time() ); // Immediate
         ExCO->set_next_mode_cte_time( this->get_cte_time() );           // Immediate

         break;

      case EXECUTION_CONTROL_RUNNING:

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_RUNNING;
         ExCO->set_next_execution_mode( EXECUTION_MODE_RUNNING );

         // Set the next mode times.
         this->next_mode_scenario_time = this->get_scenario_time();          // Immediate
         ExCO->set_next_mode_scenario_time( this->next_mode_scenario_time ); // immediate
         ExCO->set_next_mode_cte_time( this->get_cte_time() );
         if ( ExCO->get_next_mode_cte_time() > -std::numeric_limits< double >::max() ) {
            ExCO->set_next_mode_cte_time( ExCO->get_next_mode_cte_time() + this->time_padding ); // Some time in the future.
         }

         break;

      case EXECUTION_CONTROL_FREEZE:

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_FREEZE;
         ExCO->set_next_execution_mode( EXECUTION_MODE_FREEZE );

         // Set the next mode times.
         this->next_mode_scenario_time = this->get_scenario_time() + this->time_padding; // Some time in the future.
         ExCO->set_next_mode_scenario_time( this->next_mode_scenario_time );
         ExCO->set_next_mode_cte_time( this->get_cte_time() );
         if ( ExCO->get_next_mode_cte_time() > -std::numeric_limits< double >::max() ) {
            ExCO->set_next_mode_cte_time( ExCO->get_next_mode_cte_time() + this->time_padding ); // Some time in the future.
         }

         // Set the ExecutionControl freeze times.
         this->scenario_freeze_time   = this->next_mode_scenario_time;
         this->simulation_freeze_time = this->scenario_timeline->compute_simulation_time( this->next_mode_scenario_time );

         break;

      case EXECUTION_CONTROL_SHUTDOWN:

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
         ExCO->set_next_execution_mode( EXECUTION_MODE_SHUTDOWN );

         // Set the next mode times.
         this->next_mode_scenario_time = this->get_scenario_time();          // Immediate.
         ExCO->set_next_mode_scenario_time( this->next_mode_scenario_time ); // Immediate.
         ExCO->set_next_mode_cte_time( this->get_cte_time() );               // Immediate

         break;

      default:
         this->requested_execution_control_mode = EXECUTION_CONTROL_UNINITIALIZED;
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            ostringstream errmsg;
            errmsg << "DSES::ExecutionControl::set_next_execution_mode():"
                   << __LINE__ << " WARNING: Unknown execution mode value: " << exec_control
                   << THLA_ENDL;
            send_hs( stdout, (char *)errmsg.str().c_str() );
         }
         break;
   }
}

bool ExecutionControl::check_mode_transition_request()
{
   // Just return if false mode change has been requested.
   if ( !this->is_mode_transition_requested() ) {
      return false;
   }

   // Only the Master federate receives and processes Mode Transition Requests.
   if ( !this->is_master() ) {
      ostringstream errmsg;
      errmsg << "DSES::ExecutionControl::check_mode_transition_request():"
             << __LINE__ << " WARNING: Received Mode Transition Request and not Master: "
             << mtr_enum_to_string( this->pending_mtr )
             << THLA_ENDL;
      send_hs( stdout, (char *)errmsg.str().c_str() );
      return false;
   }

   // First check to see if this is a valid MTR.
   if ( !( is_mtr_valid( this->pending_mtr ) ) ) {
      ostringstream errmsg;
      errmsg << "DSES::ExecutionControl::check_mode_transition_request():"
             << __LINE__ << " WARNING: Invalid Mode Transition Request: "
             << mtr_enum_to_string( this->pending_mtr )
             << THLA_ENDL;
      send_hs( stdout, (char *)errmsg.str().c_str() );
      return false;
   }

   return true;
}

bool ExecutionControl::process_mode_transition_request()
{
   // Just return is no mode change has been requested.
   if ( !this->check_mode_transition_request() ) {
      return false;
   } else {
      // Since this is a valid MTR, set the next mode from the MTR.
      this->set_mode_request_from_mtr( this->pending_mtr );
   }

   // Reference the DSES Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = this->get_execution_configuration();

   // Print diagnostic message if appropriate.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      cout << "=============================================================" << endl
           << "ExecutionControl::process_mode_transition_request():" << __LINE__ << endl
           << "\t current_scenario_time:     " << setprecision( 18 ) << this->scenario_timeline->get_time() << endl
           << "\t scenario_time_epoch:       " << setprecision( 18 ) << this->scenario_timeline->get_epoch() << endl
           << "\t scenario_time_epoch(ExCO): " << setprecision( 18 ) << ExCO->scenario_time_epoch << endl
           << "\t scenario_time_sim_offset:  " << setprecision( 18 ) << this->scenario_timeline->get_sim_offset() << endl
           << "\t Current HLA grant time:    " << federate->get_granted_time().get_time_in_seconds() << endl
           << "\t Current HLA request time:  " << federate->get_requested_time().get_time_in_seconds() << endl
           << "\t current_sim_time:          " << setprecision( 18 ) << this->sim_timeline->get_time() << endl
           << "\t simulation_time_epoch:     " << setprecision( 18 ) << this->sim_timeline->get_epoch() << endl;
      if ( this->does_cte_timeline_exist() ) {
         cout << "\t current_CTE_time:          " << setprecision( 18 ) << this->cte_timeline->get_time() << endl
              << "\t CTE_time_epoch:            " << setprecision( 18 ) << this->cte_timeline->get_epoch() << endl;
      }
      cout << "\t next_mode_scenario_time:   " << setprecision( 18 ) << ExCO->next_mode_scenario_time << endl
           << "\t next_mode_cte_time:        " << setprecision( 18 ) << ExCO->next_mode_cte_time << endl
           << "\t scenario_freeze_time:      " << setprecision( 18 ) << this->scenario_freeze_time << endl
           << "\t simulation_freeze_time:    " << setprecision( 18 ) << this->simulation_freeze_time << endl
           << "=============================================================" << endl;
   }

   // Check Mode Transition Request.
   switch ( this->pending_mtr ) {

      case MTR_GOTO_RUN:

         // Clear the mode change request flag.
         this->clear_mode_transition_requested();

         // Transition to run can only happen from initialization or freeze.
         // We don't really need to do anything if we're in initialization.
         if ( this->current_execution_control_mode == EXECUTION_CONTROL_FREEZE ) {

            // Tell Trick to exit freeze and go to run.
            the_exec->run();

            // The run transition logic will be triggered when exiting Freeze.
            // This is done in the TrickHLA::Federate::exit_freeze() routine
            // called when exiting Freeze.
         }

         return true;
         break;

      case MTR_GOTO_FREEZE:

         // Clear the mode change request flag.
         this->clear_mode_transition_requested();

         // Transition to freeze can only happen from initialization or run.
         // We don't really need to do anything if we're in initialization.
         if ( this->current_execution_control_mode == EXECUTION_CONTROL_RUNNING ) {

            // Send out the updated ExCO.
            ExCO->send_init_data();

            // Announce the pending freeze.
            this->freeze_mode_announce();

            // Tell Trick to go into freeze at the appointed time.
            the_exec->freeze( this->simulation_freeze_time );

            // The freeze transition logic will be done just before entering
            // Freeze. This is done in the TrickHLA::Federate::freeze_init()
            // routine called when entering Freeze.
         }

         return true;
         break;

      case MTR_GOTO_SHUTDOWN:

         // Announce the shutdown.
         this->shutdown_mode_announce();

         // Tell Trick to shutdown sometime in the future.
         // The DSES ExecutionControl shutdown transition will be made from
         // the TrickHLA::Federate::shutdown() job.
         the_exec->stop( the_exec->get_sim_time() + this->time_padding );

         return true;
         break;

      default:
         break;
   }

   return false;
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Called from the ExCO unpack routine.
 *
 * @job_class{scheduled}
 */
bool ExecutionControl::process_execution_control_updates()
{
   bool          mode_change = false;
   ostringstream errmsg;

   // Reference the DSES Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // Check if there are pending changes from the ExCO.
   if ( ExCO->update_pending() ) {
      // Clear the ExCO update pending flag and continue.
      ExCO->clear_update_pending();
   } else {
      // There are no pending changes from the ExCO.
      // Return that no mode changes occurred.
      return false;
   }

   // The Master federate should never have to process ExCO updates.
   if ( this->is_master() ) {
      errmsg << "DSES::ExecutionControl::process_execution_control_updates():"
             << __LINE__ << " WARNING: Master receive an ExCO update: "
             << execution_control_enum_to_string( this->requested_execution_control_mode )
             << THLA_ENDL;
      send_hs( stdout, (char *)errmsg.str().c_str() );

      // Return that no mode changes occurred.
      return false;
   }

   // Translate the native ExCO mode values into ExecutionModeEnum.
   ExecutionModeEnum exco_cem = execution_mode_int16_to_enum( ExCO->current_execution_mode );
   ExecutionModeEnum exco_nem = execution_mode_int16_to_enum( ExCO->next_execution_mode );

   // Check for consistency between ExecutionControl and ExCO.
   if ( exco_cem != this->current_execution_control_mode ) {
      errmsg << "DSES::ExecutionControl::process_execution_control_updates():"
             << __LINE__ << " WARNING: Current execution mode mismatch between ExecutionControl ("
             << execution_control_enum_to_string( this->current_execution_control_mode )
             << ") and the ExCO current execution mode ("
             << execution_mode_enum_to_string( exco_cem )
             << ")!"
             << THLA_ENDL;
      send_hs( stdout, (char *)errmsg.str().c_str() );
   }

   // Check for change in execution mode.
   if ( exco_nem != exco_cem ) {
      mode_change = true;
      if ( exco_nem == EXECUTION_MODE_SHUTDOWN ) {
         this->requested_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
      } else if ( exco_nem == EXECUTION_MODE_RUNNING ) {
         this->requested_execution_control_mode = EXECUTION_CONTROL_RUNNING;
      } else if ( exco_nem == EXECUTION_MODE_FREEZE ) {
         this->requested_execution_control_mode = EXECUTION_CONTROL_FREEZE;
         this->scenario_freeze_time             = ExCO->next_mode_scenario_time;
         this->simulation_freeze_time           = this->scenario_timeline->compute_simulation_time( this->scenario_freeze_time );
      } else {
         errmsg << "DSES::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Invalid ExCO next execution mode: "
                << execution_mode_enum_to_string( exco_nem ) << "!"
                << THLA_ENDL;
         send_hs( stdout, (char *)errmsg.str().c_str() );

         // Return that no mode changes occurred.
         return false;
      }
   }

   // Check for CTE mode time update.
   if ( ExCO->next_mode_cte_time != this->next_mode_cte_time ) {
      this->next_mode_cte_time = ExCO->next_mode_cte_time;
   }

   // Check for mode changes.
   if ( !mode_change ) {
      // Return that no mode changes occurred.
      return false;
   }

   // Process the mode change.
   switch ( this->current_execution_control_mode ) {

      case EXECUTION_CONTROL_UNINITIALIZED:

         // Check for SHUTDOWN.
         if ( this->requested_execution_control_mode == EXECUTION_CONTROL_SHUTDOWN ) {

            // Mark the current execution mode as SHUTDOWN.
            this->current_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
            ExCO->current_execution_mode         = EXECUTION_MODE_SHUTDOWN;

            // Tell the TrickHLA::Federate to shutdown.
            // The DSES ExecutionControl shutdown transition will be made from
            // the TrickHLA::Federate::shutdown() job.
            the_exec->stop();

         } else {

            errmsg << "DSES::ExecutionControl::process_execution_control_updates():"
                   << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                   << execution_control_enum_to_string( this->current_execution_control_mode )
                   << ") and the requested execution mode ("
                   << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << ")!"
                   << THLA_ENDL;
            send_hs( stdout, (char *)errmsg.str().c_str() );

            // Return that no mode changes occurred.
            return false;
         }

         // Return that a mode change occurred.
         return true;
         break;

      case EXECUTION_CONTROL_INITIALIZING:

         // Check for SHUTDOWN.
         if ( this->requested_execution_control_mode == EXECUTION_CONTROL_SHUTDOWN ) {

            // Mark the current execution mode as SHUTDOWN.
            this->current_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
            ExCO->current_execution_mode         = EXECUTION_MODE_SHUTDOWN;

            // Tell the TrickHLA::Federate to shutdown.
            // The DSES ExecutionControl shutdown transition will be made from
            // the TrickHLA::Federate::shutdown() job.
            the_exec->stop();

         } else if ( this->requested_execution_control_mode == EXECUTION_CONTROL_RUNNING ) {

            // Tell Trick to go to in Run at startup.
            the_exec->set_freeze_command( false );

            // This is an early joining federate in initialization.
            // So, proceed to the run mode transition.
            this->run_mode_transition();

         } else if ( this->requested_execution_control_mode == EXECUTION_CONTROL_FREEZE ) {

            // Announce the pending freeze.
            this->freeze_mode_announce();

            // Tell Trick to go into freeze at startup.
            //the_exec->freeze();

            // Tell Trick to go into freeze at startup.
            the_exec->set_freeze_command( true );

            // The freeze transition logic will be done just before entering
            // Freeze. This is done in the TrickHLA::Federate::freeze_init()
            // routine called when entering Freeze.

         } else if ( this->requested_execution_control_mode == EXECUTION_CONTROL_INITIALIZING ) {

            // There's really nothing to do here.

         } else {

            errmsg << "DSES::ExecutionControl::process_execution_control_updates():"
                   << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                   << execution_control_enum_to_string( this->current_execution_control_mode )
                   << ") and the requested execution mode ("
                   << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << ")!"
                   << THLA_ENDL;
            send_hs( stdout, (char *)errmsg.str().c_str() );

            // Return that no mode changes occurred.
            return false;
         }

         // Return that a mode change occurred.
         return true;
         break;

      case EXECUTION_CONTROL_RUNNING:

         // Check for SHUTDOWN.
         if ( this->requested_execution_control_mode == EXECUTION_CONTROL_SHUTDOWN ) {

            // Print out a diagnostic warning message.
            errmsg << "DSES::ExecutionControl::process_execution_control_updates():"
                   << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                   << execution_control_enum_to_string( this->current_execution_control_mode )
                   << ") and the requested execution mode ("
                   << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << ")!"
                   << THLA_ENDL;
            send_hs( stdout, (char *)errmsg.str().c_str() );

            // Mark the current execution mode as SHUTDOWN.
            this->current_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
            ExCO->current_execution_mode         = EXECUTION_MODE_SHUTDOWN;

            // Tell the TrickHLA::Federate to shutdown.
            // The DSES ExecutionControl shutdown transition will be made from
            // the TrickHLA::Federate::shutdown() job.
            the_exec->stop();
         } else if ( this->requested_execution_control_mode == EXECUTION_CONTROL_FREEZE ) {

            // Print diagnostic message if appropriate.
            if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               cout << "DSES::ExecutionControl::process_execution_control_updates():" << __LINE__ << endl
                    << "\t current_scenario_time:     " << setprecision( 18 ) << this->scenario_timeline->get_time() << endl
                    << "\t scenario_time_epoch:       " << setprecision( 18 ) << this->scenario_timeline->get_epoch() << endl
                    << "\t scenario_time_epoch(ExCO): " << setprecision( 18 ) << ExCO->scenario_time_epoch << endl
                    << "\t scenario_time_sim_offset:  " << setprecision( 18 ) << this->scenario_timeline->get_sim_offset() << endl
                    << "\t current_sim_time:          " << setprecision( 18 ) << this->sim_timeline->get_time() << endl
                    << "\t simulation_time_epoch:     " << setprecision( 18 ) << this->sim_timeline->get_epoch() << endl;
               if ( this->does_cte_timeline_exist() ) {
                  cout << "\t current_CTE_time:          " << setprecision( 18 ) << this->cte_timeline->get_time() << endl
                       << "\t CTE_time_epoch:            " << setprecision( 18 ) << this->cte_timeline->get_epoch() << endl;
               }
               cout << "\t next_mode_scenario_time:   " << setprecision( 18 ) << ExCO->next_mode_scenario_time << endl
                    << "\t next_mode_cte_time:        " << setprecision( 18 ) << ExCO->next_mode_cte_time << endl
                    << "\t scenario_freeze_time:      " << setprecision( 18 ) << this->scenario_freeze_time << endl
                    << "\t simulation_freeze_time:    " << setprecision( 18 ) << this->simulation_freeze_time << endl
                    << "=============================================================" << endl;
            }

            // Announce the pending freeze.
            this->freeze_mode_announce();

            // Tell Trick to go into freeze at the appointed time.
            the_exec->freeze( this->simulation_freeze_time );

            // The freeze transition logic will be done just before entering
            // Freeze. This is done in the TrickHLA::Federate::freeze_init()
            // routine called when entering Freeze.

         } else {

            errmsg << "DSES::ExecutionControl::process_execution_control_updates():"
                   << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                   << execution_control_enum_to_string( this->current_execution_control_mode )
                   << ") and the requested execution mode ("
                   << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << ")!"
                   << THLA_ENDL;
            send_hs( stdout, (char *)errmsg.str().c_str() );

            // Return that no mode changes occurred.
            return false;
         }

         // Return that a mode change occurred.
         return true;
         break;

      case EXECUTION_CONTROL_FREEZE:

         // Check for SHUTDOWN.
         if ( this->requested_execution_control_mode == EXECUTION_CONTROL_SHUTDOWN ) {

            // Mark the current execution mode as SHUTDOWN.
            this->current_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
            ExCO->current_execution_mode         = EXECUTION_MODE_SHUTDOWN;

            // Shutdown the federate now.
            exec_get_exec_cpp()->stop();

         } else if ( this->requested_execution_control_mode == EXECUTION_CONTROL_RUNNING ) {

            // Tell Trick to exit freeze and go to run.
            the_exec->run();

            // The run transition logic will be done just when exiting
            // Freeze. This is done in the TrickHLA::Federate::exit_freeze()
            // routine called when entering Freeze.
            // this->run_mode_transition();

         } else {

            errmsg << "DSES::ExecutionControl::process_execution_control_updates():"
                   << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                   << execution_control_enum_to_string( this->current_execution_control_mode )
                   << ") and the requested execution mode ("
                   << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << ")!"
                   << THLA_ENDL;
            send_hs( stdout, (char *)errmsg.str().c_str() );

            // Return that no mode changes occurred.
            return false;
         }

         // Return that a mode change occurred.
         return true;
         break;

      case EXECUTION_CONTROL_SHUTDOWN:

         // Once in SHUTDOWN, we cannot do anything else.
         errmsg << "DSES::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Shutting down but received mode transition: "
                << execution_control_enum_to_string( this->requested_execution_control_mode )
                << THLA_ENDL;
         send_hs( stdout, (char *)errmsg.str().c_str() );

         // Return that no mode changes occurred.
         return false;
         break;

      default:
         break;
   }

   // Return that no mode changes occurred.
   return false;
}

bool ExecutionControl::run_mode_transition()
{
   RTIambassador *         RTI_amb  = federate->get_RTI_ambassador();
   ExecutionConfiguration *ExCO     = get_execution_configuration();
   SyncPnt *               sync_pnt = NULL;

   // Register the 'mtr_run' sync-point.
   if ( this->is_master() ) {
      sync_pnt = this->register_sync_point( *RTI_amb, MTR_RUN_SYNC_POINT );
   } else {
      sync_pnt = this->get_sync_point( MTR_RUN_SYNC_POINT );
   }

   // Make sure that we have a valid sync-point.
   if ( sync_pnt == (TrickHLA::SyncPnt *)NULL ) {
      ostringstream errmsg;
      errmsg << "DSES::ExecutionControl::run_mode_transition():" << __LINE__
             << " ERROR: The 'mtr_run' sync-point was not found!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      // Wait for 'mtr_run' sync-point announce.
      this->wait_for_announcement( federate, sync_pnt );

      // Achieve the 'mtr-run' sync-point.
      this->achieve_sync_point( *RTI_amb, sync_pnt );

      // Wait for 'mtr_run' sync-point synchronization.
      this->wait_for_synchronization( federate, sync_pnt );

      // Set the current execution mode to running.
      this->current_execution_control_mode = EXECUTION_CONTROL_RUNNING;
      ExCO->set_current_execution_mode( EXECUTION_MODE_RUNNING );

      // Check for CTE.
      if ( this->does_cte_timeline_exist() ) {

         double go_to_run_time;

         // The Master federate updates the ExCO with the CTE got-to-run time.
         if ( this->is_master() ) {

            go_to_run_time = ExCO->get_next_mode_cte_time();
            ExCO->send_init_data();

         } // Other federates wait on the ExCO update with the CTE go-to-run time.
         else {

            // Wait for the ExCO update with the CTE time.
            ExCO->wait_for_update();

            // Process the just received ExCO update.
            this->process_execution_control_updates();

            // Set the CTE time to go to run.
            go_to_run_time = ExCO->get_next_mode_cte_time();
         }

         // Wait for the CTE go-to-run time.
         double diff;
         while ( this->get_cte_time() < go_to_run_time ) {

            // Check for shutdown.
            federate->check_for_shutdown_with_termination();

            diff = go_to_run_time - this->get_cte_time();
            if ( fmod( diff, 1.0 ) == 0.0 ) {
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  send_hs( stdout, "DSES::ExecutionControl::run_mode_transition():%d Going to run in %G seconds.%c",
                           __LINE__, diff, THLA_NEWLINE );
               }
            }
         }

         // Print debug message if appropriate.
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            double curr_cte_time = this->get_cte_time();
            diff                 = curr_cte_time - go_to_run_time;
            send_hs( stdout, "DSES::ExecutionControl::run_mode_transition():%d \n  Going to run at CTE time %.18G seconds. \n  Current CTE time %.18G seconds. \n  Difference: %.9lf seconds.%c",
                     __LINE__, go_to_run_time, curr_cte_time, diff, THLA_NEWLINE );
         }
      }
   }

   return true;
}

void ExecutionControl::freeze_mode_announce()
{
   // Register the 'mtr_freeze' sync-point.
   if ( this->is_master() ) {
      this->register_sync_point( *( federate->get_RTI_ambassador() ), MTR_FREEZE_SYNC_POINT );
   }
}

bool ExecutionControl::freeze_mode_transition()
{
   RTIambassador *         RTI_amb  = federate->get_RTI_ambassador();
   ExecutionConfiguration *ExCO     = get_execution_configuration();
   TrickHLA::SyncPnt *     sync_pnt = NULL;

   // Get the 'mtr_freeze' sync-point.
   sync_pnt = this->get_sync_point( MTR_FREEZE_SYNC_POINT );

   // Make sure that we have a valid sync-point.
   if ( sync_pnt == (TrickHLA::SyncPnt *)NULL ) {
      ostringstream errmsg;
      errmsg << "DSES::ExecutionControl::freeze_mode_transition():" << __LINE__
             << " ERROR: The 'mtr_freeze' sync-point was not found!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      // Wait for 'mtr_freeze' sync-point announce.
      this->wait_for_sync_point_announcement( federate, sync_pnt );

      // Achieve the 'mtr_freeze' sync-point.
      this->achieve_sync_point( *RTI_amb, sync_pnt );

      // Wait for 'mtr_freeze' sync-point synchronization.
      this->wait_for_synchronization( federate, sync_pnt );

      // Set the current execution mode to freeze.
      this->current_execution_control_mode = EXECUTION_CONTROL_FREEZE;
      ExCO->set_current_execution_mode( EXECUTION_MODE_FREEZE );
   }
   return false;
}

bool ExecutionControl::check_freeze_exit()
{
   // Check if this is a Master federate.
   if ( this->is_master() ) {

      // Process and Mode Transition Requests.
      process_mode_transition_request();

      // Handle requests for ExCO updates.
      if ( this->execution_configuration->is_attribute_update_requested() ) {
         this->execution_configuration->send_requested_data();
      }

      // Check for Trick shutdown command.
      if ( the_exec->get_exec_command() == ExitCmd ) {
         // Tell the TrickHLA::Federate to shutdown.
         // The DSES ExecutionControl shutdown transition will be made from
         // the TrickHLA::Federate::shutdown() job.
         this->federate->shutdown();
      }

   } else {

      // Check to see if there was an ExCO update.
      this->execution_configuration->receive_init_data();

      // Process the ExCO update.
      this->process_execution_control_updates();

      // Check for shutdown.
      if ( this->current_execution_control_mode == EXECUTION_CONTROL_SHUTDOWN ) {
         // Tell the TrickHLA::Federate to shutdown.
         // The DSES Mode Manager shutdown transition will be made from
         // the TrickHLA::Federate::shutdown() job.
         this->federate->shutdown();
      }
   }

   return ( true );
}

void ExecutionControl::shutdown_mode_announce()
{

   // Only the Master federate will ever announce a shutdown.
   if ( !this->is_master() ) {
      return;
   }

   // If the current execution mode is uninitialized then we haven't gotten
   // far enough in the initialization process to shut anything down.
   if ( this->current_execution_control_mode == EXECUTION_CONTROL_UNINITIALIZED ) {
      return;
   }

   // Set the next execution mode to shutdown.
   this->set_next_execution_control_mode( EXECUTION_CONTROL_SHUTDOWN );

   // Send out the updated ExCO.
   this->get_execution_configuration()->send_init_data();

   // Clear the mode change request flag.
   this->clear_mode_transition_requested();
}

/*!
 * @job_class{shutdown}
 */
void ExecutionControl::shutdown_mode_transition()
{
   // Only the Master federate has any DSES tasks for shutdown.
   if ( !this->is_master() ) {
      return;
   }

   // If the current execution mode is uninitialized then we haven't gotten
   // far enough in the initialization process to shut anything down.
   if ( this->current_execution_control_mode == EXECUTION_CONTROL_UNINITIALIZED ) {
      return;
   }

   // Register the 'mtr_shutdown' sync-point.
   this->register_sync_point( *( federate->get_RTI_ambassador() ), MTR_SHUTDOWN_SYNC_POINT );
}

ExecutionConfiguration *ExecutionControl::get_execution_configuration()
{
   ExecutionConfiguration *ExCO;

   ExCO = dynamic_cast< ExecutionConfiguration * >( this->get_execution_configuration() );
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "DSES::ExecutionControl::epoch_and_root_frame_discovery_process():" << __LINE__
             << " ERROR: Execution Configureation is not an DSES ExCO." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return ( this->get_execution_configuration() );
}
