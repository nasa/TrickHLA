/*!
@file TrickHLA/ExecutionControlBase.cpp
@ingroup TrickHLA
@brief This class provides and abstract base class as the base implementation
for managing mode transitions.

@copyright Copyright 2026 United States Government as represented by the
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
@trick_link_dependency{ExecutionConfigurationBase.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{ObjectServices.cpp}
@trick_link_dependency{SaveRestoreServices.cpp}
@trick_link_dependency{SyncPointManagerBase.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{time/CTETimelineBase.cpp}
@trick_link_dependency{time/Int64BaseTime.cpp}
@trick_link_dependency{time/ScenarioTimeline.cpp}
@trick_link_dependency{time/SimTimeline.cpp}
@trick_link_dependency{utils/SleepTimeout.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, TrickHLA support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, May 2026, --, Adjustments for SaveRestore support.}
@revs_end

*/

// System includes.
#include <cstdint>
#include <cstring>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

// Trick includes.
#include "trick/CheckPointRestart.hh"
#include "trick/Clock.hh"
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/sim_mode.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/ObjectServices.hh"
#include "TrickHLA/SaveRestoreServices.hh"
#include "TrickHLA/SyncPointList.hh"
#include "TrickHLA/SyncPointManagerBase.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/CTETimelineBase.hh"
#include "TrickHLA/time/Int64BaseTime.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/time/ScenarioTimeline.hh"
#include "TrickHLA/time/SimTimeline.hh"
#include "TrickHLA/utils/SleepTimeout.hh"
#include "TrickHLA/utils/StringUtilities.hh"

#if defined( IEEE_1516_2010 )
// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Handle.h"
#include "RTI/RTIambassador.h"
#include "RTI/Typedefs.h"

// Access the Trick global objects for CheckPoint restart and the Clock.
extern Trick::CheckPointRestart *the_cpr;

// Access the Trick global objects the Clock.
extern Trick::Clock *the_clock;

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

// Declare default time lines.
namespace TrickHLA
{
SimTimeline      def_sim_timeline;
ScenarioTimeline def_scenario_timeline( def_sim_timeline );
} // namespace TrickHLA

/*!
 * @job_class{initialization}
 */
ExecutionControlBase::ExecutionControlBase()
   : SyncPointManagerBase(),
     scenario_timeline( &def_scenario_timeline ),
     sim_timeline( &def_sim_timeline ),
     cte_timeline( NULL ),
     use_preset_master( false ),
     master( false ),
     multiphase_init_sync_points(),
     time_padding( 0.0 ),
     enable_least_common_time_step( false ),
     least_common_time_step_seconds( -1.0 ),
     least_common_time_step( -1 ),
     execution_configuration( NULL ),
     mode_transition_requested( false ),
     requested_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     current_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     next_mode_scenario_time( std::numeric_limits< double >::lowest() ),
     next_mode_cte_time( std::numeric_limits< double >::lowest() ),
     simulation_freeze_time( 0.0 ),
     scenario_freeze_time( 0.0 ),
     announce_freeze( false ),
     freeze_the_federation( false ),
     late_joiner( false ),
     late_joiner_determined( false ),
     time_management_service( NULL ),
     object_service( NULL ),
     interaction_service( NULL ),
     save_restore_service( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
ExecutionControlBase::ExecutionControlBase(
   ExecutionConfigurationBase &exec_config )
   : SyncPointManagerBase(),
     scenario_timeline( &def_scenario_timeline ),
     sim_timeline( &def_sim_timeline ),
     cte_timeline( NULL ),
     use_preset_master( false ),
     master( false ),
     multiphase_init_sync_points(),
     time_padding( 0.0 ),
     enable_least_common_time_step( false ),
     least_common_time_step_seconds( -1.0 ),
     least_common_time_step( -1 ),
     execution_configuration( &exec_config ),
     mode_transition_requested( false ),
     requested_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     current_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     next_mode_scenario_time( std::numeric_limits< double >::lowest() ),
     next_mode_cte_time( std::numeric_limits< double >::lowest() ),
     simulation_freeze_time( 0.0 ),
     scenario_freeze_time( 0.0 ),
     announce_freeze( false ),
     freeze_the_federation( false ),
     late_joiner( false ),
     late_joiner_determined( false ),
     time_management_service( NULL ),
     object_service( NULL ),
     interaction_service( NULL ),
     save_restore_service( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ExecutionControlBase::~ExecutionControlBase()
{
   return;
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - The TrickHLA::ExecutionConfigurationBase class is actually an abstract class.
 * Therefore, the actual object instance being passed in is an instantiable
 * polymorphic child of the TrickHLA::ExecutionConfigurationBase class.
 *
 * @job_class{default_data}
 */
void ExecutionControlBase::setup(
   TrickHLA::Federate &fed )
{
   // Set the TrickHLA::Federate instance reference that exists in the
   // SyncPointManagerBase subclass we extended.
   SyncPointManagerBase::setup( &fed );

   // Set the TrickHLA::Manager instance reference.
   this->object_service = fed.get_object_service();

   // Set the TrickHLA::SaveRestoreServices instance reference.
   this->save_restore_service = fed.get_save_restore_service();

   // Set the TrickHLA::TimeManagementServices instance reference.
   this->time_management_service = fed.get_time_management_service();

   // Set the TrickHLA::InteractionServices instance reference.
   this->interaction_service = fed.get_interaction_service();

   // Set the TrickHLA::ExecutionConfigurationBase instance reference.
   this->execution_configuration = fed.get_execution_configuration();

   // Check to see if the ExecutionConfigurationBase instance is set.
   // NOTE: This should always be set!!!
   if ( this->execution_configuration != NULL ) {

      // Setup the TrickHLA::ExecutionConfigurationBase instance.
      execution_configuration->setup( *this );

      // Configure the default Execution Configuration attributes.
      execution_configuration->configure_attributes();
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::initialize()
{
   // Verify the CTE clock if used.
   if ( does_cte_timeline_exist() ) {

      if ( cte_timeline != the_clock ) {
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::initialize():" << __LINE__
                << " ERROR: The CTE timeline is specified, but it is not"
                << " configured as the Trick real time clock! Make sure"
                << " the CTETimelineBase class constructor is calling"
                << " real_time_change_clock( this );" << endl;
         DebugHandler::terminate( errmsg.str() );
      }

      // Make sure to update the clock resolution so that it uses the
      // latest Trick executive time tic value, which may have changed
      // by a setting in the input.py file. Clock time resolution is
      // maintained separately from the Trick executive time resolution,
      // which is why we need to explicitly update it.
      cte_timeline->set_clock_tics_per_sec( exec_get_time_tic_value() );
   }

   // Reset the master flag if it is not preset by the user.
   if ( !is_master_preset() ) {
      set_master( false );
   }

   // If the padding time is not set then automatically adjust it
   // to be an integer multiple of the LCTS.
   if ( this->time_padding <= 0.0 ) {
      if ( this->least_common_time_step > 0 ) {

         int64_t const pad_base_time = Int64BaseTime::to_base_time( THLA_PADDING_DEFAULT );

         // Set a padding time that is around 0.5 seconds that is also an
         // integer multiple of the LCTS.
         set_time_padding( Int64BaseTime::to_seconds(
            this->least_common_time_step * ( ( pad_base_time / this->least_common_time_step ) + 1 ) ) );
      } else {
         set_time_padding( THLA_PADDING_DEFAULT );
      }
   }

   // Verify the time constraints for the federate.
   if ( ( federate != NULL ) && !federate->time_management_service.verify_time_constraints() ) {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::initialize():" << __LINE__
             << " ERROR: Time constraints verification failed!" << endl;
      DebugHandler::terminate( errmsg.str() );
   }

   if ( !does_scenario_timeline_exist() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::initialize():%d WARNING: \
ExecutionControl 'scenario_timeline' not specified in the input.py file. Using the \
Trick simulation time as the default scenario-timeline.\n",
                          __LINE__ );
      }

      // Use the simulation timeline as the default scenario timeline.
      scenario_timeline = &def_scenario_timeline;
      if ( scenario_timeline == NULL ) { // cppcheck-suppress [knownConditionTrueFalse]
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::initialize():" << __LINE__
                << " FAILED to allocate enough memory for ScenarioTimeline class!"
                << endl;
         DebugHandler::terminate( errmsg.str() );
         return;
      }
   }

   // Depending on if Save and Restore is supported, set the intial state.
   if ( this->is_save_and_restore_supported() ) {
      this->save_restore_service->save_set_state( THLASaveProcessEnum::SAVE_NONE );
      this->save_restore_service->restore_set_state( THLARestoreProcessEnum::RESTORE_NONE );
   } else {
      this->save_restore_service->save_set_state( THLASaveProcessEnum::SAVE_UNSUPPORTED );
      this->save_restore_service->restore_set_state( THLARestoreProcessEnum::RESTORE_UNSUPPORTED );
   }

   // Initialize then Configure the ExecutionConfiguration object if present.
   if ( execution_configuration != NULL ) {
      execution_configuration->Object::initialize( this->federate );
      execution_configuration->configure();
   }
}

/*!
@job_class{initialization}
*/
void ExecutionControlBase::join_federation_process()
{
   TrickHLA::Federate *fed = get_federate();

   // Create the RTI Ambassador and connect.
   fed->create_RTI_ambassador_and_connect();

   // Destroy the federation if it was orphaned from a previous simulation
   // run that did not shutdown cleanly.
   fed->destroy_orphaned_federation();

   // All federates try to create the federation then join it because we use
   // a preset master.
   fed->create_and_join_federation();

   // Don't forget to enable asynchronous delivery of messages.
   fed->enable_async_delivery();

   // Check for a latent shutdown sync-point.
   // If shutdown sync-point is detected, then we must have entered into
   // a running federation execution that is shutting down. This is an
   // unlikely but possible race condition.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "ExecutionControl::join_federation_process():%d Checking for shutdown\n",
                       __LINE__ );
   }
   fed->check_for_shutdown_with_termination();
}

/*!
@job_class{initialization}
*/
bool ExecutionControlBase::object_instance_name_reservation_succeeded(
   wstring const &obj_instance_name )
{
   // If ExecutionConfiguration is not set, then there is no match.
   if ( execution_configuration != NULL ) {

      // We need the wide-string version of the ExCO name.
      wstring ws_exec_config_name;
      StringUtilities::to_wstring( ws_exec_config_name, execution_configuration->get_name() );

      // Check to see if the ExecutionConfiguration object instance matches this
      // object instance name.
      if ( obj_instance_name == ws_exec_config_name ) {

         // We are the Master federate if we succeeded in reserving the
         // ExecutionConfiguration object name and the master was not preset.
         if ( !is_master_preset() ) {
            set_master( true );
         }

         // The name is successfully registered.
         execution_configuration->set_name_registered();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            string name_str;
            StringUtilities::to_string( name_str, obj_instance_name );
            message_publish( MSG_NORMAL, "ExecutionControlBase::object_instance_name_reservation_succeeded():%d Name:'%s'\n",
                             __LINE__, name_str.c_str() );
         }

         return true;
      }
   }
   return false;
}

/*!
@job_class{initialization}
*/
bool ExecutionControlBase::object_instance_name_reservation_failed(
   wstring const &obj_instance_name )
{
   // If ExecutionConfiguration is not set, then there is no match.
   if ( execution_configuration == NULL ) {
      return false;
   }

   wstring ws_exec_config_name;
   StringUtilities::to_wstring( ws_exec_config_name, execution_configuration->get_name() );

   // The default ExecutionControl behavior is to handle the ExecutionConfiguration
   // instance name reservation failure to help determine the master.
   if ( obj_instance_name == ws_exec_config_name ) {

      // If this is not designated as the preset Master federate, then we are
      // NOT the Master federate since we failed to reserve the ExecutionControl
      // object instance name.
      if ( !is_master_preset() ) {
         set_master( false );
      } else { // If this is the designated preset Master federate, then this is an ERROR.
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::object_instance_name_reservation_failed:" << __LINE__
                << " FAILED to reserve the ExecutionConfiguration object instance name: '"
                << execution_configuration->get_name()
                << "'! This conflicts with this being the designated Master federate!"
                << endl;
         DebugHandler::terminate( errmsg.str() );
      }

      // We failed to register the ExecutionConfiguration object instance name
      // which means that another federate has already registered it.
      execution_configuration->set_name_registered();

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         string name_str;
         StringUtilities::to_string( name_str, obj_instance_name );
         message_publish( MSG_NORMAL, "ExecutionControlBase::object_instance_name_reservation_failed():%d Name:'%s'\n",
                          __LINE__, name_str.c_str() );
      }

      // We found a match to return 'true'.
      return true;
   }

   return false;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::register_objects_with_RTI()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "ExecutionControlBase::register_objects_with_RTI():%d\n",
                       __LINE__ );
   }

   // Register any ExecutionConfiguration objects.
   if ( execution_configuration != NULL ) {

      // Register the execution configuration object.
      execution_configuration->register_object_with_RTI();

      // Place the ExecutionConfiguration object into the ObjectServices's object map.
      add_object_to_map( execution_configuration );
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::add_object_to_map(
   Object *object )
{
   // Add the registered ExecutionConfiguration object instance to the map.
   object_service->add_object_to_map( object );
}

/*!
 * @brief Is the specified sync-point label contained in the multiphase init
 *  sync-point list.
 * @param sync_point_label Name of the synchronization point label.
 * @return True if the multiphase init sync-point list contains the sync-point,
 *  false otherwise.
 */
bool ExecutionControlBase::contains_multiphase_init_sync_point(
   wstring const &sync_point_label )
{
   return contains_sync_point( sync_point_label, TrickHLA::MULTIPHASE_INIT_SYNC_POINT_LIST );
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::add_multiphase_init_sync_points()
{
   // Add the user specified initialization synchronization points.
   // Parse the comma separated list of sync-point labels.
   vector< string > user_sync_pt_labels;
   if ( !multiphase_init_sync_points.empty() ) {
      StringUtilities::tokenize( this->multiphase_init_sync_points, user_sync_pt_labels, "," );
   }

   // Add the user specified multiphase initialization sync-points to the list.
   for ( unsigned int i = 0; i < user_sync_pt_labels.size(); ++i ) {
      wstring ws_label;
      StringUtilities::to_wstring( ws_label, user_sync_pt_labels[i] );
      if ( contains_sync_point( ws_label ) ) {
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::add_multiphase_init_sync_points:" << __LINE__
                << " ERROR: User specified multiphase init sync-point label '"
                << user_sync_pt_labels[i] << "' already added!" << endl;
         DebugHandler::terminate( errmsg.str() );
      } else {
         add_sync_point( ws_label, TrickHLA::MULTIPHASE_INIT_SYNC_POINT_LIST );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::clear_multiphase_init_sync_points()
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( federate->is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::clear_multiphase_init_sync_points():%d Late \
joining federate so this call will be ignored.\n",
                          __LINE__ );
      }
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "ExecutionControlBase::clear_multiphase_init_sync_points():%d\n",
                       __LINE__ );
   }

   // Achieve all the multiphase initialization synchronization points except.
   achieve_all_multiphase_init_sync_points();

   // Now wait for all the multiphase initialization sync-points to be
   // synchronized in the federation.
   wait_for_all_multiphase_init_sync_points();

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      print_sync_points();
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::achieve_all_multiphase_init_sync_points()
{
   // Iterate through this ExecutionControl's user defined multiphase
   // initialization synchronization point list and achieve them.
   achieve_all_sync_points( TrickHLA::MULTIPHASE_INIT_SYNC_POINT_LIST );
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::wait_for_all_multiphase_init_sync_points()
{
   // Wait for all the user defined multiphase initialization synchronization
   // points to be achieved.
   wait_for_all_sync_points_synchronized( TrickHLA::MULTIPHASE_INIT_SYNC_POINT_LIST );
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::send_execution_configuration()
{
   if ( execution_configuration == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::send_execution_configuration():%d This call \
will be ignored because the Simulation Initialization Scheme does not support it.\n",
                          __LINE__ );
      }
      return;
   }

   // Only the master federate can send the ExecutionConfiguration.
   if ( !is_master() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "ExecutionControlBase::send_ssend_execution_configurationim_config():%d\n", __LINE__ );
   }

   // Make sure we have at least one piece of ExecutionConfiguration data we can send.
   if ( execution_configuration->any_locally_owned_published_init_attribute() ) {

      // Send the ExecutionConfiguration data to the other federates.
      execution_configuration->send_init_data();

   } else {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::send_execution_configuration():" << __LINE__
             << " ERROR: ExecutionConfiguration"
             << " is not configured to send at least one object attribute. Make"
             << " sure at least one ExecutionConfiguration attribute has 'publish = true'"
             << " set. Please check your input or modified-data files to make"
             << " sure the 'publish' value is correctly specified." << endl;
      DebugHandler::terminate( errmsg.str() );
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::receive_execution_configuration()
{
   if ( execution_configuration == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::receive_execution_configuration():%d This call \
will be ignored because the Simulation Initialization Scheme does not support it.\n",
                          __LINE__ );
      }
      return;
   }

   // We can only receive the ExecutionConfiguration if we are not the master.
   if ( is_master() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "ExecutionControlBase::receive_execution_configuration():%d Waiting...\n",
                       __LINE__ );
   }

   // Make sure we have at least one piece of ExecutionConfiguration data we can receive.
   if ( execution_configuration->any_remotely_owned_subscribed_init_attribute() ) {

      SleepTimeout print_timer;
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the data to arrive.
      while ( !execution_configuration->is_changed() ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !execution_configuration->is_changed() ) {

            // To be more efficient, we get the time once and share it.
            int64_t const wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "ExecutionControlBase::receive_execution_configuration():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!" << endl;
                  DebugHandler::terminate( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "ExecutionControlBase::receive_execution_configuration():%d Waiting...\n",
                                __LINE__ );
            }
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::receive_execution_configuration():%d Received data.\n",
                          __LINE__ );
      }

      // Receive the ExecutionConfiguration data from the master federate.
      execution_configuration->receive_init_data();

   } else {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::receive_execution_configuration():" << __LINE__
             << " ERROR: ExecutionConfiguration"
             << " is not configured to receive at least one object attribute."
             << " Make sure at least one ExecutionConfiguration attribute has"
             << " 'subscribe = true' set. Please check your input or modified-data"
             << " files to make sure the 'subscribe' value is correctly specified."
             << endl;
      DebugHandler::terminate( errmsg.str() );
   }
}

/*!
 * @job_class{scheduled}
 */
void ExecutionControlBase::send_requested_data(
   Int64Time const &update_time )
{
   // Send the requested data for the ExecutionConfiguration if we have one.
   if ( execution_configuration != NULL ) {
      // Send the data for the execution-configuration.
      execution_configuration->send_requested_data( update_time );
   }
}

/*!
 * @job_class{scheduled}
 */
void ExecutionControlBase::receive_cyclic_data()
{
   // Receive the requested data for the ExecutionConfiguration if we have one.
   if ( execution_configuration != NULL ) {

      // Process all the received ExecutionConfiguration data in the
      // buffer/queue, which shows up as changed.
      while ( execution_configuration->is_changed() ) {
         execution_configuration->receive_init_data();
         process_execution_control_updates();
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void ExecutionControlBase::provide_attribute_update(
   ObjectInstanceHandle const &theObject,
   AttributeHandleSet const   &theAttributes )
{
   // If we have an ExecutionConfiguration then provide attribute updates.
   if ( ( execution_configuration != NULL )
        && ( execution_configuration->get_instance_handle() == theObject ) ) {
      execution_configuration->provide_attribute_update( theAttributes );
   }
}

/*!
 * @job_class{scheduled}
 */
Object *ExecutionControlBase::get_trickhla_object(
   string const &obj_instance_name )
{
   // Check to see if there is and ExecutionConfiguration object.
   if ( execution_configuration != NULL ) {

      // Execution Configuration object.
      if ( obj_instance_name == get_execution_configuration()->get_name() ) {
         return ( execution_configuration );
      }
   }

   // Default return if we don't have an ExecutionConfiguration match.
   return ( NULL );
}

/*!
 * @job_class{scheduled}
 */
Object *ExecutionControlBase::get_trickhla_object(
   wstring const &obj_instance_name )
{
   // Check to see if there is and ExecutionConfiguration object.
   if ( execution_configuration != NULL ) {

      // Execution Configuration object.
      wstring ws_obj_name;
      StringUtilities::to_wstring( ws_obj_name, get_execution_configuration()->get_name() );
      if ( ws_obj_name == obj_instance_name ) {
         return ( execution_configuration );
      }
   }

   // Default return if we don't have an ExecutionConfiguration match.
   return ( NULL );
}

/*!
 * @job_class{scheduled}
 */
Object *ExecutionControlBase::get_unregistered_object(
   ObjectClassHandle const &theObjectClass,
   wstring const           &theObjectInstanceName )
{
   // Check to see if there is and ExecutionConfiguration object.
   if ( execution_configuration != NULL ) {

      // Check the execution configuration next.
      if ( ( execution_configuration->get_class_handle() == theObjectClass )
           && ( !execution_configuration->is_instance_handle_valid() ) ) {

         wstring ws_obj_name;
         StringUtilities::to_wstring( ws_obj_name, get_execution_configuration()->get_name() );

         // Determine if the name matches the object instance name.
         if ( ws_obj_name == theObjectInstanceName ) {
            return ( execution_configuration );
         }
      }
   }

   // Default return if we don't have an ExecutionConfiguration match.
   return ( NULL );
}

/*!
 * @job_class{scheduled}
 */
Object *ExecutionControlBase::get_unregistered_remote_object(
   ObjectClassHandle const &theObjectClass )
{
   // Check to see if there is and ExecutionConfiguration object.
   if ( execution_configuration != NULL ) {
      // See if we have a match with the ExecutionConfigruation object.
      if ( ( !execution_configuration->is_create_HLA_instance() )
           && ( execution_configuration->get_class_handle() == theObjectClass )
           && ( !execution_configuration->is_instance_handle_valid() )
           && ( !execution_configuration->is_name_required()
                || execution_configuration->get_name().empty() ) ) {
         return ( execution_configuration );
      }
   }

   // Default return if we don't have an ExecutionConfiguration match.
   return ( NULL );
}

bool ExecutionControlBase::mark_object_as_deleted_from_federation(
   ObjectInstanceHandle const &instance_id )
{
   // Remove the ExecitionControl object if present and the ID matches.
   if ( execution_configuration != NULL
        && ( execution_configuration->get_instance_handle() == instance_id ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_id );
         message_publish( MSG_NORMAL, "ExecutionControlBase::mark_object_as_deleted_from_federation():%d Object '%s' Instance-ID:%s Valid-ID:%s\n",
                          __LINE__, execution_configuration->get_name().c_str(), id_str.c_str(),
                          ( instance_id.isValid() ? "Yes" : "No" ) );
      }
      execution_configuration->remove_object_instance();
      return true;
   }
   return false;
}

/*!
 * @job_class{logging}
 */
void ExecutionControlBase::process_deleted_objects()
{
   // Process ExecutionConfiguration deletion if we have one.
   if ( execution_configuration != NULL ) {
      if ( execution_configuration->process_object_deleted_from_RTI ) {
         execution_configuration->process_deleted_object();
      }
   }
}

double ExecutionControlBase::get_sim_time() const
{
   if ( does_sim_timeline_exist() ) {
      return sim_timeline->get_time();
   }

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::get_sim_time():" << __LINE__
             << " WARNING: The simulation timeline has not been set!"
             << " Please make sure you specify a sim-timeline in your input"
             << " file. Returning Trick simulation time instead!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }
   return exec_get_sim_time();
}

double ExecutionControlBase::get_scenario_time() const
{
   if ( does_scenario_timeline_exist() ) {
      return scenario_timeline->get_time();
   }

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::get_scenario_time():" << __LINE__
             << " WARNING: The scenario timeline has not been set!"
             << " Please make sure you specify a scenario timeline in your input"
             << " file. Returning simulation elapsed time instead!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }
   return get_sim_time();
}

double ExecutionControlBase::get_cte_time() const
{
   return does_cte_timeline_exist() ? cte_timeline->get_time()
                                    : std::numeric_limits< double >::lowest();
}

void ExecutionControlBase::clear_mode_values()
{
   this->mode_transition_requested        = false;
   this->requested_execution_control_mode = EXECUTION_CONTROL_UNINITIALIZED;
   this->current_execution_control_mode   = EXECUTION_CONTROL_UNINITIALIZED;
}

/*!
 * @job_class{shutdown}
 */
bool ExecutionControlBase::check_for_shutdown()
{
   return false;
}

/*!
 * @details NOTE: If a shutdown has been announced, this routine calls the
 * Trick exec_teminate() function. So, for shutdown, it should never return.
 * @job_class{shutdown}
 */
bool ExecutionControlBase::check_for_shutdown_with_termination()
{
   return false;
}

void ExecutionControlBase::freeze_init()
{
   // Reset the process timer.
   process_timer.reset();

   // Wait while we get an updated joined federate list.
   federate->wait_for_joined_federates_update();

   return;
}

void ExecutionControlBase::enter_freeze()
{
   // The default is to do nothing.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "ExecutionControlBase::enter_freeze():%d Freeze Announced:%s, Freeze Pending:%s\n",
                       __LINE__, ( is_freeze_announced() ? "Yes" : "No" ),
                       ( is_freeze_pending() ? "Yes" : "No" ) );
   }
}

bool ExecutionControlBase::check_freeze_exit()
{
   return false;
}

void ExecutionControlBase::exit_freeze()
{
   return;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void ExecutionControlBase::check_pause( double const check_pause_delta )
{
   return;
}
#pragma GCC diagnostic pop

void ExecutionControlBase::check_pause_at_init( double const check_pause_delta )
{
   // Dispatch to the ExecutionControl method.
   check_pause( check_pause_delta );

   // Mark that freeze has been announced in the Federate.
   set_freeze_announced( is_master() );
}

void ExecutionControlBase::set_master( bool master_flag )
{
   // Don't change the master flag setting if the user has preset a value
   // in the input.py file.
   if ( !is_master_preset() ) {
      this->master = master_flag;
      // Make sure that the Execution Configuration object is set properly.
      if ( execution_configuration != NULL ) {
         execution_configuration->set_master( master_flag );
      }
   }
}

//-------------------------------------------------------------------------
// Save and Restore
//-------------------------------------------------------------------------

/*!
 *  @job_class{scheduled}
 *  @detail The default behavior is to use the current granted time as the
 *  Save label.  This can be overridden in any extending ExecutionControl
 *  class.
 */
wstring ExecutionControlBase::generate_save_label()
{
   // FIXME: Check for time management to insure that there is a granted time.

   // Get the current HLA Logical Time.
   int64_t const granted_time = time_management_service->get_granted_time().get_base_time();

   // Formulate the save label based on the Federation Execution name and the
   // current HLA logical time.
   std::string const save_label_str = std::to_string( granted_time );

   // Convert the label to a wide string.
   wstring save_label_wstr;
   StringUtilities::to_wstring( save_label_wstr, save_label_str );

   // Return the Save label.
   return ( save_label_wstr );
}

/*
 * @job_class{scheduled}
 */
std::string const ExecutionControlBase::map_save_label_to_federates_file_name(
   wstring const &save_label )
{
   std::string save_label_str;
   std::string federates_file_name;

   // Convert the Save label wstring to a string.
   StringUtilities::to_string( save_label_str, save_label );

   // Build up the checkpoint file name.
   // First get the federation name.
   federates_file_name = federate->get_federation_name();
   federates_file_name += "_";
   // Next get the federate name.
   federates_file_name += federate->get_federate_name();
   federates_file_name += "_";
   // Add the specified HLA Save label.
   federates_file_name += save_label_str;
   // Add the running_feds suffix.
   federates_file_name += ".feds";

   return ( federates_file_name );
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::save_process()
{
   std::string save_label_str;

   // NOTE: The Save label is assumed to be set outside this function in the
   // SaveRestroreService.

   // Convert the save label for use in messages.
   StringUtilities::to_string( save_label_str, save_restore_service->save_get_label() );

   // Manage the Federate HLA Save process state.
   switch ( save_restore_service->save_state ) {

      case THLASaveProcessEnum::SAVE_NONE:
         // Save has not been initiated.  So, just proceed without action.
         break;

      case THLASaveProcessEnum::SAVE_INITIATED:
         // This federate initiated the Federation Save.
         // Make the call to the RTI ambassador to request a Federation save.
         save_restore_service->save_request();
         break;

      case THLASaveProcessEnum::SAVE_REQUESTED:
         // This federate is responding to a Save Request from the Federation.

         // Check the currency of the joined federates.
         if ( federate->verify_joined_federates() ) {

            // Call the SaveRestoreServices Save method.
            save_restore_service->save();

         } else {

            // The Save failed.
            if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               message_publish( MSG_ERROR, "ExecutionControlBase::save_process():%d Save: \'%s\' failed!\n",
                                __LINE__, save_label_str.c_str() );
            }
            // Save actions when Save failed.
            save_restore_service->save_state = THLASaveProcessEnum::SAVE_FAILED;
            save_restore_service->save_failed();
         }

         break;

      case THLASaveProcessEnum::SAVE_IN_PROGRESS:
         // A Save is in progress.  This routine checks status while waiting for
         // the save to complete.
         save_restore_service->save_in_progress_check();
         break;

      case THLASaveProcessEnum::SAVE_COMPLETE:
         // The Save was successfully completed.
         if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL, "ExecutionControlBase::save_process():%d Save: \'%s\' completed!\n",
                             __LINE__, save_label_str.c_str() );
         }
         // Save actions when Save completed successfully.
         save_restore_service->save_succeded();

         // Reset the Save state to SAVE_NONE.
         save_restore_service->save_state = THLASaveProcessEnum::SAVE_NONE;
         break;

      case THLASaveProcessEnum::SAVE_FAILED:
         // The Save failed.
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_ERROR, "ExecutionControlBase::save_process():%d Save: \'%s\' failed!\n",
                             __LINE__, save_label_str.c_str() );
         }
         // Save actions when Save failed.
         save_restore_service->save_failed();
         break;

      case THLASaveProcessEnum::SAVE_UNSUPPORTED:
         // Save is not supported.  So, just proceed without action.
         break;

      default:
         // Unknown Save state.  This is bad, so exit with error.
         ostringstream errmsg;
         errmsg << "Federate::freeze_save():" << __LINE__
                << " ERROR: Unknown Save state = "
                << static_cast< int >( save_restore_service->save_state ) << endl;
         DebugHandler::terminate( errmsg.str() );
         break;
   }

   return;
}

/*!
 *  @job_class{freeze}
 */
bool ExecutionControlBase::save( wstring const &label )
{
   THLASaveProcessEnum current_save_state;
   std::string         current_save_state_str;

   // Get the current Save state.
   current_save_state = save_restore_service->save_get_state();

   // If Federation SaveRestore is not supported then return without action.
   if ( current_save_state == THLASaveProcessEnum::SAVE_UNSUPPORTED ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::save():%d HLA SaveRetore NOT supported!\n",
                          __LINE__ );
      }

      return ( false );
   }

   // Get the Save state string for use in messages.
   current_save_state_str = TrickHLA::to_string( current_save_state );

   // Check the Federation Save state to ensure that a Save is applicable .
   if ( ( current_save_state != THLASaveProcessEnum::SAVE_NONE )
        && ( current_save_state != THLASaveProcessEnum::SAVE_REQUESTED ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::save():%d : Save already in progress: \'%s\'!\n",
                          __LINE__, current_save_state_str.c_str() );
      }

      return ( false );
   }

   // Check to see if we are initiating the Save.
   if ( current_save_state == THLASaveProcessEnum::SAVE_NONE ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::save():%d : Initiating Save: \'%s\'!\n",
                          __LINE__, current_save_state_str.c_str() );
      }

      // We are initiating the Save.
      save_restore_service->save_request( label );

   } else if ( current_save_state == THLASaveProcessEnum::SAVE_REQUESTED ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::save():%d : Save Requested: \'%s\'!\n",
                          __LINE__, current_save_state_str.c_str() );
      }

      // We have been requested to Save.
      save_restore_service->save( label );
   }

   return ( true );
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::save_at_SET(
   wstring const &label,
   double         sim_time )
{
   // If Federation SaveRestore is not supported then return without action.
   if ( !is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::save_at_SET():" << __LINE__
                << " ERROR: SaveRetore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl
                << " sim_time:" << sim_time << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // FIXME: Start the Save process here.

   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::save_at_SST(
   wstring const &label,
   double         scenario_time )
{
   // If Federation SaveRestore is not supported then return without action.
   if ( !is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::save_at_SST():" << __LINE__
                << " ERROR: SaveRetore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl
                << " scenario_time:" << scenario_time << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // FIXME: Start the Save process here.

   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::save_at_HLT(
   wstring const                        &label,
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   // If Federation SaveRestore is not supported then return without action.
   if ( !is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         string time_str;
         StringUtilities::to_string( time_str, time.toString() );
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::save_at_HLT():" << __LINE__
                << " ERROR: SaveRetore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl
                << " time:" << time_str << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // FIXME: Start the Save process here.

   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_process()
{
   string restore_label_str;

   // NOTE: The Restore label is assumed to be set outside this function in the
   // SaveRestroreService.

   // Convert the save label for use in messages.
   StringUtilities::to_string( restore_label_str, save_restore_service->restore_get_label() );

   switch ( save_restore_service->restore_state ) {

      case THLARestoreProcessEnum::RESTORE_NONE:
         // Restore process has not been activated.  So, just proceed without action.
         break;

      case THLARestoreProcessEnum::RESTORE_ACTIVATE:
         // This federate is initiating the Federation Restore.
         // The process begins with checking the Federation for a Restore status.
         this->restore_request_status();
         break;

      case THLARestoreProcessEnum::RESTORE_REQUEST_STATUS:
         // Continue checking the Restore status request state.
         this->restore_waiting_for_request_status();
         break;

      case THLARestoreProcessEnum::RESTORE_STATUS_COMPLETE:
         // The Federation Restore status is complete and compatible with
         // proceeding with the Restore process.
         // Make the Restore request to the Federation through the RTI ambassador.
         this->restore_request();
         break;

      case THLARestoreProcessEnum::RESTORE_REQUESTED:
         // This marks the phase in the Restore process where we are waiting for
         // confirmation on the restore request.
         this->restore_waiting_for_request();
         break;

      case THLARestoreProcessEnum::RESTORE_REQUEST_FAILED:
         // The Restore request failed.
         this->restore_request_failed();
         break;

      case THLARestoreProcessEnum::RESTORE_REQUEST_SUCCEEDED:
         // The Restore request succeeded.  This is a transient phase as a
         // FedAmb::federationRestoreBegun() callback should follow shortly.
         this->restore_waiting_for_begun();
         break;

      case THLARestoreProcessEnum::RESTORE_BEGUN:
         // The Federation wide Restore has begun.  This is also a transient phase as
         // an FedAmb::initiateFederateRestore() callback should follow shortly.
         this->restore_waiting_for_initiated();
         break;

      case THLARestoreProcessEnum::RESTORE_INITIATED:
         // The federate Restore has been initiated and we are now waiting for
         // the Trick load checkpoint process to complete.
         this->restore_waiting_for_checkpoint_load();
         break;

      case THLARestoreProcessEnum::RESTORE_CHECKPOINT:
         // The federate Restore has been initiated and we are now waiting for
         // the Trick checkpoint process to complete.
         this->restore_after_checkpoint_load();
         break;

      case THLARestoreProcessEnum::RESTORE_WAITING_COMPLETION:
         // The federate Restore is in progress.  This routine checks status
         // while waiting for the restore to complete.  This phase is entered
         // upon receiveing the FedAmb::initiateFederateRestore() callback and
         // persists until we receive the FedAmb::federationRestored() callback.
         this->restore_waiting_for_completion();
         break;

      case THLARestoreProcessEnum::RESTORE_COMPLETE:
         // The Federation wide Restore was successfully completed.
         if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL, "ExecutionControlBase::restore_process():%d Restore: \'%s\' completed!\n",
                             __LINE__, restore_label_str.c_str() );
         }
         // Restore actions when Restore completed successfully.
         this->restore_succeded();

         // Reset the Restore state to RESTORE_NONE.
         save_restore_service->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
         break;

      case THLARestoreProcessEnum::RESTORE_FAILED:
         // The Restore failed.
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_ERROR, "ExecutionControlBase::restore_process():%d Restore: \'%s\' failed!\n",
                             __LINE__, restore_label_str.c_str() );
         }
         // Restore actions when Restore failed.
         this->restore_failed();
         break;

      case THLARestoreProcessEnum::RESTORE_UNSUPPORTED:
         // Restore is not supported.  So, just proceed without action.
         break;

      default:
         // Unknown Restore state.  This is bad, so exit with error.
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::restore_process():" << __LINE__
                << " ERROR: Unknown Restore state = "
                << static_cast< int >( save_restore_service->restore_state ) << endl;
         DebugHandler::terminate( errmsg.str() );
         break;
   }

   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_request_status()
{
   save_restore_service->restore_request_status();
   return;
}

/*!
 *  @job_class{freeze}
 */
bool ExecutionControlBase::restore_waiting_for_request_status()
{
   save_restore_service->restore_waiting_for_request_status();
   return ( false );
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_request( std::wstring const &label )
{
   save_restore_service->restore_request( label );
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_waiting_for_request()
{
   save_restore_service->restore_waiting_for_request();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_request_failed()
{
   save_restore_service->restore_request_failed();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_waiting_for_begun()
{
   save_restore_service->restore_waiting_for_begun();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_begun()
{
   save_restore_service->restore_begun();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_waiting_for_initiated()
{
   save_restore_service->restore_waiting_for_initiated();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_initiated(
#if defined( IEEE_1516_2025 )
   wstring const        &label,
   wstring const        &federate_name,
   FederateHandle const &new_federate_handle )
#else
   wstring const &label,
   wstring const &federate_name,
   FederateHandle new_federate_handle )
#endif // IEEE_1516_2025
{
   save_restore_service->restore_initiated( label, federate_name, new_federate_handle );
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_waiting_for_checkpoint_load()
{
   save_restore_service->restore_waiting_for_checkpoint_load();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_after_checkpoint_load()
{
   save_restore_service->restore_after_checkpoint_load();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_waiting_for_completion()
{
   save_restore_service->restore_waiting_for_completion();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_succeded()
{
   save_restore_service->restore_succeded();
   return;
}

/*!
 *  @job_class{freeze}
 */
void ExecutionControlBase::restore_failed()
{
   save_restore_service->restore_failed();
   return;
}

/*!
 *  @job_class{freeze}
 */
bool ExecutionControlBase::restore( wstring const &label )
{
   THLARestoreProcessEnum current_restore_state;
   std::string            current_restore_state_str;

   // If Federation SaveRestore is not supported then return without action.
   if ( !is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::restore():" << __LINE__
                << " ERROR: SaveRetore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return ( false );
   }

   // Get the current Restore state.
   current_restore_state = save_restore_service->restore_get_state();

   // Convert the Restore label for use in messages.
   current_restore_state_str = TrickHLA::to_string( current_restore_state );

   // Check the Federation Restore state to ensure that a Restore is applicable .
   if ( current_restore_state != THLARestoreProcessEnum::RESTORE_NONE ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::restore():%d : Restore already in progress: \'%s\'!\n",
                          __LINE__, current_restore_state_str.c_str() );
      }

      return ( false );
   }

   // Initiate the Restore process.
   save_restore_service->restore_set_label( label );
   save_restore_service->restore_set_state( THLARestoreProcessEnum::RESTORE_ACTIVATE );

   return ( true );
}

/*
 * @job_class{scheduled}
 */
std::string const ExecutionControlBase::map_label_to_checkpoint_file_name(
   wstring const &save_label )
{
   std::string save_label_str;
   std::string checkpoint_file_name;

   // Convert the Save label wstring to a string.
   StringUtilities::to_string( save_label_str, save_label );

   // Build up the checkpoint file name.
   // First get the federation name.
   checkpoint_file_name = federate->get_federation_name();
   checkpoint_file_name += "_";
   // Next get the federate name.
   checkpoint_file_name += federate->get_federate_name();
   checkpoint_file_name += "_";
   // Add the specified HLA Save label.
   checkpoint_file_name += save_label_str;
   // Add the checkpoint suffix.
   checkpoint_file_name += ".chkpt";

   return ( checkpoint_file_name );
}

//-------------------------------------------------------------------------
// CheckpointConversionBase Interface.
//-------------------------------------------------------------------------

void ExecutionControlBase::convert_data_before_checkpoint()
{
   if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "ExecutionControlBase::convert_data_before_checkpoint():"
          << __LINE__ << ": Converting data for checkpointing." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Make sure to free resources before doing the data conversions to avoid
   // a memory leak.
   federate->free_converted_data_for_checkpoint();

   // Convert the federate services data before a checkpoint.
   time_management_service->convert_data_before_checkpoint();
   object_service->convert_data_before_checkpoint();
   interaction_service->convert_data_before_checkpoint();
   save_restore_service->convert_data_before_checkpoint();

   // TODO: Do the Timelines need to be converted.

   // Convert the synchronization point manager base class elements.
   SyncPointManagerBase::convert_data_before_checkpoint();

   // Convert the ExecutionConfiguration data.
   if ( execution_configuration != NULL ) {
      execution_configuration->convert_data_before_checkpoint();
   }

   return;
}

void ExecutionControlBase::restore_data_after_checkpoint()
{
   if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "ExecutionControlBase::restore_data_after_checkpoint():"
          << __LINE__ << ": Restoring data after checkpoint loading." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // TODO: Do the Timelines need to be restored.

   // Restore the federate services data after checkpoint load.
   time_management_service->restore_data_after_checkpoint();
   object_service->restore_data_after_checkpoint();
   interaction_service->restore_data_after_checkpoint();
   save_restore_service->restore_data_after_checkpoint();

   // Restoring the synchronization point lists.
   SyncPointManagerBase::restore_data_after_checkpoint();

   // Restoring the ExecutionConfiguration data.
   if ( execution_configuration != NULL ) {
      execution_configuration->restore_data_after_checkpoint();
   }

   return;
}

void ExecutionControlBase::free_converted_data_for_checkpoint()
{
   if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "ExecutionControlBase::free_converted_data_for_checkpoint():"
          << __LINE__ << ": Freeing data allocated for checkpointing." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // TODO: Do the Timelines converted data need to be free.

   // Free the converted federate services data for checkpoint.
   time_management_service->free_converted_data_for_checkpoint();
   object_service->free_converted_data_for_checkpoint();
   interaction_service->free_converted_data_for_checkpoint();
   save_restore_service->free_converted_data_for_checkpoint();

   // Freeing the synchronization point lists checkpoint data.
   SyncPointManagerBase::free_converted_data_for_checkpoint();

   // Freeing the ExecutionConfiguration checkpoint data.
   if ( execution_configuration != NULL ) {
      execution_configuration->free_converted_data_for_checkpoint();
   }

   return;
}

/*
 * @job_class{checkpoint}
 */
void ExecutionControlBase::checkpoint_before()
{
   // Don't try to convert data while in initialization.
   if ( exec_get_mode() == Initialization ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "ExecutionControlBase::checkpoint_before():"
          << __LINE__ << ": Preparing for a checkpoint." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Convert data using the top level Federate interface.
   federate->convert_data_before_checkpoint();

   return;
}

/*!
 *  @job_class{post_checkpoint}
 */
void ExecutionControlBase::checkpoint_after()
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "ExecutionControlBase::checkpoint_after():"
          << __LINE__ << ": Cleaning up after a checkpoint." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Normally there's nothing to do after dropping a checkpoint.

   return;
}

/*!
 *  @job_class{preload_checkpoint}
 */
void ExecutionControlBase::checkpoint_preload()
{
   // TrickHLA only supports a checkpoint load as part of an HLA Restore process.
   if ( save_restore_service->restore_state != THLARestoreProcessEnum::RESTORE_INITIATED ) {
      string restore_label_str;
      StringUtilities::to_string( restore_label_str, save_restore_service->restore_label );
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::checkpoint_preload():" << __LINE__
             << ": ERROR: Unexpected Restore state for label: " << restore_label_str << endl
             << "   Expected state: RESTORE_INITIATED" << endl
             << "   Current state : " << TrickHLA::to_string( save_restore_service->restore_state ) << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "ExecutionControlBase::checkpoint_preload():"
          << __LINE__ << ": Preparing to load a checkpoint." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Free any resources allocated for a previous checkpoint.  We do this
   // to help the Trick MemoryManager when resetting memory.
   federate->free_converted_data_for_checkpoint();

   // Move the Restore state to indicate that the checkpoint has started.
   save_restore_service->restore_state = THLARestoreProcessEnum::RESTORE_CHECKPOINT;

   return;
}

/*!
 *  @job_class{restart}
 */
void ExecutionControlBase::checkpoint_restart()
{
   // FIXME: Is this always the case?
   // TrickHLA only supports a checkpoint load as part of an HLA Restore process.
   if ( save_restore_service->restore_state != THLARestoreProcessEnum::RESTORE_CHECKPOINT ) {
      string restore_label_str;
      StringUtilities::to_string( restore_label_str, save_restore_service->restore_label );
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::checkpoint_restart():" << __LINE__
             << ": ERROR: Unexpected Restore state for label: " << restore_label_str << endl
             << "   Expected state: RESTORE_INITIATED" << endl
             << "   Current state : " << TrickHLA::to_string( save_restore_service->restore_state ) << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "ExecutionControlBase::checkpoint_restart():"
          << __LINE__ << ": Restarting after loading a checkpoint." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Make sure to reset the Save state.  Otherwise, the Freeze loop
   // SaveRestoreServices::save_process routine will pickup wherever the Save
   // process was when the checkpoint file was generateed.
   if ( save_restore_service->save_state != THLASaveProcessEnum::SAVE_NONE ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, save_restore_service->restore_label );
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::checkpoint_restart():" << __LINE__
                << ": WARNING: Resetting Save state to THLASaveProcessEnum::SAVE_NONE!" << endl
                << " Label: '" << label_str << "'" << endl
                << " State: '" << TrickHLA::to_string( save_restore_service->restore_state ) << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      save_restore_service->save_state = THLASaveProcessEnum::SAVE_NONE;
   }

   // Call the SyncpointManagerBase function.
   SyncPointManagerBase::checkpoint_restart();

   return;
}

/*!
 * @job_class{shutdown}
 */
void ExecutionControlBase::remove_execution_configuration()
{
   // Remove the ExecutionConfiguration object instance if present.
   if ( execution_configuration != NULL ) {
      execution_configuration->remove();
   }
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionControlBase::set_least_common_time_step(
   double const lcts )
{
   this->least_common_time_step_seconds = lcts;
   this->least_common_time_step         = Int64BaseTime::to_base_time( lcts );
}

void ExecutionControlBase::set_least_common_time_step(
   int64_t const lcts )
{
   this->least_common_time_step         = lcts;
   this->least_common_time_step_seconds = Int64BaseTime::to_seconds( lcts );
}

void ExecutionControlBase::refresh_least_common_time_step()
{
   // Refresh the LCTS by setting the value again, which will calculate a new
   // LCTS using the HLA base time units.
   set_least_common_time_step( this->least_common_time_step_seconds );
}

void ExecutionControlBase::set_time_padding( double const t )
{
   this->time_padding = t;
}

#if defined( IEEE_1516_2010 )
// Pop off the stack the GCC arguments specific to this file.
#   pragma GCC diagnostic pop
#endif
