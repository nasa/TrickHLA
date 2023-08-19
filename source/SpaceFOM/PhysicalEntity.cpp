/*!
@file SpaceFOM/PhysicalEntity.cpp
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM PhysicalEntities.

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
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{PhysicalEntity.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Cleaned up and filled out.}
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
             << " ERROR: Unexpected NULL PhysicalEntityData: " << name << THLA_ENDL;
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
             << " ERROR: Unexpected NULL PhysicalEntityData: " << name << THLA_ENDL;
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
   ostringstream errmsg;
   int           iinc;

   // Check for initialization.
   if ( !initialized ) {
      cout << "JEODPhysicalEntity::pack() ERROR: The initialize() function has not"
           << " been called!" << endl;
   }

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Check for name change.
   if ( physical_data->name != NULL ) {
      if ( strcmp( physical_data->name, name ) ) {
         trick_MM->delete_var( (void *)name );
         name = trick_MM->mm_strdup( physical_data->name );
      }
   } else {
      errmsg << "SpaceFOM::PhysicalEntity::pack():" << __LINE__
             << " ERROR: Unexpected NULL name for PhysicalEntity!" << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check for type change.
   if ( physical_data->type != NULL ) {
      if ( type != NULL ) {
         if ( strcmp( physical_data->type, type ) ) {
            trick_MM->delete_var( (void *)type );
            type = trick_MM->mm_strdup( physical_data->type );
         }
      } else {
         type = trick_MM->mm_strdup( physical_data->type );
      }
   } else {
      if ( type != NULL ) {
         trick_MM->delete_var( (void *)type );
         type = NULL;
      }
   }

   // Check for status change.
   if ( physical_data->status != NULL ) {
      if ( status != NULL ) {
         if ( strcmp( physical_data->status, status ) ) {
            trick_MM->delete_var( (void *)status );
            status = trick_MM->mm_strdup( physical_data->status );
         }
      } else {
         status = trick_MM->mm_strdup( physical_data->status );
      }
   } else {
      if ( status != NULL ) {
         trick_MM->delete_var( (void *)status );
         status = NULL;
      }
   }

   // Check for parent frame change.
   if ( physical_data->parent_frame != NULL ) {
      // We have a parent frame; so, check to see if frame names are different.
      if ( strcmp( physical_data->parent_frame, parent_frame ) ) {
         // Frames are different, so reassign the new frame string.
         trick_MM->delete_var( (void *)parent_frame );
         parent_frame = trick_MM->mm_strdup( physical_data->parent_frame );
      }
   } else {
      errmsg << "SpaceFOM::PhysicalEntity::pack():" << __LINE__
             << " ERROR: Unexpected NULL parent frame for PhysicalEntity: "
             << physical_data->name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

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

   // Set the acceleration data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      accel[iinc] = physical_data->accel[iinc];
   }

   // Set the rotational acceleration data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      rot_accel[iinc] = physical_data->rot_accel[iinc];
   }

   // Set the center of mass location.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      cm[iinc] = physical_data->cm[iinc];
   }

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
           << "\tname:   '" << ( name != NULL ? name : "" ) << "'" << endl
           << "\ttype:   '" << ( type != NULL ? type : "" ) << "'" << endl
           << "\tstatus: '" << ( status != NULL ? status : "" ) << "'" << endl
           << "\tparent: '" << ( parent_frame != NULL ? parent_frame : "" ) << "'" << endl
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

   // Unpack the space-time coordinate state data.
   if ( state_attr->is_received() ) {

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
   }

   // Set the entity name, type, status, and parent frame name.
   if ( name_attr->is_received() ) {
      if ( physical_data->name != NULL ) {
         if ( !strcmp( physical_data->name, name ) ) {
            trick_MM->delete_var( (void *)physical_data->name );
            physical_data->name = trick_MM->mm_strdup( name );
         }
      } else {
         physical_data->name = trick_MM->mm_strdup( name );
      }
   }

   if ( type_attr->is_received() ) {
      if ( physical_data->type != NULL ) {
         if ( !strcmp( physical_data->type, type ) ) {
            trick_MM->delete_var( (void *)physical_data->type );
            physical_data->type = trick_MM->mm_strdup( type );
         }
      } else {
         physical_data->type = trick_MM->mm_strdup( type );
      }
   }

   if ( status_attr->is_received() ) {
      if ( physical_data->status != NULL ) {
         if ( !strcmp( physical_data->status, status ) ) {
            trick_MM->delete_var( (void *)physical_data->status );
            physical_data->status = trick_MM->mm_strdup( status );
         }
      } else {
         physical_data->status = trick_MM->mm_strdup( status );
      }
   }

   if ( parent_frame_attr->is_received() ) {
      if ( physical_data->parent_frame != NULL ) {
         if ( !strcmp( physical_data->parent_frame, parent_frame ) ) {
            trick_MM->delete_var( (void *)physical_data->parent_frame );
            if ( parent_frame[0] != '\0' ) {
               physical_data->parent_frame = trick_MM->mm_strdup( parent_frame );
            } else {
               physical_data->parent_frame = NULL;
            }
         }
      } else {
         if ( parent_frame[0] != '\0' ) {
            physical_data->parent_frame = trick_MM->mm_strdup( parent_frame );
         }
      }
   }

   // Unpack the body to structural attitude data.
   if ( body_frame_attr->is_received() ) {

      // Body to structure frame orientation.
      physical_data->body_wrt_struct.scalar = body_wrt_struct.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->body_wrt_struct.vector[iinc] = body_wrt_struct.vector[iinc];
      }
   }

   // Print out debug information if desired.
   if ( debug ) {
      cout.precision( 15 );
      cout << "PhysicalEntity::unpack():" << __LINE__ << endl
           << "\tObject-Name: '" << object->get_name() << "'" << endl
           << "\tname:   '" << ( name != NULL ? name : "" ) << "'" << endl
           << "\ttype:   '" << ( type != NULL ? type : "" ) << "'" << endl
           << "\tstatus: '" << ( status != NULL ? status : "" ) << "'" << endl
           << "\tparent: '" << ( parent_frame != NULL ? parent_frame : "" ) << "'" << endl
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

   return;
}
