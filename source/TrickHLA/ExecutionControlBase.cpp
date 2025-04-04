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
@trick_link_dependency{CTETimelineBase.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionConfigurationBase.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{ScenarioTimeline.cpp}
@trick_link_dependency{SimTimeline.cpp}
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{SyncPointManagerBase.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, TrickHLA support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/Clock.hh"
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"
#include "trick/trick_byteswap.h"

// TrickHLA include files.
#include "TrickHLA/CTETimelineBase.hh"
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/ScenarioTimeline.hh"
#include "TrickHLA/SimTimeline.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPointManagerBase.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

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
     multiphase_init_sync_points( NULL ),
     time_padding( 0.0 ),
     enable_least_common_time_step( false ),
     least_common_time_step_seconds( -1.0 ),
     least_common_time_step( -1 ),
     execution_configuration( NULL ),
     mode_transition_requested( false ),
     requested_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     current_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     next_mode_scenario_time( -std::numeric_limits< double >::max() ),
     next_mode_cte_time( -std::numeric_limits< double >::max() ),
     simulation_freeze_time( 0.0 ),
     scenario_freeze_time( 0.0 ),
     announce_freeze( false ),
     freeze_the_federation( false ),
     late_joiner( false ),
     late_joiner_determined( false ),
     manager( NULL )
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
     multiphase_init_sync_points( NULL ),
     time_padding( 0.0 ),
     enable_least_common_time_step( false ),
     least_common_time_step_seconds( -1.0 ),
     least_common_time_step( -1 ),
     execution_configuration( &exec_config ),
     mode_transition_requested( false ),
     requested_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     current_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     next_mode_scenario_time( -std::numeric_limits< double >::max() ),
     next_mode_cte_time( -std::numeric_limits< double >::max() ),
     simulation_freeze_time( 0.0 ),
     scenario_freeze_time( 0.0 ),
     announce_freeze( false ),
     freeze_the_federation( false ),
     late_joiner( false ),
     late_joiner_determined( false ),
     manager( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ExecutionControlBase::~ExecutionControlBase()
{
   // TODO: Should not call a virtual function from within virtual destructor.
   // clear_mode_values();

   // Free the memory used for the multiphase initialization synchronization points.
   if ( multiphase_init_sync_points != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( multiphase_init_sync_points ) ) ) {
         message_publish( MSG_WARNING, "ExecutionControlBase::~ExecutionControlBase():%d WARNING failed to delete Trick Memory for 'multiphase_init_sync_points'\n",
                          __LINE__ );
      }
      multiphase_init_sync_points = NULL;
   }
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
   TrickHLA::Federate                   &fed,
   TrickHLA::Manager                    &mgr,
   TrickHLA::ExecutionConfigurationBase &exec_config )
{
   // Set the TrickHLA::Federate instance reference that exists in the
   // SyncPointManagerBase subclass we extended.
   SyncPointManagerBase::setup( &fed );

   // Set the TrickHLA::Manager instance reference.
   this->manager = &mgr;

   // Set the TrickHLA::ExecutionConfigurationBase instance reference.
   this->execution_configuration = &exec_config;

   // Setup the TrickHLA::ExecutionConfigurationBase instance.
   execution_configuration->setup( *this );

   // Configure the default Execution Configuration attributes.
   execution_configuration->configure_attributes();
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - This assumes that the TrickHLA::ExecutionConfigurationBase instance is
 * set elsewhere.
 *
 * @job_class{default_data}
 */
void ExecutionControlBase::setup(
   TrickHLA::Federate &fed,
   TrickHLA::Manager  &mgr )
{
   // Set the TrickHLA::Federate instance reference that exists in the
   // SyncPointManagerBase subclass we extended.
   SyncPointManagerBase::setup( &fed );

   // Set the TrickHLA::Manager instance reference.
   this->manager = &mgr;

   // Check to see if the ExecutionConfigurationBase instance is set.
   if ( this->execution_configuration != NULL ) {

      // Setup the TrickHLA::ExecutionConfigurationBase instance.
      execution_configuration->setup( *this );

      // Configure the default Execution Configuration attributes.
      execution_configuration->configure_attributes();
   }
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
                << " ERROR: The CTE timeline is specified but it is not"
                << " configured as the Trick real time clock! Make sure"
                << " the CTETimelineBase class constructor is calling"
                << " real_time_change_clock( this );\n";
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
   if ( ( federate != NULL ) && !federate->verify_time_constraints() ) {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::initialize():" << __LINE__
             << " ERROR: Time constraints verification failed!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( !does_scenario_timeline_exist() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "ExecutionControlBase::initialize():%d WARNING: \
ExecutionControl 'scenario_timeline' not specified in the input.py file. Using the \
Trick simulation time as the default scenario-timeline.\n",
                          __LINE__ );
      }

      // Use the simulation timeline as the default scenario timeline.
      scenario_timeline = &def_scenario_timeline;
      if ( scenario_timeline == NULL ) { // cppcheck-suppress [knownConditionTrueFalse]
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::initialize():" << __LINE__
                << " FAILED to allocate enough memory for ScenarioTimeline class!\n";
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
      message_publish( MSG_NORMAL, "ExecutionControl::join_federation_process():%d Checking for shutdown \n",
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
                << "'! This conflicts with this being the designated Master federate!\n";
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
bool const ExecutionControlBase::contains_multiphase_init_sync_point(
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
   if ( this->multiphase_init_sync_points != NULL ) {
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
                << user_sync_pt_labels[i] << "' already added!\n";
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
      message_publish( MSG_NORMAL, "ExecutionControlBase::clear_multiphase_init_sync_points():%d \n",
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
             << " sure the 'publish' value is correctly specified.\n";
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

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the data to arrive.
      while ( !execution_configuration->is_changed() ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !execution_configuration->is_changed() ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "ExecutionControlBase::receive_execution_configuration():" << __LINE__
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
             << " files to make sure the 'subscribe' value is correctly specified.\n";
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
      if ( obj_instance_name == get_execution_configuration()->get_name_string() ) {
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
                || ( execution_configuration->get_name() == NULL )
                || ( *( execution_configuration->get_name() ) == '\0' ) ) ) {
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
         message_publish( MSG_NORMAL, "ExecutionControlBase::mark_object_as_deleted_from_federation():%d Object '%s' Instance-ID:%s Valid-ID:%s \n",
                          __LINE__, execution_configuration->get_name(), id_str.c_str(),
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

double ExecutionControlBase::get_sim_time()
{
   if ( does_sim_timeline_exist() ) {
      return sim_timeline->get_time();
   }

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::get_sim_time():" << __LINE__
             << " WARNING: Unexpected NULL 'THLA.federate.get_sim_time'!"
             << " Please make sure you specify a sim-timeline in your input"
             << " file. Returning Trick simulation time instead!\n";
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
   }
   return exec_get_sim_time();
}

double ExecutionControlBase::get_scenario_time()
{
   if ( does_scenario_timeline_exist() ) {
      return scenario_timeline->get_time();
   }

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::get_scenario_time():" << __LINE__
             << " WARNING: Unexpected NULL 'THLA.federate.scenario_timeline'!"
             << " Please make sure you specify a scenario-timeline in your input"
             << " file. Returning Trick simulation time instead!\n";
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
   }

   return get_sim_time();
}

double ExecutionControlBase::get_cte_time()
{
   return does_cte_timeline_exist() ? cte_timeline->get_time()
                                    : -std::numeric_limits< double >::max();
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

void ExecutionControlBase::check_pause( double const check_pause_delta )
{
   return;
}

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

void ExecutionControlBase::encode_checkpoint()
{
   SyncPointManagerBase::encode_checkpoint();

   // Setup checkpoint for ExecutionConfiguration if we have one.
   if ( execution_configuration != NULL ) {
      // Any object with a valid instance handle must be marked as required
      // to ensure the restore process will wait for this object instance
      // to exist.
      if ( execution_configuration->is_instance_handle_valid() ) {
         execution_configuration->mark_required();
      }
      execution_configuration->encode_checkpoint();
   }
}

void ExecutionControlBase::decode_checkpoint()
{
   SyncPointManagerBase::decode_checkpoint();

   // Decode checkpoint for ExecutionConfiguration if we have one.
   if ( execution_configuration != NULL ) {
      execution_configuration->decode_checkpoint();
   }
}

void ExecutionControlBase::free_checkpoint()
{
   SyncPointManagerBase::free_checkpoint();

   // Clear/release the memory used for the checkpoint data structures.
   if ( execution_configuration != NULL ) {
      execution_configuration->free_checkpoint();
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
   // WARNING: Only the Master federate should ever set this.
   if ( is_master() ) {
      this->least_common_time_step_seconds = lcts;
      this->least_common_time_step         = Int64BaseTime::to_base_time( lcts );
   }
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
