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
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{SyncPnt.cpp}
@trick_link_dependency{SyncPntListBase.cpp}
@trick_link_dependency{TimedSyncPnt.cpp}
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

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/release.h"

// HLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/LoggableSyncPnt.hh"
#include "TrickHLA/LoggableTimedSyncPnt.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPnt.hh"
#include "TrickHLA/SyncPntListBase.hh"
#include "TrickHLA/TimedSyncPnt.hh"
#include "TrickHLA/TimedSyncPntList.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
TimedSyncPntList::TimedSyncPntList()
{
   return;
}

void TimedSyncPntList::add_sync_point(
   wstring const &label )
{
   Int64Time time( 0.0 );
   add_sync_point( label, time );
}

void TimedSyncPntList::add_sync_point(
   wstring const   &label,
   const Int64Time &time )
{
   TimedSyncPnt *sp = new TimedSyncPnt( time, label );

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );
   sync_point_list.push_back( sp );
}

bool TimedSyncPntList::achieve_all_sync_points(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   const Int64Time                  &checkTime )
{
   bool wasAcknowledged = false;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;

      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         // Cast the SyncPnt pointer to a TimedSyncPnt pointer.
         TimedSyncPnt *sp = dynamic_cast< TimedSyncPnt * >( *i );

         if ( sp != NULL && sp->exists() && !sp->is_achieved() ) {
            if ( sp->get_time() <= checkTime ) {
               if ( this->achieve_sync_point( rti_ambassador, sp ) ) {
                  wasAcknowledged = true;
               }
            }
         }
      }
   }
   return wasAcknowledged;
}

bool TimedSyncPntList::check_sync_points(
   const Int64Time &checkTime )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         // Cast the SyncPnt pointer to a TimedSyncPnt pointer.
         TimedSyncPnt *timed_i = dynamic_cast< TimedSyncPnt * >( *i );
         if ( ( timed_i->get_state() == SYNC_PT_STATE_EXISTS )
              && ( timed_i->get_time() <= checkTime ) ) {
            return true;
         }
      }
   }
   return false;
}

void TimedSyncPntList::convert_sync_points( LoggableSyncPnt *sync_points )
{
   // Cast the LoggableSyncPnt pointer to a LoggableTimedSyncPnt pointer.
   LoggableTimedSyncPnt *timed_sync_points = dynamic_cast< LoggableTimedSyncPnt * >( sync_points );

   // If the cast failed, then treat it like a regular SyncPnt but warn user.
   if ( timed_sync_points == NULL ) {
      ostringstream errmsg;
      errmsg << "TimedSyncPntList::convert_sync_pts():" << __LINE__
             << ": Could not cast synchronization points to timed synchronization points!"
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      SyncPntListBase::convert_sync_points( sync_points );
   } else {
      int                                 loop = 0;
      vector< SyncPnt * >::const_iterator i;

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

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
}

void TimedSyncPntList::print_sync_points()
{
   vector< SyncPnt * >::const_iterator i;

   string sync_point_label;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   ostringstream msg;
   msg << "TimedSyncPntList::print_sync_points():" << __LINE__ << endl
       << "#############################" << endl
       << "Sync Point Dump: " << sync_point_list.size() << endl;

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
}
