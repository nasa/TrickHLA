/*!
@file TrickHLA/Interaction.cpp
@ingroup TrickHLA
@brief This class represents an HLA Interaction that is managed by Trick.

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
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Int64Interval.cpp}
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{Interaction.cpp}
@trick_link_dependency{InteractionHandler.cpp}
@trick_link_dependency{InteractionItem.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{Parameter.cpp}
@trick_link_dependency{ParameterItem.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Aug 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/InteractionHandler.hh"
#include "TrickHLA/InteractionItem.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/ParameterItem.hh"
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

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Interaction::Interaction()
   : FOM_name( NULL ),
     publish( false ),
     subscribe( false ),
     preferred_order( TRANSPORT_SPECIFIED_IN_FOM ),
     param_count( 0 ),
     parameters( NULL ),
     handler( NULL ),
     mutex(),
     changed( false ),
     received_as_TSO( false ),
     time( 0.0 ),
     manager( NULL ),
     user_supplied_tag_size( 0 ),
     user_supplied_tag_capacity( 0 ),
     user_supplied_tag( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
Interaction::~Interaction()
{
   // Remove this interaction from the federation execution.
   remove();

   if ( user_supplied_tag != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( user_supplied_tag ) ) ) {
         message_publish( MSG_WARNING, "Interaction::~Interaction():%d WARNING failed to delete Trick Memory for 'user_supplied_tag'\n",
                          __LINE__ );
      }
      user_supplied_tag      = NULL;
      user_supplied_tag_size = 0;
   }

   // Make sure we destroy the mutex.
   mutex.destroy();
}

/*!
 * @job_class{initialization}
 */
void Interaction::initialize(
   Manager *trickhla_mgr )
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( trickhla_mgr == NULL ) {
      ostringstream errmsg;
      errmsg << "Interaction::initialize():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA-Manager!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->manager = trickhla_mgr;

   // Make sure we have a valid object FOM name.
   if ( ( FOM_name == NULL ) || ( *FOM_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Interaction::initialize():" << __LINE__
             << " ERROR: Missing Interaction FOM Name."
             << " Please check your input or modified-data files to make sure the"
             << " Interaction FOM name is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // TODO: Get the preferred order by parsing the FOM.
   //
   // Do a quick bounds check on the 'preferred_order' value.
   if ( ( preferred_order < TRANSPORT_FIRST_VALUE ) || ( preferred_order > TRANSPORT_LAST_VALUE ) ) {
      ostringstream errmsg;
      errmsg << "Interaction::initialize():" << __LINE__
             << " ERROR: For Interaction '"
             << FOM_name << "', the 'preferred_order' is not valid and must be one"
             << " of TRANSPORT_SPECIFIED_IN_FOM, TRANSPORT_TIMESTAMP_ORDER or"
             << " TRANSPORT_RECEIVE_ORDER. Please check your input or modified-data"
             << " files to make sure the 'preferred_order' is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have an parameter count but no parameters then let the user know.
   if ( ( param_count > 0 ) && ( parameters == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Interaction::initialize():" << __LINE__
             << " ERROR: For Interaction '"
             << FOM_name << "', the 'param_count' is " << param_count
             << " but no 'parameters' are specified. Please check your input or"
             << " modified-data files to make sure the Interaction Parameters are"
             << " correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have parameters but the parameter-count is invalid then let
   // the user know.
   if ( ( param_count <= 0 ) && ( parameters != NULL ) ) {
      ostringstream errmsg;
      errmsg << "Interaction::initialize():" << __LINE__
             << " ERROR: For Interaction '"
             << FOM_name << "', the 'param_count' is " << param_count
             << " but 'parameters' have been specified. Please check your input"
             << " or modified-data files to make sure the Interaction Parameters"
             << " are correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Parameters count if it is negative or if there
   // are no attributes.
   if ( ( param_count < 0 ) || ( parameters == NULL ) ) {
      param_count = 0;
   }

   // Verify parameter FOM names and also check for duplicate parameter FOM names.
   for ( int i = 0; i < param_count; ++i ) {
      // Validate the FOM-name to make sure we don't have a problem with the
      // list of names as well as get a difficult to debug runtime error for
      // the string constructor if we had a null FOM-name.
      if ( ( parameters[i].get_FOM_name() == NULL ) || ( *( parameters[i].get_FOM_name() ) == '\0' ) ) {
         ostringstream errmsg;
         errmsg << "Interaction::initialize():" << __LINE__
                << " ERROR: Interaction '" << FOM_name << "' has a missing Parameter"
                << " FOM Name at array index " << i << ". Please check your input"
                << " or modified-data files to make sure the interaction parameter"
                << " FOM name is correctly specified.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      string fom_name_str( parameters[i].get_FOM_name() );

      // Since Interaction updates are sent as a ParameterHandleValueMap there can be
      // no duplicate Parameters because the map only allows unique ParameterHandles.
      for ( int k = i + 1; k < param_count; ++k ) {
         if ( ( parameters[k].get_FOM_name() != NULL ) && ( *( parameters[k].get_FOM_name() ) != '\0' ) ) {

            if ( fom_name_str == string( parameters[k].get_FOM_name() ) ) {
               ostringstream errmsg;
               errmsg << "Interaction::initialize():" << __LINE__
                      << " ERROR: Interaction '" << FOM_name << "' has Parameters"
                      << " at array indexes " << i << " and " << k
                      << " that have the same FOM Name '" << fom_name_str
                      << "'. Please check your input or modified-data files to"
                      << " make sure the interaction parameters do not use"
                      << " duplicate FOM names.\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }
      }
   }

   // We must have an interaction handler specified, otherwise we can not
   // process the interaction.
   if ( handler == NULL ) {
      ostringstream errmsg;
      errmsg << "Interaction::initialize():" << __LINE__
             << " ERROR: An Interaction-Handler for"
             << " 'handler' was not specified for the '" << FOM_name << "'"
             << " interaction. Please check your input or modified-data files to"
             << " make sure an Interaction-Handler is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      // Initialize the Interaction-Handler.
      handler->initialize_callback( this );
   }
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Interaction::set_user_supplied_tag(
   unsigned char const *tag,
   int                  tag_size )
{
   if ( tag_size > user_supplied_tag_capacity ) {
      user_supplied_tag_capacity = tag_size;
      if ( user_supplied_tag == NULL ) {
         user_supplied_tag = static_cast< unsigned char * >( TMM_declare_var_1d( "unsigned char", user_supplied_tag_capacity ) );
      } else {
         user_supplied_tag = static_cast< unsigned char * >( TMM_resize_array_1d_a( user_supplied_tag, user_supplied_tag_capacity ) );
      }
   }
   user_supplied_tag_size = tag_size;
   if ( tag != NULL ) {
      memcpy( user_supplied_tag, tag, user_supplied_tag_size );
   } else {
      memset( user_supplied_tag, 0, user_supplied_tag_size );
   }
}

/*!
 * @details Called from the virtual destructor.
 * @job_class{shutdown}
 */
void Interaction::remove() // RETURN: -- None.
{
   // Only remove the Interaction if the manager has not been shutdown.
   if ( !is_shutdown_called() ) {

      // Get the RTI-Ambassador and check for NULL.
      RTIambassador *rti_amb = get_RTI_ambassador();
      if ( rti_amb != NULL ) {
         if ( is_publish() ) {

            // Macro to save the FPU Control Word register value.
            TRICKHLA_SAVE_FPU_CONTROL_WORD;

            // Un-publish the Interaction.
            try {
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
                  message_publish( MSG_NORMAL, "Interaction::remove():%d Unpublish Interaction '%s'.\n",
                                   __LINE__, get_FOM_name() );
               }

               rti_amb->unpublishInteractionClass( get_class_handle() );
            } catch ( RTI1516_EXCEPTION const &e ) {
               string rti_err_msg;
               StringUtilities::to_string( rti_err_msg, e.what() );
               message_publish( MSG_WARNING, "Interaction::remove():%d Unpublish Interaction '%s' exception '%s'\n",
                                __LINE__, get_FOM_name(), rti_err_msg.c_str() );
            }

            // Macro to restore the saved FPU Control Word register value.
            TRICKHLA_RESTORE_FPU_CONTROL_WORD;
            TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
         }

         // Un-subscribe from the Interaction
         unsubscribe_from_interaction();
      }
   }
}

void Interaction::setup_preferred_order_with_RTI()
{
   // Just return if the user wants to use the default preferred order
   // specified in the FOM. Return if the interaction is not published since
   // we can only change the preferred order for publish interactions.
   if ( ( preferred_order == TRANSPORT_SPECIFIED_IN_FOM ) || !is_publish() ) {
      return;
   }

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Interaction::setup_preferred_order_with_RTI():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      message_publish( MSG_NORMAL, "Interaction::setup_preferred_order_with_RTI():%d \
Published Interaction '%s' Preferred-Order:%s\n",
                       __LINE__, get_FOM_name(),
                       ( preferred_order == TRANSPORT_TIMESTAMP_ORDER ? "TIMESTAMP" : "RECEIVE" ) );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Change the preferred order.
   try {
      switch ( preferred_order ) {
         case TRANSPORT_RECEIVE_ORDER: {
            rti_amb->changeInteractionOrderType( this->class_handle,
                                                 RTI1516_NAMESPACE::RECEIVE );
            break;
         }
         case TRANSPORT_TIMESTAMP_ORDER:
         default: {
            rti_amb->changeInteractionOrderType( this->class_handle,
                                                 RTI1516_NAMESPACE::TIMESTAMP );
            break;
         }
      }
   } catch ( RTI1516_NAMESPACE::InteractionClassNotPublished const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: InteractionClassNotPublished for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: FederateNotExecutionMember for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::InteractionClassNotDefined const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: InteractionClassNotDefined for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: RestoreInProgress for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: RTIinternalError for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: SaveInProgress for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: NotConnected for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " RTI1516_EXCEPTION for Interaction '" << get_FOM_name()
             << "' with error '" << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Interaction::publish_interaction()
{
   // The RTI must be ready and the flag must be set to publish.
   if ( !is_publish() ) {
      return;
   }

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Interaction::publish_interaction():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      message_publish( MSG_NORMAL, "Interaction::publish_interaction():%d Interaction '%s'.\n",
                       __LINE__, get_FOM_name() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Publish the Interaction
   try {
      rti_amb->publishInteractionClass( this->class_handle );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: FederateNotExecutionMember for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::InteractionClassNotDefined const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: InteractionClassNotDefined for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: RestoreInProgress for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: RTIinternalError for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: SaveInProgress for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: NotConnected for Interaction '"
             << get_FOM_name() << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " RTI1516_EXCEPTION for Interaction '" << get_FOM_name()
             << "' with error '" << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Interaction::unpublish_interaction()
{
   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Interaction::unpublish_interaction():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

   // Subscribe to the interaction.
   if ( is_publish() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         message_publish( MSG_NORMAL, "Interaction::unpublish_interaction():%d Interaction '%s'\n",
                          __LINE__, get_FOM_name() );
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      try {
         rti_amb->unpublishInteractionClass( this->class_handle );
      } catch ( RTI1516_NAMESPACE::InteractionClassNotDefined const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: InteractionClassNotDefined for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: FederateNotExecutionMember for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: SaveInProgress for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: RestoreInProgress for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: NotConnected for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: RTIinternalError for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " RTI1516_EXCEPTION for Interaction '" << get_FOM_name()
                << "' with error '" << rti_err_msg << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

void Interaction::subscribe_to_interaction()
{
   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Interaction::subscribe_to_interaction():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

   // Subscribe to the interaction.
   if ( is_subscribe() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         message_publish( MSG_NORMAL, "Interaction::subscribe_to_interaction():%d Interaction '%s'\n",
                          __LINE__, get_FOM_name() );
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      try {
         rti_amb->subscribeInteractionClass( this->class_handle, true );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: FederateNotExecutionMember for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::FederateServiceInvocationsAreBeingReportedViaMOM const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: FederateServiceInvocationsAreBeingReportedViaMOM for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::InteractionClassNotDefined const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: InteractionClassNotDefined for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: RestoreInProgress for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: RTIinternalError for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION:  SaveInProgress for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: NotConnected for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " RTI1516_EXCEPTION for Interaction '" << get_FOM_name()
                << "' with error '" << rti_err_msg << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

void Interaction::unsubscribe_from_interaction()
{
   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Interaction::unsubscribe_from_interaction():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

   // Make sure we only unsubscribe an interaction that was subscribed to.
   if ( is_subscribe() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         message_publish( MSG_NORMAL, "Interaction::unsubscribe_from_interaction():%d Interaction '%s'\n",
                          __LINE__, get_FOM_name() );
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      try {
         rti_amb->unsubscribeInteractionClass( this->class_handle );
      } catch ( RTI1516_NAMESPACE::InteractionClassNotDefined const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: InteractionClassNotDefined for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: FederateNotExecutionMember for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: SaveInProgress for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: RestoreInProgress for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: NotConnected for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: RTIinternalError for Interaction '"
                << get_FOM_name() << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " RTI1516_EXCEPTION for Interaction '" << get_FOM_name()
                << "' with error '" << rti_err_msg << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

bool Interaction::send(
   RTI1516_USERDATA const &the_user_supplied_tag )
{
   // RTI must be ready and the flag must be set to publish.
   if ( !is_publish() ) {
      return ( false );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the Trick-Federate.
   Federate const *federate = get_federate();

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Interaction::send():%d As Receive-Order: Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return ( false );
   }

   ParameterHandleValueMap param_values_map;

   // For thread safety, lock here to avoid corrupting the parameters and use
   // braces to create scope for the mutex-protection to auto unlock the mutex.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      // Add all the parameter values to the map.
      for ( int i = 0; i < param_count; ++i ) {
         param_values_map[parameters[i].get_parameter_handle()] = parameters[i].get_encoded_parameter_value();
      }

      // Release mutex lock as auto_unlock_mutex goes out of scope
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      message_publish( MSG_NORMAL, "Interaction::send():%d As Receive-Order: Interaction '%s'\n",
                       __LINE__, get_FOM_name() );
   }

   bool successfuly_sent = false;
   try {
      // RECEIVE_ORDER with no timestamp.

      // Do not send any interactions if federate save / restore has begun (see
      // IEEE-1516.1-2010 sections 4.12, 4.20)
      if ( federate->should_publish_data() ) {
         // This call returns an event retraction handle but we
         // don't support event retraction so no need to store it.
         rti_amb->sendInteraction( this->class_handle,
                                   param_values_map,
                                   the_user_supplied_tag );
         successfuly_sent = true;
      }
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Interaction::send():%d As Receive-Order: Interaction '%s' with exception '%s'\n",
                       __LINE__, get_FOM_name(), rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Free the memory used in the parameter values map.
   param_values_map.clear();

   return ( successfuly_sent );
}

bool Interaction::send(
   double                  send_HLA_time,
   RTI1516_USERDATA const &the_user_supplied_tag )
{
   // RTI must be ready and the flag must be set to publish.
   if ( !is_publish() ) {
      return ( false );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Interaction::send():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return ( false );
   }

   ParameterHandleValueMap param_values_map;

   // For thread safety, lock here to avoid corrupting the parameters and use
   // braces to create scope for the mutex-protection to auto unlock the mutex.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      // Add all the parameter values to the map.
      for ( int i = 0; i < param_count; ++i ) {
         if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
            message_publish( MSG_NORMAL, "Interaction::send():%d Adding '%s' to parameter map.\n",
                             __LINE__, parameters[i].get_FOM_name() );
         }
         param_values_map[parameters[i].get_parameter_handle()] = parameters[i].get_encoded_parameter_value();
      }

      // auto_unlock_mutex unlocks the mutex here as it goes out of scope.
   }

   // Update the timestamp.
   time.set( send_HLA_time );

   // Get the Trick-Federate.
   Federate const *federate = get_federate();

   // Determine if the interaction should be sent with a timestamp.
   // See IEEE 1516.1-2010 Section 6.12.
   bool const send_with_timestamp = federate->in_time_regulating_state()
                                    && ( preferred_order != TRANSPORT_RECEIVE_ORDER );

   bool successfuly_sent = false;
   try {
      // Do not send any interactions if federate save or restore has begun (see
      // IEEE-1516.1-2010 sections 4.12, 4.20)
      if ( federate->should_publish_data() ) {

         // The message will only be sent as TSO if our Federate is in the HLA Time
         // Regulating state and the interaction prefers timestamp order.
         // See IEEE-1516.1-2010, Sections 6.6 and 8.1.1.
         if ( send_with_timestamp ) {

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
               message_publish( MSG_NORMAL, "Interaction::send():%d As Timestamp-Order: Interaction '%s' sent for time %lf seconds.\n",
                                __LINE__, get_FOM_name(), time.get_time_in_seconds() );
            }

            // This call returns an event retraction handle but we
            // don't support event retraction so no need to store it.
            // Send in Timestamp Order.
            rti_amb->sendInteraction( this->class_handle,
                                      param_values_map,
                                      the_user_supplied_tag,
                                      time.get() );
            successfuly_sent = true;

         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
               message_publish( MSG_NORMAL, "Interaction::send():%d As Receive-Order: \
Interaction '%s' is time-regulating:%s, preferred-order:%s.\n",
                                __LINE__, get_FOM_name(),
                                ( federate->in_time_regulating_state() ? "Yes" : "No" ),
                                ( ( preferred_order == TRANSPORT_RECEIVE_ORDER ) ? "receive" : "timestamp" ) );
            }

            // Send in Receive Order (i.e. with no timestamp).
            rti_amb->sendInteraction( this->class_handle,
                                      param_values_map,
                                      the_user_supplied_tag );
            successfuly_sent = true;
         }
      }
   } catch ( RTI1516_NAMESPACE::InvalidLogicalTime const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Interaction::send():" << __LINE__ << " As "
             << ( send_with_timestamp ? "Timestamp Order" : "Receive Order" )
             << ", InvalidLogicalTime exception for " << get_FOM_name()
             << "  time=" << time.get_time_in_seconds() << " ("
             << time.get_base_time() << " " << Int64BaseTime::get_units()
             << " error message:'" << rti_err_msg << "'\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Interaction::send():" << __LINE__ << " As "
             << ( send_with_timestamp ? "Timestamp Order" : "Receive Order" )
             << ", Interaction '" << get_FOM_name() << "' with exception '"
             << rti_err_msg << "'\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Free the memory used in the parameter values map.
   param_values_map.clear();

   return ( successfuly_sent );
}

void Interaction::process_interaction()
{
   // The Interaction data must have changed and the RTI must be ready.
   if ( !is_changed() ) {
      return;
   }

   // For thread safety, lock here to avoid corrupting the parameters and use
   // braces to create scope for the mutex-protection to auto unlock the mutex.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      // Check the change flag again now that we have the lock on the mutex.
      if ( !is_changed() ) {
         return;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, class_handle );
         if ( received_as_TSO ) {
            message_publish( MSG_NORMAL, "Interaction::process_interaction():%d ID:%s, FOM_name:'%s', HLA time:%G, Timestamp-Order\n",
                             __LINE__, handle_str.c_str(), get_FOM_name(),
                             time.get_time_in_seconds() );
         } else {
            message_publish( MSG_NORMAL, "Interaction::process_interaction():%d ID:%s, FOM_name:'%s', Receive-Order\n",
                             __LINE__, handle_str.c_str(), get_FOM_name() );
         }
      }

      // Unpack all the parameter data.
      for ( int i = 0; i < param_count; ++i ) {
         parameters[i].unpack_parameter_buffer();
      }

      mark_unchanged();

      // Unlock the mutex as the auto_unlock_mutex goes out of scope.
   }

   // Call the users interaction handler (callback) so that they can
   // continue processing the interaction.
   if ( handler != NULL ) {
      if ( user_supplied_tag_size > 0 ) {
         handler->receive_interaction( RTI1516_USERDATA( user_supplied_tag, user_supplied_tag_size ) );
      } else {
         handler->receive_interaction( RTI1516_USERDATA( 0, 0 ) );
      }
   }
}

bool Interaction::extract_data(
   InteractionItem *interaction_item )
{
   // Must be set to subscribe to the interaction and the interaction item
   // is not null, otherwise just return.
   if ( !is_subscribe() || ( interaction_item == NULL ) ) {
      return false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      string handle_str;
      StringUtilities::to_string( handle_str, class_handle );
      message_publish( MSG_NORMAL, "Interaction::extract_data():%d ID:%s, FOM_name:'%s'\n",
                       __LINE__, handle_str.c_str(), get_FOM_name() );
   }

   // For thread safety, lock here to avoid corrupting the parameters.

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( interaction_item->is_timestamp_order() ) {
      // Update the timestamp.
      time.set( interaction_item->time );

      // Received in Timestamp Order (TSO).
      received_as_TSO = true;
   } else {
      // Receive Order (RO), not Timestamp Order (TSO).
      received_as_TSO = false;
   }

   // Extract the user supplied tag.
   if ( interaction_item->user_supplied_tag_size > 0 ) {
      set_user_supplied_tag( interaction_item->user_supplied_tag, interaction_item->user_supplied_tag_size );
      mark_changed();
   } else {
      set_user_supplied_tag( (unsigned char *)NULL, 0 );
   }

   bool any_param_received = false;

   // Process all the parameter-items in the queue.
   while ( !interaction_item->parameter_queue.empty() ) {

      ParameterItem const *param_item = static_cast< ParameterItem * >( interaction_item->parameter_queue.front() );

      // Determine if we have a valid parameter-item.
      if ( ( param_item != NULL ) && ( param_item->index >= 0 ) && ( param_item->index < param_count ) ) {

         if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
            message_publish( MSG_NORMAL, "Interaction::extract_data():%d Decoding '%s' from parameter map.\n",
                             __LINE__, parameters[param_item->index].get_FOM_name() );
         }
         // Extract the parameter data for the given parameter-item.
         if ( parameters[param_item->index].extract_data( param_item->size, param_item->data ) ) {
            any_param_received = true;
         }
      }

      // Now that we extracted the data from the parameter-item remove it
      // from the queue.
      interaction_item->parameter_queue.pop();
   }

   if ( any_param_received ) {
      // Mark the interaction as changed.
      mark_changed();
   }
   return any_param_received;
}

void Interaction::mark_unchanged()
{
   this->changed = false;

   // Clear the change flag for each of the attributes as well.
   for ( int i = 0; i < param_count; ++i ) {
      parameters[i].mark_unchanged();
   }
}

/*!
 * @details If the manager does not exist, -1.0 seconds is assigned to the returned object.
 */
Int64Interval Interaction::get_lookahead() const
{
   Int64Interval i;
   if ( manager != NULL ) {
      i = manager->get_lookahead();
   } else {
      i = Int64Interval( -1.0 );
   }
   return i;
}

/*!
 * @details If the manager does not exist, Int64BaseTime::get_max_logical_time_in_seconds()
 * is assigned to the returned object.
 */
Int64Time Interaction::get_granted_time() const
{
   Int64Time t;
   if ( manager != NULL ) {
      t = manager->get_granted_time();
   } else {
      t = Int64Time( Int64BaseTime::get_max_logical_time_in_seconds() );
   }
   return t;
}

Federate *Interaction::get_federate()
{
   return ( ( this->manager != NULL ) ? manager->get_federate() : NULL );
}

RTIambassador *Interaction::get_RTI_ambassador()
{
   return ( ( this->manager != NULL ) ? manager->get_RTI_ambassador() : NULL );
}

bool Interaction::is_shutdown_called() const
{
   return ( ( this->manager != NULL ) ? manager->is_shutdown_called() : false );
}
