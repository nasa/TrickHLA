/*!
@file TrickHLA/TimedSyncPnt.cpp
@ingroup TrickHLA
@brief This class provides a sync-point implementation for storing and managing
TrickHLA synchronization points.

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
@trick_link_dependency{TimedSyncPnt.cpp}
@trick_link_dependency{Int64Time.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA JSC ER7, TrickHLA, Jan 2019, --, Create from old TrickHLASyncPtsBase class.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

//System includes.
#include <unistd.h>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA includes.
#include "TrickHLA/Federate.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/TimedSyncPnt.hh"
#include "TrickHLA/Utilities.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
TimedSyncPnt::TimedSyncPnt()
   : time( 0.0 )
{
   return;
}

/*!
 * @job_class{initialization}
 */
TimedSyncPnt::TimedSyncPnt( std::wstring const &l )
   : SyncPnt( l ), time( 0.0 )
{
   return;
}

/*!
 * @job_class{initialization}
 */
TimedSyncPnt::TimedSyncPnt( const Int64Time &t, std::wstring const &l )
   : SyncPnt( l ), time( t )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
TimedSyncPnt::~TimedSyncPnt()
{
   return;
}

std::wstring TimedSyncPnt::to_string()
{
   wstring result = L"[" + label + L"/" + time.toString() + L"] -- ";
   switch ( this->state ) {

   case SYNC_PNT_STATE_ERROR:
      result += L"SYNC_PNT_STATE_ERROR";
      break;

   case SYNC_PNT_STATE_EXISTS:
      result += L"SYNC_PNT_STATE_EXISTS";
      break;

   case SYNC_PNT_STATE_REGISTERED:
      result += L"SYNC_PNT_STATE_REGISTERED";
      break;

   case SYNC_PNT_STATE_ANNOUNCED:
      result += L"SYNC_PNT_STATE_ANNOUNCED";
      break;

   case SYNC_PNT_STATE_ACHIEVED:
      result += L"SYNC_PNT_STATE_ACHIEVED";
      break;

   default:
      result += L"SYNC_PNT_STATE_UNKNOWN";
   }

   return result;
}

void TimedSyncPnt::convert( LoggableSyncPnt &log_sync_pnt )
{
   // Cast the LoggableSyncPnt to a LoggableTimedSyncPnt.
   LoggableTimedSyncPnt *timed_log_sync_pnt = dynamic_cast< LoggableTimedSyncPnt * >( &log_sync_pnt );

   // If the cast failed, then treat it like a regular SyncPnt but warn user.
   if ( timed_log_sync_pnt == NULL ) {
      ostringstream errmsg;
      errmsg
         << "TimedSyncPnt::convert():" << __LINE__
         << ": Could not cast synchronization point to timed synchronization point!" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      log_sync_pnt.label = StringUtilities::ip_strdup_wstring( this->label );
      log_sync_pnt.state = this->state;
   } else {
      timed_log_sync_pnt->time  = this->time.getTimeInMicros();
      timed_log_sync_pnt->label = StringUtilities::ip_strdup_wstring( this->label );
      timed_log_sync_pnt->state = this->state;
   }

   return;
}
