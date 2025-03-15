/*!
@file SpaceFOM/DynamicalEntityOwnershipHandler.cpp
@ingroup SpaceFOM
@brief Ownership transfer for the HLA object attributes of a SpaceFOM
DynamicalEntity.

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
@trick_link_dependency{../TrickHLA/Object.cpp}
@trick_link_dependency{../TrickHLA/OwnershipHandler.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{DynamicalEntityOwnershipHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

// TrickHLA include files.
#include "TrickHLA/Object.hh"
#include "TrickHLA/OwnershipHandler.hh"
#include "TrickHLA/Types.hh"

// Model include files.
#include "SpaceFOM/DynamicalEntityOwnershipHandler.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityOwnershipHandler::DynamicalEntityOwnershipHandler()
   : SpaceFOM::PhysicalEntityOwnershipHandler()
{
   return;
} // Default constructor.

/*!
 * @job_class{shutdown}
 */
DynamicalEntityOwnershipHandler::~DynamicalEntityOwnershipHandler()
{
   return;
}

/*!
 * @details From the TrickHLA::OwnershipHandler class. We override this
 * function so that we can initialize ownership transfer of some attributes
 * at a specific time.
 *
 * @job_class{initialization}
 */
void DynamicalEntityOwnershipHandler::initialize_callback(
   TrickHLA::Object *obj )
{
   // Make sure we call the original function so that the callback is initialized.
   PhysicalEntityOwnershipHandler::initialize_callback( obj );

   return;
}
