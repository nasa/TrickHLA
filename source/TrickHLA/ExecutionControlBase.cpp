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
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, TrickHLA support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iomanip>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>

// Trick includes.
#include "trick/Executive.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"
#include "trick/trick_byteswap.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace std;
using namespace RTI1516_NAMESPACE;
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
   : scenario_timeline( &def_scenario_timeline ),
     sim_timeline( &def_sim_timeline ),
     cte_timeline( NULL ),
     use_preset_master( false ),
     master( false ),
     multiphase_init_sync_points( NULL ),
     time_padding( 5.0 ),
     least_common_time_step( -1 ),
     execution_configuration( NULL ),
     multiphase_init_sync_pnt_list(),
     init_complete_sp_exists( false ),
     mode_transition_requested( false ),
     requested_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     current_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     next_mode_scenario_time( -std::numeric_limits< double >::max() ),
     next_mode_cte_time( -std::numeric_limits< double >::max() ),
     simulation_freeze_time( 0.0 ),
     scenario_freeze_time( 0.0 ),
     late_joiner( false ),
     late_joiner_determined( false ),
     federate( NULL ),
     manager( NULL ),
     logged_sync_pts_count( 0 ),
     loggable_sync_pts( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
ExecutionControlBase::ExecutionControlBase(
   ExecutionConfigurationBase &exec_config )
   : scenario_timeline( &def_scenario_timeline ),
     sim_timeline( &def_sim_timeline ),
     cte_timeline( NULL ),
     use_preset_master( false ),
     master( false ),
     multiphase_init_sync_points( NULL ),
     time_padding( 5.0 ),
     least_common_time_step( -1 ),
     execution_configuration( &exec_config ),
     multiphase_init_sync_pnt_list(),
     init_complete_sp_exists( false ),
     mode_transition_requested( false ),
     requested_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     current_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED ),
     next_mode_scenario_time( -std::numeric_limits< double >::max() ),
     next_mode_cte_time( -std::numeric_limits< double >::max() ),
     simulation_freeze_time( 0.0 ),
     scenario_freeze_time( 0.0 ),
     late_joiner( false ),
     late_joiner_determined( false ),
     federate( NULL ),
     manager( NULL ),
     logged_sync_pts_count( 0 ),
     loggable_sync_pts( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ExecutionControlBase::~ExecutionControlBase()
{
   clear_mode_values();

   // Free the memory used for the multiphase initialization synchronization points.
   if ( multiphase_init_sync_points != static_cast< char * >( NULL ) ) {
      if ( TMM_is_alloced( multiphase_init_sync_points ) ) {
         TMM_delete_var_a( multiphase_init_sync_points );
      }
      multiphase_init_sync_points = static_cast< char * >( NULL );
   }

   // Free the memory used by the array of running Federates for the Federation.
   if ( loggable_sync_pts != static_cast< LoggableTimedSyncPnt * >( NULL ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "~TrickHLA::ExecutionControlBase() logged_sync_pts_count=%d %c",
                  logged_sync_pts_count, THLA_NEWLINE );
      }
      for ( size_t i = 0; i < logged_sync_pts_count; ++i ) {
         loggable_sync_pts[i].clear();
      }
      TMM_delete_var_a( loggable_sync_pts );
      loggable_sync_pts     = static_cast< LoggableTimedSyncPnt * >( NULL );
      logged_sync_pts_count = 0;
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
   TrickHLA::Federate &                  federate,
   TrickHLA::Manager &                   manager,
   TrickHLA::ExecutionConfigurationBase &exec_config )
{
   // Set the TrickHLA::Federate instance reference.
   this->federate = &federate;

   // Set the TrickHLA::Manager instance reference.
   this->manager = &manager;

   // Set the TrickHLA::ExecutionConfigurationBase instance reference.
   this->execution_configuration = &exec_config;

   // Setup the TrickHLA::ExecutionConfigurationBase instance.
   this->execution_configuration->setup( *this );

   // Configure the default Execution Configuration attributes.
   this->execution_configuration->configure_attributes();
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - This assumes that the TrickHLA::ExecutionConfigurationBase instance is
 * set elsewhere.
 *
 * @job_class{default_data}
 */
void ExecutionControlBase::setup(
   TrickHLA::Federate &federate,
   TrickHLA::Manager & manager )
{
   // Set the TrickHLA::Federate instance reference.
   this->federate = &federate;

   // Set the TrickHLA::Manager instance reference.
   this->manager = &manager;

   // Check to see if the ExecutionConfigurationBase instance is set.
   if ( execution_configuration != NULL ) {

      // Setup the TrickHLA::ExecutionConfigurationBase instance.
      this->execution_configuration->setup( *this );

      // Configure the default Execution Configuration attributes.
      this->execution_configuration->configure_attributes();
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::initialize()
{
   // Set Trick's realtime clock to the CTE clock if used.
   if ( this->does_cte_timeline_exist() ) {
      this->cte_timeline->clock_init();
   }

   // Reset the master flag if it is not preset by the user.
   if ( !this->is_master_preset() ) {
      this->set_master( false );
   }

   if ( !does_scenario_timeline_exist() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "TrickHLA::ExecutionControlBase::initialize():%d WARNING: \
ExecutionControl 'scenario_timeline' not specified in the input file. Using the \
Trick simulation time as the default scenario-timeline.%c",
                  __LINE__, THLA_NEWLINE );
      }

      // Use the simulation timeline as the default scenario timeline.
      scenario_timeline = &def_scenario_timeline;
      if ( scenario_timeline == static_cast< ScenarioTimeline * >( NULL ) ) {
         ostringstream errmsg;
         errmsg << "TrickHLA::ExecutionControlBase::initialize():" << __LINE__
                << " FAILED to allocate enough memory for ScenarioTimeline class!"
                << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // Initialize then Configure the ExecutionConfiguration object if present.
   if ( execution_configuration != NULL ) {
      execution_configuration->initialize( this->manager );
      execution_configuration->configure();
   }
}

/*!
@job_class{initialization}
*/
void ExecutionControlBase::join_federation_process()
{
   TrickHLA::Federate *federate = this->get_federate();

   // Create the RTI Ambassador and connect.
   federate->create_RTI_ambassador_and_connect();

   // Destroy the federation if it was orphaned from a previous simulation
   // run that did not shutdown cleanly.
   federate->destroy_orphaned_federation();

   // All federates try to create the federation then join it because we use
   // a preset master.
   federate->create_and_join_federation();

   // Don't forget to enable asynchronous delivery of messages.
   federate->enable_async_delivery();

   // Check for a latent shutdown sync-point.
   // If shutdown sync-point is detected, then we must have entered into
   // a running federation execution that is shutting down. This is an
   // unlikely but possible race condition.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      send_hs( stdout, "TrickHLA::ExecutionControl::join_federation_process():%d Checking for shutdown %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->check_for_shutdown_with_termination();
}

/*!
@job_class{initialization}
*/
bool ExecutionControlBase::object_instance_name_reservation_succeeded(
   std::wstring const &obj_instance_name )
{
   wstring ws_exec_config_name;

   // If ExcutionConfiguration is not set, then there is no match.
   if ( execution_configuration != NULL ) {
      return false;
   }

   // Check to see if the ExecutionConfiguration object instance matches this
   // object instance name.
   StringUtilities::to_wstring( ws_exec_config_name, execution_configuration->get_name() );
   if ( obj_instance_name == ws_exec_config_name ) {

      // We are the Master federate if we succeeded in reserving the
      // ExecutionConfiguration object name and the master was not preset.
      if ( !this->is_master_preset() ) {
         this->set_master( true );
      }

      // The name is successfully registered.
      execution_configuration->set_name_registered();

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "TrickHLA::ExecutionControlBase::object_instance_name_reservation_succeeded():%d Name:'%ls'%c",
                  __LINE__, obj_instance_name.c_str(), THLA_NEWLINE );
      }

      return true;
   }

   return false;
}

/*!
@job_class{initialization}
*/
bool ExecutionControlBase::object_instance_name_reservation_failed(
   std::wstring const &obj_instance_name )
{
   // If ExcutionConfiguration is not set, then there is no match.
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
      if ( !this->is_master_preset() ) {
         this->set_master( false );
      } else { // If this is the designated preset Master federate, then this is an ERROR.
         ostringstream errmsg;
         errmsg << "TrickHLA::ExecutionControlBase::object_instance_name_reservation_failed:" << __LINE__
                << " Failed to reserve the ExecutionConfiguration object instance name: '" << obj_instance_name.c_str()
                << "'! This conflicts with this being the designated Master federate!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // We failed to register the ExecutionConfiguration object instance name
      // which means that another federate has already registered it.
      execution_configuration->set_name_registered();

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "TrickHLA::ExecutionControlBase::object_instance_name_reservation_failed():%d Name:'%ls'%c",
                  __LINE__, obj_instance_name.c_str(), THLA_NEWLINE );
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
      send_hs( stdout, "TrickHLA::ExecutionControlBase::register_objects_with_RTI():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Register any ExecutionConfiguration objects.
   if ( execution_configuration != NULL ) {

      // Register the execution configuration object.
      execution_configuration->register_object_with_RTI();

      // Place the ExecutionConfiguration object into the Manager's object map.
      this->add_object_to_map( execution_configuration );
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::add_object_to_map(
   Object *object )
{
   // Add the registered ExecutionConfiguration object instance to the map.
   this->manager->add_object_to_map( object );
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
      StringUtilities::tokenize( this->multiphase_init_sync_points,
                                 user_sync_pt_labels, "," );
   }

   // Add the user specified multiphase initialization sync-points to the list.
   for ( unsigned int i = 0; i < user_sync_pt_labels.size(); ++i ) {
      wstring ws_label;
      StringUtilities::to_wstring( ws_label, user_sync_pt_labels.at( i ) );
      multiphase_init_sync_pnt_list.add_sync_point( ws_label );
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::clear_multiphase_init_sync_points()
{

   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( this->manager->is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "TrickHLA::ExecutionControlBase::clear_multiphase_init_sync_points():%d Late \
joining federate so this call will be ignored.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      send_hs( stdout, "TrickHLA::ExecutionControlBase::clear_multiphase_init_sync_points():%d %c",
               __LINE__, THLA_NEWLINE );
   }

   try {

      // Achieve all the multiphase initialization synchronization points except.
      this->achieve_all_multiphase_init_sync_points( *federate->get_RTI_ambassador() );

      // Now wait for all the multiphase initialization sync-points to be
      // synchronized in the federation.
      this->wait_for_all_multiphase_init_sync_points();

   } catch ( RTI1516_NAMESPACE::SynchronizationPointLabelNotAnnounced &e ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControlBase::clear_multiphase_init_sync_points():" << __LINE__
             << " Exception: SynchronizationPointLabelNotAnnounced" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember &e ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControlBase::clear_multiphase_init_sync_points():" << __LINE__
             << " Exception: FederateNotExecutionMember" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress &e ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControlBase::clear_multiphase_init_sync_points():" << __LINE__
             << " Exception: SaveInProgress" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress &e ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControlBase::clear_multiphase_init_sync_points():" << __LINE__
             << " Exception: RestoreInProgress" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::NotConnected &e ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControlBase::clear_multiphase_init_sync_points():" << __LINE__
             << " Exception: NotConnected" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError &e ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControlBase::clear_multiphase_init_sync_points():" << __LINE__
             << " Exception: RTIinternalError" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControlBase::clear_multiphase_init_sync_points():" << __LINE__
             << " Exception: RTI1516_EXCEPTION " << rti_err_msg << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      this->print_sync_points();
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::achieve_all_multiphase_init_sync_points(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador )
{
   // Iterate through this ExecutionControl's user defined multiphase
   // initialization synchronization point list and achieve them.
   multiphase_init_sync_pnt_list.achieve_all_sync_points( rti_ambassador );
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::wait_for_all_multiphase_init_sync_points()
{
   // Wait for all the user defined multiphase initialization sychronization
   // points to be achieved.
   multiphase_init_sync_pnt_list.wait_for_list_synchronization( federate );
}

/*!
 * @job_class{initialization}
 */
void ExecutionControlBase::send_execution_configuration()
{
   if ( execution_configuration == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "TrickHLA::ExecutionControlBase::send_execution_configuration():%d This call \
will be ignored because the Simulation Initialization Scheme does not support it.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   // Only the master federate can send the ExecutionConfiguration.
   if ( !this->is_master() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      send_hs( stdout, "TrickHLA::ExecutionControlBase::send_ssend_execution_configurationim_config():%d%c", __LINE__, THLA_NEWLINE );
   }

   // Make sure we have at least one piece of ExecutionConfiguration data we can send.
   if ( execution_configuration->any_locally_owned_published_init_attribute() ) {

      // Send the ExecutionConfiguration data to the other federates.
      execution_configuration->send_init_data();

   } else {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControlBase::send_execution_configuration():" << __LINE__
             << " ERROR: ExecutionConfiguration"
             << " is not configured to send at least one object attribute. Make"
             << " sure at least one ExecutionConfiguration attribute has 'publish = true'"
             << " set. Please check your input or modified-data files to make"
             << " sure the 'publish' value is correctly specified." << THLA_ENDL;
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
         send_hs( stdout, "TrickHLA::ExecutionControlBase::receive_execution_configuration():%d This call \
will be ignored because the Simulation Initialization Scheme does not support it.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   // We can only receive the ExecutionConfiguration if we are not the master.
   if ( this->is_master() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      send_hs( stdout, "TrickHLA::ExecutionControlBase::receive_execution_configuration():%d Waiting...%c",
               __LINE__, THLA_NEWLINE );
   }

   // Make sure we have at least one piece of ExecutionConfiguration data we can receive.
   if ( execution_configuration->any_remotely_owned_subscribed_init_attribute() ) {

      SleepTimeout print_timer( federate->wait_status_time, THLA_DEFAULT_SLEEP_WAIT_IN_MICROS );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the data to arrive.
      while ( !execution_configuration->is_changed() ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         (void)sleep_timer.sleep();

         if ( !execution_configuration->is_changed() ) {

            if ( sleep_timer.timeout() ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "TrickHLA::ExecutionControlBase::receive_execution_configuration():" << __LINE__
                         << " Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!"
                         << THLA_ENDL;
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout() ) {
               print_timer.reset();
               send_hs( stdout, "TrickHLA::ExecutionControlBase::receive_execution_configuration():%d Waiting...%c",
                        __LINE__, THLA_NEWLINE );
            }
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         send_hs( stdout, "TrickHLA::ExecutionControlBase::receive_execution_configuration():%d Received data.%c",
                  __LINE__, THLA_NEWLINE );
      }

      // Receive the ExecutionConfiguration data from the master federate.
      execution_configuration->receive_init_data();

   } else {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControlBase::receive_execution_configuration():" << __LINE__
             << " ERROR: ExecutionConfiguration"
             << " is not configured to receive at least one object attribute."
             << " Make sure at least one ExecutionConfiguration attribute has"
             << " 'subscribe = true' set. Please check your input or modified-data"
             << " files to make sure the 'subscribe' value is correctly specified."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*!
 * @job_class{scheduled}
 */
void ExecutionControlBase::send_requested_data(
   double current_time,
   double job_cycle_time )
{
   // Send the requested data for the ExecutionConfiguration if we have one.
   if ( execution_configuration != NULL ) {
      // Send the data for the execution-configuration.
      execution_configuration->send_requested_data( current_time, job_cycle_time );
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
         this->process_execution_control_updates();
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void ExecutionControlBase::provide_attribute_update(
   ObjectInstanceHandle const &theObject,
   AttributeHandleSet const &  theAttributes )
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
   wstring const &obj_instance_name )
{
   wstring ws_obj_name;

   // Check to see if there is and ExecutionConfiguration object.
   if ( execution_configuration != NULL ) {

      // Execution Configuration object.
      StringUtilities::to_wstring( ws_obj_name, get_execution_configuration()->get_name() );
      if ( ws_obj_name == obj_instance_name ) {
         return ( execution_configuration );
      }
   }

   // Default return if we don't have an ExecutionConfiguration match.
   return ( static_cast< Object * >( NULL ) );
}

/*!
 * @job_class{scheduled}
 */
Object *ExecutionControlBase::get_unregistered_object(
   RTI1516_NAMESPACE::ObjectClassHandle const &theObjectClass,
   std::wstring const &                        theObjectInstanceName )
{
   wstring ws_obj_name;

   // Check to see if there is and ExecutionConfiguration object.
   if ( execution_configuration != NULL ) {

      // Check the execution configuration next.
      if ( ( execution_configuration->get_class_handle() == theObjectClass )
           && ( !execution_configuration->is_instance_handle_valid() ) ) {

         StringUtilities::to_wstring( ws_obj_name, get_execution_configuration()->get_name() );

         // Determine if the name matches the object instance name.
         if ( ws_obj_name == theObjectInstanceName ) {
            return ( execution_configuration );
         }
      }
   }

   // Default return if we don't have an ExecutionConfiguration match.
   return ( static_cast< Object * >( NULL ) );
}

/*!
 * @job_class{scheduled}
 */
Object *ExecutionControlBase::get_unregistered_remote_object(
   RTI1516_NAMESPACE::ObjectClassHandle const &theObjectClass )
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
   return ( static_cast< Object * >( NULL ) );
}

bool ExecutionControlBase::mark_object_as_deleted_from_federation(
   RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_id )
{
   // Remove the ExecitionControl object if present and the ID matches.
   if ( execution_configuration != NULL
        && ( execution_configuration->get_instance_handle() == instance_id ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_id );
         send_hs( stdout, "TrickHLA::ExecutionControlBase::mark_object_as_deleted_from_federation():%d Object '%s' Instance-ID:%s Valid-ID:%s %c",
                  __LINE__, execution_configuration->get_name(), id_str.c_str(),
                  ( instance_id.isValid() ? "Yes" : "No" ), THLA_NEWLINE );
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
      errmsg << "TrickHLA::ExecutionControlBase::get_sim_time():" << __LINE__
             << " WARNING: Unexpected NULL 'THLA.federate.get_sim_time'!"
             << " Please make sure you specify a sim-timeline in your input"
             << " file. Returning Trick simulation time instead!" << THLA_ENDL;
      send_hs( stdout, (char *)errmsg.str().c_str() );
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
      errmsg << "TrickHLA::ExecutionControlBase::get_scenario_time():" << __LINE__
             << " WARNING: Unexpected NULL 'THLA.federate.scenario_timeline'!"
             << " Please make sure you specify a scenario-timeline in your input"
             << " file. Returning Trick simulation time instead!" << THLA_ENDL;
      send_hs( stdout, (char *)errmsg.str().c_str() );
   }

   return this->get_sim_time();
}

double ExecutionControlBase::get_cte_time()
{
   if ( does_cte_timeline_exist() ) {
      return cte_timeline->get_time();
   }

   return -std::numeric_limits< double >::max();
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
      send_hs( stdout, "TrickHLA::ExecutionControlBase::enter_freeze():%d Freeze Announced:%s, Freeze Pending:%s%c",
               __LINE__, ( federate->get_freeze_announced() ? "Yes" : "No" ),
               ( federate->get_freeze_pending() ? "Yes" : "No" ), THLA_NEWLINE );
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

void ExecutionControlBase::check_pause( const double check_pause_delta )
{
   return;
}

void ExecutionControlBase::check_pause_at_init( const double check_pause_delta )
{
   // Dispatch to the ExecutionControl method.
   this->check_pause( check_pause_delta );

   // Mark that freeze has been announced in the Federate.
   federate->set_freeze_announced( this->is_master() );
}

void ExecutionControlBase::set_master( bool master_flag )
{
   // Don't change the master flag setting if the user has preset a value
   // in the input file.
   if ( !is_master_preset() ) {
      this->master = master_flag;
      // Make sure that the Execution Configuration object is set properly.
      if ( execution_configuration != NULL ) {
         execution_configuration->set_master( master_flag );
      }
   }
}

void ExecutionControlBase::setup_checkpoint()
{
   // Setup checkpoint for ExecutionConfiguration if we have one.
   if ( execution_configuration != NULL ) {
      // Any object with a valid instance handle must be marked as required
      // to ensure the restore process will wait for this object instance
      // to exist.
      if ( execution_configuration->is_instance_handle_valid() ) {
         execution_configuration->mark_required();
      }
      execution_configuration->setup_ownership_transfer_checkpointed_data();
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
   int64_t lcts )
{
   // TODO: Need more checking here.
   // WARNING: Only the Master federate should ever set this.
   if ( this->is_master() ) {
      this->least_common_time_step = lcts;
   }
}

void ExecutionControlBase::set_time_padding( double t )
{
   this->time_padding = t;
}
