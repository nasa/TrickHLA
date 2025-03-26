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
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Object.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{PhysicalEntity.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Cleaned up and filled out.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Refactored.}
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
   return;
}

/*!
 * @job_class{initialization}
 */
PhysicalEntity::PhysicalEntity( PhysicalEntityData &physical_data_ref ) // RETURN: -- None.
   : physical_data( &physical_data_ref )
{
   return;
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
void PhysicalEntity::configure( PhysicalEntityData *physical_data_ptr )
{
   // First call the base class pre_initialize function.
   PhysicalEntityBase::configure();

   // Set the reference to the PhysicalEntity data.
   if ( physical_data_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL PhysicalEntityData: "
             << pe_packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->physical_data = physical_data_ptr;

   // Return to calling routine.
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntity::initialize()
{
   // Check to make sure the PhysicalEntity data is set.
   if ( physical_data == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL PhysicalEntityData: "
             << pe_packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Mark this as initialized.
   PhysicalEntityBase::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntity::pack_from_working_data()
{
   int iinc;

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Check for name change.
   if ( physical_data->name != NULL ) {

      if ( pe_packing_data.name != NULL ) {

         // Compare names.
         if ( strcmp( physical_data->name, pe_packing_data.name ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( pe_packing_data.name ) ) ) {
               message_publish( MSG_WARNING, "PhysicalEntity::pack_from_working_data():%d WARNING failed to delete Trick Memory for 'pe_packing_data.name'\n",
                                __LINE__ );
            }
            pe_packing_data.name = trick_MM->mm_strdup( physical_data->name );
         }

      } else {
         // No name to compare so copy name.

         pe_packing_data.name = trick_MM->mm_strdup( physical_data->name );
      }

   } else {
      // This is bad scoobies so just punt.

      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalEntity::copy_working_data():" << __LINE__
             << " ERROR: Unexpected NULL name for PhysicalEntity!\n";
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check for type change.
   if ( physical_data->type != NULL ) {
      if ( pe_packing_data.type != NULL ) {
         if ( strcmp( physical_data->type, pe_packing_data.type ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( pe_packing_data.type ) ) ) {
               message_publish( MSG_WARNING, "PhysicalEntity::copy_working_data():%d WARNING failed to delete Trick Memory for 'pe_packing_data.type'\n",
                                __LINE__ );
            }
            pe_packing_data.type = trick_MM->mm_strdup( physical_data->type );
         }
      } else {
         pe_packing_data.type = trick_MM->mm_strdup( physical_data->type );
      }
   } else {
      if ( pe_packing_data.type != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( pe_packing_data.type ) ) ) {
            message_publish( MSG_WARNING, "PhysicalEntity::copy_working_data():%d WARNING failed to delete Trick Memory for 'pe_packing_data.type'\n",
                             __LINE__ );
         }
         pe_packing_data.type = NULL;
      }
   }

   // Check for status change.
   if ( physical_data->status != NULL ) {
      if ( pe_packing_data.status != NULL ) {
         if ( strcmp( physical_data->status, pe_packing_data.status ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( pe_packing_data.status ) ) ) {
               message_publish( MSG_WARNING, "PhysicalEntity::copy_working_data():%d WARNING failed to delete Trick Memory for 'pe_packing_data.status'\n",
                                __LINE__ );
            }
            pe_packing_data.status = trick_MM->mm_strdup( physical_data->status );
         }
      } else {
         pe_packing_data.status = trick_MM->mm_strdup( physical_data->status );
      }
   } else {
      if ( pe_packing_data.status != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( pe_packing_data.status ) ) ) {
            message_publish( MSG_WARNING, "PhysicalEntity::copy_working_data():%d WARNING failed to delete Trick Memory for 'pe_packing_data.status'\n",
                             __LINE__ );
         }
         pe_packing_data.status = NULL;
      }
   }

   // Check for parent frame change.
   if ( physical_data->parent_frame != NULL ) {

      if ( pe_packing_data.parent_frame != NULL ) {

         // We have a parent frame; so, check to see if frame names are different.
         if ( strcmp( physical_data->parent_frame, pe_packing_data.parent_frame ) ) {
            // Frames are different, so reassign the new frame string.
            if ( trick_MM->delete_var( static_cast< void * >( pe_packing_data.parent_frame ) ) ) {
               message_publish( MSG_WARNING, "PhysicalEntity::copy_working_data():%d WARNING failed to delete Trick Memory for 'pe_packing_data.parent_frame'\n",
                                __LINE__ );
            }
            pe_packing_data.parent_frame = trick_MM->mm_strdup( physical_data->parent_frame );
         }

      } else {
         // No parent frame name to compare so copy name.
         pe_packing_data.parent_frame = trick_MM->mm_strdup( physical_data->parent_frame );
      }

   } else {
      // This is bad scoobies so just punt.
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalEntity::copy_working_data():" << __LINE__
             << " ERROR: Unexpected NULL parent frame for PhysicalEntity: "
             << physical_data->name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Pack the state time coordinate data.
   // Position and velocity vectors.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      pe_packing_data.state.pos[iinc] = physical_data->state.pos[iinc];
      pe_packing_data.state.vel[iinc] = physical_data->state.vel[iinc];
   }
   // Attitude quaternion.
   pe_packing_data.state.att.scalar = physical_data->state.att.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      pe_packing_data.state.att.vector[iinc] = physical_data->state.att.vector[iinc];
      pe_packing_data.state.ang_vel[iinc]    = physical_data->state.ang_vel[iinc];
   }

   // Time tag for this state data.
   pe_packing_data.state.time = get_scenario_time();

   // Set the acceleration data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      pe_packing_data.accel[iinc] = physical_data->accel[iinc];
   }

   // Set the rotational acceleration data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      pe_packing_data.ang_accel[iinc] = physical_data->ang_accel[iinc];
   }

   // Set the center of mass location.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      pe_packing_data.cm[iinc] = physical_data->cm[iinc];
   }

   // Pack the body to structural reference frame attitude quaternion.
   pe_packing_data.body_wrt_struct.scalar = physical_data->body_wrt_struct.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      pe_packing_data.body_wrt_struct.vector[iinc] = physical_data->body_wrt_struct.vector[iinc];
   }

   // Return to the calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntity::unpack_into_working_data()
{
   // If the HLA attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value.  If we locally own the attribute then we do not want to
   // override it's value.  If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state.  We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.

   // Set the entity name, type, status, and parent frame name.
   if ( name_attr->is_received() ) {
      if ( physical_data->name != NULL ) {
         if ( !strcmp( physical_data->name, pe_packing_data.name ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( physical_data->name ) ) ) {
               message_publish( MSG_WARNING, "PhysicalEntity::unpack_into_working_data():%d WARNING failed to delete Trick Memory for 'physical_data->name'\n",
                                __LINE__ );
            }
            physical_data->name = trick_MM->mm_strdup( pe_packing_data.name );
         }
      } else {
         physical_data->name = trick_MM->mm_strdup( pe_packing_data.name );
      }
   }

   if ( type_attr->is_received() ) {
      if ( physical_data->type != NULL ) {
         if ( !strcmp( physical_data->type, pe_packing_data.type ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( physical_data->type ) ) ) {
               message_publish( MSG_WARNING, "PhysicalEntity::unpack_into_working_data():%d WARNING failed to delete Trick Memory for 'physical_data->type'\n",
                                __LINE__ );
            }
            physical_data->type = trick_MM->mm_strdup( pe_packing_data.type );
         }
      } else {
         physical_data->type = trick_MM->mm_strdup( pe_packing_data.type );
      }
   }

   if ( status_attr->is_received() ) {
      if ( physical_data->status != NULL ) {
         if ( !strcmp( physical_data->status, pe_packing_data.status ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( physical_data->status ) ) ) {
               message_publish( MSG_WARNING, "PhysicalEntity::unpack_into_working_data():%d WARNING failed to delete Trick Memory for 'physical_data->status'\n",
                                __LINE__ );
            }
            physical_data->status = trick_MM->mm_strdup( pe_packing_data.status );
         }
      } else {
         physical_data->status = trick_MM->mm_strdup( pe_packing_data.status );
      }
   }

   if ( parent_frame_attr->is_received() ) {
      if ( physical_data->parent_frame != NULL ) {
         if ( !strcmp( physical_data->parent_frame, pe_packing_data.parent_frame ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( physical_data->parent_frame ) ) ) {
               message_publish( MSG_WARNING, "PhysicalEntity::unpack_into_working_data():%d WARNING failed to delete Trick Memory for 'physical_data->parent_frame'\n",
                                __LINE__ );
            }
            if ( pe_packing_data.parent_frame[0] != '\0' ) {
               physical_data->parent_frame = trick_MM->mm_strdup( pe_packing_data.parent_frame );
            } else {
               physical_data->parent_frame = NULL;
            }
         }
      } else {
         if ( pe_packing_data.parent_frame[0] != '\0' ) {
            physical_data->parent_frame = trick_MM->mm_strdup( pe_packing_data.parent_frame );
         }
      }
   }

   // Unpack the space-time coordinate state data.
   if ( state_attr->is_received() ) {

      // Unpack the data.
      // Position and velocity vectors.
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->state.pos[iinc] = pe_packing_data.state.pos[iinc];
         physical_data->state.vel[iinc] = pe_packing_data.state.vel[iinc];
      }
      // Attitude quaternion.
      physical_data->state.att.scalar = pe_packing_data.state.att.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->state.att.vector[iinc] = pe_packing_data.state.att.vector[iinc];
         physical_data->state.ang_vel[iinc]    = pe_packing_data.state.ang_vel[iinc];
      }
      // Time tag for this state data.
      physical_data->state.time = pe_packing_data.state.time;
   }

   // Unpack the translational acceleration data.
   if ( accel_attr->is_received() ) {
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->accel[iinc] = pe_packing_data.accel[iinc];
      }
   }

   // Unpack the rotational acceleration data.
   if ( ang_accel_attr->is_received() ) {
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->ang_accel[iinc] = pe_packing_data.ang_accel[iinc];
      }
   }

   // Unpack the center of mass data.
   if ( cm_attr->is_received() ) {
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->cm[iinc] = pe_packing_data.cm[iinc];
      }
   }

   // Unpack the body to structural attitude data.
   if ( body_frame_attr->is_received() ) {
      // Body to structure frame orientation.
      physical_data->body_wrt_struct.scalar = pe_packing_data.body_wrt_struct.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         physical_data->body_wrt_struct.vector[iinc] = pe_packing_data.body_wrt_struct.vector[iinc];
      }
   }

   return;
}
