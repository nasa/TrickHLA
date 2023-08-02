/*!
@file SpaceFOM/PhysicalEntity.cpp
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM PhysicalEntities.

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
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{PhysicalEntity.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Initial version.}
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
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntity.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntity::PhysicalEntity() // RETURN: -- None.
   : physical_data( NULL )
{
}

/*!
 * @job_class{shutdown}
 */
PhysicalEntity::~PhysicalEntity() // RETURN: -- None.
{
   physical_data = NULL;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntity::initialize()
{
   ostringstream errmsg;

   // Check to make sure the PhysicalEntity data is set.
   if ( physical_data == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL PhysicalEntityData: " << this->name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Mark this as initialized.
   PhysicalEntityBase::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntity::initialize( PhysicalEntityData *physical_data_ptr )
{
   ostringstream errmsg;

   // Set the reference to the PhysicalEntity data.
   if ( physical_data_ptr == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL PhysicalEntityData: " << this->name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->physical_data = physical_data_ptr;

   // Mark this as initialized.
   this->initialize();

   // Return to calling routine.
   return;
}



void PhysicalEntity::pack()
{
   int iinc;

   // Check for initialization.
   if ( !initialized ) {
      cout << "JEODPhysicalEntity::pack() ERROR: The initialize() function has not"
           << " been called!" << endl;
   }

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Pack the state time coordinate data.
   // Position and velocity vectors.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      state.pos[iinc] = physical_data->state.pos[iinc];
      state.vel[iinc] = physical_data->state.vel[iinc];
   }
   // Attitude quaternion.
   state.quat_scalar = physical_data->state.quat_scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      state.quat_vector[iinc] = physical_data->state.quat_vector[iinc];
      state.ang_vel[iinc]     = physical_data->state.ang_vel[iinc];
   }

   // Time tag for this state data.
   state.time = get_scenario_time();

   // Pack the body to structural reference frame attitude quaternion.
   body_wrt_struct.scalar = physical_data->body_wrt_struct.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      body_wrt_struct.vector[iinc] = physical_data->body_wrt_struct.vector[iinc];
   }

   // Print out debug information if desired.
   if ( debug ) {
      cout.precision( 15 );
      cout << "PhysicalEntity::pack():" << __LINE__ << endl
           << "\tObject-Name: '" << object->get_name() << "'" << endl
           << "\tname:   '" << ( this->name != NULL ? this->name : "" ) << "'" << endl
           << "\ttype:   '" << ( this->type != NULL ? this->type : "" ) << "'" << endl
           << "\tstatus: '" << ( this->status != NULL ? this->status : "" ) << "'" << endl
           << "\tparent: '" << ( this->parent_ref_frame != NULL ? this->parent_ref_frame : "" ) << "'" << endl
           << "\ttime: " << state.time << endl
           << "\tposition: " << endl
           << "\t\t" << state.pos[0] << endl
           << "\t\t" << state.pos[1] << endl
           << "\t\t" << state.pos[2] << endl
           << "\tattitude (quaternion:s,v): " << endl
           << "\t\t" << state.quat_scalar << endl
           << "\t\t" << state.quat_vector[0] << endl
           << "\t\t" << state.quat_vector[1] << endl
           << "\t\t" << state.quat_vector[2] << endl
           << endl;
   }

   // Encode the data into the buffer.
   stc_encoder.encode();
   quat_encoder.encode();

   return;
}



void PhysicalEntity::unpack()
{
   // double dt; // Local vs. remote time difference.

   if ( !initialized ) {
      cout << "PhysicalEntity::unpack():" << __LINE__
           << " ERROR: The initialize() function has not been called!" << endl;
   }

   // Use the HLA encoder helpers to decode the PhysicalEntity fixed record.
   stc_encoder.decode();
   quat_encoder.decode();

   // If the HLA attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value.  If we locally own the attribute then we do not want to
   // override it's value.  If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state.  We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.
   if ( entity_attr->is_received() ) {
      // Print out debug information if desired.
      if ( debug ) {
         cout.precision( 15 );
         cout << "PhysicalEntity::pack():" << __LINE__ << endl
              << "\tObject-Name: '" << object->get_name() << "'" << endl
              << "\tname:   '" << ( this->name != NULL ? this->name : "" ) << "'" << endl
              << "\type:    '" << ( this->type != NULL ? this->type : "" ) << "'" << endl
              << "\tstatus: '" << ( this->status != NULL ? this->status : "" ) << "'" << endl
              << "\tparent: '" << ( this->parent_ref_frame != NULL ? this->parent_ref_frame : "" ) << "'" << endl
              << "\ttime: " << state.time << endl
              << "\tposition: " << endl
              << "\t\t" << state.pos[0] << endl
              << "\t\t" << state.pos[1] << endl
              << "\t\t" << state.pos[2] << endl
              << "\tattitude (quaternion:s,v): " << endl
              << "\t\t" << state.quat_scalar << endl
              << "\t\t" << state.quat_vector[0] << endl
              << "\t\t" << state.quat_vector[1] << endl
              << "\t\t" << state.quat_vector[2] << endl
              << endl;
      }

      // Unpack the data.
      // Position and velocity vectors.
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->state.pos[iinc] = state.pos[iinc];
         physical_data->state.vel[iinc] = state.vel[iinc];
      }
      // Attitude quaternion.
      physical_data->state.quat_scalar = state.quat_scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->state.quat_vector[iinc] = state.quat_vector[iinc];
         physical_data->state.ang_vel[iinc]     = state.ang_vel[iinc];
      }
      // Time tag for this state data.
      physical_data->state.time = state.time;

      // Body to structure frame orientation.
      physical_data->body_wrt_struct.scalar = body_wrt_struct.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->body_wrt_struct.vector[iinc] = body_wrt_struct.vector[iinc];
      }

      // Set the entity name, type, status, and parent frame name.
      if ( physical_data->name != NULL ) {
         free( physical_data->name );
         physical_data->name = NULL;
      }
      physical_data->name = strdup( this->name );

      if ( physical_data->type != NULL ) {
         free( physical_data->type );
         physical_data->type = NULL;
      }
      physical_data->type = strdup( this->type );

      if ( physical_data->status != NULL ) {
         free( physical_data->status );
         physical_data->status = NULL;
      }
      physical_data->status = strdup( this->status );

      if ( physical_data->parent_frame != NULL ) {
         free( physical_data->parent_frame );
         physical_data->parent_frame = NULL;
      }
      if ( this->parent_ref_frame != NULL ) {
         if ( this->parent_ref_frame[0] != '\0' ) {
            physical_data->parent_frame = strdup( this->parent_ref_frame );
         }
      }
   }
   return;
}

