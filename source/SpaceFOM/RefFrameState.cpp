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
#include "SpaceFOM/RefFrameState.hh"

using namespace std;
using namespace SpaceFOM;

#define REF_FRAME_PACKING_DEBUG 0
#define REF_FRAME_PACKING_EXTRA_DEBUG 0

/*!
 * @job_class{initialization}
 */
RefFrameState::RefFrameState()
   : ref_frame_data(NULL)
{
   return;
}

/*!
 * @job_class{shutdown}
 */
RefFrameState::~RefFrameState()
{
}

/*!
 * @job_class{initialization}
 */
void RefFrameState::initialize(
   RefFrameData *ref_frame_data_ptr )
{

   // Set the reference to the reference frame.
   if ( ref_frame_data_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameState::initialize():" << __LINE__
             << " ERROR: Unexpected NULL reference frame: " << this->name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->ref_frame_data = ref_frame_data_ptr;

   // Mark this as initialized.
   RefFrameBase::initialize();

   // Return to calling routine.
   return;
}

void RefFrameState::pack()
{
   ostringstream errmsg;
   int iinc;

   // Check for initialization.
   if ( !initialized ) {
      cout << "RefFrameState::pack() ERROR: The initialize() function has not"
           << " been called!" << endl;
   }

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Check for name change.
   if ( ref_frame_data->name != NULL ) {
      if ( strcmp(ref_frame_data->name, name) ){
         trick_MM->delete_var( (void *)name );
         name = trick_MM->mm_strdup( ref_frame_data->name  );
      }
   }
   else {
      errmsg << "SpaceFOM::RefFrameState::pack():" << __LINE__
             << " ERROR: Unexpected NULL name for ReferenceFrame!" << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check for parent frame change.
   if ( ref_frame_data->parent_name != NULL ) {
      if( parent_name != NULL ){
         // We have a parent frame; so, check to see if frame names are different.
         if ( strcmp(ref_frame_data->parent_name, parent_name) ){
            // Frames are different, so reassign the new frame string.
            trick_MM->delete_var( (void *)parent_name );
            parent_name = trick_MM->mm_strdup( ref_frame_data->parent_name  );
         }
      }
      else{
         parent_name = trick_MM->mm_strdup( ref_frame_data->parent_name  );
      }
   }
   else {
      if( parent_name != NULL ){
         trick_MM->delete_var( (void *)parent_name );
         parent_name = NULL;
      }
   }

   // Pack the data.
   // Position and velocity vectors.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      stc_data.pos[iinc] = ref_frame_data->state.pos[iinc];
      stc_data.vel[iinc] = ref_frame_data->state.vel[iinc];
   }
   // Attitude quaternion.
   stc_data.quat_scalar = ref_frame_data->state.quat_scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      stc_data.quat_vector[iinc] = ref_frame_data->state.quat_vector[iinc];
      stc_data.ang_vel[iinc]     = ref_frame_data->state.ang_vel[iinc];
   }
   // Time tag for this state data.
   // stc_data.time = ref_frame->state.time;
   this->time = get_scenario_time();
   stc_data.time = ref_frame_data->state.time = this->time;

   // Print out debug information if desired.
   if ( debug ) {
      cout.precision( 15 );
      cout << "RefFrameState::pack():" << __LINE__ << endl
           << "\tObject-Name: '" << object->get_name() << "'" << endl
           << "\tname: '" << ( this->name != NULL ? this->name : "" ) << "'" << endl
           << "\tparent_name: '" << ( this->parent_name != NULL ? this->parent_name : "" ) << "'" << endl
           << "\ttime: " << stc_data.time << endl
           << "\tposition: " << endl
           << "\t\t" << stc_data.pos[0] << endl
           << "\t\t" << stc_data.pos[1] << endl
           << "\t\t" << stc_data.pos[2] << endl
           << endl;
   }

   // Encode the data into the reference frame buffer.
   stc_encoder.encode();

   return;
}

void RefFrameState::unpack()
{
   // double dt; // Local vs. remote time difference.

   if ( !initialized ) {
      cout << "RefFrameState::unpack():" << __LINE__
           << " ERROR: The initialize() function has not been called!" << endl;
   }

   // Use the HLA encoder helpers to decode the reference frame fixed record.
   stc_encoder.decode();

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
         if ( !strcmp(ref_frame_data->name, name ) ){
            trick_MM->delete_var( (void *)ref_frame_data->name );
            ref_frame_data->name = trick_MM->mm_strdup( name );
         }
      }
      else {
         ref_frame_data->name = trick_MM->mm_strdup( name );
      }
   }

   if ( parent_name_attr->is_received() ) {
      if ( ref_frame_data->parent_name != NULL ) {
         if ( !strcmp(ref_frame_data->parent_name, parent_name ) ){
            trick_MM->delete_var( (void *)ref_frame_data->parent_name );
            if ( parent_name[0] != '\0' ) {
               ref_frame_data->parent_name = trick_MM->mm_strdup( parent_name );
            }
            else{
               ref_frame_data->parent_name = NULL;
            }
         }
      }
      else {
         if ( parent_name[0] != '\0' ) {
            ref_frame_data->parent_name = trick_MM->mm_strdup( parent_name );
         }
      }
   }

   // Unpack the ReferenceFrame space-time coordinate state.
   if ( state_attr->is_received() ) {

      // Unpack the data.
      // Position and velocity vectors.
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         ref_frame_data->state.pos[iinc] = stc_data.pos[iinc];
         ref_frame_data->state.vel[iinc] = stc_data.vel[iinc];
      }
      // Attitude quaternion.
      ref_frame_data->state.quat_scalar = stc_data.quat_scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         ref_frame_data->state.quat_vector[iinc] = stc_data.quat_vector[iinc];
         ref_frame_data->state.ang_vel[iinc]     = stc_data.ang_vel[iinc];
      }
      // Time tag for this state data.
      this->time = stc_data.time;
      ref_frame_data->state.time = stc_data.time;

   }

   // Print out debug information if desired.
   if ( debug ) {
      cout.precision( 15 );
      cout << "RefFrameState::unpack():" << __LINE__ << endl
           << "\tObject-Name: '" << object->get_name() << "'" << endl
           << "\tname: '" << ( this->name != NULL ? this->name : "" ) << "'" << endl
           << "\tparent_name: '" << ( this->parent_name != NULL ? this->parent_name : "" ) << "'" << endl
           << "\ttime: " << stc_data.time << endl
           << "\tposition: " << endl
           << "\t\t" << stc_data.pos[0] << endl
           << "\t\t" << stc_data.pos[1] << endl
           << "\t\t" << stc_data.pos[2] << endl
           << endl;
   }

   return;
}
