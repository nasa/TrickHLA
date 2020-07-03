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
@trick_link_dependency{../source/TrickHLA/LagCompensation.o}
@trick_link_dependency{sine/src/SineLagCompensation.o}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2006, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>
#include <stdlib.h>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h" // for send_hs
#include "trick/trick_math.h"

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
   : sim_data( NULL ),
     lag_comp_data( NULL ),
     time_attr( NULL ),
     value_attr( NULL ),
     dvdt_attr( NULL ),
     phase_attr( NULL ),
     freq_attr( NULL ),
     amp_attr( NULL ),
     tol_attr( NULL ),
     name_attr( NULL )
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
void SineLagCompensation::initialize(
   SineData *sim_data,
   SineData *lag_comp_data )
{
   this->sim_data      = sim_data;
   this->lag_comp_data = lag_comp_data;
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
   time_attr  = get_attribute_and_validate( "Time" );
   value_attr = get_attribute_and_validate( "Value" );
   dvdt_attr  = get_attribute_and_validate( "dvdt" );
   phase_attr = get_attribute_and_validate( "Phase" );
   freq_attr  = get_attribute_and_validate( "Frequency" );
   amp_attr   = get_attribute_and_validate( "Amplitude" );
   tol_attr   = get_attribute_and_validate( "Tolerance" );
   name_attr  = get_attribute_and_validate( "Name" );
}

void SineLagCompensation::send_lag_compensation()
{
   double dt   = get_fed_lookahead().get_time_in_seconds();
   double time = get_scenario_time() + dt;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( should_print( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      cout << "******* SineLagCompensation::send_lag_compensation()" << endl
           << " scenario-time:" << get_scenario_time() << endl
           << "     data-time:" << sim_data->get_time() << endl
           << "            dt:" << dt << endl
           << " adjusted-time:" << time << endl;
   }

   if ( lag_comp_data != sim_data ) {
      // Copy the current sine state over to the predicted sine state.
      *lag_comp_data = *sim_data;
   }
   lag_comp_data->compute_value( time );
   lag_comp_data->compute_derivative( time );
}

void SineLagCompensation::receive_lag_compensation()
{
   // If the HLA time attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value. If we locally own the attribute then we do not want to
   // override it's value. If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state. We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.
   double time = get_scenario_time();
   double dt   = time - lag_comp_data->get_time();

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( should_print( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      cout << "******* SineLagCompensation::receive_lag_compensation()" << endl
           << " scenario-time:" << get_scenario_time() << endl
           << "     data-time:" << lag_comp_data->get_time() << endl
           << "            dt:" << dt << endl
           << " adjusted-time:" << time << endl;

      cout << "SineLagCompensation::receive_lag_compensation() Before Lag Compensation:" << endl
           << "\t Time \tlagging_data: " << lag_comp_data->get_time() << endl
           << "\t Value \tlagging_data: " << lag_comp_data->get_value() << endl
           << "\t dvdt \tlagging_data: " << lag_comp_data->get_derivative() << endl
           << "\t Phase \tlagging_data: " << lag_comp_data->get_phase() << endl
           << "\t Freq \tlagging_data: " << lag_comp_data->get_frequency() << endl
           << "\t Amp \tlagging_data: " << lag_comp_data->get_amplitude() << endl
           << "\t Tol \tlagging_data: " << lag_comp_data->get_tolerance() << endl;
   }

   // Because of ownership transfers and attributes being sent at different
   // rates we need to check to see if we received data for each attribute.
   if ( value_attr->is_received() ) {
      sim_data->set_value( lag_comp_data->get_value() );
   }
   if ( dvdt_attr->is_received() ) {
      sim_data->set_derivative( lag_comp_data->get_derivative() );
   }
   if ( phase_attr->is_received() ) {
      sim_data->set_phase( lag_comp_data->get_phase() );
   }
   if ( freq_attr->is_received() ) {
      sim_data->set_frequency( lag_comp_data->get_frequency() );
   }
   if ( amp_attr->is_received() ) {
      sim_data->set_amplitude( lag_comp_data->get_amplitude() );
   }
   if ( tol_attr->is_received() ) {
      sim_data->set_tolerance( lag_comp_data->get_tolerance() );
   }
   if ( name_attr->is_received() ) {
      sim_data->set_name( lag_comp_data->get_name() );
   }

   // Do the lag compensation by computing values to the time.
   sim_data->compute_value( time );
   sim_data->compute_derivative( time );

   sim_data->adjust_phase( time );

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( should_print( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      cout << "SineLagCompensation::receive_lag_compensation() AFTER LAG COMPENSATION:" << endl
           << "\t Time \tsim_data: " << sim_data->get_time() << endl
           << "\t Value \tsim_data: " << sim_data->get_value() << endl
           << "\t dvdt \tsim_data: " << sim_data->get_derivative() << endl
           << "\t Phase \tsim_data: " << sim_data->get_phase() << endl
           << "\t Freq \tsim_data: " << sim_data->get_frequency() << endl
           << "\t Amp \tsim_data: " << sim_data->get_amplitude() << endl
           << "\t Tol \tsim_data: " << sim_data->get_tolerance() << endl;
   }
}
