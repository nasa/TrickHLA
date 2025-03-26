/*!
@ingroup Sine
@file models/sine/src/SineConditional.cpp
@brief Implements the Conditional API to conditionally send attributes.

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
@trick_link_dependency{../../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../../source/TrickHLA/Conditional.cpp}
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{sine/src/SineConditional.cpp}
@trick_link_dependency{sine/src/SineData.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, November 2023, --, Using the updated Conditional API.}
@revs_end

*/

// System include files.
#include <iostream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/Object.hh"

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
   : SineData(),
     TrickHLA::Conditional(),
     sim_data( NULL ),
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
SineConditional::~SineConditional()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void SineConditional::configure(
   SineData *data )
{
   this->sim_data = data;

   if ( this->sim_data == NULL ) {
      message_publish( MSG_ERROR, "SineConditional::initialize():%d ERROR: Unexpected NULL sim_data!",
                       __LINE__ );
      exit( -1 );
   }
   return;
}

/*!
 * @job_class{initialization}
 */
void SineConditional::initialize()
{

   if ( this->sim_data == NULL ) {
      message_publish( MSG_ERROR, "SineConditional::initialize():%d ERROR: Unexpected NULL sim_data!",
                       __LINE__ );
      exit( -1 );
   }

   // Make a copy of the incoming data so that we have a previous state
   // to compare to.
   set_name( sim_data->get_name() );
   set_time( sim_data->get_time() );
   set_value( sim_data->get_value() );
   set_derivative( sim_data->get_derivative() );
   set_phase( sim_data->get_phase() );
   set_frequency( sim_data->get_frequency() );
   set_amplitude( sim_data->get_amplitude() );
   set_tolerance( sim_data->get_tolerance() );

   return;
}

/*!
 * @details From the TrickHLA::Conditional class. We override this function
 * so that we can initialize references to the TrickHLA::Attribute's that are
 * used in the should_send function to determine if an attribute value should
 * be sent.
 *
 * @job_class{initialization}
 */
void SineConditional::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   Conditional::initialize_callback( obj );

   // Get a reference to the TrickHL-AAttribute for all the FOM attributes
   // names. We do this here so that we only do the attribute lookup once
   // instead of looking it up every time the should_send() function is called.
   name_attr  = get_attribute_and_validate( "Name" );
   time_attr  = get_attribute_and_validate( "Time" );
   value_attr = get_attribute_and_validate( "Value" );
   dvdt_attr  = get_attribute_and_validate( "dvdt" );
   phase_attr = get_attribute_and_validate( "Phase" );
   freq_attr  = get_attribute_and_validate( "Frequency" );
   amp_attr   = get_attribute_and_validate( "Amplitude" );
   tol_attr   = get_attribute_and_validate( "Tolerance" );
}

/*!
 * @job_class{scheduled}
 */
bool SineConditional::should_send(
   TrickHLA::Attribute *attr )
{
   if ( this->sim_data == NULL ) {
      message_publish( MSG_ERROR, "SineConditional::should_send('%s'):%d ERROR: Unexpected NULL sim_data!",
                       attr->get_FOM_name(), __LINE__ );
      exit( -1 );
   }

   bool send_attr = false;

   // Check the value of the current simulation variable versus the previous
   // value and return true if there was a change.
   if ( attr == name_attr ) {
      send_attr = true; // Always send the name.
   } else if ( attr == time_attr ) {
      if ( sim_data->get_time() != get_time() ) {
         send_attr = true;
         set_time( sim_data->get_time() ); // Update to the current state
      }
   } else if ( attr == value_attr ) {
      if ( sim_data->get_value() != get_value() ) {
         send_attr = true;
         set_value( sim_data->get_value() ); // Update to the current state
      }
   } else if ( attr == dvdt_attr ) {
      if ( sim_data->get_derivative() != get_derivative() ) {
         send_attr = true;
         set_derivative( sim_data->get_derivative() ); // Update to the current state
      }
   } else if ( attr == phase_attr ) {
      if ( sim_data->get_phase() != get_phase() ) {
         send_attr = true;
         set_phase( sim_data->get_phase() ); // Update to the current state
      }
   } else if ( attr == freq_attr ) {
      if ( sim_data->get_frequency() != get_frequency() ) {
         send_attr = true;
         set_frequency( sim_data->get_frequency() ); // Update to the current state
      }
   } else if ( attr == amp_attr ) {
      if ( sim_data->get_amplitude() != get_amplitude() ) {
         send_attr = true;
         set_amplitude( sim_data->get_amplitude() ); // Update to the current state
      }
   } else if ( attr == tol_attr ) {
      if ( sim_data->get_tolerance() != get_tolerance() ) {
         send_attr = true;
         set_tolerance( sim_data->get_tolerance() ); // Update to the current state
      }
   } else {
      message_publish( MSG_ERROR, "SineConditional::should_send('%s'):%d ERROR: \
Could not find the data for the specified FOM attribute!",
                       attr->get_FOM_name(), __LINE__ );
      exit( -1 );
   }

   return send_attr;
}
