/*!
@file SpaceFOM/DynamicalEntityDeleted.cpp
@ingroup SpaceFOM
@brief Callback class the user writes to do something once the object has been
deleted from the RTI.

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
@trick_link_dependency{DynamicalEntityDeleted.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <sstream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Object.hh"

// Model include files.
#include "SpaceFOM/DynamicalEntityDeleted.hh"

using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityDeleted::DynamicalEntityDeleted()
   : SpaceFOM::PhysicalEntityDeleted()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
DynamicalEntityDeleted::~DynamicalEntityDeleted()
{
   return;
}

void DynamicalEntityDeleted::deleted()
{
   std::ostringstream msg;
   msg << "SpaceFOM::DynamicalEntityDeleted::deleted():" << __LINE__
       << " Object '" << object->get_name() << "' deleted from the federation.";
   message_publish( MSG_NORMAL, msg.str().c_str() );

   // Call the PhysicalEntityDeleted base class method.
   PhysicalEntityDeleted::deleted();

   return;
}
