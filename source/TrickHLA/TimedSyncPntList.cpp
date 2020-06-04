/*!
@file TrickHLA/TimedSyncPntList.cpp
@ingroup TrickHLA
@brief This class provides and abstract base class as the base implementation
for storing and managing HLA synchronization points for Trick.

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
@trick_link_dependency{SyncPnt.cpp}
@trick_link_dependency{TimedSyncPntList.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <unistd.h>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/release.h"

// HLA include files.
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/TimedSyncPntList.hh"
#include "TrickHLA/Utilities.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
TimedSyncPntList::TimedSyncPntList()
{
}

void TimedSyncPntList::add_sync_pnt(
   wstring const &label )
{
   Int64Time time( 0.0 );

   add_sync_pnt( label, time );
}

void TimedSyncPntList::add_sync_pnt(
   wstring const &  label,
   const Int64Time &time )
{
   TimedSyncPnt *sp = new TimedSyncPnt( time, label );
   lock_read_write();
   sync_point_list.push_back( sp );
   unlock_read_write();
}

bool TimedSyncPntList::achieve_all_sync_pnts(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   const Int64Time &                 checkTime )
/*
  throws
    SynchronizationPointLabelNotAnnounced,
    FederateNotExecutionMember,
    SaveInProgress,
    RestoreInProgress,
    RTIinternalError
*/
{
   bool wasAcknowledged = false;

   lock_read_only();
   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;

      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         // Cast the SyncPnt pointer to a TimedSyncPnt pointer.
         TimedSyncPnt *sp = dynamic_cast< TimedSyncPnt * >( *i );

         if ( sp != NULL && sp->exists() && !sp->is_achieved() ) {
            if ( sp->get_time() <= checkTime ) {
               if ( this->achieve_sync_pnt( rti_ambassador, sp ) ) {
                  wasAcknowledged = true;
               }
            }
         }
      }
   }
   unlock_read_only();

   return wasAcknowledged;
}

bool TimedSyncPntList::check_sync_pnts(
   const Int64Time &checkTime )
{
   lock_read_only();
   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         // Cast the SyncPnt pointer to a TimedSyncPnt pointer.
         TimedSyncPnt *timed_i = dynamic_cast< TimedSyncPnt * >( *i );
         if ( ( timed_i->get_state() == SYNC_PNT_STATE_EXISTS ) && ( timed_i->get_time() <= checkTime ) ) {
            unlock_read_only();
            return true;
         }
      }
   }
   unlock_read_only();
   return false;
}

void TimedSyncPntList::convert_sync_pts( LoggableSyncPnt *sync_points )
{
   // Cast the LoggableSyncPnt pointer to a LoggableTimedSyncPnt pointer.
   LoggableTimedSyncPnt *timed_sync_points = dynamic_cast< LoggableTimedSyncPnt * >( sync_points );

   // If the cast failed, then treat it like a regular SyncPnt but warn user.
   if ( timed_sync_points == NULL ) {
      ostringstream errmsg;
      errmsg
         << "TimedSyncPntList::convert_sync_pts():" << __LINE__
         << ": Could not cast synchronization points to timed synchronization points!" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      SyncPntListBase::convert_sync_pts( sync_points );
   } else {
      int                                 loop = 0;
      vector< SyncPnt * >::const_iterator i;

      lock_read_only();
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         // Cast the SyncPnt pointer to a TimedSyncPnt pointer.
         TimedSyncPnt *timed_i = dynamic_cast< TimedSyncPnt * >( *i );
         if ( timed_i == NULL ) {
            ( *i )->convert( sync_points[loop++] );
         } else {
            timed_i->convert( timed_sync_points[loop++] );
         }
      }
   }

   unlock_read_only();
}

void TimedSyncPntList::print_sync_pnts()
{
   vector< SyncPnt * >::const_iterator i;
   string                              sync_point_label;

   ostringstream msg;
   msg << "TimedSyncPntList::print_sync_pnts():" << __LINE__ << endl
       << "#############################" << endl
       << "Sync Point Dump: " << sync_point_list.size() << endl;
   lock_read_only();
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      // Cast the SyncPnt pointer to a TimedSyncPnt pointer.
      TimedSyncPnt *timed_i = dynamic_cast< TimedSyncPnt * >( *i );
      sync_point_label.assign( ( *i )->get_label().begin(), ( *i )->get_label().end() );
      msg << sync_point_label << " "
          << timed_i->get_time().getDoubleTime() << " "
          << ( *i )->get_state() << endl;
   }
   msg << "#############################" << endl;
   send_hs( stdout, (char *)msg.str().c_str() );

   unlock_read_only();
}
