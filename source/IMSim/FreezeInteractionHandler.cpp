/*!
@file IMSim/FreezeInteractionHandler.cpp
@ingroup IMSim
@brief This class handles the HLA Freeze interactions.

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
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Federate.cpp}
@trick_link_dependency{../TrickHLA/Int64BaseTime.cpp}
@trick_link_dependency{../TrickHLA/Int64Interval.cpp}
@trick_link_dependency{../TrickHLA/Int64Time.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{FreezeInteractionHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, DSES, July 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cmath>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/Types.hh"

// IMSim include files.
#include "IMSim/ExecutionControl.hh"
#include "IMSim/FreezeInteractionHandler.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA Encoder helper includes.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;
using namespace IMSim;

#define THLA_FREEZE_INTERACTION_DEBUG 0

/*!
 * @job_class{initialization}
 */
FreezeInteractionHandler::FreezeInteractionHandler() // RETURN: -- None.
   : execution_control( NULL ),
     time( 0.0 )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
FreezeInteractionHandler::~FreezeInteractionHandler() // RETURN: -- None.
{
   return;
}

/*!
 *  @details The interaction time is computed to be the current granted time
 *  plus lookahead. If the supplied time does not fall on the HLA and lookahead
 *  time boundaries, the time is rounded up to the next highest integer
 *  multiple of the lookahead time. If the time is less than the granted time
 *  plus one lookahead, this means that the freeze time is invalid so it is
 *  updated to be 'granted time plus one lookahead'. If the federate is a
 *  late-joining federate, the freeze time is slipped one frame because it
 *  would occur in a previous frame when the other non-late-joiner federates
 *  receive the freeze interaction time.
 */
void FreezeInteractionHandler::send_scenario_freeze_interaction(
   double &freeze_time,
   bool    late_joining_federate )
{
   // if the interaction was not initialized in the parent class, get out of here...
   if ( interaction == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::FreezeInteractionHandler::send_scenario_freeze_interaction("
             << freeze_time << ", " << (int)late_joining_federate << "):" << __LINE__
             << " ERROR: 'interaction' was not initialized to callback an"
             << " Interaction class. Cannot send out an interaction in"
             << " order for the rest of the federates to participate in a"
             << " federation freeze.\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      return;
   }

#if THLA_FREEZE_INTERACTION_DEBUG
   ostringstream msg;
   msg << "IMSim::FreezeInteractionHandler::send_scenario_freeze_interaction():" << __LINE__
       << " ===> debug <===" << endl
       << " granted-time:" << interaction->get_granted_time().get_time_in_seconds() << endl
       << " lookahead-time:" << interaction->get_lookahead().get_time_in_seconds() << endl;
   message_publish( MSG_NORMAL, msg.str().c_str() );
#endif

   /// We will calculate two important times related to the freeze scenario time:
   /// -# The HLA logical timestamp the freeze interaction is sent for (TSO),
   ///    which is the interaction_hla_time variable.
   /// -# The Freeze scenario time that is an integer multiple of the lookahead
   ///    and is >= the equivalent freeze interaction scenario time from step 1,
   ///    which is the freeze_scenario_time variable.
   ///
   /// We need to find the equivalent simulation-time and HLA-time for a
   /// given freeze scenario-time so that we can do the correct time
   /// comparisons. Also, if we are a late joining federate the sim-time
   /// and HLA-time will not be aligned as shown in this example.
   ///\verbatim
   ///      HLA-time |-------------------------|-------------------------|
   ///               101.0                     102.0                     103.0
   ///
   /// Scenario-time |-------------------------|-------------------------|
   ///               March 2, 2032 @ 19:20:07  March 2, 2032 @ 19:20:08  March 2, 2032 @ 19:20:09
   ///
   ///      Sim-time |-------------------------|-------------------------|
   ///               0.0                       1.0                       2.0
   ///\endverbatim
   /// Scenario-time and Sim-time change at the same rate but they have
   /// different starting epochs.\n
   /// freeze-sim-time = current-sim-time + (freeze-scenario-time - current-scenario-time)\n
   /// freeze-hla-time = granted-hla-time + (freeze-scenario-time - current-scenario-time)

   /// We must wait for a valid Time Advance Grant (TAG) so the HLA granted time
   /// will be valid and with proper alignment with the scenario and simulation
   /// timelines. NOTE: We can only do the blocking wait for the time advance
   /// grant here because this is an end_of_frame job and if we don't have a
   /// granted time then we are at the end of the frame that made the TAR call.
   /// The wait for Time Advance Grant will be at the top of the next frame.
   if ( !interaction->get_federate()->is_time_advance_granted() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         message_publish( MSG_NORMAL, "IMSim::FreezeInteractionHandler::send_scenario_freeze_interaction():%d Waiting for HLA Time Advance Grant (TAG).\n",
                          __LINE__ );
      }
      interaction->get_federate()->wait_for_time_advance_grant();
   }

   Int64Interval lookahead              = interaction->get_lookahead();
   Int64Time     granted                = interaction->get_granted_time();
   Int64Time     granted_plus_lookahead = granted + lookahead;

   double curr_scenario_time   = interaction->get_manager()->get_execution_control()->get_scenario_time();
   double freeze_scenario_time = freeze_time;
   double freeze_hla_time      = granted.get_time_in_seconds() + ( freeze_scenario_time - curr_scenario_time );

   // The freeze interaction will go out as soon as possible even if the
   // federation freeze time is further out in the future. This is the HLA
   // timestamp the interaction is sent for.
   Int64Time interaction_hla_time = granted_plus_lookahead;

   // If we are late joining federate, adjust the HLA time the freeze
   // interaction will go out by one more lookahead time.
   if ( late_joining_federate ) {
      interaction_hla_time += lookahead;

      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         message_publish( MSG_NORMAL, "IMSim::FreezeInteractionHandler::send_scenario_freeze_interaction():%d \
Late joining federate, Freeze Interaction will now be sent for HLA time:%lf \n",
                          __LINE__, interaction_hla_time.get_time_in_seconds() );
      }
   }

   // To avoid possible round off errors, do the math using the native 64-bit
   // integer type.
   Int64Time interation_time_plus_lookahead = interaction_hla_time + lookahead;

   // Make sure the time we freeze the federation (on the HLA timeline) is
   // greater than the HLA time the interaction will go out on (TSO).
   if ( freeze_hla_time < interation_time_plus_lookahead.get_time_in_seconds() ) {

      // Update the time to the earliest time that we can freeze the federation,
      // which is one lookahead frame after the freeze interaction goes out.
      freeze_hla_time = interation_time_plus_lookahead.get_time_in_seconds();

      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         // Recalculate the freeze scenario time from the updated freeze HLA time.
         freeze_scenario_time = curr_scenario_time + ( freeze_hla_time - granted.get_time_in_seconds() );

         ostringstream infomsg;
         infomsg << "IMSim::FreezeInteractionHandler::send_scenario_freeze_interaction():" << __LINE__ << '\n'
                 << "  Invalid freeze scenario time:" << freeze_time << '\n'
                 << "  Current scenario time:" << curr_scenario_time << '\n'
                 << "  Updated Freeze scenario time:" << freeze_scenario_time << '\n'
                 << "  Freeze federation at HLA time:" << freeze_hla_time << '\n'
                 << "  Freeze Interaction sent for HLA time:" << interaction_hla_time.get_time_in_seconds() << '\n'
                 << "  Current granted HLA time:" << granted.get_time_in_seconds() << '\n';
         message_publish( MSG_NORMAL, infomsg.str().c_str() );
      }
   }

   // Make sure the freeze HLA time is an integer multiple of the lookahead.
   if ( lookahead > 0.0 ) {
      double freeze_t = trunc( freeze_hla_time / lookahead.get_time_in_seconds() ) * lookahead.get_time_in_seconds();
      if ( freeze_hla_time > freeze_t ) {
         freeze_hla_time = freeze_t + lookahead.get_time_in_seconds();

         if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
            message_publish( MSG_NORMAL, "IMSim::FreezeInteractionHandler::send_scenario_freeze_interaction():%d \
Freeze HLA time is not an integer multiple of the lookahead time:%lf, using \
new freeze HLA time:%lf \n",
                             __LINE__, lookahead.get_time_in_seconds(),
                             freeze_hla_time );
         }
      }
   }

   // Recalculate the freeze scenario time from the updated freeze HLA time.
   freeze_scenario_time = curr_scenario_time + ( freeze_hla_time - granted.get_time_in_seconds() );

   // Make sure we update the passed in time so we pass back the right value.
   freeze_time = freeze_scenario_time;

   // Capture the current federation save time attribute value for the interaction.
   this->time = freeze_time;

   // Notify the parent interaction handler to send the interaction using
   // Timestamp Order at the earliest convenience, even if the federation is to
   // freeze in the future...
   if ( InteractionHandler::send_interaction( interaction_hla_time.get_time_in_seconds() ) ) {
      ostringstream infomsg;
      infomsg << "IMSim::FreezeInteractionHandler::send_scenario_freeze_interaction(Timestamp Order):"
              << __LINE__ << '\n'
              << "  Freeze Interaction sent TSO at HLA time:" << interaction_hla_time.get_time_in_seconds() << " ("
              << interaction_hla_time.get_base_time() << " " << Int64BaseTime::get_units()
              << ")\n"
              << "  Federation Freeze scenario time:" << time << " ("
              << Int64BaseTime::to_base_time( time ) << " " << Int64BaseTime::get_units()
              << ")\n"
              << "  Federation Freeze HLA time:" << freeze_hla_time << " ("
              << freeze_hla_time << " " << Int64BaseTime::get_units()
              << ")\n";
      message_publish( MSG_NORMAL, infomsg.str().c_str() );

      // Inform the Federate the scenario time to freeze the simulation on.
      execution_control->add_freeze_scenario_time( time );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_INTERACTION ) ) {
         message_publish( MSG_NORMAL, "IMSim::FreezeInteractionHandler::send_scenario_freeze_interaction()%d: Federation freeze scenario time:%lf \n",
                          __LINE__, time );
      }
   } else {
      // The interaction was Not sent.
      ostringstream infomsg;
      infomsg << "IMSim::FreezeInteractionHandler::send_scenario_freeze_interaction(Timestamp Order):"
              << __LINE__ << " ERROR: Freeze Interaction Not Sent\n"
              << "  Freeze Interaction sent TSO at HLA time:" << interaction_hla_time.get_time_in_seconds() << " ("
              << interaction_hla_time.get_base_time() << " " << Int64BaseTime::get_units() << ")\n"
              << "  Federation Freeze scenario time:" << time << " ("
              << Int64BaseTime::to_base_time( time ) << " " << Int64BaseTime::get_units() << ")\n"
              << "  Federation Freeze HLA time:" << freeze_hla_time << " ("
              << freeze_hla_time << " " << Int64BaseTime::get_units()
              << ")\n";
      message_publish( MSG_NORMAL, infomsg.str().c_str() );
   }
}

void FreezeInteractionHandler::receive_interaction(
   RTI1516_USERDATA const &theUserSuppliedTag )
{
   ostringstream msg;
   msg << "IMSim::FreezeInteractionHandler::receive_interaction():"
       << __LINE__ << '\n'
       << "  Freeze scenario-time:" << time << " ("
       << Int64BaseTime::to_base_time( time ) << " " << Int64BaseTime::get_units()
       << ")\n";
   message_publish( MSG_NORMAL, msg.str().c_str() );

   // if the interaction was not initialized into the parent class, get out of here...
   if ( interaction == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::FreezeInteractionHandler::receive_interaction():"
             << __LINE__ << " ERROR:"
             << " 'interaction' was not initialized to callback an Interaction"
             << " class. Cannot send the time to the Interaction in order for it to"
             << " participate in a federation freeze.\n";
      message_publish( MSG_NORMAL, errmsg.str().c_str() );
   } else {
      // Inform the Federate the scenario time to freeze the simulation on.
      execution_control->add_freeze_scenario_time( time );

#if THLA_FREEZE_INTERACTION_DEBUG
      ostringstream infomsg;
      infomsg << "IMSim::FreezeInteractionHandler::receive_interaction():"
              << __LINE__
              << " ===> debug <===" << endl
              << " granted-time:" << interaction->get_granted_time().get_time_in_seconds() << endl
              << " lookahead-time:" << interaction->get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_NORMAL, infomsg.str().c_str() );
#endif
   }
}
