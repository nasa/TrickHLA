/*!
@file SpaceFOM/ExecutionControl.cpp
@ingroup SpaceFOM
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
@trick_link_dependency{../TrickHLA/CTETimelineBase.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/ExecutionConfigurationBase.cpp}
@trick_link_dependency{../TrickHLA/Federate.cpp}
@trick_link_dependency{../TrickHLA/Int64BaseTime.cpp}
@trick_link_dependency{../TrickHLA/Int64Time.cpp}
@trick_link_dependency{../TrickHLA/InteractionItem.cpp}
@trick_link_dependency{../TrickHLA/Manager.cpp}
@trick_link_dependency{../TrickHLA/Parameter.cpp}
@trick_link_dependency{../TrickHLA/SleepTimeout.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{ExecutionConfiguration.cpp}
@trick_link_dependency{ExecutionControl.cpp}
@trick_link_dependency{RefFrameBase.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SpaceFOM support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <math.h>
#include <string>

// Trick includes.
#include "trick/Clock.hh"
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/CTETimelineBase.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/InteractionItem.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/ExecutionConfiguration.hh"
#include "SpaceFOM/ExecutionControl.hh"
#include "SpaceFOM/RefFrameBase.hh"
#include "SpaceFOM/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

#define THLA_TIME_DEBUG 0

// SpaceFOM file level declarations.
namespace SpaceFOM
{
// ExecutionControl type string.
std::string const ExecutionControl::type = "SpaceFOM";
} // namespace SpaceFOM

// Access the Trick global objects the Clock.
extern Trick::Clock *the_clock;

// Create the Trick ATTRIBUTES arrays needed for local allocations.
#ifdef __cplusplus
extern "C" {
#endif
// C based model includes.
extern ATTRIBUTES attrSpaceFOM__MTRInteractionHandler[];
#ifdef __cplusplus
}
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl()
   : TrickHLA::ExecutionControlBase(),
     designated_late_joiner( false ),
     pacing( false ),
     root_frame_pub( false ),
     root_ref_frame( NULL ),
     pending_mtr( SpaceFOM::MTR_UNINITIALIZED ),
     mtr_interaction( NULL ),
     mtr_interaction_handler( NULL )
{
   this->enable_least_common_time_step = true;
}

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl(
   SpaceFOM::ExecutionConfiguration &exec_config )
   : TrickHLA::ExecutionControlBase( exec_config ),
     designated_late_joiner( false ),
     pacing( false ),
     root_frame_pub( false ),
     root_ref_frame( NULL ),
     pending_mtr( SpaceFOM::MTR_UNINITIALIZED ),
     mtr_interaction( NULL ),
     mtr_interaction_handler( NULL )
{
   this->enable_least_common_time_step = true;
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
input.py files and reduce input.py file setting errors.

@job_class{initialization}
*/
void ExecutionControl::initialize()
{
#if THLA_TIME_DEBUG
   print_clock_summary( "ExecutionControl::initialize():" + std::to_string( __LINE__ ) + "\n" );
#endif

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg;
      msg << "SpaceFOM::ExecutionControl::initialize():" << __LINE__
          << " Initialization-Scheme:'" << get_type() << "'\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // There are things that must me set for the SpaceFOM initialization.
   this->use_preset_master = true;

   // If this is the Master or Pacing federate, then it must support Time
   // Management and be both Time Regulating and Time Constrained.
   if ( is_master() || is_pacing() ) {
      federate->time_management  = true;
      federate->time_regulating  = true;
      federate->time_constrained = true;
   }

   // For SpaceFOM, we must freeze on the Trick software frame boundary because
   // of the relationship between the Least Common Time Step (LCTS) and the
   // Trick software frame where the following must be true:
   // (LCTS >= software_frame) && (LCTS % software_frame == 0)
   exec_set_freeze_on_frame_boundary( true );

   // Add the Mode Transition Request synchronization points.
   add_sync_point( SpaceFOM::MTR_RUN_SYNC_POINT, SpaceFOM::SPACEFOM_SYNC_POINT_LIST );
   add_sync_point( SpaceFOM::MTR_FREEZE_SYNC_POINT, SpaceFOM::SPACEFOM_SYNC_POINT_LIST );
   add_sync_point( SpaceFOM::MTR_SHUTDOWN_SYNC_POINT, SpaceFOM::SPACEFOM_SYNC_POINT_LIST );

   // Make sure we initialize the base class.
   TrickHLA::ExecutionControlBase::initialize();

   // Must use a preset master.
   if ( !is_master_preset() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::initialize():" << __LINE__
             << " WARNING: Only a preset master is supported. Make sure to set"
             << " 'THLA.federate.use_preset_master = true' in your input.py file."
             << " Setting use_preset_master to true!\n";
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
      this->use_preset_master = true;
   }

   // A designated late joiner federate cannot be the preset Master.
   if ( is_designated_late_joiner() && is_master() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::initialize():" << __LINE__
             << " ERROR: This federate is configured as both a designated late"
             << " joiner and as the preset Master, which is not allowed. Check"
             << " your input.py file or Modified data files to make sure this"
             << " federate is not configured as both the preset master and a"
             << " designated late joiner.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      if ( is_master() ) {
         message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::initialize():%d\n    I AM THE PRESET MASTER\n",
                          __LINE__ );
      } else if ( is_designated_late_joiner() ) {
         message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::initialize():%d\n    I AM A DESIGNATED LATE JOINER AND NOT THE PRESET MASTER\n",
                          __LINE__ );
      } else {
         message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::initialize():%d\n    I AM NOT THE PRESET MASTER\n",
                          __LINE__ );
      }
   }
}

/*!
 * @details This routine is used to perform and inline build of the Trick
 * ref ATTRIBUTES for the SpaceFOM ExecutionControl.
 *
 * @job_class{initialization}
 */
void ExecutionControl::setup_object_ref_attributes()
{
   return;
}

/*!
 * @details This routine is used to perform and inline build of the Trick
 * ref ATTRIBUTES for the SpaceFOM Mode Transition Request (MTR) interaction.
 * This is used by federates, other than the Master, to request mode
 * transitions.
 *
 * @job_class{initialization}
 */
void ExecutionControl::setup_interaction_ref_attributes()
{
   // Allocate the Mode Transition Request Interaction.
   mtr_interaction = reinterpret_cast< Interaction * >( alloc_type( 1, "TrickHLA::Interaction" ) );
   if ( mtr_interaction == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::setup_MTR_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for Interaction specialized"
             << " to MTR the sim!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Set up name, handler, publish and subscribe.
   mtr_interaction->set_FOM_name( const_cast< char * >( "ModeTransitionRequest" ) ); // cppcheck-suppress [nullPointerRedundantCheck,unmatchedSuppression]
   mtr_interaction->set_handler( &mtr_interaction_handler );                         // cppcheck-suppress [nullPointerRedundantCheck,unmatchedSuppression]
   mtr_interaction_handler.set_name( "ModeTransitionRequest" );
   if ( is_master() ) {
      mtr_interaction->set_subscribe();
   } else {
      mtr_interaction->set_publish();
   }

   // Set up parameters.
   mtr_interaction->set_parameter_count( 1 );
   Parameter *tParm = reinterpret_cast< Parameter * >(
      alloc_type( mtr_interaction->get_parameter_count(),
                  "TrickHLA::Parameter" ) );
   if ( tParm == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to allocate enough memory for the parameters of the"
             << " MTR interaction!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   tParm[0].set_FOM_name( "execution_mode" );
   tParm[0].set_encoding( ENCODING_LITTLE_ENDIAN );

   mtr_interaction->set_parameters( tParm );

   // Since this is an interaction handler generated on the fly, there is no
   // Trick variable to resolve to at run time, which is supplied by the
   // input.py file. we must build data structures with sufficient information
   // for the Parameter class to link itself into the just generated
   // Freeze Interaction Handler, and its sole parameter ('execution_mode').

   // Allocate the trick ATTRIBUTES data structure with room for two
   // entries: 1) the 'execution_mode' parameter and 2) an empty entry
   // marking the end of the structure.
   ATTRIBUTES *mode_attr = static_cast< ATTRIBUTES * >( malloc( 2 * sizeof( ATTRIBUTES ) ) );
   if ( mode_attr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
             << " FAILED to aallocate enough memory for the ATTRIBUTES for the"
             << " 'execution_mode' value of the MTR interaction!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Find the 'execution_mode' value in the MTR Interaction Handler's ATTRIBUTES.
   // since we may not know the total # of elements, we look for an empty
   // element as an ending marker of the ATTRIBUTES.
   int attr_index = 0;

   // loop until the current ATTRIBUTES name is a NULL string
   while ( strcmp( attrSpaceFOM__MTRInteractionHandler[attr_index].name, "" ) != 0 ) {
      if ( strcmp( attrSpaceFOM__MTRInteractionHandler[attr_index].name, "mtr_mode_int" ) == 0 ) {
         memcpy( &mode_attr[0],
                 &attrSpaceFOM__MTRInteractionHandler[attr_index],
                 sizeof( ATTRIBUTES ) );
      }
      ++attr_index;
   }

   // Now that we have hit the end of the ATTRIBUTES array, copy the last
   // entry into my mode_attr array to make it a valid ATTRIBUTE array.
   memcpy( &mode_attr[1],
           &attrSpaceFOM__MTRInteractionHandler[attr_index],
           sizeof( ATTRIBUTES ) );

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg2;
      msg2 << "SpaceFOM::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__ << '\n'
           << "--------------- Trick REF-Attributes ---------------\n"
           << " FOM-Interaction:'" << mtr_interaction->get_FOM_name() << "'\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   // Initialize the TrickHLA Interaction before we use it.
   mtr_interaction->initialize( this->manager );

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg2;
      msg2 << "SpaceFOM::ExecutionControl::setup_interaction_ref_attributes():" << __LINE__
           << " FOM-Parameter:'" << tParm[0].get_FOM_name() << "'"
           << " NOTE: This is an auto-generated parameter so there is no"
           << " associated 'Trick-Name'.\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   // Initialize the TrickHLA Parameter. Since we built the interaction handler
   // in-line, and not via the Trick input.py file, use the alternate version of
   // the initialize routine which does not resolve the fully-qualified Trick
   // name to access the ATTRIBUTES if the trick variable...
   tParm[0].initialize( mtr_interaction->get_FOM_name(),
                        mtr_interaction_handler.get_address_of_interaction_mode(),
                        static_cast< ATTRIBUTES * >( mode_attr ) );
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::setup_object_RTI_handles()
{
   ExecutionConfiguration *ExCO = get_execution_configuration();
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL ExCO!\n";
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
   // Setup the RTI handles for the SpaceFOM MTR Interaction.
   if ( mtr_interaction != NULL ) {
      // SpaceFOM Mode Transition Request (MTR) Interaction.
      manager->setup_interaction_RTI_handles( 1, mtr_interaction );
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::add_initialization_sync_points()
{
   // Add the initialization synchronization points used for startup regulation.
   add_sync_point( SpaceFOM::OBJECTS_DISCOVERED_SYNC_POINT, SpaceFOM::SPACEFOM_SYNC_POINT_LIST );
   add_sync_point( SpaceFOM::ROOT_FRAME_DISCOVERED_SYNC_POINT, SpaceFOM::SPACEFOM_SYNC_POINT_LIST );
   add_sync_point( SpaceFOM::INIT_COMPLETED_SYNC_POINT, SpaceFOM::SPACEFOM_SYNC_POINT_LIST );
   add_sync_point( SpaceFOM::INIT_STARTED_SYNC_POINT, SpaceFOM::SPACEFOM_SYNC_POINT_LIST );

   // Add the multiphase initialization synchronization points.
   add_multiphase_init_sync_points();
}

void ExecutionControl::sync_point_announced(
   wstring const          &label,
   RTI1516_USERDATA const &user_supplied_tag )
{
   // Unrecognized sync-point label if not seen before or if it is in the
   // Unknown list (i.e. seen before but still unrecognized).
   if ( !contains_sync_point( label ) || contains_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {

      // Unrecognized sync-point. Achieve all unrecognized sync-points.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         message_publish( MSG_NORMAL, "=========================== SpaceFOM::ExecutionControl::sync_point_announced():%d Unrecognized sync-point:'%s', which will be achieved.\n",
                          __LINE__, label_str.c_str() );
      }

      // Achieve all Unrecognized sync-points but don't wait for the
      // federation to be synchronized on it.
      if ( !achieve_sync_point( label, user_supplied_tag ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         message_publish( MSG_WARNING, "SpaceFOM::ExecutionControl::sync_point_announced():%d Failed to achieve unrecognized sync-point:'%s'.\n",
                          __LINE__, label_str.c_str() );
      }
   } else {
      // Known sync-point that is already in one of the sync-point lists.

      // Mark known sync-point as announced.
      if ( mark_sync_point_announced( label, user_supplied_tag ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            string label_str;
            StringUtilities::to_string( label_str, label );
            message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::sync_point_announced():%d Marked sync-point announced:'%s'\n",
                             __LINE__, label_str.c_str() );
         }
      } else {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SpaceFOM::ExecutionControl::sync_point_announced():" << __LINE__
                << " ERROR: Failed to mark sync-point '" << label_str
                << "' as announced.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // For a designated late joiner, achieve sync-points during initialization.
      if ( is_designated_late_joiner()
           && ( ( get_current_execution_control_mode() == EXECUTION_CONTROL_INITIALIZING )
                || ( get_current_execution_control_mode() == EXECUTION_CONTROL_UNINITIALIZED ) ) ) {

         // Achieve all sync-points during initialization except for
         // init-completed and mtr-shutdown.
         if ( ( label.compare( SpaceFOM::INIT_COMPLETED_SYNC_POINT ) != 0 )
              && ( label.compare( SpaceFOM::MTR_SHUTDOWN_SYNC_POINT ) != 0 ) ) {

            // Achieve all other sync-points.
            if ( achieve_sync_point( label, user_supplied_tag ) ) {
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  string label_str;
                  StringUtilities::to_string( label_str, label );
                  message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::sync_point_announced():%d SpaceFOM designated late joiner, achieved sync-point:'%s'\n",
                                   __LINE__, label_str.c_str() );
               }
            }
         }
      }
   }
}

void ExecutionControl::publish()
{
   // Check to see if we are the Master federate.
   if ( is_master() ) {
      // Publish the execution configuration if we are the master federate.
      execution_configuration->publish_object_attributes();
   } else {
      // Only publish the MTR interaction if we are NOT the Master federate.
      mtr_interaction->publish_interaction();
   }
}

void ExecutionControl::unpublish()
{
   // Unpublish the execution configuration object.
   if ( is_master() ) {
      // Unpublish the execution configuration if we are the master federate.
      execution_configuration->unpublish_all_object_attributes();
   }

   // Unpublish the mtr_interactions.
   // Only publish an MTR interaction that we publish.
   if ( mtr_interaction->is_publish() ) {
      mtr_interaction->unpublish_interaction();
   }
}

void ExecutionControl::subscribe()
{
   // Check to see if we are the Master federate.
   if ( is_master() ) {
      // Only subscribe to the MTR interaction if this is the Master federate.
      mtr_interaction->subscribe_to_interaction();
   } else {
      // Subscribe to the execution configuration if we are not the master federate.
      execution_configuration->subscribe_to_object_attributes();
   }
}

void ExecutionControl::unsubscribe()
{
   // Check to see if we are the Master federate.
   if ( !is_master() ) {
      // Unsubscribe from the execution configuration if we are NOT the Master federate.
      execution_configuration->unsubscribe_all_object_attributes();
   }

   // Unsubscribe the mtr_interactions.
   // Only unsubscribe an MTR interaction that we subscribe to.
   if ( mtr_interaction->is_subscribe() ) {
      mtr_interaction->unsubscribe_from_interaction();
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
   // Process the MTR interaction if we subscribed to it and we have the
   // same class handle.
   if ( mtr_interaction->is_subscribe()
        && ( mtr_interaction->get_class_handle() == theInteraction ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         string handle;
         StringUtilities::to_string( handle, theInteraction );

         if ( received_as_TSO ) {
            Int64Time _time;
            _time.set( theTime );

            message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::receive_interaction(ModeTransitionRequest):%d ID:%s, HLA-time:%G\n",
                             __LINE__, handle.c_str(), _time.get_time_in_seconds() );
         } else {
            message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::receive_interaction(ModeTransitionRequest):%d ID:%s\n",
                             __LINE__, handle.c_str() );
         }
      }

      if ( received_as_TSO ) {
         InteractionItem item( 0,
                               INTERACTION_TYPE_BUILTIN_MTR,
                               mtr_interaction->get_parameter_count(),
                               mtr_interaction->get_parameters(),
                               theParameterValues,
                               theUserSuppliedTag,
                               theTime );

         mtr_interaction->extract_data( &item );
         mtr_interaction->process_interaction();
      } else {
         InteractionItem item( 0,
                               INTERACTION_TYPE_BUILTIN_MTR,
                               mtr_interaction->get_parameter_count(),
                               mtr_interaction->get_parameters(),
                               theParameterValues,
                               theUserSuppliedTag );

         mtr_interaction->extract_data( &item );
         mtr_interaction->process_interaction();
      }

      // Indicate we processed the interaction.
      return true;
   }

   // Not processed
   return false;
}

/*!
@details This routine implements the SpaceFOM Join Federation Process described
in section 7.2 and figure 7-3.

@job_class{initialization}
*/
void ExecutionControl::join_federation_process()
{
   // The base class implementation is good enough for now.
   TrickHLA::ExecutionControlBase::join_federation_process();
}

/*!
@details This routine implements the SpaceFOM Role Determination Process
described in section 7.2 and figure 7-4.

@job_class{initialization}
*/
void ExecutionControl::role_determination_process()
{
   // Initialize the MOM interface handles.
   federate->initialize_MOM_handles();

   // Check for Master initialization lane.
   if ( is_master() ) {

      // The Master federate MUST be an early joiner.
      this->late_joiner            = false;
      this->late_joiner_determined = true;

      // Backup then disable the Auto-Provide setting in the switches table.
      federate->backup_auto_provide_setting_from_MOM_then_disable();

      // Make sure all required federates have joined the federation.
      federate->wait_for_required_federates_to_join();

      // Register the initialization synchronization points used to control
      // the SpaceFOM startup process. Section 7.2 Figure 7-4.
      register_sync_point( SpaceFOM::INIT_STARTED_SYNC_POINT,
                           federate->get_joined_federate_handles() );
      register_sync_point( SpaceFOM::OBJECTS_DISCOVERED_SYNC_POINT,
                           federate->get_joined_federate_handles() );
      register_sync_point( SpaceFOM::ROOT_FRAME_DISCOVERED_SYNC_POINT,
                           federate->get_joined_federate_handles() );

      // Register all the user defined multiphase initialization
      // synchronization points just for the joined federates.
      register_all_sync_points( TrickHLA::MULTIPHASE_INIT_SYNC_POINT_LIST,
                                federate->get_joined_federate_handles() );
   } else {

      //
      // Determine if we are a early or late joiner.
      //

      // Print out diagnostic message if appropriate.
      if ( !this->late_joiner_determined ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::role_determination_process():%d Waiting...\n",
                             __LINE__ );
         }
      }

      bool         print_summary = DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL );
      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer;

      // Block until we have determined if we are a late joining federate.
      while ( !this->late_joiner_determined ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         // We are not a late joiner if we received the announce for the
         // 'initialization started' sync-point. (Nominal Initialization)

         if ( is_sync_point_announced( SpaceFOM::INIT_STARTED_SYNC_POINT ) ) {
            this->late_joiner            = false;
            this->late_joiner_determined = true;
         }

         // Determine if the Initialization Complete sync-point is announded,
         // which means at this point we are a late joining federate.
         if ( ( !late_joiner_determined ) && is_sync_point_announced( SpaceFOM::INIT_COMPLETED_SYNC_POINT ) ) {
            this->late_joiner            = true;
            this->late_joiner_determined = true;
         }

         // Wait for a little while to give the sync-points time to come in.
         if ( !this->late_joiner_determined ) {

            // Check for shutdown.
            federate->check_for_shutdown_with_termination();

            // Short sleep to release process and not hog CPU.
            sleep_timer.sleep();

            // Periodically check if we are still an execution member and
            // display sync-point status if needed as well.
            if ( !this->late_joiner_determined ) { // cppcheck-suppress [knownConditionTrueFalse]

               // To be more efficient, we get the time once and share it.
               wallclock_time = sleep_timer.time();

               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();

                  if ( !print_summary ) {
                     print_summary = DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL );
                  }

                  // Check that we maintain federation membership.
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "SpaceFOM::ExecutionControl::role_determination_process():" << __LINE__
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
                  print_summary = true;
               }
            }
         }

         if ( print_summary ) {
            print_summary = false;

            string init_completed_label;
            StringUtilities::to_string( init_completed_label, SpaceFOM::INIT_COMPLETED_SYNC_POINT );
            ostringstream message;
            message << "SpaceFOM::ExecutionControl::role_determination_process():"
                    << __LINE__ << " Sync-point status: "
                    << to_string( SpaceFOM::INIT_STARTED_SYNC_POINT )
                    << ", '" << init_completed_label << "' sync-point announced: "
                    << ( is_sync_point_announced( SpaceFOM::INIT_COMPLETED_SYNC_POINT ) ? "Yes" : "No" )
                    << ", Still waiting...\n";
            message_publish( MSG_NORMAL, message.str().c_str() );
         }
      }

      // Display a status message for the role of this federate.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         if ( this->late_joiner ) {
            if ( is_designated_late_joiner() ) {
               message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::role_determination_process():%d This is a Designated Late Joining Federate.\n",
                                __LINE__ );
            } else {
               message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::role_determination_process():%d This is a Late Joining Federate.\n",
                                __LINE__ );
            }
         } else {
            if ( is_designated_late_joiner() ) {
               message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::role_determination_process():%d This is an Early Joining Federate configured to be a Designated Late Joining Federate.\n",
                                __LINE__ );
            } else {
               message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::role_determination_process():%d This is an Early Joining Federate.\n",
                                __LINE__ );
            }
         }
      }

   } // End of check for Master federate.
}

/*!
@details This routine implements the SpaceFOM Master and Early Joiner HLA
initialization process described in section 7.2 and figure 7-5.

@job_class{initialization}
*/
void ExecutionControl::early_joiner_hla_init_process()
{
   ExecutionConfigurationBase *ExCO = get_execution_configuration();

   // Wait for the SpaceFOM initialization ExecutionConrtol synchronization
   // points for Early Joiner: INIT_STARTED_SYNC_POINT,
   // OBJECTS_DISCOVERED_SYNC_POINT, and ROOT_FRAME_DISCOVERED_SYNC_POINT.
   // and "startup" sync-points to be registered (i.e. announced).
   // Note: Do NOT register the INIT_COMPLETED_SYNC_POINT synchronization
   // point yet. That marks the successful completion of initialization.

   wait_for_sync_point_announced( SpaceFOM::INIT_STARTED_SYNC_POINT );
   wait_for_sync_point_announced( SpaceFOM::OBJECTS_DISCOVERED_SYNC_POINT );
   wait_for_sync_point_announced( SpaceFOM::ROOT_FRAME_DISCOVERED_SYNC_POINT );

   // Setup all the RTI handles for the objects, attributes and interaction
   // parameters.
   manager->setup_all_RTI_handles();

   // Call publish_and_subscribe AFTER we've initialized the manager,
   // federate, and FedAmb.
   manager->publish_and_subscribe();

   // If this is the Master federate, then setup the ExCO.
   if ( is_master() ) {
      // Reserve ExCO object instance name.
      ExCO->reserve_object_name_with_RTI();

      // Wait for success or failure for the ExCO name reservation.
      ExCO->wait_for_object_name_reservation();
   } else {
      // NOTE:
      // Should publish the MTR interaction here. However, this
      // is currently handled in the manager->setup_all_RTI_handles()
      // and manager->publish_and_subscribe()!
   }

   // Reserve the RTI object instance names with the RTI, but only for
   // the objects that are locally owned.
   manager->reserve_object_names_with_RTI();

   // Waits on the reservation of the RTI object instance names for the
   // locally owned objects. Calling this function will block until all
   // the object instances names for the locally owned objects have been
   // reserved.
   manager->wait_for_reservation_of_object_names();

   // Creates an RTI object instance and registers it with the RTI, but
   // only for the objects that we create.
   manager->register_objects_with_RTI();

   // Waits on the registration of all the required RTI object instances
   // with the RTI. Calling this function will block until all the
   // required object instances in the Federation have been registered.
   manager->wait_for_registration_of_required_objects();

   // Achieve the "objects_discovered" sync-point and wait for the
   // federation to be synchronized on it. There is a race condition
   // between when objects are discovered and we start sending data.
   // Initialization data could be sent before a federate has even
   // discovered an object instance resulting in the federate not receiving
   // the expected data.
   achieve_sync_point_and_wait_for_synchronization( SpaceFOM::OBJECTS_DISCOVERED_SYNC_POINT );
}

/*!
@details This routine implements the SpaceFOM Designated Late Joiner
initialization process that achieves any unknown announced sync-points and
waits for the initialization_complete sync-point to be announced.

@job_class{initialization}
*/
void ExecutionControl::designated_late_joiner_init_process()
{
   // Master Federate can not be a designated late joiner or if are not
   // configured by the user to be a designated late joiner just return.
   if ( is_master() || !is_designated_late_joiner() ) {
      return;
   }

   // Override settings because this is a designated late joiner.
   this->late_joiner            = true;
   this->late_joiner_determined = true;

   // Print out diagnostic message if appropriate.
   if ( !is_sync_point_announced( SpaceFOM::INIT_COMPLETED_SYNC_POINT ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::designated_late_joiner_init_process():%d Waiting...\n",
                          __LINE__ );
      }
   }

   bool         print_summary = DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL );
   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   // Block until we see the intitialization_complete sync-point announced.
   while ( !is_sync_point_announced( SpaceFOM::INIT_COMPLETED_SYNC_POINT ) ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // Short sleep to release process and not hog CPU.
      sleep_timer.sleep();

      // Periodically check if we are still an execution member and
      // display sync-point status if needed as well.
      if ( !is_sync_point_announced( SpaceFOM::INIT_COMPLETED_SYNC_POINT ) ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();

            if ( !print_summary ) {
               print_summary = DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL );
            }

            // Check that we maintain federation membership.
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SpaceFOM::ExecutionControl::designated_late_joiner_init_process():" << __LINE__
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
            print_summary = true;
         }
      }

      if ( print_summary ) {
         print_summary = false;

         string sp_label;
         StringUtilities::to_string( sp_label, SpaceFOM::INIT_COMPLETED_SYNC_POINT );
         ostringstream message;
         message << "SpaceFOM::ExecutionControl::designated_late_joiner_init_process():"
                 << __LINE__
                 << " Sync-point '" << sp_label << "' announced:"
                 << ( is_sync_point_announced( SpaceFOM::INIT_COMPLETED_SYNC_POINT ) ? "Yes" : "No, Still waiting..." )
                 << '\n';
         message_publish( MSG_NORMAL, message.str().c_str() );
      }
   }

   // Print out diagnostic message if appropriate.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::designated_late_joiner_init_process():%d This is a Designated Late Joining Federate.\n",
                       __LINE__ );
   }
}

/*!
@details This routine implements the SpaceFOM Late Joiner initialization process
described in section 7.2 and figure 7-9.

@job_class{initialization}
*/
void ExecutionControl::late_joiner_hla_init_process()
{
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // Setup all the RTI handles for the objects, attributes and interaction
   // parameters.
   manager->setup_all_RTI_handles();

   // Subscribe to the ExCO attributes.
   ExCO->subscribe_to_object_attributes();

   // NOTE:
   // Should publish the MTR interaction here. However, this
   // is currently handled in the manager->setup_all_RTI_handles()
   // and manager->publish_and_subscribe()!

   // Wait for the registration of the ExCO. Calling this function will
   // block until the ExCO object instances has been registered.
   ExCO->wait_for_registration();

   // Request a ExCO object update.
   manager->request_data_update( ExCO->get_name() );

   // Wait for the ExCO attribute reflection.
   ExCO->wait_for_update();

   // Set scenario timeline epoch and time.
   scenario_timeline->set_epoch( ExCO->get_scenario_time_epoch() );

   // Print diagnostic message if appropriate.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg;
      msg << "SpaceFOM::ExecutionControl::late_joiner_hla_init_process()\n"
          << "\t current_scenario_time:     " << setprecision( 18 ) << scenario_timeline->get_time() << '\n'
          << "\t scenario_time_epoch:       " << setprecision( 18 ) << scenario_timeline->get_epoch() << '\n'
          << "\t scenario_time_epoch(ExCO): " << setprecision( 18 ) << ExCO->scenario_time_epoch << '\n'
          << "\t scenario_time_sim_offset:  " << setprecision( 18 ) << scenario_timeline->get_sim_offset() << '\n'
          << "\t Current HLA grant time:    " << setprecision( 18 ) << federate->get_granted_time().get_time_in_seconds() << '\n'
          << "\t Current HLA request time:  " << setprecision( 18 ) << federate->get_requested_time().get_time_in_seconds() << '\n'
          << "\t current_sim_time:          " << setprecision( 18 ) << sim_timeline->get_time() << '\n'
          << "\t simulation_time_epoch:     " << setprecision( 18 ) << sim_timeline->get_epoch() << '\n';
      if ( does_cte_timeline_exist() ) {
         msg << "\t current-CTE-time:          " << setprecision( 18 ) << cte_timeline->get_time() << '\n'
             << "\t CTE-time-epoch:            " << setprecision( 18 ) << cte_timeline->get_epoch() << '\n';
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Set the requested execution mode from the ExCO.
   set_requested_execution_control_mode( ExCO->next_execution_mode );

   // Process the just received ExCO update.
   process_execution_control_updates();

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
   // only for the objects that we create.
   manager->register_objects_with_RTI();

   // Waits on the registration of all the required RTI object instances
   // with the RTI. Calling this function will block until all the
   // required object instances in the Federation have been registered.
   manager->wait_for_registration_of_required_objects();

   // NOTE: The remainder of this SpaceFOM process is handled in the
   // TrickHLA::Manager::initialization_complete() step after any
   // potential Late Joiner specific multi-phase initialization with
   // other late joining federates.
}

/*!
@details This routine implements the SpaceFOM pre multi-phase initialization
process described in section 7.2 figure 7-2.

@job_class{initialization}
*/
void ExecutionControl::pre_multi_phase_init_processes()
{
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // The User Must specify an ExCO.
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
             << " ERROR: Unexpected NULL THLA.manager.exec_config object.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Make sure the ExCO has at least a FOM-name to be valid.
   if ( ExCO->get_FOM_name() == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
             << " ERROR: Unexpected NULL FOM-name for the THLA.manager.exec_config object.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // The User Must specify a root reference frame.
   if ( this->root_ref_frame == NULL ) {
      // The Master federate and the Root Reference Frame Publisher federate
      // must have the root_ref_frame reference set.
      if ( is_master() || is_root_frame_publisher() ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                << " ERROR: Unexpected NULL THLA.manager.root_ref_frame object.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::pre_multi_phase_init_processes():%d WARNING: No root reference frame!\n",
                             __LINE__ );
         }
      }
   }

   // If we are the master federate then validate the Least-Common-Time-Step
   // (LCTS) time.
   if ( is_master() ) {

      // The LCTS must match in both this ExecutionControl and the
      // associated ExCO.
      if ( this->least_common_time_step != ExCO->get_least_common_time_step() ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                << " ERROR: Least-Common-Time-Step (LCTS) time ("
                << this->least_common_time_step << " " << Int64BaseTime::get_units()
                << ") is not equal to ExCO LCTS ("
                << ExCO->get_least_common_time_step() << " " << Int64BaseTime::get_units()
                << ")!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // The Master federate must have a padding time set.
      if ( get_time_padding() <= 0.0 ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                << " ERROR: For this Master federate, the time padding ("
                << get_time_padding() << " seconds) must be greater than zero!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // Verify the Master federate time constraints.
      if ( !federate->verify_time_constraints() ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                << " ERROR: Time constraints verification failed!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // Set the ExCO current and next run modes.
   set_current_execution_control_mode( EXECUTION_CONTROL_INITIALIZING );
   ExCO->set_current_execution_mode( EXECUTION_MODE_INITIALIZING );
   set_requested_execution_control_mode( EXECUTION_CONTROL_INITIALIZING );
   if ( is_master() ) {
      set_next_execution_control_mode( EXECUTION_CONTROL_INITIALIZING );
   }

   // Reset the ExCO required flag to make it required.
   ExCO->mark_required();

   // Reset the ExCO preferred-order for attributes to Receive-Order.
   ExCO->reset_preferred_order();

   // Reset the ownership flags and the attribute configuration flags for
   // the ExCO object. This resets all attribute config states to CONFIG_INITIALIZE.
   // This also resets the create instance flag to true.
   ExCO->reset_ownership_states();

   // Setup the ExCO object now because we use a preset master.
   ExCO->set_master( is_master() );

   // Setup all the Trick Ref-Attributes for the user specified objects,
   // attributes, interactions and parameters.
   manager->setup_all_ref_attributes();

   // Add the SpaceFOM ExecutionControl and user defined multiphase
   // initialization synchronization points to the list before we join the
   // federation to avoid a race condition with announces.
   add_initialization_sync_points();

   // Perform the SpaceFOM join federation process.
   join_federation_process();

   // Perform the SpaceFOM role determination process.
   role_determination_process();

   // Branch off between Early and Late Joiner initialization.
   if ( late_joiner ) {

      // Perform Late Joiner HLA initialization process.
      late_joiner_hla_init_process();

   } else if ( !is_master() && is_designated_late_joiner() ) {
      // Early Joiner but this federate is a designated late joiner.

      // Wait for initialization_complete sync-point and achieve unknown sync-points.
      designated_late_joiner_init_process();

      // Perform Late Joiner HLA initialization process.
      late_joiner_hla_init_process();

   } else {
      // Early Joiners

      // Perform Early Joiner HLA initialization process.
      early_joiner_hla_init_process();

      // Perform the scenario epoch and root reference frame discovery process.
      epoch_and_root_frame_discovery_process();
   }

   if ( !is_master() ) {
      // Verify the non-Master federate time constraints.
      if ( !federate->verify_time_constraints() ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
                << " ERROR: Time constraints verification failed!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}

/*!
@details This routine implements the SpaceFOM post multi-phase initialization
process described in section 7.2 and figures 7-8 and 7-9.

@job_class{initialization}
*/
void ExecutionControl::post_multi_phase_init_processes()
{
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // Setup HLA time management.
   federate->setup_time_management();

   // Branch between late joining and early joining federates.
   if ( is_late_joiner() ) {

      //
      // Late Joining Federate (SpaceFOM Fig: 7-9).
      //

      // Jump to the a logical time that is a GALT time that is an integer
      // multiple of the least-common-lookahead (CL) time, otherwise we
      // will not be in sync with the other federates on the HLA logical
      // timeline.
      federate->time_advance_request_to_GALT_LCTS_multiple();

      // Need to compute the late joiner simulation time offset for the
      // scenario time line.
      scenario_timeline->set_sim_offset( federate->get_requested_time().get_time_in_seconds() );

      // Print diagnostic message if appropriate.
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         ostringstream msg;
         msg << "SpaceFOM::ExecutionControl::late_joiner_hla_init_process()\n"
             << "\t current_scenario_time:     " << setprecision( 18 ) << scenario_timeline->get_time() << '\n'
             << "\t scenario_time_epoch:       " << setprecision( 18 ) << scenario_timeline->get_epoch() << '\n'
             << "\t scenario_time_epoch(ExCO): " << setprecision( 18 ) << ExCO->scenario_time_epoch << '\n'
             << "\t scenario_time_sim_offset:  " << setprecision( 18 ) << scenario_timeline->get_sim_offset() << '\n'
             << "\t Current HLA grant time:    " << setprecision( 18 ) << federate->get_granted_time().get_time_in_seconds() << '\n'
             << "\t Current HLA request time:  " << setprecision( 18 ) << federate->get_requested_time().get_time_in_seconds() << '\n'
             << "\t current_sim_time:          " << setprecision( 18 ) << sim_timeline->get_time() << '\n'
             << "\t simulation_time_epoch:     " << setprecision( 18 ) << sim_timeline->get_epoch() << '\n';
         if ( does_cte_timeline_exist() ) {
            msg << "\t current_CTE_time:          " << setprecision( 18 ) << cte_timeline->get_time() << '\n'
                << "\t CTE_time_epoch:            " << setprecision( 18 ) << cte_timeline->get_epoch() << '\n';
         }
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      // Check the current ExecutionControl execution mode to figure out
      // if we need to go to run, freeze, or shutdown.
      switch ( this->requested_execution_control_mode ) {

         case EXECUTION_CONTROL_RUNNING: {
            // Since this is a late joining federate, do NOT call the SpaceFOM
            // ExecutionControl run_mode_transition() function here. Late joiners
            // do NOT use the 'mtr_run' sync-points at initialization. Late
            // joining federates go straight to run if the federation is in
            // run execution mode.

            // Set the current execution mode to running.
            this->current_execution_control_mode = EXECUTION_CONTROL_RUNNING;

            // Check to make sure that Trick is not starting in freeze.
            the_exec->set_freeze_command( false );
            break;
         }
         case EXECUTION_CONTROL_FREEZE: {
            // Since this is a late joining federate, do NOT call the
            // SpaceFOM ExecutionControl freeze_mode_transition() function here.
            // Late joiners do NOT use the 'mtr_freeze' sync-points at
            // initialization. Instead, tell the Trick executive to startup
            // in Freeze mode.

            // Tell the Trick executive to start up in freeze mode.
            the_exec->set_freeze_command( true );
            break;
         }
         case EXECUTION_CONTROL_SHUTDOWN: {

            if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::post_multi_phase_init_process():%d Commanding Trick Exec to stop.\n",
                                __LINE__ );
            }

            // Tell Trick to shutdown.
            // The SpaceFOM ExecutionControl shutdown transition will be made from
            // the TrickHLA::Federate::shutdown() job.
            the_exec->stop();
            break;
         }
         default: {
            ExCO->print_execution_configuration();
            ostringstream errmsg;
            errmsg << "SpaceFOM::ExecutionControl::post_multi_phase_init_process():" << __LINE__
                   << " ERROR: SpaceFOM ExecutionControl invalid execution mode"
                   << " (" << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << "), exiting...\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
      }

   } else {

      //
      // Early Joining Federate (SpaceFOM Fig: 7-8).
      //

      // Achieve the "initialization_started" sync-point and wait for the
      // federation to be synchronized on it.
      achieve_sync_point_and_wait_for_synchronization( SpaceFOM::INIT_STARTED_SYNC_POINT );

      // Check to see if this is the Master federate.
      if ( is_master() ) {

         // Restore the original Auto-Provide state.
         federate->restore_orig_MOM_auto_provide_setting();

         // Let the late joining federates know that we have completed initialization.
         register_sync_point( SpaceFOM::INIT_COMPLETED_SYNC_POINT );

         // Check for an initialization mode transition request.
         if ( check_mode_transition_request() ) {

            // Since this is a valid MTR, set the next mode from the MTR.
            set_mode_request_from_mtr( this->pending_mtr );

            switch ( this->requested_execution_control_mode ) {

               case EXECUTION_CONTROL_RUNNING: {
                  // Tell Trick to startup in Run.
                  the_exec->set_freeze_command( false );
                  break;
               }
               case EXECUTION_CONTROL_FREEZE: {
                  // Tell Trick to startup in Freeze.
                  the_exec->set_freeze_command( true );
                  break;
               }
               case EXECUTION_CONTROL_SHUTDOWN: {

                  // Announce the shutdown.
                  shutdown_mode_announce();

                  // Tell the TrickHLA::Federate to shutdown.
                  // The SpaceFOM ExecutionControl shutdown transition will be made
                  // from the TrickHLA::Federate::shutdown() job.
                  the_exec->stop();
                  break;
               }
               default: {
                  ExCO->print_execution_configuration();

                  ostringstream errmsg;
                  errmsg << "SpaceFOM::ExecutionControl::post_multi_phase_init_process():" << __LINE__
                         << " ERROR: SpaceFOM ExecutionControl invalid execution mode"
                         << " (" << execution_control_enum_to_string( this->requested_execution_control_mode )
                         << "), exiting...\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
                  break;
               }
            }

         } // End of MTR check.

         // Check with Trick to see if we are starting in Freeze.
         if ( the_exec->get_freeze_command() ) {

            // Set the next execution mode to freeze.
            set_next_execution_control_mode( EXECUTION_CONTROL_FREEZE );

            // Send the ExCO with the updated run mode.
            ExCO->send_init_data();

            // Announce freeze mode.
            freeze_mode_announce();

            // NOTE: This is handled in the freeze_init job.
            // Transition to freeze mode.
            // freeze_mode_transition();

         } else {
            // Not starting in Freeze.

            // Set the next execution mode to running.
            set_next_execution_control_mode( EXECUTION_CONTROL_RUNNING );

            // Send the ExCO with the updated run mode.
            ExCO->send_init_data();

            // Transition to run mode.
            run_mode_transition();

         } // End of check for starting in Freeze.

      } else {
         // This is an early joining but not a Master federate.

         // Wait on the ExCO update.
         ExCO->wait_for_update();

         // Process the just received ExCO update.
         process_execution_control_updates();

      } // End of check for Master federate.

   } // End of check for late v. early joining federate.
}

/*!
 * @job_class{shutdown}
 */
void ExecutionControl::shutdown()
{
   // If this is the Master, let's pause for a moment to let things
   // propagate through the federate before tearing things down.
   if ( current_execution_control_mode != EXECUTION_CONTROL_SHUTDOWN ) {

      // Tell Execution Control to announce the shutdown.
      if ( is_master() ) {

         // Tell the SpaceFOM execution control to announce the shutdown.
         shutdown_mode_announce();

         // Let's pause for a moment (1 second) to let things propagate
         // through the federation before tearing things down.
         Utilities::micro_sleep( 1000000 );
      }

      // Tell the SpaceFOM execution control to transition to shutdown.
      shutdown_mode_transition();
   }
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
   ExecutionConfiguration const *ExCO = get_execution_configuration();
   if ( ExCO != NULL ) {
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
   }
   return false;
}

void ExecutionControl::set_mode_request_from_mtr(
   MTREnum mtr_value )
{
   switch ( mtr_value ) {
      case MTR_UNINITIALIZED: {
         this->pending_mtr = MTR_UNINITIALIZED;
         set_next_execution_control_mode( EXECUTION_CONTROL_UNINITIALIZED );
         break;
      }
      case MTR_INITIALIZING: {
         this->pending_mtr = MTR_INITIALIZING;
         set_next_execution_control_mode( EXECUTION_CONTROL_INITIALIZING );
         break;
      }
      case MTR_GOTO_RUN: {
         this->pending_mtr = MTR_GOTO_RUN;
         set_next_execution_control_mode( EXECUTION_CONTROL_RUNNING );
         break;
      }
      case MTR_GOTO_FREEZE: {
         this->pending_mtr = MTR_GOTO_FREEZE;
         set_next_execution_control_mode( EXECUTION_CONTROL_FREEZE );
         break;
      }
      case MTR_GOTO_SHUTDOWN: {
         this->pending_mtr = MTR_GOTO_SHUTDOWN;
         set_next_execution_control_mode( EXECUTION_CONTROL_SHUTDOWN );
         break;
      }
      default: {
         this->pending_mtr = MTR_UNINITIALIZED;
         break;
      }
   }
}

void ExecutionControl::set_next_execution_control_mode(
   TrickHLA::ExecutionControlEnum exec_control )
{
   // Reference the SpaceFOM Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // This should only be called by the Master federate.
   if ( !is_master() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::set_next_execution_control_mode():" << __LINE__
             << " ERROR: This should only be called by the Master federate!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   switch ( exec_control ) {
      case EXECUTION_CONTROL_UNINITIALIZED: {

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_UNINITIALIZED;
         ExCO->set_next_execution_mode( EXECUTION_MODE_UNINITIALIZED );

         // Set the next mode times.
         this->next_mode_scenario_time = get_scenario_time();      // Immediate
         ExCO->set_next_mode_scenario_time( get_scenario_time() ); // Immediate
         ExCO->set_next_mode_cte_time( get_cte_time() );           // Immediate
         break;
      }

      case EXECUTION_CONTROL_INITIALIZING: {

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_INITIALIZING;
         ExCO->set_next_execution_mode( EXECUTION_MODE_INITIALIZING );

         // Set the next mode times.
         ExCO->set_scenario_time_epoch( get_scenario_time() );     // Now.
         this->next_mode_scenario_time = get_scenario_time();      // Immediate
         ExCO->set_next_mode_scenario_time( get_scenario_time() ); // Immediate
         ExCO->set_next_mode_cte_time( get_cte_time() );           // Immediate
         break;
      }

      case EXECUTION_CONTROL_RUNNING: {

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_RUNNING;
         ExCO->set_next_execution_mode( EXECUTION_MODE_RUNNING );

         // Set the next mode times.
         this->next_mode_scenario_time = get_scenario_time();                // Immediate
         ExCO->set_next_mode_scenario_time( this->next_mode_scenario_time ); // immediate
         ExCO->set_next_mode_cte_time( get_cte_time() );
         if ( ExCO->get_next_mode_cte_time() > -std::numeric_limits< double >::max() ) {
            ExCO->set_next_mode_cte_time( ExCO->get_next_mode_cte_time() + get_time_padding() ); // Some time in the future.
         }
         break;
      }

      case EXECUTION_CONTROL_FREEZE: {

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_FREEZE;
         ExCO->set_next_execution_mode( EXECUTION_MODE_FREEZE );

         // Compute the sim-time from the current scenario time since the
         // scenario timeline is common to all federates.
         double const sim_time = scenario_timeline->compute_simulation_time( get_scenario_time() );

         // The next mode sim-time needs to be on an LCTS boundary.
         int64_t next_mode_sim_base_time = Int64BaseTime::to_base_time( sim_time + get_time_padding() );
         if ( next_mode_sim_base_time % this->least_common_time_step != 0 ) {
            next_mode_sim_base_time = this->least_common_time_step
                                      * ( ( next_mode_sim_base_time / this->least_common_time_step ) + 1 );
         }

         // Convert back to the scenario time.
         this->next_mode_scenario_time = scenario_timeline->time_from_simulation_time(
            Int64BaseTime::to_seconds( next_mode_sim_base_time ) );

         // Set the next mode times.
         ExCO->set_next_mode_scenario_time( this->next_mode_scenario_time );
         ExCO->set_next_mode_cte_time( get_cte_time() );
         if ( ExCO->get_next_mode_cte_time() > -std::numeric_limits< double >::max() ) {
            // Use the same delta time used for the next mode scenario time
            // that is an integer multiple of the LCTS.
            double const delta_time = this->next_mode_scenario_time - get_scenario_time();
            ExCO->set_next_mode_cte_time( ExCO->get_next_mode_cte_time() + delta_time );
         }

         // Set the ExecutionControl freeze times.
         this->scenario_freeze_time   = this->next_mode_scenario_time;
         this->simulation_freeze_time = scenario_timeline->compute_simulation_time( this->next_mode_scenario_time );

         if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            ostringstream errmsg;
            errmsg << "SpaceFOM::ExecutionControl::set_next_execution_control_mode():" << __LINE__ << '\n'
                   << "      Requested-Mode: EXECUTION_CONTROL_FREEZE\n"
                   << "       Scenario-time: " << setprecision( 18 ) << get_scenario_time()
                   << " seconds (" << Int64BaseTime::to_base_time( get_scenario_time() )
                   << " " << Int64BaseTime::get_units() << ")\n"
                   << "        Time-padding: " << setprecision( 18 ) << get_time_padding()
                   << " seconds (" << Int64BaseTime::to_base_time( get_time_padding() )
                   << " " << Int64BaseTime::get_units() << ")\n"
                   << "                LCTS: " << setprecision( 18 )
                   << Int64BaseTime::to_seconds( this->least_common_time_step )
                   << " seconds (" << this->least_common_time_step
                   << " " << Int64BaseTime::get_units() << ")\n"
                   << "Scenario-Freeze-Time: " << setprecision( 18 ) << this->scenario_freeze_time
                   << " seconds (" << Int64BaseTime::to_base_time( this->scenario_freeze_time )
                   << " " << Int64BaseTime::get_units() << ")\n"
                   << "     Sim-Freeze-Time: " << setprecision( 18 ) << this->simulation_freeze_time
                   << " seconds (" << Int64BaseTime::to_base_time( this->simulation_freeze_time )
                   << " " << Int64BaseTime::get_units() << ")\n";
            message_publish( MSG_NORMAL, errmsg.str().c_str() );
         }
         break;
      }

      case EXECUTION_CONTROL_SHUTDOWN: {

         // Set the next execution mode.
         this->requested_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
         ExCO->set_next_execution_mode( EXECUTION_MODE_SHUTDOWN );

         // Set the next mode times.
         this->next_mode_scenario_time = get_scenario_time();                // Immediate.
         ExCO->set_next_mode_scenario_time( this->next_mode_scenario_time ); // Immediate.
         ExCO->set_next_mode_cte_time( get_cte_time() );                     // Immediate
         break;
      }

      default: {
         this->requested_execution_control_mode = EXECUTION_CONTROL_UNINITIALIZED;
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            ostringstream errmsg;
            errmsg << "SpaceFOM::ExecutionControl::set_next_execution_control_mode():" << __LINE__
                   << " WARNING: Unknown execution mode value: " << exec_control
                   << '\n';
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
      errmsg << "SpaceFOM::ExecutionControl::check_mode_transition_request():" << __LINE__
             << " WARNING: Received Mode Transition Request and not Master: "
             << mtr_enum_to_string( this->pending_mtr )
             << '\n';
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
      return false;
   }

   // First check to see if this is a valid MTR.
   if ( !is_mtr_valid( this->pending_mtr ) ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::check_mode_transition_request():" << __LINE__
             << " WARNING: Invalid Mode Transition Request: "
             << mtr_enum_to_string( this->pending_mtr ) << '\n';
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
      return false;
   }

   return true;
}

bool ExecutionControl::process_mode_interaction()
{
   return ( process_mode_transition_request() );
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

   // Reference the SpaceFOM Execution Configuration Object (ExCO)
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // Print diagnostic message if appropriate.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg;
      msg << "=============================================================\n"
          << "SpaceFOM::ExecutionControl::process_mode_transition_request()\n"
          << "\t current_scenario_time:     " << setprecision( 18 ) << scenario_timeline->get_time() << '\n'
          << "\t scenario_time_epoch:       " << setprecision( 18 ) << scenario_timeline->get_epoch() << '\n'
          << "\t scenario_time_epoch(ExCO): " << setprecision( 18 ) << ExCO->scenario_time_epoch << '\n'
          << "\t scenario_time_sim_offset:  " << setprecision( 18 ) << scenario_timeline->get_sim_offset() << '\n'
          << "\t Current HLA grant time:    " << setprecision( 18 ) << federate->get_granted_time().get_time_in_seconds() << '\n'
          << "\t Current HLA request time:  " << setprecision( 18 ) << federate->get_requested_time().get_time_in_seconds() << '\n'
          << "\t current_sim_time:          " << setprecision( 18 ) << sim_timeline->get_time() << '\n'
          << "\t simulation_time_epoch:     " << setprecision( 18 ) << sim_timeline->get_epoch() << '\n';
      if ( does_cte_timeline_exist() ) {
         msg << "\t current_CTE_time:          " << setprecision( 18 ) << cte_timeline->get_time() << '\n'
             << "\t CTE_time_epoch:            " << setprecision( 18 ) << cte_timeline->get_epoch() << '\n';
      }
      msg << "\t next_mode_scenario_time:   " << setprecision( 18 ) << ExCO->next_mode_scenario_time << '\n'
          << "\t next_mode_cte_time:        " << setprecision( 18 ) << ExCO->next_mode_cte_time << '\n'
          << "\t scenario_freeze_time:      " << setprecision( 18 ) << this->scenario_freeze_time << '\n'
          << "\t simulation_freeze_time:    " << setprecision( 18 ) << this->simulation_freeze_time << '\n'
          << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Check Mode Transition Request.
   switch ( this->pending_mtr ) {

      case MTR_GOTO_RUN: {

         // Clear the mode change request flag.
         clear_mode_transition_requested();

         // Transition to run can only happen from initialization or freeze.
         // We don't really need to do anything if we're in initialization
         // because Trick will automatically transitionton to run.
         if ( this->current_execution_control_mode == EXECUTION_CONTROL_FREEZE ) {

            // Tell Trick to exit freeze and go to run.
            the_exec->run();

            // The run transition logic will be triggered when exiting Freeze.
            // This is done in the TrickHLA::Federate::exit_freeze() routine
            // called when exiting Freeze.
         }
         return true;
      }
      case MTR_GOTO_FREEZE: {

         // Clear the mode change request flag.
         clear_mode_transition_requested();

         // Transition to freeze can only happen from initialization or run.
         // We don't really need to do anything if we're in initialization.
         if ( this->current_execution_control_mode == EXECUTION_CONTROL_RUNNING ) {

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
      case MTR_GOTO_SHUTDOWN: {

         // Announce the shutdown.
         shutdown_mode_announce();

         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::process_mode_transition_request():%d MTR_GOTO_SHUTDOWN \n",
                             __LINE__ );
         }

         // Tell Trick to shutdown sometime in the future.
         // The SpaceFOM ExecutionControl shutdown transition will be made from
         // the TrickHLA::Federate::shutdown() job.
         the_exec->stop( the_exec->get_sim_time() + get_time_padding() );
         return true;
      }
      default: {
         // Nothing to do.
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
   bool mode_change = false;

   // Reference the SpaceFOM Execution Configuration Object (ExCO)
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
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
             << " WARNING: Master Federate received an unexpected ExCO update: "
             << execution_control_enum_to_string( this->requested_execution_control_mode )
             << '\n';
      message_publish( MSG_NORMAL, errmsg.str().c_str() );

      // Return that no mode changes occurred.
      return false;
   }

   // If the LCTS is enabled and we have a valid ExCO value then update
   // the ExecutionControl value if it is not initialized. Otherwise
   // ensure they match.
   if ( is_enabled_least_common_time_step()
        && ( ExCO->get_least_common_time_step() > 0 ) ) {

      // Set the Least Common Time Step (LCTS) in ExecutionControl if it has
      // not been initialized and the ExCO has a valid value. This allows a
      // non-Master federate to initialize LCTS from the reflected ExCO value.
      if ( this->least_common_time_step <= 0 ) {

         // This will sync the LCTS value in the ExCO and ExecutionControl.
         this->least_common_time_step_seconds = Int64BaseTime::to_seconds( ExCO->get_least_common_time_step() );
         this->least_common_time_step         = ExCO->get_least_common_time_step();
      } else {

         // The LCTS must match in both this ExecutionControl and the
         // associated ExCO.
         if ( this->least_common_time_step != ExCO->get_least_common_time_step() ) {
            ostringstream errmsg;
            errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
                   << " ERROR: Least-Common-Time-Step (LCTS) time ("
                   << this->least_common_time_step << " " << Int64BaseTime::get_units()
                   << ") is not equal to ExCO LCTS ("
                   << ExCO->get_least_common_time_step() << " " << Int64BaseTime::get_units()
                   << ")!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      }
   }

   // Translate the native ExCO mode values into ExecutionModeEnum.
   ExecutionModeEnum exco_cem              = execution_mode_int16_to_enum( ExCO->current_execution_mode );
   ExecutionModeEnum exco_nem              = execution_mode_int16_to_enum( ExCO->next_execution_mode );
   ExecutionModeEnum current_exection_mode = from_execution_control_enum( this->current_execution_control_mode );

   // Check for consistency between ExecutionControl and ExCO ExecutionMode.
   if ( exco_cem != current_exection_mode ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
             << " WARNING: Current execution mode mismatch between ExecutionControl ("
             << execution_control_enum_to_string( this->current_execution_control_mode )
             << ") and the ExCO current execution mode ("
             << execution_mode_enum_to_string( exco_cem )
             << ") with ExCO next execution mode being ("
             << execution_mode_enum_to_string( exco_nem )
             << ")!\n";
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
   }

   // Check for change in execution mode.
   if ( exco_nem != exco_cem ) {
      mode_change = true;

      switch ( exco_nem ) {
         case EXECUTION_MODE_SHUTDOWN: {
            this->requested_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
            break;
         }
         case EXECUTION_MODE_RUNNING: {
            this->requested_execution_control_mode = EXECUTION_CONTROL_RUNNING;
            break;
         }
         case EXECUTION_MODE_FREEZE: {
            this->requested_execution_control_mode = EXECUTION_CONTROL_FREEZE;
            this->scenario_freeze_time             = ExCO->next_mode_scenario_time;
            this->simulation_freeze_time           = scenario_timeline->compute_simulation_time( this->scenario_freeze_time );
            break;
         }
         default: {
            ostringstream errmsg;
            errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
                   << " WARNING: Invalid ExCO next execution mode: "
                   << execution_mode_enum_to_string( exco_nem ) << "!\n";
            message_publish( MSG_NORMAL, errmsg.str().c_str() );

            // Return that no mode changes occurred.
            return false;
         }
      }
   }

   // Enforce CTE mode time update.
   this->next_mode_cte_time = ExCO->next_mode_cte_time;

   // Check for mode changes.
   if ( !mode_change ) {
      // Return that no mode changes occurred.
      return false;
   }

   // Process the mode change.
   switch ( this->current_execution_control_mode ) {

      case EXECUTION_CONTROL_UNINITIALIZED: {

         switch ( this->requested_execution_control_mode ) {
            case EXECUTION_CONTROL_SHUTDOWN: { // Check for SHUTDOWN.
               // Mark the current execution mode as SHUTDOWN.
               this->current_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
               ExCO->current_execution_mode         = EXECUTION_MODE_SHUTDOWN;

               if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::process_execution_control_updates():%d EXECUTION_CONTROL_SHUTDOWN \n",
                                   __LINE__ );
               }

               // Tell the TrickHLA::Federate to shutdown.
               // The SpaceFOM ExecutionControl shutdown transition will be made from
               // the TrickHLA::Federate::shutdown() job.
               the_exec->stop();
               break;
            }
            default: {
               ostringstream errmsg;
               errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
                      << " WARNING: Execution mode mismatch between current mode ("
                      << execution_control_enum_to_string( this->current_execution_control_mode )
                      << ") and the requested execution mode ("
                      << execution_control_enum_to_string( this->requested_execution_control_mode )
                      << ")!\n";
               message_publish( MSG_NORMAL, errmsg.str().c_str() );

               // Return that no mode changes occurred.
               return false;
            }
         }
         // Return that a mode change occurred.
         return true;
      }
      case EXECUTION_CONTROL_INITIALIZING: {

         switch ( this->requested_execution_control_mode ) {
            case EXECUTION_CONTROL_SHUTDOWN: { // Check for SHUTDOWN.
               // Mark the current execution mode as SHUTDOWN.
               this->current_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
               ExCO->current_execution_mode         = EXECUTION_MODE_SHUTDOWN;

               if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::process_execution_control_updates():%d EXECUTION_CONTROL_SHUTDOWN \n",
                                   __LINE__ );
               }

               // Tell the TrickHLA::Federate to shutdown.
               // The SpaceFOM ExecutionControl shutdown transition will be made from
               // the TrickHLA::Federate::shutdown() job.
               the_exec->stop();
               break;
            }
            case EXECUTION_CONTROL_RUNNING: {
               // Tell Trick to go to in Run at startup.
               the_exec->set_freeze_command( false );

               // This is an early joining federate in initialization.
               // So, proceed to the run mode transition.
               run_mode_transition();
               break;
            }
            case EXECUTION_CONTROL_FREEZE: {
               // Announce the pending freeze.
               freeze_mode_announce();

               // Tell Trick to go into freeze at startup.
               // the_exec->freeze();

               // Tell Trick to go into freeze at startup.
               the_exec->set_freeze_command( true );

               // The freeze transition logic will be done just before entering
               // Freeze. This is done in the TrickHLA::Federate::freeze_init()
               // routine called when entering Freeze.
               break;
            }
            case EXECUTION_CONTROL_INITIALIZING: {
               // There's really nothing to do here.
               break;
            }
            default: {
               ostringstream errmsg;
               errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
                      << " WARNING: Execution mode mismatch between current mode ("
                      << execution_control_enum_to_string( this->current_execution_control_mode )
                      << ") and the requested execution mode ("
                      << execution_control_enum_to_string( this->requested_execution_control_mode )
                      << ")!\n";
               message_publish( MSG_NORMAL, errmsg.str().c_str() );

               // Return that no mode changes occurred.
               return false;
            }
         }
         // Return that a mode change occurred.
         return true;
      }
      case EXECUTION_CONTROL_RUNNING: {

         switch ( this->requested_execution_control_mode ) {
            case EXECUTION_CONTROL_SHUTDOWN: { // Check for SHUTDOWN.
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  // Print out a diagnostic warning message.
                  ostringstream errmsg;
                  errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
                         << " WARNING: Execution mode mismatch between current mode ("
                         << execution_control_enum_to_string( this->current_execution_control_mode )
                         << ") and the requested execution mode ("
                         << execution_control_enum_to_string( this->requested_execution_control_mode )
                         << ")!\n";
                  message_publish( MSG_NORMAL, errmsg.str().c_str() );
               }

               // Mark the current execution mode as SHUTDOWN.
               this->current_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
               ExCO->current_execution_mode         = EXECUTION_MODE_SHUTDOWN;

               if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::process_execution_control_updates():%d EXECUTION_CONTROL_SHUTDOWN \n",
                                   __LINE__ );
               }

               // Tell the TrickHLA::Federate to shutdown.
               // The SpaceFOM ExecutionControl shutdown transition will be made from
               // the TrickHLA::Federate::shutdown() job.
               the_exec->stop();
               break;
            }
            case EXECUTION_CONTROL_FREEZE: {
               // Print diagnostic message if appropriate.
               if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  ostringstream msg;
                  msg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__ << '\n'
                      << "\t current_scenario_time:     " << setprecision( 18 ) << scenario_timeline->get_time() << '\n'
                      << "\t scenario_time_epoch:       " << setprecision( 18 ) << scenario_timeline->get_epoch() << '\n'
                      << "\t scenario_time_epoch(ExCO): " << setprecision( 18 ) << ExCO->scenario_time_epoch << '\n'
                      << "\t scenario_time_sim_offset:  " << setprecision( 18 ) << scenario_timeline->get_sim_offset() << '\n'
                      << "\t current_sim_time:          " << setprecision( 18 ) << sim_timeline->get_time() << '\n'
                      << "\t simulation_time_epoch:     " << setprecision( 18 ) << sim_timeline->get_epoch() << '\n';
                  if ( does_cte_timeline_exist() ) {
                     msg << "\t current_CTE_time:          " << setprecision( 18 ) << cte_timeline->get_time() << '\n'
                         << "\t CTE_time_epoch:            " << setprecision( 18 ) << cte_timeline->get_epoch() << '\n';
                  }
                  msg << "\t next_mode_scenario_time:   " << setprecision( 18 ) << ExCO->next_mode_scenario_time << '\n'
                      << "\t next_mode_cte_time:        " << setprecision( 18 ) << ExCO->next_mode_cte_time << '\n'
                      << "\t scenario_freeze_time:      " << setprecision( 18 ) << this->scenario_freeze_time << '\n'
                      << "\t simulation_freeze_time:    " << setprecision( 18 ) << this->simulation_freeze_time << '\n'
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
               break;
            }
            default: {
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  ostringstream errmsg;
                  errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
                         << " WARNING: Execution mode mismatch between current mode ("
                         << execution_control_enum_to_string( this->current_execution_control_mode )
                         << ") and the requested execution mode ("
                         << execution_control_enum_to_string( this->requested_execution_control_mode )
                         << ")!\n";
                  message_publish( MSG_NORMAL, errmsg.str().c_str() );
               }
               // Return that no mode changes occurred.
               return false;
            }
         }
         // Return that a mode change occurred.
         return true;
      }
      case EXECUTION_CONTROL_FREEZE: {

         switch ( this->requested_execution_control_mode ) {
            case EXECUTION_CONTROL_SHUTDOWN: { // Check for SHUTDOWN.
               // Mark the current execution mode as SHUTDOWN.
               this->current_execution_control_mode = EXECUTION_CONTROL_SHUTDOWN;
               ExCO->current_execution_mode         = EXECUTION_MODE_SHUTDOWN;

               if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::process_execution_control_updates():%d EXECUTION_CONTROL_SHUTDOWN \n",
                                   __LINE__ );
               }

               // Shutdown the federate now.
               exec_get_exec_cpp()->stop();
               break;
            }
            case EXECUTION_CONTROL_RUNNING: {

               // Tell Trick to exit freeze and go to run.
               the_exec->run();

               // The run transition logic will be done just when exiting
               // Freeze. This is done in the TrickHLA::Federate::exit_freeze()
               // routine called when entering Freeze, which will then call
               // run_mode_transition();
               break;
            }
            default: {
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
                  ostringstream errmsg;
                  errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
                         << " WARNING: Execution mode mismatch between current mode ("
                         << execution_control_enum_to_string( this->current_execution_control_mode )
                         << ") and the requested execution mode ("
                         << execution_control_enum_to_string( this->requested_execution_control_mode )
                         << ")!\n";
                  message_publish( MSG_NORMAL, errmsg.str().c_str() );
               }
               // Return that no mode changes occurred.
               return false;
            }
         }
         // Return that a mode change occurred.
         return true;
      }
      case EXECUTION_CONTROL_SHUTDOWN: {

         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            // Once in SHUTDOWN, we cannot do anything else.
            ostringstream errmsg;
            errmsg << "SpaceFOM::ExecutionControl::process_execution_control_updates():" << __LINE__
                   << " WARNING: Shutting down but received mode transition: "
                   << execution_control_enum_to_string( this->requested_execution_control_mode )
                   << '\n';
            message_publish( MSG_NORMAL, errmsg.str().c_str() );
         }
         // Return that no mode changes occurred.
         return false;
      }
      default: {
         // Nothing to do.
         break;
      }
   }

   // Return that no mode changes occurred.
   return false;
}

/*!
 * @details Calling this function will block until the all early joiner
 * federates have discovered the root reference frame and achieved the
 * root_frame_discovered synchronization point.
 * @job_class{initialization}
 */
void ExecutionControl::wait_for_root_frame_discovered_synchronization()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::wait_for_root_frame_discovered_synchronization():%d\n",
                       __LINE__ );
   }

   achieve_sync_point_and_wait_for_synchronization( SpaceFOM::ROOT_FRAME_DISCOVERED_SYNC_POINT );
}

void ExecutionControl::send_mode_transition_interaction(
   ModeTransitionEnum requested_mode )
{
   send_MTR_interaction( from_mode_transition_enum( requested_mode ) );
}

/*!
 * @job_class{scheduled}
 */
void ExecutionControl::send_MTR_interaction(
   SpaceFOM::MTREnum requested_mode )
{
   mtr_interaction_handler.send_interaction( requested_mode );
}

bool ExecutionControl::run_mode_transition()
{
   // Register the 'mtr_run' sync-point.
   if ( is_master() ) {
      register_sync_point( SpaceFOM::MTR_RUN_SYNC_POINT );
   }

   // Make sure that we have a valid sync-point.
   if ( !contains_sync_point( SpaceFOM::MTR_RUN_SYNC_POINT ) ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::run_mode_transition():" << __LINE__
             << " ERROR: The 'mtr_run' sync-point was not found!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Wait for 'mtr_run' sync-point announce.
   wait_for_sync_point_announced( SpaceFOM::MTR_RUN_SYNC_POINT );

   // Achieve the 'mtr-run' sync-point and wait for synchronization.
   achieve_sync_point_and_wait_for_synchronization( SpaceFOM::MTR_RUN_SYNC_POINT );

   // Set the current execution mode to running.
   ExecutionConfiguration *ExCO         = get_execution_configuration();
   this->current_execution_control_mode = EXECUTION_CONTROL_RUNNING;
   ExCO->set_current_execution_mode( EXECUTION_MODE_RUNNING );

   // Check for CTE.
   if ( does_cte_timeline_exist() ) {

      double go_to_run_time;

      // The Master federate updates the ExCO with the CTE got-to-run time.
      if ( is_master() ) {

         go_to_run_time = ExCO->get_next_mode_cte_time();
         ExCO->send_init_data();

      } else {
         // Other federates wait on the ExCO update with the CTE go-to-run time.

         // Wait for the ExCO update with the CTE time.
         ExCO->wait_for_update();

         // Process the just received ExCO update.
         process_execution_control_updates();

         // Set the CTE time to go to run.
         go_to_run_time = ExCO->get_next_mode_cte_time();
      }

      double cte_time, cte_time_diff;

      // Wait for the CTE go-to-run time.
      cte_time = get_cte_time();
      while ( cte_time < go_to_run_time ) {

         // Always check for shutdown in wait loops.
         federate->check_for_shutdown_with_termination();

         cte_time_diff = go_to_run_time - cte_time;
         if ( fmod( cte_time_diff, 1.0 ) == 0.0 ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
               ostringstream msg;
               msg << "SpaceFOM::ExecutionControl::run_mode_transition():" << __LINE__
                   << " Going to run in " << cte_time_diff << " seconds.\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
            }
         }

         cte_time = get_cte_time();
      }

      cte_time_diff = cte_time - go_to_run_time;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         ostringstream msg;
         msg << "SpaceFOM::ExecutionControl::run_mode_transition():" << __LINE__ << "\n"
             << "Go to RUN at CTE time: " << setprecision( 18 ) << go_to_run_time << " seconds\n"
             << "     Current CTE time: " << setprecision( 18 ) << cte_time << " seconds\n"
             << "           Difference: " << setprecision( 9 ) << cte_time_diff << " seconds\n";
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      // Always show a warning message if the manager does not have a big
      // enough time padding specified by the user when using CTE.
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         if ( cte_time_diff >= 0.1 ) {
            if ( is_master() ) {
               ostringstream msg;
               msg << "SpaceFOM::ExecutionControl::run_mode_transition():" << __LINE__
                   << " WARNING: Current CTE time exceeded the go-to-run time by"
                   << " more than 0.1 seconds. Please add more time to the"
                   << " time padding configured in your input.py file, which is"
                   << " currently set to 'federate.set_time_padding( "
                   << setprecision( 9 ) << get_time_padding()
                   << " )', to allow the go-to-run CTE message to propagate to"
                   << " all federates in time to be used.\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
            } else {
               ostringstream msg;
               msg << "SpaceFOM::ExecutionControl::run_mode_transition():" << __LINE__
                   << " WARNING: Current CTE time exceeded the go-to-run time by"
                   << " more than 0.1 seconds. Please update the Master federate"
                   << " to add more time to the time padding configured in the"
                   << " input.py file for this call 'federate.set_time_padding( pad )',"
                   << " to allow the go-to-run CTE message to propagate to"
                   << " all federates in time to be used.\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
            }
         }
      }
   }

   return true;
}

void ExecutionControl::freeze_mode_announce()
{
   // Register the 'mtr_freeze' sync-point.
   if ( is_master() ) {
      register_sync_point( SpaceFOM::MTR_FREEZE_SYNC_POINT );
   }
}

bool ExecutionControl::freeze_mode_transition()
{
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
      ExecutionConfiguration *ExCO         = get_execution_configuration();
      this->current_execution_control_mode = EXECUTION_CONTROL_FREEZE;
      ExCO->set_current_execution_mode( EXECUTION_MODE_FREEZE );
   }
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
   execution_configuration->send_init_data();

   // Clear the mode change request flag.
   clear_mode_transition_requested();
}

/*!
 * @job_class{shutdown}
 */
void ExecutionControl::shutdown_mode_transition()
{
   // Only the Master federate has any SpaceFOM tasks for shutdown.
   if ( !is_master() ) {
      return;
   }

   // If the current execution mode is uninitialized then we haven't gotten
   // far enough in the initialization process to shut anything down.
   if ( this->current_execution_control_mode == EXECUTION_CONTROL_UNINITIALIZED ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::shutdown_mode_transition():%d Registered 'mtr_shutdown' synchronization point.\n",
                       __LINE__ );
   }
   // Register the 'mtr_shutdown' sync-point.
   register_sync_point( SpaceFOM::MTR_SHUTDOWN_SYNC_POINT );

   // Wait for the 'mtr_shutdown' announcement to make sure it goes through
   // before we shutdown.
   wait_for_sync_point_announced( SpaceFOM::MTR_SHUTDOWN_SYNC_POINT );
}

/*!
 * @job_class{shutdown}
 */
bool ExecutionControl::check_for_shutdown()
{
   if ( DebugHandler::show( DEBUG_LEVEL_FULL_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::check_for_shutdown():%d Checking for shutdown \n",
                       __LINE__ );
   }

   // Check to see if the mtr_shutdown sync-point has been announced or if the
   // ExCO object has been deleted as indicators to shutdown.
   return ( is_sync_point_announced( SpaceFOM::MTR_SHUTDOWN_SYNC_POINT )
            || execution_configuration->object_deleted_from_RTI );
}

/*!
 * @details NOTE: If a shutdown has been announced, this routine calls the
 * Trick exec_teminate() function. So, for shutdown, it should never return.
 * @job_class{shutdown}
 */
bool ExecutionControl::check_for_shutdown_with_termination()
{
   if ( DebugHandler::show( DEBUG_LEVEL_FULL_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::check_for_shutdown_with_termination():%d Checking for shutdown.\n",
                       __LINE__ );
   }

   // Check to see if the mtr_shutdown sync-point has been announced.
   // If so, it's time to say good bye.
   if ( check_for_shutdown() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::check_for_shutdown_with_termination():" << __LINE__
             << " WARNING: This Federate '" << federate->get_federate_name()
             << "' detected a";
      if ( is_sync_point_announced( SpaceFOM::MTR_SHUTDOWN_SYNC_POINT ) ) {
         errmsg << " Shutdown sync-point 'mtr_shutdown',";
      }
      if ( execution_configuration->object_deleted_from_RTI ) {
         errmsg << " Execution Configuration Object (ExCO) deleted from the RTI";
      }
      errmsg << " for the '" << federate->get_federation_name()
             << "' Federation.\n";

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }

      // Tell the federate to shutdown.
      federate->shutdown();

      // Wait a little while for the Federate HLA interface to shutdown before
      // we terminate.
      Utilities::micro_sleep( 500000 );
      DebugHandler::terminate_with_message( errmsg.str() );

      return true;
   }
   return false;
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Currently only used with SpaceFOM initialization schemes.
 *  @job_class{freeze_init}
 */
void ExecutionControl::freeze_init()
{
   // Mark the freeze as announced.
   set_freeze_announced( true );

   // Transition to freeze. However, we need to check for special case
   // where this is a late joining federate in initialization. For that
   // one case, do NOT use the SpaceFOM::ExecutionControlBase::freeze_mode_transition()
   // routine. Just proceed to freeze.
   if ( !is_late_joiner() || ( get_current_execution_control_mode() != EXECUTION_CONTROL_INITIALIZING ) ) {
      // Tell Execution Control to transition to Freeze.
      freeze_mode_transition();
   }

   // Make sure that the current execution mode is set to Freeze.
   set_current_execution_control_mode( EXECUTION_CONTROL_FREEZE );
}

void ExecutionControl::enter_freeze()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      string exec_cmd_str;
      switch ( exec_get_exec_command() ) {
         case NoCmd: {
            exec_cmd_str = "NoCmd";
            break;
         }
         case FreezeCmd: {
            exec_cmd_str = "FreezeCmd";
            break;
         }
         case RunCmd: {
            exec_cmd_str = "RunCmd";
            break;
         }
         case ExitCmd: {
            exec_cmd_str = "ExitCmd";
            break;
         }
         default: {
            exec_cmd_str = "Unknown";
            break;
         }
      }
      ostringstream msg;
      msg << "SpaceFOM::ExecutionControl::enter_freeze():" << __LINE__ << '\n'
          << "   Master-federate:" << ( is_master() ? "Yes" : "No" )
          << '\n'
          << "   Requested-ExCO-mode:" << execution_mode_enum_to_string( from_execution_control_enum( get_requested_execution_control_mode() ) )
          << '\n'
          << "   Trick-sim-time:" << setprecision( 18 ) << exec_get_sim_time()
          << "   Trick-exec-command:" << exec_cmd_str
          << '\n'
          << "   Sim-freeze-time:" << setprecision( 18 ) << get_simulation_freeze_time()
          << "   Freeze-announced:" << ( is_freeze_announced() ? "Yes" : "No" )
          << "   Freeze-pending:" << ( is_freeze_pending() ? "Yes" : "No" )
          << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Determine if we are already processing a freeze command.
   if ( get_requested_execution_control_mode() == EXECUTION_CONTROL_FREEZE ) {

      // Determine if the Trick control panel "freeze" button was pressed more
      // than once if the simulation time is less than the current scheduled
      // freeze time for a FreezeCmd.
      if ( ( exec_get_exec_command() == FreezeCmd )
           && ( exec_get_sim_time() < get_simulation_freeze_time() ) ) {

         // The Trick simulation control panel "freeze" button was hit more than
         // once before the scheduled freeze time, so go back to run.
         exec_run();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::enter_freeze():%d Detected duplicate Freeze command, ignoring.\n",
                             __LINE__ );
         }
      }

      // The requested mode is already EXECUTION_CONTROL_FREEZE so return. This
      // can happen if the scheduled freeze is activated and this end of frame
      // function is called again instead of the Trick Sim Control Panel
      // Freeze button being pressed.
      return;
   }

   // Check for freeze command from Simulation control panel.
   if ( exec_get_exec_command() == FreezeCmd ) {

      // Only the Master federate can command freeze.
      if ( !is_master() ) {

         // Since only the Master federate can command freeze, override
         // the current Trick freeze command.
         // NOTE: This will prevent the SimControl panel freeze button
         // from working.
         // Uncomment the following line if you really want this behavior.
         // federate->un_freeze();

         return;
      }

      // Okay, we can't go directly into freeze. First we need to
      // tell the other federates to go to freeze at some time in the
      // future. Then we can go to freeze at that time.

      // Set the next execution mode to freeze.
      set_next_execution_control_mode( EXECUTION_CONTROL_FREEZE );

      // Send the ExCO with the updated run mode.
      get_execution_configuration()->send_init_data();

      // Tell Execution Control to announce Freeze.
      freeze_mode_announce();

      // Tell Trick to go into freeze at the appointed time.
      federate->un_freeze();
      the_exec->freeze( get_simulation_freeze_time() );

      // NOTE: The actual freeze transition will be done in the
      // Federate::freeze_init() job.
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::enter_freeze():%d Freeze Announced:%s, Freeze Pending:%s\n",
                       __LINE__, ( is_freeze_announced() ? "Yes" : "No" ),
                       ( is_freeze_pending() ? "Yes" : "No" ) );
   }
}

bool ExecutionControl::check_freeze_exit()
{
   // If freeze has not been announced, then return false.
   if ( !is_freeze_announced() ) {
      return ( false );
   }

   // Check if this is a Master federate.
   if ( is_master() ) {

      // Process and Mode Transition Requests.
      process_mode_transition_request();

      // Handle requests for ExCO updates.
      if ( execution_configuration->is_attribute_update_requested() ) {
         execution_configuration->send_requested_data();
      }

      // Check for Trick shutdown command.
      if ( the_exec->get_exec_command() == ExitCmd ) {

         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::check_freeze_exit():%d Trick shutdown commanded.\n",
                             __LINE__ );
         }

         // Tell the TrickHLA::Federate to shutdown.
         // The SpaceFOM ExecutionControl shutdown transition will be made from
         // the TrickHLA::Federate::shutdown() job.
         federate->shutdown();
      }

   } else {

      // Check to see if there was an ExCO update.
      execution_configuration->receive_init_data();

      // Process the ExCO update.
      process_execution_control_updates();

      // Check for shutdown.
      if ( this->current_execution_control_mode == EXECUTION_CONTROL_SHUTDOWN ) {
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
            message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::check_freeze_exit():%d Execution Control Shutdown commanded.\n",
                             __LINE__ );
         }

         // Tell the TrickHLA::Federate to shutdown.
         // The SpaceFOM execution control shutdown transition will be made from
         // the TrickHLA::Federate::shutdown() job.
         federate->shutdown();
      }
   }

   return ( true );
}

void ExecutionControl::exit_freeze()
{
   // If the Master federate, then send out the updated ExCO.
   if ( is_master() ) {
      // Set the next mode to run.
      set_next_execution_control_mode( EXECUTION_CONTROL_RUNNING );
      // Send out an ExCO update.
      get_execution_configuration()->send_init_data();
   }

   // Transition to run mode.
   run_mode_transition();

#if 0 // TEST ONLY
   // TEMP Test adding a delay to non-master federates to make them late to
   // go to run. This will allow us to verify we calculate the correct clock
   // reset value with the expected behavior being the federates only overrun
   // for a few frames until they catch back up to realtime.
   if ( !is_master() ) {
      int sleep_time_usec = 4.0 * ( 1000000 * exec_get_software_frame() );
      cout << "========== Software Frame:" << exec_get_software_frame()
           << " seconds, delay time:" << sleep_time_usec << " microseconds\n";
      Utilities::micro_sleep( sleep_time_usec );
   }
#endif

#if THLA_TIME_DEBUG
   print_clock_summary( "ExecutionControl::exit_freeze():" + std::to_string( __LINE__ )
                        + "\n BEFORE CLOCK RESET \n" );
#endif

   // Reset the Trick realtime clock. We need to do this since the exit_freeze
   // job waits an indeterminate amount of time to synchronize the mtr_goto_run
   // mode transition. This is particularly true when using the CTE clock and a
   // large mode transition padding time that controls when we go to run.
   long long ref;
   if ( does_cte_timeline_exist() ) {
      ExecutionConfiguration *ExCO = get_execution_configuration();

      // Account for the difference between the go-to-run CTE time and the
      // current CTE time. We may have been late trying to meet the go-to-run
      // time because there was not enough padding time. Adjust the Trick
      // clock reference to account for this to keep it's clock reference
      // aligned with the other federates.
      ref = the_exec->get_time_tics()
            + (long long)( ( get_cte_time() - ExCO->get_next_mode_cte_time() )
                           * the_exec->get_time_tic_value() );
   } else {
      ref = the_exec->get_time_tics();
   }
   the_clock->clock_reset( ref );

#if THLA_TIME_DEBUG
   print_clock_summary( "ExecutionControl::exit_freeze():" + std::to_string( __LINE__ )
                        + "\n AFTER CLOCK RESET \n"
                        + "clock_reset to: " + std::to_string( ref ) + "\n" );
#endif
}

ExecutionConfiguration *ExecutionControl::get_execution_configuration()
{
   ExecutionConfiguration *ExCO = dynamic_cast< ExecutionConfiguration * >( execution_configuration );
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::get_execution_configuration():" << __LINE__
             << " ERROR: Execution Configuration is not an SpaceFOM ExCO.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return ( ExCO );
}

/*!
@details This routine implements the SpaceFOM Epoch and Root Reference Frame
Discovery initialization process described in section 7.2 and figure 7-6.

@job_class{initialization}
*/
void ExecutionControl::epoch_and_root_frame_discovery_process()
{
   ExecutionConfiguration *ExCO = get_execution_configuration();

   // Proceed in to the Root Reference Frame discovery process.
   // This is discussed in section 7.2.1.2 of the Space Reference FOM
   // document and diagram in figure 7-6.

   // Branch between Master federate and Early Joiner federate behavior.
   if ( is_master() ) {

      // Send the first ExCO with the scenario timeline epoch.
      ExCO->set_scenario_time_epoch( scenario_timeline->get_epoch() );
      ExCO->send_init_data();

   } else {

      // Wait on the ExCO update with the scenario timeline epoch.
      ExCO->wait_for_update();

      // Process the just received ExCO update.
      process_execution_control_updates();

      // Set scenario timeline epoch and time.
      scenario_timeline->set_epoch( ExCO->get_scenario_time_epoch() );
   }

   // If the Root Reference Frame Publisher (RRFP) then send out the
   // root reference frame update. Otherwise wait on the delivery of
   // the root reference frame update.
   if ( is_root_frame_publisher() ) {

      // Set the Root Reference Frame name in the ExCO.
      ExCO->set_root_frame_name( root_ref_frame->get_name() );

      // Send the ExCO update with the root reference frame name.
      send_init_root_ref_frame();

   } else {

      // Wait for the initial root reference frame update.
      receive_init_root_ref_frame();

      // Set the Root Reference Frame name in the ExCO if a
      // root reference frame object exists in this federate.
      if ( root_ref_frame != NULL ) {
         ExCO->set_root_frame_name( root_ref_frame->get_name() );
      }
   }

   // Branch between Master federate and Early Joiner federate behavior.
   if ( is_master() ) {

      // Now send the latest ExCO update with the root reference frame update.
      ExCO->send_init_data();

   } else {

      // Now receive the latest ExCO update with the root reference frame update.
      ExCO->wait_for_update();

      // Process the just received ExCO update.
      process_execution_control_updates();
   }

   // Wait on the root_frame_discovered sync-point
   wait_for_root_frame_discovered_synchronization();
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::send_init_root_ref_frame()
{

   // Late joining federates cannot be root frame publishers so just return.
   if ( manager->is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::send_init_root_ref_frame():%d Late joining \
federate so the data will not be sent for '%s'.\n",
                          __LINE__, execution_configuration->get_name() );
      }
      return;
   }

   send_root_ref_frame();
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::send_root_ref_frame()
{
   TrickHLA::Object *rrf_obj;

   // Only the root reference frame publisher (RRFP) can publish the root frame.
   if ( !is_root_frame_publisher() ) {
      return;
   }

   // Make sure that the root reference frame is set.
   if ( root_ref_frame == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::send_root_ref_frame():" << __LINE__
             << " ERROR: Root Reference Frame is not set!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }
   rrf_obj = root_ref_frame->get_object(); // cppcheck-suppress [nullPointerRedundantCheck,unmatchedSuppression]

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::send_root_ref_frame():%d\n",
                       __LINE__ );
   }

   // Make sure we have at least one piece of ExCO data we can send.
   if ( rrf_obj->any_locally_owned_published_init_attribute() ) {

      // Send the reference frame data to the other federates.
      rrf_obj->send_init_data();

   } else {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::send_root_ref_frame():" << __LINE__
             << " ERROR: Root Reference Frame is not configured to send at"
             << " least one object attribute. Make sure at least one"
             << " 'root_ref_frame' attribute has 'publish = true' set. Please"
             << " check your input or modified-data files to make sure the"
             << " 'publish' value is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::receive_init_root_ref_frame()
{
   // Late joining federates will get root reference frame from ExCO update.
   if ( manager->is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::receive_init_root_ref_frame():%d Late joining federate so skipping data for '%s'\n",
                          __LINE__, root_ref_frame->get_name() );
      }
      return;
   }

   receive_root_ref_frame();
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::receive_root_ref_frame()
{
   // If the root_reference frame is not set, then just return.
   if ( root_ref_frame == NULL ) {
      return;
   }

   // Get the Object associated with the root reference frame.
   TrickHLA::Object *rrf_object = root_ref_frame->get_object();

   // The root reference frame publisher (RRFP) only publishes the root frame.
   if ( is_root_frame_publisher() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "SpaceFOM::ExecutionControl::receive_root_ref_frame():%d Waiting...\n",
                       __LINE__ );
   }

   // Make sure we have at least one piece of root reference frame data we can receive.
   if ( rrf_object->any_remotely_owned_subscribed_init_attribute() ) {

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer;

      // Wait for the data to arrive.
      while ( !rrf_object->is_changed() ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !rrf_object->is_changed() ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "SpaceFOM::ExecutionControl::receive_root_ref_frame():" << __LINE__
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
               message_publish( MSG_NORMAL, "SpaceFOM::ExectionControl::receive_root_ref_frame():%d Waiting...\n",
                                __LINE__ );
            }
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         message_publish( MSG_NORMAL, "SpaceFOM::ExectionControl::receive_root_ref_frame():%d Received data.\n",
                          __LINE__ );
      }

      // Receive the root reference frame data from the RRFP federate.
      rrf_object->receive_init_data();

   } else {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::receive_root_ref_frame():" << __LINE__
             << " ERROR: Execution-Configuration is not configured to receive"
             << " at least one object attribute. Make sure at least one"
             << " 'root_ref_frame' attribute has 'subscribe = true' set. Please"
             << " check your input or modified-data files to make sure the"
             << " 'subscribe' value is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

void ExecutionControl::start_federation_save_at_scenario_time(
   double      freeze_scenario_time,
   char const *file_name )
{
   ostringstream errmsg;
   errmsg << "SpaceFOM::ExecutionControl::start_federation_save_at_scenario_time:" << __LINE__
          << " ERROR: The ExecutionControl does not yet support SAVE/RESTORE!\n";
   DebugHandler::terminate_with_message( errmsg.str() );
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionControl::set_least_common_time_step(
   double const lcts )
{
   // SpaceFOM depends on the LCTS being enabled and used.
   if ( !is_enabled_least_common_time_step() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::set_least_common_time_step():" << __LINE__
             << " ERROR: Least Common Time Step (LCTS) is not enabled!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // WARNING: Only the Master federate should ever set this.
   if ( is_master() ) {

      ExecutionConfiguration *ExCO = dynamic_cast< ExecutionConfiguration * >( execution_configuration );
      if ( ExCO == NULL ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::ExecutionControl::set_least_common_time_step():" << __LINE__
                << " ERROR: Execution Configuration is not an SpaceFOM ExCO.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      // Make sure to set this for both the ExecutionControl and the ExCO.
      this->least_common_time_step_seconds = lcts;
      this->least_common_time_step         = Int64BaseTime::to_base_time( lcts );
      ExCO->set_least_common_time_step( lcts );
   }
}

void ExecutionControl::refresh_least_common_time_step()
{
   // Refresh the LCTS by setting the value again, which will calculate a new
   // LCTS using the HLA base time units.
   set_least_common_time_step( this->least_common_time_step_seconds );
}

void ExecutionControl::set_time_padding(
   double t )
{
   if ( t <= 0.0 ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::set_time_padding():" << __LINE__
             << " ERROR: Time padding value (" << t
             << " seconds) must be greater than zero!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   int64_t padding_base_time = Int64BaseTime::to_base_time( t );

   // At a minimum the Padding time must be >= LCTS.
   if ( padding_base_time < this->least_common_time_step ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::set_time_padding():" << __LINE__
             << " ERROR: Mode transition padding time ("
             << padding_base_time << " " << Int64BaseTime::get_units()
             << ") can not be less than the ExCO Least Common Time Step (LCTS:"
             << this->least_common_time_step << " " << Int64BaseTime::get_units()
             << ")!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Time padding needs to be an integer multiple of the LCTS.
   if ( ( padding_base_time % this->least_common_time_step ) != 0 ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::set_time_padding():" << __LINE__
             << " ERROR: Time padding value ("
             << padding_base_time << " " << Int64BaseTime::get_units()
             << ") must be an integer multiple of the Least Common Time Step (LCTS:"
             << this->least_common_time_step << " " << Int64BaseTime::get_units()
             << ")!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // The Master federate padding time must be 3 or more times the Least
   // Common Time Step (LCTS) or two seconds. This will give commands time to
   // propagate through the system and still have time for mode transitions.
   if ( ( padding_base_time < Int64BaseTime::to_base_time( THLA_PADDING_DEFAULT ) )
        && ( padding_base_time < ( 3 * this->least_common_time_step ) ) ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::ExecutionControl::set_time_padding():" << __LINE__
             << " ERROR: Mode transition padding time ("
             << padding_base_time << " " << Int64BaseTime::get_units()
             << ") is not a multiple of 3 or more of the ExCO"
             << " Least Common Time Step (LCTS:"
             << this->least_common_time_step << " " << Int64BaseTime::get_units()
             << ") when the time padding is less than "
             << THLA_PADDING_DEFAULT << " seconds!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // For a Master federate using CTE, we need to make sure the padding
   // time is at least two times the freeze frame time. Otherwise we can
   // not coordinate a go to run in time.
   if ( is_master() && does_cte_timeline_exist() ) {

      if ( t <= ( 2.0 * exec_get_freeze_frame() ) ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::ExecutionControl::set_time_padding():" << __LINE__
                << " ERROR: Mode transition padding time (" << t
                << " seconds) must be more than two times the Trick freeze"
                << " frame time (" << exec_get_freeze_frame() << " seconds)!"
                << " In your input.py file, please update the padding time"
                << " and/or the Trick freeze frame time using directives"
                << " like the following:\n"
                << "   federate.set_time_padding( pad )\n"
                << "   trick.exec_set_freeze_frame( frame_time )\n"
                << "For example, adjusting the freeze frame time for the"
                << " given time padding:\n";
         if ( t > ( 2.0 * exec_get_software_frame() ) ) {
            // Example using the Trick software frame time to set freeze frame.
            errmsg << "   federate.set_time_padding( " << t << " )\n"
                   << "   trick.exec_set_freeze_frame( " << exec_get_software_frame() << " )\n";
         } else {
            errmsg << "   federate.set_time_padding( " << t << " )\n"
                   << "   trick.exec_set_freeze_frame( " << ( t / 4 ) << " )\n";
         }
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // Set the padding time in seconds.
   this->time_padding = Int64BaseTime::to_seconds( padding_base_time );
}

void ExecutionControl::print_clock_summary(
   string const &message )
{
   long long const clk_time = the_clock->clock_time();
   long long const wc_time  = the_clock->wall_clock_time();
   ostringstream   msg;
   msg << message
       << "      global-clock-name: '" << the_clock->get_name() << "'\n";
   if ( does_cte_timeline_exist() ) {
      msg << "         CTE-clock-name: '" << cte_timeline->get_name() << "'\n"
          << "      CTE-clock-min-res: " << setprecision( 10 ) << cte_timeline->get_min_resolution() << " seconds\n";
   }
   msg << "     exec_get_time_tics: " << exec_get_time_tics() << "\n"
       << "the_exec->get_time_tics: " << the_exec->get_time_tics() << "\n"
       << "             clock_time: " << clk_time << "\n"
       << "        wall_clock_time: " << wc_time << "\n"
       << "          ref_time_tics: " << the_clock->ref_time_tics << "\n"
       << "wall_clock_time - ref_time_tics: " << ( wc_time - the_clock->ref_time_tics ) << "\n"
       << "         rt_clock_ratio: " << the_clock->get_rt_clock_ratio() << "\n"
       << "          sim_tic_ratio: " << the_clock->sim_tic_ratio << "\n"
       << "     clock_tics_per_sec: " << the_clock->clock_tics_per_sec << "\n"
       << "exec_get_time_tic_value: " << exec_get_time_tic_value() << "\n";
   message_publish( MSG_NORMAL, msg.str().c_str() );
}
