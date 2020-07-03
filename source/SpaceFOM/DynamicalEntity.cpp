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
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{DynamicalEntity.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, SISO, Sept 2010, --, Smackdown implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
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
#include "trick/exec_proto.hh"
#include "trick/matrix_macros.h"
#include "trick/message_proto.h"
#include "trick/vector_macros.h"

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntity.hh"

using namespace std;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntity::DynamicalEntity() // RETURN: -- None.
   : mass( 0.0 ),
     mass_rate( 0.0 )
{
   V_INIT( force );
   V_INIT( torque );
   M_IDENT( inertia );
   M_IDENT( inertia_rate );
}

/*!
 * @job_class{shutdown}
 */
DynamicalEntity::~DynamicalEntity() // RETURN: -- None.
{
   return;
}

void DynamicalEntity::pack()
{
   PhysicalEntityBase::pack();
}

void DynamicalEntity::unpack()
{
   PhysicalEntityBase::unpack();
}
