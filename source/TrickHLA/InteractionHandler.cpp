/*!
@file TrickHLA/InteractionHandler.cpp
@ingroup TrickHLA
@brief This class is the abstract base class for handling HLA interactions.

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
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Int64Interval.cpp}
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{Interaction.cpp}
@trick_link_dependency{InteractionHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, April 2016, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <iostream>
#include <limits>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/InteractionHandler.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
InteractionHandler::InteractionHandler() // RETURN: -- None.
   : interaction( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
InteractionHandler::~InteractionHandler() // RETURN: -- None.
{
   return;
}

/*!
 * @job_class{initialization}
 */
void InteractionHandler::initialize_callback(
   Interaction *inter )
{
   this->interaction = inter;
}

bool InteractionHandler::send_interaction()
{
   return ( ( interaction != NULL ) ? interaction->send( RTI1516_USERDATA( 0, 0 ) )
                                    : false );
}

bool InteractionHandler::send_interaction(
   RTI1516_USERDATA const &the_user_supplied_tag )
{
   return ( ( interaction != NULL ) ? interaction->send( the_user_supplied_tag )
                                    : false );
}

bool InteractionHandler::send_interaction(
   double send_HLA_time )
{
   return ( ( interaction != NULL ) ? interaction->send( send_HLA_time, RTI1516_USERDATA( 0, 0 ) )
                                    : false );
}

bool InteractionHandler::send_interaction(
   double                  send_HLA_time,
   RTI1516_USERDATA const &the_user_supplied_tag )
{
   return ( ( interaction != NULL ) ? interaction->send( send_HLA_time, the_user_supplied_tag )
                                    : false );
}

void InteractionHandler::receive_interaction(
   RTI1516_USERDATA const &the_user_supplied_tag )
{
   message_publish( MSG_NORMAL, "InteractionHandler::receive_interaction():%d \n",
                    __LINE__ );
}

Int64Interval InteractionHandler::get_lookahead() const
{
   Int64Interval di;
   if ( interaction != NULL ) {
      di = interaction->get_lookahead();
   } else {
      di = Int64Interval( -1.0 );
   }
   return di;
}

Int64Time InteractionHandler::get_granted_time() const
{
   Int64Time dt;
   if ( interaction != NULL ) {
      dt = interaction->get_granted_time();
   } else {
      dt = Int64Time( Int64BaseTime::get_max_logical_time_in_seconds() );
   }
   return dt;
}

double InteractionHandler::get_sim_time()
{
   if ( ( interaction != NULL ) && ( interaction->get_federate() != NULL ) ) {
      ExecutionControlBase *execution_control = interaction->get_federate()->get_execution_control();
      return ( execution_control->get_sim_time() );
   }
   return -std::numeric_limits< double >::max();
}

double InteractionHandler::get_scenario_time()
{
   if ( ( interaction != NULL ) && ( interaction->get_federate() != NULL ) ) {
      ExecutionControlBase *execution_control = interaction->get_federate()->get_execution_control();
      return ( execution_control->get_scenario_time() );
   }
   return -std::numeric_limits< double >::max();
}

double InteractionHandler::get_cte_time()
{
   if ( ( interaction != NULL ) && ( interaction->get_federate() != NULL ) ) {
      ExecutionControlBase *execution_control = interaction->get_federate()->get_execution_control();
      if ( execution_control->does_cte_timeline_exist() ) {
         return execution_control->get_cte_time();
      }
   }
   return -std::numeric_limits< double >::max();
}
