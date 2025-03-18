/*!
@file SpaceFOM/DynamicalEntity.cpp
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
@trick_link_dependency{DynamicalEntity.cpp}


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
#include "SpaceFOM/DynamicalEntity.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntity::DynamicalEntity() // RETURN: -- None.
   : dynamical_data( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
DynamicalEntity::DynamicalEntity(
   PhysicalEntityData  &physical_data_ref,
   DynamicalEntityData &dynamics_data_ref ) // RETURN: -- None.
   : PhysicalEntity( physical_data_ref ),
     dynamical_data( &dynamics_data_ref )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
DynamicalEntity::~DynamicalEntity() // RETURN: -- None.
{
   physical_data = NULL;
}

/*!
 * @job_class{initialization}
 */
void DynamicalEntity::configure(
   PhysicalEntityData  *physical_data_ptr,
   DynamicalEntityData *dynamics_data_ptr )
{
   // First call the base class pre_initialize function.
   DynamicalEntityBase::configure();

   // Set the reference to the reference frame.
   if ( dynamics_data_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::DynamicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL DynamicalEntityData: "
             << pe_packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->dynamical_data = dynamics_data_ptr;

   // Mark this as initialized.
   PhysicalEntity::configure( physical_data_ptr );

   // Return to calling routine.
   return;
}

/*!
 * @job_class{initialization}
 */
void DynamicalEntity::initialize()
{
   // Check to make sure the DynamicalEntity data is set.
   if ( dynamical_data == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::DynamicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL DynamicalEntityData: "
             << pe_packing_data.name << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Mark this as initialized.
   PhysicalEntity::initialize();
   DynamicalEntityBase::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntity::pack_from_working_data()
{
   int iinc, jinc;

   // Call the base class pack from working data function.
   PhysicalEntity::pack_from_working_data();

   // Set the force data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      de_packing_data.force[iinc] = dynamical_data->force[iinc];
   }

   // Set the torque data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      de_packing_data.torque[iinc] = dynamical_data->torque[iinc];
   }

   // Set the mass and mass rate values.
   de_packing_data.mass      = dynamical_data->mass;
   de_packing_data.mass_rate = dynamical_data->mass_rate;

   // Set the inertia matrix and inertia rate data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      for ( jinc = 0; jinc < 3; ++jinc ) {
         de_packing_data.inertia[iinc][jinc]      = dynamical_data->inertia[iinc][jinc];
         de_packing_data.inertia_rate[iinc][jinc] = dynamical_data->inertia_rate[iinc][jinc];
      }
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntity::unpack_into_working_data()
{
   int iinc, jinc;

   // Call the base class unpack into working data function.
   PhysicalEntity::unpack_into_working_data();

   // If the HLA attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value.  If we locally own the attribute then we do not want to
   // override it's value.  If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state.  We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.

   // Check for force data.
   if ( force_attr->is_received() ) {
      for ( iinc = 0; iinc < 3; ++iinc ) {
         dynamical_data->force[iinc] = de_packing_data.force[iinc];
      }
   }

   // Check for torque data.
   if ( torque_attr->is_received() ) {
      for ( iinc = 0; iinc < 3; ++iinc ) {
         dynamical_data->torque[iinc] = de_packing_data.torque[iinc];
      }
   }

   // Check for mass data.
   if ( mass_attr->is_received() ) {
      dynamical_data->mass = de_packing_data.mass;
   }

   // Check for mass rate data.
   if ( mass_rate_attr->is_received() ) {
      dynamical_data->mass_rate = de_packing_data.mass_rate;
   }

   // Check for inertia data.
   if ( inertia_attr->is_received() ) {
      for ( iinc = 0; iinc < 3; ++iinc ) {
         for ( jinc = 0; jinc < 3; ++jinc ) {
            dynamical_data->inertia[iinc][jinc] = de_packing_data.inertia[iinc][jinc];
         }
      }
   }

   // Check for inertia rate data.
   if ( inertia_rate_attr->is_received() ) {
      for ( iinc = 0; iinc < 3; ++iinc ) {
         for ( jinc = 0; jinc < 3; ++jinc ) {
            dynamical_data->inertia_rate[iinc][jinc] = de_packing_data.inertia_rate[iinc][jinc];
         }
      }
   }

   return;
}
