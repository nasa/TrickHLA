/*!
@file TrickHLA/Packing.cpp
@ingroup TrickHLA
@brief This class is the abstract base class for Trick HLA Packing class.

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
@trick_link_dependency{ObjectCallbackBase.cpp}
@trick_link_dependency{Packing.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial version.}
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Sept 2009, --, Updated Packing API.}
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Oct 2009, --, Added get attribute function.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, September 2025, --, Extends ObjectCallbackBase.}
@revs_end

*/

// System includes
#include <string>

// TrickHLA includes.
#include "TrickHLA/ObjectCallbackBase.hh"
#include "TrickHLA/Packing.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Packing::Packing()
   : TrickHLA::ObjectCallbackBase()
{
   return;
}

/*!
 * @job_class{initialization}
 */
Packing::Packing(
   string name ) // cppcheck-suppress [passedByValue]
   : TrickHLA::ObjectCallbackBase( name )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
Packing::~Packing()
{
   return;
}
