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
@trick_link_dependency{../TrickHLA/Federate.cpp}
@trick_link_dependency{../TrickHLA/Int64BaseTime.cpp}
@trick_link_dependency{../TrickHLA/Manager.cpp}
@trick_link_dependency{../TrickHLA/SleepTimeout.cpp}
@trick_link_dependency{../TrickHLA/SyncPoint.cpp}
@trick_link_dependency{../TrickHLA/SyncPointTimed.cpp}
@trick_link_dependency{../TrickHLA/SyncPointManagerBase.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{../TrickHLA/Utilities.cpp}
@trick_link_dependency{ExecutionConfiguration.cpp}
@trick_link_dependency{ExecutionControl.cpp}
@trick_link_dependency{FreezeInteractionHandler.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, IMSim support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2024, --, Refactor to use sync-point manager.}
@revs_end

*/

// System include files.
#include <cstdint>
#include <float.h>
#include <iomanip>
#include <limits>
#include <math.h>
#include <string>

// Trick includes.
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/InteractionItem.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPoint.hh"
#include "TrickHLA/SyncPointTimed.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

// IMSim include files.
#include "IMSim/ExecutionConfiguration.hh"
#include "IMSim/ExecutionControl.hh"
#include "IMSim/FreezeInteractionHandler.hh"
#include "IMSim/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA Encoder helper includes.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

// IMSim file level declarations.
namespace IMSim
{

// ExecutionControl type string.
std::string const ExecutionControl::type = "IMSim";

} // namespace IMSim

// Create the Trick ATTRIBUTES arrays needed for local allocations.
#ifdef __cplusplus
extern "C" {
#endif
// C based model includes.
extern ATTRIBUTES attrIMSim__FreezeInteractionHandler[];
#ifdef __cplusplus
}
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;
using namespace IMSim;

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl(
   IMSim::ExecutionConfiguration &imsim_config )
   : TrickHLA::ExecutionControlBase( imsim_config ),
     pending_mtr( IMSim::MTR_UNINITIALIZED ),
     freeze_inter_count( 0 ),
     freeze_interaction( NULL ),
     freeze_scenario_times(),
     scenario_time_epoch( 0.0 ),
     current_execution_mode( TrickHLA::EXECUTION_CONTROL_UNINITIALIZED ),
     next_execution_mode( TrickHLA::EXECUTION_CONTROL_UNINITIALIZED )
{
   // The next_mode_scenario_time time for the next federation execution mode
   // change expressed as a federation scenario time reference. Note: this is
   // value is only meaningful for going into freeze; exiting freeze is
   // coordinated through a sync point mechanism.
   // Inherited from ExecutionControlBase, and for IMSim the default is 0.0;
   this->next_mode_scenario_time = 0.0;

   // The time for the next federation execution mode change expressed as a
   // Central Timing Equipment (CTE) time reference. The standard for this
   // reference shall be defined in the federation agreement when CTE is used.
   // Inherited from ExecutionControlBase, and for IMSim the default is 0.0;
   this->next_mode_cte_time = 0.0;

   // A 64 bit integer time that represents the base time for the least common
   // value of all the time step values in the federation execution (LCTS).
   // This value is set by the Master Federate and does not change during the
   // federation execution. This is used in the computation to find the next
   // HLA Logical Time Boundary (HLTB) available to all federates in the
   // federation execution. The basic equation is
   //     HLTB = ( floor(GALT/LCTS) + 1 ) * LCTS,
   // where GALT is the greatest available logical time. This is used to
   // synchronize the federates in a federation execution to be on a common
   // logical time boundary.
   // Inherited from ExecutionControlBase, and for IMSim the default is 0;
   this->least_common_time_step         = 0;
   this->least_common_time_step_seconds = 0.0;
}

/*!
 * @job_class{shutdown}
 */
ExecutionControl::~ExecutionControl()
{
   clear_mode_values();

   // Free up the allocated Freeze Interaction.
   if ( freeze_interaction != NULL ) {
      if ( freeze_interaction->get_handler() != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( freeze_interaction->get_handler() ) ) ) {
            message_publish( MSG_WARNING, "IMSim::ExecutionControl::~ExecutionControl():%d WARNING failed to delete Trick Memory for 'freeze_interaction->get_handler()'\n",
                             __LINE__ );
         }
         freeze_interaction->set_handler( NULL );
      }
      if ( trick_MM->delete_var( static_cast< void * >( freeze_interaction ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionControl::~ExecutionControl():%d WARNING failed to delete Trick Memory for 'freeze_interaction'\n",
                          __LINE__ );
      }
      freeze_interaction = NULL;
      freeze_inter_count = 0;
   }
}

/*!
@details This routine will set a lot of the data in the TrickHLA::Federate that
is required for this execution control scheme. This should greatly simplify
input.py files and reduce input.py file setting errors.

@job_class{initialization}
*/
void ExecutionControl::initialize()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg;
      msg << "IMSim::ExecutionControl::initialize():" << __LINE__
          << " Initialization-Scheme:'" << get_type() << "'\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // FIXME: This is not consistent with the IMSim design document.
   // There are things that must me set for the IMSim initialization.
   // this->use_preset_master = true;
   this->use_preset_master = false;

   // FIXME: We won't know this until we try to create the federation execution.
   // If this is the Master federate, then it must support Time
   // Management and be both Time Regulating and Time Constrained.
   if ( is_master() ) {
      this->federate->time_management  = true;
      this->federate->time_regulating  = true;
      this->federate->time_constrained = true;

      // The software frame is set from the Least Common Time Step.
      // For the Master federate the Trick simulation software frame must
      // match the Least Common Time Step (LCTS).
      double software_frame_time = Int64BaseTime::to_seconds( this->least_common_time_step );
      exec_set_software_frame( software_frame_time );
   }

   // Make sure we initialize the base class.
   TrickHLA::ExecutionControlBase::initialize();

   // FIXME: This is not consistent with the IMSim design document.
   // Must use a preset master.
   /*
      if ( !is_master_preset() ) {
         ostringstream errmsg;
         errmsg << "IMSim::ExecutionControl::initialize():" << __LINE__
                << " WARNING: Only a preset master is supported. Make sure to set"
                << " 'THLA.federate.use_preset_master = true' in your input.py file."
                << " Setting use_preset_master to true!\n";
         message_publish( MSG_NORMAL, errmsg.str().c_str() );
         this->use_preset_master = true;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         if ( is_master() ) {
            message_publish( MSG_NORMAL, "IMSim::ExecutionControl::initialize():%d\n    I AM THE PRESET MASTER\n",
                     __LINE__ );
         } else {
            message_publish( MSG_NORMAL, "IMSim::ExecutionControl::initialize():%d\n    I AM NOT THE PRESET MASTER\n",
                     __LINE__ );
         }
      }
   */

   return;
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
      message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n",
                       __LINE__ );
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
   manager->setup_all_ref_attributes();

   // Add the IMSim initialization and user multiphase initialization
   // sync-points now that the ExecutionConfiguration has been initialized
   // in the call to the setup_all_ref_attributes() function. We do this
   // here so that we can handle the RTI callbacks that use them.
   add_initialization_sync_points();

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
   if ( !is_master_preset() ) {
      set_master( federate->is_federation_created_by_federate() );
      execution_configuration->set_master( is_master() );
   }

   // The Master federate must have a padding time set.
   if ( is_master() && ( get_time_padding() <= 0.0 ) ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
             << " ERROR: For this Master federate, the time padding ("
             << get_time_padding() << " seconds) must be greater than zero!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify the federate time constraints.
   if ( !federate->verify_time_constraints() ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
             << " ERROR: Time constraints verification failed!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Don't forget to enable asynchronous delivery of messages.
   federate->enable_async_delivery();

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      if ( is_master() ) {
         message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n    I AM THE MASTER\n",
                          __LINE__ );
      } else {
         message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n    I AM NOT THE MASTER\n",
                          __LINE__ );
      }
   }

   // Save restore_file_name before it gets wiped out with the loading of the checkpoint file...
   char *tRestoreName = NULL;
   if ( manager->restore_file_name != NULL ) {
      // we don't want this to get wiped out when trick clears memory for load checkpoint, so don't allocate with TMM
      tRestoreName = strdup( manager->restore_file_name );
   }

   // Initialize the MOM interface handles.
   federate->initialize_MOM_handles();

   if ( is_master() ) {
      //**** This federate is the Master for the multiphase ****
      //**** initialization process                         ****

      // if you want to restore from a check point, force the loading of the
      // checkpoint file here...
      if ( manager->restore_federation ) {
         if ( ( tRestoreName != NULL ) && ( *tRestoreName != '\0' ) ) {

            // make sure that we have a valid absolute path to the files.
            federate->check_HLA_save_directory();

            // signal the MASTER federate to track all federates who join,
            // looking for anyone who is not required.
            federate->set_restore_is_imminent();

            // read the required federates data from external file, replacing
            // the contents of 'known_feds'.
            federate->read_running_feds_file( string( tRestoreName ) );

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d \
You indicated that you want a restore => I AM THE MASTER <= \
Waiting for the required federates to join.\n",
                                __LINE__ );
            }
            // Make sure only the required federates have joined the federation.
            string return_string;
            return_string = federate->wait_for_required_federates_to_join();
            if ( !return_string.empty() ) {
               ostringstream errmsg;
               errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                      << " ERROR: " << return_string << '\n';
               DebugHandler::terminate_with_message( errmsg.str() );
            }

            // Load the MASTER federate from the checkpoint file...
            federate->restore_checkpoint( string( tRestoreName ) );

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
               message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d \
You indicated that you want a restore => I AM THE MASTER <= \
initiating restore request for '%s' with the RTI.\n",
                                __LINE__, tRestoreName );
            }
            // request federation restore from RTI
            federate->initiate_restore_announce( string( tRestoreName ) );

            // wait for the success / failure response from the RTI
            federate->wait_for_restore_request_callback();

            if ( federate->has_restore_request_failed() ) {
               ostringstream errmsg;
               errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():"
                      << __LINE__
                      << " ERROR: You indicated that you wanted to restore a "
                      << "checkpoint => I AM THE MASTER <= RTI rejected the "
                      << "restore request!!!! Make sure that you are restoring "
                      << "the federates from an identical federation save set."
                      << "\n      See IEEE 1516.1-2010, Section 4.18 for "
                      << "further info for the reasons why the RTI would reject"
                      << " the federation restore request...\n";
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
                      << " ERROR: You indicated that you wanted to restore a "
                      << "checkpoint => I AM THE MASTER <= "
                      << "wait_for_federation_restore_to_complete() failed!!!\n";
               errmsg << '\n'
                      << tStr;
               DebugHandler::terminate_with_message( errmsg.str() );
            }

            // Rebuild the 'federate handle set' because the federation was restored
            federate->restore_federate_handles_from_MOM();

            // Add and register the "STARTUP" sync point with all joined federates
            // so we can achieve it later.
            if ( !contains_sync_point( IMSim::STARTUP_SYNC_POINT ) ) {
               add_sync_point( IMSim::STARTUP_SYNC_POINT, IMSim::IMSIM_SYNC_POINT_LIST );
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  string label_str;
                  StringUtilities::to_string( label_str, IMSim::STARTUP_SYNC_POINT );
                  message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d Label: '%s'\n",
                                   __LINE__, label_str.c_str() );
               }
            }
            register_sync_point( IMSim::STARTUP_SYNC_POINT );

            // Wait for the announcement of "STARTUP" sync-point before proceeding.
            wait_for_sync_point_announced( IMSim::STARTUP_SYNC_POINT );

            // Restart myself...
            manager->restart_initialization();

            // Restart the checkpoint...
            federate->restart_checkpoint();

            // Achieve the "STARTUP" sync-point and wait for the
            // federation synchronize on it.
            achieve_sync_point_and_wait_for_synchronization( IMSim::STARTUP_SYNC_POINT );

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               if ( is_late_joiner() ) {
                  message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n\t\
=> I AM THE MASTER ** originally a late joining federate ** <= Federation restore is complete\n    \
Simulation has started and is now running...\n",
                                   __LINE__ );
               } else {
                  message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n    \
=> I AM THE MASTER <= Federation restore is complete\n\t\
Simulation has started and is now running...\n",
                                   __LINE__ );
               }
            }

            federate->set_federate_has_begun_execution();
         } else {
            ostringstream errmsg;
            errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                   << " ERROR: You indicated that you wanted to restore a checkpoint"
                   << " => I AM THE MASTER <= but you failed to specify the"
                   << "  checkpoint FILE NAME!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      } else { // MASTER but restore was not specified

         // Setup all the RTI handles for the objects, attributes and interaction
         // parameters.
         manager->setup_all_RTI_handles();

         // Make sure all required federates have joined the federation.
         federate->wait_for_required_federates_to_join();

         // Register the initialization synchronization points used to control
         // the IMSim startup process.
         register_sync_point( IMSim::SIM_CONFIG_SYNC_POINT,
                              federate->get_joined_federate_handles() );
         register_sync_point( IMSim::INITIALIZE_SYNC_POINT,
                              federate->get_joined_federate_handles() );
         register_sync_point( IMSim::STARTUP_SYNC_POINT,
                              federate->get_joined_federate_handles() );

         // Register all the user defined multiphase initialization
         // synchronization points just for the joined federates.
         register_all_sync_points( TrickHLA::MULTIPHASE_INIT_SYNC_POINT_LIST,
                                   federate->get_joined_federate_handles() );

         // Call publish_and_subscribe AFTER we've initialized the manager,
         // federate, and FedAmb.
         manager->publish_and_subscribe();

         // Reserve "SimConfig" object instance name.
         execution_configuration->reserve_object_name_with_RTI();

         // Reserve the RTI object instance names with the RTI, but only for
         // the objects that are locally owned.
         manager->reserve_object_names_with_RTI();

         // Wait for success or failure for the "SimConfig" name reservation.
         execution_configuration->wait_for_object_name_reservation();

         // Waits on the reservation of the RTI object instance names for the
         // locally owned objects. Calling this function will block until all
         // the object instances names for the locally owned objects have been
         // reserved.
         manager->wait_for_reservation_of_object_names();

         // Creates an RTI object instance and registers it with the RTI, but
         // only for the objects that we create.
         manager->register_objects_with_RTI();

         // Setup the preferred order for all object attributes and interactions.
         manager->setup_preferred_order_with_RTI();

         // Waits on the registration of all the required RTI object instances
         // with the RTI. Calling this function will block until all the
         // required object instances in the Federation have been registered.
         manager->wait_for_registration_of_required_objects();

         // Wait for the "sim_config", "initialize", and "startup" sync-points
         // to be announced.
         wait_for_sync_point_announced( IMSim::SIM_CONFIG_SYNC_POINT );
         wait_for_sync_point_announced( IMSim::INITIALIZE_SYNC_POINT );
         wait_for_sync_point_announced( IMSim::STARTUP_SYNC_POINT );

         // Achieve the "sim_config" sync-point and wait for the federation
         // to be synchronized on it.
         achieve_sync_point_and_wait_for_synchronization( IMSim::SIM_CONFIG_SYNC_POINT );

         // Send the "Simulation Configuration".
         send_execution_configuration();

         /* TODO: REMOVE
         // DANNY2.7 When master is started in freeze, create a pause sync point
         //  so other feds will start in freeze.
         if ( exec_get_freeze_command() != 0 ) {
            register_sync_point( IMSim::PAUSE_SYNC_POINT );
         } */

         // Achieve the "initialize" sync-point and wait for the federation
         // to be synchronized on it.
         achieve_sync_point_and_wait_for_synchronization( IMSim::INITIALIZE_SYNC_POINT );
      }
   } else {

      // Determine if the federate is a late-joiner or was instructed to restore
      // the federation.
      if ( determine_if_late_joining_or_restoring_federate() == FEDERATE_JOIN_RESTORING ) {

         // make sure that we have a valid absolute path to the files.
         federate->check_HLA_save_directory();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d \
You indicated that you want a restore => I AM NOT THE MASTER <= \
loading of the federate from the checkpoint file '%s'.\n",
                             __LINE__, tRestoreName );
         }
         federate->restore_checkpoint( string( tRestoreName ) );

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
                   << " ERROR: You indicated that you wanted to restore a "
                   << "checkpoint => I AM THE NOT MASTER <= "
                   << "wait_for_federation_restore_to_complete() failed!!!"
                   << '\n'
                   << '\n'
                   << tStr;
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         // Wait for the announcement of "STARTUP" sync-point before proceeding.
         wait_for_sync_point_announced( IMSim::STARTUP_SYNC_POINT );

         // restart myself...
         manager->restart_initialization();

         // restart the federate...
         federate->restart_checkpoint();

         // Achieve the "STARTUP" sync-point and wait for the federation
         // to be synchronized on it.
         achieve_sync_point_and_wait_for_synchronization( IMSim::STARTUP_SYNC_POINT );

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            if ( is_late_joiner() ) {
               message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes():%d\n\t\
=> I AM NOT THE MASTER ** originally late joining federate ** <= Federation restore is complete\n    \
Simulation has started and is now running...\n",
                                __LINE__ );
            } else {
               message_publish( MSG_NORMAL, "IMSim::ExecutionControl::pre_multi_phase_init_processes2():%d\n    \
=> I AM NOT THE MASTER <= Federation restore is complete\n    \
Simulation has started and is now running...\n",
                                __LINE__ );
            }
         }

         federate->set_federate_has_begun_execution();
      } else { // non-MASTER federate; not restoring a checkpoint

         // Setup all the RTI handles for the objects, attributes and interaction
         // parameters.
         manager->setup_all_RTI_handles();

         if ( !manager->is_late_joining_federate() ) {
            //**** Non-Master federate that is Not late in joining the ****
            //**** federation, so it can participate in the multiphase ****
            //**** initialization process                              ****

            // Call publish_and_subscribe AFTER we've initialized the manager,
            // federate, and FedAmb.
            manager->publish_and_subscribe();

            // Reserve the RTI object instance names with the RTI, but only for
            // the objects that are locally owned.
            manager->reserve_object_names_with_RTI();

            // Waits on the reservation of the RTI object instance names for the
            // locally owned objects. Calling this function will block until all
            // the object instances names for the locally owned objects have been
            // reserved.
            manager->wait_for_reservation_of_object_names();

            // Creates an RTI object instance and registers it with the RTI, but
            // only for the objects that are locally owned.
            manager->register_objects_with_RTI();

            // Setup the preferred order for all object attributes and interactions.
            manager->setup_preferred_order_with_RTI();

            // Waits on the registration of all the required RTI object
            // instances with the RTI. Calling this function will block until
            // all the required object instances in the Federation have been
            // registered.
            manager->wait_for_registration_of_required_objects();

            // Wait for the "sim_config", "initialize", and "startup" sync-points
            // to be announced.
            wait_for_sync_point_announced( IMSim::SIM_CONFIG_SYNC_POINT );
            wait_for_sync_point_announced( IMSim::INITIALIZE_SYNC_POINT );
            wait_for_sync_point_announced( IMSim::STARTUP_SYNC_POINT );

            // Achieve the "sim_config" sync-point and wait for the federation
            // to be synchronized on it.
            achieve_sync_point_and_wait_for_synchronization( IMSim::SIM_CONFIG_SYNC_POINT );

            // Wait for the "Simulation Configuration" object attribute reflection.
            receive_execution_configuration();

            // Achieve the "initialize" sync-point and wait for the federation
            // to be synchronized on it.
            achieve_sync_point_and_wait_for_synchronization( IMSim::INITIALIZE_SYNC_POINT );

         } else {
            //**** Late Joining Federate. ****

            if ( !federate->is_time_management_enabled() ) {
               ostringstream errmsg;
               errmsg << "IMSim::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                      << " ERROR: Late joining federates that do not use HLA"
                      << " time management are not supported yet!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }

            // Subscribe to the simulation configuration attributes.
            execution_configuration->subscribe_to_object_attributes();

            // Wait for the registration of the simulation configuration object.
            // Calling this function will block until all the simulation config
            // object instances in the Federation has been registered.
            execution_configuration->wait_for_registration();

            // Request a simulation configuration object update.
            manager->request_data_update( execution_configuration->get_name() );

            // Wait for the "Simulation Configuration" object attribute reflection.
            execution_configuration->wait_for_update();

            // Call publish_and_subscribe AFTER we've initialized the manager,
            // federate, and FedAmb.
            manager->publish_and_subscribe();

            // Wait for all objects to be discovered, if necessary.
            manager->wait_for_discovery_of_objects();

            // If this is a rejoining federate, re-aquire ownership of its attributes.
            if ( manager->is_this_a_rejoining_federate() ) {

               // Force ownership restore from other federate(s) and wait for
               // the ownership to complete before proceeding.
               manager->pull_ownership_upon_rejoin();
            } else {

               // Federate is not rejoining the federation. proceed with reserving
               // and registering the local data object with the RTI.

               // Reserve the RTI object instance names with the RTI, but only for
               // the objects that are locally owned.
               manager->reserve_object_names_with_RTI();

               // Waits on the reservation of the RTI object instance names for the
               // locally owned objects. Calling this function will block until all
               // the object instances names for the locally owned objects have been
               // reserved.
               manager->wait_for_reservation_of_object_names();

               // Creates an RTI object instance and registers it with the RTI, but
               // only for the objects that are locally owned.
               manager->register_objects_with_RTI();
            }

            // Setup the preferred order for all object attributes and interactions.
            manager->setup_preferred_order_with_RTI();

            // Waits on the registration of all the required RTI object
            // instances with the RTI. Calling this function will block until
            // all the required object instances in the Federation have been
            // registered.
            manager->wait_for_registration_of_required_objects();
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
   while ( !late_joiner_determined && !manager->is_restore_determined() ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // We are not a late joiner if the Sim-Config sync-point exists are we
      // are a member for it.
      if ( !manager->is_restore_determined() && contains_sync_point( IMSim::SIM_CONFIG_SYNC_POINT ) ) { // cppcheck-suppress [knownConditionTrueFalse]
         this->late_joiner            = false;
         this->late_joiner_determined = true;
      }

      // Determine if the Initialization Complete sync-point exists, which
      // means at this point we are a late joining federate.
      if ( !late_joiner_determined
           && !manager->is_restore_determined()
           && is_sync_point_announced( IMSim::INIT_COMPLETE_SYNC_POINT ) ) {
         this->late_joiner            = true;
         this->late_joiner_determined = true;
      }

      // when we receive the signal to restore, set the flag.
      if ( !late_joiner_determined
           && federate->has_restore_been_announced()
           && federate->is_start_to_restore() ) {
         manager->set_restore_determined( true );
         manager->set_restore_federate( true );
      }

      // Wait for a little while to give the sync-points time to come in.
      if ( !late_joiner_determined && !manager->is_restore_determined() ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !late_joiner_determined && !manager->is_restore_determined() ) { // cppcheck-suppress [knownConditionTrueFalse]

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():%d Waiting...\n",
                                __LINE__ );
            }
         }
      }
   }

   if ( late_joiner_determined ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():%d Late Joining Federate:%s\n",
                          __LINE__, ( is_late_joiner() ? "Yes" : "No" ) );
      }
      return TrickHLA::FEDERATE_JOIN_LATE;

   } else if ( manager->is_restore_determined() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():%d Restoring the Federate!\n",
                          __LINE__ );
      }
      return TrickHLA::FEDERATE_JOIN_RESTORING;

   } else {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::determine_if_late_joining_or_restoring_federate_IMSim():"
             << __LINE__ << " ERROR: Failed to determine if late joiner or restore federate!!!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return TrickHLA::FEDERATE_JOIN_NOMINAL;
}

/*!
@details This routine implements the IMSim post multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::post_multi_phase_init_processes()
{

   if ( is_master() ) {
      // Let the late joining federates know that we have completed initialization.
      register_sync_point( IMSim::INIT_COMPLETE_SYNC_POINT );
   }

   // When we join the federation, setup the list of current federates.
   // When a federate joins / resigns, this list will be automatically
   // updated by each federate.
   federate->load_and_print_running_federate_names();

   // Setup HLA time management.
   federate->setup_time_management();

   if ( manager->is_late_joining_federate() ) {
      // Jump to the GALT time, otherwise we will not be in sync with the
      // other federates on the HLA logical timeline.
      federate->time_advance_request_to_GALT();
   } else {
      // Achieve the "startup" sync-point and wait for the federation
      // to be synchronized on it.
      achieve_sync_point_and_wait_for_synchronization( IMSim::STARTUP_SYNC_POINT );
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
 * ref ATTRIBUTES for the IMSim ExecutionControl.
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

   if ( freeze_interaction == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for Interaction specialized"
             << " to FREEZE the sim!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   FreezeInteractionHandler *freeze_handler =
      reinterpret_cast< FreezeInteractionHandler * >(
         alloc_type( 1, "IMSim::FreezeInteractionHandler" ) );

   if ( freeze_handler == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for FreezeInteractionHandler!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Set the reference to this ExecutionControl instance in the
   // IMSim::FreezeIntrationHandler.
   freeze_handler->execution_control = this;

   // Configure the Freeze interaction handler.
   freeze_interaction->set_handler( freeze_handler );
   freeze_interaction->set_FOM_name( "Freeze" );
   freeze_interaction->set_publish();
   freeze_interaction->set_subscribe();

   // Configure the Parameters.
   freeze_interaction->set_parameter_count( 1 );
   Parameter *tParm = reinterpret_cast< Parameter * >(
      alloc_type( freeze_interaction->get_parameter_count(), "TrickHLA::Parameter" ) );

   if ( tParm == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the parameters of the"
             << " FREEZE interaction!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }
   tParm[0].set_FOM_name( "time" );
   tParm[0].set_encoding( ENCODING_LOGICAL_TIME );
   freeze_interaction->set_parameters( tParm );

   // Since this is an interaction handler generated on the fly, there is no
   // trick variable to resolve to at run time, which is supplied by the
   // input.py file. we must build data structures with sufficient information
   // for the Parameter class to link itself into the just generated
   // Freeze Interaction Handler, and its sole parameter ('time').

   // Allocate the trick ATTRIBUTES data structure with room for two
   // entries: 1) the 'time' parameter and 2) an empty entry marking the end
   // of the structure.
   ATTRIBUTES *time_attr;
   time_attr = static_cast< ATTRIBUTES * >( malloc( 2 * sizeof( ATTRIBUTES ) ) );
   if ( time_attr == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the ATTRIBUTES for the"
             << " 'time' value of the FREEZE interaction!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Find the 'time' value in the Freeze Interaction Handler's ATTRIBUTES.
   // since we may not know the total # of elements, we look for an empty
   // element as an ending marker of the ATTRIBUTES.
   int attr_index = 0;

   // loop until the current ATTRIBUTES name is a NULL string
   while ( strcmp( attrIMSim__FreezeInteractionHandler[attr_index].name, "" ) != 0 ) {
      if ( strcmp( attrIMSim__FreezeInteractionHandler[attr_index].name, "time" ) == 0 ) {
         memcpy( &time_attr[0],
                 &attrIMSim__FreezeInteractionHandler[attr_index],
                 sizeof( ATTRIBUTES ) );
      }
      ++attr_index;
   }

   // Now that we have hit the end of the ATTRIBUTES array, copy the last
   // entry into my time_attr array to make it a valid ATTRIBUTE array.
   memcpy( &time_attr[1],
           &attrIMSim__FreezeInteractionHandler[attr_index],
           sizeof( ATTRIBUTES ) );

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg2;
      msg2 << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__ << '\n'
           << "--------------- Trick REF-Attributes ---------------\n"
           << " FOM-Interaction:'" << freeze_interaction->get_FOM_name() << "'\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   // Initialize the TrickHLA Interaction before we use it.
   freeze_interaction->initialize( this->manager );

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg2;
      msg2 << "IMSim::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
           << " FOM-Parameter:'" << tParm[0].get_FOM_name() << "'"
           << " NOTE: This is an auto-generated parameter so there is no"
           << " associated 'Trick-Name'.\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   // Initialize the TrickHLA Parameter. Since we built the interaction handler
   // in-line, and not via the trick input.py file, use the alternate version of
   // the initialize routine which does not resolve the fully-qualified trick
   // name to access the ATTRIBUTES if the trick variable...
   tParm[0].initialize( freeze_interaction->get_FOM_name(),
                        static_cast< void * >( freeze_handler->get_address_of_interaction_time() ),
                        static_cast< ATTRIBUTES * >( time_attr ) );
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::setup_object_RTI_handles()
{
   ExecutionConfiguration *ExCO = get_execution_configuration();
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL SimConfig!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }
   manager->setup_object_RTI_handles( 1, ExCO );
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::setup_interaction_RTI_handles()
{
   // Ask the manager to setup the Freeze interaction RTI handles.
   manager->setup_interaction_RTI_handles( 1, freeze_interaction );

   return;
}

/*! Add initialization synchronization points to regulate startup. */
void ExecutionControl::add_initialization_sync_points()
{
   // Add the IMSim initialization synchronization points used for startup
   // regulation to the sync-point manager.
   add_sync_point( IMSim::SIM_CONFIG_SYNC_POINT, IMSim::IMSIM_SYNC_POINT_LIST );
   add_sync_point( IMSim::INITIALIZE_SYNC_POINT, IMSim::IMSIM_SYNC_POINT_LIST );
   add_sync_point( IMSim::STARTUP_SYNC_POINT, IMSim::IMSIM_SYNC_POINT_LIST );
   add_sync_point( IMSim::INIT_COMPLETE_SYNC_POINT, IMSim::IMSIM_SYNC_POINT_LIST );
   add_sync_point( IMSim::FEDSAVE_SYNC_POINT, IMSim::IMSIM_SYNC_POINT_LIST );
   add_sync_point( IMSim::FEDRUN_SYNC_POINT, IMSim::IMSIM_SYNC_POINT_LIST );

   // Add the multiphase initialization synchronization points.
   add_multiphase_init_sync_points();
}

void ExecutionControl::sync_point_announced(
   wstring const          &label,
   RTI1516_USERDATA const &user_supplied_tag )
{
   // In this case the default SyncPointManagerBase::sync_point_announced()
   // function works. Strictly speaking, we could just not define this.
   // However, this provides a place to implement if that changes.
   SyncPointManagerBase::sync_point_announced( label, user_supplied_tag );
}

void ExecutionControl::publish()
{
   // Check to see if we are the Master federate.
   if ( is_master() ) {
      // Publish the simulation configuration if we are the master federate.
      execution_configuration->publish_object_attributes();

      // Publish the freeze_interactions.
      for ( int n = 0; n < freeze_inter_count; ++n ) {
         freeze_interaction[n].publish_interaction();
      }
   }
}

void ExecutionControl::unpublish()
{
   if ( is_master() ) {
      // Unpublish the execution configuration if we are the master federate.
      execution_configuration->unpublish_all_object_attributes();

      // Unpublish all the freeze_interactions.
      for ( int i = 0; i < freeze_inter_count; ++i ) {

         // Only unpublish a FREEZE interaction that we publish.
         if ( freeze_interaction[i].is_publish() ) {

            bool do_unpublish = true;
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
}

void ExecutionControl::subscribe()
{
   if ( !is_master() ) {
      // Subscribe to the execution configuration if we are not the master federate.
      execution_configuration->subscribe_to_object_attributes();

      // Subscribe to the Freeze interactions if this is not the Master federate.
      for ( int n = 0; n < freeze_inter_count; ++n ) {
         freeze_interaction[n].subscribe_to_interaction();
      }
   }
}

void ExecutionControl::unsubscribe()
{
   if ( !is_master() ) {
      // Unsubscribe from the execution configuration if we are NOT the Master federate.
      execution_configuration->unsubscribe_all_object_attributes();

      // Unsubscribe from all the freeze_interactions.
      for ( int i = 0; i < freeze_inter_count; ++i ) {
         // Only unsubscribe from FREEZE interactions that are subscribed to.
         if ( freeze_interaction[i].is_subscribe() ) {
            bool do_unsubscribe = true;
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
}

/*!
 * @job_class{scheduled}
 */
bool ExecutionControl::receive_interaction(
   InteractionClassHandle const  &theInteraction,
   ParameterHandleValueMap const &theParameterValues,
   RTI1516_USERDATA const        &theUserSuppliedTag,
   LogicalTime const             &theTime,
   bool const                     received_as_TSO )
{
   // Find the TrickHLAFreezeInteraction we have data for.
   for ( int i = 0; i < freeze_inter_count; ++i ) {
      // Process the FREEZE interaction if we subscribed to it and we have the
      // same class handle.
      if ( freeze_interaction[i].is_subscribe()
           && ( freeze_interaction[i].get_class_handle() == theInteraction ) ) {

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            string handle;
            StringUtilities::to_string( handle, theInteraction );

            if ( received_as_TSO ) {
               Int64Time _time;
               _time.set( theTime );
               message_publish( MSG_NORMAL, "IMSim::ExecutionControl::receive_interaction(FREEZE):%d ID:%s, HLA-time:%G\n",
                                __LINE__, handle.c_str(), _time.get_time_in_seconds() );
            } else {
               message_publish( MSG_NORMAL, "IMSim::ExecutionControl::receive_interaction(FREEZE):%d ID:%s\n",
                                __LINE__, handle.c_str() );
            }
         }

         if ( received_as_TSO ) {
            InteractionItem item( i,
                                  INTERACTION_TYPE_BUILTIN_FREEZE,
                                  freeze_interaction[i].get_parameter_count(),
                                  freeze_interaction[i].get_parameters(),
                                  theParameterValues,
                                  theUserSuppliedTag,
                                  theTime );

            freeze_interaction[i].extract_data( &item );
            freeze_interaction[i].process_interaction();
         } else {
            InteractionItem item( i,
                                  INTERACTION_TYPE_BUILTIN_FREEZE,
                                  freeze_interaction[i].get_parameter_count(),
                                  freeze_interaction[i].get_parameters(),
                                  theParameterValues,
                                  theUserSuppliedTag );

            freeze_interaction[i].extract_data( &item );
            freeze_interaction[i].process_interaction();
         }

         // Indicate the interaction has been processed into the queue.
         return true;
      }
   }

   // Not processed.
   return false;
}

void ExecutionControl::send_mode_transition_interaction(
   ModeTransitionEnum requested_mode )
{
   return;
}

bool ExecutionControl::set_pending_mtr(
   MTREnum mtr_value )
{
   if ( is_mtr_valid( mtr_value ) ) {
      this->pending_mtr = mtr_value;
   }
   return false;
}

bool ExecutionControl::is_mtr_valid(
   MTREnum mtr_value )
{
   switch ( mtr_value ) {
      case IMSim::MTR_GOTO_RUN: {
         return ( ( current_execution_control_mode == TrickHLA::EXECUTION_CONTROL_INITIALIZING )
                  || ( current_execution_control_mode == TrickHLA::EXECUTION_CONTROL_FREEZE ) );
      }
      case IMSim::MTR_GOTO_FREEZE: {
         return ( ( current_execution_control_mode == TrickHLA::EXECUTION_CONTROL_INITIALIZING )
                  || ( current_execution_control_mode == TrickHLA::EXECUTION_CONTROL_RUNNING ) );
      }
      case IMSim::MTR_GOTO_SHUTDOWN: {
         return ( current_execution_control_mode != TrickHLA::EXECUTION_CONTROL_SHUTDOWN );
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
      case IMSim::MTR_UNINITIALIZED: {
         this->pending_mtr = IMSim::MTR_UNINITIALIZED;
         set_next_execution_control_mode( TrickHLA::EXECUTION_CONTROL_UNINITIALIZED );
         break;
      }
      case IMSim::MTR_INITIALIZING: {
         this->pending_mtr = IMSim::MTR_INITIALIZING;
         set_next_execution_control_mode( TrickHLA::EXECUTION_CONTROL_INITIALIZING );
         break;
      }
      case IMSim::MTR_GOTO_RUN: {
         this->pending_mtr = IMSim::MTR_GOTO_RUN;
         set_next_execution_control_mode( TrickHLA::EXECUTION_CONTROL_RUNNING );
         break;
      }
      case IMSim::MTR_GOTO_FREEZE: {
         this->pending_mtr = IMSim::MTR_GOTO_FREEZE;
         set_next_execution_control_mode( TrickHLA::EXECUTION_CONTROL_FREEZE );
         break;
      }
      case IMSim::MTR_GOTO_SHUTDOWN: {
         this->pending_mtr = IMSim::MTR_GOTO_SHUTDOWN;
         set_next_execution_control_mode( TrickHLA::EXECUTION_CONTROL_SHUTDOWN );
         break;
      }
      default: {
         this->pending_mtr = IMSim::MTR_UNINITIALIZED;
         break;
      }
   }
}

void ExecutionControl::set_next_execution_control_mode(
   TrickHLA::ExecutionControlEnum exec_control )
{

   // This should only be called by the Master federate.
   if ( !is_master() ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::set_next_execution_mode():" << __LINE__
             << " ERROR: This should only be called by the Master federate!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   switch ( exec_control ) {
      case TrickHLA::EXECUTION_CONTROL_UNINITIALIZED: {

         // Set the next execution mode.
         this->requested_execution_control_mode = TrickHLA::EXECUTION_CONTROL_UNINITIALIZED;
         this->next_execution_mode              = IMSim::EXECUTION_MODE_UNINITIALIZED;

         // Set the next mode times.
         this->next_mode_scenario_time = get_scenario_time(); // Immediate
         this->next_mode_cte_time      = get_cte_time();      // Immediate
         break;
      }
      case TrickHLA::EXECUTION_CONTROL_INITIALIZING: {

         // Set the next execution mode.
         this->requested_execution_control_mode = TrickHLA::EXECUTION_CONTROL_INITIALIZING;
         this->next_execution_mode              = IMSim::EXECUTION_MODE_INITIALIZING;

         // Set the next mode times.
         this->scenario_time_epoch     = get_scenario_time(); // Now.
         this->next_mode_scenario_time = get_scenario_time(); // Immediate
         this->next_mode_cte_time      = get_cte_time();      // Immediate
         break;
      }
      case TrickHLA::EXECUTION_CONTROL_RUNNING: {

         // Set the next execution mode.
         this->requested_execution_control_mode = TrickHLA::EXECUTION_CONTROL_RUNNING;
         this->next_execution_mode              = IMSim::EXECUTION_MODE_RUNNING;

         // Set the next mode times.
         this->next_mode_scenario_time = get_scenario_time(); // Immediate
         this->next_mode_cte_time      = get_cte_time();
         if ( this->next_mode_cte_time > -std::numeric_limits< double >::max() ) {
            this->next_mode_cte_time = this->next_mode_cte_time + get_time_padding(); // Some time in the future.
         }
         break;
      }
      case TrickHLA::EXECUTION_CONTROL_FREEZE: {

         // Set the next execution mode.
         this->requested_execution_control_mode = TrickHLA::EXECUTION_CONTROL_FREEZE;
         this->next_execution_mode              = IMSim::EXECUTION_MODE_FREEZE;

         // Set the next mode times.
         this->next_mode_scenario_time = get_scenario_time() + this->time_padding; // Some time in the future.
         this->next_mode_cte_time      = get_cte_time();
         if ( this->next_mode_cte_time > -std::numeric_limits< double >::max() ) {
            this->next_mode_cte_time = this->next_mode_cte_time + get_time_padding(); // Some time in the future.
         }

         // Set the ExecutionControl freeze times.
         this->scenario_freeze_time   = this->next_mode_scenario_time;
         this->simulation_freeze_time = scenario_timeline->compute_simulation_time( this->next_mode_scenario_time );
         break;
      }
      case TrickHLA::EXECUTION_CONTROL_SHUTDOWN: {

         // Set the next execution mode.
         this->requested_execution_control_mode = TrickHLA::EXECUTION_CONTROL_SHUTDOWN;
         this->next_execution_mode              = IMSim::EXECUTION_MODE_SHUTDOWN;

         // Set the next mode times.
         this->next_mode_scenario_time = get_scenario_time(); // Immediate.
         this->next_mode_cte_time      = get_cte_time();      // Immediate
         break;
      }
      default: {
         this->requested_execution_control_mode = TrickHLA::EXECUTION_CONTROL_UNINITIALIZED;
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            ostringstream errmsg;
            errmsg << "IMSim::ExecutionControl::set_next_execution_mode():"
                   << __LINE__ << " WARNING: Unknown execution mode value: "
                   << exec_control << '\n';
            message_publish( MSG_NORMAL, errmsg.str().c_str() );
         }
         break;
      }
   }
}

bool ExecutionControl::check_mode_transition_request()
{
   // Just return if false mode change has been requested.
   if ( !is_mode_transition_requested() ) {
      return false;
   }

   // Only the Master federate receives and processes Mode Transition Requests.
   if ( !is_master() ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::check_mode_transition_request():"
             << __LINE__ << " WARNING: Received Mode Transition Request and not Master: "
             << mtr_enum_to_string( this->pending_mtr ) << '\n';
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
      return false;
   }

   // First check to see if this is a valid MTR.
   if ( !is_mtr_valid( this->pending_mtr ) ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::check_mode_transition_request():"
             << __LINE__ << " WARNING: Invalid Mode Transition Request: "
             << mtr_enum_to_string( this->pending_mtr ) << '\n';
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
      return false;
   }

   return true;
}

bool ExecutionControl::process_mode_interaction()
{
   return process_mode_transition_request();
}

bool ExecutionControl::process_mode_transition_request()
{
   // Just return is no mode change has been requested.
   if ( !check_mode_transition_request() ) {
      return false;
   } else {
      // Since this is a valid MTR, set the next mode from the MTR.
      set_mode_request_from_mtr( this->pending_mtr );
   }

   // Reference the IMSim Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // Print diagnostic message if appropriate.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg;
      msg << "=============================================================\n"
          << "IMSim::ExecutionControl::process_mode_transition_request()\n"
          << "\t current_scenario_time:     " << setprecision( 18 ) << scenario_timeline->get_time() << '\n'
          << "\t scenario_time_epoch:       " << setprecision( 18 ) << scenario_timeline->get_epoch() << '\n'
          << "\t scenario_time_epoch(ExCO): " << setprecision( 18 ) << scenario_time_epoch << '\n'
          << "\t scenario_time_sim_offset:  " << setprecision( 18 ) << scenario_timeline->get_sim_offset() << '\n'
          << "\t Current HLA grant time:    " << federate->get_granted_time().get_time_in_seconds() << '\n'
          << "\t Current HLA request time:  " << federate->get_requested_time().get_time_in_seconds() << '\n'
          << "\t current_sim_time:          " << setprecision( 18 ) << sim_timeline->get_time() << '\n'
          << "\t simulation_time_epoch:     " << setprecision( 18 ) << sim_timeline->get_epoch() << '\n';
      if ( does_cte_timeline_exist() ) {
         msg << "\t current_CTE_time:          " << setprecision( 18 ) << cte_timeline->get_time() << '\n'
             << "\t CTE_time_epoch:            " << setprecision( 18 ) << cte_timeline->get_epoch() << '\n';
      }
      msg << "\t next_mode_scenario_time:   " << setprecision( 18 ) << next_mode_scenario_time << '\n'
          << "\t next_mode_cte_time:        " << setprecision( 18 ) << next_mode_cte_time << '\n'
          << "\t scenario_freeze_time:      " << setprecision( 18 ) << scenario_freeze_time << '\n'
          << "\t simulation_freeze_time:    " << setprecision( 18 ) << simulation_freeze_time << '\n'
          << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Check Mode Transition Request.
   switch ( this->pending_mtr ) {

      case IMSim::MTR_GOTO_RUN: {

         // Clear the mode change request flag.
         clear_mode_transition_requested();

         // Transition to run can only happen from initialization or freeze.
         // We don't really need to do anything if we're in initialization.
         if ( this->current_execution_control_mode == TrickHLA::EXECUTION_CONTROL_FREEZE ) {

            // Tell Trick to exit freeze and go to run.
            the_exec->run();

            // The run transition logic will be triggered when exiting Freeze.
            // This is done in the TrickHLA::Federate::exit_freeze() routine
            // called when exiting Freeze.
         }

         return true;
      }
      case IMSim::MTR_GOTO_FREEZE: {

         // Clear the mode change request flag.
         clear_mode_transition_requested();

         // Transition to freeze can only happen from initialization or run.
         // We don't really need to do anything if we're in initialization.
         if ( this->current_execution_control_mode == TrickHLA::EXECUTION_CONTROL_RUNNING ) {

            // Send out the updated ExCO.
            ExCO->send_init_data();

            // Announce the pending freeze.
            freeze_mode_announce();

            // Tell Trick to go into freeze at the appointed time.
            the_exec->freeze( this->simulation_freeze_time );

            // The freeze transition logic will be done just before entering
            // Freeze. This is done in the TrickHLA::Federate::freeze_init()
            // routine called when entering Freeze.
         }

         return true;
      }
      case IMSim::MTR_GOTO_SHUTDOWN: {

         // Announce the shutdown.
         shutdown_mode_announce();

         // Tell Trick to shutdown sometime in the future.
         // The IMSim ExecutionControl shutdown transition will be made from
         // the TrickHLA::Federate::shutdown() job.
         the_exec->stop( the_exec->get_sim_time() + get_time_padding() );

         return true;
      }
      default: {
         break;
      }
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
   if ( is_master() ) {
      errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
             << __LINE__ << " WARNING: Master receive an ExCO update: "
             << execution_control_enum_to_string( this->requested_execution_control_mode )
             << '\n';
      message_publish( MSG_NORMAL, errmsg.str().c_str() );

      // Return that no mode changes occurred.
      return false;
   }

   // Translate the native ExCO mode values into ExecutionModeEnum.
   ExecutionModeEnum exco_cem = execution_mode_int16_to_enum( this->current_execution_mode );
   ExecutionModeEnum exco_nem = execution_mode_int16_to_enum( this->next_execution_mode );

   // Check for consistency between ExecutionControl and ExCO.
   if ( exco_cem != execution_control_enum_to_int16( this->current_execution_control_mode ) ) {
      errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
             << __LINE__ << " WARNING: Current execution mode mismatch between ExecutionControl ("
             << execution_control_enum_to_string( this->current_execution_control_mode )
             << ") and the ExCO current execution mode ("
             << execution_mode_enum_to_string( exco_cem )
             << ")!\n";
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
   }

   // Check for change in execution mode.
   if ( exco_nem != exco_cem ) {
      mode_change = true;
      if ( exco_nem == IMSim::EXECUTION_MODE_SHUTDOWN ) {
         this->requested_execution_control_mode = TrickHLA::EXECUTION_CONTROL_SHUTDOWN;
      } else if ( exco_nem == IMSim::EXECUTION_MODE_RUNNING ) {
         this->requested_execution_control_mode = TrickHLA::EXECUTION_CONTROL_RUNNING;
      } else if ( exco_nem == IMSim::EXECUTION_MODE_FREEZE ) {
         this->requested_execution_control_mode = TrickHLA::EXECUTION_CONTROL_FREEZE;
         this->scenario_freeze_time             = this->next_mode_scenario_time;
         this->simulation_freeze_time           = scenario_timeline->compute_simulation_time( this->scenario_freeze_time );
      } else {
         errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Invalid ExCO next execution mode: "
                << execution_mode_enum_to_string( exco_nem ) << "!\n";
         message_publish( MSG_NORMAL, errmsg.str().c_str() );

         // Return that no mode changes occurred.
         return false;
      }
   }

   // Check for CTE mode time update.
   if ( this->next_mode_cte_time != this->next_mode_cte_time ) {
      // FIXME:
      // this->next_mode_cte_time = this->next_mode_cte_time;
   }

   // Check for mode changes.
   if ( !mode_change ) {
      // Return that no mode changes occurred.
      return false;
   }

   // Process the mode change.
   switch ( this->current_execution_control_mode ) {

      case TrickHLA::EXECUTION_CONTROL_UNINITIALIZED: {

         // Check for SHUTDOWN.
         if ( this->requested_execution_control_mode == TrickHLA::EXECUTION_CONTROL_SHUTDOWN ) {

            // Mark the current execution mode as SHUTDOWN.
            this->current_execution_control_mode = TrickHLA::EXECUTION_CONTROL_SHUTDOWN;
            this->current_execution_mode         = IMSim::EXECUTION_MODE_SHUTDOWN;

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
                   << ")!\n";
            message_publish( MSG_NORMAL, errmsg.str().c_str() );

            // Return that no mode changes occurred.
            return false;
         }

         // Return that a mode change occurred.
         return true;
      }
      case TrickHLA::EXECUTION_CONTROL_INITIALIZING: {

         // Check for SHUTDOWN.
         if ( this->requested_execution_control_mode == TrickHLA::EXECUTION_CONTROL_SHUTDOWN ) {

            // Mark the current execution mode as SHUTDOWN.
            this->current_execution_control_mode = TrickHLA::EXECUTION_CONTROL_SHUTDOWN;
            this->current_execution_mode         = IMSim::EXECUTION_MODE_SHUTDOWN;

            // Tell the TrickHLA::Federate to shutdown.
            // The IMSim ExecutionControl shutdown transition will be made from
            // the TrickHLA::Federate::shutdown() job.
            the_exec->stop();

         } else if ( this->requested_execution_control_mode == TrickHLA::EXECUTION_CONTROL_RUNNING ) {

            // Tell Trick to go to in Run at startup.
            the_exec->set_freeze_command( false );

            // This is an early joining federate in initialization.
            // So, proceed to the run mode transition.
            run_mode_transition();

         } else if ( this->requested_execution_control_mode == TrickHLA::EXECUTION_CONTROL_FREEZE ) {

            // Announce the pending freeze.
            freeze_mode_announce();

            // Tell Trick to go into freeze at startup.
            // the_exec->freeze();

            // Tell Trick to go into freeze at startup.
            the_exec->set_freeze_command( true );

            // The freeze transition logic will be done just before entering
            // Freeze. This is done in the TrickHLA::Federate::freeze_init()
            // routine called when entering Freeze.

         } else if ( this->requested_execution_control_mode == TrickHLA::EXECUTION_CONTROL_INITIALIZING ) {

            // There's really nothing to do here.

         } else {

            errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
                   << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                   << execution_control_enum_to_string( this->current_execution_control_mode )
                   << ") and the requested execution mode ("
                   << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << ")!\n";
            message_publish( MSG_NORMAL, errmsg.str().c_str() );

            // Return that no mode changes occurred.
            return false;
         }

         // Return that a mode change occurred.
         return true;
      }
      case TrickHLA::EXECUTION_CONTROL_RUNNING: {

         // Check for SHUTDOWN.
         if ( this->requested_execution_control_mode == EXECUTION_CONTROL_SHUTDOWN ) {

            // Print out a diagnostic warning message.
            errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
                   << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                   << execution_control_enum_to_string( this->current_execution_control_mode )
                   << ") and the requested execution mode ("
                   << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << ")!\n";
            message_publish( MSG_NORMAL, errmsg.str().c_str() );

            // Mark the current execution mode as SHUTDOWN.
            this->current_execution_control_mode = TrickHLA::EXECUTION_CONTROL_SHUTDOWN;
            this->current_execution_mode         = IMSim::EXECUTION_MODE_SHUTDOWN;

            // Tell the TrickHLA::Federate to shutdown.
            // The IMSim ExecutionControl shutdown transition will be made from
            // the TrickHLA::Federate::shutdown() job.
            the_exec->stop();
         } else if ( this->requested_execution_control_mode == TrickHLA::EXECUTION_CONTROL_FREEZE ) {

            // Print diagnostic message if appropriate.
            if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               ostringstream msg;
               msg << "ExecutionControl::process_execution_control_updates():" << __LINE__ << '\n'
                   << "\t current_scenario_time:     " << setprecision( 18 ) << scenario_timeline->get_time() << '\n'
                   << "\t scenario_time_epoch:       " << setprecision( 18 ) << scenario_timeline->get_epoch() << '\n'
                   << "\t scenario_time_epoch(ExCO): " << setprecision( 18 ) << scenario_time_epoch << '\n'
                   << "\t scenario_time_sim_offset:  " << setprecision( 18 ) << scenario_timeline->get_sim_offset() << '\n'
                   << "\t current_sim_time:          " << setprecision( 18 ) << sim_timeline->get_time() << '\n'
                   << "\t simulation_time_epoch:     " << setprecision( 18 ) << sim_timeline->get_epoch() << '\n';
               if ( does_cte_timeline_exist() ) {
                  msg << "\t current_CTE_time:          " << setprecision( 18 ) << cte_timeline->get_time() << '\n'
                      << "\t CTE_time_epoch:            " << setprecision( 18 ) << cte_timeline->get_epoch() << '\n';
               }
               msg << "\t next_mode_scenario_time:   " << setprecision( 18 ) << next_mode_scenario_time << '\n'
                   << "\t next_mode_cte_time:        " << setprecision( 18 ) << next_mode_cte_time << '\n'
                   << "\t scenario_freeze_time:      " << setprecision( 18 ) << scenario_freeze_time << '\n'
                   << "\t simulation_freeze_time:    " << setprecision( 18 ) << simulation_freeze_time << '\n'
                   << "=============================================================\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
            }

            // Announce the pending freeze.
            freeze_mode_announce();

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
                   << ")!\n";
            message_publish( MSG_NORMAL, errmsg.str().c_str() );

            // Return that no mode changes occurred.
            return false;
         }

         // Return that a mode change occurred.
         return true;
      }
      case TrickHLA::EXECUTION_CONTROL_FREEZE: {

         // Check for SHUTDOWN.
         if ( this->requested_execution_control_mode == TrickHLA::EXECUTION_CONTROL_SHUTDOWN ) {

            // Mark the current execution mode as SHUTDOWN.
            this->current_execution_control_mode = TrickHLA::EXECUTION_CONTROL_SHUTDOWN;
            this->current_execution_mode         = IMSim::EXECUTION_MODE_SHUTDOWN;

            // Shutdown the federate now.
            exec_get_exec_cpp()->stop();

         } else if ( this->requested_execution_control_mode == TrickHLA::EXECUTION_CONTROL_RUNNING ) {

            // Tell Trick to exit freeze and go to run.
            the_exec->run();

            // The run transition logic will be done just when exiting
            // Freeze. This is done in the TrickHLA::Federate::exit_freeze()
            // routine called when entering Freeze.
            // run_mode_transition();

         } else {

            errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
                   << __LINE__ << " WARNING: Execution mode mismatch between current mode ("
                   << execution_control_enum_to_string( this->current_execution_control_mode )
                   << ") and the requested execution mode ("
                   << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << ")!\n";
            message_publish( MSG_NORMAL, errmsg.str().c_str() );

            // Return that no mode changes occurred.
            return false;
         }

         // Return that a mode change occurred.
         return true;
      }
      case TrickHLA::EXECUTION_CONTROL_SHUTDOWN: {

         // Once in SHUTDOWN, we cannot do anything else.
         errmsg << "IMSim::ExecutionControl::process_execution_control_updates():"
                << __LINE__ << " WARNING: Shutting down but received mode transition: "
                << execution_control_enum_to_string( this->requested_execution_control_mode )
                << '\n';
         message_publish( MSG_NORMAL, errmsg.str().c_str() );

         // Return that no mode changes occurred.
         return false;
      }
      default: {
         break;
      }
   }

   // Return that no mode changes occurred.
   return false;
}

bool ExecutionControl::run_mode_transition()
{
   /* TODO: Update for IMSim

   RTIambassador          *RTI_amb  = federate->get_RTI_ambassador();
   ExecutionConfiguration *ExCO     = get_execution_configuration();
   SyncPoint              *sync_pnt = NULL;

     // Register the 'mtr_run' sync-point.
     if ( is_master() ) {
        sync_pnt = register_sync_point( *RTI_amb, IMSim::MTR_RUN_SYNC_POINT );
     } else {
        sync_pnt = get_sync_point( IMSim::MTR_RUN_SYNC_POINT );
     }

     // Make sure that we have a valid sync-point.
     if ( sync_pnt == NULL ) {
        ostringstream errmsg;
        errmsg << "IMSim::ExecutionControl::run_mode_transition():" << __LINE__
               << " ERROR: The 'mtr_run' sync-point was not found!\n";
        DebugHandler::terminate_with_message( errmsg.str() );
     } else {

        // Wait for 'mtr_run' sync-point announce.
        wait_for_sync_point_announcement( federate, sync_pnt );

        // Achieve the 'mtr-run' sync-point.
        achieve_sync_point( sync_pnt );

        // Wait for 'mtr_run' sync-point synchronization.
        wait_for_synchronization( federate, sync_pnt );

        // Set the current execution mode to running.
        this->current_execution_control_mode = EXECUTION_CONTROL_RUNNING;
        this->current_execution_mode         = EXECUTION_MODE_RUNNING;

        // Check for CTE.
        if ( does_cte_timeline_exist() ) {

           double go_to_run_time;

           // The Master federate updates the ExCO with the CTE got-to-run time.
           if ( is_master() ) {

              go_to_run_time = this->next_mode_cte_time;
              ExCO->send_init_data();

           } // Other federates wait on the ExCO update with the CTE go-to-run time.
           else {

              // Wait for the ExCO update with the CTE time.
              ExCO->wait_for_update();

              // Process the just received ExCO update.
              process_execution_control_updates();

              // Set the CTE time to go to run.
              go_to_run_time = this->next_mode_cte_time;
           }

           // Wait for the CTE go-to-run time.
           double diff;
           while ( get_cte_time() < go_to_run_time ) {

              // Check for shutdown.
              federate->check_for_shutdown_with_termination();

              diff = go_to_run_time - get_cte_time();
              if ( fmod( diff, 1.0 ) == 0.0 ) {
                 if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                    message_publish( MSG_NORMAL, "IMSim::ExecutionControl::run_mode_transition():%d Going to run in %G seconds.\n",
                             __LINE__, diff );
                 }
              }
           }

           // Print debug message if appropriate.
           if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
              double curr_cte_time = get_cte_time();
              diff                 = curr_cte_time - go_to_run_time;
              message_publish( MSG_NORMAL, "IMSim::ExecutionControl::run_mode_transition():%d \n  Going to run at CTE time %.18G seconds. \n  Current CTE time %.18G seconds. \n  Difference: %.9lf seconds.\n",
                       __LINE__, go_to_run_time, curr_cte_time, diff );
           }
        }
     }
     */
   return true;
}

void ExecutionControl::freeze_mode_announce()
{
   /*TODO: Implement for IMSim

   // Register the 'mtr_freeze' sync-point.
   if ( is_master() ) {
      register_sync_point( *( federate->get_RTI_ambassador() ), IMSim::MTR_FREEZE_SYNC_POINT );
   }
   */
}

bool ExecutionControl::freeze_mode_transition()
{
   /*TODO: Implement for IMSim

   // Make sure that we have a valid sync-point.
   if ( !contains_sync_point( SpaceFOM::MTR_FREEZE_SYNC_POINT ) ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::freeze_mode_transition():" << __LINE__
             << " ERROR: The 'mtr_freeze' sync-point was not found!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      // Wait for 'mtr_freeze' sync-point announce.
      wait_for_sync_point_announced( SpaceFOM::MTR_FREEZE_SYNC_POINT );

      // Achieve the 'mtr_freeze' sync-point and wait for synchronization.
      achieve_sync_point_and_wait_for_synchronization( SpaceFOM::MTR_FREEZE_SYNC_POINT );

      // Set the current execution mode to freeze.
      this->current_execution_control_mode = EXECUTION_CONTROL_FREEZE;
      get_execution_configuration()->set_current_execution_mode( EXECUTION_MODE_FREEZE );
   }*/
   return false;
}

void ExecutionControl::shutdown_mode_announce()
{

   // Only the Master federate will ever announce a shutdown.
   if ( !is_master() ) {
      return;
   }

   // If the current execution mode is uninitialized then we haven't gotten
   // far enough in the initialization process to shut anything down.
   if ( this->current_execution_control_mode == EXECUTION_CONTROL_UNINITIALIZED ) {
      return;
   }

   // Set the next execution mode to shutdown.
   set_next_execution_control_mode( EXECUTION_CONTROL_SHUTDOWN );

   // Send out the updated ExCO.
   get_execution_configuration()->send_init_data();

   // Clear the mode change request flag.
   clear_mode_transition_requested();
}

/*!
 * @job_class{shutdown}
 */
void ExecutionControl::shutdown_mode_transition()
{

   // Only the Master federate has any IMSim tasks for shutdown.
   if ( !is_master() ) {
      return;
   }

   // If the current execution mode is uninitialized then we haven't gotten
   // far enough in the initialization process to shutdown anything.
   if ( this->current_execution_control_mode == EXECUTION_CONTROL_UNINITIALIZED ) {
      return;
   }

   /*TODO: Implement for IMSim

   // Register the 'mtr_shutdown' sync-point.
   register_sync_point( *( federate->get_RTI_ambassador() ), IMSim::MTR_SHUTDOWN_SYNC_POINT );
   */
}

void ExecutionControl::enter_freeze()
{
   // DANNY2.7 send a freeze interaction when master hits Sim Control Panel
   //  Freeze button. Determine if I am the federate that clicked Freeze
   if ( get_sim_time() <= 0.0 ) {
      set_freeze_announced( is_master() );
   } else if ( !is_freeze_pending() ) {
      set_freeze_announced( true );
   }

   if ( is_freeze_announced() ) {
      // Send interaction unless: we are here because we are at the freeze
      // interaction time, or we are here because we started in freeze
      if ( ( !is_freeze_pending() ) && ( get_sim_time() > 0.0 ) ) {
         double freeze_scenario_time = -DBL_MAX; // freeze immediately

         if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL,
                             "IMSim::ExecutionControl::enter_freeze():%d announce_freeze:%s, freeze_federation:%s, freeze_scenario_time:%g \n",
                             __LINE__, ( is_freeze_announced() ? "Yes" : "No" ),
                             ( is_freeze_pending() ? "Yes" : "No" ),
                             freeze_scenario_time );
         }

         trigger_freeze_interaction( freeze_scenario_time );

         set_freeze_pending( true ); // TEMP

         // TEMP   federate->un_freeze(); // will freeze again for real when we hit the freeze interaction time
      }
   }
}

bool ExecutionControl::check_freeze_exit()
{
   // If freeze has been announced and we are not in initialization then return true.
   if ( is_freeze_announced() && ( get_sim_time() <= 0.0 ) ) {
      return ( true );
   }
   return ( false );
}

void ExecutionControl::exit_freeze()
{
   if ( is_freeze_announced() ) {                              // DANNY2.7
      if ( is_freeze_pending() && ( get_sim_time() > 0.0 ) ) { // coming out of freeze due to freeze interaction

         // TODO: remove federate->register_generic_sync_point( IMSim::FEDRUN_SYNC_POINT ); // this tells federates to go to run
      }
      if ( is_freeze_pending() ) {      // coming out of freeze due to interaction OR sync point at init time
         set_freeze_announced( false ); // reset for the next time we freeze
      }
   }
}

void ExecutionControl::check_pause( double const check_pause_delta )
{
   // For IMSim, check_pause is only used at init time to handle start in freeze mode.
   if ( get_sim_time() > 0.0 ) {
      return;
   }

   if ( is_freeze_pending() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "IMSim::ExecutionControl::check_pause():%d Commanding Trick Executive to FREEZE.\n",
                          __LINE__ );
      }
      if ( get_sim_time() <= 0.0 ) {
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
   manager->get_execution_control()->check_pause_at_init( check_pause_delta );
}

ExecutionConfiguration *ExecutionControl::get_execution_configuration()
{
   ExecutionConfiguration *ExCO = dynamic_cast< ExecutionConfiguration * >( ExecutionControlBase::get_execution_configuration() );
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionControl::get_execution_configuration():" << __LINE__
             << " ERROR: Execution Configuration base is not an IMSim::ExecutionConfiguration instance.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return ( ExCO );
}

void ExecutionControl::start_federation_save_at_scenario_time(
   double      freeze_scenario_time,
   char const *file_name )
{
   if ( freeze_interaction->get_handler() != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "IMSim::ExecutionControl::start_federation_save_at_scenario_time(%g, '%s'):%d\n",
                          freeze_scenario_time, file_name, __LINE__ );
      }
      federate->set_announce_save();

      double new_scenario_time = freeze_scenario_time;

      trigger_freeze_interaction( new_scenario_time );

      manager->initiate_federation_save( file_name );
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "IMSim::ExecutionControl::start_federation_save_at_scenario_time(%g, '%s'):%d \
freeze_interaction's HANLDER is NULL! Request was ignored!\n",
                          freeze_scenario_time, file_name, __LINE__ );
      }
   }
}

void ExecutionControl::add_freeze_scenario_time(
   double t )
{
   if ( manager->is_late_joining_federate() ) {

      if ( federate->get_announce_save() ) {
         freeze_scenario_times.insert( t );
      } else {
         // If we received the interaction, save on the current frame.
         freeze_scenario_times.insert( get_scenario_time() );
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
      static_cast< FreezeInteractionHandler * >( freeze_interaction->get_handler() );

   freeze_intr->send_scenario_freeze_interaction( new_freeze_time, is_late_joiner() );

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
      exec_freeze(); // go to freeze at top of next frame (other federates MUST have their software frame set in input.py file!)
      // If we are to initiate the federation save, register a sync point
      // which must be acknowledged only in freeze mode!!!
      if ( federate->get_announce_save() ) {
         register_sync_point( IMSim::FEDSAVE_SYNC_POINT );
         set_freeze_announced( true );
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

         FreezeTimeSet::const_iterator iter = freeze_scenario_times.begin();

         double freeze_time = *iter;

         // Get the current Trick sim-time.
         double curr_sim_time = get_sim_time();

         // Get the current scenario-time.
         double curr_scenario_time = get_scenario_time();

         // Jump to Trick Freeze mode if the current scenario time is greater
         // than or equal to the requested freeze scenario time.
         if ( curr_scenario_time >= freeze_time ) {
            found_valid_freeze_time = true;
            do_immediate_freeze     = true;
            freeze_scenario_times.erase( iter );
            set_freeze_pending( true );

            if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               // Determine the freeze simulation-time for the equivalent freeze
               // scenario-time.
               double freeze_sim_time = curr_sim_time + ( freeze_time - curr_scenario_time );

               ostringstream infomsg;
               infomsg << "IMSim::ExecutionControl::check_scenario_freeze_time():" << __LINE__
                       << " Going to Trick FREEZE mode immediately:\n";
               if ( federate->is_time_management_enabled() ) {
                  infomsg << "  Granted HLA-time:"
                          << federate->get_granted_time().get_time_in_seconds()
                          << '\n';
               }
               infomsg << "  Trick sim-time:" << curr_sim_time << '\n'
                       << "  Freeze sim-time:" << freeze_sim_time << '\n'
                       << "  Current scenario-time:" << curr_scenario_time << '\n'
                       << "  Freeze scenario-time:" << freeze_time << '\n';
               message_publish( MSG_NORMAL, infomsg.str().c_str() );
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
   if ( federate->get_announce_save()
        && !federate->get_initiate_save_flag()
        && !federate->get_save_completed() ) {
      register_sync_point( IMSim::FEDSAVE_SYNC_POINT );

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer;

      while ( !federate->get_initiate_save_flag() ) { // wait for federation to be synced

         sleep_timer.sleep();

         if ( !federate->get_initiate_save_flag() ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "IMSim::ExecutionControl::setup_checkpoint():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "IMSim::ExecutionControl::setup_checkpoint():%d Waiting '%s'\n",
                                __LINE__ );
            }
         }
      }
   }
   return ( true );
}

bool ExecutionControl::perform_save()
{
   if ( federate->get_announce_save()
        && federate->get_initiate_save_flag()
        && !federate->get_start_to_save() ) {
      // We are here because user called start_federation_save, so
      // must force the perform_checkpoint code to execute.
      federate->set_announce_save( false );
      return ( true );
   }

   return ( false );
}
