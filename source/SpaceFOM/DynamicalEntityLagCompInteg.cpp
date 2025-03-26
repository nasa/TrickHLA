/*!
@file SpaceFOM/DynamicalEntityLagCompInteg.cpp
@ingroup SpaceFOM
@brief This class provides the implementation for a TrickHLA SpaceFOM
physical entity latency/lag compensation class that uses integration to
compensate the state.

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
@trick_link_dependency{../TrickHLA/LagCompensationInteg.cpp}
@trick_link_dependency{DynamicalEntityLagCompInteg.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityLagCompInteg.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityLagCompInteg::DynamicalEntityLagCompInteg( DynamicalEntityBase &entity_ref ) // RETURN: -- None.
   : DynamicalEntityLagCompBase( entity_ref ),
     TrickHLA::LagCompensationInteg()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
DynamicalEntityLagCompInteg::~DynamicalEntityLagCompInteg() // RETURN: -- None.
{
   return;
}

/*!
 * @job_class{initialization}
 */
void DynamicalEntityLagCompInteg::initialize()
{
   if ( this->integ_dt < this->integ_tol ) {
      ostringstream errmsg;

      errmsg << "SpaceFOM::DynamicalEntityLagCompInteg::initialize():" << __LINE__ << '\n'
             << " ERROR: Tolerance must be less that the dt!: dt = "
             << this->integ_dt << "; tolerance = " << this->integ_tol << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Call the base class initialize routine.
   DynamicalEntityLagCompBase::initialize();

   // Return to calling routine.
   return;
}
