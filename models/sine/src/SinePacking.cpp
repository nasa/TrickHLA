/*!
@ingroup Sine
@file models/sine/src/SinePacking.cpp
@brief This class provides data packing for the sine wave data.

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
@trick_link_dependency{sine/src/SinePacking.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/message_proto.h"

// TrickHLA model include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"

// Model include files.
#include "../include/SineData.hh"
#include "../include/SinePacking.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
SinePacking::SinePacking()
   : SineData(),
     TrickHLA::Packing(),
     sim_data( NULL ),
     phase_deg( 0.0 ),
     pack_count( 0 ),
     buff_size( 0 ),
     buff( NULL ),
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
SinePacking::~SinePacking()
{
   if ( buff != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( buff ) ) ) {
         message_publish( MSG_WARNING, "TrickHLAModel::SinePacking::~SinePacking():%d WARNING failed to delete Trick Memory for 'buff'\n",
                          __LINE__ );
      }
      buff      = NULL;
      buff_size = 0;
   }
}

/*!
 * @job_class{initialization}
 */
void SinePacking::configure(
   SineData *sim_data )
{
   this->sim_data = sim_data;

   return;
}

/*!
 * @job_class{initialization}
 */
void SinePacking::initialize()
{
   // Mark this as initialized.
   TrickHLA::Packing::initialize();

   return;
}

/*!
 * @details From the TrickHLA::Packing class. We override this function so
 * that we can initialize references to the TrickHLA::Attribute's that are
 * used in the unpack function to handle attribute ownership and different
 * attribute data rates. Use the initialize callback function as a way to
 * setup TrickHLA-Attribute references which are used to determine ownership
 * or if data for an attribute was received.
 *
 * @job_class{initialization}
 */
void SinePacking::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   Packing::initialize_callback( obj );

   // Get a reference to the TrickHL-AAttribute for all the FOM attributes
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
}

void SinePacking::pack()
{
   if ( !initialized ) {
      ostringstream msg;
      msg << "SinePacking::pack():" << __LINE__
          << " ERROR: The initialize() function has not been called!\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Just count the number of times the pack() function gets called.
   ++pack_count;

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Copy over the sim-data over to the packing data as a starting point.
   set_name( sim_data->get_name() );
   set_time( sim_data->get_time() );
   set_value( sim_data->get_value() );
   set_derivative( sim_data->get_derivative() );
   set_phase( sim_data->get_phase() );
   set_frequency( sim_data->get_frequency() );
   set_amplitude( sim_data->get_amplitude() );
   set_tolerance( sim_data->get_tolerance() );

   // For this example to show how to use the Packing API's, we will assume
   // that the phase shared between federates is in degrees so covert it from
   // radians to degrees.
   phase_deg = sim_data->get_phase() * 180.0 / M_PI;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_PACKING ) ) {
      string        obj_name = ( this->object != NULL ) ? object->get_name_string() : "";
      ostringstream msg;
      msg << "SinePacking::pack():" << __LINE__ << '\n'
          << "\t Object-Name:'" << obj_name << "'\n"

          << "\t sim_data->name:'" << sim_data->get_name()
          << "', Send-HLA-Data:"
          << ( ( name_attr->is_publish() && name_attr->is_locally_owned() ) ? "Yes" : "No" )
          << '\n'

          << "\t sim_data->time:" << setprecision( 18 ) << sim_data->get_time() << " seconds"
          << ", Send-HLA-Data:"
          << ( ( time_attr->is_publish() && time_attr->is_locally_owned() ) ? "Yes" : "No" )
          << '\n'

          << "\t sim_data->value:" << sim_data->get_value()
          << ", Send-HLA-Data:"
          << ( ( value_attr->is_publish() && value_attr->is_locally_owned() ) ? "Yes" : "No" )
          << '\n'

          << "\t sim_data->dvdt:" << sim_data->get_derivative()
          << ", Send-HLA-Data:"
          << ( ( dvdt_attr->is_publish() && dvdt_attr->is_locally_owned() ) ? "Yes" : "No" )
          << '\n'

          << "\t sim_data->phase:" << sim_data->get_phase() << " radians"
          << " ==> packing-phase:" << phase_deg << " degrees"
          << ", Send-HLA-Data:"
          << ( ( phase_attr->is_publish() && phase_attr->is_locally_owned() ) ? "Yes" : "No" )
          << '\n'

          << "\t sim_data->amp:" << sim_data->get_amplitude()
          << ", Send-HLA-Data:"
          << ( ( amp_attr->is_publish() && amp_attr->is_locally_owned() ) ? "Yes" : "No" )
          << '\n'

          << "\t sim_data->freq:" << sim_data->get_frequency()
          << ", Send-HLA-Data:"
          << ( ( freq_attr->is_publish() && freq_attr->is_locally_owned() ) ? "Yes" : "No" )
          << '\n'

          << "\t sim_data->tol:" << sim_data->get_tolerance()
          << ", Send-HLA-Data:"
          << ( ( tol_attr->is_publish() && tol_attr->is_locally_owned() ) ? "Yes" : "No" )
          << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Output more debug information for a higher debug-level.
   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_PACKING ) ) {

      ostringstream msg;

      if ( buff != NULL ) {

         msg << " SinePacking::pack():" << __LINE__ << " ADDITIONAL DEBUG:\n"
             << " buff_size: " << buff_size << '\n';

         unsigned char c = pack_count % 10;
         for ( int i = 0; i < buff_size; ++i ) {
            buff[i] = c;
            msg << " SinePacking::pack():" << __LINE__
                << " buffer[" << i << "] = " << (int)buff[i] << '\n';
         }
      }

      string obj_name = ( this->object != NULL ) ? object->get_name_string() : "";

      msg << "SinePacking::pack():" << __LINE__ << " ADDITIONAL DEBUG:\n"
          << "\t Object-Name:'" << obj_name << "'\n";

      // This part of the example goes a little deeper into the details of
      // the TrickHLA API's, where most users may never go.
      string     name_attr_str = "Name";
      Attribute *attr          = get_attribute( name_attr_str.c_str() );

      msg << "\t FOM-Attribute '" << name_attr_str << "'\n";

      if ( attr != NULL ) {

         // Get the address of the "name" simulation data variable.
         char *name_sim_var = static_cast< char * >( attr->get_sim_variable_address() );

         // Make a little change to the name and show it.
         if ( name_sim_var != NULL ) {

            // Number of bytes ref-attributes says this variable is.
            int const name_sim_var_size = attr->get_attribute_size();

            // NOTE: Make the last character either a '0' through '9' character
            // so that we can see that the name is changing.
            int name_len = strnlen( name_sim_var, name_sim_var_size );
            if ( name_len > 0 ) {
               name_sim_var[name_len - 1] = (char)( '0' + ( pack_count % 10 ) );
            }

            msg << "\t Value:'" << string( name_sim_var ) << "'\n";
         } else {
            msg << "\t Value:NULL\n";
         }

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PACKING ) ) {
            // Print the state of the TrickHLA-Attribute internal buffer.
            attr->print_buffer();
         }
      } else {
         msg << "\t NULL Attribute for FOM-Attribute '" << name_attr_str << "'\n";
      }

      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

void SinePacking::unpack()
{
   if ( !initialized ) {
      ostringstream msg;
      msg << "SinePacking::unpack():" << __LINE__
          << " ERROR: The initialize() function has not been called!\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // If the HLA phase attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value. If we locally own the "Phase" attribute then we do not
   // want to override it's value. If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the "phase_deg" local variable, which would cause data
   // corruption of the state. We always need to do this check because
   // ownership transfers could happen at any time or the data could be at a
   // different rate.

   // Make sure to copy over the packing data over to the sim-data.
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
   if ( freq_attr->is_received() ) {
      sim_data->set_frequency( get_frequency() );
   }
   if ( amp_attr->is_received() ) {
      sim_data->set_amplitude( get_amplitude() );
   }
   if ( tol_attr->is_received() ) {
      sim_data->set_tolerance( get_tolerance() );
   }

   if ( phase_attr->is_received() ) {
      // For this example to show how to use the Packing API's, we will
      // assume that the phase shared between federates is in degrees so
      // covert it back from degrees to radians.
      sim_data->set_phase( phase_deg * M_PI / 180.0 );
   }

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_PACKING ) ) {

      string obj_name = ( this->object != NULL ) ? object->get_name_string() : "";

      ostringstream msg;
      msg << "SinePacking::unpack():" << __LINE__ << '\n'
          << "\t Object-Name:'" << obj_name << "'\n"

          << "\t sim_data->name:'" << sim_data->get_name()
          << "', Received-HLA-Data:"
          << ( name_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t sim_data->time:" << setprecision( 18 ) << sim_data->get_time()
          << " seconds, Received-HLA-Data:"
          << ( time_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t sim_data->value:" << sim_data->get_value()
          << ", Received-HLA-Data:"
          << ( value_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t sim_data->dvdt:" << sim_data->get_derivative()
          << ", Received-HLA-Data:"
          << ( dvdt_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t packing-phase:" << phase_deg << " degrees ==> sim_data->phase:"
          << sim_data->get_phase() << " radians, Received-HLA-Data:"
          << ( phase_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t sim_data->amp:" << sim_data->get_amplitude()
          << ", Received-HLA-Data:"
          << ( amp_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t sim_data->freq:" << sim_data->get_frequency()
          << ", Received-HLA-Data:"
          << ( freq_attr->is_received() ? "Yes" : "No" ) << '\n'

          << "\t sim_data->tol:" << sim_data->get_tolerance()
          << ", Received-HLA-Data:"
          << ( tol_attr->is_received() ? "Yes" : "No" ) << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Output more debug information for a higher debug-level.
   if ( DebugHandler::show( DEBUG_LEVEL_10_TRACE, DEBUG_SOURCE_PACKING ) ) {

      ostringstream msg;

      if ( buff != NULL ) {
         msg << " SinePacking::unpack():" << __LINE__ << " ADDITIONAL DEBUG:\n"
             << " buff_size: " << buff_size << '\n';
         for ( int i = 0; i < buff_size; ++i ) {
            msg << " SinePacking::unpack():" << __LINE__
                << " buffer[" << i << "] = " << (int)buff[i] << '\n';
         }
      }

      string obj_name = ( this->object != NULL ) ? object->get_name_string() : "";

      msg << "SinePacking::unpack():" << __LINE__ << " ADDITIONAL DEBUG:\n"
          << "\t Object-Name:'" << obj_name << "'\n";

      // This part of the example goes a little deeper into the details of
      // the TrickHLA API's where most users may never go.
      string     name_attr_str = "Name";
      Attribute *attr          = get_attribute( name_attr_str.c_str() );

      msg << "\t FOM-Attribute '" << name_attr_str << "'\n";

      if ( attr != NULL ) {

         // Get the address of the "name" simulation data variable.
         char *name_sim_var = static_cast< char * >( attr->get_sim_variable_address() );

         // Display the name.
         if ( name_sim_var != NULL ) {
            msg << "\t Value:'" << string( name_sim_var ) << "'\n";
         } else {
            msg << "\t Value:NULL\n";
         }

         if ( DebugHandler::show( DEBUG_LEVEL_11_TRACE, DEBUG_SOURCE_PACKING ) ) {
            // Print the state of the TrickHLA-Attribute internal buffer.
            attr->print_buffer();
         }
      } else {
         msg << "\t NULL Attribute for FOM-Attribute '" << name_attr_str
             << "'\n";
      }

      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}
