/*!
@file TrickHLA/SyncPnt.cpp
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
@trick_link_dependency{SyncPnt.cpp}

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

// TrickHLA includes.
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPnt.hh"
#include "../../include/TrickHLA/SyncPntLoggable.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPnt::SyncPnt()
   : label( L"" ),
     state( SYNC_PT_STATE_EXISTS )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPnt::SyncPnt(
   std::wstring const &l )
   : label( l ),
     state( SYNC_PT_STATE_EXISTS )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SyncPnt::~SyncPnt()
{
   return;
}

bool SyncPnt::is_valid()
{
   return ( ( this->state == SYNC_PT_STATE_EXISTS )
            || ( this->state == SYNC_PT_STATE_REGISTERED )
            || ( this->state == SYNC_PT_STATE_ANNOUNCED )
            || ( this->state == SYNC_PT_STATE_ACHIEVED )
            || ( this->state == SYNC_PT_STATE_SYNCHRONIZED ) );
}

bool SyncPnt::exists()
{
   return ( this->state == SYNC_PT_STATE_EXISTS );
}

bool SyncPnt::is_registered()
{
   return ( this->state == SYNC_PT_STATE_REGISTERED );
}

bool SyncPnt::is_announced()
{
   return ( this->state == SYNC_PT_STATE_ANNOUNCED );
}

bool SyncPnt::is_achieved()
{
   return ( this->state == SYNC_PT_STATE_ACHIEVED );
}

bool SyncPnt::is_synchronized()
{
   return ( this->state == SYNC_PT_STATE_SYNCHRONIZED );
}

bool SyncPnt::is_error()
{
   return ( ( this->state != SYNC_PT_STATE_EXISTS )
            && ( this->state != SYNC_PT_STATE_REGISTERED )
            && ( this->state != SYNC_PT_STATE_ANNOUNCED )
            && ( this->state != SYNC_PT_STATE_ACHIEVED )
            && ( this->state != SYNC_PT_STATE_SYNCHRONIZED ) );
}

std::wstring SyncPnt::to_wstring()
{
   wstring result = L"[" + label + L"] -- ";
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

      case SYNC_PT_STATE_SYNCHRONIZED:
         result += L"SYNC_PT_STATE_SYNCHRONIZED";
         break;

      default:
         result += L"SYNC_PT_STATE_UNKNOWN";
   }
   return result;
}

void SyncPnt::convert( SyncPntLoggable &log_sync_pnt )
{
   log_sync_pnt.label = StringUtilities::ip_strdup_wstring( this->label );
   log_sync_pnt.state = this->state;
   return;
}
