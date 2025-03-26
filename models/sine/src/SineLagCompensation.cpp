/*!
@ingroup Sine
@file models/sine/src/SineLagCompensation.cpp
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
#include "trick/message_proto.h"
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
   LagCompensation::initialize_callback( obj );

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
      string obj_name = ( this->object != NULL ) ? object->get_name_string() : "";

      ostringstream msg;
      msg << "******* SineLagCompensation::send_lag_compensation():" << __LINE__ << '\n'
          << "    object-name:'" << obj_name << "'\n"
          << " lag-comp-type:" << lag_comp_type_str << '\n'
          << " scenario-time:" << setprecision( 18 ) << get_scenario_time() << '\n'
          << "     data-time:" << setprecision( 18 ) << sim_data->get_time() << '\n'
          << "            dt:" << setprecision( 18 ) << dt << '\n'
          << " adjusted-time:" << setprecision( 18 ) << time << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Copy the current sine state over to the predicted sine state.
   set_name( sim_data->get_name() );
   set_value( sim_data->get_value() );
   set_derivative( sim_data->get_derivative() );
   set_phase( sim_data->get_phase() );
   set_frequency( sim_data->get_frequency() );
   set_amplitude( sim_data->get_amplitude() );
   set_tolerance( sim_data->get_tolerance() );

   set_time( time );

   compute_value( time );
   compute_derivative( time );
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
      string obj_name = ( this->object != NULL ) ? object->get_name_string() : "";

      ostringstream msg;
      msg << "******* SineLagCompensation::bypass_send_lag_compensation():" << __LINE__ << '\n'
          << "   object-name:'" << obj_name << "'\n"
          << " lag-comp-type:" << lag_comp_type_str << '\n'
          << " scenario-time:" << setprecision( 18 ) << get_scenario_time() << '\n'
          << "     data-time:" << setprecision( 18 ) << sim_data->get_time() << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Bypass send lag compensation by copying the current sim-data to the
   // lag-comp data structure. We need to ensure the lac-comp data structure
   // is updated to ensure any downstream calculations still get data.
   set_name( sim_data->get_name() );
   set_time( sim_data->get_time() );
   set_value( sim_data->get_value() );
   set_derivative( sim_data->get_derivative() );
   set_phase( sim_data->get_phase() );
   set_frequency( sim_data->get_frequency() );
   set_amplitude( sim_data->get_amplitude() );
   set_tolerance( sim_data->get_tolerance() );
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
      string obj_name = ( this->object != NULL ) ? object->get_name_string() : "";

      ostringstream msg;
      msg << "******* SineLagCompensation::receive_lag_compensation():" << __LINE__ << '\n'
          << "   object-name:'" << obj_name << "'\n"
          << " lag-comp-type:" << lag_comp_type_str << '\n'
          << " scenario-time:" << setprecision( 18 ) << get_scenario_time() << '\n';
      if ( time_attr->is_received() ) {
         double const dt = time - get_time();
         msg << "     data-time:" << setprecision( 18 ) << get_time() << " Received Update\n"
             << "            dt:" << setprecision( 18 ) << dt << '\n';
      } else {
         msg << "     data-time:" << setprecision( 18 ) << get_time() << " Stale: No Update Received!\n"
             << "            dt: Invalid - No Time Received!\n";
      }
      msg << " adjusted-time:" << setprecision( 18 ) << time << '\n'
          << " BEFORE Lag Compensation:\n"
          << "\t Name  lag-comp: '" << get_name()
          << "', received update:" << ( name_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Time  lag-comp: " << setprecision( 18 ) << get_time()
          << ", received update:" << ( time_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Value lag-comp: " << get_value()
          << ", received update:" << ( value_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t dvdt  lag-comp: " << get_derivative()
          << ", received update:" << ( dvdt_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Phase lag-comp: " << get_phase()
          << ", received update:" << ( phase_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Amp   lag-comp: " << get_amplitude()
          << ", received update:" << ( amp_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Freq  lag-comp: " << get_frequency()
          << ", received update:" << ( freq_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Tol   lag-comp: " << get_tolerance()
          << ", received update:" << ( tol_attr->is_received() ? "Yes" : "No" ) << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
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
      sim_data->set_name( get_name() );
   }
   if ( value_attr->is_received() ) {
      sim_data->set_value( get_value() );
   }
   if ( dvdt_attr->is_received() ) {
      sim_data->set_derivative( get_derivative() );
   }
   if ( phase_attr->is_received() ) {
      sim_data->set_phase( get_phase() );
   }
   if ( freq_attr->is_received() ) {
      sim_data->set_frequency( get_frequency() );
   }
   if ( amp_attr->is_received() ) {
      sim_data->set_amplitude( get_amplitude() );
   }
   if ( tol_attr->is_received() ) {
      sim_data->set_tolerance( get_tolerance() );
   }

   sim_data->set_time( time );

   // Do the lag compensation by computing values to the time.
   sim_data->compute_value( time );
   sim_data->compute_derivative( time );

   sim_data->adjust_phase( time );

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream msg;
      msg << "SineLagCompensation::receive_lag_compensation():" << __LINE__ << '\n'
          << " AFTER LAG COMPENSATION:\n"
          << "\t Name  sim_data: '" << sim_data->get_name() << "'\n"
          << "\t Time  sim_data: " << setprecision( 18 ) << sim_data->get_time() << '\n'
          << "\t Value sim_data: " << sim_data->get_value() << '\n'
          << "\t dvdt  sim_data: " << sim_data->get_derivative() << '\n'
          << "\t Phase sim_data: " << sim_data->get_phase() << '\n'
          << "\t Amp   sim_data: " << sim_data->get_amplitude() << '\n'
          << "\t Freq  sim_data: " << sim_data->get_frequency() << '\n'
          << "\t Tol   sim_data: " << sim_data->get_tolerance() << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
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
      string obj_name = ( this->object != NULL ) ? object->get_name_string() : "";

      ostringstream msg;
      msg << "******* SineLagCompensation::bypass_receive_lag_compensation():" << __LINE__ << '\n'
          << "   object-name:'" << obj_name << "'\n"
          << " lag-comp-type:" << lag_comp_type_str << '\n'
          << " scenario-time:" << setprecision( 18 ) << get_scenario_time() << '\n';
      if ( time_attr->is_received() ) {
         double const dt = time - get_time();
         msg << "     data-time:" << setprecision( 18 ) << get_time() << " Received Update\n"
             << "            dt:" << setprecision( 18 ) << dt << '\n';
      } else {
         msg << "     data-time:" << setprecision( 18 ) << get_time() << " Stale: No Update Received!\n"
             << "            dt: Invalid - No Time Received!\n";
      }
      msg << " BEFORE Bypassing Lag Compensation:\n"
          << "\t Name  lag-comp: '" << get_name()
          << "', received update:" << ( name_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Time  lag-comp: " << setprecision( 18 ) << get_time()
          << ", received update:" << ( time_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Value lag-comp: " << get_value()
          << ", received update:" << ( value_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t dvdt  lag-comp: " << get_derivative()
          << ", received update:" << ( dvdt_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Phase lag-comp: " << get_phase()
          << ", received update:" << ( phase_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Amp   lag-comp: " << get_amplitude()
          << ", received update:" << ( amp_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Freq  lag-comp: " << get_frequency()
          << ", received update:" << ( freq_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t Tol   lag-comp: " << get_tolerance()
          << ", received update:" << ( tol_attr->is_received() ? "Yes" : "No" ) << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
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
      sim_data->set_name( get_name() );
   }
   if ( time_attr->is_received() ) {
      sim_data->set_time( get_time() );
   }
   if ( value_attr->is_received() ) {
      sim_data->set_value( get_value() );
   }
   if ( dvdt_attr->is_received() ) {
      sim_data->set_derivative( get_derivative() );
   }
   if ( phase_attr->is_received() ) {
      sim_data->set_phase( get_phase() );
   }
   if ( freq_attr->is_received() ) {
      sim_data->set_frequency( get_frequency() );
   }
   if ( amp_attr->is_received() ) {
      sim_data->set_amplitude( get_amplitude() );
   }
   if ( tol_attr->is_received() ) {
      sim_data->set_tolerance( get_tolerance() );
   }

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream msg;
      msg << "SineLagCompensation::bypass_receive_lag_compensation():" << __LINE__ << '\n'
          << " AFTER BYPASSING LAG COMPENSATION:\n"
          << "\t Name  sim_data: '" << sim_data->get_name() << "'\n"
          << "\t Time  sim_data: " << setprecision( 18 ) << sim_data->get_time() << '\n'
          << "\t Value sim_data: " << sim_data->get_value() << '\n'
          << "\t dvdt  sim_data: " << sim_data->get_derivative() << '\n'
          << "\t Phase sim_data: " << sim_data->get_phase() << '\n'
          << "\t Amp   sim_data: " << sim_data->get_amplitude() << '\n'
          << "\t Freq  sim_data: " << sim_data->get_frequency() << '\n'
          << "\t Tol   sim_data: " << sim_data->get_tolerance() << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}
