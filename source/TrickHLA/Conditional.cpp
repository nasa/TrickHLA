/*!
@file TrickHLA/Conditional.cpp
@ingroup TrickHLA
@brief This class provides a user API for the handling of a CONDITIONAL
attribute.

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
@trick_link_dependency{Conditional.cpp}
@trick_link_dependency{Attribute.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, DSES, Oct 2009, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <iostream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"

// TrickHLA include files.
#include "TrickHLA/Conditional.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Conditional::Conditional() // RETURN: -- None.
{
   return;
}

/*!
 * @job_class{shutdown}
 */
Conditional::~Conditional() // RETURN: -- None.
{
   return;
}

bool Conditional::should_send( // RETURN: -- None.
   Attribute *attr )           // IN: ** Attribute to send
{
   return true;
}
