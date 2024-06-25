/*!
@file TrickHLA/SyncPntTimed.cpp
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
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{SyncPoint.cpp}
@trick_link_dependency{SyncPntTimed.cpp}

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
#include "TrickHLA/SyncPntTimed.hh"
#include "TrickHLA/SyncPntTimedLoggable.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPoint.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPntTimed::SyncPntTimed()
   : time( 0.0 )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPntTimed::SyncPntTimed( std::wstring const &label )
   : SyncPoint( label ), time( 0.0 )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPntTimed::SyncPntTimed( Int64Time const &t, std::wstring const &label )
   : SyncPoint( label ), time( t )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SyncPntTimed::~SyncPntTimed()
{
   return;
}

std::wstring SyncPntTimed::to_wstring()
{
   wstring result = L"[" + label + L"/" + time.to_wstring() + L"] -- ";
   switch ( this->state ) {

      case SYNC_PT_STATE_ERROR:
         result += L"SYNC_PT_STATE_ERROR";
         break;

      case SYNC_PT_STATE_EXISTS:
         result += L"SYNC_PT_STATE_EXISTS";
         break;

      case SYNC_PT_STATE_REGISTERED:
         result += L"SYNC_PT_STATE_REGISTERED";
         break;

      case SYNC_PT_STATE_ANNOUNCED:
         result += L"SYNC_PT_STATE_ANNOUNCED";
         break;

      case SYNC_PT_STATE_ACHIEVED:
         result += L"SYNC_PT_STATE_ACHIEVED";
         break;

      default:
         result += L"SYNC_PT_STATE_UNKNOWN";
   }

   return result;
}

void SyncPntTimed::convert( SyncPntLoggable &log_sync_pnt )
{
   // Cast the SyncPntLoggable to a SyncPntTimedLoggable.
   SyncPntTimedLoggable *timed_log_sync_pnt = dynamic_cast< SyncPntTimedLoggable * >( &log_sync_pnt );

   // If the cast failed, then treat it like a regular SyncPoint but warn user.
   if ( timed_log_sync_pnt == NULL ) {
      ostringstream errmsg;
      errmsg
         << "SyncPntTimed::convert():" << __LINE__
         << ": Could not cast synchronization point to timed synchronization point!" << THLA_ENDL;
      send_hs( stderr, errmsg.str().c_str() );
      log_sync_pnt.label = StringUtilities::ip_strdup_wstring( this->label );
      log_sync_pnt.state = this->state;
   } else {
      timed_log_sync_pnt->time  = this->time.get_base_time();
      timed_log_sync_pnt->label = StringUtilities::ip_strdup_wstring( this->label );
      timed_log_sync_pnt->state = this->state;
   }

   return;
}
