/*!
@file TrickHLA/InteractionServices.cpp
@ingroup TrickHLA
@brief This class manages the HLA Interaction Services.

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
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Interaction.cpp}
@trick_link_dependency{InteractionItem.cpp}
@trick_link_dependency{InteractionServices.cpp}
@trick_link_dependency{Parameter.cpp}
@trick_link_dependency{ParameterItem.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{time/Int64Time.cpp}
@trick_link_dependency{utils/MutexLock.cpp}
@trick_link_dependency{utils/MutexProtection.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, April 2026, --, Refactored from old Manager class.}
@revs_end

*/

// System includes.
#include <climits>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/InteractionItem.hh"
#include "TrickHLA/InteractionServices.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/ParameterItem.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/utils/MutexProtection.hh"
#include "TrickHLA/utils/StringUtilities.hh"
#include "TrickHLA/utils/Utilities.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Exception.h"
#include "RTI/Handle.h"
#include "RTI/RTIambassador.h"
#include "RTI/Typedefs.h"
#include "RTI/VariableLengthData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
InteractionServices::InteractionServices( Federate &fed )
   : inter_count( 0 ),
     interactions( NULL ),
     interactions_queue(),
     check_interactions_count( 0 ),
     check_interactions( NULL ),
     federate( &fed )
{
   return;
}

/*!
 * @details Frees the Trick allocated memory.
 * @job_class{shutdown}
 */
InteractionServices::~InteractionServices()
{
   return;
}

void InteractionServices::restart_initialization()
{
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "InteractionServices::restart_initialization():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
      message_publish( MSG_NORMAL, "InteractionServices::restart_initialization():%d\n", __LINE__ );
   }

   // Verify the user specified object and interaction arrays and counts.
   verify_interaction_arrays();

   // The set_master() function set's additional parameter so call it again to
   // force the a complete master state.
   bool master_flag = federate->execution_control->is_master();
   federate->execution_control->set_master( master_flag );

   // Setup all the Trick Ref-Attributes for the user specified objects,
   // attributes, interactions and parameters.
   setup_interaction_ref_attributes();

   // Setup all the RTI handles for the objects, attributes and interaction
   // parameters.
   setup_interaction_RTI_handles();

   // Restore checkpointed interactions.
   restore_data_after_checkpoint();
}

/*! @brief Verify the user specified object and interaction arrays and counts. */
void InteractionServices::verify_interaction_arrays()
{
   // Check for the error condition of a valid interaction count but a null
   // interactions array.
   if ( ( inter_count > 0 ) && ( interactions == NULL ) ) {
      ostringstream errmsg;
      errmsg << "InteractionServices::verify_interaction_arrays():" << __LINE__
             << " ERROR: Unexpected NULL 'interactions' array for a non zero"
             << " inter_count:" << inter_count << ". Please check your input or"
             << " modified-data files to make sure the 'InteractionServices::interactions'"
             << " array is correctly configured." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // If we have a non-NULL interactions array but the interactions-count is
   // invalid then let the user know.
   if ( ( inter_count <= 0 ) && ( interactions != NULL ) ) {
      ostringstream errmsg;
      errmsg << "InteractionServices::verify_interaction_arrays():" << __LINE__
             << " ERROR: Unexpected " << ( ( inter_count == 0 ) ? "zero" : "negative" )
             << " inter_count:" << inter_count << " for a non-NULL 'interactions'"
             << " array. Please check your input or modified-data files to make"
             << " sure the 'InteractionServices::interactions' array is correctly configured."
             << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( inter_count >= INT_MAX ) {
      ostringstream errmsg;
      errmsg << "InteractionServices::verify_interaction_arrays():" << __LINE__
             << " ERROR: Unexpected inter_count:" << inter_count << " >= " << INT_MAX
             << ". Please check your input or modified-data files to make sure"
             << " the 'InteractionServices::interactions' array is correctly configured."
             << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Interaction count if negative.
   if ( inter_count < 0 ) {
      inter_count = 0;
   }

   // Interactions must be unique and can not be a duplicate for a given
   // FOM-name. Only one interaction per FOM-name.
   for ( int i = 0; i < inter_count; ++i ) {
      if ( !interactions[i].get_FOM_name().empty() ) {

         for ( int k = i + 1; k < inter_count; ++k ) {
            if ( !interactions[k].get_FOM_name().empty() ) {

               if ( interactions[i].get_FOM_name() == interactions[k].get_FOM_name() ) {
                  ostringstream errmsg;
                  errmsg << "InteractionServices::verify_interaction_arrays():" << __LINE__
                         << " ERROR: Interaction '" << interactions[i].get_FOM_name()
                         << "' at array index " << i << " has the same FOM name"
                         << " as interaction '" << interactions[k].get_FOM_name()
                         << "' at array index " << k << ". Please check your"
                         << " input or modified-data files to make sure the"
                         << " interaction FOM names are unique with no duplicates." << endl;
                  DebugHandler::terminate_with_message( errmsg.str() );
                  return;
               }
            }
         }
      }
   }

   // Get a comma separated list of the execution control interaction FOM names.
   VectorOfStrings exec_fom_names_vector;
   StringUtilities::tokenize( federate->execution_control->get_interaction_FOM_names(),
                              exec_fom_names_vector,
                              "," );

   // Make sure there is not already a user defined Interaction that uses
   // the same interaction FOM name as the execution control interaction.
   for ( size_t i = 0; i < exec_fom_names_vector.size(); ++i ) {

      // Make sure Execution Control interactions names are not duplicates.
      for ( size_t n = i + 1; n < exec_fom_names_vector.size(); ++n ) {
         if ( exec_fom_names_vector[n] == exec_fom_names_vector[i] ) {
            ostringstream errmsg;
            errmsg << "InteractionServices::verify_interaction_arrays():" << __LINE__
                   << " ERROR: Execution Control has duplicate Interactions for '"
                   << exec_fom_names_vector[i]
                   << "'. Please check your Execution Control implementation to"
                   << " make sure only one interaction implementation exists per"
                   << " HLA interaction class FOM name." << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
            return;
         }
      }

      // Check Execution Control interaction names against user defined interactions.
      for ( int k = 0; k < inter_count; ++k ) {
         if ( !interactions[k].get_FOM_name().empty()
              && ( exec_fom_names_vector[i] == interactions[k].FOM_name ) ) {
            ostringstream errmsg;
            errmsg << "InteractionServices::verify_interaction_arrays():" << __LINE__
                   << " ERROR: Execution Control Interaction '"
                   << exec_fom_names_vector[i]
                   << "' has the same FOM name as user specified interaction '"
                   << interactions[k].FOM_name << "' at array index " << k
                   << ". Please check your input or modified-data files to"
                   << " make sure the interaction FOM names are unique with"
                   << " no duplicates." << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
            return;
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void InteractionServices::setup_interaction_ref_attributes()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
      message_publish( MSG_NORMAL, "InteractionServices::setup_interaction_ref_attributes():%d\n",
                       __LINE__ );
   }

   // Interactions.
   for ( int n = 0; n < inter_count; ++n ) {
      ostringstream msg;

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
         msg << "InteractionServices::setup_interaction_ref_attributes():" << __LINE__ << endl
             << "--------------- Trick REF-Attributes ---------------\n"
             << " FOM-Interaction:'" << interactions[n].get_FOM_name() << "'" << endl;
      }

      // Initialize the TrickHLA Interaction before we use it.
      interactions[n].initialize( this->federate );

      int const  param_count = interactions[n].get_parameter_count();
      Parameter *params      = interactions[n].get_parameters();

      // Process the attributes for this object.
      for ( int i = 0; i < param_count; ++i ) {

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
            msg << "   " << ( i + 1 ) << "/" << param_count
                << " FOM-Parameter:'" << params[i].get_FOM_name() << "'"
                << " Trick-Name:'" << params[i].get_trick_name() << "'" << endl;
         }

         // Initialize the TrickHLA Parameter.
         params[i].initialize( interactions[n].get_FOM_name(), n, i );
      }

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }

   // Tell the ExecutionControl object to setup the appropriate Trick Ref
   // ATTRIBUTES associated with the execution control mechanism.
   federate->execution_control->setup_interaction_ref_attributes();
}

/*!
 * @job_class{initialization}
 */
void InteractionServices::setup_interaction_RTI_handles()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
      message_publish( MSG_NORMAL, "InteractionServices::setup_interaction_RTI_handles():%d\n",
                       __LINE__ );
   }

   // Set up the object RTI handles for the ExecutionControl mechanisms.
   federate->execution_control->setup_interaction_RTI_handles();

   // Simulation Interactions.
   setup_interaction_RTI_handles( inter_count, interactions );
}

/*!
 * @job_class{initialization}
 */
void InteractionServices::setup_interaction_RTI_handles(
   int const    interactions_counter,
   Interaction *in_interactions )
{
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = federate->get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
      message_publish( MSG_NORMAL, "InteractionServices::setup_interaction_RTI_handles():%d\n",
                       __LINE__ );
   }

   string inter_FOM_name = "";
   string param_FOM_name = "";
   int    FOM_name_type  = 0; // 0:NA 1:Interaction 2:Parameter  What name we are dealing with.

   // Initialize the Interaction and Parameter RTI handles.
   try {
      wstring ws_FOM_name = L"";

      // Process all the Interactions.
      for ( int n = 0; n < interactions_counter; ++n ) {
         ostringstream msg;

         // The Interaction FOM name.
         FOM_name_type  = 1; // Interaction
         inter_FOM_name = in_interactions[n].get_FOM_name();
         StringUtilities::to_wstring( ws_FOM_name, inter_FOM_name );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
            msg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__ << endl
                << "----------------- RTI Handles (Interactions & Parameters) ---------------\n"
                << "Getting RTI Interaction-Class-Handle for"
                << " FOM-Name:'" << inter_FOM_name << "'" << endl;
         }

         // Get the Interaction class handle.
         in_interactions[n].set_class_handle( rti_amb->getInteractionClassHandle( ws_FOM_name ) );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
            string handle_str;
            StringUtilities::to_string( handle_str, in_interactions[n].get_class_handle() );
            msg << "  Result for Interaction"
                << " FOM-Name:'" << inter_FOM_name << "'"
                << " Interaction-ID:" << handle_str << endl;
         }

         // The parameters.
         int const  param_count = in_interactions[n].get_parameter_count();
         Parameter *params      = in_interactions[n].get_parameters();

         // Process the parameters for the interaction.
         for ( int i = 0; i < param_count; ++i ) {

            // The Parameter FOM name.
            FOM_name_type  = 2; // Parameter
            param_FOM_name = params[i].get_FOM_name();
            StringUtilities::to_wstring( ws_FOM_name, param_FOM_name );

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
               msg << "\tGetting RTI Parameter-Handle for '"
                   << inter_FOM_name << "'->'" << param_FOM_name << "'" << endl;
            }

            // Get the Parameter Handle.
            params[i].set_parameter_handle(
               rti_amb->getParameterHandle(
                  in_interactions[n].get_class_handle(),
                  ws_FOM_name ) );

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
               string handle_str;
               StringUtilities::to_string( handle_str, params[i].get_parameter_handle() );
               msg << "\t  Result for Parameter '"
                   << inter_FOM_name << "'->'" << param_FOM_name << "'"
                   << " Parameter-ID:" << handle_str << endl;
            }
         }

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
            message_publish( MSG_NORMAL, msg.str().c_str() );
         }
      }
   } catch ( NameNotFound const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      switch ( FOM_name_type ) {
         case 1: { // Interaction
            ostringstream errmsg;
            errmsg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__
                   << " ERROR: Interaction FOM Name '" << inter_FOM_name << "' Not Found. Please"
                   << " check your input or modified-data files to make sure the"
                   << " Interaction FOM Name is correctly specified." << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         case 2: { // Parameter
            ostringstream errmsg;
            errmsg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__
                   << " ERROR: For Interaction FOM Name '" << inter_FOM_name
                   << "', Parameter FOM Name '" << param_FOM_name
                   << "' Not Found. Please check your input or modified-data files"
                   << " to make sure the Interaction Parameter FOM Name is"
                   << " correctly specified." << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         default: { // FOM name we are working with is unknown.
            ostringstream errmsg;
            errmsg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__
                   << " ERROR: Interaction or Parameter FOM Name Not Found. Please check your input"
                   << " or modified-data files to make sure the FOM Name is"
                   << " correctly specified." << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
      }
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: FederateNotExecutionMember!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: NotConnected!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: RTIinternalError: '" << rti_err_msg << "'" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "InteractionServices::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: Exception for '" << rti_err_msg << "'" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{initialization}
 */
void InteractionServices::publish()
{
   if ( !federate->is_RTI_ready( "publish" ) ) {
      return;
   }

   // Publish the interactions.
   for ( int n = 0; n < inter_count; ++n ) {
      interactions[n].publish_interaction();
   }
}

/*!
 * @job_class{initialization}
 */
void InteractionServices::unpublish()
{
   if ( !federate->is_RTI_ready( "unpublish" ) ) {
      return;
   }

   // Unpublish all the interactions.
   for ( int i = 0; i < inter_count; ++i ) {
      // Only unpublish an interaction that we publish.
      if ( interactions[i].is_publish() ) {
         bool do_unpublish = true;
         for ( int k = 0; ( k < i ) && do_unpublish; ++k ) {
            // Unpublish an interaction Class only once, so see if we have
            // already unpublished the same interaction class that was published.
            if ( interactions[k].is_publish()
                 && ( interactions[i].get_class_handle() == interactions[k].get_class_handle() ) ) {
               do_unpublish = false;
            }
         }
         if ( do_unpublish ) {
            interactions[i].unpublish_interaction();
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void InteractionServices::subscribe()
{
   if ( !federate->is_RTI_ready( "subscribe" ) ) {
      return;
   }

   // Subscribe to the interactions.
   for ( int n = 0; n < inter_count; ++n ) {
      interactions[n].subscribe_to_interaction();
   }
}

/*!
 * @job_class{initialization}
 */
void InteractionServices::unsubscribe()
{
   if ( !federate->is_RTI_ready( "unsubscribe" ) ) {
      return;
   }

   // Unsubscribe from all the interactions.
   for ( int i = 0; i < inter_count; ++i ) {
      // Only unsubscribe from interactions that are subscribed to.
      if ( interactions[i].is_subscribe() ) {
         bool do_unsubscribe = true;
         for ( int k = 0; ( k < i ) && do_unsubscribe; ++k ) {
            // Unsubscribe from an interaction Class only once, so see if
            // we have already unsubscribed from the same interaction class
            // that was subscribed to.
            if ( interactions[k].is_subscribe()
                 && ( interactions[i].get_class_handle() == interactions[k].get_class_handle() ) ) {
               do_unsubscribe = false;
            }
         }
         if ( do_unsubscribe ) {
            interactions[i].unsubscribe_from_interaction();
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void InteractionServices::setup_preferred_order_with_RTI()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
      message_publish( MSG_NORMAL, "InteractionServices::setup_preferred_order_with_RTI():%d\n",
                       __LINE__ );
   }

   // Setup the preferred order for all the interactions.
   for ( int i = 0; i < inter_count; ++i ) {
      interactions[i].setup_preferred_order_with_RTI();
   }
}

/*!
 * @job_class{scheduled}
 */
void InteractionServices::process_interactions()
{
   // Process any ExecutionControl mode transitions.
   federate->execution_control->process_mode_interaction();

   // Just return if the interaction queue is empty.
   if ( interactions_queue.empty() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
      message_publish( MSG_NORMAL, "InteractionServices::process_interactions():%d\n", __LINE__ );
   }

   // Process all the interactions in the queue.
   while ( !interactions_queue.empty() ) {

      // Get a reference to the first item on the queue.
      InteractionItem *item = static_cast< InteractionItem * >( interactions_queue.front() );

      switch ( item->interaction_type ) {
         case INTERACTION_TYPE_USER_DEFINED: {
            // Process the interaction if we subscribed to it and the interaction
            // index is valid.
            if ( ( item->index < (size_t)inter_count )
                 && interactions[item->index].is_subscribe() ) {

               interactions[item->index].decode( item );

               interactions[item->index].process_interaction();
            }
            break;
         }
         default: {
            ostringstream errmsg;
            errmsg << "InteractionServices::process_interactions():" << __LINE__
                   << " ERROR: Encountered an invalid interaction type: "
                   << item->interaction_type
                   << ". Verify that you are specifying the correct interaction "
                   << "type defined in 'InteractionServicesTypeOfInteractionEnum' enum "
                   << "found in 'InteractionServices.hh' and re-run." << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
            return;
         }
      }

      // Now that we processed the interaction-item remove it from the queue,
      // which will result in the item being deleted and no longer valid.
      interactions_queue.pop();
   }
}

/*!
 * @job_class{scheduled}
 */
void InteractionServices::receive_interaction(
   InteractionClassHandle const  &theInteraction,
   ParameterHandleValueMap const &theParameterValues,
   VariableLengthData const      &theUserSuppliedTag,
   LogicalTime const             &theTime,
   bool const                     received_as_TSO )
{
   // Let the ExectionControl receive and process the interaction
   // immediately if it uses it. Otherwise handle as a user interaction.
   if ( !federate->execution_control->receive_interaction( theInteraction,
                                                           theParameterValues,
                                                           theUserSuppliedTag,
                                                           theTime,
                                                           received_as_TSO ) ) {

      // Find the user Interaction we received data for.
      for ( int i = 0; i < inter_count; ++i ) {

         // Process the interaction if we subscribed to it and we have the same class handle.
         if ( interactions[i].is_subscribe()
              && ( interactions[i].get_class_handle() == theInteraction ) ) {

            InteractionItem *item;
            if ( received_as_TSO ) {
               item = new InteractionItem( i,
                                           INTERACTION_TYPE_USER_DEFINED,
                                           interactions[i].get_parameter_count(),
                                           interactions[i].get_parameters(),
                                           theParameterValues,
                                           theUserSuppliedTag,
                                           theTime );
            } else {
               item = new InteractionItem( i,
                                           INTERACTION_TYPE_USER_DEFINED,
                                           interactions[i].get_parameter_count(),
                                           interactions[i].get_parameters(),
                                           theParameterValues,
                                           theUserSuppliedTag );
            }

            // Add the interaction item to the queue.
            interactions_queue.push( item );

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
               string handle;
               StringUtilities::to_string( handle, theInteraction );

               if ( received_as_TSO ) {
                  Int64Time _time;
                  _time.set( theTime );
                  message_publish( MSG_NORMAL, "InteractionServices::receive_interaction():%d ID:%s, HLA-time:%G\n",
                                   __LINE__, handle.c_str(), _time.get_time_in_seconds() );
               } else {
                  message_publish( MSG_NORMAL, "InteractionServices::receive_interaction():%d ID:%s\n",
                                   __LINE__, handle.c_str() );
               }
            }

            // Return now that we put the interaction-item into the queue
            // for processing later in the S_define main thread when the
            // federate.process_interactions() job is called to ensure data
            // coherency. Only one interaction handler per HLA interaction
            // class is supported.
            return;
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void InteractionServices::convert_data_before_checkpoint()
{
   for ( int i = 0; i < inter_count; ++i ) {
      interactions[i].convert_data_before_checkpoint();
   }

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &interactions_queue.mutex );

   if ( !interactions_queue.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
         message_publish( MSG_NORMAL, "InteractionServices::convert_data_before_checkpoint():%d interactions_queue.size():%d\n",
                          __LINE__, interactions_queue.size() );
      }

      // Get the count to use for the check.
      check_interactions_count = interactions_queue.size();

      // Allocate the interaction items base don the count.
      check_interactions = reinterpret_cast< InteractionItem * >(
         alloc_type( (int)check_interactions_count, "TrickHLA::InteractionItem" ) );
      if ( check_interactions == NULL ) {
         ostringstream errmsg;
         errmsg << "InteractionServices::convert_data_before_checkpoint():" << __LINE__
                << " ERROR: Failed to allocate enough memory for check_interactions"
                << " linear array of " << check_interactions_count << " elements." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
         interactions_queue.dump_linked_list( "InteractionServices::convert_data_before_checkpoint()" );
      }

      size_t           i;
      InteractionItem *item;

      // Iterate through the interactions-queue.
      for ( i = 0, item = static_cast< InteractionItem * >( interactions_queue.front() );
            ( i < check_interactions_count ) && ( item != NULL );
            ++i, item = static_cast< InteractionItem * >( item->next ) ) {

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
            message_publish( MSG_NORMAL, "InteractionServices::convert_data_before_checkpoint():%d \
   Checkpointing into check_interactions[%d] from interaction index %d.\n",
                             __LINE__, i, item->index );
         }
         check_interactions[i].index            = item->index;
         check_interactions[i].interaction_type = item->interaction_type;

         item->checkpoint_queue();

         check_interactions[i].parm_items_count = item->parm_items_count;
         check_interactions[i].parm_items       = item->parm_items;

         check_interactions[i].user_supplied_tag_size = item->user_supplied_tag_size;
         if ( item->user_supplied_tag_size > 0 ) {
            check_interactions[i].user_supplied_tag =
               static_cast< unsigned char * >(
                  trick_MM->declare_var( "unsigned char",
                                         (int)item->user_supplied_tag_size ) );

            memcpy( check_interactions[i].user_supplied_tag, // flawfinder: ignore
                    item->user_supplied_tag,
                    item->user_supplied_tag_size );
         } else {
            check_interactions[i].user_supplied_tag = NULL;
         }

         check_interactions[i].order_is_TSO = item->order_is_TSO;
         check_interactions[i].time         = item->time;
      }
   }
}

void InteractionServices::restore_data_after_checkpoint()
{
   if ( check_interactions_count > 0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
         message_publish( MSG_NORMAL, "InteractionServices::restore_data_after_checkpoint():%d check_interactions_count=%d\n",
                          __LINE__, check_interactions_count );
      }

      for ( int i = 0; i < inter_count; ++i ) {
         interactions[i].restore_data_after_checkpoint();
      }

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &interactions_queue.mutex );

      if ( check_interactions != NULL ) {
         for ( size_t i = 0; i < check_interactions_count; ++i ) {

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTER_SERVICES ) ) {
               message_publish( MSG_NORMAL, "InteractionServices::restore_data_after_checkpoint():%d \
restoring check_interactions[%d] into interaction index %d, parm_count=%d\n",
                                __LINE__, i, check_interactions[i].index,
                                check_interactions[i].parm_items_count );
            }

            interactions_queue.push( new InteractionItem( check_interactions[i] ) );
         }
      }
   }
}

void InteractionServices::free_converted_data_for_checkpoint()
{
   for ( int i = 0; i < inter_count; ++i ) {
      interactions[i].free_converted_data_for_checkpoint();
   }

   if ( check_interactions_count > 0 ) {
      for ( size_t i = 0; i < check_interactions_count; ++i ) {
         check_interactions[i].clear_parm_items();
      }
      if ( trick_MM->delete_var( static_cast< void * >( check_interactions ) ) ) {
         message_publish( MSG_WARNING, "InteractionServices::free_converted_data_for_checkpoint():%d WARNING failed to delete Trick Memory for 'check_interactions'\n",
                          __LINE__ );
      }
      check_interactions       = NULL;
      check_interactions_count = 0;
   }
}

void InteractionServices::print_converted_checkpoint()
{
   if ( check_interactions_count > 0 ) {
      ostringstream msg;
      msg << "InteractionServices::print_converted_checkpoint():" << __LINE__
          << "check_interactions contains these "
          << check_interactions_count << " elements:" << endl;
      for ( size_t i = 0; i < check_interactions_count; ++i ) {
         msg << "check_interactions[" << i << "].index                  = "
             << check_interactions[i].index << endl
             << "check_interactions[" << i << "].interaction_type       = '"
             << check_interactions[i].interaction_type << "'\n"
             << "check_interactions[" << i << "].parm_items_count       = "
             << check_interactions[i].parm_items_count << endl;
         for ( size_t k = 0; k < check_interactions[i].parm_items_count; ++k ) {
            msg << "check_interactions[" << i << "].parm_items[" << k << "].index    = "
                << check_interactions[i].parm_items[k].index << endl
                << "check_interactions[" << i << "].parm_items[" << k << "].size     = "
                << check_interactions[i].parm_items[k].size << endl;
         }
         msg << "check_interactions[" << i << "].user_supplied_tag_size = "
             << check_interactions[i].user_supplied_tag_size << endl
             << "check_interactions[" << i << "].order_is_TSO           = "
             << check_interactions[i].order_is_TSO << endl
             << "check_interactions[" << i << "].time                   = "
             << check_interactions[i].time.get_base_time() << endl;
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}
