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
#include "trick/exec_proto.h"
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
      if ( TMM_is_alloced( (char *)user_supplied_tag ) ) {
         TMM_delete_var_a( user_supplied_tag );
      }
      user_supplied_tag      = NULL;
      user_supplied_tag_size = 0;
   }

   // Make sure we destroy the mutex.
   (void)mutex.unlock();
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
             << " ERROR: Unexpected NULL TrickHLA-Manager!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->manager = trickhla_mgr;

   // Make sure we have a valid object FOM name.
   if ( ( FOM_name == NULL ) || ( *FOM_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Interaction::initialize():" << __LINE__
             << " ERROR: Missing Interaction FOM Name."
             << " Please check your input or modified-data files to make sure the"
             << " Interaction FOM name is correctly specified." << THLA_ENDL;
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
             << " files to make sure the 'preferred_order' is correctly specified."
             << THLA_ENDL;
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
             << " correctly specified." << THLA_ENDL;
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
             << " are correctly specified." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Parameters count if it is negative or if there
   // are no attributes.
   if ( ( param_count < 0 ) || ( parameters == NULL ) ) {
      param_count = 0;
   }

   // We must have an interaction handler specified, otherwise we can not
   // process the interaction.
   if ( handler == NULL ) {
      ostringstream errmsg;
      errmsg << "Interaction::initialize():" << __LINE__
             << " ERROR: An Interaction-Handler for"
             << " 'handler' was not specified for the '" << FOM_name << "'"
             << " interaction. Please check your input or modified-data files to"
             << " make sure an Interaction-Handler is correctly specified."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      // Initialize the Interaction-Handler.
      handler->initialize_callback( this );
   }
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Interaction::set_user_supplied_tag(
   unsigned char *tag,
   size_t         tag_size )
{
   if ( tag_size > user_supplied_tag_capacity ) {
      user_supplied_tag_capacity = tag_size;
      if ( user_supplied_tag == NULL ) {
         user_supplied_tag = (unsigned char *)TMM_declare_var_1d( "char", (int)user_supplied_tag_capacity );
      } else {
         user_supplied_tag = (unsigned char *)TMM_resize_array_1d_a( user_supplied_tag, (int)user_supplied_tag_capacity );
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
   if ( is_shutdown_called() ) {

      // Get the RTI-Ambassador and check for NULL.
      RTIambassador *rti_amb = get_RTI_ambassador();
      if ( rti_amb != NULL ) {
         if ( is_publish() ) {

            // Macro to save the FPU Control Word register value.
            TRICKHLA_SAVE_FPU_CONTROL_WORD;

            // Un-publish the Interaction.
            try {
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
                  send_hs( stdout, "Interaction::remove():%d Unpublish Interaction '%s'.%c",
                           __LINE__, get_FOM_name(), THLA_NEWLINE );
               }

               rti_amb->unpublishInteractionClass( get_class_handle() );
            } catch ( RTI1516_EXCEPTION const &e ) {
               string rti_err_msg;
               StringUtilities::to_string( rti_err_msg, e.what() );
               send_hs( stderr, "Interaction::remove():%d Unpublish Interaction '%s' exception '%s'%c",
                        __LINE__, get_FOM_name(), rti_err_msg.c_str(), THLA_NEWLINE );
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
      send_hs( stderr, "Interaction::setup_preferred_order_with_RTI():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      send_hs( stdout, "Interaction::setup_preferred_order_with_RTI():%d \
Published Interaction '%s' Preferred-Order:%s%c",
               __LINE__, get_FOM_name(),
               ( preferred_order == TRANSPORT_TIMESTAMP_ORDER ? "TIMESTAMP" : "RECEIVE" ),
               THLA_NEWLINE );
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
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: FederateNotExecutionMember for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::InteractionClassNotDefined const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: InteractionClassNotDefined for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: RestoreInProgress for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: RTIinternalError for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: SaveInProgress for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::setup_preferred_order_with_RTI():" << __LINE__
             << " EXCEPTION: NotConnected for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
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
             << "' with error '" << rti_err_msg << "'" << THLA_ENDL;
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
      send_hs( stderr, "Interaction::publish_interaction():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      send_hs( stdout, "Interaction::publish_interaction():%d Interaction '%s'.%c",
               __LINE__, get_FOM_name(), THLA_NEWLINE );
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
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::InteractionClassNotDefined const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: InteractionClassNotDefined for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: RestoreInProgress for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: RTIinternalError for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: SaveInProgress for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Interaction::publish_interaction():" << __LINE__
             << " EXCEPTION: NotConnected for Interaction '"
             << get_FOM_name() << "'" << THLA_ENDL;
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
             << "' with error '" << rti_err_msg << "'" << THLA_ENDL;
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
      send_hs( stderr, "Interaction::unpublish_interaction():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   // Subscribe to the interaction.
   if ( is_publish() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         send_hs( stdout, "Interaction::unpublish_interaction():%d Interaction '%s'%c",
                  __LINE__, get_FOM_name(), THLA_NEWLINE );
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
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: FederateNotExecutionMember for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: SaveInProgress for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: RestoreInProgress for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: NotConnected for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unpublish_interaction():" << __LINE__
                << " EXCEPTION: RTIinternalError for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
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
                << "' with error '" << rti_err_msg << "'" << THLA_ENDL;
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
      send_hs( stderr, "Interaction::subscribe_to_interaction():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   // Subscribe to the interaction.
   if ( is_subscribe() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         send_hs( stdout, "Interaction::subscribe_to_interaction():%d Interaction '%s'%c",
                  __LINE__, get_FOM_name(), THLA_NEWLINE );
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
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::FederateServiceInvocationsAreBeingReportedViaMOM const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: FederateServiceInvocationsAreBeingReportedViaMOM for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::InteractionClassNotDefined const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: InteractionClassNotDefined for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: RestoreInProgress for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: RTIinternalError for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION:  SaveInProgress for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::subscribe_to_interaction():" << __LINE__
                << " EXCEPTION: NotConnected for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
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
                << "' with error '" << rti_err_msg << "'" << THLA_ENDL;
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
      send_hs( stderr, "Interaction::unsubscribe_from_interaction():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   // Make sure we only unsubscribe an interaction that was subscribed to.
   if ( is_subscribe() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         send_hs( stdout, "Interaction::unsubscribe_from_interaction():%d Interaction '%s'%c",
                  __LINE__, get_FOM_name(), THLA_NEWLINE );
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
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: FederateNotExecutionMember for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: SaveInProgress for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: RestoreInProgress for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: NotConnected for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Interaction::unsubscribe_from_interaction():" << __LINE__
                << " EXCEPTION: RTIinternalError for Interaction '"
                << get_FOM_name() << "'" << THLA_ENDL;
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
                << "' with error '" << rti_err_msg << "'" << THLA_ENDL;
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
      send_hs( stderr, "Interaction::send():%d As Receive-Order: Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
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
      for ( unsigned int i = 0; i < param_count; ++i ) {
         param_values_map[parameters[i].get_parameter_handle()] = parameters[i].get_encoded_parameter_value();
      }

      // Release mutex lock as auto_unlock_mutex goes out of scope
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      send_hs( stdout, "Interaction::send():%d As Receive-Order: Interaction '%s'%c",
               __LINE__, get_FOM_name(), THLA_NEWLINE );
   }

   bool successfuly_sent = false;
   try {
      // RECEIVE_ORDER with no timestamp.

      // Do not send any interactions if federate save / restore has begun (see
      // IEEE-1516.1-2000 sections 4.12, 4.20)
      if ( federate->should_publish_data() ) {
         // This call returns an event retraction handle but we
         // don't support event retraction so no need to store it.
         (void)rti_amb->sendInteraction( this->class_handle,
                                         param_values_map,
                                         the_user_supplied_tag );
         successfuly_sent = true;
      }
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Interaction::send():%d As Receive-Order: Interaction '%s' with exception '%s'%c",
               __LINE__, get_FOM_name(), rti_err_msg.c_str(), THLA_NEWLINE );
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
      send_hs( stderr, "Interaction::send():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
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
      for ( unsigned int i = 0; i < param_count; ++i ) {
         if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
            send_hs( stdout, "Interaction::send():%d Adding '%s' to parameter map.%c",
                     __LINE__, parameters[i].get_FOM_name(), THLA_NEWLINE );
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
      // IEEE-1516.1-2000 sections 4.12, 4.20)
      if ( federate->should_publish_data() ) {

         // The message will only be sent as TSO if our Federate is in the HLA Time
         // Regulating state and the interaction prefers timestamp order.
         // See IEEE-1516.1-2000, Sections 6.6 and 8.1.1.
         if ( send_with_timestamp ) {

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
               send_hs( stdout, "Interaction::send():%d As Timestamp-Order: Interaction '%s' sent for time %lf seconds.%c",
                        __LINE__, get_FOM_name(), time.get_time_in_seconds(), THLA_NEWLINE );
            }

            // This call returns an event retraction handle but we
            // don't support event retraction so no need to store it.
            // Send in Timestamp Order.
            (void)rti_amb->sendInteraction( this->class_handle,
                                            param_values_map,
                                            the_user_supplied_tag,
                                            time.get() );
            successfuly_sent = true;

         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
               send_hs( stdout, "Interaction::send():%d As Receive-Order: \
Interaction '%s' is time-regulating:%s, preferred-order:%s.%c",
                        __LINE__, get_FOM_name(),
                        ( federate->in_time_regulating_state() ? "Yes" : "No" ),
                        ( ( preferred_order == TRANSPORT_RECEIVE_ORDER ) ? "receive" : "timestamp" ),
                        THLA_NEWLINE );
            }

            // Send in Receive Order (i.e. with no timestamp).
            (void)rti_amb->sendInteraction( this->class_handle,
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
             << " error message:'" << rti_err_msg << "'" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Interaction::send():" << __LINE__ << " As "
             << ( send_with_timestamp ? "Timestamp Order" : "Receive Order" )
             << ", Interaction '" << get_FOM_name() << "' with exception '"
             << rti_err_msg << "'" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
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
            send_hs( stdout, "Interaction::process_interaction():%d ID:%s, FOM_name:'%s', HLA time:%G, Timestamp-Order%c",
                     __LINE__, handle_str.c_str(), get_FOM_name(),
                     time.get_time_in_seconds(), THLA_NEWLINE );
         } else {
            send_hs( stdout, "Interaction::process_interaction():%d ID:%s, FOM_name:'%s', Receive-Order%c",
                     __LINE__, handle_str.c_str(), get_FOM_name(), THLA_NEWLINE );
         }
      }

      // Unpack all the parameter data.
      for ( unsigned int i = 0; i < param_count; ++i ) {
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

void Interaction::extract_data(
   InteractionItem *interaction_item )
{
   // Must be set to subscribe to the interaction and the interaction item
   // is not null, otherwise just return.
   if ( !is_subscribe() || ( interaction_item == NULL ) ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      string handle_str;
      StringUtilities::to_string( handle_str, class_handle );
      send_hs( stdout, "Interaction::extract_data():%d ID:%s, FOM_name:'%s'%c",
               __LINE__, handle_str.c_str(), get_FOM_name(), THLA_NEWLINE );
   }

   if ( interaction_item->is_timestamp_order() ) {
      // Update the timestamp.
      time.set( interaction_item->time );

      // Received in Timestamp Order (TSO).
      received_as_TSO = true;
   } else {
      // Receive Order (RO), not Timestamp Order (TSO).
      received_as_TSO = false;
   }

   // For thread safety, lock here to avoid corrupting the parameters.

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   // Extract the user supplied tag.
   if ( interaction_item->user_supplied_tag_size > 0 ) {
      set_user_supplied_tag( interaction_item->user_supplied_tag, interaction_item->user_supplied_tag_size );
      mark_changed();
   } else {
      set_user_supplied_tag( (unsigned char *)NULL, 0 );
   }

   // Process all the parameter-items in the queue.
   while ( !interaction_item->parameter_queue.empty() ) {

      ParameterItem const *param_item = static_cast< ParameterItem * >( interaction_item->parameter_queue.front() );

      // Determine if we have a valid parameter-item.
      if ( ( param_item != NULL ) && ( param_item->index >= 0 ) && ( param_item->index < param_count ) ) {

         if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
            send_hs( stdout, "Interaction::extract_data():%d Decoding '%s' from parameter map.%c",
                     __LINE__, parameters[param_item->index].get_FOM_name(),
                     THLA_NEWLINE );
         }
         // Extract the parameter data for the given parameter-item.
         parameters[param_item->index].extract_data( param_item->size, param_item->data );

         // Mark the interaction as changed.
         mark_changed();
      }

      // Now that we extracted the data from the parameter-item remove it
      // from the queue.
      interaction_item->parameter_queue.pop();
   }

   // Unlock the mutex as auto_unlock_mutex goes out of scope.
}

void Interaction::mark_unchanged()
{
   this->changed = false;

   // Clear the change flag for each of the attributes as well.
   for ( unsigned int i = 0; i < param_count; ++i ) {
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
   return ( ( this->manager != NULL ) ? this->manager->get_federate() : NULL );
}

RTIambassador *Interaction::get_RTI_ambassador()
{
   return ( ( this->manager != NULL ) ? this->manager->get_RTI_ambassador()
                                      : static_cast< RTI1516_NAMESPACE::RTIambassador * >( NULL ) );
}

bool Interaction::is_shutdown_called() const
{
   return ( ( this->manager != NULL ) ? this->manager->is_shutdown_called() : false );
}
