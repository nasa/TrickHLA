/*!
@file models/sine/src/SineLagCompensation.cpp
@ingroup TrickHLAModel
@brief This class provides lag compensation for the sine wave object.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{sine/src/SineData.cpp}
@trick_link_dependency{sine/src/SineLagCompensation.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2006, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, October 2023, --, Added lag-comp bypass functions.}
@revs_end

*/

// System include files.
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h" // for send_hs
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"

// Model include files.
#include "../include/SineData.hh"
#include "../include/SineLagCompensation.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
SineLagCompensation::SineLagCompensation()
   : SineData(),
     TrickHLA::LagCompensation(),
     sim_data( NULL ),
     time_attr( NULL ),
     value_attr( NULL ),
     dvdt_attr( NULL ),
     phase_attr( NULL ),
     freq_attr( NULL ),
     amp_attr( NULL ),
     tol_attr( NULL ),
     name_attr( NULL ),
     lag_comp_type_str( "Unknown" )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SineLagCompensation::~SineLagCompensation()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void SineLagCompensation::configure(
   SineData *sim_data )
{
   this->sim_data = sim_data;

   return;
}

/*!
 * @job_class{initialization}
 */
void SineLagCompensation::initialize()
{
   // Call the base class initialize function.
   TrickHLA::LagCompensation::initialize();

   return;
}

/*!
 * @details Use the initialize callback function as a way to setup
 * TrickHLA::Attribute references which are used to determine ownership or if
 * data for an attribute was received.
 * @job_class{initialization}
 */
void SineLagCompensation::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   this->LagCompensation::initialize_callback( obj );

   // Get a reference to the TrickHLA-Attribute for all the FOM attributes
   // names. We do this here so that we only do the attribute lookup once
   // instead of looking it up every time the unpack function is called.
   name_attr  = get_attribute_and_validate( "Name" );
   time_attr  = get_attribute_and_validate( "Time" );
   value_attr = get_attribute_and_validate( "Value" );
   dvdt_attr  = get_attribute_and_validate( "dvdt" );
   phase_attr = get_attribute_and_validate( "Phase" );
   freq_attr  = get_attribute_and_validate( "Frequency" );
   amp_attr   = get_attribute_and_validate( "Amplitude" );
   tol_attr   = get_attribute_and_validate( "Tolerance" );

   // To show the effects of ownership transfers on lag-compenstion, get the
   // lag-comp type so that we can display it in the debug messages.
   switch ( obj->lag_comp_type ) {
      case LAG_COMPENSATION_NONE:
         lag_comp_type_str = "LAG_COMPENSATION_NONE";
         break;
      case LAG_COMPENSATION_SEND_SIDE:
         lag_comp_type_str = "LAG_COMPENSATION_SEND_SIDE";
         break;
      case LAG_COMPENSATION_RECEIVE_SIDE:
         lag_comp_type_str = "LAG_COMPENSATION_RECEIVE_SIDE";
         break;
      default:
         lag_comp_type_str = "Unknown";
         break;
   }
}

/*!
 * @brief Send side lag-compensation where we propagate the sine wave state
 *  head by dt to predict the value at the next data cycle.
 */
void SineLagCompensation::send_lag_compensation()
{
   double const dt   = get_lookahead().get_time_in_seconds();
   double const time = get_scenario_time() + dt;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      string obj_name = ( this->object != NULL ) ? this->object->get_name_string() : "";

      cout << "******* SineLagCompensation::send_lag_compensation():" << __LINE__ << endl
           << "    object-name:'" << obj_name << "'" << endl
           << " lag-comp-type:" << lag_comp_type_str << endl
           << " scenario-time:" << setprecision( 18 ) << get_scenario_time() << endl
           << "     data-time:" << setprecision( 18 ) << sim_data->get_time() << endl
           << "            dt:" << setprecision( 18 ) << dt << endl
           << " adjusted-time:" << setprecision( 18 ) << time << endl;
   }

   // Copy the current sine state over to the predicted sine state.
   this->set_name( sim_data->get_name() );
   this->set_value( sim_data->get_value() );
   this->set_derivative( sim_data->get_derivative() );
   this->set_phase( sim_data->get_phase() );
   this->set_frequency( sim_data->get_frequency() );
   this->set_amplitude( sim_data->get_amplitude() );
   this->set_tolerance( sim_data->get_tolerance() );

   this->set_time( time );

   this->compute_value( time );
   this->compute_derivative( time );
}

/*!
 * @brief When lag compensation is disabled, this function is called to
 * bypass the send side lag compensation and your implementation must copy
 * the sim-data to the lag-comp data to effect the bypass.
 */
void SineLagCompensation::bypass_send_lag_compensation()
{
   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      string obj_name = ( this->object != NULL ) ? this->object->get_name_string() : "";

      cout << "******* SineLagCompensation::bypass_send_lag_compensation():" << __LINE__ << endl
           << "   object-name:'" << obj_name << "'" << endl
           << " lag-comp-type:" << lag_comp_type_str << endl
           << " scenario-time:" << setprecision( 18 ) << get_scenario_time() << endl
           << "     data-time:" << setprecision( 18 ) << sim_data->get_time() << endl;
   }

   // Bypass send lag compensation by copying the current sim-data to the
   // lag-comp data structure. We need to ensure the lac-comp data structure
   // is updated to ensure any downstream calculations still get data.
   this->set_name( sim_data->get_name() );
   this->set_time( sim_data->get_time() );
   this->set_value( sim_data->get_value() );
   this->set_derivative( sim_data->get_derivative() );
   this->set_phase( sim_data->get_phase() );
   this->set_frequency( sim_data->get_frequency() );
   this->set_amplitude( sim_data->get_amplitude() );
   this->set_tolerance( sim_data->get_tolerance() );
}

/*!
 * @brief Receive side lag-compensation where we propagate the sine wave
 * state ahead by dt to predict the value at the next data cycle.
 */
void SineLagCompensation::receive_lag_compensation()
{
   double const time = get_scenario_time();

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      string obj_name = ( this->object != NULL ) ? this->object->get_name_string() : "";

      cout << "******* SineLagCompensation::receive_lag_compensation():" << __LINE__ << endl
           << "   object-name:'" << obj_name << "'" << endl
           << " lag-comp-type:" << lag_comp_type_str << endl
           << " scenario-time:" << setprecision( 18 ) << get_scenario_time() << endl;
      if ( time_attr->is_received() ) {
         double const dt = time - this->get_time();
         cout << "     data-time:" << setprecision( 18 ) << this->get_time() << " Received Update" << endl
              << "            dt:" << setprecision( 18 ) << dt << endl;
      } else {
         cout << "     data-time:" << setprecision( 18 ) << this->get_time() << " Stale: No Update Received!" << endl
              << "            dt: Invalid - No Time Received!" << endl;
      }
      cout << " adjusted-time:" << setprecision( 18 ) << time << endl
           << " BEFORE Lag Compensation:" << endl
           << "\t Name  lag-comp: '" << this->get_name()
           << "', received update:" << ( name_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Time  lag-comp: " << setprecision( 18 ) << this->get_time()
           << ", received update:" << ( time_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Value lag-comp: " << this->get_value()
           << ", received update:" << ( value_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t dvdt  lag-comp: " << this->get_derivative()
           << ", received update:" << ( dvdt_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Phase lag-comp: " << this->get_phase()
           << ", received update:" << ( phase_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Amp   lag-comp: " << this->get_amplitude()
           << ", received update:" << ( amp_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Freq  lag-comp: " << this->get_frequency()
           << ", received update:" << ( freq_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Tol   lag-comp: " << this->get_tolerance()
           << ", received update:" << ( tol_attr->is_received() ? "Yes" : "No" ) << endl;
   }

   // If the HLA time attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value. If we locally own the attribute then we do not want to
   // override it's value. If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state. We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.
   // Because of ownership transfers and attributes being sent at different
   // rates we need to check to see if we received data for each attribute.
   if ( name_attr->is_received() ) {
      sim_data->set_name( this->get_name() );
   }
   if ( value_attr->is_received() ) {
      sim_data->set_value( this->get_value() );
   }
   if ( dvdt_attr->is_received() ) {
      sim_data->set_derivative( this->get_derivative() );
   }
   if ( phase_attr->is_received() ) {
      sim_data->set_phase( this->get_phase() );
   }
   if ( freq_attr->is_received() ) {
      sim_data->set_frequency( this->get_frequency() );
   }
   if ( amp_attr->is_received() ) {
      sim_data->set_amplitude( this->get_amplitude() );
   }
   if ( tol_attr->is_received() ) {
      sim_data->set_tolerance( this->get_tolerance() );
   }

   sim_data->set_time( time );

   // Do the lag compensation by computing values to the time.
   sim_data->compute_value( time );
   sim_data->compute_derivative( time );

   sim_data->adjust_phase( time );

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      cout << "SineLagCompensation::receive_lag_compensation():" << __LINE__ << endl
           << " AFTER LAG COMPENSATION:" << endl
           << "\t Name  sim_data: '" << sim_data->get_name() << "'" << endl
           << "\t Time  sim_data: " << setprecision( 18 ) << sim_data->get_time() << endl
           << "\t Value sim_data: " << sim_data->get_value() << endl
           << "\t dvdt  sim_data: " << sim_data->get_derivative() << endl
           << "\t Phase sim_data: " << sim_data->get_phase() << endl
           << "\t Amp   sim_data: " << sim_data->get_amplitude() << endl
           << "\t Freq  sim_data: " << sim_data->get_frequency() << endl
           << "\t Tol   sim_data: " << sim_data->get_tolerance() << endl;
   }
}

/*!
 * @brief When lag compensation is disabled, this function is called to
 * bypass the receive side lag compensation and your implementation must
 * copy the lag-comp data to the sim-data to effect the bypass. You must
 * make sure to check the lag-comp data was received before copying to
 * the sim-data otherwise you will be copying stale data.
 */
void SineLagCompensation::bypass_receive_lag_compensation()
{
   double const time = get_scenario_time();

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      string obj_name = ( this->object != NULL ) ? this->object->get_name_string() : "";

      cout << "******* SineLagCompensation::bypass_receive_lag_compensation():" << __LINE__ << endl
           << "   object-name:'" << obj_name << "'" << endl
           << " lag-comp-type:" << lag_comp_type_str << endl
           << " scenario-time:" << setprecision( 18 ) << get_scenario_time() << endl;
      if ( time_attr->is_received() ) {
         double const dt = time - this->get_time();
         cout << "     data-time:" << setprecision( 18 ) << this->get_time() << " Received Update" << endl
              << "            dt:" << setprecision( 18 ) << dt << endl;
      } else {
         cout << "     data-time:" << setprecision( 18 ) << this->get_time() << " Stale: No Update Received!" << endl
              << "            dt: Invalid - No Time Received!" << endl;
      }
      cout << " BEFORE Bypassing Lag Compensation:" << endl
           << "\t Name  lag-comp: '" << this->get_name()
           << "', received update:" << ( name_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Time  lag-comp: " << setprecision( 18 ) << this->get_time()
           << ", received update:" << ( time_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Value lag-comp: " << this->get_value()
           << ", received update:" << ( value_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t dvdt  lag-comp: " << this->get_derivative()
           << ", received update:" << ( dvdt_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Phase lag-comp: " << this->get_phase()
           << ", received update:" << ( phase_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Amp   lag-comp: " << this->get_amplitude()
           << ", received update:" << ( amp_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Freq  lag-comp: " << this->get_frequency()
           << ", received update:" << ( freq_attr->is_received() ? "Yes" : "No" ) << endl

           << "\t Tol   lag-comp: " << this->get_tolerance()
           << ", received update:" << ( tol_attr->is_received() ? "Yes" : "No" ) << endl;
   }

   // If the HLA time attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value. If we locally own the attribute then we do not want to
   // override it's value. If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state. We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.
   // Because of ownership transfers and attributes being sent at different
   // rates we need to check to see if we received data for each attribute.

   // Bypass receive lag compensation by copying lag-comp data to sim-data.
   if ( name_attr->is_received() ) {
      sim_data->set_name( this->get_name() );
   }
   if ( time_attr->is_received() ) {
      sim_data->set_time( this->get_time() );
   }
   if ( value_attr->is_received() ) {
      sim_data->set_value( this->get_value() );
   }
   if ( dvdt_attr->is_received() ) {
      sim_data->set_derivative( this->get_derivative() );
   }
   if ( phase_attr->is_received() ) {
      sim_data->set_phase( this->get_phase() );
   }
   if ( freq_attr->is_received() ) {
      sim_data->set_frequency( this->get_frequency() );
   }
   if ( amp_attr->is_received() ) {
      sim_data->set_amplitude( this->get_amplitude() );
   }
   if ( tol_attr->is_received() ) {
      sim_data->set_tolerance( this->get_tolerance() );
   }

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      cout << "SineLagCompensation::bypass_receive_lag_compensation():" << __LINE__ << endl
           << " AFTER BYPASSING LAG COMPENSATION:" << endl
           << "\t Name  sim_data: '" << sim_data->get_name() << "'" << endl
           << "\t Time  sim_data: " << setprecision( 18 ) << sim_data->get_time() << endl
           << "\t Value sim_data: " << sim_data->get_value() << endl
           << "\t dvdt  sim_data: " << sim_data->get_derivative() << endl
           << "\t Phase sim_data: " << sim_data->get_phase() << endl
           << "\t Amp   sim_data: " << sim_data->get_amplitude() << endl
           << "\t Freq  sim_data: " << sim_data->get_frequency() << endl
           << "\t Tol   sim_data: " << sim_data->get_tolerance() << endl;
   }
}
