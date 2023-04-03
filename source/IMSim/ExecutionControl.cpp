/*!
@file IMSim/ExecutionControl.cpp
@ingroup IMSim
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
@trick_link_dependency{PausePointList.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, IMSim support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <float.h>
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
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

// IMSim include files.
#include "IMSim/ExecutionConfiguration.hh"
#include "IMSim/ExecutionControl.hh"

// IMSim file level declarations.
namespace IMSim
{

// ExecutionControl type string.
std::string const ExecutionControl::type = "IMSim";

// The IMSim Multiphase initialization HLA synchronization-points (version 2).
static std::wstring const SIM_CONFIG_SYNC_POINT     = L"sim_config_v2";
static std::wstring const INITIALIZE_SYNC_POINT     = L"initialize_v2";
static std::wstring const INIT_COMPLETE_SYNC_POINT  = L"initialization_complete_v2";
static std::wstring const STARTUP_SYNC_POINT        = L"startup_v2";
static std::wstring const FEDSAVE_SYNC_POINT        = L"FEDSAVE_v2";
static std::wstring const FEDRUN_SYNC_POINT         = L"FEDRUN_v2";
static std::wstring const STARTUP_FREEZE_SYNC_POINT = L"pause_0.0";

// SISO SpaceFOM Mode Transition Request (MTR) synchronization-points.
static std::wstring const MTR_RUN_SYNC_POINT      = L"mtr_run";
static std::wstring const MTR_FREEZE_SYNC_POINT   = L"mtr_freeze";
static std::wstring const MTR_SHUTDOWN_SYNC_POINT = L"mtr_shutdown";

} // namespace IMSim

// Create the Trick ATTRIBUTES arrays needed for local allocations.
#ifdef __cplusplus
extern "C" {
#endif
// C based model includes.
extern ATTRIBUTES attrTrickHLA__FreezeInteractionHandler[];
#ifdef __cplusplus
}
#endif

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;
using namespace IMSim;

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl()
   : pending_mtr( IMSim::MTR_UNINITIALIZED ),
     freeze_inter_count( 0 ),
     freeze_interaction( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ExecutionControl::~ExecutionControl()
{
   this->clear_mode_values();

   // Free up the allocated Freeze Interaction.
   if ( freeze_interaction != static_cast< Interaction * >( NULL ) ) {
      FreezeInteractionHandler *ptr =
         static_cast< FreezeInteractionHandler * >(
            freeze_interaction->get_handler() );
      if ( ptr != static_cast< FreezeInteractionHandler * >( NULL ) ) {
         TMM_delete_var_a( ptr );
         freeze_interaction->set_handler( NULL );
      }
      TMM_delete_var_a( freeze_interaction );
      freeze_interaction = NULL;
      freeze_inter_count = 0;
   }
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
      msg << "IMSim::ExecutionControl::initialize():" << __LINE__
          << " Initialization-Scheme:'" << get_type()
          << "'" << THLA_ENDL;
      send_hs( stdout, (char *)msg.str().c_str() );
   }

   // Set the reference to the TrickHLA::Federate.
   this->federate = &federate;

   // Initialize the ExCO so that it knows about the manager.
   this->execution_configuration->initialize( this->get_manager() );

   // There are things that must me set for the IMSim initialization.
   this->use_preset_master = true;

   // If this is the Master federate, then it must support Time
   // Management and be both Time Regulating and Time Constrained.
   if ( this->is_master() ) {
      this->federate->time_management  = true;
      this->federate->time_regulating  = true;
      this->federate->time_constrained = true;

      // The software frame is set from the ExCO Least Common Time Step.
      // For the Master federate the Trick simulation software frame must
      // match the Least Common Time Step (LCTS).
      double software_frame_time = Int64Interval::to_seconds( this->least_common_time_step );
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
      errmsg << "IMSim::ExecutionControl::initialize():" << __LINE__
             << " WARNING: Only a preset master is supported. Make sure to set"
             << " 'THLA.federate.use_preset_master = true' in your input file."
             << " Setting use_preset_master to true!"
             << THLA_ENDL;
      send_hs( stdout, (char *)errmsg.str().c_str() );
      this->use_preset_master = true;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      if ( this->is_master() ) {
         send_hs( stdout, "IMSim::ExecutionControl::initialize():%d\n    I AM THE PRESET MASTER%c",
                  __LINE__, THLA_NEWLINE );
      } else {
         send_hs( stdout, "IMSim::ExecutionControl::initialize():%d\n    I AM NOT THE PRESET MASTER%c",
                  __LINE__, THLA_NEWLINE );
      }
   }

   // Call the IMSim ExecutionControl pre-multi-phase initialization processes.
   pre_multi_phase_init_processes();
}
/*!
@details This routine implements the IMSim Join Federation Process described
in section 7.2 and figure 7-3.

@job_class{initialization}
*/
void ExecutionControl::join_federation_process()
{
   // The base class implementation is good enough for now.
   TrickHLA::ExecutionControlBase::join_federation_process();
}

/*!
@details This routine implements the IMSim pre multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::pre_multi_phase_init_processes()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d%c",
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

   // Add the IMSim multiphase initialization sync-points now that the
   // ExecutionConfiguration has been initialized in the call to
   // the setup_all_ref_attributes() function. We do this here so
   // that we can handle the RTI callbacks that use them.
   this->add_multiphase_init_sync_points();

   // Create the RTI Ambassador and connect.
   federate->create_RTI_ambassador_and_connect();

   // Destroy the federation if it was orphaned from a previous simulation
   // run that did not shutdown cleanly.
   federate->destroy_orphaned_federation();

   // Create and then join the federation. We also determine if this federate
   // is the master if it successfully created the federation.
   federate->create_and_join_federation();

   // We are the master if we successfully created the federation and the
   // user has not preset a master value.
   if ( !this->is_master_preset() ) {
      this->set_master( federate->is_federation_created_by_federate() );
      execution_configuration->set_master( this->is_master() );
   }

   // Don't forget to enable asynchronous delivery of messages.
   federate->enable_async_delivery();

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      if ( this->is_master() ) {
         send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n    I AM THE MASTER%c",
                  __LINE__, THLA_NEWLINE );
      } else {
         send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n    I AM NOT THE MASTER%c",
                  __LINE__, THLA_NEWLINE );
      }
   }

   // Save restore_file_name before it gets wiped out with the loading of the checkpoint file...
   char *tRestoreName = NULL;
   if ( get_manager()->restore_file_name != NULL ) {
      // we don't want this to get wiped out when trick clears memory for load checkpoint, so don't allocate with TMM
      tRestoreName = strdup( get_manager()->restore_file_name );
   }

   // Initialize the MOM interface handles.
   federate->initialize_MOM_handles();

   if ( this->is_master() ) {
      //**** This federate is the Master for the multiphase ****
      //**** initialization process                         ****

      // if you want to restore from a check point, force the loading of the
      // checkpoint file here...
      if ( get_manager()->restore_federation ) {
         if ( ( tRestoreName != NULL ) && ( *tRestoreName != '\0' ) ) {

            // make sure that we have a valid absolute path to the files.
            federate->check_HLA_save_directory();

            // signal the MASTER federate to track all federates who join,
            // looking for anyone who is not required.
            federate->set_restore_is_imminent();

            // read the required federates data from external file, replacing
            // the contents of 'known_feds'.
            federate->read_running_feds_file( tRestoreName );

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d \
You indicated that you want a restore => I AM THE MASTER <= \
Waiting for the required federates to join.%c",
                        __LINE__, THLA_NEWLINE );
            }
            // Make sure only the required federates have joined the federation.
            string return_string;
            return_string = federate->wait_for_required_federates_to_join();
            if ( !return_string.empty() ) {
               ostringstream errmsg;
               errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                      << " " << return_string << THLA_ENDL;
               DebugHandler::terminate_with_message( errmsg.str() );
            }

            // Load the MASTER federate from the checkpoint file...
            federate->restore_checkpoint( tRestoreName );

            //
            // Even though the multiphase initialization document does not tell
            // us that we need to re-copy the '.running_feds' file over the
            // known_feds, which was overwritten with its contents from the
            // checkpoint file, we need to do so.
            //
            // The contents of the known_feds data structure needs to be fixed
            // so it contains the MOM names of all federates which were running
            // when the federation was saved.
            //
            // The Federate class uses the known_feds data structure
            // to re-establish all instance names based on the MOM names found
            // in the known_feds data structure.
            //
            // If known_feds contains an incomplete list, we run the risk of not
            // re-registering all objects and this may lead to strange and
            // unexpected results in the restored federation...
            //
            federate->copy_running_feds_into_known_feds();

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d \
You indicated that you want a restore => I AM THE MASTER <= \
initiating restore request for '%s' with the RTI.%c",
                        __LINE__, tRestoreName, THLA_NEWLINE );
            }
            // request federation restore from RTI
            federate->initiate_restore_announce( tRestoreName );

            // wait for the success / failure response from the RTI
            federate->wait_for_restore_request_callback();

            if ( federate->has_restore_request_failed() ) {
               ostringstream errmsg;
               errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():"
                      << __LINE__
                      << " You indicated that you wanted to restore a "
                      << "checkpoint => I AM THE MASTER <= RTI rejected the "
                      << "restore request!!!! Make sure that you are restoring "
                      << "the federates from an identical federation save set."
                      << "\n      See IEEE 1516.1-2000, Section 4.18 for "
                      << "further info for the reasons why the RTI would reject"
                      << " the federation restore request..." << THLA_ENDL;
               DebugHandler::terminate_with_message( errmsg.str() );
            }

            // Wait for RTI to inform us that the federation restore has
            // begun before informing the RTI that we are done.
            federate->wait_for_federation_restore_begun();

            // Wait for RTI to inform us that the federation restore has
            // begun before informing the RTI that we are done.
            federate->wait_until_federation_is_ready_to_restore();

            // Signal RTI that the MASTER federate has already been loaded
            // (above).
            federate->inform_RTI_of_restore_completion();

            // Wait until we get a callback to inform us that the federation
            // restore is complete. if a non-NULL string is returned, there was
            // an error so take appropriate action.
            string tStr = federate->wait_for_federation_restore_to_complete();
            if ( tStr.length() ) {
               federate->wait_for_federation_restore_failed_callback_to_complete();

               ostringstream errmsg;
               errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():"
                      << __LINE__
                      << " You indicated that you wanted to restore a "
                      << "checkpoint => I AM THE MASTER <= "
                      << "wait_for_federation_restore_to_complete() failed!!!"
                      << THLA_ENDL;
               errmsg << endl
                      << tStr;
               DebugHandler::terminate_with_message( errmsg.str() );
            }

            // Rebuild the 'federate handle set' because the federation was restored
            federate->restore_federate_handles_from_MOM();

            // Add and register the "STARTUP" sync point with all joined federates
            // so we can achieve it later.
            if ( this->add_sync_point( IMSim::STARTUP_SYNC_POINT ) ) {
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d Label: '%ls'%c",
                           __LINE__, IMSim::STARTUP_SYNC_POINT, THLA_NEWLINE );
               }
            } else {
               send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d Did not add duplicate synchronization point label '%ls'.%c",
                        __LINE__, IMSim::STARTUP_SYNC_POINT, THLA_NEWLINE );
            }
            this->register_sync_point( *federate->get_RTI_ambassador(),
                                       federate->get_joined_federate_handles(),
                                       IMSim::STARTUP_SYNC_POINT );

            // Wait for the announcement of "STARTUP" sync-point before proceeding.
            this->wait_for_sync_point_announcement( federate, IMSim::STARTUP_SYNC_POINT );

            // Restart myself...
            get_manager()->restart_initialization();

            // Restart the checkpoint...
            federate->restart_checkpoint();

            // Achieve the "STARTUP" sync-point and wait for the
            // federation synchronize on it.
            federate->achieve_and_wait_for_synchronization( IMSim::STARTUP_SYNC_POINT );

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               if ( this->is_late_joiner() ) {
                  send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n\t\
=> I AM THE MASTER ** originally a late joining federate ** <= Federation restore is complete\n    \
Simulation has started and is now running...%c",
                           __LINE__, THLA_NEWLINE );
               } else {
                  send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n    \
=> I AM THE MASTER <= Federation restore is complete\n\t\
Simulation has started and is now running...%c",
                           __LINE__, THLA_NEWLINE );
               }
            }

            federate->set_federate_has_begun_execution();
         } else {
            ostringstream errmsg;
            errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                   << " You indicated that you wanted to restore a checkpoint"
                   << " => I AM THE MASTER <= but you failed to specify the"
                   << "  checkpoint FILE NAME!" << THLA_ENDL;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      } else { // MASTER but restore was not specified

         // Setup all the RTI handles for the objects, attributes and interaction
         // parameters.
         get_manager()->setup_all_RTI_handles();

         // Make sure all required federates have joined the federation.
         federate->wait_for_required_federates_to_join();

         // Register the Multi-phase intialization sync-points just for the
         // joined federates.
         this->register_all_sync_points( *federate->get_RTI_ambassador(),
                                         federate->get_joined_federate_handles() );

         // Call publish_and_subscribe AFTER we've initialized the manager,
         // federate, and FedAmb.
         get_manager()->publish_and_subscribe();

         // Reserve "SimConfig" object instance name.
         execution_configuration->reserve_object_name_with_RTI();

         // Reserve the RTI object instance names with the RTI, but only for
         // the objects that are locally owned.
         get_manager()->reserve_object_names_with_RTI();

         // Wait for success or failure for the "SimConfig" name reservation.
         execution_configuration->wait_for_object_name_reservation();

         // Waits on the reservation of the RTI object instance names for the
         // locally owned objects. Calling this function will block until all
         // the object instances names for the locally owned objects have been
         // reserved.
         get_manager()->wait_for_reservation_of_object_names();

         // Creates an RTI object instance and registers it with the RTI, but
         // only for the objects that we create.
         get_manager()->register_objects_with_RTI();

         // Setup the preferred order for all object attributes and interactions.
         get_manager()->setup_preferred_order_with_RTI();

         // Waits on the registration of all the required RTI object instances
         // with the RTI. Calling this function will block until all the
         // required object instances in the Federation have been registered.
         get_manager()->wait_for_registration_of_required_objects();

         // Wait for the "sim_config", "initialize", and "startup" sync-points
         // to be registered.
         this->wait_for_all_announcements( federate );

         // Achieve the "sim_config" sync-point and wait for the federation
         // to be synchronized on it.
         federate->achieve_and_wait_for_synchronization( IMSim::SIM_CONFIG_SYNC_POINT );

         // Send the "Simulation Configuration".
         this->send_execution_configuration();

         // DANNY2.7 When master is started in freeze, create a pause sync point
         //  so other feds will start in freeze.
         if ( exec_get_freeze_command() != 0 ) {
            federate->register_generic_sync_point( IMSim::STARTUP_FREEZE_SYNC_POINT, 0.0 );
         }

         // Achieve the "initialize" sync-point and wait for the federation
         // to be synchronized on it.
         federate->achieve_and_wait_for_synchronization( IMSim::INITIALIZE_SYNC_POINT );
      }
   } else {

      // Determine if the federate is a late-joiner or was instructed to restore
      // the federation.
      if ( determine_if_late_joining_or_restoring_federate() == FEDERATE_JOIN_RESTORING ) {

         // make sure that we have a valid absolute path to the files.
         federate->check_HLA_save_directory();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d \
You indicated that you want a restore => I AM NOT THE MASTER <= \
loading of the federate from the checkpoint file '%s'.%c",
                     __LINE__, tRestoreName, THLA_NEWLINE );
         }
         federate->restore_checkpoint( tRestoreName );

         //
         // Even though the multiphase initialization document does not tell
         // us that we need to re-copy the '.running_feds' file over the
         // known_feds, which was overwritten with its contents from the
         // checkpoint file, we need to do so.
         //
         // the contents of the known_feds data structure needs to be fixed
         // so it contains the MOM names of all federates which were running
         // when the federation was saved.
         //
         // the Federate class uses the known_feds data structure
         // to re-establish all instance names based on the MOM names found
         // in the known_feds data structure.
         //
         // if known_feds contains an incomplete list, we run the risk of not
         // re-registering all objects and this may lead to strange and
         // unexpected results in the restored federation...
         //
         federate->copy_running_feds_into_known_feds();

         // wait for RTI to inform us that the federation restore has
         // begun before informing the RTI that we are done.
         federate->wait_for_federation_restore_begun();

         // wait for RTI to inform us that the federation restore has
         // begun before informing the RTI that we are done.
         federate->wait_until_federation_is_ready_to_restore();

         // signal RTI that we are done loading.
         federate->inform_RTI_of_restore_completion();

         // wait until we get a callback to inform us that the federation
         // restore is complete...
         string tStr = federate->wait_for_federation_restore_to_complete();
         if ( tStr.length() ) {
            federate->wait_for_federation_restore_failed_callback_to_complete();

            ostringstream errmsg;
            errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():"
                   << __LINE__
                   << " You indicated that you wanted to restore a "
                   << "checkpoint => I AM THE NOT MASTER <= "
                   << "wait_for_federation_restore_to_complete() failed!!!"
                   << THLA_ENDL
                   << endl
                   << tStr;
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // Wait for the announcement of "STARTUP" sync-point before proceeding.
         this->wait_for_sync_point_announcement( federate, IMSim::STARTUP_SYNC_POINT );

         // restart myself...
         get_manager()->restart_initialization();

         // restart the federate...
         federate->restart_checkpoint();

         // Achieve the "STARTUP" sync-point and wait for the federation
         // to be synchronized on it.
         federate->achieve_and_wait_for_synchronization( IMSim::STARTUP_SYNC_POINT );

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            if ( this->is_late_joiner() ) {
               send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n\t\
=> I AM NOT THE MASTER ** originally late joining federate ** <= Federation restore is complete\n    \
Simulation has started and is now running...%c",
                        __LINE__, THLA_NEWLINE );
            } else {
               send_hs( stdout, "IMSim::ExecutionControl::pre_multi_phase_init_processes2():%d\n    \
=> I AM NOT THE MASTER <= Federation restore is complete\n    \
Simulation has started and is now running...%c",
                        __LINE__, THLA_NEWLINE );
            }
         }

         federate->set_federate_has_begun_execution();
      } else { // non-MASTER federate; not restoring a checkpoint

         // Setup all the RTI handles for the objects, attributes and interaction
         // parameters.
         get_manager()->setup_all_RTI_handles();

         if ( !get_manager()->is_late_joining_federate() ) {
            //**** Non-Master federate that is Not late in joining the ****
            //**** federation, so it can participate in the multiphase ****
            //**** initialization process                              ****

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

            // Waits on the registration of all the required RTI object
            // instances with the RTI. Calling this function will block until
            // all the required object instances in the Federation have been
            // registered.
            get_manager()->wait_for_registration_of_required_objects();

            // Wait for the "sim_config", "initialize", and "startup" sync-points
            // to be registered.
            this->wait_for_all_announcements( federate );

            // Achieve the "sim_config" sync-point and wait for the federation
            // to be synchronized on it.
            federate->achieve_and_wait_for_synchronization( IMSim::SIM_CONFIG_SYNC_POINT );

            // Wait for the "Simulation Configuration" object attribute reflection.
            this->receive_execution_configuration();

            // Achieve the "initialize" sync-point and wait for the federation
            // to be synchronized on it.
            federate->achieve_and_wait_for_synchronization( IMSim::INITIALIZE_SYNC_POINT );

         } else {
            //**** Late Joining Federate. ****

            if ( !federate->is_time_management_enabled() ) {
               ostringstream errmsg;
               errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                      << " ERROR: Late joining federates that do not use HLA"
                      << " time management are not supported yet!" << THLA_ENDL;
               DebugHandler::terminate_with_message( errmsg.str() );
            }

            // Subscribe to the simulation configuration attributes.
            execution_configuration->subscribe_to_object_attributes();

            // Wait for the registration of the simulation configuration object.
            // Calling this function will block until all the simulation config
            // object instances in the Federation has been registered.
            execution_configuration->wait_for_registration();

            // Request a simulation configuration object update.
            get_manager()->request_data_update( execution_configuration->get_name() );

            // Wait for the "Simulation Configuration" object attribute reflection.
            execution_configuration->wait_for_update();

            // Call publish_and_subscribe AFTER we've initialized the manager,
            // federate, and FedAmb.
            get_manager()->publish_and_subscribe();

            // Wait for all objects to be discovered, if necessary.
            get_manager()->wait_for_discovery_of_objects();

            // If this is a rejoining federate, re-aquire ownership of its attributes.
            if ( get_manager()->is_this_a_rejoining_federate() ) {

               // Force ownership restore from other federate(s) and wait for
               // the ownership to complete before proceeding.
               get_manager()->pull_ownership_upon_rejoin();
            } else {

               // Federate is not rejoining the federation. proceed with reserving
               // and registering the local data object with the RTI.

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
            }

            // Setup the preferred order for all object attributes and interactions.
            get_manager()->setup_preferred_order_with_RTI();

            // Waits on the registration of all the required RTI object
            // instances with the RTI. Calling this function will block until
            // all the required object instances in the Federation have been
            // registered.
            get_manager()->wait_for_registration_of_required_objects();
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
FederateJoinEnum ExecutionControl::determine_if_late_joining_or_restoring_federate()
{
   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   // Block until we have determined if we are a late joining federate.
   while ( !late_joiner_determined && !get_manager()->restore_determined ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // We are not a late joiner if the Sim-Config sync-point exists are we
      // are a member for it.
      if ( !this->get_manager()->restore_determined
           && this->contains( IMSim::SIM_CONFIG_SYNC_POINT ) ) {
         this->late_joiner            = false;
         this->late_joiner_determined = true;
      }

      // Determine if the Initialization Complete sync-point exists, which
      // means at this point we are a late joining federate.
      if ( !late_joiner_determined
           && !get_manager()->restore_determined
           && this->does_init_complete_sync_point_exist() ) {
         this->late_joiner            = true;
         this->late_joiner_determined = true;
      }

      // when we receive the signal to restore, set the flag.
      if ( !late_joiner_determined && federate->has_restore_been_announced() && federate->is_start_to_restore() ) {
         get_manager()->restore_determined = true;
         get_manager()->restore_federate   = true;
      }

      // Wait for a little while to give the sync-points time to come in.
      if ( !late_joiner_determined && !get_manager()->restore_determined ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         (void)sleep_timer.sleep();

         if ( !late_joiner_determined && !get_manager()->restore_determined ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():" << __LINE__
                         << " Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!"
                         << THLA_ENDL;
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               send_hs( stdout, "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():%d Waiting...%c",
                        __LINE__, THLA_NEWLINE );
            }
         }
      }
   }

   if ( late_joiner_determined ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():%d Late Joining Federate:%s%c",
                  __LINE__, ( this->is_late_joiner ? "Yes" : "No" ), THLA_NEWLINE );
      }
      return FEDERATE_JOIN_LATE;

   } else if ( get_manager()->restore_determined ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():%d Restoring the Federate!%c",
                  __LINE__, THLA_NEWLINE );
      }
      return FEDERATE_JOIN_RESTORING;

   } else {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():"
             << __LINE__ << " failed to determine if late joiner or restore federate!!!"
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return FEDERATE_JOIN_NOMINAL;
}

/*!
@details This routine implements the IMSim post multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::post_multi_phase_init_process()
{

   if ( this->is_master() ) {
      // Let the late joining federates know that we have completed initialization.
      federate->register_generic_sync_point( IMSim::INIT_COMPLETE_SYNC_POINT );
   }

   // When we join the federation, setup the list of current federates.
   // When a federate joins / resigns, this list will be automatically
   // updated by each federate.
   federate->load_and_print_running_federate_names();

   // Setup HLA time management.
   federate->setup_time_management();

   if ( this->get_manager()->is_late_joining_federate() ) {
      // Jump to the GALT time, otherwise we will not be in sync with the
      // other federates on the HLA logical timeline.
      federate->time_advance_request_to_GALT();
   } else {
      // Achieve the "startup" sync-point and wait for the federation
      // to be synchronized on it.
      federate->achieve_and_wait_for_synchronization( IMSim::STARTUP_SYNC_POINT );
   }
}

/*!
 * @job_class{shutdown}
 */
void ExecutionControl::shutdown()
{
   return;
}

/*!
 * @details This routine is used to perform and inline build of the Trick
 * ref ATTRIBUTES for the IMSim ExcutionControl.
 *
 * @job_class{initialization}
 */
void ExecutionControl::setup_object_ref_attributes()
{
   return;
}

/*!
 * @details This routine is used to perform and inline build of the Trick
 * ref ATTRIBUTES for the IMSim freeze interaction.
 * This is used by federates, other than the Master, to request mode
 * transitions.
 *
 * @job_class{initialization}
 */
void ExecutionControl::setup_interaction_ref_attributes()
{
   // setup FreezeInteraction and its FreezeInteractionHandler
   freeze_inter_count = 1;
   freeze_interaction = reinterpret_cast< Interaction * >(
      alloc_type( freeze_inter_count, "TrickHLA::Interaction" ) );
   if ( freeze_interaction == static_cast< Interaction * >( NULL ) ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for Interaction specialized"
             << " to FREEZE the sim!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   FreezeInteractionHandler *fiHandler =
      reinterpret_cast< FreezeInteractionHandler * >(
         alloc_type( 1, "TrickHLA::FreezeInteractionHandler" ) );
   if ( fiHandler == static_cast< FreezeInteractionHandler * >( NULL ) ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for FreezeInteractionHandler!"
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   freeze_interaction->set_handler( fiHandler );
   freeze_interaction->set_FOM_name( (char *)"Freeze" );
   // pass the debug flag into the interaction object so that we see its messages
   // in case the user turns our messages on...
   freeze_interaction->set_publish();
   freeze_interaction->set_subscribe();
   freeze_interaction->set_parameter_count( 1 );
   Parameter *tParm = reinterpret_cast< Parameter * >(
      alloc_type( freeze_interaction->get_parameter_count(),
                  "TrickHLA::Parameter" ) );
   if ( tParm == static_cast< Parameter * >( NULL ) ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the parameters of the"
             << " FREEZE interaction!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      tParm[0].set_FOM_name( "time" );
      tParm[0].set_encoding( ENCODING_LOGICAL_TIME );

      freeze_interaction->set_parameters( tParm );
   }

   // since this is an interaction handler generated on the fly, there is no
   // trick variable to resolve to at run time, which is supplied by the
   // input file. we must build data structures with sufficient information
   // for the Parameter class to link itself into the just generated
   // Freeze Interaction Handler, and its sole parameter ('time').

   // allocate the trick ATTRIBUTES data structure with room for two
   // entries: 1) the 'time' parameter and 2) an empty entry marking the end
   // of the structure.
   ATTRIBUTES *time_attr;
   time_attr = (ATTRIBUTES *)malloc( 2 * sizeof( ATTRIBUTES ) );
   if ( time_attr == static_cast< ATTRIBUTES * >( NULL ) ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the ATTRIBUTES for the"
             << " 'time' value of the FREEZE interaction!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // find the 'time' value in the Freeze Interaction Handler's ATTRIBUTES.
   // since we may not know the total # of elements, we look for an empty
   // element as an ending marker of the ATTRIBUTES.
   int attr_index = 0;

   // loop until the current ATTRIBUTES name is a NULL string
   while ( strcmp( attrTrickHLA__FreezeInteractionHandler[attr_index].name, "" ) != 0 ) {
      if ( strcmp( attrTrickHLA__FreezeInteractionHandler[attr_index].name, "time" ) == 0 ) {
         memcpy( &time_attr[0],
                 &attrTrickHLA__FreezeInteractionHandler[attr_index],
                 sizeof( ATTRIBUTES ) );
      }
      attr_index++;
   }

   // now that we have hit the end of the ATTRIBUTES array, copy the last
   // entry into my time_attr array to make it a valid ATTRIBUTE array.
   memcpy( &time_attr[1],
           &attrTrickHLA__FreezeInteractionHandler[attr_index],
           sizeof( ATTRIBUTES ) );

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg2;
      msg2 << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__ << endl
           << "--------------- Trick REF-Attributes ---------------" << endl
           << " FOM-Interaction:'" << freeze_interaction->get_FOM_name() << "'"
           << THLA_NEWLINE;
      send_hs( stdout, (char *)msg2.str().c_str() );
   }

   // Initialize the TrickHLA Interaction before we use it.
   freeze_interaction->initialize( this->get_manager() );

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg2;
      msg2 << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
           << " FOM-Parameter:'" << tParm[0].get_FOM_name() << "'"
           << " NOTE: This is an auto-generated parameter so there is no"
           << " associated 'Trick-Name'." << THLA_NEWLINE;
      send_hs( stdout, (char *)msg2.str().c_str() );
   }

   // Initialize the TrickHLA Parameter. Since we built the interaction handler
   // in-line, and not via the trick input file, use the alternate version of
   // the initialize routine which does not resolve the fully-qualified trick
   // name to access the ATTRIBUTES if the trick variable...
   if ( tParm != static_cast< Parameter * >( NULL ) ) {
      tParm[0].initialize( freeze_interaction->get_FOM_name(),
                           (void *)fiHandler->get_address_of_interaction_time(),
                           (ATTRIBUTES *)time_attr );
   }
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
   this->add_sync_point( IMSim::STARTUP_SYNC_POINT );
   this->add_sync_point( IMSim::INITIALIZE_SYNC_POINT );
   this->add_sync_point( IMSim::SIM_CONFIG_SYNC_POINT );
}

void ExecutionControl::announce_sync_point(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   wstring const                    &label,
   RTI1516_USERDATA const           &user_supplied_tag )
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
         // If we have a user supplied tag it should hold the pause time.
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
            pauseTime->set( 0.0 );
         }
      } catch ( CouldNotDecode &e ) {
         if ( pauseTime == NULL ) {
            pauseTime = new Int64Time( 0.0 );
         } else {
            pauseTime->set( 0.0 );
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::announce_sync_point():%d IMSim Pause Sync-Point:'%ls' Pause-time:%g %c",
                  __LINE__, label.c_str(), pauseTime->get_time_in_seconds(), THLA_NEWLINE );
      }
      this->add_pause( pauseTime, label );

   } else if ( label.compare( IMSim::INIT_COMPLETE_SYNC_POINT ) == 0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::announce_sync_point():%d IMSim Sync-Point '%ls' Exists %c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
      }
      this->get_manager()->get_execution_control()->init_complete_sp_exists = true;

   } else if ( this->contains( label ) ) {
      // Mark init sync-point as existing/announced.
      if ( this->mark_announced( label ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            send_hs( stdout, "IMSim::ExecutionControl::announce_sync_point():%d IMSim Multiphase Init Sync-Point:'%ls'%c",
                     __LINE__, label.c_str(), THLA_NEWLINE );
         }
      }

   } else if ( ( label.compare( IMSim::FEDSAVE_SYNC_POINT ) == 0 )
               || ( label.compare( IMSim::FEDRUN_SYNC_POINT ) == 0 ) ) {
      // TODO: For now these sync-points are treated like a pause sync-point
      // until we refactor the generic sync-points to not use a time.

      // Process the sync-point as a requested pause.
      Int64Time *pauseTime = new Int64Time( 0.0 );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         if ( label.compare( IMSim::FEDRUN_SYNC_POINT ) == 0 ) {
            send_hs( stdout, "IMSim::ExecutionControl::announce_sync_point():%d IMSim Go to Run Sync-Point:'%ls'%c",
                     __LINE__, label.c_str(), THLA_NEWLINE );
         } else {
            send_hs( stdout, "IMSim::ExecutionControl::announce_sync_point():%d IMSim Pause Sync-Point:'%ls' Pause-time:%g %c",
                     __LINE__, label.c_str(), pauseTime->get_time_in_seconds(), THLA_NEWLINE );
         }
      }
      this->add_pause( pauseTime, label );

   } // By default, mark an unrecognized synchronization point as achieved.
   else {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::announce_sync_point():%d Unrecognized synchronization point:'%ls', which will be achieved.%c",
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
           && ( sp->label.compare( IMSim::STARTUP_SYNC_POINT ) != 0 )
           && ( sp->label.compare( IMSim::INITIALIZE_SYNC_POINT ) != 0 )
           && ( sp->label.compare( IMSim::SIM_CONFIG_SYNC_POINT ) != 0 ) ) {
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
           && ( sp->label.compare( IMSim::STARTUP_SYNC_POINT ) != 0 )
           && ( sp->label.compare( IMSim::INITIALIZE_SYNC_POINT ) != 0 )
           && ( sp->label.compare( IMSim::SIM_CONFIG_SYNC_POINT ) != 0 ) ) {

         int64_t      wallclock_time;
         SleepTimeout print_timer( federate->wait_status_time );
         SleepTimeout sleep_timer;

         // Wait for the federation to synchronized on the sync-point.
         while ( !sp->is_achieved() ) {

            // Always check to see is a shutdown was received.
            federate->check_for_shutdown_with_termination();

            // Pause and release the processor for short sleep value.
            (void)sleep_timer.sleep();

            // Periodically check to make sure the federate is still part of
            // the federation execution.
            if ( !sp->is_achieved() ) {

               // To be more efficient, we get the time once and share it.
               wallclock_time = sleep_timer.time();

               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "IMSim::ExecutionControl::wait_for_all_multiphase_init_sync_points():" << __LINE__
                            << " Unexpectedly the Federate is no longer an execution"
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
                  send_hs( stdout, "ExecutionControl::wait_for_all_multiphase_init_sync_points():%d Waiting...%c",
                           __LINE__, THLA_NEWLINE );
               }
            }
         }
      }
   }
}

void ExecutionControl::publish()
{
   // Publish the freeze_interactions.
   for ( int n = 0; n < freeze_inter_count; ++n ) {
      freeze_interaction[n].publish_interaction();
   }
}

void ExecutionControl::unpublish()
{
   bool do_unpublish;

   if ( this->is_master() ) {
      // Unpublish the execution configuration if we are the master federate.
      execution_configuration->unpublish_all_object_attributes();
   }

   // Unpublish all the freeze_interactions.
   for ( int i = 0; i < freeze_inter_count; ++i ) {

      // Only unpublish a FREEZE interaction that we publish.
      if ( freeze_interaction[i].is_publish() ) {

         do_unpublish = true;
         for ( int k = 0; ( k < i ) && do_unpublish; ++k ) {
            // Unpublish an interaction Class only once, so see if we have
            // already unpublished the same interaction class that was published.
            if ( freeze_interaction[k].is_publish()
                 && ( freeze_interaction[i].get_class_handle()
                      == freeze_interaction[k].get_class_handle() ) ) {
               do_unpublish = false;
            }
         }
         if ( do_unpublish ) {
            freeze_interaction[i].unpublish_interaction();
         }
      }
   }
}

void ExecutionControl::subscribe()
{
   // Check to see if we are the Master federate.
   if ( this->is_master() ) {
      // Only subscribe to the Freeze interactions if this is the Master federate.
      for ( int n = 0; n < freeze_inter_count; ++n ) {
         freeze_interaction[n].subscribe_to_interaction();
      }
   } else {
      // Subscribe to the execution configuration if we are not the master federate.
      execution_configuration->subscribe_to_object_attributes();
   }
}

void ExecutionControl::unsubscribe()
{
   bool do_unsubscribe;

   // Check to see if we are the Master federate.
   if ( this->is_master() ) {
      // Unsubscribe from the execution configuration if we are NOT the Master federate.
      execution_configuration->unsubscribe_all_object_attributes();
   }

   // Unsubscribe the mtr_interactions.
   // Only unsubscribe an MTR interaction that we subscribe.
   // Unsubscribe from all the freeze_interactions.
   for ( int i = 0; i < freeze_inter_count; ++i ) {
      // Only unsubscribe from FREEZE interactions that are subscribed to.
      if ( freeze_interaction[i].is_subscribe() ) {
         do_unsubscribe = true;
         for ( int k = 0; ( k < i ) && do_unsubscribe; ++k ) {
            // Unsubscribe from an interaction Class only once, so see if
            // we have already unsubscribed from the same interaction class
            // that was subscribed to.
            if ( freeze_interaction[k].is_subscribe()
                 && ( freeze_interaction[i].get_class_handle()
                      == freeze_interaction[k].get_class_handle() ) ) {
               do_unsubscribe = false;
            }
         }
         if ( do_unsubscribe ) {
            freeze_interaction[i].unsubscribe_from_interaction();
         }
      }
   }
}

bool ExecutionControl::mark_synchronized( std::wstring const &label )
{

   // FIXME: Is this really necessary.
   clear_pause( label );

   size_t found;
   found = label.find( IMSim::FEDSAVE_SYNC_POINT );
   if ( found != wstring::npos ) {
      // If this is the federate which is to initiate the federation save,
      // set the flag which will signal perform_checkpoint() to do so...
      if ( federate->announce_save ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            send_hs( stdout, "IMSim::ExecutionControl::mark_synchronized():%d '%ls' Synchronization Point, setting initiate_save_flag!%c",
                     __LINE__, label.c_str(), THLA_NEWLINE );
         }
         federate->initiate_save_flag = true;
      }
   }
   found = label.find( IMSim::FEDRUN_SYNC_POINT );
   if ( found != wstring::npos ) {
      // DANNY2.7 We've been in freeze because master clicked Freeze;
      // this sync point indicates we clicked Run
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::mark_synchronized():%d '%ls' UNFREEZING!%c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
      }
      federate->un_freeze();
   }
   if ( this->get_sim_time() <= 0.0 ) {
      // DANNY.2.7 coming out of freeze at init time
      federate->un_freeze();
   }

   return ( false );
}

/*!
 * @job_class{scheduled}
 */
void ExecutionControl::receive_interaction(
   RTI1516_NAMESPACE::InteractionClassHandle const  &theInteraction,
   RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
   RTI1516_USERDATA const                           &theUserSuppliedTag,
   RTI1516_NAMESPACE::LogicalTime const             &theTime,
   bool const                                        received_as_TSO )
{

   // Find the TrickHLAFreezeInteraction we have data for.
   for ( int i = 0; i < freeze_inter_count; ++i ) {
      // Process the FREEZE interaction if we subscribed to it and we have the
      // same class handle.
      if ( freeze_interaction[i].is_subscribe() && ( freeze_interaction[i].get_class_handle() == theInteraction ) ) {

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            if ( received_as_TSO ) {
               Int64Time _time;
               _time.set( theTime );

               string handle;
               StringUtilities::to_string( handle, theInteraction );
               send_hs( stdout, "Manager::receive_interaction(FREEZE):%d ID:%s, HLA-time:%G%c",
                        __LINE__, handle.c_str(), _time.get_time_in_seconds(),
                        THLA_NEWLINE );
            } else {
               string handle;
               StringUtilities::to_string( handle, theInteraction );
               send_hs( stdout, "Manager::receive_interaction(FREEZE):%d ID:%s%c",
                        __LINE__, handle.c_str(), THLA_NEWLINE );
            }
         }

         if ( received_as_TSO ) {
            InteractionItem item( i,
                                  TRICKHLA_MANAGER_BUILTIN_FREEZE_INTERACTION,
                                  freeze_interaction[i].get_parameter_count(),
                                  freeze_interaction[i].get_parameters(),
                                  theParameterValues,
                                  theUserSuppliedTag,
                                  theTime );

            freeze_interaction[i].extract_data( &item );
            freeze_interaction[i].process_interaction();
         } else {
            InteractionItem item( i,
                                  TRICKHLA_MANAGER_BUILTIN_FREEZE_INTERACTION,
                                  freeze_interaction[i].get_parameter_count(),
                                  freeze_interaction[i].get_parameters(),
                                  theParameterValues,
                                  theUserSuppliedTag );

            freeze_interaction[i].extract_data( &item );
            freeze_interaction[i].process_interaction();
         }

         // Return now that we put the interaction-item into the queue.
         return;
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
   // Reference the IMSim Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // This should only be called by the Master federate.
   if ( !this->is_master() ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::set_next_execution_mode():" << __LINE__
             << " This should only be called by the Master federate!" << THLA_ENDL;
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
            errmsg << "IMSim::ExecutionControl::set_next_execution_mode():"
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
      errmsg << "IMSim::ExecutionControl::check_mode_transition_request():"
             << __LINE__ << " WARNING: Received Mode Transition Request and not Master: "
             << mtr_enum_to_string( this->pending_mtr )
             << THLA_ENDL;
      send_hs( stdout, (char *)errmsg.str().c_str() );
      return false;
   }

   // First check to see if this is a valid MTR.
   if ( !is_mtr_valid( this->pending_mtr ) ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::check_mode_transition_request():"
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

   // Reference the IMSim Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = this->get_execution_configuration();

   // Print diagnostic message if appropriate.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      cout << "=============================================================" << endl
           << "ExecutionControl::process_mode_transition_request()" << endl
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
         // The IMSim ExecutionControl shutdown transition will be made from
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

   // Reference the IMSim Execution Configuration Object (ExCO)
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
      errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
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
      errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
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
         errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
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
            // The IMSim ExecutionControl shutdown transition will be made from
            // the TrickHLA::Federate::shutdown() job.
            the_exec->stop();

         } else {

            errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
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
            // The IMSim ExecutionControl shutdown transition will be made from
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
            // the_exec->freeze();

            // Tell Trick to go into freeze at startup.
            the_exec->set_freeze_command( true );

            // The freeze transition logic will be done just before entering
            // Freeze. This is done in the TrickHLA::Federate::freeze_init()
            // routine called when entering Freeze.

         } else if ( this->requested_execution_control_mode == EXECUTION_CONTROL_INITIALIZING ) {

            // There's really nothing to do here.

         } else {

            errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
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
            errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
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
            // The IMSim ExecutionControl shutdown transition will be made from
            // the TrickHLA::Federate::shutdown() job.
            the_exec->stop();
         } else if ( this->requested_execution_control_mode == EXECUTION_CONTROL_FREEZE ) {

            // Print diagnostic message if appropriate.
            if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               cout << "ExecutionControl::process_execution_control_updates():" << __LINE__ << endl
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

            errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
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

            errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
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
         errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
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
   RTIambassador          *RTI_amb  = federate->get_RTI_ambassador();
   ExecutionConfiguration *ExCO     = get_execution_configuration();
   SyncPnt                *sync_pnt = NULL;

   // Register the 'mtr_run' sync-point.
   if ( this->is_master() ) {
      sync_pnt = this->register_sync_point( *RTI_amb, MTR_RUN_SYNC_POINT );
   } else {
      sync_pnt = this->get_sync_point( MTR_RUN_SYNC_POINT );
   }

   // Make sure that we have a valid sync-point.
   if ( sync_pnt == (TrickHLA::SyncPnt *)NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::run_mode_transition():" << __LINE__
             << " The 'mtr_run' sync-point was not found!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      // Wait for 'mtr_run' sync-point announce.
      this->wait_for_sync_point_announcement( federate, sync_pnt );

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
                  send_hs( stdout, "IMSim::ExecutionControl::run_mode_transition():%d Going to run in %G seconds.%c",
                           __LINE__, diff, THLA_NEWLINE );
               }
            }
         }

         // Print debug message if appropriate.
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            double curr_cte_time = this->get_cte_time();
            diff                 = curr_cte_time - go_to_run_time;
            send_hs( stdout, "IMSim::ExecutionControl::run_mode_transition():%d \n  Going to run at CTE time %.18G seconds. \n  Current CTE time %.18G seconds. \n  Difference: %.9lf seconds.%c",
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
   RTIambassador          *RTI_amb  = federate->get_RTI_ambassador();
   ExecutionConfiguration *ExCO     = get_execution_configuration();
   TrickHLA::SyncPnt      *sync_pnt = NULL;

   // Get the 'mtr_freeze' sync-point.
   sync_pnt = this->get_sync_point( MTR_FREEZE_SYNC_POINT );

   // Make sure that we have a valid sync-point.
   if ( sync_pnt == (TrickHLA::SyncPnt *)NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::freeze_mode_transition():" << __LINE__
             << " The 'mtr_freeze' sync-point was not found!" << THLA_ENDL;
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

   // Only the Master federate has any IMSim tasks for shutdown.
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

void ExecutionControl::enter_freeze()
{
   // DANNY2.7 send a freeze interaction when master hits Sim Control Panel
   //  Freeze button. Determine if I am the federate that clicked Freeze
   if ( this->get_sim_time() <= 0.0 ) {
      federate->announce_freeze = this->is_master();
   } else if ( !federate->freeze_the_federation ) {
      federate->announce_freeze = true;
   }

   if ( federate->announce_freeze ) {
      // Send interaction unless: we are here because we are at the freeze
      // interaction time, or we are here because we started in freeze
      if ( ( !federate->freeze_the_federation ) && ( this->get_sim_time() > 0.0 ) ) {
         double freeze_scenario_time = -DBL_MAX; // freeze immediately

         if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            send_hs( stdout,
                     "IMSim::ExecutionControl::enter_freeze():%d announce_freeze:%s, freeze_federation:%s, freeze_scenario_time:%g %c",
                     __LINE__, ( federate->announce_freeze ? "Yes" : "No" ),
                     ( federate->freeze_the_federation ? "Yes" : "No" ),
                     freeze_scenario_time, THLA_NEWLINE );
         }

         this->trigger_freeze_interaction( freeze_scenario_time );
         federate->un_freeze(); // will freeze again for real when we hit the freeze interaction time
      }
   }
}

bool ExecutionControl::check_freeze_exit()
{
   // If freeze has been announced and we are not in initialization then return true.
   if ( federate->announce_freeze && ( this->get_sim_time() <= 0.0 ) ) {
      return ( true );
   }

   try {
      pause_sync_pts.achieve_all_sync_points( *( federate->get_RTI_ambassador() ), checktime );
   } catch ( SynchronizationPointLabelNotAnnounced &e ) {
      send_hs( stderr, "IMSim::ExecutionControl::check_freeze():%d EXCEPTION: SynchronizationPointLabelNotAnnounced%c",
               __LINE__, THLA_NEWLINE );
   } catch ( FederateNotExecutionMember &e ) {
      send_hs( stderr, "IMSim::ExecutionControl::check_freeze():%d EXCEPTION: FederateNotExecutionMember%c",
               __LINE__, THLA_NEWLINE );
   } catch ( SaveInProgress &e ) {
      send_hs( stderr, "IMSim::ExecutionControl::check_freeze():%d EXCEPTION: SaveInProgress%c",
               __LINE__, THLA_NEWLINE );
   } catch ( RestoreInProgress &e ) {
      send_hs( stderr, "IMSim::ExecutionControl::check_freeze():%d EXCEPTION: RestoreInProgress%c",
               __LINE__, THLA_NEWLINE );
   } catch ( RTIinternalError &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "IMSim::ExecutionControl::check_freeze():%d EXCEPTION: RTIinternalError: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   }

   pause_sync_pts.check_state();

   if ( pause_sync_pts.should_exit() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::check_freeze():%d SHUTDOWN NOW!%c",
                  __LINE__, THLA_NEWLINE );
      }
      federate->set_restart( false );
      federate->set_restart_cfg( false );

      exec_set_exec_command( ExitCmd );
   } else if ( pause_sync_pts.should_restart() ) {
      federate->set_restart( true );
      exec_set_exec_command( ExitCmd );
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::check_freeze():%d RESTART NOW!%c",
                  __LINE__, THLA_NEWLINE );
      }
   } else if ( pause_sync_pts.should_reconfig() ) {
      federate->set_restart_cfg( true );
      exec_set_exec_command( ExitCmd );
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::check_freeze():%d RESTART RECONFIG NOW!%c",
                  __LINE__, THLA_NEWLINE );
      }
   } else if ( pause_sync_pts.should_run() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::check_freeze():%d DOING UNFREEZE NOW!%c",
                  __LINE__, THLA_NEWLINE );
      }
      federate->un_freeze();
   }

   return ( false );
}

void ExecutionControl::exit_freeze()
{
   if ( federate->announce_freeze ) {                                            // DANNY2.7
      if ( federate->freeze_the_federation && ( this->get_sim_time() > 0.0 ) ) { // coming out of freeze due to freeze interaction
         federate->register_generic_sync_point( IMSim::FEDRUN_SYNC_POINT );      // this tells federates to go to run

         int64_t      wallclock_time;
         SleepTimeout print_timer( federate->wait_status_time );
         SleepTimeout sleep_timer;

         while ( !this->pause_sync_pts.check_sync_points( this->checktime ) ) {
            // wait for it to be announced
            (void)sleep_timer.sleep();

            if ( !this->pause_sync_pts.check_sync_points( this->checktime ) ) {

               // To be more efficient, we get the time once and share it.
               wallclock_time = sleep_timer.time();

               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "IMSim::ExecutionControl::exit_freeze():" << __LINE__
                            << " Unexpectedly the Federate is no longer an execution"
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
                  send_hs( stderr, "IMSim::ExecutionControl::exit_freeze():%d Waiting...%c",
                           __LINE__, THLA_NEWLINE );
               }
            }
         }
      }
      if ( federate->freeze_the_federation ) { // coming out of freeze due to interaction OR sync point at init time
         federate->announce_freeze = false;    // reset for the next time we freeze
      }
      try {
         this->pause_sync_pts.achieve_all_sync_points( *federate->get_RTI_ambassador(), this->checktime );
      } catch ( SynchronizationPointLabelNotAnnounced &e ) {
         send_hs( stderr, "IMSim::ExecutionControl::exit_freeze():%d EXCEPTION: SynchronizationPointLabelNotAnnounced%c",
                  __LINE__, THLA_NEWLINE );
      } catch ( FederateNotExecutionMember &e ) {
         send_hs( stderr, "IMSim::ExecutionControl::exit_freeze():%d EXCEPTION: FederateNotExecutionMember%c",
                  __LINE__, THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         send_hs( stderr, "IMSim::ExecutionControl::exit_freeze():%d EXCEPTION: SaveInProgress%c",
                  __LINE__, THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         send_hs( stderr, "IMSim::ExecutionControl::exit_freeze():%d EXCEPTION: RestoreInProgress%c",
                  __LINE__, THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "IMSim::ExecutionControl::exit_freeze():%d EXCEPTION: RTIinternalError: '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      }
   }
}

void ExecutionControl::un_freeze()
{
   // Clear the pause sync-point master state so that we don't accidently
   // go into the run state again.
   pause_sync_pts.clear_state();
}

void ExecutionControl::check_pause( double const check_pause_delta )
{
   // DANNY2.7 for IMSim, check_pause is only used at init time to handle start
   //  in freeze mode.
   if ( this->get_sim_time() > 0.0 ) {
      return;
   }

   this->checktime.set( this->get_sim_time() + check_pause_delta );

   if ( this->pause_sync_pts.check_sync_points( this->checktime ) ) {
      federate->freeze_the_federation = true;
   } else if ( this->get_manager()->is_late_joining_federate() ) {
      // check if the requested time has a sync-point.
      this->checktime = federate->requested_time;
      if ( this->pause_sync_pts.check_sync_points( this->checktime ) ) {
         federate->freeze_the_federation = true;
      }
   }

   if ( federate->freeze_the_federation ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::check_pause():%d Commanding Trick Executive to FREEZE.%c",
                  __LINE__, THLA_NEWLINE );
      }
      if ( this->get_sim_time() <= 0.0 ) {
         exec_set_freeze_command( true );
      } else {
         exec_freeze();
      }
   }
}

// FIXME: See if this is still needed. Trick 17 may have fixed this.
/*!
 *  @details Note that we could just have one check_pause routine and 2
 *  instances of it in the S_define file (one would be an initialization job
 *  and one would be a logging job). But early Trick 10 versions cannot
 *  distinguish between multiple instances when setting job cycle, so having
 *  this check_pause_at_init routine solves that problem.
 *  @job_class{initialization}
 */
void ExecutionControl::check_pause_at_init(
   double const check_pause_delta )
{
   // Dispatch to the ExecutionControl method.
   this->get_manager()->get_execution_control()->check_pause_at_init( check_pause_delta );
}

void ExecutionControl::add_pause(
   Int64Time     *time,
   wstring const &label )
{
   pause_sync_pts.add_sync_point( label, *time );
}

void ExecutionControl::clear_pause(
   wstring const &label )
{
   pause_sync_pts.clear_sync_point( label );
}

ExecutionConfiguration *ExecutionControl::get_execution_configuration()
{
   ExecutionConfiguration *ExCO;

   ExCO = dynamic_cast< ExecutionConfiguration * >( this->get_execution_configuration() );
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::epoch_and_root_frame_discovery_process():" << __LINE__
             << " ERROR: Execution Configureation is not an IMSim ExCO." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return ( this->get_execution_configuration() );
}

void ExecutionControl::start_federation_save_at_scenario_time(
   double      freeze_scenario_time,
   char const *file_name )
{

   if ( freeze_interaction->get_handler() != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::start_federation_save_at_scenario_time(%g, '%s'):%d%c",
                  freeze_scenario_time, file_name, __LINE__, THLA_NEWLINE );
      }
      federate->set_announce_save();

      double new_scenario_time = freeze_scenario_time;

      trigger_freeze_interaction( new_scenario_time );

      get_manager()->initiate_federation_save( file_name );
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "IMSim::ExecutionControl::start_federation_save_at_scenario_time(%g, '%s'):%d \
freeze_interaction's HANLDER is NULL! Request was ignored!%c",
                  freeze_scenario_time, file_name, __LINE__, THLA_NEWLINE );
      }
   }
}

void ExecutionControl::add_freeze_scenario_time(
   double t )
{
   if ( this->get_manager()->is_late_joining_federate() ) {

      if ( federate->announce_save ) {
         freeze_scenario_times.insert( t );
      } else {
         // If we received the interaction, save on the current frame.
         freeze_scenario_times.insert( this->get_scenario_time() );
      }
   } else {
      freeze_scenario_times.insert( t );
   }
}

void ExecutionControl::trigger_freeze_interaction(
   double &freeze_scenario_time )
{

   double new_freeze_time = freeze_scenario_time;

   FreezeInteractionHandler *freeze_intr =
      static_cast< FreezeInteractionHandler * >(
         freeze_interaction->get_handler() );

   freeze_intr->send_scenario_freeze_interaction( new_freeze_time, this->is_late_joiner() );

   // Save the new time into the passed-in value...
   freeze_scenario_time = new_freeze_time;
}

/*!
 *  @details If found, clears the element, registers the FEDSAVE_v2 sync point
 *  with the RTI if we are the master federate and returns true. Otherwise,
 *  when a freeze interaction time was not found, false is returned.
 */
bool ExecutionControl::check_freeze_time()
{

   bool do_immediate_freeze = check_scenario_freeze_time();

   if ( do_immediate_freeze ) {
      // DANNY2.7 Go to FREEZE at top of next frame.
      exec_freeze(); // go to freeze at top of next frame (other federates MUST have their software frame set in input file!)
      // If we are to initiate the federation save, register a sync point
      // which must be acknowledged only in freeze mode!!!
      if ( federate->announce_save ) {
         federate->register_generic_sync_point( IMSim::FEDSAVE_SYNC_POINT );
         federate->announce_freeze = true;
      }
   }
   return do_immediate_freeze;
}

bool ExecutionControl::check_scenario_freeze_time()
{
   bool do_immediate_freeze = false;
   bool found_valid_freeze_time;

   do {
      found_valid_freeze_time = false;

      if ( !freeze_scenario_times.empty() ) {
         FreezeTimeSet::const_iterator iter;
         iter               = freeze_scenario_times.begin();
         double freeze_time = *iter;

         // We need to find the equivalent simulation-time and HLA-time for a
         // given freeze scenario-time so that we can do the correct time
         // comparisons. Also, if we are a late joining federate the sim-time
         // and HLA-time will not be aligned as shown in this example.
         //
         //      HLA-time |-------------------------|-------------------------|
         //               101.0                     102.0                     103.0
         //
         // Scenario-time |-------------------------|-------------------------|
         //               March 2, 2032 @ 19:20:07  March 2, 2032 @ 19:20:08  March 2, 2032 @ 19:20:09
         //
         //      Sim-time |-------------------------|-------------------------|
         //               0.0                       1.0                       2.0
         // Scenario-time and Sim-time change at the same rate but they have
         // different starting epochs.
         // freeze-sim-time = current-sim-time + (freeze-scenario-time - current-scenario-time)
         // freeze-hla-time = granted-hla-time + (freeze-scenario-time - current-scenario-time)

         // Get the current Trick sim-time.
         double curr_sim_time = this->get_sim_time();

         // Get the current scenario-time.
         double curr_scenario_time = this->get_scenario_time();

         // Determine the freeze simulation-time for the equivalent freeze
         // scenario-time.
         double freeze_sim_time = curr_sim_time + ( freeze_time - curr_scenario_time );

         // Jump to Trick Freeze mode if the current scenario time is greater
         // than or equal to the requested freeze scenario time.
         if ( curr_scenario_time >= freeze_time ) {
            found_valid_freeze_time = true;
            do_immediate_freeze     = true;
            freeze_scenario_times.erase( iter );
            federate->freeze_the_federation = true;

            if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               ostringstream infomsg;
               infomsg << "IMSim::ExecutionControl::check_scenario_freeze_time():" << __LINE__
                       << " Going to Trick FREEZE mode immediately:" << endl;
               if ( federate->time_management ) {
                  infomsg << "  Granted HLA-time:" << federate->granted_time.get_time_in_seconds() << endl;
               }
               infomsg << "  Trick sim-time:" << curr_sim_time << endl
                       << "  Freeze sim-time:" << freeze_sim_time << endl
                       << "  Current scenario-time:" << curr_scenario_time << endl
                       << "  Freeze scenario-time:" << freeze_time << THLA_ENDL;
               send_hs( stdout, (char *)infomsg.str().c_str() );
            }
         }
      }
   } while ( found_valid_freeze_time );

   return do_immediate_freeze;
}

/*!
 *  @details This routine will block on the FEDSAVE_SYNC_POINT synchronization
 *  point until it is achieved and Save is initiated.
 */
bool ExecutionControl::is_save_initiated()
{
   // When user calls start_federation_save, initiate_save_flag is
   // set in federation_synchronized when feds sync to FEDSAVE_v2 sync point
   // if it's not set, we are here because Dump Chkpnt was clicked, so we
   // need to register sync point
   if ( federate->announce_save && !federate->initiate_save_flag && !federate->save_completed ) {
      federate->register_generic_sync_point( IMSim::FEDSAVE_SYNC_POINT );

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer;

      while ( !federate->initiate_save_flag ) { // wait for federation to be synced
         this->pause_sync_pts.achieve_all_sync_points( *federate->get_RTI_ambassador(), this->checktime );
         (void)sleep_timer.sleep();

         if ( !federate->initiate_save_flag ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "IMSim::ExecutionControl::setup_checkpoint():" << __LINE__
                         << " Unexpectedly the Federate is no longer an execution"
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
               send_hs( stdout, "IMSim::ExecutionControl::setup_checkpoint():%d Waiting '%s'%c",
                        __LINE__, THLA_NEWLINE );
            }
         }
      }
   }
   return ( true );
}

bool ExecutionControl::perform_save()
{
   if ( federate->announce_save && federate->initiate_save_flag && !federate->start_to_save ) {
      // We are here because user called start_federation_save, so
      // must force the perform_checkpoint code to execute.
      federate->announce_save = false;
      return ( true );
   }

   return ( false );
}

/*!
 *  @job_class{checkpoint}
 */
void ExecutionControl::convert_loggable_sync_pts()
{

   if ( this->logged_sync_pts_count > 0 ) {
      this->loggable_sync_pts = reinterpret_cast< LoggableTimedSyncPnt * >(
         alloc_type( (int)this->logged_sync_pts_count, "TrickHLA::LoggableSyncPts" ) );
      if ( this->loggable_sync_pts == static_cast< LoggableTimedSyncPnt * >( NULL ) ) {
         ostringstream errmsg;
         errmsg << "IMSim::ExecutionControl::convert_sync_pts():" << __LINE__
                << " FAILED to allocate enough memory for the loggable sync points!"
                << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      pause_sync_pts.convert_sync_points( this->loggable_sync_pts );
   }
}

void ExecutionControl::reinstate_logged_sync_pts()
{
   if ( this->logged_sync_pts_count > 0 ) {
      Int64Time sync_point_time;
      wstring   sync_point_label;

      pause_sync_pts.reset();

      for ( size_t i = 0; i < this->logged_sync_pts_count; ++i ) {
         sync_point_time = Int64Time( this->loggable_sync_pts[i].time );

         if ( this->loggable_sync_pts[i].label != NULL ) {
            StringUtilities::to_wstring( sync_point_label, this->loggable_sync_pts[i].label );
            // since the RTI already contains the registered sync points prior
            // to the checkpoint, we just need to add them back into myself...
            // pause_sync_pts.add_sync_point( sync_point_label, sync_point_time );
            /***
            //DANNY2.7 TODO: you sometimes get an exception for sync point not announced when you restore,
            //         is that an RTI bug or something we can fix here?
            if (is_master() && (sync_point_time.get_time_in_seconds() <= get_sim_time()) ) {
                register_generic_sync_point( sync_point_label, sync_point_time.get_time_in_seconds());
            }
            ***/
         }
      }
      // set the checktime to the first of the new entries...
      sync_point_time = Int64Time( this->loggable_sync_pts[0].time );
      checktime.set( sync_point_time );
   }
}
