/*!
@file models/sine/src/SineInteractionHandler.cpp
@ingroup TrickHLAModel
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
@trick_link_dependency{sine/src/SineInteractionHandler.o}

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
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h" // for send_hs

// Model include files.
#include "../include/SineInteractionHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

// Set to 1 to send interaction as Timestamp Order (TSO), 0 for Receive Order (RO).
#define SINE_SEND_INTERACTION_TSO 1

/*!
 * @job_class{initialization}
 */
SineInteractionHandler::SineInteractionHandler()
   : name( NULL ),
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
   double send_time )
{
   // Update the time with the simulation time.
   time = send_time;

   ostringstream msg;
   msg << "Interaction from:\"" << ( ( name != NULL ) ? name : "Unknown" ) << "\" "
       << "Send-count:" << ( send_cnt + 1 );

   if ( ( message != NULL ) && TMM_is_alloced( message ) ) {
      TMM_delete_var_a( message );
   }
   message = TMM_strdup( (char *)msg.str().c_str() );

   // Create a User Supplied Tag based off the name in this example.
   RTI1516_USERDATA user_supplied_tag;
   if ( name != NULL ) {
      user_supplied_tag = RTI1516_USERDATA( name, strlen( name ) );
   } else {
      user_supplied_tag = RTI1516_USERDATA( 0, 0 );
   }

   // Get the HLA granted time and lookahead time.
   double hla_granted_time = get_granted_fed_time().get_double_time();
   double lookahead_time   = get_fed_lookahead().get_double_time();

   // Calculate the timestamp we will use to send the interaction in Timestamp
   // Order by using the HLA granted time and the lookahead time.
   // double timestamp = hla_granted_time + lookahead_time;
   double timestamp = time + lookahead_time; //DANNY2.7 use sim time because granted time may be behind a frame

#if SINE_SEND_INTERACTION_TSO
   // Notify the parent interaction handler to send the interaction using
   // Timestamp Order (TSO) at the current simulation time plus the
   // lookahead_time.
   bool was_sent = this->InteractionHandler::send_interaction( timestamp, user_supplied_tag );
#else
   // Notify the parent interaction handler to send the interaction using
   // Receive Order (RO).
   bool was_sent = this->InteractionHandler::send_interaction( user_supplied_tag );
#endif

   if ( was_sent ) {
      if ( should_print( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         string user_supplied_tag_string;
         StringUtilities::to_string( user_supplied_tag_string, user_supplied_tag );

         cout << "++++SENDING++++ SineInteractionHandler::send_sine_interaction("
#if SINE_SEND_INTERACTION_TSO
              << "Timestamp Order)" << endl
#else
              << "Receive Order)" << endl
#endif
              << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'" << endl
              << "  message:'" << ( ( message != NULL ) ? message : "NULL" ) << "'" << endl
              << "  message length:" << ( ( message != NULL ) ? strlen( message ) : 0 ) << endl
              << "  user-supplied-tag:'" << user_supplied_tag_string << "'" << endl
              << "  user-supplied-tag-size:" << user_supplied_tag.size() << endl
              << "  hla_granted_time:" << send_time << " ("
              << Int64Interval::to_microseconds( hla_granted_time ) << " microseconds)" << endl
              << "  send_time:" << send_time << " ("
              << Int64Interval::to_microseconds( send_time ) << " microseconds)" << endl
              << "  lookahead_time:" << lookahead_time << " ("
              << Int64Interval::to_microseconds( lookahead_time ) << " microseconds)" << endl
              << "  timestamp:" << timestamp << " ("
              << Int64Interval::to_microseconds( timestamp ) << " microseconds)" << endl
              << "  time:" << time << endl
              << "  year:" << year << endl
              << "  send_cnt:" << ( send_cnt + 1 ) << endl;
      }

      // Update the send count, which is just used for the message in this example.
      send_cnt++;
   } else {
      // Use the inherited debug-handler to allow debug comments to be turned
      // on and off from a setting in the input file. Use a higher debug level.
      if ( should_print( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         // The interaction was Not sent.
         cout << "+-+-NOT SENT-+-+ SineInteractionHandler::send_sine_interaction()" << endl
              << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'" << endl;
      }
   }
}

void SineInteractionHandler::receive_interaction(
   RTI1516_USERDATA const &the_user_supplied_tag )
{
   receive_cnt++;

   // Convert the HLA User Supplied Tag back into a string we can use.
   string user_tag_string;
   StringUtilities::to_string( user_tag_string, the_user_supplied_tag );

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( should_print( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
      cout << "++++RECEIVING++++ SineInteractionHandler::receive_interaction()" << endl
           << "  name:'" << ( ( name != NULL ) ? name : "NULL" ) << "'" << endl
           << "  message:'" << ( ( message != NULL ) ? message : "NULL" ) << "'" << endl
           << "  message length:" << ( ( message != NULL ) ? strlen( message ) : 0 ) << endl
           << "  user-supplied-tag:'" << user_tag_string << "'" << endl
           << "  user-supplied-tag-size:" << the_user_supplied_tag.size() << endl
           << "  scenario_time:" << get_scenario_time() << endl
           << "  time:" << time << endl
           << "  year:" << year << endl
           << "  receive_cnt:" << receive_cnt << endl;
   }
}
