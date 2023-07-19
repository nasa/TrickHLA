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
@trick_link_dependency{../TrickHLA/CompileConfig.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{DynamicalEntity.cpp}


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
void DynamicalEntity::initialize(
   PhysicalEntityData  * physical_data_ptr,
   DynamicalEntityData * dynamics_data_ptr  )
{
   ostringstream errmsg;

   // Set the reference to the reference frame.
   if ( dynamics_data_ptr == NULL ) {
      errmsg << "SpaceFOM::DynamicalEntity::initialize():" << __LINE__
             << " ERROR: Unexpected NULL DynamicalEntityData: " << this->name << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Mark this as initialized.
   PhysicalEntity::initialize(physical_data_ptr);

   // Return to calling routine.
   return;
}


void DynamicalEntity::pack()
{

   // Call the PhysicalEntity pack routine.
   PhysicalEntity::pack();

   // Print out debug information if desired.
   if ( debug ) {
      cout.precision( 15 );
      cout << "DynamicalEntity::pack():" << __LINE__ << endl
           << "\mass: " << mass << endl
           << "\mass_rate: " << mass_rate << endl
           << "\tforce: " << endl
           << "\t\t" << force[0] << endl
           << "\t\t" << force[1] << endl
           << "\t\t" << force[2] << endl
           << "\ttorque: " << endl
           << "\t\t" << torque[0] << endl
           << "\t\t" << torque[1] << endl
           << "\t\t" << torque[2] << endl
           << endl;
   }

   // Note: All the DynamicalEntity data is either already encoded
   // in PhysicalEntity above or is handled by TrickHLA in the
   // automated mapping of basic data types.

   return;
}



void DynamicalEntity::unpack()
{

   // Call the PhysicalEntity pack routine.
   PhysicalEntity::unpack();

   // If the HLA attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value.  If we locally own the attribute then we do not want to
   // override it's value.  If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state.  We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.
   if ( entity_attr->is_received() ) {

      // Print out debug information if desired.
      if ( debug ) {
         cout.precision( 15 );
         cout << "DynamicalEntity::pack():" << __LINE__ << endl
              << "\mass: " << mass << endl
              << "\mass_rate: " << mass_rate << endl
              << "\tforce: " << endl
              << "\t\t" << force[0] << endl
              << "\t\t" << force[1] << endl
              << "\t\t" << force[2] << endl
              << "\ttorque: " << endl
              << "\t\t" << torque[0] << endl
              << "\t\t" << torque[1] << endl
              << "\t\t" << torque[2] << endl
              << endl;
      }

      // Note: All the DynamicalEntity data is either already decoded
      // in PhysicalEntity above or is handled by TrickHLA in the
      // automated mapping of basic data types.

   }

   return;
}

