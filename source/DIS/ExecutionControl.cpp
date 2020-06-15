/*!
@file DIS/ExecutionControl.cpp
@ingroup DIS
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
@trick_link_dependency{../TrickHLA/SyncPntListBase.cpp}
@trick_link_dependency{ExecutionControl.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, DIS support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iomanip>
#include <math.h>
#include <unistd.h>

// Trick includes.
#include "trick/Executive.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Manager.hh"

// DIS include files.
#include "DIS/ExecutionConfiguration.hh"
#include "DIS/ExecutionControl.hh"

// DIS file level declarations.
namespace DIS
{

// ExecutionControl type string.
const std::wstring ExecutionControl::type = L"DIS";

// For DIS: a pause sync point for when master starts up in freeze to make other feds do the same
static const std::wstring INITIALIZE_SYNC_POINT     = L"initialize";
static const std::wstring STARTUP_SYNC_POINT        = L"startup";
static const std::wstring STARTUP_FREEZE_SYNC_POINT = L"pause_0.0";

} // namespace DIS

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;
using namespace DIS;

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl()
   : time_padding( 5.0 ),
     mode_transition_requested( false ),
     pending_mtr( DIS::MTR_UNINITIALIZED ),
     requested_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     current_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     next_mode_scenario_time( -std::numeric_limits< double >::max() ),
     next_mode_cte_time( -std::numeric_limits< double >::max() ),
     simulation_freeze_time( 0.0 ),
     scenario_freeze_time( 0.0 ),
     late_joiner( false ),
     late_joiner_determined( false ),
     federate( NULL ),
     wait_sleep( 1000 ),
     wait_timeout( 10000000 )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ExecutionControl::~ExecutionControl()
{
   clear_mode_values();
   return;
}

/*!
@details This routine will set a lot of the data in the TrickHLA::Federate that
is required for this execution control scheme.  This should greatly simplify
input files and reduce input file setting errors.

@job_class{initialization}
*/
void ExecutionControl::initialize(
   TrickHLA::Federate &fed )
{
   // Sanity check.
   if ( fed == NULL ) {
      send_hs( stdout, "DIS::ExecutionControl::initialize()%d : Null TrickHLA::Federate pointer!%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "DIS::ExecutionControl::initialize() ERROR: NULL pointer to TrickHLA::Federate!" );
   }

   // Set the reference to the TrickHLA::Federate.
   this->federate = fed;

   // Initialize the ExCO so that it knows about the manager.
   this->execution_configuration->initialize( this->get_manager() );

   // There are things that must me set for the DIS initialization.
   this->use_preset_master = true;

   // If this is the Master federate, then it must support Time
   // Management and be both Time Regulating and Time Constrained.
   if ( this->is_master() ) {
      federate->time_management  = true;
      federate->time_regulating  = true;
      federate->time_constrained = true;
   }

   // The software frame is set from the ExCO Least Common Time Step.
   // For the Master federate the Trick simulation software frame must
   // match the Least Common Time Step (LCTS).
   if ( this->is_master() ) {
      int64_t lcts                = least_common_time_step;
      double  software_frame_time = double( lcts ) / 1000000.0;
      exec_set_software_frame( software_frame_time );
   }

   // Add the Mode Transition Request synchronization points.
   this->add_sync_pnt( L"mtr_run" );
   this->add_sync_pnt( L"mtr_freeze" );
   this->add_sync_pnt( L"mtr_shutdown" );

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "DIS::ExecutionControl::initialize():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Must use a preset master.
   if ( !this->is_master_preset() ) {
      ostringstream errmsg;
      errmsg << "DIS::ExecutionControl::initialize(): WARNING:" << __LINE__
             << " Only a preset master is supported. Make sure to set"
             << " 'THLA.federate.use_preset_master = true' in your input file."
             << " Setting use_preset_master to true!"
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      this->use_preset_master = true;
   }

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      if ( this->is_master() ) {
         send_hs( stdout, "DIS::ExecutionControl::initialize():%d\n    I AM THE PRESET MASTER%c",
                  __LINE__, THLA_NEWLINE );
      } else {
         send_hs( stdout, "DIS::ExecutionControl::initialize():%d\n    I AM NOT THE PRESET MASTER%c",
                  __LINE__, THLA_NEWLINE );
      }
   }

   // Call the DIS ExecutionControl pre-multi-phasse initialization processes.
   pre_multi_phase_init_processes();

   return;
}

/*!
@details This routine implements the DIS Join Federation Process described
in section 7.2 and figure 7-3.

@job_class{initialization}
*/
void ExecutionControl::join_federation_process()
{

   // The base class implementation is good enough for now.
   TrickHLA::ExecutionControlBase::join_federation_process();

   return;
}

/*!
@details This routine implements the DIS pre multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::pre_multi_phase_init_processes()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "DIS::ExecutionControl::pre_multi_phase_init_processes:%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Reset the ownership flags and the attribute configuration flags for
   // the simulation configuration object.
   execution_configuration->reset_ownership_states();

   // Setup all the Trick Ref-Attributes for the user specified objects,
   // attributes, interactions and parameters.
   get_manager()->setup_all_ref_attributes();

   if ( !get_manager()->federate_has_been_restored ) {
      // Add the DIS initialization sync-points now that Trick ref attributes
      // have been setup. We do this here so that we can handle the RTI
      // callbacks that use them.
      this->add_multiphase_init_sync_points();
   }

   // Create the RTI Ambassador and connect.
   federate->create_RTI_ambassador_and_connect();

   // Destroy the federation if it was orphaned from a previous simulation
   // run that did not shutdown cleanly.
   federate->destroy_orphaned_federation();

   // Create and then join the federation.
   federate->create_and_join_federation();

   if ( !get_manager()->federate_has_been_restored ) {
      // Don't forget to enable asynchronous delivery of messages.
      federate->enable_async_delivery();
   }

   // Setup all the RTI handles for the objects, attributes and interaction
   // parameters.
   get_manager()->setup_all_RTI_handles();

   // Call publish_and_subscribe AFTER we've initialized the manager, federate,
   // and FedAmb.
   get_manager()->publish_and_subscribe();

   // If this is a restarted federate, re-discover the objects without
   // re-registering them.
   if ( get_manager()->federate_has_been_restored ) {

      // Setup all the Trick Ref-Attributes for the user specified objects,
      // attributes, interactions and parameters.
      get_manager()->setup_all_ref_attributes();

      // Set the object instance handles based on its name.
      get_manager()->set_all_object_instance_handles_by_name();

      // Re-Initialize the MOM interface handles.
      federate->initialize_MOM_handles();

   } else {
      // Reserve the RTI object instance names with the RTI, but only for
      // the objects that are locally owned.
      get_manager()->reserve_object_names_with_RTI();

      // Waits on the reservation of the RTI object instance names for the
      // locally owned objects. Calling this function will block until all
      // the object instances names for the locally owned objects have been
      // reserved.
      get_manager()->wait_on_reservation_of_object_names();

      // Creates an RTI object instance and registers it with the RTI, but
      // only for the objects that are locally owned.
      get_manager()->register_objects_with_RTI();

      // Setup the preferred order for all object attributes and interactions.
      get_manager()->setup_preferred_order_with_RTI();

      //DANNY2.7 moved this here because there is race condition (starting with
      // Pitch 4.4) after wait_on_registration_of_required_objects any federate
      // (instead of the 1st one started) can become the master because
      // wait_on_registration_of_required_objects is random. We need the 1st
      // federate started to be the master so that master's sim control panel
      // controls pausing and checkpoints.
      // Determine if this federate is the Master.
      determine_federation_master();

      // Waits on the registration of all the required RTI object instances with
      // the RTI. Calling this function will block until all the required object
      // instances in the Federation have been registered.
      get_manager()->wait_on_registration_of_required_objects();

      // Initialize the MOM interface handles.
      federate->initialize_MOM_handles();

      // Perform the next few steps if we are the Master federate.
      if ( this->is_master() ) {

         // Make sure all required federates have joined the federation.
         (void)federate->wait_for_required_federates_to_join();

         //DANNY2.7 when master is started in freeze, create a pause sync point
         // so other feds will start in freeze
         if ( exec_get_freeze_command() != 0 ) {
            federate->register_generic_sync_point( DIS::STARTUP_FREEZE_SYNC_POINT, 0.0 );
         }
      }

      // Achieve the "initialize" sync-point and wait for the federation
      // to be synchronized on it.
      federate->achieve_and_wait_for_synchronization( DIS::INITIALIZE_SYNC_POINT );
   }

   return;
}

/*!
@details This routine implements the DIS post multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::post_multi_phase_init_process()
{
   //DANNY2.7 need this so that write_running_feds_file for checkpoint will work
   federate->load_and_print_running_federate_names();

   // Setup HLA time management.
   federate->setup_time_management();

   // Check to see if this federate has been restored.
   if ( !get_manager()->federate_has_been_restored ) {
      // Achieve the "startup" sync-point and wait for the federation
      // to be synchronized on it.
      federate->achieve_and_wait_for_synchronization( DIS::STARTUP_SYNC_POINT );
   }

   return;
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
void ExecutionControl::determine_federation_master()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::determine_federation_master():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Register the initialization synchronization points and wait for confirmation.
   this->register_all_sync_pnts( *( federate->get_RTI_ambassador() ) );
   this->wait_for_all_announcements( federate );

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      if ( this->is_master() ) {
         send_hs( stdout, "Manager::determine_federation_master():%d\n    I AM THE MASTER%c",
                  __LINE__, THLA_NEWLINE );
      } else {
         send_hs( stdout, "Manager::determine_federation_master():%d\n    I AM NOT THE MASTER%c",
                  __LINE__, THLA_NEWLINE );
      }
   }

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
   // Register initialization synchronization points used for startup regulation.
   this->add_sync_pnt( DIS::STARTUP_SYNC_POINT );
   this->add_sync_pnt( DIS::INITIALIZE_SYNC_POINT );

   return;
}

void ExecutionControl::announce_sync_point(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   wstring const &                   label,
   RTI1516_USERDATA const &          user_supplied_tag )
{
   // Parse the sync-point to see if we have a pause sync-point.
   string label_str;
   StringUtilities::to_string( label_str, label );
   vector< string > pause_label_vec;
   StringUtilities::tokenize( label_str, pause_label_vec, "_" );

   // Determine if we have a pause sync-point of the form "pause_1.0".
   if ( !pause_label_vec.empty() && ( pause_label_vec[0] == "pause" ) ) {
      Int64Time *pauseTime = NULL;

      try {
         // If we have a suer supplied tag it should hold the pause time.
         if ( user_supplied_tag.size() > 0 ) {
            pauseTime = new Int64Time;
            pauseTime->decode( user_supplied_tag );
         } else if ( pause_label_vec.size() == 2 ) {
            // We have the second part after the "_" delimiter of the
            // "pause_1.0" label, which is the pause time so decode it.
            pauseTime = new Int64Time( atof( pause_label_vec[1].c_str() ) );
         } else {
            // We could not extract a pause time so use a default of 0.0.
            pauseTime = new Int64Time( 0.0 );
         }
      } catch ( InternalError &e ) {
         if ( pauseTime == NULL ) {
            pauseTime = new Int64Time( 0.0 );
         } else {
            pauseTime->setTo( 0.0 );
         }
      } catch ( CouldNotDecode &e ) {
         if ( pauseTime == NULL ) {
            pauseTime = new Int64Time( 0.0 );
         } else {
            pauseTime->setTo( 0.0 );
         }
      }

      if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         send_hs( stdout, "DIS::ExecutionControl::announce_sync_point():%d DIS Pause Sync-Point:'%ls' Pause-time:%g %c",
                  __LINE__, label.c_str(), pauseTime->getDoubleTime(), THLA_NEWLINE );
      }
      this->add_pause( pauseTime, label );

   } else if ( this->contains( label ) ) {
      // Mark init sync-point as existing.
      if ( this->mark_announced( label ) ) {
         if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            send_hs( stdout, "DIS::ExecutionControl::announce_sync_point():%d DIS Simulation Init Sync-Point:'%ls'%c",
                     __LINE__, label.c_str(), THLA_NEWLINE );
         }
      }

   } // By default, mark an unrecognized synchronization point is achieved.
   else {

      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         send_hs( stdout, "DIS::ExecutionControl::announce_sync_point():%d Unrecognized synchronization point:'%ls', which will be achieved.%c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
      }

      // Unknown synchronization point so achieve it but don't wait for the
      // federation to be synchronized on it.
      this->achieve_sync_pnt( rti_ambassador, label );
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::clear_multiphase_init_sync_points()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      ostringstream errmsg;
      errmsg << "DIS::ExecutionControl::clear_multiphase_init_sync_points():" << __LINE__
             << " This call will be ignored because this ExecutionControl does not"
             << " support multiphase initialization synchronization points." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   }

   return;
}

void ExecutionControl::sync_point_registration_succeeded(
   wstring const &label )
{

   if ( label.compare( DIS::INITIALIZE_SYNC_POINT ) == 0 ) {
      this->set_master( true );
   } else if ( label.compare( DIS::STARTUP_SYNC_POINT ) == 0 ) {
      federate->set_startup( true );
   }

   return;
}

void ExecutionControl::sync_point_registration_failed(
   wstring const &label,
   bool           not_unique )
{

   if ( label.compare( DIS::INITIALIZE_SYNC_POINT ) == 0 ) {
      if ( not_unique ) {
         this->set_master( false );
      } else {
         send_hs( stderr, "DIS::ExecutionControl::sync_point_registration_failed():%d \
Federate shutting down.%c",
                  __LINE__, THLA_NEWLINE );
         exit( 1 );
      }
   } else if ( label.compare( DIS::STARTUP_SYNC_POINT ) == 0 ) {
      if ( not_unique ) {
         federate->set_startup( true );
      } else {
         send_hs( stderr, "DIS::ExecutionControl::sync_point_registration_failed():%d \
Startup Synchronization Point failure, federate shutting down.%c",
                  __LINE__,
                  THLA_NEWLINE );
         exit( 1 );
      }
   }

   return;
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

   // For DIS compatible initialization, if the federate has been
   // restored, the re-reservation of the name is not a fatal condition!
   if ( this->get_manager()->federate_has_been_restored ) {
      return ( true );
   }

   return ( false );
}

bool ExecutionControl::set_pending_mtr(
   MTREnum mtr_value )
{
   if ( this->is_mtr_valid( mtr_value ) ) {
      this->pending_mtr = mtr_value;
   }
   return False;
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

   return;
}

void ExecutionControl::set_next_execution_control_mode(
   TrickHLA::ExecutionControlEnum exec_control )
{
   // Reference the DIS Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // This should only be called by the Master federate.
   if ( !this->is_master() ) {
      ostringstream errmsg;
      errmsg << "DIS::ExecutionControl::set_next_execution_mode():" << __LINE__
             << " This should only be called by the Master federate!" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
      return;
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
      if ( get_manager()->should_print( TrickHLA::DEBUG_LEVEL_1_TRACE,
                                        TrickHLA::DEBUG_SOURCE_MANAGER ) ) {
         ostringstream errmsg;
         errmsg << "DIS::ExecutionControl::set_next_execution_mode():"
                << __LINE__ << " WARNING: Unknown execution mode value: " << exec_control
                << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );
      }
      break;
   }

   return;
}

bool ExecutionControl::check_mode_transition_request()
{
   // Just return if False mode change has been requested.
   if ( !this->is_mode_transition_requested() ) {
      return False;
   }

   // Only the Master federate receives and processes Mode Transition Requests.
   if ( !this->is_master() ) {
      ostringstream errmsg;
      errmsg << "DIS::ExecutionControl::check_mode_transition_request():"
             << __LINE__ << " WARNING: Received Mode Transition Request and not Master: "
             << mtr_enum_to_string( this->pending_mtr )
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      return False;
   }

   // First check to see if this is a valid MTR.
   if ( !( is_mtr_valid( this->pending_mtr ) ) ) {
      ostringstream errmsg;
      errmsg << "DIS::ExecutionControl::check_mode_transition_request():"
             << __LINE__ << " WARNING: Invalid Mode Transition Request: "
             << mtr_enum_to_string( this->pending_mtr )
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      return False;
   }

   return true;
}

bool ExecutionControl::process_mode_transition_request()
{
   // Just return is no mode change has been requested.
   if ( !this->check_mode_transition_request() ) {
      return False;
   } else {
      // Since this is a valid MTR, set the next mode from the MTR.
      this->set_mode_request_from_mtr( this->pending_mtr );
   }

   // Reference the DIS Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = this->get_execution_configuration();

   // Print diagnostic message if appropriate.
   if ( federate->should_print( TrickHLA::DEBUG_LEVEL_4_TRACE, TrickHLA::DEBUG_SOURCE_MANAGER ) ) {
      cout << "=============================================================" << endl
           << "ExecutionControl::process_mode_transition_request()" << endl
           << "\t current_scenario_time:     " << setprecision( 18 ) << this->scenario_timeline->get_time() << endl
           << "\t scenario_time_epoch:       " << setprecision( 18 ) << this->scenario_timeline->get_epoch() << endl
           << "\t scenario_time_epoch(ExCO): " << setprecision( 18 ) << ExCO->scenario_time_epoch << endl
           << "\t scenario_time_sim_offset:  " << setprecision( 18 ) << this->scenario_timeline->get_sim_offset() << endl
           << "\t Current HLA grant time:    " << federate->get_granted_time() << endl
           << "\t Current HLA request time:  " << federate->get_requested_time() << endl
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
         // Freeze.  This is done in the TrickHLA::Federate::freeze_init()
         // routine called when entering Freeze.
      }

      return true;
      break;

   case MTR_GOTO_SHUTDOWN:

      // Announce the shutdown.
      this->shutdown_mode_announce();

      // Tell Trick to shutdown sometime in the future.
      // The DIS ExecutionControl shutdown transition will be made from
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

   // Reference the DIS Execution Configuration Object (ExCO)
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
      errmsg << "DIS::ExecutionControl::process_execution_control_updates():"
             << __LINE__ << " WARNING: Master receive an ExCO update: "
             << execution_control_enum_to_string( this->requested_execution_control_mode )
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );

      // Return that no mode changes occurred.
      return false;
   }

   // Translate the native ExCO mode values into ExecutionModeEnum.
   ExecutionModeEnum exco_cem = execution_mode_int16_to_enum( ExCO->current_execution_mode );
   ExecutionModeEnum exco_nem = execution_mode_int16_to_enum( ExCO->next_execution_mode );

   // Check for consistency between ExecutionControl and ExCO.
   if ( exco_cem != this->current_execution_control_mode ) {
      errmsg << "DIS::ExecutionControl::process_execution_control_updates():"
             << __LINE__ << " WARNING: Current execution mode mismatch between ExecutionControl ("
             << execution_control_enum_to_string( this->current_execution_control_mode )
             << ") and the ExCO current execution mode ("
             << execution_mode_enum_to_string( exco_cem )
             << ")!"
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
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
         errmsg << "DIS::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Invalid ExCO next execution mode: "
                << execution_mode_enum_to_string( exco_nem ) << "!"
                << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );

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
         // The DIS ExecutionControl shutdown transition will be made from
         // the TrickHLA::Federate::shutdown() job.
         the_exec->stop();

      } else {

         errmsg << "DIS::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                << execution_control_enum_to_string( this->current_execution_control_mode )
                << ") and the requested execution mode ("
                << execution_control_enum_to_string( this->requested_execution_control_mode )
                << ")!"
                << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );

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
         // The DIS ExecutionControl shutdown transition will be made from
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
         // Freeze.  This is done in the TrickHLA::Federate::freeze_init()
         // routine called when entering Freeze.

      } else if ( this->requested_execution_control_mode == EXECUTION_CONTROL_INITIALIZING ) {

         // There's really nothing to do here.

      } else {

         errmsg << "DIS::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                << execution_control_enum_to_string( this->current_execution_control_mode )
                << ") and the requested execution mode ("
                << execution_control_enum_to_string( this->requested_execution_control_mode )
                << ")!"
                << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );

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
         errmsg << "DIS::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                << execution_control_enum_to_string( this->current_execution_control_mode )
                << ") and the requested execution mode ("
                << execution_control_enum_to_string( this->requested_execution_control_mode )
                << ")!"
                << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );

         // Mark the current execution mode as SHUTDOWN.
         this->current_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
         ExCO->current_execution_mode         = EXECUTION_MODE_SHUTDOWN;

         // Tell the TrickHLA::Federate to shutdown.
         // The DIS ExecutionControl shutdown transition will be made from
         // the TrickHLA::Federate::shutdown() job.
         the_exec->stop();
      } else if ( this->requested_execution_control_mode == EXECUTION_CONTROL_FREEZE ) {

         // Print diagnostic message if appropriate.
         if ( federate->should_print( TrickHLA::DEBUG_LEVEL_4_TRACE, TrickHLA::DEBUG_SOURCE_MANAGER ) ) {
            cout << "ExecutionControl::process_execution_control_updates()" << endl
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
         // Freeze.  This is done in the TrickHLA::Federate::freeze_init()
         // routine called when entering Freeze.

      } else {

         errmsg << "DIS::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                << execution_control_enum_to_string( this->current_execution_control_mode )
                << ") and the requested execution mode ("
                << execution_control_enum_to_string( this->requested_execution_control_mode )
                << ")!"
                << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );

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
         // Freeze.  This is done in the TrickHLA::Federate::exit_freeze()
         // routine called when entering Freeze.
         // this->run_mode_transition();

      } else {

         errmsg << "DIS::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                << execution_control_enum_to_string( this->current_execution_control_mode )
                << ") and the requested execution mode ("
                << execution_control_enum_to_string( this->requested_execution_control_mode )
                << ")!"
                << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );

         // Return that no mode changes occurred.
         return false;
      }

      // Return that a mode change occurred.
      return true;
      break;

   case EXECUTION_CONTROL_SHUTDOWN:

      // Once in SHUTDOWN, we cannot do anything else.
      errmsg << "DIS::ExecutionControl::process_execution_control_updates():"
             << __LINE__ << " WARNING: Shutting down but received mode transition: "
             << execution_control_enum_to_string( this->requested_execution_control_mode )
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );

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
      sync_pnt = this->register_sync_pnt( *RTI_amb, L"mtr_run" );
   } else {
      sync_pnt = this->get_sync_pnt( L"mtr_run" );
   }

   // Make sure that we have a valid sync-point.
   if ( sync_pnt == (TrickHLA::SyncPnt *)NULL ) {
      ostringstream errmsg;
      errmsg << "DIS::ExecutionControl::run_mode_transition():" << __LINE__
             << " The 'mtr_run' sync-point was not found!" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } else {

      // Wait for 'mtr_run' sync-point announce.
      sync_pnt->wait_for_announce( federate );

      // Achieve the 'mtr-run' sync-point.
      sync_pnt->achieve_sync_point( *RTI_amb );

      // Wait for 'mtr_run' sync-point synchronization.
      sync_pnt->wait_for_synchronization( federate );

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
            ExCO->wait_on_update();

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
               if ( debug_handler.should_print( TrickHLA::DEBUG_LEVEL_2_TRACE,
                                                TrickHLA::DEBUG_SOURCE_MANAGER ) ) {
                  send_hs( stdout, "ExecutionControl::run_mode_transition():%d Going to run in %G seconds.%c",
                           __LINE__, diff, THLA_NEWLINE );
               }
            }
         }

         // Print debug message if appropriate.
         if ( debug_handler.should_print( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_MANAGER ) ) {
            double curr_cte_time = this->get_cte_time();
            diff                 = curr_cte_time - go_to_run_time;
            send_hs( stdout, "ExecutionControl::run_mode_transition():%d \n  Going to run at CTE time %.18G seconds. \n  Current CTE time %.18G seconds. \n  Difference: %.9lf seconds.%c",
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
      this->register_sync_pnt( *( federate->get_RTI_ambassador() ), L"mtr_freeze" );
   }

   return;
}

bool ExecutionControl::freeze_mode_transition()
{
   RTIambassador *         RTI_amb  = federate->get_RTI_ambassador();
   ExecutionConfiguration *ExCO     = get_execution_configuration();
   TrickHLA::SyncPnt *     sync_pnt = NULL;

   // Get the 'mtr_freeze' sync-point.
   sync_pnt = this->get_sync_pnt( L"mtr_freeze" );

   // Make sure that we have a valid sync-point.
   if ( sync_pnt == (TrickHLA::SyncPnt *)NULL ) {
      ostringstream errmsg;
      errmsg << "DIS::ExecutionControl::freeze_mode_transition():" << __LINE__
             << " The 'mtr_freeze' sync-point was not found!" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } else {

      // Wait for 'mtr_freeze' sync-point announce.
      sync_pnt->wait_for_announce( federate );

      // Achieve the 'mtr_freeze' sync-point.
      sync_pnt->achieve_sync_point( *RTI_amb );

      // Wait for 'mtr_freeze' sync-point synchronization.
      sync_pnt->wait_for_synchronization( federate );

      // Set the current execution mode to freeze.
      this->current_execution_control_mode = EXECUTION_CONTROL_FREEZE;
      ExCO->set_current_execution_mode( EXECUTION_MODE_FREEZE );
   }

   return false;
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

   return;
}

/*!
 * @job_class{shutdown}
 */
void ExecutionControl::shutdown_mode_transition()
{

   // Only the Master federate has any DIS tasks for shutdown.
   if ( !this->is_master() ) {
      return;
   }

   // If the current execution mode is uninitialized then we haven't gotten
   // far enough in the initialization process to shut anything down.
   if ( this->current_execution_control_mode == EXECUTION_CONTROL_UNINITIALIZED ) {
      return;
   }

   // Register the 'mtr_shutdown' sync-point.
   this->register_sync_pnt( *( federate->get_RTI_ambassador() ), L"mtr_shutdown" );

   return;
}

void ExecutionControl::check_freeze()
{
   return;
}

void ExecutionControl::enter_freeze()
{
   //DANNY2.7 create a pause sync point when master hits Sim Control Panel
   // Freeze button (if the Federation Manager is master then this has
   // no effect) determine if I am the federate that clicked Freeze
   if ( this->get_sim_time() <= 0.0 ) {
      federate->announce_freeze = this->is_master();
   } else if ( !federate->freeze_the_federation ) {
      federate->announce_freeze = true;
   }

   if ( federate->announce_freeze ) {
      // Register sync point unless: We are here because we have reached
      // a pause sync-pt, or we are here because we started in freeze
      if ( ( !federate->freeze_the_federation ) && ( this->get_sim_time() > 0.0 ) ) {
         double  pause_time;
         char    pause_label[32];
         wstring pause_label_ws;
         // WARNING: DIS inherent race condition here:
         // DIS uses sync points to freeze, so we must give federates
         // ample time to achieve the sync point. We do this by setting
         // the sync point time to freeze_delay_frames * lookahead_time,
         // check_pause will put us into freeze at that time (user may
         // need to increase freeze_delay_frames).
         pause_time = this->get_sim_time() + ( federate->freeze_delay_frames * federate->lookahead_time );
         if ( pause_time <= this->get_sim_time() ) {
            pause_time = this->get_sim_time() + 3.0;
         }

         sprintf( pause_label, "pause_%f", pause_time );
         StringUtilities::to_wstring( pause_label_ws, pause_label );

         if ( should_print( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            send_hs( stdout,
                     "DIS::ExecutionControl::enter_freeze():%d announce_freeze:%s, freeze_federation:%s, pause_time:%g %c",
                     __LINE__, ( federate->announce_freeze ? "Yes" : "No" ),
                     ( federate->freeze_the_federation ? "Yes" : "No" ), pause_time,
                     THLA_NEWLINE );
         }

         federate->register_generic_sync_point( pause_label_ws, pause_time );
         federate->un_freeze(); // will freeze again for real when we hit pause sync-pt
      }
   }

   return;
}

bool ExecutionControl::check_freeze_exit()
{
   // If freeze has not been announced, then return false.
   if ( !federate->announce_freeze ) {
      return ( false );
   }

   return ( true );
}

void ExecutionControl::exit_freeze()
{
   if ( federate->announce_freeze ) {          //DANNY2.7
      if ( federate->freeze_the_federation ) { // coming out of freeze due to reaching sync point
         federate->announce_freeze = false;    // reset for the next time we freeze
      }
      // so acknowledge the pause sync point
      //(if the Federation Manager is master then this has no effect)
      try {
         this->pause_sync_pts.achieve_all_sync_pnts( *federate->get_RTI_ambassador(), this->checktime );
      } catch ( SynchronizationPointLabelNotAnnounced &e ) {
         send_hs( stderr, "DIS::ExecutionControl::exit_freeze():%d EXCEPTION: SynchronizationPointLabelNotAnnounced%c",
                  __LINE__, THLA_NEWLINE );
      } catch ( FederateNotExecutionMember &e ) {
         send_hs( stderr, "DIS::ExecutionControl::exit_freeze():%d EXCEPTION: FederateNotExecutionMember%c",
                  __LINE__, THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         send_hs( stderr, "DIS::ExecutionControl::exit_freeze():%d EXCEPTION: SaveInProgress%c",
                  __LINE__, THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         send_hs( stderr, "DIS::ExecutionControl::exit_freeze():%d EXCEPTION: RestoreInProgress%c",
                  __LINE__, THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "DIS::ExecutionControl::exit_freeze():%d EXCEPTION: RTIinternalError: '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      }
   }

   return;
}

void ExecutionControl::add_pause(
   Int64Time *    time,
   wstring const &label )
{
   pause_sync_pts.add_sync_pnt( label, *time );
}

ExecutionConfiguration *ExecutionControl::get_execution_configuration()
{
   ExecutionConfiguration *ExCO;

   ExCO = dynamic_cast< ExecutionConfiguration * >( this->get_execution_configuration() );
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "DIS::ExecutionControl::epoch_and_root_frame_discovery_process():" << __LINE__
             << " ERROR: Execution Configureation is not an DIS ExCO." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }
   return ( this->get_execution_configuration() );
}

bool ExecutionControl::is_save_initiated()
{
   //TODO: should DIS use a sync point like IMSim here ?
   federate->initiate_save_flag = true;
   return ( true );
}
