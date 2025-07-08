/*!
@file TrickHLA/ObjectDeleted.cpp
@ingroup TrickHLA
@brief This class is the abstract base class for a callback of identification
of deleted objects from the RTI.

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
@trick_link_dependency{ObjectDeleted.cpp}
@trick_link_dependency{Object.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3-Communications, DSES, June 2008, --, IMSim: report TrickHLA::Object as deleted via a callback.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, November 2023, --, Added initialize_callback().}
@revs_end

*/

// System includes.
#include <cstddef>

// TrickHLA includes.
#include "TrickHLA/Object.hh"
#include "TrickHLA/ObjectDeleted.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ObjectDeleted::ObjectDeleted() // RETURN: -- None.
   : object( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ObjectDeleted::~ObjectDeleted() // RETURN: -- None.
{
   return;
}

/*!
 * @brief Initialize the callback object to the supplied Object pointer.
 * @param obj Associated object for this class.
 */
void ObjectDeleted::initialize_callback(
   Object *obj )
{
   this->object = obj;
}
