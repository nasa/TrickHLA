/*!
@file SpaceFOM/PhysicalInterface.cpp
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM PhysicalInterfaces.

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
@trick_link_dependency{PhysicalInterface.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
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
#include "SpaceFOM/PhysicalInterface.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalInterface::PhysicalInterface()
   : interface_data( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
PhysicalInterface::PhysicalInterface(
   PhysicalInterfaceData &interface_data_ref )
   : interface_data( &interface_data_ref )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
PhysicalInterface::~PhysicalInterface()
{
   interface_data = NULL;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterface::configure( PhysicalInterfaceData *interface_data_ptr )
{
   ostringstream errmsg;

   // First call the base class pre_initialize function.
   PhysicalInterfaceBase::configure();

   // Set the reference to the PhysicalInterface data.
   if ( interface_data_ptr == NULL ) {
      errmsg << "SpaceFOM::PhysicalInterface::initialize():" << __LINE__
             << " ERROR: Unexpected NULL PhysicalInterfaceData: " << packing_data.name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->interface_data = interface_data_ptr;

   // Return to calling routine.
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterface::initialize()
{
   ostringstream errmsg;

   // Check to make sure the PhysicalInterface data is set.
   if ( interface_data == NULL ) {
      errmsg << "SpaceFOM::PhysicalInterface::initialize():" << __LINE__
             << " ERROR: Unexpected NULL PhysicalInterfaceData: " << packing_data.name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Mark this as initialized.
   PhysicalInterfaceBase::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalInterface::pack_from_working_data()
{
   ostringstream errmsg;
   int           iinc;

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Check for name change.
   if ( interface_data->name != NULL ) {

      if ( packing_data.name != NULL ) {

         // Compare names.
         if ( strcmp( interface_data->name, packing_data.name ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( packing_data.name ) ) ) {
               send_hs( stderr, "PhysicalInterface::pack_from_working_data():%d ERROR deleting Trick Memory for 'packing_data.name'%c",
                        __LINE__, THLA_NEWLINE );
            }
            packing_data.name = trick_MM->mm_strdup( interface_data->name );
         }

      } // No name to compare so copy name.
      else {

         packing_data.name = trick_MM->mm_strdup( interface_data->name );
      }

   } // This is bad scoobies so just punt.
   else {
      errmsg << "SpaceFOM::PhysicalInterface::pack():" << __LINE__
             << " ERROR: Unexpected NULL name for PhysicalInterface!" << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check for parent name change.
   if ( interface_data->parent_name != NULL ) {

      if ( packing_data.parent_name != NULL ) {

         // We have a parent name; so, check to see if names are different.
         if ( strcmp( interface_data->parent_name, packing_data.parent_name ) ) {
            // Names are different, so reassign the new name string.
            if ( trick_MM->delete_var( static_cast< void * >( packing_data.parent_name ) ) ) {
               send_hs( stderr, "SpaceFOM::PhysicalInterface::pack():%d ERROR deleting Trick Memory for 'packing_data.parent_name'%c",
                        __LINE__, THLA_NEWLINE );
            }
            packing_data.parent_name = trick_MM->mm_strdup( interface_data->parent_name );
         }

      } // No parent frame name to compare so copy name.
      else {

         packing_data.parent_name = trick_MM->mm_strdup( interface_data->parent_name );
      }

   } // This is bad scoobies so just punt.
   else {
      errmsg << "SpaceFOM::PhysicalInterface::pack():" << __LINE__
             << " ERROR: Unexpected NULL parent name for PhysicalInterface: "
             << interface_data->name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Set the position data.
   for ( iinc = 0; iinc < 3; ++iinc ) {
      packing_data.position[iinc] = interface_data->position[iinc];
   }

   // Pack the interface attitude quaternion.
   packing_data.attitude.scalar = interface_data->attitude.scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      packing_data.attitude.vector[iinc] = interface_data->attitude.vector[iinc];
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalInterface::unpack_into_working_data()
{

   // If the HLA attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value.  If we locally own the attribute then we do not want to
   // override it's value.  If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state.  We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.

   // Set the interface name and parent name.
   if ( name_attr->is_received() ) {
      if ( interface_data->name != NULL ) {
         if ( !strcmp( interface_data->name, packing_data.name ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( interface_data->name ) ) ) {
               send_hs( stderr, "PhysicalInterface::unpack_into_working_data():%d ERROR deleting Trick Memory for 'interface_data->name'%c",
                        __LINE__, THLA_NEWLINE );
            }
            interface_data->name = trick_MM->mm_strdup( packing_data.name );
         }
      } else {
         interface_data->name = trick_MM->mm_strdup( packing_data.name );
      }
   }

   if ( parent_attr->is_received() ) {
      if ( interface_data->parent_name != NULL ) {
         if ( !strcmp( interface_data->parent_name, packing_data.parent_name ) ) {
            if ( trick_MM->delete_var( static_cast< void * >( interface_data->parent_name ) ) ) {
               send_hs( stderr, "PhysicalInterface::unpack_into_working_data():%d ERROR deleting Trick Memory for 'interface_data->parent_name'%c",
                        __LINE__, THLA_NEWLINE );
            }
            if ( packing_data.parent_name[0] != '\0' ) {
               interface_data->parent_name = trick_MM->mm_strdup( packing_data.parent_name );
            } else {
               interface_data->parent_name = NULL;
            }
         }
      } else {
         if ( packing_data.parent_name[0] != '\0' ) {
            interface_data->parent_name = trick_MM->mm_strdup( packing_data.parent_name );
         }
      }
   }

   // Unpack the interface position data.
   if ( position_attr->is_received() ) {
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         interface_data->position[iinc] = packing_data.position[iinc];
      }
   }

   // Unpack the interface attitude data.
   if ( attitude_attr->is_received() ) {

      // Interface frame orientation.
      interface_data->attitude.scalar = packing_data.attitude.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         interface_data->attitude.vector[iinc] = packing_data.attitude.vector[iinc];
      }
   }

   return;
}
