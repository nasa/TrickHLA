/*!
@file TrickHLA/LagCompensation.cpp
@ingroup TrickHLA
@brief This class is the abstract base class for Trick HLA lag compensation.

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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{LagCompensation.cpp}
@trick_link_dependency{ObjectCallbackBase.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2006, --, DSES Initial Lag Compensation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, October 2023, --, Added lag-comp bypass functions.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, September 2025, --, Extends ObjectCallbackBase.}
@revs_end

*/

// System includes.
#include <ostream>
#include <sstream>
#include <string>

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/LagCompensation.hh"
#include "TrickHLA/ObjectCallbackBase.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
LagCompensation::LagCompensation()
   : TrickHLA::ObjectCallbackBase()
{
   return;
}

/*!
 * @job_class{initialization}
 */
LagCompensation::LagCompensation(
   string name ) // cppcheck-suppress [passedByValue]
   : TrickHLA::ObjectCallbackBase( name )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
LagCompensation::~LagCompensation()
{
   return;
}

/*!
 * @brief Send side lag compensation callback.
 */
void LagCompensation::send_lag_compensation()
{
   ostringstream errmsg;
   errmsg << "LagCompensation::send_lag_compensation():" << __LINE__
          << " ERROR: Your class that extends LagCompensation must implement"
          << " the 'virtual void send_lag_compensation()' function!" << endl;
   DebugHandler::terminate_with_message( errmsg.str() );
}

/*!
 * @brief Receive side lag compensation callback.
 */
void LagCompensation::receive_lag_compensation()
{
   ostringstream errmsg;
   errmsg << "LagCompensation::receive_lag_compensation():" << __LINE__
          << " ERROR: Your class that extends LagCompensation must implement"
          << " the 'virtual void receive_lag_compensation()' function!" << endl;
   DebugHandler::terminate_with_message( errmsg.str() );
}
