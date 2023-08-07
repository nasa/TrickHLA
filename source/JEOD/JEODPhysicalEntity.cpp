/*!
@file JEOD/JEODPhysicalEntity.cpp
@ingroup JEOD
@brief This class provides data packing for the SpaceFOM PhysicalEntity and
the interface with a JEOD DynBody instance.

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
@trick_link_dependency{JEODPhysicalEntity.cpp}


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
#include "JEOD/JEODPhysicalEntity.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
JEODPhysicalEntity::JEODPhysicalEntity() // RETURN: -- None.
   : dyn_body_data( NULL )
{
}

/*!
 * @job_class{shutdown}
 */
JEODPhysicalEntity::~JEODPhysicalEntity() // RETURN: -- None.
{
   dyn_body_data = NULL;
}

/*!
 * @job_class{initialization}
 */
void JEODPhysicalEntity::initialize()
{
   ostringstream errmsg;

   // Check to make sure the JEODPhysicalEntity data is set.
   if ( dyn_body_data == NULL ) {
      errmsg << "SpaceFOM::JEODPhysicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL dyn_body_data: " << name << THLA_ENDL;
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
void JEODPhysicalEntity::initialize( jeod::DynBody *dyn_body_data_ptr )
{
   ostringstream errmsg;

   // Set the reference to the JEODPhysicalEntity data.
   if ( dyn_body_data_ptr == NULL ) {
      errmsg << "SpaceFOM::JEODPhysicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL JEODPhysicalEntityData: " << name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->dyn_body_data = dyn_body_data_ptr;

   // Mark this as initialized.
   this->initialize();

   // Return to calling routine.
   return;
}


void JEODPhysicalEntity::pack()
{
   ostringstream errmsg;
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

   // NOTE: THIS ASSUMES THE JEOD DYNBODY IS A ROOT BODY AND ITS PROPAGATION
   // FRAME MATCHES THE SPACEFOM PhysicalEntity PARENT FRAME!  IF NOT, A LOT
   // MORE COMPUTATIONS HAVE TO BE DONE HERE!!!!!

   // Pack the state time coordinate data.
   // Position and velocity vectors.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      state.pos[iinc] = dyn_body_data->composite_body.state.trans.position[iinc];
      state.vel[iinc] = dyn_body_data->composite_body.state.trans.velocity[iinc];
   }
   // Attitude quaternion.
   state.quat_scalar = dyn_body_data->composite_body.state.rot.Q_parent_this.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      state.quat_vector[iinc] = dyn_body_data->composite_body.state.rot.Q_parent_this.vector[iinc];
      state.ang_vel[iinc]     = dyn_body_data->composite_body.state.rot.ang_vel_this[iinc];
   }

   // Time tag for this state data.
   state.time = get_scenario_time();

   // Set the acceleration data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      accel[iinc] = dyn_body_data->derivs.trans_accel[iinc];
   }

   // Set the rotational acceleration data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      rot_accel[iinc] = dyn_body_data->derivs.rot_accel[iinc];
   }

   // Set the center of mass location.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      cm[iinc] = dyn_body_data->mass.composite_properties.position[iinc];
   }

   // Pack the body to structural reference frame attitude quaternion.
   body_wrt_struct.scalar = dyn_body_data->mass.composite_properties.Q_parent_this.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      body_wrt_struct.vector[iinc] = dyn_body_data->mass.composite_properties.Q_parent_this.vector[iinc];
   }

   // Print out debug information if desired.
   if ( debug ) {
      cout.precision( 15 );
      cout << "JEODPhysicalEntity::pack():" << __LINE__ << endl
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



void JEODPhysicalEntity::unpack()
{
   // double dt; // Local vs. remote time difference.

   if ( !initialized ) {
      cout << "JEODPhysicalEntity::unpack():" << __LINE__
           << " ERROR: The initialize() function has not been called!" << endl;
   }

   // Use the HLA encoder helpers to decode the JEODPhysicalEntity fixed record.
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
         dyn_body_data->composite_body.state.trans.position[iinc] = state.pos[iinc];
         dyn_body_data->composite_body.state.trans.velocity[iinc] = state.vel[iinc];
      }
      // Attitude quaternion.
      dyn_body_data->composite_body.state.rot.Q_parent_this.scalar = state.quat_scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         dyn_body_data->composite_body.state.rot.Q_parent_this.vector[iinc] = state.quat_vector[iinc];
         dyn_body_data->composite_body.state.rot.ang_vel_this[iinc] = state.ang_vel[iinc];
      }
      // Time tag for this state data.
      //dyn_body_data->state.time = state.time;

   }

   // Set the reference frame name and parent frame name.
   if ( name_attr->is_received() ) {
      // NOTE: We don't currently support renaming a ReferenceFrame for JEOD
      // based applications.  The changed name is updated in the RefFrameBase
      // name attribute but we do not do anything with it now.
   }

   if ( type_attr->is_received() ) {
      // NOTE: The changed type is updated in the RefFrameBase
      // name attribute but we do not do anything with it now.
   }

   if ( status_attr->is_received() ) {
      // NOTE: The changed status is updated in the RefFrameBase
      // name attribute but we do not do anything with it now.
   }

   if ( parent_frame_attr->is_received() ) {
      // NOTE: We don't currently support reparenting a ReferenceFrame for JEOD
      // based applications.  The changed the ReferencFrame parent name is
      // ignored for now.
   }

   // Unpack the body to structural attitude data.
   if ( body_frame_attr->is_received() ) {

      // Body to structure frame orientation.
      dyn_body_data->mass.composite_properties.Q_parent_this.scalar = body_wrt_struct.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         dyn_body_data->mass.composite_properties.Q_parent_this.vector[iinc] = body_wrt_struct.vector[iinc];
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

