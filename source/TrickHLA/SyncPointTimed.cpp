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
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPoint.hh"
#include "TrickHLA/SyncPointTimed.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/BasicDataElements.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPointTimed::SyncPointTimed()
   : SyncPoint(),
     time( 0.0 )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPointTimed::SyncPointTimed(
   wstring const &label )
   : SyncPoint( label ),
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
   : SyncPoint( label ),
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

RTI1516_USERDATA const SyncPointTimed::encode_user_supplied_tag()
{
   return time.encode();
}

void SyncPointTimed::decode_user_supplied_tag(
   RTI1516_USERDATA const &supplied_tag )
{
   time.decode( supplied_tag );
}

std::string SyncPointTimed::to_string()
{
   string label_str;
   StringUtilities::to_string( label_str, this->label );

   string result = "[" + label_str + "/" + time.to_string() + "] -- ";
   switch ( this->state ) {
      case TrickHLA::SYNC_PT_STATE_ERROR: {
         result += "SYNC_PT_STATE_ERROR";
         break;
      }
      case TrickHLA::SYNC_PT_STATE_KNOWN: {
         result += "SYNC_PT_STATE_KNOWN";
         break;
      }
      case TrickHLA::SYNC_PT_STATE_REGISTERED: {
         result += "SYNC_PT_STATE_REGISTERED";
         break;
      }
      case TrickHLA::SYNC_PT_STATE_ANNOUNCED: {
         result += "SYNC_PT_STATE_ANNOUNCED";
         break;
      }
      case TrickHLA::SYNC_PT_STATE_ACHIEVED: {
         result += "SYNC_PT_STATE_ACHIEVED";
         break;
      }
      default: {
         result += "SYNC_PT_STATE_UNKNOWN";
         break;
      }
   }

   return result;
}
