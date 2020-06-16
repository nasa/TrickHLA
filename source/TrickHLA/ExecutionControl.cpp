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
@trick_link_dependency{SyncPntListBase.cpp}
@trick_link_dependency{ExecutionControl.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, TrickHLA support and testing.}
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
#include "TrickHLA/ExecutionConfiguration.hh"
#include "TrickHLA/ExecutionControl.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/InteractionItem.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/Parameter.hh"

// Simple TrickHL::ExecutionControl file level declarations.
namespace TrickHLA
{

// ExecutionControl type string.
const std::wstring ExecutionControl::type = L"Simple";

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

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl()
{
   return;
}

/*!
 * @job_class{initialization}
 */
ExecutionControl::ExecutionControl(
   ExecutionConfiguration &exec_config )
   : ExecutionControlBase( exec_config )
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
void ExecutionControl::initialize()
{

   // Simple initialization does not support a Master.
   this->use_preset_master = false;

   // Simple initialization does not support known federates.
   federate->enable_known_feds = false;
   federate->known_feds_count  = 0;

   return;
}

/*!
@details This routine implements the TrickHLA Join Federation Process described.

@job_class{initialization}
*/
void ExecutionControl::join_federation_process()
{

   // The base class implementation is good enough for now.
   TrickHLA::ExecutionControlBase::join_federation_process();

   return;
}

/*!
@details This routine implements the TrickHLA pre multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::pre_multi_phase_init_processes()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "TrickHLA::ExecutionControl::pre_multi_phase_init_processes():%d", __LINE__ );
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
   if ( !this->is_master_preset() ) {
      this->set_master( federate->is_federation_created_by_federate() );
   }

   // Don't forget to enable asynchronous delivery of messages.
   federate->enable_async_delivery();

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      if ( this->is_master() ) {
         send_hs( stdout, "TrickHLA::ExecutionControl::pre_multi_phase_init_processes():%d\n    I AM THE MASTER%c",
                  __LINE__, THLA_NEWLINE );
      } else {
         send_hs( stdout, "TrickHLA::ExecutionControl::pre_multi_phase_init_processes():%d\n    I AM NOT THE MASTER%c",
                  __LINE__, THLA_NEWLINE );
      }
   }

   // Setup the simulation configuration object now that we know if we are
   // the "Master" federate or not.
   execution_configuration->set_master( this->is_master() );

   if ( this->is_master() ) {
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
   get_manager()->wait_on_reservation_of_object_names();

   // Creates an RTI object instance and registers it with the RTI, but
   // only for the objects that are locally owned.
   get_manager()->register_objects_with_RTI();

   // Setup the preferred order for all object attributes and interactions.
   get_manager()->setup_preferred_order_with_RTI();

   return;
}

/*!
@details This routine implements the TrickHLA post multi-phase initialization process.

@job_class{initialization}
*/
void ExecutionControl::post_multi_phase_init_processes()
{
   // Make sure we setup time constrained and time regulating with the RTI.
   federate->setup_time_management();

   // Jump to the GALT.
   federate->time_advance_request_to_GALT();

   return;
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
 * ref ATTRIBUTES for any mode transition interactions.  This implementation
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
   ExecutionConfiguration *ExCO = this->get_execution_configuration();
   if ( ExCO != NULL ) {
      this->manager->setup_object_RTI_handles( 1, ExCO );
   } else {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::setup_object_RTI_handles():" << __LINE__
             << " Unexpected NULL ExCO!" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }
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
void ExecutionControl::add_initialization_sync_points()
{

   // Add the initialization synchronization points used for startup regulation.
   // This version of ExecutionControl does not have any.

   // Add the multiphase initialization synchronization points.
   this->add_multiphase_init_sync_points();

   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::add_multiphase_init_sync_points()
{
   return;
}

void ExecutionControl::announce_sync_point(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   wstring const &                   label,
   RTI1516_USERDATA const &          user_supplied_tag )
{

   // In this case the default SyncPntListBase::announce_sync_point works.
   // Strictly speaking, we could just not define this.  However, this provides
   // a place to implement if that changes.
   SyncPntListBase::announce_sync_point( rti_ambassador, label, user_supplied_tag );

   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionControl::clear_multiphase_init_sync_points()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::clear_multiphase_init_sync_points():" << __LINE__
             << " This call will be ignored because this ExecutionControl does not"
             << " support multiphase initialization synchronization points." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
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
 * @job_class{scheduled}
 */
void ExecutionControl::receive_interaction(
   RTI1516_NAMESPACE::InteractionClassHandle const & theInteraction,
   RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
   RTI1516_USERDATA const &                          theUserSuppliedTag,
   RTI1516_NAMESPACE::LogicalTime const &            theTime,
   bool                                              received_as_TSO )
{

   // Return now that we put the interaction-item into the queue.
   return;
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
   ExecutionConfiguration *ExCO;

   ExCO = dynamic_cast< ExecutionConfiguration * >( execution_configuration );
   if ( ExCO == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::get_execution_configuration():" << __LINE__
             << " ERROR: Execution Configuration is not an TrickHLA ExCO." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }
   return ( ExCO );
}

/*!
 * @details WARNING: Only the Master federate should ever set this.
 */
void ExecutionControl::set_least_common_time_step(
   int64_t lcts )
{
   // WARNING: Only the Master federate should ever set this.
   if ( this->is_master() ) {

      ExecutionConfiguration *ExCO = dynamic_cast< ExecutionConfiguration * >( execution_configuration );
      if ( ExCO == NULL ) {
         ostringstream errmsg;
         errmsg << "TrickHLA::ExecutionControl::set_least_common_time_step():" << __LINE__
                << " ERROR: Execution Configuration is not an TrickHLA ExCO." << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );
         exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
      }

      // Set for the ExecutionControl.
      this->least_common_time_step = lcts;
   }
   return;
}

void ExecutionControl::set_time_padding( double t )
{
   int64_t       int_time;
   ostringstream msg;

   // Need to check that time padding is valid.
   int_time = ( int64_t )( t * 1000000.0 );
   if ( ( int_time % least_common_time_step ) != 0 ) {
      msg << "TrickHLA::ExecutionControl::set_time_padding():" << __LINE__
          << " Time padding value (" << t
          << " must be an integer multiple of the Least Common Time Step ("
          << least_common_time_step << ")!" << THLA_NEWLINE;
      send_hs( stderr, msg.str().c_str() );
      exec_terminate( __FILE__, msg.str().c_str() );
      return;
   }

   // The Master federate padding time must be an integer multiple of 3 or
   // more times the Least Common Time Step (LCTS).  This will give commands
   // time to propagate through the system and still have time for mode
   // transitions.
   if ( int_time < ( 3 * least_common_time_step ) ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionControl::set_time_padding():" << __LINE__
             << " ERROR: Mode transition padding time (" << int_time
             << " microseconds) is not a multiple of 3 or more of the ExCO"
             << " Least Common Time Step (" << least_common_time_step
             << " microseconds)!" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   this->time_padding = t;

   return;
}

void ExecutionControl::start_federation_save_at_scenario_time(
   double      freeze_scenario_time,
   const char *file_name )
{
   ostringstream errmsg;
   errmsg << "TrickHLA::ExecutionControl::start_federation_save_at_scenario_time:" << __LINE__
          << " ERROR: The ExecutionControl does not yet support SAVE/RESTORE!" << THLA_ENDL;
   send_hs( stderr, (char *)errmsg.str().c_str() );
   exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   return;
}
