/*!
@file JEOD/JEODRefFrameState.cpp
@ingroup JEOD
@brief This class provides data packing for the SpaceFOM Reference Frames and
the interface with a JEOD Reference Frame State instance.

This is the base implementation for the Space Reference FOM (SpaceFOM)
interface to the SpaceFOM Reference Frame object.  This class provides the
interface code to encode and decode SpaceFOM Reference Frame Objects and
JEOD Reference Frame State instances. This needs to be available to the
SpaceFOM initialization process for the root reference frame discovery step
in the initialization process.

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
@trick_link_dependency{../SpaceFOM/RefFrameBase.cpp}
@trick_link_dependency{JEODRefFrameState.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, May 2023, --, Based off of RefFrameBase.}
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
#include "JEOD/JEODRefFrameState.hh"

using namespace std;
using namespace SpaceFOM;

#define REF_FRAME_PACKING_DEBUG 0
#define REF_FRAME_PACKING_EXTRA_DEBUG 0

/*!
 * @job_class{initialization}
 */
JEODRefFrameState::JEODRefFrameState()
   : RefFrameBase(),
     time_tt( NULL ),
     ref_frame_state( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
JEODRefFrameState::~JEODRefFrameState()
{
}

/*!
 * @job_class{initialization}
 */
void JEODRefFrameState::initialize(
   jeod::TimeTT        &time_tt_in,
   jeod::RefFrameState *ref_frame_state_ptr )
{
   // Must have federation instance name.
   if ( this->name == NULL ) {
      if ( debug ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::JEODRefFrameState::initialize():" << __LINE__
                << " WARNING: Unexpected NULL federation instance frame name!"
                << "  Setting frame name to empty string." << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );
      }
      this->name = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance parent frame name.
   if ( this->parent_name == NULL ) {
      if ( debug ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::JEODRefFrameState::initialize():" << __LINE__
                << " WARNING: Unexpected NULL federation instance parent frame name!"
                << "  Setting parent frame name to empty string." << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );
      }
      this->parent_name = trick_MM->mm_strdup( "" );
   }

   // Set the reference to the reference frame.
   if ( ref_frame_state_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODRefFrameState::initialize():" << __LINE__
             << " ERROR: Unexpected NULL reference frame: " << this->name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->ref_frame_state = ref_frame_state_ptr;

   // Set the JEOD time reference.
   time_tt = &time_tt_in;

   // Mark this as initialized.
   RefFrameBase::initialize();

   // Return to calling routine.
   return;
}

void JEODRefFrameState::pack()
{
   int iinc;

   // Check for initialization.
   if ( !initialized ) {
      cout << "JEODRefFrameState::pack() ERROR: The initialize() function has not"
           << " been called!" << endl;
   }

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Pack the data.
   // Position and velocity vectors.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      stc_data.pos[iinc] = ref_frame_state->trans.position[iinc];
      stc_data.vel[iinc] = ref_frame_state->trans.velocity[iinc];
   }
   // Attitude quaternion.
   stc_data.quat_scalar = ref_frame_state->rot.Q_parent_this.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      stc_data.quat_vector[iinc] = ref_frame_state->rot.Q_parent_this.vector[iinc];
      stc_data.ang_vel[iinc]     = ref_frame_state->rot.ang_vel_this[iinc];
   }
   // Time tag for this state data.
   // stc_data.time = ref_frame->state.time;
   // FIXME: Need to check if get_scenario_time is really what we want here?
   this->time = get_scenario_time();
   stc_data.time = this->time;

   // Print out debug information if desired.
   if ( debug ) {
      cout.precision( 15 );
      cout << "JEODRefFrameState::pack():" << __LINE__ << endl
           << "\tObject-Name: '" << object->get_name() << "'" << endl
           << "\tname: '" << ( this->name != NULL ? this->name : "" ) << "'" << endl
           << "\tparent_name: '" << ( this->parent_name != NULL ? this->parent_name : "" ) << "'" << endl
           << "\ttime: " << stc_data.time << endl
           << "\tposition: " << endl
           << "\t\t" << stc_data.pos[0] << endl
           << "\t\t" << stc_data.pos[1] << endl
           << "\t\t" << stc_data.pos[2] << endl
           << "\tattitude (quaternion:s,v): " << endl
           << "\t\t" << stc_data.quat_scalar << endl
           << "\t\t" << stc_data.quat_vector[0] << endl
           << "\t\t" << stc_data.quat_vector[1] << endl
           << "\t\t" << stc_data.quat_vector[2] << endl
           << endl;
   }
   if ( debug ) {
      cout << "JEODRefFrameState::pack():" << __LINE__ << endl
           << "\tSim Sec: " << exec_get_sim_time() << endl
           << "\tSeconds: " << ( time_tt->trunc_julian_time * 86400.0 ) << endl
           << "\tDate: " << time_tt->calendar_year
           << "-" << time_tt->calendar_month
           << "-" << time_tt->calendar_day
           << "::" << time_tt->calendar_hour
           << ":" << time_tt->calendar_minute
           << ":" << time_tt->calendar_second << endl
           << endl;
   }

   // Encode the data into the reference frame buffer.
   stc_encoder.encode();

   return;
}

void JEODRefFrameState::unpack()
{
   // double dt; // Local vs. remote time difference.

   if ( !initialized ) {
      cout << "JEODRefFrameState::unpack():" << __LINE__
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
      // NOTE: We don't currently support renaming a ReferenceFrame for JEOD
      // based applications.  The changed name is updated in the RefFrameBase
      // name attribute but we do not do anything with it now.
   }

   if ( parent_name_attr->is_received() ) {
      // NOTE: We don't currently support reparenting a ReferenceFrame for JEOD
      // based applications.  The changed the ReferencFrame parent name is
      // ignored for now.
   }

   // Unpack the ReferenceFrame space-time coordinate state.
   if ( state_attr->is_received() ) {

      // Unpack the data.
      // Position and velocity vectors.
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         ref_frame_state->trans.position[iinc] = stc_data.pos[iinc];
         ref_frame_state->trans.velocity[iinc] = stc_data.vel[iinc];
      }
      // Attitude quaternion.
      ref_frame_state->rot.Q_parent_this.scalar = stc_data.quat_scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         ref_frame_state->rot.Q_parent_this.vector[iinc] = stc_data.quat_vector[iinc];
         ref_frame_state->rot.ang_vel_this[iinc]         = stc_data.ang_vel[iinc];
      }
      // Time tag for this state data.
      this->time = stc_data.time;
   }

   // The frame name and parent name are already 'unpacked'.
   // Print out debug information if desired.
   if ( debug ) {
      cout.precision( 15 );
      cout << "JEODRefFrameState::unpack():" << __LINE__ << endl
           << "\tObject-Name: '" << object->get_name() << "'" << endl
           << "\tname: '" << ( this->name != NULL ? this->name : "" ) << "'" << endl
           << "\tparent_name: '" << ( this->parent_name != NULL ? this->parent_name : "" ) << "'" << endl
           << "\ttime: " << stc_data.time << endl
           << "\tposition: " << endl
           << "\t\t" << stc_data.pos[0] << endl
           << "\t\t" << stc_data.pos[1] << endl
           << "\t\t" << stc_data.pos[2] << endl
           << "\tattitude (quaternion:s,v): " << endl
           << "\t\t" << stc_data.quat_scalar << endl
           << "\t\t" << stc_data.quat_vector[0] << endl
           << "\t\t" << stc_data.quat_vector[1] << endl
           << "\t\t" << stc_data.quat_vector[2] << endl
           << endl;
   }
   if ( debug ) {
      cout << "JEODRefFrameState::unpack():" << __LINE__ << endl
           << "\tSim Sec: " << exec_get_sim_time() << endl
           << "\tSeconds: " << ( time_tt->trunc_julian_time * 86400.0 ) << endl
           << "\tDate: " << time_tt->calendar_year
           << "-" << time_tt->calendar_month
           << "-" << time_tt->calendar_day
           << "::" << time_tt->calendar_hour
           << ":" << time_tt->calendar_minute
           << ":" << time_tt->calendar_second << endl
           << endl;
   }

   return;
}
