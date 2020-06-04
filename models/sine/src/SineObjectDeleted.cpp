/*!
@file models/sine/src/SineObjectDeleted.cpp
@ingroup TrickHLAModel
@brief Callback class the user writes to do something once the object has been
deleted from the RTI.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{sine/src/SineObjectDeleted.o}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3-Communications, DSES, June 2008, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <sstream>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h" // for send_hs

// TrickHLA include files.
#include "TrickHLA/Object.hh"

// Model include files.
#include "../include/SineObjectDeleted.hh"

using namespace TrickHLA;
using namespace TrickHLAModel;


/*!
 * @job_class{initialization}
 */
SineObjectDeleted::SineObjectDeleted()
   : TrickHLA::ObjectDeleted()
{
}


/*!
 * @job_class{shutdown}
 */
SineObjectDeleted::~SineObjectDeleted()
{
}


void SineObjectDeleted::deleted(
   TrickHLA::Object *obj)
{
   std::ostringstream msg;
   msg << "SineObjectDeleted::deleted() Object '" << obj->get_name()
       << "' deleted from the federation.";
   send_hs(stdout, (char *)msg.str().c_str());
}
