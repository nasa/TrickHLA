/*!
@file TrickHLA/ExecutionControlBase.cpp
@ingroup TrickHLA
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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionConfigurationBase.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Manager.cpp}
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
#include "trick/CheckPointRestart_c_intf.hh"
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
#include "TrickHLA/Manager.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/SaveRestoreServices.hh"
#include "TrickHLA/SyncPointManagerBase.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/CTETimelineBase.hh"
#include "TrickHLA/time/Int64BaseTime.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/time/ScenarioTimeline.hh"
#include "TrickHLA/time/SimTimeline.hh"
#include "TrickHLA/utils/SleepTimeout.hh"
#include "TrickHLA/utils/StringUtilities.hh"
#include "TrickHLA/utils/Utilities.hh"

#if defined( IEEE_1516_2010 )
// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Exception.h"
#include "RTI/Handle.h"
#include "RTI/RTIambassador.h"
#include "RTI/RTIambassadorFactory.h"
#include "RTI/Typedefs.h"
#include "RTI/time/HLAinteger64Time.h"

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
     execution_has_begun( false ),
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
     manager( NULL ),
     time_management_srvc( NULL ),
     save_restore_srvc( NULL )
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
     execution_has_begun( false ),
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
     manager( NULL ),
     time_management_srvc( NULL ),
     save_restore_srvc( NULL )
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
   this->manager = fed.get_manager();

   // Set the TrickHLA::SaveRestoreServices instance reference.
   this->save_restore_srvc = fed.get_save_restore_service();

   // Set the TrickHLA::TimeManagementServices instance reference.
   this->time_management_srvc = fed.get_time_management_services();

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
         DebugHandler::terminate_with_message( errmsg.str() );
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
   if ( ( federate != NULL ) && !federate->time_management_srvc.verify_time_constraints() ) {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::initialize():" << __LINE__
             << " ERROR: Time constraints verification failed!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
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
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }
   }

   // Initialize then Configure the ExecutionConfiguration object if present.
   if ( execution_configuration != NULL ) {
      execution_configuration->Object::initialize( this->manager );
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
   std::wstring const &obj_instance_name )
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
   std::wstring const &obj_instance_name )
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
         DebugHandler::terminate_with_message( errmsg.str() );
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

      // Place the ExecutionConfiguration object into the Manager's object map.
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
   manager->add_object_to_map( object );
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
         DebugHandler::terminate_with_message( errmsg.str() );
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
   if ( manager->is_late_joining_federate() ) {
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
      DebugHandler::terminate_with_message( errmsg.str() );
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

      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the data to arrive.
      while ( !execution_configuration->is_changed() ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !execution_configuration->is_changed() ) {

            // To be more efficient, we get the time once and share it.
            int64_t wallclock_time = sleep_timer.time();

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
                  DebugHandler::terminate_with_message( errmsg.str() );
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
      DebugHandler::terminate_with_message( errmsg.str() );
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
   std::wstring const      &theObjectInstanceName )
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

void ExecutionControlBase::convert_data_before_checkpoint()
{
   SyncPointManagerBase::convert_data_before_checkpoint();

   // Setup checkpoint for ExecutionConfiguration if we have one.
   if ( execution_configuration != NULL ) {
      // Any object with a valid instance handle must be marked as required
      // to ensure the restore process will wait for this object instance
      // to exist.
      if ( execution_configuration->is_instance_handle_valid() ) {
         execution_configuration->set_required( true );
      }
      execution_configuration->convert_data_before_checkpoint();
   }
}

void ExecutionControlBase::restore_data_after_checkpoint()
{
   SyncPointManagerBase::restore_data_after_checkpoint();

   // Decode checkpoint for ExecutionConfiguration if we have one.
   if ( execution_configuration != NULL ) {
      execution_configuration->restore_data_after_checkpoint();
   }
}

void ExecutionControlBase::free_converted_data_for_checkpoint()
{
   SyncPointManagerBase::free_converted_data_for_checkpoint();

   // Clear/release the memory used for the checkpoint data structures.
   if ( execution_configuration != NULL ) {
      execution_configuration->free_converted_data_for_checkpoint();
   }
}

/*! @brief Perform setup for federate save. */
void ExecutionControlBase::setup_checkpoint()
{
   // Do not do federate save during Init or Exit (this allows "regular" init
   // and shutdown checkpoints)
   if ( ( exec_get_mode() == Initialization ) || ( exec_get_mode() == ExitMode ) ) {
      return;
   }

   string str_save_label( save_restore_srvc->get_save_label() );

   // Determine if I am the federate that clicked Dump Chkpnt on sim control panel
   // or I am the federate that called start_federation_save

   save_restore_srvc->set_announce_save( !save_restore_srvc->is_start_to_save() );

   // Check to see if the save has been initiated in the ExecutionControl process?
   // If not then just return.
   if ( !is_save_initiated() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "ExecutionControlBase::setup_checkpoint():%d Federate Save Pre-checkpoint\n",
                       __LINE__ );
   }

   // If I announced the save, must initiate federation save
   if ( save_restore_srvc->is_announce_save() ) {
      if ( save_restore_srvc->get_save_name().length() ) {
         // When user calls start_federation_save, save_name is already set
      } else {
         // When user clicks Dump Chkpnt, we need to set the save_name here
         string trick_filename;
         string slash( "/" );
         size_t found;
         string save_name_str;

         // get checkpoint file name specified in control panel
         trick_filename = checkpoint_get_output_file();

         // Trick filename contains dir/filename,
         // need to prepend federation name to filename entered in sim control panel popup
         found = trick_filename.rfind( slash );
         if ( found != string::npos ) {
            save_name_str              = trick_filename.substr( found + 1 );
            string federation_name_str = federate->get_federation_name();
            if ( save_name_str.compare( 0, federation_name_str.length(), federation_name_str ) != 0 ) {
               // dir/federation_filename
               trick_filename.replace( found, slash.length(), slash + federation_name_str + "_" );
            } else {
               // If it already has federation name prepended, output_file name
               // is good to go but remove it from save_name_str so our
               // str_save_label setting below is correct
               save_name_str = trick_filename.substr( found + 1 + federation_name_str.length() + 1 ); // filename
            }
         } else {
            save_name_str = trick_filename;
         }

         // TODO: Clean this up later.
         // Set the checkpoint restart filename.
         the_cpr->output_file = trick_filename;

         // federation_filename
         str_save_label = federate->get_federation_name() + "_" + save_name_str;

         // Set the federate save_name to filename (without the federation name)
         // - this gets announced to other feds
         wstring save_name_ws;
         StringUtilities::to_wstring( save_name_ws, save_name_str );

         save_restore_srvc->set_save_name( save_name_ws );
      } // end set save_name

      // Don't request a save if another federate has already requested one
      if ( save_restore_srvc->is_initiate_save_flag() ) {
         // initiate_save_flag becomes false if another save is occurring
         save_restore_srvc->request_federation_save_status();
         save_restore_srvc->wait_for_save_status_to_complete();

         save_restore_srvc->request_federation_save();

         SleepTimeout print_timer( federate->wait_status_time );
         SleepTimeout sleep_timer;

         // need to wait for federation to initiate save
         while ( !save_restore_srvc->is_start_to_save() ) {

            // Check for shutdown.
            check_for_shutdown_with_termination();

            sleep_timer.sleep();

            if ( !save_restore_srvc->is_start_to_save() ) {

               // To be more efficient, we get the time once and share it.
               int64_t wallclock_time = sleep_timer.time();

               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "ExecutionControlBase::setup_checkpoint():" << __LINE__
                            << " ERROR: Unexpectedly the Federate is no longer an execution"
                            << " member. This means we are either not connected to the"
                            << " RTI or we are no longer joined to the federation"
                            << " execution because someone forced our resignation at"
                            << " the Central RTI Component (CRC) level!" << endl;
                     DebugHandler::terminate_with_message( errmsg.str() );
                  }
               }

               if ( print_timer.timeout( wallclock_time ) ) {
                  print_timer.reset();
                  message_publish( MSG_NORMAL, "ExecutionControlBase::setup_checkpoint():%d Federate Save Pre-checkpoint, wiating...\n",
                                   __LINE__ );
               }
            }
         }
         save_restore_srvc->set_initiate_save_flag( false );
      } else {
         message_publish( MSG_NORMAL, "ExecutionControlBase::setup_checkpoint():%d Federation Save is already in progress!\n",
                          __LINE__ );
         return;
      }
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;
   try {
      // FIXME: Should this be in the Federate and not TimeManagementServices?
      // Or, should this be in the Save & Restore services.
      federate->get_RTI_ambassador()->federateSaveBegun();
   } catch ( SaveNotInitiated const &e ) {
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: SaveNotInitiated\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // This is a shortcut so that we can enforce that only these federates exist
   // when we restore
   save_restore_srvc->write_running_feds_file( str_save_label );

   // Tell the manager to setup the checkpoint data structures.
   manager->convert_data_before_checkpoint();

   // Save any synchronization points.
   save_restore_srvc->convert_sync_pts();
}

/*! @brief Federates that did not announce the save, perform a checkpoint. */
void ExecutionControlBase::perform_checkpoint()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_save_and_restore_supported() ) {
      return;
   }

   // Dispatch to the ExecutionControl method.
   bool force_checkpoint = perform_save();

   if ( save_restore_srvc->is_start_to_save() || force_checkpoint ) {
      // If I announced the save, sim control panel was clicked and invokes the checkpoint
      if ( !save_restore_srvc->is_announce_save() ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "ExecutionControlBase::perform_checkpoint():%d Federate Save Started\n",
                             __LINE__ );
         }
         // Create the filename from the Federation name and the "save-name".
         // Replace all directory characters with an underscore.
         string save_name_str;
         StringUtilities::to_string( save_name_str, save_restore_srvc->get_save_name() );
         string str_save_label = federate->get_federation_name() + "_" + save_name_str;
         for ( size_t i = 0; i < str_save_label.length(); ++i ) {
            if ( str_save_label[i] == '/' ) {
               str_save_label[i] = '_';
            }
         }

         // calls setup_checkpoint first
         checkpoint( str_save_label.c_str() );
      }
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::perform_checkpoint():%d Checkpoint Dump Completed.\n",
                          __LINE__ );
      }

      post_checkpoint();
   }
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSim initialization schemes.
 *  @job_class{post_checkpoint}
 */
void ExecutionControlBase::post_checkpoint()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_save_and_restore_supported() ) {
      return;
   }

   if ( save_restore_srvc->is_start_to_save() ) {

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;
      try {
         federate->get_RTI_ambassador()->federateSaveComplete();
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "ExecutionControlBase::post_checkpoint():%d Federate Save Completed.\n",
                             __LINE__ );
         }
         save_restore_srvc->set_start_to_save( false );
      } catch ( FederateHasNotBegunSave const &e ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::post_checkpoint():%d EXCEPTION: FederateHasNotBegunSave\n",
                          __LINE__ );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::post_checkpoint():%d EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::post_checkpoint():%d EXCEPTION: RestoreInProgress\n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::post_checkpoint():%d EXCEPTION: NotConnected\n",
                          __LINE__ );
         federate->set_connection_lost();
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "ExecutionControlBase::post_checkpoint():%d EXCEPTION: RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::post_checkpoint():%d Federate Save Already Completed.\n",
                          __LINE__ );
      }
   }
}

/*! @brief Perform setup for federate restore. */
void ExecutionControlBase::setup_restore()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_save_and_restore_supported() ) {
      return;
   }

   // if restoring at startup, do nothing here (that is handled in restore_checkpoint)
   if ( !federate->is_federate_executing() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "ExecutionControlBase::setup_restore():%d Federate Restore Pre-load.\n",
                       __LINE__ );
   }
   // Determine if I am the federate that clicked Load Chkpnt on sim control panel
   save_restore_srvc->set_announce_restore( !save_restore_srvc->is_start_to_restore() );
   set_freeze_announced( save_restore_srvc->is_announce_restore() );

   // if I announced the restore, must initiate federation restore
   if ( save_restore_srvc->is_announce_restore() ) {
      string trick_filename;
      string slash_fedname( "/" + federate->get_federation_name() + "_" );
      size_t found;

      // Otherwise set restore_name_str using trick's file name
      trick_filename = checkpoint_get_load_file();

      // Trick memory manager load_checkpoint_file_name already contains correct dir/federation_filename
      // (chosen in sim control panel popup) we need just the filename minus the federation name to initiate restore
      found = trick_filename.rfind( slash_fedname );
      string restore_name_str;
      if ( found != string::npos ) {
         restore_name_str = trick_filename.substr( found + slash_fedname.length() ); // filename
      } else {
         restore_name_str = trick_filename;
      }
      // federation_filename
      string str_restore_label = federate->get_federation_name() + "_" + restore_name_str;

      // make sure we have a save directory specified
      save_restore_srvc->check_HLA_save_directory();

      // make sure only the required federates are in the federation before we do the restore
      save_restore_srvc->read_running_feds_file( str_restore_label );

      string return_string;
      return_string = federate->wait_for_required_federates_to_join(); // sets running_feds_count
      if ( !return_string.empty() ) {
         return_string += '\n';
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::setup_restore():" << __LINE__ << endl
                << "ERROR: " << return_string;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      // set the federate restore_name to filename (without the federation name)- this gets announced to other feds
      save_restore_srvc->initiate_restore_announce( restore_name_str );

      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer;

      // need to wait for federation to initiate restore
      while ( !save_restore_srvc->is_start_to_restore() ) {

         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !save_restore_srvc->is_start_to_restore() ) {

            // To be more efficient, we get the time once and share it.
            int64_t wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "ExecutionControlBase::setup_restore():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!" << endl;
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "ExecutionControlBase::setup_restore():%d Federate Restore Pre-load, waiting...\n",
                                __LINE__ );
            }
         }
      }
   }

   save_restore_srvc->set_restore_process( RESTORE_IN_PROGRESS );
}

/*! @brief Federates that did not announce the restore, perform a restore. */
void ExecutionControlBase::perform_restore()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_save_and_restore_supported() ) {
      return;
   }

   if ( save_restore_srvc->is_start_to_restore() ) {
      // if I announced the restore, sim control panel was clicked and invokes the load
      if ( !save_restore_srvc->is_announce_restore() ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "ExecutionControlBase::perform_restore():%d Federate Restore Started.\n",
                             __LINE__ );
         }

         // Create the filename from the Federation name and the "restore-name".
         // Replace all directory characters with an underscore.
         string restore_name_str;
         StringUtilities::to_string( restore_name_str, save_restore_srvc->get_restore_name() );
         string str_restore_label = federate->get_federation_name() + "_" + restore_name_str;
         for ( size_t i = 0; i < str_restore_label.length(); ++i ) {
            if ( str_restore_label[i] == '/' ) {
               str_restore_label[i] = '_';
            }
         }
         message_publish( MSG_NORMAL, "ExecutionControlBase::perform_restore():%d LOADING %s\n",
                          __LINE__, str_restore_label.c_str() );

         // make sure we have a save directory specified
         save_restore_srvc->check_HLA_save_directory();

         // This will run pre-load-checkpoint jobs, clear memory, read checkpoint file, and run restart jobs
         load_checkpoint( ( save_restore_srvc->get_HLA_save_directory() + "/" + str_restore_label ).c_str() );

         load_checkpoint_job();

         // exec_freeze();
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::perform_restore():%d Checkpoint Load Completed.\n",
                          __LINE__ );
      }

      post_restore();
   }
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSim initialization schemes.
 */
void ExecutionControlBase::post_restore()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_save_and_restore_supported() ) {
      return;
   }

   if ( save_restore_srvc->is_start_to_restore() ) {
      save_restore_srvc->set_restore_process( RESTORE_COMPLETE );

      // Make a copy of restore_process because it is used in the
      // inform_RTI_of_restore_completion() function.
      // (backward compatibility with previous restore process)
      save_restore_srvc->preserve_restore_process();

      save_restore_srvc->copy_running_feds_into_known_feds();

      // wait for RTI to inform us that the federation restore has
      // begun before informing the RTI that we are done.
      save_restore_srvc->wait_for_federation_restore_begun();

      // signal RTI that this federate has already been loaded
      save_restore_srvc->inform_RTI_of_restore_completion();

      // wait until we get a callback to inform us that the federation restore is complete
      string tStr = save_restore_srvc->wait_for_federation_restore_to_complete();
      if ( tStr.length() ) {
         save_restore_srvc->wait_for_federation_restore_failed_callback_to_complete();
         ostringstream errmsg;
         errmsg << "TrickExecutionControlBase::post_restore():" << __LINE__
                << " ERROR: " << tStr << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::post_restore():%d Federation Restore Completed.\n",
                          __LINE__ );
         message_publish( MSG_NORMAL, "ExecutionControlBase::post_restore():%d Rebuilding HLA Handles.\n",
                          __LINE__ );
      }

      // get us restarted again...
      // reset RTI data to the state it was in when checkpointed
      manager->reset_mgr_initialized();
      manager->setup_object_ref_attributes();
      manager->setup_interaction_ref_attributes();
      manager->setup_object_RTI_handles();
      manager->setup_interaction_RTI_handles();
      manager->set_all_object_instance_handles_by_name();

      if ( save_restore_srvc->is_announce_restore() ) {
         federate->set_all_federate_MOM_instance_handles_by_name();
         federate->restore_federate_handles_from_MOM();
      }

      // Restore interactions and sync points
      manager->restore_interactions_after_checkpoint();
      reinstate_logged_sync_pts();

      // Restore ownership transfer data for all objects
      Object *objects   = manager->get_objects();
      int     obj_count = manager->get_object_count();
      for ( int i = 0; i < obj_count; ++i ) {
         objects[i].restore_data_after_checkpoint();
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;
      try {
         HLAinteger64Time time;
         federate->get_RTI_ambassador()->queryLogicalTime( time );
         federate->time_management_srvc.set_granted_time( time );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::post_restore():%d queryLogicalTime EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::post_restore():%d queryLogicalTime EXCEPTION: SaveInProgress\n",
                          __LINE__ );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::post_restore():%d queryLogicalTime EXCEPTION: RestoreInProgress\n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::post_restore():%d queryLogicalTime EXCEPTION: NotConnected\n",
                          __LINE__ );
         federate->set_connection_lost();
      } catch ( RTIinternalError const &e ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::post_restore():%d queryLogicalTime EXCEPTION: RTIinternalError\n",
                          __LINE__ );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      federate->time_management_srvc.set_requested_time_to_granted_time();

      save_restore_srvc->federation_restored();

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::post_restore():%d Federate Restart Completed.\n",
                          __LINE__ );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::post_restore():%d Federate Restore Already Completed.\n",
                          __LINE__ );
      }
   }
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
