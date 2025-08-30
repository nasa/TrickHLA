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
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Object.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{PhysicalInterface.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

// System includes.
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <sstream>

// SpaceFOM includes.
#include "SpaceFOM/PhysicalInterface.hh"
#include "SpaceFOM/PhysicalInterfaceBase.hh"
#include "SpaceFOM/PhysicalInterfaceData.hh"
#include "SpaceFOM/QuaternionData.hh"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"

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
void PhysicalInterface::configure()
{

   // Check the reference to the PhysicalInterface data.
   if ( interface_data == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterface::configure():" << __LINE__
             << " ERROR: Unexpected NULL PhysicalInterfaceData: "
             << packing_data.name << endl;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Call the base class pre_initialize function.
   PhysicalInterfaceBase::configure();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterface::initialize()
{
   // Check to make sure the PhysicalInterface data is set.
   if ( interface_data == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterface::initialize():" << __LINE__
             << " ERROR: Unexpected NULL PhysicalInterfaceData: "
             << packing_data.name << endl;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
      return;
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
   int iinc;

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Check for name change.
   if ( interface_data->name != packing_data.name ) {
      packing_data.name = interface_data->name;
   }

   // Check for parent name change.
   if ( interface_data->parent_name != packing_data.parent_name ) {
      packing_data.parent_name = interface_data->parent_name;
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
      interface_data->name = packing_data.name;
   }

   if ( parent_attr->is_received() ) {
      interface_data->parent_name = packing_data.parent_name;
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

/*!
 * @job_class{initialization}
 */
void PhysicalInterface::set_name( std::string const &new_name )
{
   // Call the base class method.
   PhysicalInterfaceBase::set_name( new_name );

   // Make sure that the interface data is also set.
   this->interface_data->name = new_name;

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterface::set_parent( std::string const &new_parent_name )
{
   // Call the base class method.
   PhysicalInterfaceBase::set_parent( new_parent_name );

   // Make sure that the interface data is also set.
   this->interface_data->parent_name = new_parent_name;

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterface::set_data( PhysicalInterfaceData *interface_data_ptr )
{

   // Set the reference to the PhysicalInterface data.
   if ( interface_data_ptr == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterface::initialize():" << __LINE__
             << " ERROR: Unexpected NULL PhysicalInterfaceData: "
             << packing_data.name << endl;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }
   this->interface_data = interface_data_ptr;

   // Return to calling routine.
   return;
}
