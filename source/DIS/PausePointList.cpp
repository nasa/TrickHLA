/*!
@file DIS/PausePointList.cpp
@ingroup DIS
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

@python_module{DIS}

@tldh
@trick_link_dependency{../TrickHLA/SyncPnt.cpp}
@trick_link_dependency{../TrickHLA/TimedSyncPnt.cpp}
@trick_link_dependency{PausePointList.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Robert G. Phillips, Titan Systems Corp., DIS, Oct 2004, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
//#include <iostream>
//#include <sstream>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/release.h"

// HLA include files.
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPnt.hh"
#include "TrickHLA/TimedSyncPnt.hh"

// IMSim include files.
#include "DIS/PausePointList.hh"
#include "DIS/Types.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;
using namespace DIS;

/*!
 * @job_class{initialization}
 */
PausePointList::PausePointList()
   : state( PAUSE_POINT_STATE_UNKNOWN )
{
   return;
}

bool PausePointList::clear_sync_pnt(
   wstring const &label )
{
   lock_read_write();
   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL )
              && ( sp->get_state() == SYNC_PNT_STATE_ACHIEVED )
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

            unlock_read_write();
            return true;
         }
      }
   }
   unlock_read_write();
   return false;
}

void PausePointList::check_state()
{
   if ( ( state == PAUSE_POINT_STATE_EXIT )
        || ( state == PAUSE_POINT_STATE_RESTART )
        || ( state == PAUSE_POINT_STATE_RECONFIG ) ) {
      return;
   }

   lock_read_only();
   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         if ( ( *i )->get_state() == SYNC_PNT_STATE_ACHIEVED ) {
            this->state = PAUSE_POINT_STATE_FREEZE;
            unlock_read_only();
            return;
         }
      }
   }
   unlock_read_only();

   // FIXME: Commenting out to test. This needs to be split between the
   // DIS::ExecutinoControl and the IMSim::ExecutionControl.
   // This code needs to be moved to DIS implementation.
   //if ( init_scheme == INIT_SCHEME_DIS_COMPATIBLE ) {
   // Transition into a RUN state immediately.
   //this->state = PAUSE_POINT_STATE_RUN;
   //} // This code needs to be moved to an IMSim implementation.
   //else {
   // We can only transition to the Run state if we are not currently in an
   // Unknown state. Also, do not jump into Run state if we are currently in
   // Freeze mode.
   if ( ( state != PAUSE_POINT_STATE_FREEZE ) && ( state != PAUSE_POINT_STATE_UNKNOWN ) ) {
      this->state = PAUSE_POINT_STATE_RUN;
   }
   //}
}

wstring PausePointList::to_string()
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
   lock_read_only();
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      if ( sp != NULL ) {
         result += L"  " + sp->to_string() + L"\n";
      }
   }
   result += L"\n";
   unlock_read_only();

   return result;
}

void PausePointList::print_sync_pnts()
{
   vector< SyncPnt * >::const_iterator i;
   string                              sync_point_label;

   ostringstream msg;
   msg << "DIS::PausePointList::print_sync_pnts():" << __LINE__ << endl
       << "#############################" << endl
       << "Pause Point Dump: " << sync_point_list.size() << endl;
   lock_read_only();
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      // Cast the SyncPnt pointer to a TimedSyncPnt pointer.
      TimedSyncPnt *timed_i = dynamic_cast< TimedSyncPnt * >( *i );
      sync_point_label.assign( ( *i )->get_label().begin(), ( *i )->get_label().end() );
      msg << sync_point_label << " "
          << timed_i->get_time().get_time_in_seconds() << " "
          << ( *i )->get_state() << endl;
   }
   msg << "#############################" << endl;
   send_hs( stdout, (char *)msg.str().c_str() );

   unlock_read_only();
}
