/*!
@ingroup Sine
@file models/sine/src/SineInteractionHandler.cpp
@brief This class handles the HLA interactions for the sine wave simulation.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLAModel}

@tldh
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../../source/TrickHLA/Int64BaseTime.cpp}
@trick_link_dependency{../../../source/TrickHLA/InteractionHandler.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{sine/src/SineInteractionHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, Aug 2006, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/message_proto.h"

// Model include files.
#include "../include/SineInteractionHandler.hh"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/InteractionHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

// Set to 1 to send interaction as Timestamp Order (TSO), 0 for Receive Order (RO).
#define SINE_SEND_INTERACTION_TSO 1

/*!
 * @job_class{initialization}
 */
SineInteractionHandler::SineInteractionHandler()
   : TrickHLA::InteractionHandler(),
     name( NULL ),
     message( NULL ),
     time( 0.0 ),
     year( 2007 ),
     send_cnt( 0 ),
     receive_cnt( 0 )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SineInteractionHandler::~SineInteractionHandler()
{
   return;
}

void SineInteractionHandler::send_sine_interaction(
   double const send_time )
{
   // Update the time with the simulation time.
   time = send_time;

   ostringstream msg;
   msg << "Interaction from:\"" << ( ( name != NULL ) ? name : "Unknown" ) << "\" "
       << "Send-count:" << ( send_cnt + 1 );
   message_publish( MSG_NORMAL, msg.str().c_str() );

   if ( message != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( message ) ) ) {
         message_publish( MSG_WARNING, "TrickHLAModel::SineInteractionHandler::send_sine_interaction():%d WARNING failed to delete Trick Memory for 'message'\n",
                          __LINE__ );
      }
   }
   message = trick_MM->mm_strdup( msg.str().c_str() );

   // Create a User Supplied Tag based off the name in this example.
   RTI1516_USERDATA user_supplied_tag;
   if ( name != NULL ) {
      user_supplied_tag = RTI1516_USERDATA( name, strlen( name ) );
   } else {
      user_supplied_tag = RTI1516_USERDATA( 0, 0 );
   }

   // Get the HLA granted time and lookahead time.
   double hla_granted_time = get_granted_time().get_time_in_seconds();
   double lookahead_time   = get_lookahead().get_time_in_seconds();

   // Calculate the timestamp we will use to send the interaction in Timestamp
   // Order by using the HLA granted time and the lookahead time.
   // double timestamp = hla_granted_time + lookahead_time;
   double timestamp = time + lookahead_time; // DANNY2.7 use sim time because granted time may be behind a frame

#if SINE_SEND_INTERACTION_TSO
   // Notify the parent interaction handler to send the interaction using
   // Timestamp Order (TSO) at the current simulation time plus the
   // lookahead_time.
   bool was_sent = InteractionHandler::send_interaction( timestamp, user_supplied_tag );
#else
   // Notify the parent interaction handler to send the interaction using
   // Receive Order (RO).
   bool was_sent = InteractionHandler::send_interaction( user_supplied_tag );
#endif

   if ( was_sent ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         string user_supplied_tag_string;
         StringUtilities::to_string( user_supplied_tag_string, user_supplied_tag );

         ostringstream msg2;
         msg2 << "++++SENDING++++ SineInteractionHandler::send_sine_interaction("
#if SINE_SEND_INTERACTION_TSO
              << "Timestamp Order):"
#else
              << "Receive Order):"
#endif
              << __LINE__ << '\n'
              << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'\n"
              << "  message:'" << ( ( message != NULL ) ? message : "NULL" ) << "'\n"
              << "  message length:" << ( ( message != NULL ) ? strlen( message ) : 0 ) << '\n'
              << "  user-supplied-tag:'" << user_supplied_tag_string << "'\n"
              << "  user-supplied-tag-size:" << user_supplied_tag.size() << '\n'
              << "  hla_granted_time:" << send_time << " ("
              << Int64BaseTime::to_base_time( hla_granted_time ) << " " << Int64BaseTime::get_units() << ")\n"
              << "  send_time:" << send_time << " ("
              << Int64BaseTime::to_base_time( send_time ) << " " << Int64BaseTime::get_units() << ")\n"
              << "  lookahead_time:" << lookahead_time << " ("
              << Int64BaseTime::to_base_time( lookahead_time ) << " " << Int64BaseTime::get_units() << ")\n"
              << "  timestamp:" << timestamp << " ("
              << Int64BaseTime::to_base_time( timestamp ) << " " << Int64BaseTime::get_units() << ")\n"
              << "  time:" << time << '\n'
              << "  year:" << year << '\n'
              << "  send_cnt:" << ( send_cnt + 1 ) << '\n';
         message_publish( MSG_NORMAL, msg2.str().c_str() );
      }

      // Update the send count, which is just used for the message in this example.
      ++send_cnt;
   } else {
      // Use the inherited debug-handler to allow debug comments to be turned
      // on and off from a setting in the input file. Use a higher debug level.
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         // The interaction was Not sent.
         ostringstream msg2;
         msg2 << "+-+-NOT SENT-+-+ SineInteractionHandler::send_sine_interaction():"
              << __LINE__ << '\n'
              << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'\n";
         message_publish( MSG_NORMAL, msg2.str().c_str() );
      }
   }
}

void SineInteractionHandler::receive_interaction(
   RTI1516_USERDATA const &the_user_supplied_tag )
{
   ++receive_cnt;

   // Convert the HLA User Supplied Tag back into a string we can use.
   string user_tag_string;
   StringUtilities::to_string( user_tag_string, the_user_supplied_tag );

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      ostringstream msg;
      msg << "++++RECEIVING++++ SineInteractionHandler::receive_interaction():"
          << __LINE__ << '\n'
          << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'\n"
          << "  message:'" << ( ( message != NULL ) ? message : "NULL" ) << "'\n"
          << "  message length:" << ( ( message != NULL ) ? strlen( message ) : 0 ) << '\n'
          << "  user-supplied-tag:'" << user_tag_string << "'\n"
          << "  user-supplied-tag-size:" << the_user_supplied_tag.size() << '\n'
          << "  scenario_time:" << get_scenario_time() << '\n'
          << "  time:" << time << '\n'
          << "  year:" << year << '\n'
          << "  receive_cnt:" << receive_cnt << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}
