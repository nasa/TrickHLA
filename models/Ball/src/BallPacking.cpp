/*!
@file Ball/BallPacking.cpp
@ingroup Ball
@brief This class provides data packing for the BallState.

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
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{BallPacking.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, April 2025, --, Initial version.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/matrix_macros.h"
#include "trick/message_proto.h"
#include "trick/vector_macros.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/Types.hh"

// Ball include files.
#include "Ball/include/BallPacking.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
BallPacking::BallPacking()
   : debug( false ),
     name_attr( NULL ),
     time_attr( NULL ),
     position_attr( NULL ),
     velocity_attr( NULL ),
     acceleration_attr( NULL ),
     force_attr( NULL ),
     name( NULL ),
     time( 0.0 ),
     ball_state( NULL )
{
   //
   // Initialize the state.
   //
   position[0]       = 0.0;
   position[0]       = 0.0;
   velocity[0]       = 0.0;
   velocity[0]       = 0.0;
   acceleration[0]   = 0.0;
   acceleration[0]   = 0.0;
   external_force[0] = 0.0;
   external_force[0] = 0.0;

   return;
}

/*!
 * @job_class{initialization}
 */
BallPacking::BallPacking( BallState &ball_state_ref )
   : debug( false ),
     name_attr( NULL ),
     time_attr( NULL ),
     position_attr( NULL ),
     velocity_attr( NULL ),
     acceleration_attr( NULL ),
     force_attr( NULL ),
     name( NULL ),
     time( 0.0 ),
     ball_state( &ball_state_ref )
{
   //
   // Initialize the state.
   //
   position[0]       = 0.0;
   position[0]       = 0.0;
   velocity[0]       = 0.0;
   velocity[0]       = 0.0;
   acceleration[0]   = 0.0;
   acceleration[0]   = 0.0;
   external_force[0] = 0.0;
   external_force[0] = 0.0;

   return;
}

/*!
 * @job_class{shutdown}
 */
BallPacking::~BallPacking()
{
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         message_publish( MSG_WARNING, "Ball::BallPacking::~BallPacking():%d WARNING deleting Trick Memory for 'this->name'\n",
                          __LINE__ );
      }
      this->name = NULL;
   }
   initialized       = false;
   name_attr         = NULL;
   time_attr         = NULL;
   position_attr     = NULL;
   velocity_attr     = NULL;
   acceleration_attr = NULL;
   force_attr        = NULL;
   ball_state        = NULL;
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{initialization}
 */
void BallPacking::base_config(
   char const       *sim_obj_name,
   char const       *ball_obj_name,
   char const       *ball_name,
   bool              publishes,
   TrickHLA::Object *mngr_object )
{
   string entity_name_str = string( sim_obj_name ) + "." + string( ball_obj_name );
   string trick_name_str;

   // Associate the instantiated Manager object with this packing object.
   this->object = mngr_object;

   // Set the Ball name.
   if ( ball_name != NULL ) {
      name = trick_MM->mm_strdup( ball_name );
   } else {
      ostringstream errmsg;
      errmsg << "Ball::BallPacking::base_config():" << __LINE__
             << " ERROR: Unexpected NULL federation instance Ball name!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   object->FOM_name            = "Ball";
   object->name                = ball_name;
   object->create_HLA_instance = publishes;
   object->packing             = this;
   // Allocate the attributes for the Ball HLA object.
   object->attr_count = 6;
   object->attributes = static_cast< TrickHLA::Attribute * >( trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count ) );

   //
   // Specify the Reference Frame attributes.
   //
   object->attributes[0].FOM_name      = "name";
   trick_name_str                      = entity_name_str + string( ".name" );
   object->attributes[0].trick_name    = trick_name_str;
   object->attributes[0].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC );
   object->attributes[0].publish       = publishes;
   object->attributes[0].subscribe     = !publishes;
   object->attributes[0].locally_owned = publishes;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[1].FOM_name      = "time";
   trick_name_str                      = entity_name_str + string( ".time" );
   object->attributes[1].trick_name    = trick_name_str;
   object->attributes[1].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC );
   object->attributes[1].publish       = publishes;
   object->attributes[1].subscribe     = !publishes;
   object->attributes[1].locally_owned = publishes;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[2].FOM_name      = "position";
   trick_name_str                      = entity_name_str + string( ".position" );
   object->attributes[2].trick_name    = trick_name_str;
   object->attributes[2].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC );
   object->attributes[2].publish       = publishes;
   object->attributes[2].subscribe     = !publishes;
   object->attributes[2].locally_owned = publishes;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[3].FOM_name      = "velocity";
   trick_name_str                      = entity_name_str + string( ".velocity" );
   object->attributes[3].trick_name    = trick_name_str;
   object->attributes[3].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC );
   object->attributes[3].publish       = publishes;
   object->attributes[3].subscribe     = !publishes;
   object->attributes[3].locally_owned = publishes;
   object->attributes[3].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[4].FOM_name      = "acceleration";
   trick_name_str                      = entity_name_str + string( ".acceleration" );
   object->attributes[4].trick_name    = trick_name_str;
   object->attributes[4].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC );
   object->attributes[4].publish       = publishes;
   object->attributes[4].subscribe     = !publishes;
   object->attributes[4].locally_owned = publishes;
   object->attributes[4].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[5].FOM_name      = "force";
   trick_name_str                      = entity_name_str + string( ".external_force" );
   object->attributes[5].trick_name    = trick_name_str;
   object->attributes[5].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC );
   object->attributes[5].publish       = publishes;
   object->attributes[5].subscribe     = !publishes;
   object->attributes[5].locally_owned = publishes;
   object->attributes[5].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   return;
}

/*!
 * @job_class{initialization}
 */
void BallPacking::initialize()
{
   // Must have federation instance name.
   if ( name == NULL ) {
      ostringstream errmsg;
      errmsg << "Ball::BallPacking::initialize():" << __LINE__
             << " WARNING: Unexpected NULL entity name!"
             << " Setting frame name to empty string.\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      this->name = trick_MM->mm_strdup( "" );
   }

   // Mark this as initialized.
   TrickHLA::Packing::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @details From the TrickHLA::Packing class. We override this function so
 * that we can initialize references to the TrickHLA::Attribute's that are
 * used in the unpack function to handle attribute ownership and different
 * attribute data rates.
 *
 * Use the initialize callback function as a way to setup TrickHLA::Attribute
 * references which are use to determine ownership or if data for an attribute
 * was received.
 *
 * @job_class{initialization}
 */
void BallPacking::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   TrickHLA::Packing::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   name_attr         = get_attribute_and_validate( "name" );
   time_attr         = get_attribute_and_validate( "time" );
   position_attr     = get_attribute_and_validate( "position" );
   velocity_attr     = get_attribute_and_validate( "velocity" );
   acceleration_attr = get_attribute_and_validate( "acceleration" );
   force_attr        = get_attribute_and_validate( "force" );

   // Initialize with the working data in the packing data.
   pack_from_working_data();

   return;
}

/*!
 * @job_class{initialization}
 */
void BallPacking::set_name( char const *new_name )
{
   // Free the existing name.
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         message_publish( MSG_WARNING, "Ball::BallPacking::set_name():%d WARNING deleting Trick Memory for 'this->name'\n",
                          __LINE__ );
      }
      this->name = NULL;
   }

   // Check for NULL new name.
   if ( new_name == NULL ) {
      // Allocate and assign and empty string.
      this->name = trick_MM->mm_strdup( "" );
   } else {
      // Allocate and assign new name in trick memory.
      this->name = trick_MM->mm_strdup( new_name );
   }
   return;
}

/*!
 * @job_class{scheduled}
 */
void BallPacking::pack()
{
   // Check for initialization.
   if ( !initialized ) {
      message_publish( MSG_WARNING, "BallPacking::pack():%d WARNING: The initialize() function has not been called!\n",
                       __LINE__ );
   }

   // Check for latency/lag compensation.
   if ( this->object->lag_comp == NULL ) {
      pack_from_working_data();
   }

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "BallPacking::pack():" << __LINE__ << "\n";
      debug_print( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void BallPacking::unpack()
{

   if ( !initialized ) {
      message_publish( MSG_WARNING, "BallPacking::unpack():%d WARNING: The initialize() function has not been called!\n",
                       __LINE__ );
   }

   // Transfer the packing data into the working data.
   unpack_into_working_data();

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "BallPacking::unpack():" << __LINE__ << "\n";
      debug_print( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void BallPacking::pack_from_working_data()
{
   int iinc;

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Check for name change.
   if ( ball_state->name != NULL ) {

      if ( name != NULL ) {

         // Compare names.
         if ( strcmp( ball_state->name, name ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( name ) ) ) {
               message_publish( MSG_WARNING, "Ball::pack_from_working_data():%d WARNING failed to delete Trick Memory for 'pe_packing_data.name'\n",
                                __LINE__ );
            }
            name = trick_MM->mm_strdup( ball_state->name );
         }

      } else {
         // No name to compare so copy name.

         name = trick_MM->mm_strdup( ball_state->name );
      }

   } else {
      // This is bad scoobies so just punt.

      ostringstream errmsg;
      errmsg << "TrickHLAModel::Ball::pack_from_working_data():" << __LINE__
             << " ERROR: Unexpected NULL name for Ball!\n";
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Time tag for this state data.
   time = get_scenario_time();

   // Set the position data.
   for ( iinc = 0; iinc < 2; ++iinc ) {
      position[iinc] = ball_state->output.position[iinc];
   }

   // Set the velocity data.
   for ( iinc = 0; iinc < 2; ++iinc ) {
      velocity[iinc] = ball_state->output.velocity[iinc];
   }

   // Set the acceleration data.
   for ( iinc = 0; iinc < 2; ++iinc ) {
      acceleration[iinc] = ball_state->output.acceleration[iinc];
   }

   // Set the external force data.
   for ( iinc = 0; iinc < 2; ++iinc ) {
      external_force[iinc] = ball_state->output.external_force[iinc];
   }

   // Return to the calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void BallPacking::unpack_into_working_data()
{
   // If the HLA attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value.  If we locally own the attribute then we do not want to
   // override it's value.  If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state.  We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.

   // Set the entity name.
   if ( name_attr->is_received() ) {
      if ( ball_state->name != NULL ) {
         if ( !strcmp( ball_state->name, name ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( ball_state->name ) ) ) {
               message_publish( MSG_WARNING, "BallPacking::unpack_into_working_data():%d WARNING failed to delete Trick Memory for 'ball_state->name'\n",
                                __LINE__ );
            }
            ball_state->name = trick_MM->mm_strdup( name );
         }
      } else {
         ball_state->name = trick_MM->mm_strdup( name );
      }
   }

   // Unpack the time.
   if ( time_attr->is_received() ) {
      // Time tag for this state data.
      // For now, we dump the time tag.
      // ball_state->time = time;
   }

   // Unpack the position data.
   if ( position_attr->is_received() ) {
      for ( int iinc = 0; iinc < 2; ++iinc ) {
         ball_state->output.position[iinc] = position[iinc];
      }
   }

   // Unpack the velocity data.
   if ( velocity_attr->is_received() ) {
      for ( int iinc = 0; iinc < 2; ++iinc ) {
         ball_state->output.velocity[iinc] = velocity[iinc];
      }
   }

   // Unpack the translational acceleration data.
   if ( acceleration_attr->is_received() ) {
      for ( int iinc = 0; iinc < 2; ++iinc ) {
         ball_state->output.acceleration[iinc] = acceleration[iinc];
      }
   }

   // Unpack the external_force data.
   if ( force_attr->is_received() ) {
      for ( int iinc = 0; iinc < 2; ++iinc ) {
         ball_state->output.external_force[iinc] = external_force[iinc];
      }
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void BallPacking::debug_print( std::ostream &stream ) const
{

   // Set the print precision.
   stream.precision( 15 );

   stream << "\tObject-Name: '" << object->get_name() << "'\n"
          << "\tname:          '" << ( name != NULL ? name : "" ) << "'\n"
          << "\ttime:          " << time << "\n"
          << "\tposition:      " << position[0] << ", " << position[1] << "\n"
          << "\tvelocity:      " << velocity[0] << ", " << velocity[1] << "\n"
          << "\tacceleration:  " << acceleration[0] << ", " << acceleration[1] << "\n"
          << "\external force: " << external_force[0] << ", " << external_force[1] << "\n";

   return;
}
