/*!
@file IMSim/SyncPntPauseList.cpp
@ingroup IMSim
@brief Represents an HLA Synchronization Point in Trick.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{IMSim}

@tldh
@trick_link_dependency{../TrickHLA/MutexLock.cpp}
@trick_link_dependency{../TrickHLA/MutexProtection.cpp}
@trick_link_dependency{../TrickHLA/SyncPnt.cpp}
@trick_link_dependency{../TrickHLA/SyncPntTimed.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{SyncPntPauseList.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Robert G. Phillips, Titan Systems Corp., DIS, Oct 2004, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <algorithm>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/release.h"

#include "../../include/IMSim/SyncPntPauseList.hh"
// HLA include files.
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/SyncPnt.hh"
#include "TrickHLA/Types.hh"

// IMSim include files.
#include "../../include/TrickHLA/SyncPntTimed.hh"
#include "IMSim/Types.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;
using namespace IMSim;

/*!
 * @job_class{initialization}
 */
SyncPntPauseList::SyncPntPauseList()
   : state( PAUSE_POINT_STATE_UNKNOWN )
{
   return;
}

bool SyncPntPauseList::clear_sync_point(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL )
              && ( sp->get_state() == SYNC_PT_STATE_ACHIEVED )
              && ( label.compare( sp->get_label() ) == 0 ) ) {

            if ( sp->get_label().find( L"stop", 0 ) == 0 ) {
               this->state = PAUSE_POINT_STATE_EXIT;
            } else if ( sp->get_label().find( L"restart", 0 ) == 0 ) {
               this->state = PAUSE_POINT_STATE_RESTART;
            } else if ( sp->get_label().find( L"reconfig", 0 ) == 0 ) {
               this->reconfig_name = sp->get_label().substr( 9 );
               this->state         = PAUSE_POINT_STATE_RECONFIG;
            }

            sync_point_list.erase( i );
            delete sp;
            i = sync_point_list.end();

            return true;
         }
      }
   }
   return false;
}

bool SyncPntPauseList::is_sync_point_state_achieved( SyncPnt const *sync_pnt )
{
   return ( sync_pnt->get_state() == SYNC_PT_STATE_ACHIEVED );
}

void SyncPntPauseList::check_state()
{
   if ( ( state == PAUSE_POINT_STATE_EXIT )
        || ( state == PAUSE_POINT_STATE_RESTART )
        || ( state == PAUSE_POINT_STATE_RECONFIG ) ) {
      return;
   }

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !sync_point_list.empty() ) {
      if ( std::any_of( sync_point_list.begin(),
                        sync_point_list.end(),
                        is_sync_point_state_achieved ) ) {
         this->state = PAUSE_POINT_STATE_FREEZE;
         return;
      }
   }

   // FIXME: Commenting out to test. This needs to be split between the
   // DIS::ExecutinoControl and the IMSim::ExecutionControl.
   // This code needs to be moved to DIS implementation.
   // if ( init_scheme == INIT_SCHEME_DIS_COMPATIBLE ) {
   // Transition into a RUN state immediately.
   // this->state = PAUSE_POINT_STATE_RUN;
   //} // This code needs to be moved to an IMSim implementation.
   // else {
   // We can only transition to the Run state if we are not currently in an
   // Unknown state. Also, do not jump into Run state if we are currently in
   // Freeze mode.
   if ( ( state != PAUSE_POINT_STATE_FREEZE ) && ( state != PAUSE_POINT_STATE_UNKNOWN ) ) {
      this->state = PAUSE_POINT_STATE_RUN;
   }
}

wstring SyncPntPauseList::to_wstring()
{
   wstring result;

   result = L"Pause Points\n  state: ";

   switch ( state ) {

      case PAUSE_POINT_STATE_ERROR:
         result += L"PAUSE_POINT_STATE_ERROR";
         break;

      case PAUSE_POINT_STATE_PENDING:
         result += L"PAUSE_POINT_STATE_PENDING";
         break;

      case PAUSE_POINT_STATE_ACKNOWLEDGED:
         result += L"PAUSE_POINT_STATE_ACKNOWLEDGED";
         break;

      case PAUSE_POINT_STATE_RUN:
         result += L"PAUSE_POINT_STATE_RUN";
         break;

      case PAUSE_POINT_STATE_EXIT:
         result += L"PAUSE_POINT_STATE_EXIT";
         break;

      case PAUSE_POINT_STATE_RESTART:
         result += L"PAUSE_POINT_STATE_RESTART";
         break;

      case PAUSE_POINT_STATE_RECONFIG:
         result += L"PAUSE_POINT_STATE_RECONFIG";
         break;

      case PAUSE_POINT_STATE_UNKNOWN:
      default:
         result += L"PAUSE_POINT_STATE_UNKNOWN";
         break;
   }

   vector< SyncPnt * >::const_iterator i;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      if ( sp != NULL ) {
         result += L"  " + sp->to_wstring() + L"\n";
      }
   }
   result += L"\n";

   return result;
}

void SyncPntPauseList::print_sync_points()
{
   vector< SyncPnt * >::const_iterator i;
   string                              sync_point_label;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   ostringstream msg;
   msg << "IMSim::SyncPntPauseList::print_sync_points():" << __LINE__ << endl
       << "#############################" << endl
       << "Pause Point Dump: " << sync_point_list.size() << endl;

   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      // Cast the SyncPnt pointer to a SyncPntTimed pointer.
      SyncPntTimed const *timed_i = dynamic_cast< SyncPntTimed * >( *i );
      sync_point_label.assign( ( *i )->get_label().begin(), ( *i )->get_label().end() );
      msg << sync_point_label << " "
          << timed_i->get_time().get_time_in_seconds() << " "
          << ( *i )->get_state() << endl;
   }
   msg << "#############################" << endl;
   send_hs( stdout, msg.str().c_str() );
}
