/*!
@file models/sine/src/SineConditional.cpp
@ingroup TrickHLAModel
@brief Subclass the base class to provide sine wave-specific CONDITIONAL attribute.

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
@trick_link_dependency{sine/src/SineConditional.cpp}
@trick_link_dependency{sine/src/SineData.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, DSES, October 2009, --, Initial implementation.}
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

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"

// Model include files.
#include "../include/SineConditional.hh"
#include "../include/SineData.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
SineConditional::SineConditional()
   : TrickHLA::Conditional(),
     sim_data( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SineConditional::~SineConditional()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void SineConditional::initialize(
   SineData *data )
{
   this->sim_data = data;

   // Make a copy of the incoming data so that we have a previous state
   // to compare to.
   prev_sim_data.set_name( sim_data->get_name() );
   prev_sim_data.set_time( sim_data->get_time() );
   prev_sim_data.set_value( sim_data->get_value() );
   prev_sim_data.set_derivative( sim_data->get_derivative() );
   prev_sim_data.set_phase( sim_data->get_phase() );
   prev_sim_data.set_frequency( sim_data->get_frequency() );
   prev_sim_data.set_amplitude( sim_data->get_amplitude() );
   prev_sim_data.set_tolerance( sim_data->get_tolerance() );
}

/*!
 * @job_class{scheduled}
 */
bool SineConditional::should_send(
   TrickHLA::Attribute *attr )
{
   if ( this->sim_data == NULL ) {
      send_hs( stderr, "SineConditional::should_send('%s'):%d => ERROR: Unexpected NULL sim_data!",
               attr->get_FOM_name(), __LINE__ );
      exit( 1 );
   }

   bool send_attr = false;

   // Convert the FOM name to a string.
   string attr_FOM_name = ( attr->get_FOM_name() != NULL ) ? attr->get_FOM_name() : "";

   // If there is simulation data to compare to and if the attribute FOM name
   // has been specified, check the value of the current simulation variable
   // versus the previous value. return true if there was a change.
   //
   if ( attr_FOM_name == "Name" ) {
      send_attr = true; // Always send the name.

   } else if ( attr_FOM_name == "Time" ) {
      if ( sim_data->get_time() != prev_sim_data.get_time() ) {
         send_attr = true;
         prev_sim_data.set_time( sim_data->get_time() );
      }

   } else if ( attr_FOM_name == "Value" ) {
      if ( sim_data->get_value() != prev_sim_data.get_value() ) {
         send_attr = true;
         prev_sim_data.set_value( sim_data->get_value() );
      }

   } else if ( attr_FOM_name == "dvdt" ) {
      if ( sim_data->get_derivative() != prev_sim_data.get_derivative() ) {
         send_attr = true;
         prev_sim_data.set_derivative( sim_data->get_derivative() );
      }

   } else if ( attr_FOM_name == "Phase" ) {
      if ( sim_data->get_phase() != prev_sim_data.get_phase() ) {
         send_attr = true;
         prev_sim_data.set_phase( sim_data->get_phase() );
      }

   } else if ( attr_FOM_name == "Frequency" ) {
      if ( sim_data->get_frequency() != prev_sim_data.get_frequency() ) {
         send_attr = true;
         prev_sim_data.set_frequency( sim_data->get_frequency() );
      }

   } else if ( attr_FOM_name == "Amplitude" ) {
      if ( sim_data->get_amplitude() != prev_sim_data.get_amplitude() ) {
         send_attr = true;
         prev_sim_data.set_amplitude( sim_data->get_amplitude() );
      }

   } else if ( attr_FOM_name == "Tolerance" ) {
      if ( sim_data->get_tolerance() != prev_sim_data.get_tolerance() ) {
         send_attr = true;
         prev_sim_data.set_tolerance( sim_data->get_tolerance() );
      }

   } else {
      send_hs( stderr, "SineConditional::should_send('%s'):%d => ERROR: \
Could not find the data for the specified FOM attribute!",
               attr->get_FOM_name(), __LINE__ );
      exit( 1 );
   }

   return send_attr;
}
