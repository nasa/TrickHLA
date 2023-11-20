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
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh"
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
   // Check to make sure the JEODPhysicalEntity data is set.
   if ( dyn_body_data == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL dyn_body_data: "
             << this->pe_packing_data.name << THLA_ENDL;
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
   // Set the reference to the JEODPhysicalEntity data.
   if ( dyn_body_data_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL JEODPhysicalEntityData: "
             << this->pe_packing_data.name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->dyn_body_data = dyn_body_data_ptr;

   // Mark this as initialized.
   this->initialize();

   // Return to calling routine.
   return;
}

void JEODPhysicalEntity::pack_from_working_data()
{
   int iinc;

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
      this->pe_packing_data.state.pos[iinc] = dyn_body_data->composite_body.state.trans.position[iinc];
      this->pe_packing_data.state.vel[iinc] = dyn_body_data->composite_body.state.trans.velocity[iinc];
   }
   // Attitude quaternion.
   this->pe_packing_data.state.att.scalar = dyn_body_data->composite_body.state.rot.Q_parent_this.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      this->pe_packing_data.state.att.vector[iinc] = dyn_body_data->composite_body.state.rot.Q_parent_this.vector[iinc];
      this->pe_packing_data.state.ang_vel[iinc]     = dyn_body_data->composite_body.state.rot.ang_vel_this[iinc];
   }

   // Time tag for this state data.
   this->pe_packing_data.state.time = get_scenario_time();

   // Set the acceleration data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      this->pe_packing_data.accel[iinc] = dyn_body_data->derivs.trans_accel[iinc];
   }

   // Set the rotational acceleration data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      this->pe_packing_data.ang_accel[iinc] = dyn_body_data->derivs.rot_accel[iinc];
   }

   // Set the center of mass location.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      this->pe_packing_data.cm[iinc] = dyn_body_data->mass.composite_properties.position[iinc];
   }

   // Pack the body to structural reference frame attitude quaternion.
   this->pe_packing_data.body_wrt_struct.scalar = dyn_body_data->mass.composite_properties.Q_parent_this.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      this->pe_packing_data.body_wrt_struct.vector[iinc] = dyn_body_data->mass.composite_properties.Q_parent_this.vector[iinc];
   }

   return;
}

void JEODPhysicalEntity::unpack_into_working_data()
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

   // Unpack the space-time coordinate state data.
   if ( state_attr->is_received() ) {

      // Unpack the data.
      // Position and velocity vectors.
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         dyn_body_data->composite_body.state.trans.position[iinc] = this->pe_packing_data.state.pos[iinc];
         dyn_body_data->composite_body.state.trans.velocity[iinc] = this->pe_packing_data.state.vel[iinc];
      }
      // Attitude quaternion.
      dyn_body_data->composite_body.state.rot.Q_parent_this.scalar = this->pe_packing_data.state.att.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         dyn_body_data->composite_body.state.rot.Q_parent_this.vector[iinc] = this->pe_packing_data.state.att.vector[iinc];
         dyn_body_data->composite_body.state.rot.ang_vel_this[iinc]         = this->pe_packing_data.state.ang_vel[iinc];
      }
      // Time tag for this state data.
      // dyn_body_data->state.time = state.time;
   }

   // Unpack the translational acceleration data.
   if ( accel_attr->is_received() ) {
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         dyn_body_data->derivs.trans_accel[iinc] = pe_packing_data.accel[iinc];
      }
   }

   // Unpack the rotational acceleration data.
   if ( ang_accel_attr->is_received() ) {
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         dyn_body_data->derivs.rot_accel[iinc] = pe_packing_data.ang_accel[iinc];
      }
   }

   // Unpack the center of mass data.
   if ( cm_attr->is_received() ) {
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         dyn_body_data->mass.composite_properties.position[iinc] = pe_packing_data.cm[iinc];
      }
   }

   // Unpack the body to structural attitude data.
   if ( body_frame_attr->is_received() ) {

      // Body to structure frame orientation.
      dyn_body_data->mass.composite_properties.Q_parent_this.scalar = this->pe_packing_data.body_wrt_struct.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         dyn_body_data->mass.composite_properties.Q_parent_this.vector[iinc] = this->pe_packing_data.body_wrt_struct.vector[iinc];
      }
   }

   return;
}
