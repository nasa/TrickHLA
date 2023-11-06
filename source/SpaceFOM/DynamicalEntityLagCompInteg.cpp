/*!
@file SpaceFOM/DynamicalEntityLagCompInteg.cpp
@ingroup SpaceFOM
@brief This class provides the implementation for a TrickHLA SpaceFOM
physical entity latency/lag compensation class that uses integration to
compensate the state.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{DynamicalEntityLagCompInteg.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, October 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Attribute.hh"

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityLagCompInteg.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityLagCompInteg::DynamicalEntityLagCompInteg( DynamicalEntityBase & entity_ref ) // RETURN: -- None.
   : DynamicalEntityLagCompBase( entity_ref ),
     integ_t( 0.0 ),
     integ_dt( 0.05 ),
     integ_tol( 1.0e-8 )
{

}


/*!
 * @job_class{shutdown}
 */
DynamicalEntityLagCompInteg::~DynamicalEntityLagCompInteg() // RETURN: -- None.
{
}


/*!
 * @job_class{initialization}
 */
void DynamicalEntityLagCompInteg::initialize()
{
   ostringstream errmsg;

   if ( this->integ_dt < this->integ_tol ) {

      errmsg << "SpaceFOM::DynamicalEntityLagCompInteg::initialize():" << __LINE__<< endl
             << " ERROR: Tolerance must be less that the dt!: dt = "
             << this->integ_dt << "; tolerance = " << this->integ_tol << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );

   }

   // Call the base class initialize routine.
   DynamicalEntityLagCompBase::initialize();

   // Return to calling routine.
   return;
}


/*! @brief Sending side latency compensation callback interface from the
 *  TrickHLALagCompensation class. */
void DynamicalEntityLagCompInteg::send_lag_compensation()
{
   double begin_t = get_scenario_time();
   double end_t;

   // Save the compensation time step.
   this->compensate_dt = get_lookahead().get_time_in_seconds();
   end_t = begin_t + this->compensate_dt;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      cout << "******* DynamicalEntityLagCompInteg::send_lag_compensation():" << __LINE__ << endl
           << " scenario-time:" << get_scenario_time() << endl
           << "     lookahead:" << this->compensate_dt << endl
           << " adjusted-time:" << end_t << endl;
   }

   // Copy the current DynamicalEntity state over to the lag compensated state.
   this->entity.pack_from_working_data();
   this->load_lag_comp_data();
   this->Q_dot.derivative_first( this->lag_comp_data.att,
                                 this->lag_comp_data.ang_vel );

   // Print out debug information if desired.
   if ( debug ) {
      cout << "Send data before compensation: " << endl;
      this->print_lag_comp_data();
   }

   // Compensate the data
   this->compensate( begin_t, end_t );

   // Print out debug information if desired.
   if ( debug ) {
      cout << "Send data after compensation: " << endl;
      this->print_lag_comp_data();
   }

   // Copy the compensated state to the packing data.
   this->unload_lag_comp_data();

   // Return to calling routine.
   return;
}


/*! @brief Receive side latency compensation callback interface from the
 *  TrickHLALagCompensation class. */
void DynamicalEntityLagCompInteg::receive_lag_compensation()
{
   double end_t  = get_scenario_time();
   double data_t = entity.get_time();

   // Save the compensation time step.
   this->compensate_dt = end_t - data_t;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      cout << "******* DynamicalEntityLagCompInteg::receive_lag_compensation():" << __LINE__ << endl
           << "  scenario-time:" << end_t  << endl
           << "      data-time:" << data_t << endl
           << " comp-time-step:" << this->compensate_dt  << endl;
   }

   // Because of ownership transfers and attributes being sent at different
   // rates we need to check to see if we received attribute data.
   if ( this->state_attr->is_received() ) {

      // Copy the current DynamicalEntity state over to the lag compensated state.
      this->load_lag_comp_data();
      this->Q_dot.derivative_first( this->lag_comp_data.att,
                                    this->lag_comp_data.ang_vel );

      // Print out debug information if desired.
      if ( debug ) {
         cout << "Receive data before compensation: " << endl;
         this->print_lag_comp_data();
      }

      // Compensate the data
      this->compensate( data_t, end_t );

      // Print out debug information if desired.
      if ( debug ) {
         cout << "Receive data after compensation: " << endl;
         this->print_lag_comp_data();
      }

   }
   else {
      if ( debug ) {
         cout << "DynamicalEntityLagCompInteg::receive_lag_compensation(): No state data received." << endl;
      }
   }

   // Copy the compensated state to the packing data.
   this->unload_lag_comp_data();

   // Move the unpacked data into the working data.
   this->entity.unpack_into_working_data();

   // Return to calling routine.
   return;
}
