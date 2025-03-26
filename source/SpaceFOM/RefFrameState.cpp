/*!
@file SpaceFOM/RefFrameState.cpp
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM Reference Frames.

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
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Object.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{ExecutionControl.cpp}
@trick_link_dependency{RefFrameBase.cpp}
@trick_link_dependency{RefFrameState.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2023, --, Refactored to use pure virtual base class.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"

// TrickHLA model include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/ExecutionControl.hh"
#include "SpaceFOM/RefFrameBase.hh"
#include "SpaceFOM/RefFrameState.hh"

using namespace std;
using namespace SpaceFOM;

#define REF_FRAME_PACKING_DEBUG 0
#define REF_FRAME_PACKING_EXTRA_DEBUG 0

/*!
 * @job_class{initialization}
 */
RefFrameState::RefFrameState()
   : RefFrameBase(),
     ref_frame_data( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
RefFrameState::RefFrameState(
   RefFrameData &ref_frame_data_ref )
   : RefFrameBase(),
     ref_frame_data( &ref_frame_data_ref )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
RefFrameState::~RefFrameState()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameState::configure(
   RefFrameData *ref_frame_data_ptr )
{
   // First call the base class pre_initialize function.
   RefFrameBase::configure();

   // Set the reference to the reference frame.
   if ( ref_frame_data_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameState::pre_initialize():" << __LINE__
             << " ERROR: Unexpected NULL reference frame: " << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->ref_frame_data = ref_frame_data_ptr;

   // Return to calling routine.
   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameState::initialize()
{
   // Set the reference to the reference frame.
   if ( ref_frame_data == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameState::initialize():" << __LINE__
             << " ERROR: Unexpected NULL reference frame: " << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Mark this as initialized.
   RefFrameBase::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameState::pack_from_working_data()
{
   int iinc;

   // Check for parent frame change.
   if ( ref_frame_data->parent_name != NULL ) {
      if ( packing_data.parent_name != NULL ) {
         // We have a parent frame; so, check to see if frame names are different.
         if ( strcmp( ref_frame_data->parent_name, packing_data.parent_name ) ) {
            // Frames are different, so reassign the new frame string.
            if ( trick_MM->delete_var( static_cast< void * >( packing_data.parent_name ) ) ) {
               message_publish( MSG_WARNING, "RefFrameState::pack_from_working_data():%d WARNING failed to delete Trick Memory for 'packing_data.parent_name'\n",
                                __LINE__ );
            }
            packing_data.parent_name = trick_MM->mm_strdup( ref_frame_data->parent_name );
         }
      } else {
         packing_data.parent_name = trick_MM->mm_strdup( ref_frame_data->parent_name );
      }
   } else {
      if ( packing_data.parent_name != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( packing_data.parent_name ) ) ) {
            message_publish( MSG_WARNING, "RefFrameState::pack_from_working_data():%d WARNING failed to delete Trick Memory for 'packing_data.parent_name'\n",
                             __LINE__ );
         }
         // For a NULL parent frame, we must pack an 'empty' string.
         packing_data.parent_name = trick_MM->mm_strdup( "" );
      }
   }

   // Pack the data.
   // Position and velocity vectors.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      packing_data.state.pos[iinc] = ref_frame_data->state.pos[iinc];
      packing_data.state.vel[iinc] = ref_frame_data->state.vel[iinc];
      packing_data.accel[iinc]     = ref_frame_data->accel[iinc];
   }
   // Attitude quaternion.
   packing_data.state.att.scalar = ref_frame_data->state.att.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      packing_data.state.att.vector[iinc] = ref_frame_data->state.att.vector[iinc];
      packing_data.state.ang_vel[iinc]    = ref_frame_data->state.ang_vel[iinc];
      packing_data.ang_accel[iinc]        = ref_frame_data->ang_accel[iinc];
   }
   // Time tag for this state data.
   packing_data.state.time = ref_frame_data->state.time = get_scenario_time();

   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameState::unpack_into_working_data()
{
   // If the HLA attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value.  If we locally own the attribute then we do not want to
   // override it's value.  If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state.  We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.

   // Set the reference frame name and parent frame name.
   if ( name_attr->is_received() ) {
      if ( ref_frame_data->name != NULL ) {
         if ( !strcmp( ref_frame_data->name, packing_data.name ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( ref_frame_data->name ) ) ) {
               message_publish( MSG_WARNING, "RefFrameState::unpack_into_working_data():%d WARNING failed to delete Trick Memory for 'ref_frame_data->name'\n",
                                __LINE__ );
            }
            ref_frame_data->name = trick_MM->mm_strdup( packing_data.name );
         }
      } else {
         ref_frame_data->name = trick_MM->mm_strdup( packing_data.name );
      }
   }

   if ( parent_name_attr->is_received() ) {
      if ( ref_frame_data->parent_name != NULL ) {
         if ( !strcmp( ref_frame_data->parent_name, packing_data.parent_name ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( ref_frame_data->parent_name ) ) ) {
               message_publish( MSG_WARNING, "RefFrameState::unpack_into_working_data():%d WARNING failed to delete Trick Memory for 'ref_frame_data->parent_name'\n",
                                __LINE__ );
            }

            if ( packing_data.parent_name[0] != '\0' ) {
               ref_frame_data->parent_name = trick_MM->mm_strdup( packing_data.parent_name );
            } else {
               ref_frame_data->parent_name = NULL;
            }
         }
      } else {
         if ( packing_data.parent_name[0] != '\0' ) {
            ref_frame_data->parent_name = trick_MM->mm_strdup( packing_data.parent_name );
         }
      }
   }

   // Unpack the ReferenceFrame space-time coordinate state.
   if ( state_attr->is_received() ) {

      // Unpack the data.
      // Position and velocity vectors.
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         ref_frame_data->state.pos[iinc] = packing_data.state.pos[iinc];
         ref_frame_data->state.vel[iinc] = packing_data.state.vel[iinc];
      }
      // Attitude quaternion.
      ref_frame_data->state.att.scalar = packing_data.state.att.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         ref_frame_data->state.att.vector[iinc] = packing_data.state.att.vector[iinc];
         ref_frame_data->state.ang_vel[iinc]    = packing_data.state.ang_vel[iinc];
      }
      // Time tag for this state data.
      ref_frame_data->state.time = packing_data.state.time;
   }

   // FIXME: Need to support acceleration data.
   // if ( accel_attr->is_received() ) {
   //   for ( int iinc = 0; iinc < 3; ++iinc ) {
   //      ref_frame_data->accel[iinc] = packing_data.accel[iinc];
   //   }
   //}
   // if ( ang_accel_attr->is_received() ) {
   //   for ( int iinc = 0; iinc < 3; ++iinc ) {
   //    ref_frame_data->ang_accel[iinc] = packing_data.ang_accel[iinc];
   //   }
   //}

   return;
}
