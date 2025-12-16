/*!
@file Ball/WallsPacking.cpp
@ingroup Ball
@brief This class provides data packing for the BallWalls.

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
@trick_link_dependency{WallsPacking.cpp}


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

// Wall include files.
#include "Ball/include/WallsPacking.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
WallsPacking::WallsPacking()
   : debug( false ),
     floor_y_pos_attr( NULL ),
     right_wall_x_pos_attr( NULL ),
     ceiling_y_pos_attr( NULL ),
     left_wall_x_pos_attr( NULL ),
     floor_y_pos( 0.0 ),
     right_wall_x_pos( 0.0 ),
     ceiling_y_pos( 0.0 ),
     left_wall_x_pos( 0.0 ),
     walls( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
WallsPacking::WallsPacking( BallWalls &walls_ref )
   : debug( false ),
     floor_y_pos_attr( NULL ),
     right_wall_x_pos_attr( NULL ),
     ceiling_y_pos_attr( NULL ),
     left_wall_x_pos_attr( NULL ),
     floor_y_pos( 0.0 ),
     right_wall_x_pos( 0.0 ),
     ceiling_y_pos( 0.0 ),
     left_wall_x_pos( 0.0 ),
     walls( &walls_ref )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
WallsPacking::~WallsPacking()
{
   initialized           = false;
   floor_y_pos_attr      = NULL;
   right_wall_x_pos_attr = NULL;
   ceiling_y_pos_attr    = NULL;
   left_wall_x_pos_attr  = NULL;
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{initialization}
 */
void WallsPacking::base_config(
   char const       *sim_obj_name,
   char const       *walls_obj_name,
   char const       *walls_name,
   bool              publishes,
   TrickHLA::Object *mngr_object )
{
   string entity_name_str = string( sim_obj_name ) + "." + string( walls_obj_name );
   string trick_name_str;

   // Associate the instantiated Manager object with this packing object.
   this->object = mngr_object;

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   object->FOM_name            = "Walls";
   object->name                = walls_name;
   object->create_HLA_instance = publishes;
   object->packing             = this;
   // Allocate the attributes for the Wall HLA object.
   object->attr_count = 4;
   object->attributes = static_cast< TrickHLA::Attribute * >( trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count ) );

   //
   // Specify the Reference Frame attributes.
   //
   object->attributes[0].FOM_name      = "ceiling";
   trick_name_str                      = entity_name_str + string( ".ceiling_y_pos" );
   object->attributes[0].trick_name    = trick_name_str;
   object->attributes[0].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE );
   object->attributes[0].publish       = publishes;
   object->attributes[0].subscribe     = !publishes;
   object->attributes[0].locally_owned = publishes;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[1].FOM_name      = "right";
   trick_name_str                      = entity_name_str + string( ".right_wall_x_pos" );
   object->attributes[1].trick_name    = trick_name_str;
   object->attributes[1].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE );
   object->attributes[1].publish       = publishes;
   object->attributes[1].subscribe     = !publishes;
   object->attributes[1].locally_owned = publishes;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[2].FOM_name      = "left";
   trick_name_str                      = entity_name_str + string( ".left_wall_x_pos" );
   object->attributes[2].trick_name    = trick_name_str;
   object->attributes[2].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE );
   object->attributes[2].publish       = publishes;
   object->attributes[2].subscribe     = !publishes;
   object->attributes[2].locally_owned = publishes;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[3].FOM_name      = "floor";
   trick_name_str                      = entity_name_str + string( ".floor_y_pos" );
   object->attributes[3].trick_name    = trick_name_str;
   object->attributes[3].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE );
   object->attributes[3].publish       = publishes;
   object->attributes[3].subscribe     = !publishes;
   object->attributes[3].locally_owned = publishes;
   object->attributes[3].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   return;
}

/*!
 * @job_class{initialization}
 */
void WallsPacking::initialize()
{
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
void WallsPacking::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   TrickHLA::Packing::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   floor_y_pos_attr      = get_attribute_and_validate( "floor" );
   right_wall_x_pos_attr = get_attribute_and_validate( "right" );
   ceiling_y_pos_attr    = get_attribute_and_validate( "ceiling" );
   left_wall_x_pos_attr  = get_attribute_and_validate( "left" );

   // Initialize with the working data in the packing data.
   pack_from_working_data();

   return;
}

/*!
 * @job_class{scheduled}
 */
void WallsPacking::pack()
{
   // Check for initialization.
   if ( !initialized ) {
      message_publish( MSG_WARNING, "WallsPacking::pack():%d WARNING: The initialize() function has not been called!\n",
                       __LINE__ );
   }

   // Check for latency/lag compensation.
   if ( this->object->lag_comp == NULL ) {
      pack_from_working_data();
   }

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "WallsPacking::pack():" << __LINE__ << "\n";
      debug_print( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void WallsPacking::unpack()
{

   if ( !initialized ) {
      message_publish( MSG_WARNING, "WallsPacking::unpack():%d WARNING: The initialize() function has not been called!\n",
                       __LINE__ );
   }

   // Transfer the packing data into the working data.
   unpack_into_working_data();

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "WallsPacking::unpack():" << __LINE__ << "\n";
      debug_print( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void WallsPacking::pack_from_working_data()
{

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   floor_y_pos      = walls->floor_y_pos;
   right_wall_x_pos = walls->right_wall_x_pos;
   ceiling_y_pos    = walls->ceiling_y_pos;
   left_wall_x_pos  = walls->left_wall_x_pos;

   // Return to the calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void WallsPacking::unpack_into_working_data()
{
   // If the HLA attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value.  If we locally own the attribute then we do not want to
   // override it's value.  If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state.  We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.

   // Unpack the floor Y position.
   if ( floor_y_pos_attr->is_received() ) {
      walls->floor_y_pos = floor_y_pos;
   }

   // Unpack the right wall X position.
   if ( right_wall_x_pos_attr->is_received() ) {
      walls->right_wall_x_pos = right_wall_x_pos;
   }

   // Unpack the ceiling Y position.
   if ( ceiling_y_pos_attr->is_received() ) {
      walls->ceiling_y_pos = ceiling_y_pos;
   }

   // Unpack the left wall X position.
   if ( left_wall_x_pos_attr->is_received() ) {
      walls->left_wall_x_pos = left_wall_x_pos;
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void WallsPacking::debug_print( std::ostream &stream ) const
{

   // Set the print precision.
   stream.precision( 15 );

   stream << "\tObject-Name: '" << object->get_name() << "'\n"
          << "\tCeiling:    " << ceiling_y_pos << "\n"
          << "\tRight Wall: " << right_wall_x_pos << "\n"
          << "\tLeft Wall:  " << left_wall_x_pos << "\n"
          << "\tFloor:      " << floor_y_pos << "\n";

   return;
}
