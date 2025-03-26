/*!
@file JEOD/JEODPhysicalInterface.cpp
@ingroup JEOD
@brief This class provides data packing for the SpaceFOM PhysicalInterface and
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
@trick_link_dependency{JEODPhysicalInterface.cpp}


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
#include "JEOD/JEODPhysicalInterface.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
JEODPhysicalInterface::JEODPhysicalInterface()
   : dyn_body( NULL ),
     vehicle_point_id( NULL ),
     vehicle_point_data( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
JEODPhysicalInterface::JEODPhysicalInterface(
   jeod::DynBody &dyn_body_ref )
   : dyn_body( &dyn_body_ref ),
     vehicle_point_id( NULL ),
     vehicle_point_data( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
JEODPhysicalInterface::JEODPhysicalInterface(
   jeod::DynBody      &dyn_body_ref,
   jeod::BodyRefFrame &vehicle_point_ref )
   : dyn_body( &dyn_body_ref ),
     vehicle_point_id( NULL ),
     vehicle_point_data( &vehicle_point_ref )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
JEODPhysicalInterface::~JEODPhysicalInterface()
{
   dyn_body           = NULL;
   vehicle_point_data = NULL;

   if ( this->vehicle_point_id != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->vehicle_point_id ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::JEODPhysicalInterface::~JEODPhysicalInterface():%d WARNING failed to delete Trick Memory for 'this->vehicle_point_id'\n",
                          __LINE__ );
      }
      this->vehicle_point_id = NULL;
   }
}

/*!
 * @job_class{initialization}
 */
void JEODPhysicalInterface::configure() // cppcheck-suppress [duplInheritedMember]
{
   // First call the base class pre_initialize function.
   PhysicalInterfaceBase::configure();

   // Make sure that we have an associated JEOD::DynBody.
   if ( dyn_body == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::configure():" << __LINE__
             << " ERROR: Unexpected NULL dyn_body_ptr: for interface "
             << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make sure that we have a vehicle point ID to work with.
   if ( this->vehicle_point_id == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::configure():" << __LINE__
             << " ERROR: Unexpected NULL vehicle_point_id for interface "
             << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Postpone looking up the actual VehiclePoint until initialization.
}

/*!
 * @job_class{initialization}
 */
void JEODPhysicalInterface::configure(
   jeod::DynBody *dyn_body_ptr ) // cppcheck-suppress [constParameterPointer]
{
   // Make sure that we have a vehicle point ID to work with.
   if ( this->vehicle_point_id == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::configure():" << __LINE__
             << " ERROR: Unexpected NULL vehicle_point_id for interface "
             << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Look up the vehicle point by name.
   if ( dyn_body_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::configure():" << __LINE__
             << " ERROR: Unexpected NULL dyn_body_ptr: for interface "
             << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   } else {
      this->vehicle_point_data = dyn_body_ptr->find_vehicle_point( vehicle_point_id );
   }

   // Make sure that we found the vehicle point.
   if ( this->vehicle_point_data == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::configure():" << __LINE__
             << " ERROR: Unexpected NULL vehicle_point_data for interface "
             << vehicle_point_id << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Call the final configuration check.
   configure();
}

/*!
 * @job_class{initialization}
 */
void JEODPhysicalInterface::configure(
   jeod::BodyRefFrame *vehicle_point_ptr )
{
   // Set the reference to the JEODPhysicalInterface data.
   if ( vehicle_point_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::configure():" << __LINE__
             << " ERROR: Unexpected NULL vehicle_point_ptr: "
             << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->vehicle_point_data = vehicle_point_ptr;

   // Call the final configuration check.
   configure();
}

/*!
 * @job_class{initialization}
 */
void JEODPhysicalInterface::configure(
   jeod::DynBody      *dyn_body_ptr,
   jeod::BodyRefFrame *vehicle_point_ptr )
{
   // Make sure that we have a dyn_body pointer to assign.
   if ( dyn_body_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::configure():" << __LINE__
             << " ERROR: Unexpected NULL dyn_body_ptr: for interface "
             << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   } else {
      this->dyn_body = dyn_body_ptr;
   }

   // Set the reference to the JEODPhysicalInterface data.
   if ( vehicle_point_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::configure():" << __LINE__
             << " ERROR: Unexpected NULL vehicle_point_ptr: "
             << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   } else {
      this->vehicle_point_data = vehicle_point_ptr;
   }

   // Call the final configuration check.
   configure();
}

/*!
 * @job_class{initialization}
 */
void JEODPhysicalInterface::initialize()
{
   // Check if the DynBody is set.
   if ( dyn_body == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::initialize():" << __LINE__
             << " ERROR: Unexpected NULL dyn_body reference: for interface "
             << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make sure that we have a vehicle point ID to work with.
   if ( this->vehicle_point_id == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::initialize():" << __LINE__
             << " ERROR: Unexpected NULL vehicle_point_id for interface "
             << this->packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check to make sure the JEODPhysicalInterface data is set.
   if ( vehicle_point_data == NULL ) {
      // It is not already set; so, let's try to look it up.
      this->vehicle_point_data = dyn_body->find_vehicle_point( vehicle_point_id );
   }

   // Make sure that we found the vehicle point.
   if ( this->vehicle_point_data == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::JEODPhysicalInterface::initialize():" << __LINE__
             << " ERROR: Unexpected NULL vehicle_point_data for interface "
             << vehicle_point_id << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Mark this as initialized.
   PhysicalInterfaceBase::initialize();
}

void JEODPhysicalInterface::pack_from_working_data()
{
   int iinc;

   // Check for NULL vehicle point.
   // Note: This should never be true, but just in case.
   if ( this->vehicle_point_data == NULL ) {
      message_publish( MSG_WARNING,
                       "SpaceFOM::JEODPhysicalInterface::pack():%d NULL vehicle point data!\n",
                       __LINE__ );
      return;
   }

   // Short cut to the mass point data.
   jeod::MassPoint *mass_point_ptr = vehicle_point_data->mass_point;

   // Check for initialization.
   if ( !initialized ) {
      ostringstream errmsg;
      errmsg << "JEODPhysicalInterface::pack() ERROR: The initialize() function has not"
             << " been called!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // NOTE: THIS ASSUMES THE JEOD DYNBODY IS A ROOT BODY AND ITS PROPAGATION
   // FRAME MATCHES THE SPACEFOM PhysicalInterface PARENT FRAME!  IF NOT, A LOT
   // MORE COMPUTATIONS HAVE TO BE DONE HERE!!!!!

   // Pack the state time coordinate data.
   // Position vector.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      this->packing_data.position[iinc] = mass_point_ptr->position[iinc];
   }
   // Attitude quaternion.
   this->packing_data.attitude.scalar = mass_point_ptr->Q_parent_this.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      this->packing_data.attitude.vector[iinc] = mass_point_ptr->Q_parent_this.vector[iinc];
   }
}

void JEODPhysicalInterface::unpack_into_working_data()
{
   jeod::MassPoint *mass_point_ptr;

   // Check for NULL vehicle point.
   // Note: This should never be true, but just in case.
   if ( this->vehicle_point_data == NULL ) {
      message_publish( MSG_WARNING,
                       "SpaceFOM::JEODPhysicalInterface::unpack():%d NULL vehicle point data!\n",
                       __LINE__ );
      return;
   }

   // Short cut to mass point data.
   mass_point_ptr = vehicle_point_data->mass_point;

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
      // NOTE: We don't currently support renaming a MassPointState for JEOD
      // based applications.  The changed name is updated in the
      // PhysicalInterfaceBase name attribute but we do not do anything with
      // it now.
   }

   if ( parent_attr->is_received() ) {
      // NOTE: We don't currently support reparenting a MassPointState for JEOD
      // based applications.  The changed parent name is updated in the
      // PhysicalInterfaceBase but the MassPointState parent name is
      // ignored for now.
   }

   // Unpack the position data.
   if ( position_attr->is_received() ) {
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         mass_point_ptr->position[iinc] = this->packing_data.position[iinc];
      }
   }

   // Unpack the interface attitude data.
   if ( attitude_attr->is_received() ) {
      mass_point_ptr->Q_parent_this.scalar = this->packing_data.attitude.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         mass_point_ptr->Q_parent_this.vector[iinc] = this->packing_data.attitude.vector[iinc];
      }
      // Compute the associated transformation matrix.
      mass_point_ptr->Q_parent_this.left_quat_to_transformation( mass_point_ptr->T_parent_this );
   }

   // FIXME: We probably need to update the vehicle point state based on these updates!!
}

/*!
 * @job_class{initialization}
 */
void JEODPhysicalInterface::set_vehicle_point_id( char const *new_id )
{
   if ( this->vehicle_point_id != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->vehicle_point_id ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::JEODPhysicalInterface::set_vehicle_point_id():%d WARNING failed to delete Trick Memory for 'this->vehicle_point_id'\n",
                          __LINE__ );
      }
   }
   vehicle_point_id = trick_MM->mm_strdup( new_id );
}
