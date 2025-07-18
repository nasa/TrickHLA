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
@trick_link_dependency{Parameter.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, April 2016, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System includes.
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>

// Trick includes.
#include <trick/message_proto.h>
#include <trick/message_type.h>

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/InteractionHandler.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/StandardsSupport.hh"

// HLA includes.
#include "RTI/VariableLengthData.h"

using namespace std;
using namespace TrickHLA;
using namespace RTI1516_NAMESPACE;

/*!
 * @job_class{initialization}
 */
InteractionHandler::InteractionHandler() // RETURN: -- None.
   : initialized( false ),
     interaction( NULL )
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

/*!
 * @job_class{default_data}
 */
void InteractionHandler::set_interaction( TrickHLA::Interaction *inter )
{
   // Check for initialization.
   if ( initialized ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::InteractionHandler::set_interaction():" << __LINE__
             << " ERROR: The initialize() function has already been called\n";
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Assign the object.
   this->interaction = inter;

   return;
}

bool InteractionHandler::send_interaction()
{
   return ( ( interaction != NULL )
               ? interaction->send( VariableLengthData( NULL, 0 ) )
               : false );
}

bool InteractionHandler::send_interaction(
   VariableLengthData const &the_user_supplied_tag )
{
   return ( ( interaction != NULL )
               ? interaction->send( the_user_supplied_tag )
               : false );
}

bool InteractionHandler::send_interaction(
   double send_HLA_time )
{
   return ( ( interaction != NULL )
               ? interaction->send( send_HLA_time, VariableLengthData( NULL, 0 ) )
               : false );
}

bool InteractionHandler::send_interaction(
   double                    send_HLA_time,
   VariableLengthData const &the_user_supplied_tag )
{
   return ( ( interaction != NULL )
               ? interaction->send( send_HLA_time, the_user_supplied_tag )
               : false );
}

void InteractionHandler::receive_interaction(
   VariableLengthData const &the_user_supplied_tag )
{
   message_publish( MSG_NORMAL, "InteractionHandler::receive_interaction():%d \n",
                    __LINE__ );
}

Int64Interval InteractionHandler::get_lookahead() const
{
   return ( ( interaction != NULL ) ? interaction->get_lookahead() : Int64Interval( -1.0 ) );
}

Int64Time InteractionHandler::get_granted_time() const
{
   return ( ( interaction != NULL )
               ? interaction->get_granted_time()
               : Int64Time( Int64BaseTime::get_max_logical_time_in_seconds() ) );
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

/*!
 * @brief Get the Parameter by FOM name.
 * @return Parameter for the given name.
 * @param param_FOM_name Parameter FOM name.
 */
Parameter *InteractionHandler::get_parameter(
   string const &param_FOM_name )
{
   return interaction->get_parameter( param_FOM_name );
}

/*!
 * @brief This function returns the Parameter for the given parameter FOM name.
 * @return Parameter for the given name.
 * @param param_FOM_name Parameter FOM name.
 */
Parameter *InteractionHandler::get_parameter_and_validate(
   string const &param_FOM_name )
{
   // Make sure the FOM name is not NULL.
   if ( param_FOM_name.empty() ) {
      ostringstream errmsg;
      errmsg << "InteractionHandler::get_parameter_and_validate():" << __LINE__
             << " ERROR: Unexpected NULL parameter FOM name specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Get the parameter by FOM name.
   Parameter *param = get_parameter( param_FOM_name );

   // Make sure we have found the parameter.
   if ( param == NULL ) {
      ostringstream errmsg;
      errmsg << "InteractionHandler::get_parameter_and_validate():" << __LINE__
             << " ERROR: For FOM interaction '" << interaction->get_FOM_name()
             << "', failed to find the TrickHLA Parameter for an parameter named"
             << " '" << param_FOM_name << "'. Make sure the FOM parameter name is"
             << " correct, the FOM contains an parameter named '"
             << param_FOM_name << "' and that your input.py file is properly"
             << " configured for this parameter.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return param;
}
