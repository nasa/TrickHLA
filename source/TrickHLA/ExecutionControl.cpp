/*!
@file TrickHLA/ExecutionControl.cpp
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
@trick_link_dependency{ExecutionConfiguration.cpp}
@trick_link_dependency{ExecutionControl.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Manager.cpp}
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
#include <math.h>
#include <string>

// Trick includes.
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionConfiguration.hh"
#include "TrickHLA/ExecutionControl.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

// Simple TrickHL::ExecutionControl file level declarations.
namespace TrickHLA
{

// ExecutionControl type string.
std::string const ExecutionControl::type = "Simple";

} // namespace TrickHLA

// Create the Trick ATTRIBUTES arrays needed for local allocations.
#ifdef __cplusplus
extern "C" {
#endif
// C based model includes.
extern ATTRIBUTES attrTrickHLA__MTRInteractionHandler[];
#ifdef __cplusplus
}
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl()
   : TrickHLA::ExecutionControlBase()
{
   return;
}

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl(
   TrickHLA::ExecutionConfiguration &exec_config )
   : TrickHLA::ExecutionControlBase( exec_config )
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
 * @details This routine will set a lot of the data in the TrickHLA::Federate that
 * is required for this execution control scheme. This should greatly simplify
 * input.py files and reduce input.py file setting errors.
 * @job_class{initialization}
 */
void ExecutionControl::initialize()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream msg;
      msg << "TrickHLA::ExecutionControl::initialize():" << __LINE__
          << " Initialization-Scheme:'" << get_type() << "'\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Simple initialization does not support a Master.
   this->use_preset_master = false;

   // Simple initialization does not support known federates.
   federate->enable_known_feds = false;
   federate->known_feds_count  = 0;

   // Make sure we initialize the base class.
   TrickHLA::ExecutionControlBase::initialize();
}

/*!
@details This routine implements the TrickHLA Join Federation Process described.

@job_class{initialization}
*/
void ExecutionControl::join_federation_process()
{
   // The base class implementation is good enough for now.
   TrickHLA::ExecutionControlBase::join_federation_process();
}

/*!
@details This routine implements the TrickHLA pre multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::pre_multi_phase_init_processes()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      message_publish( MSG_NORMAL, "TrickHLA::ExecutionControl::pre_multi_phase_init_processes():%d\n", __LINE__ );
   }

   // Setup all the Trick Ref-Attributes for the user specified objects,
   // attributes, interactions and parameters.
   get_manager()->setup_all_ref_attributes();

   // Create the RTI Ambassador and connect.
   federate->create_RTI_ambassador_and_connect();

   // Destroy the federation if it was orphaned from a previous simulation
   // run that did not shutdown cleanly.
   federate->destroy_orphaned_federation();

   // Create and join the federation.
   federate->create_and_join_federation();

   // We are the master if we successfully created the federation and the
   // user has not preset a master value.
   if ( !is_master_preset() ) {
      set_master( federate->is_federation_created_by_federate() );
   }

   // Don't forget to enable asynchronous delivery of messages.
   federate->enable_async_delivery();

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      if ( is_master() ) {
         message_publish( MSG_NORMAL, "TrickHLA::ExecutionControl::pre_multi_phase_init_processes():%d\n    I AM THE MASTER\n",
                          __LINE__ );
      } else {
         message_publish( MSG_NORMAL, "TrickHLA::ExecutionControl::pre_multi_phase_init_processes():%d\n    I AM NOT THE MASTER\n",
                          __LINE__ );
      }
   }

   // Setup the simulation configuration object now that we know if we are
   // the "Master" federate or not.
   execution_configuration->set_master( is_master() );

   // The Master federate must have a padding time set.
   if ( is_master() && ( get_time_padding() <= 0.0 ) ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
             << " ERROR: For this Master federate, the time padding ("
             << get_time_padding() << " seconds) must be greater than zero!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify the federate time constraints.
   if ( !federate->verify_time_constraints() ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::pre_multi_phase_init_processes():" << __LINE__
             << " ERROR: Time constraints verification failed!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( is_master() ) {
      // Initialize the MOM interface handles.
      federate->initialize_MOM_handles();
   }

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
}

/*!
@details This routine implements the TrickHLA post multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::post_multi_phase_init_processes()
{
   // Make sure we setup time constrained and time regulating with the RTI.
   federate->setup_time_management();

   // Jump to the GALT that is a multiple of the lookahead time.
   federate->time_advance_request_to_GALT();
}

void ExecutionControl::shutdown()
{
   return;
}

/*!
 * @details This routine is used to perform and inline build of the Trick
 * ref ATTRIBUTES.
 *
 * @job_class{initialization}
 */
void ExecutionControl::setup_object_ref_attributes()
{
   return;
}

/*!
 * @details This routine is used to perform and inline build of the Trick
 * ref ATTRIBUTES for any mode transition interactions. This implementation
 * does not have any.
 *
 * @job_class{initialization}
 */
void ExecutionControl::setup_interaction_ref_attributes()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::setup_object_RTI_handles()
{
   ExecutionConfiguration *ExCO = get_execution_configuration();
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::setup_object_RTI_handles():" << __LINE__
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
   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::add_initialization_sync_points()
{

   // Add the initialization synchronization points used for startup regulation.
   // This version of ExecutionControl does not have any.

   // Add the multiphase initialization synchronization points.
   add_multiphase_init_sync_points();
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::add_multiphase_init_sync_points()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::add_multiphase_init_sync_points():" << __LINE__
             << " This call will be ignored because this ExecutionControl does not"
             << " support multiphase initialization synchronization points.\n";
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
   }
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::clear_multiphase_init_sync_points()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::clear_multiphase_init_sync_points():" << __LINE__
             << " This call will be ignored because this ExecutionControl does not"
             << " support multiphase initialization synchronization points.\n";
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
   }
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
 * @job_class{scheduled}
 */
bool ExecutionControl::receive_interaction(
   InteractionClassHandle const  &theInteraction,
   ParameterHandleValueMap const &theParameterValues,
   RTI1516_USERDATA const        &theUserSuppliedTag,
   LogicalTime const             &theTime,
   bool const                     received_as_TSO )
{
   // No execution control specific interaction to process.
   return false;
}

void ExecutionControl::send_mode_transition_interaction(
   ModeTransitionEnum requested_mode )
{
   return;
}

void ExecutionControl::set_next_execution_control_mode(
   TrickHLA::ExecutionControlEnum exec_control )
{
   return;
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Called from the ExCO unpack routine.
 *
 * @job_class{scheduled}
 */
bool ExecutionControl::process_execution_control_updates()
{

   // Return that no mode changes occurred.
   return false;
}

bool ExecutionControl::run_mode_transition()
{

   return true;
}

void ExecutionControl::freeze_mode_announce()
{
   return;
}

bool ExecutionControl::freeze_mode_transition()
{
   return false;
}

void ExecutionControl::shutdown_mode_announce()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
void ExecutionControl::shutdown_mode_transition()
{
   return;
}

ExecutionConfiguration *ExecutionControl::get_execution_configuration()
{
   ExecutionConfiguration *ExCO = dynamic_cast< ExecutionConfiguration * >( execution_configuration );
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::get_execution_configuration():" << __LINE__
             << " ERROR: Execution Configuration is not an TrickHLA ExCO.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return ( ExCO );
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionControl::set_least_common_time_step(
   double const lcts )
{
   // WARNING: Only the Master federate should ever set this.
   if ( is_master() ) {
      ExecutionConfiguration const *ExCO = dynamic_cast< ExecutionConfiguration * >( execution_configuration );
      if ( ExCO == NULL ) {
         ostringstream errmsg;
         errmsg << "TrickHLA::ExecutionControl::set_least_common_time_step():" << __LINE__
                << " ERROR: Execution Configuration is not an TrickHLA ExCO.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      // Set for the ExecutionControl.
      this->least_common_time_step_seconds = lcts;
      this->least_common_time_step         = Int64BaseTime::to_base_time( lcts );
   }
}

void ExecutionControl::refresh_least_common_time_step()
{
   // Refresh the LCTS by setting the value again, which will calculate a new
   // LCTS using the HLA base time units.
   set_least_common_time_step( this->least_common_time_step_seconds );
}

void ExecutionControl::set_time_padding( double t )
{
   int64_t padding_base_time = Int64BaseTime::to_base_time( t );

   // The Master federate padding time must be an integer multiple of 3 or
   // more times the Least Common Time Step (LCTS). This will give commands
   // time to propagate through the system and still have time for mode
   // transitions.
   if ( padding_base_time < ( 3 * this->least_common_time_step ) ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::set_time_padding():" << __LINE__
             << " ERROR: Mode transition padding time (" << padding_base_time
             << " " << Int64BaseTime::get_units()
             << ") is not a multiple of 3 or more of the ExCO"
             << " Least Common Time Step (" << this->least_common_time_step
             << " " << Int64BaseTime::get_units() << ")!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Need to check that time padding is valid.
   if ( ( padding_base_time % this->least_common_time_step ) != 0 ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::set_time_padding():" << __LINE__
             << " ERROR: Time padding value (" << t
             << " seconds) must be an integer multiple of the Least Common Time Step ("
             << this->least_common_time_step << " "
             << Int64BaseTime::get_units() << ")!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // The Master federate padding time must be 3 or more times the Least
   // Common Time Step (LCTS) or two seconds. This will give commands time to
   // propagate through the system and still have time for mode transitions.
   if ( ( padding_base_time < Int64BaseTime::to_base_time( THLA_PADDING_DEFAULT ) )
        && ( padding_base_time < ( 3 * this->least_common_time_step ) ) ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::set_time_padding():" << __LINE__
             << " ERROR: Padding time ("
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
         errmsg << "TrickHLA::ExecutionControl::set_time_padding():" << __LINE__
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

   this->time_padding = Int64BaseTime::to_seconds( padding_base_time );
}

void ExecutionControl::start_federation_save_at_scenario_time(
   double      freeze_scenario_time,
   char const *file_name )
{
   ostringstream errmsg;
   errmsg << "TrickHLA::ExecutionControl::start_federation_save_at_scenario_time:" << __LINE__
          << " ERROR: The ExecutionControl does not yet support SAVE/RESTORE!\n";
   DebugHandler::terminate_with_message( errmsg.str() );
}
