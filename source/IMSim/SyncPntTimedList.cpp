/*!
@file IMSim/SyncPntTimedList.cpp
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
@trick_link_dependency{../TrickHLA/Int64Time.cpp}
@trick_link_dependency{../TrickHLA/MutexLock.cpp}
@trick_link_dependency{../TrickHLA/MutexProtection.cpp}
@trick_link_dependency{../TrickHLA/SyncPoint.cpp}
@trick_link_dependency{../TrickHLA/SyncPntListBase.cpp}
@trick_link_dependency{SyncPntTimed.cpp}
@trick_link_dependency{SyncPntTimedList.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/release.h"

// HLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/SyncPntListBase.hh"
#include "TrickHLA/SyncPntLoggable.hh"
#include "TrickHLA/SyncPoint.hh"

// IMSim include files.
#include "IMSim/SyncPntTimed.hh"
#include "IMSim/SyncPntTimedList.hh"
#include "IMSim/SyncPntTimedLoggable.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;
using namespace IMSim;

/*!
 * @job_class{initialization}
 */
SyncPntTimedList::SyncPntTimedList()
{
   return;
}

SyncPoint *SyncPntTimedList::add_sync_point(
   wstring const &label )
{
   Int64Time time( 0.0 );
   return add_sync_point( label, time );
}

SyncPoint *SyncPntTimedList::add_sync_point(
   wstring const   &label,
   Int64Time const &time )
{
   SyncPntTimed *sp = new SyncPntTimed( time, label );

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );
   sync_point_list.push_back( sp );

   return sp;
}

bool SyncPntTimedList::achieve_all_sync_points(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   Int64Time const                  &check_time )
{
   bool achieved = false;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !sync_point_list.empty() ) {
      vector< SyncPoint * >::const_iterator i;

      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         // Cast the SyncPoint pointer to a SyncPntTimed pointer.
         SyncPntTimed *sp = dynamic_cast< SyncPntTimed * >( *i );

         if ( ( sp != NULL ) && !sp->is_achieved() ) {
            if ( sp->get_time() <= check_time ) {
               if ( achieve_sync_point( rti_ambassador, sp ) ) {
                  achieved = true;
               }
            }
         }
      }
   }
   return achieved;
}

bool SyncPntTimedList::check_sync_points(
   Int64Time const &check_time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !sync_point_list.empty() ) {
      vector< SyncPoint * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         // Cast the SyncPoint pointer to a SyncPntTimed pointer.
         SyncPntTimed const *timed_i = dynamic_cast< SyncPntTimed * >( *i );
         if ( ( timed_i->get_state() == TrickHLA::SYNC_PT_STATE_EXISTS )
              && ( timed_i->get_time() <= check_time ) ) {
            return true;
         }
      }
   }
   return false;
}

void SyncPntTimedList::convert_sync_points( SyncPntLoggable *sync_points )
{
   // Cast the SyncPntLoggable pointer to a SyncPntTimedLoggable pointer.
   SyncPntTimedLoggable *timed_sync_points = dynamic_cast< SyncPntTimedLoggable * >( sync_points );

   // If the cast failed, then treat it like a regular SyncPoint but warn user.
   if ( timed_sync_points == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntTimedList::convert_sync_pts():" << __LINE__
             << ": Could not cast synchronization points to timed synchronization points!"
             << THLA_ENDL;
      send_hs( stderr, errmsg.str().c_str() );
      SyncPntListBase::convert_sync_points( sync_points );
   } else {
      int                                   loop = 0;
      vector< SyncPoint * >::const_iterator i;

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         // Cast the SyncPoint pointer to a SyncPntTimed pointer.
         SyncPntTimed *timed_i = dynamic_cast< SyncPntTimed * >( *i );
         if ( timed_i == NULL ) {
            ( *i )->convert( sync_points[loop++] );
         } else {
            timed_i->convert( timed_sync_points[loop++] );
         }
      }
   }
}

void SyncPntTimedList::print_sync_points()
{
   vector< SyncPoint * >::const_iterator i;

   string sync_point_label;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   ostringstream msg;
   msg << "SyncPntTimedList::print_sync_points():" << __LINE__ << endl
       << "#############################" << endl
       << "Sync Point Dump: " << sync_point_list.size() << endl;

   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      // Cast the SyncPoint pointer to a SyncPntTimed pointer.
      SyncPntTimed const *timed_i = dynamic_cast< SyncPntTimed * >( *i );
      sync_point_label.assign( ( *i )->get_label().begin(), ( *i )->get_label().end() );
      msg << sync_point_label << " "
          << timed_i->get_time().get_time_in_seconds() << " "
          << ( *i )->get_state() << endl;
   }
   msg << "#############################" << endl;
   send_hs( stdout, msg.str().c_str() );
}
