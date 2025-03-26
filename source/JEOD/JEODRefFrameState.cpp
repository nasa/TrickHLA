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

// JEOD include files.
#include "utils/orientation/include/orientation.hh"

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
 * @job_class{initialization}
 */
JEODRefFrameState::JEODRefFrameState(
   jeod::TimeTT        &time_tt_in,
   jeod::RefFrameState &ref_frame_state_ref )
   : RefFrameBase(),
     time_tt( &time_tt_in ),
     ref_frame_state( &ref_frame_state_ref )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
JEODRefFrameState::~JEODRefFrameState()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void JEODRefFrameState::configure(
   jeod::TimeTT        *time_tt_in,
   jeod::RefFrameState *ref_frame_state_ptr )
{
   // First call the base class pre_initialize function.
   RefFrameBase::configure();

   // Set the reference to the reference frame.
   if ( ref_frame_state_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODRefFrameState::pre_initialize():" << __LINE__
             << " ERROR: Unexpected NULL reference frame: " << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->ref_frame_state = ref_frame_state_ptr;

   // Set the JEOD time reference.
   if ( time_tt_in == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODRefFrameState::pre_initialize():" << __LINE__
             << " ERROR: Unexpected NULL time reference: " << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   time_tt = time_tt_in;
}

/*!
 * @job_class{initialization}
 */
void JEODRefFrameState::initialize()
{
   // Check for the reference frame data.
   if ( this->ref_frame_state == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODRefFrameState::initialize():" << __LINE__
             << " ERROR: Unexpected NULL reference frame data: " << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check for the JEOD time reference.
   if ( this->time_tt == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODRefFrameState::initialize():" << __LINE__
             << " ERROR: Unexpected NULL time reference: " << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Mark this as initialized.
   RefFrameBase::initialize();
}

/*!
 * @job_class{scheduled}
 */
void JEODRefFrameState::pack_from_working_data()
{
   int iinc;

   // Set the reference frame name and parent frame name.
   // if ( packing_data.name != NULL ) {
   // NOTE: We don't currently support renaming an ReferenceFrame for JEOD
   // based applications.  The changed name is updated in the RefFrameBase
   // name attribute but we do not do anything with it now.
   //}

   // if ( packing_data.parent_name != NULL ) {
   // NOTE: We don't currently support reparenting a ReferenceFrame for JEOD
   // based applications.  The changed the ReferencFrame parent name is
   // ignored for now.
   //}

   // Pack the data.
   // Position and velocity vectors.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      packing_data.state.pos[iinc] = ref_frame_state->trans.position[iinc];
      packing_data.state.vel[iinc] = ref_frame_state->trans.velocity[iinc];
   }
   // Attitude quaternion.
   packing_data.state.att.scalar = ref_frame_state->rot.Q_parent_this.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      packing_data.state.att.vector[iinc] = ref_frame_state->rot.Q_parent_this.vector[iinc];
      packing_data.state.ang_vel[iinc]    = ref_frame_state->rot.ang_vel_this[iinc];
   }
   // Time tag for this state data.
   // FIXME: Need to check if get_scenario_time is really what we want here?
   packing_data.state.time = get_scenario_time();

   if ( debug ) {
      ostringstream msg;
      msg << "JEODRefFrameState::pack_from_working_data():" << __LINE__ << '\n'
          << "\tSim Sec: " << exec_get_sim_time() << '\n'
          << "\tSeconds: " << ( time_tt->trunc_julian_time * 86400.0 ) << '\n'
          << "\tDate: " << time_tt->calendar_year
          << "-" << time_tt->calendar_month
          << "-" << time_tt->calendar_day
          << "::" << time_tt->calendar_hour
          << ":" << time_tt->calendar_minute
          << ":" << time_tt->calendar_second << '\n'
          << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*!
 * @job_class{scheduled}
 */
void JEODRefFrameState::unpack_into_working_data()
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
         ref_frame_state->trans.position[iinc] = packing_data.state.pos[iinc];
         ref_frame_state->trans.velocity[iinc] = packing_data.state.vel[iinc];
      }
      // Attitude quaternion.
      ref_frame_state->rot.Q_parent_this.scalar = packing_data.state.att.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         ref_frame_state->rot.Q_parent_this.vector[iinc] = packing_data.state.att.vector[iinc];
         ref_frame_state->rot.ang_vel_this[iinc]         = packing_data.state.ang_vel[iinc];
      }
   }

   if ( debug ) {
      ostringstream msg;
      msg << "JEODRefFrameState::unpack_into_working_data():" << __LINE__ << '\n'
          << "\tSim Sec: " << exec_get_sim_time() << '\n'
          << "\tSeconds: " << ( time_tt->trunc_julian_time * 86400.0 ) << '\n'
          << "\tDate: " << time_tt->calendar_year
          << "-" << time_tt->calendar_month
          << "-" << time_tt->calendar_day
          << "::" << time_tt->calendar_hour
          << ":" << time_tt->calendar_minute
          << ":" << time_tt->calendar_second << '\n'
          << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}
