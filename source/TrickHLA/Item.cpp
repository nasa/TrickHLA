/*!
@file TrickHLA/Item.cpp
@ingroup TrickHLA
@brief This class represents an Item in the item queue.

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
@trick_link_dependency{Item.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, Feb 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <string>

// TrickHLA include files.
#include "TrickHLA/Item.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
Item::Item() // RETURN: -- None.
   : next( NULL )
{
}

/*!
 * @job_class{shutdown}
 */
Item::~Item() // RETURN: -- None.
{
}
