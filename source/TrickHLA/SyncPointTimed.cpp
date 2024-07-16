/*!
@file TrickHLA/SyncPointTimed.cpp
@ingroup TrickHLA
@brief This class provides a sync-point with a time-stamp.

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
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{SyncPoint.cpp}
@trick_link_dependency{SyncPointTimed.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA JSC ER7, TrickHLA, Jan 2019, --, Create from old TrickHLASyncPtsBase class.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <string>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPoint.hh"
#include "TrickHLA/SyncPointTimed.hh"
#include "TrickHLA/Types.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPointTimed::SyncPointTimed()
   : SyncPoint( TrickHLA::SYNC_PT_TYPE_TIME ),
     time( 0.0 )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPointTimed::SyncPointTimed(
   wstring const &label )
   : SyncPoint( label, TrickHLA::SYNC_PT_TYPE_TIME ),
     time( 0.0 )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPointTimed::SyncPointTimed(
   wstring const   &label,
   Int64Time const &t )
   : SyncPoint( label, TrickHLA::SYNC_PT_TYPE_TIME ),
     time( t )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SyncPointTimed::~SyncPointTimed()
{
   return;
}

std::string SyncPointTimed::to_string()
{
   string label_str;
   StringUtilities::to_string( label_str, this->label );

   string result = "[" + label_str + "/" + time.to_string() + "] -- ";
   switch ( this->state ) {

      case TrickHLA::SYNC_PT_STATE_ERROR:
         result += "SYNC_PT_STATE_ERROR";
         break;

      case TrickHLA::SYNC_PT_STATE_KNOWN:
         result += "SYNC_PT_STATE_KNOWN";
         break;

      case TrickHLA::SYNC_PT_STATE_REGISTERED:
         result += "SYNC_PT_STATE_REGISTERED";
         break;

      case TrickHLA::SYNC_PT_STATE_ANNOUNCED:
         result += "SYNC_PT_STATE_ANNOUNCED";
         break;

      case TrickHLA::SYNC_PT_STATE_ACHIEVED:
         result += "SYNC_PT_STATE_ACHIEVED";
         break;

      default:
         result += "SYNC_PT_STATE_UNKNOWN";
   }

   return result;
}
