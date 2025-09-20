/*!
@file SpaceFOM/PhysicalInterfaceDeleted.cpp
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
@trick_link_dependency{../TrickHLA/ObjectDeletedHandler.cpp}
@trick_link_dependency{PhysicalInterfaceDeleted.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System includes.
#include <ostream>
#include <sstream>

// Trick includes.
#include "trick/message_proto.h"
#include "trick/message_type.h"

// SpaceFOM includes.
#include "SpaceFOM/PhysicalInterfaceDeleted.hh"

// TrickHLA includes.
#include "TrickHLA/Object.hh"
#include "TrickHLA/ObjectDeletedHandler.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalInterfaceDeleted::PhysicalInterfaceDeleted()
   : TrickHLA::ObjectDeletedHandler()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
PhysicalInterfaceDeleted::~PhysicalInterfaceDeleted()
{
   return;
}

void PhysicalInterfaceDeleted::deleted()
{
   ostringstream msg;
   msg << "SpaceFOM::PhysicalInterfaceDeleted::deleted():" << __LINE__
       << " Object '" << object->get_name() << "' deleted from the federation."
       << endl;
   message_publish( MSG_NORMAL, msg.str().c_str() );
}
