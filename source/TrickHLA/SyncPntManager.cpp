/*!
@file TrickHLA/SyncPntManager.cpp
@ingroup TrickHLA
@brief This class will manage different lists of HLA synchronization points. It
       is intended for this class to be extended by an Execution Control class.

@copyright Copyright 2024 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{SyncPnt.cpp}
@trick_link_dependency{SyncPntManager.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2024, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/release.h"

// HLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPnt.hh"
#include "TrickHLA/SyncPntManager.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPntManager::SyncPntManager()
   : mutex(),
     sync_pnt_lists()
{
   return;
}

/*!
 * @details This is a pure virtual destructor.
 * @job_class{shutdown}
 */
SyncPntManager::~SyncPntManager()
{
   // Clear/remove everything out of the lists.
   SyncPntListMap::const_iterator iter;
   for ( iter = sync_pnt_lists.begin(); iter != sync_pnt_lists.end(); ++iter ) {
      SyncPntListVec list = iter->second;
      while ( !list.empty() ) {
         if ( *list.begin() != NULL ) {
            delete ( *list.begin() );
            list.erase( list.begin() );
         }
      }
      list.clear();
   }
   sync_pnt_lists.clear();

   // Make sure we destroy the mutex.
   mutex.destroy();
}

bool SyncPntManager::add_sync_point_list(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   // Create the named list only if it does not already exist.
   bool status = false;
   if ( !contains_sync_point_list_name( list_name ) ) {
      sync_pnt_lists[list_name] = SyncPntListVec();
      status = true;
   }
   return status;
}

SyncPnt *SyncPntManager::get_sync_point( std::wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPntListMap::const_iterator iter;
   for ( iter = sync_pnt_lists.begin(); iter != sync_pnt_lists.end(); ++iter ) {

      SyncPntListVec::const_iterator i;
      for ( i = iter->second.begin(); i != iter->second.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL ) && ( label.compare( sp->get_label() ) == 0 ) ) {
            return sp;
         }
      }
   }
   return NULL;
}

SyncPntListVec SyncPntManager::get_sync_point_list(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   // Use an iterator so that we don't throw an exception if we used an access
   // function instead (i.e. [] or at()) and the named liest does not exist.
   SyncPntListMap::const_iterator iter;
   for ( iter = sync_pnt_lists.begin(); iter != sync_pnt_lists.end(); ++iter ) {
      if ( list_name.compare( iter->first ) == 0 ) {
         return iter->second;
      }
   }
   return SyncPntListVec();
}

bool SyncPntManager::remove_sync_point_list(
   string const &list_name )
{
   return false;
}

bool SyncPntManager::add_sync_point(
   wstring const &label,
   string const  &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   if ( contains_sync_point( label ) ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::add_sync_point():" << __LINE__
             << " ERROR: The sync-point label '" << label.c_str()
             << "' has already been added!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( !contains_sync_point_list_name( list_name ) ) {
      if ( !add_sync_point_list( list_name ) ) {
         ostringstream errmsg;
         errmsg << "SyncPntManager::add_sync_point():" << __LINE__
                << " ERROR: Could not add the named sync-point list for '"
                << list_name << "'!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // Add the sync-point to the corresponding named list.
   SyncPnt *sp = new SyncPnt( label );
   sync_pnt_lists[list_name].push_back( sp );
   return true;
}

bool SyncPntManager::add_sync_point(
   wstring const &label,
   string const  &list_name,
   Int64Time      time )
{
   // TODO: Add as a timed sync point.
   return add_sync_point( label, list_name );
}

bool SyncPntManager::contains_sync_point(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPntListMap::const_iterator iter;
   for ( iter = sync_pnt_lists.begin(); iter != sync_pnt_lists.end(); ++iter ) {

      SyncPntListVec::const_iterator i;
      for ( i = iter->second.begin(); i != iter->second.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL ) && ( label.compare( sp->get_label() ) == 0 ) ) {
            return true;
         }
      }
   }
   return false;
}

bool SyncPntManager::contains_sync_point_list_name(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   // Use an iterator so that we don't throw an exception if we used an access
   // function instead (i.e. [] or at()) and the named liest does not exist.
   SyncPntListMap::const_iterator iter;
   for ( iter = sync_pnt_lists.begin(); iter != sync_pnt_lists.end(); ++iter ) {
      if ( list_name.compare( iter->first ) == 0 ) {
         return true;
      }
   }
   return false;
}

bool SyncPntManager::is_sync_point_registered(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_registered() );
}

bool SyncPntManager::register_sync_point(
   wstring const &label )
{
   return register_sync_point( get_sync_point( label ) );
}

bool SyncPntManager::register_sync_point(
   wstring const           &label,
   FederateHandleSet const &handle_set )
{
   return register_sync_point( get_sync_point( label ), handle_set );
}

// True if at least one sync-point is registered.
bool SyncPntManager::register_all_sync_points(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   bool           status = false;
   SyncPntListVec list   = get_sync_point_list( list_name );

   SyncPntListVec::const_iterator i;
   for ( i = list.begin(); i != list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      if ( register_sync_point( sp ) ) {
         status = true;
      }
   }
   return status;
}

bool SyncPntManager::register_all_sync_points(
   string const            &list_name,
   FederateHandleSet const &handle_set )
{
   MutexProtection auto_unlock_mutex( &mutex );

   bool           status = false;
   SyncPntListVec list   = get_sync_point_list( list_name );

   SyncPntListVec::const_iterator i;
   for ( i = list.begin(); i != list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      if ( register_sync_point( sp, handle_set ) ) {
         status = true;
      }
   }
   return status;
}

bool register_sync_point(
   SyncPnt *sp )
{
   MutexProtection auto_unlock_mutex( &mutex );

   //TODO: Add implementation.
   return false;
}

bool register_sync_point(
   SyncPnt                 *sp,
   FederateHandleSet const &handle_set )
{
   MutexProtection auto_unlock_mutex( &mutex );

   //TODO: Add implementation.
   return false;
}

bool SyncPntManager::is_sync_point_announced(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_announced() );
}

bool SyncPntManager::wait_for_sync_point_announced(
   wstring const &label )
{
   return wait_for_sync_point_announced( get_sync_point( label ) );
}

bool SyncPntManager::wait_for_all_sync_points_announced(
   string const &list_name )
{
   // NOTE: We can't hold the mutex lock while waiting, otherwise
   // it will cause deadlock.

   bool           status = false;
   SyncPntListVec list   = get_sync_point_list( list_name );

   SyncPntListVec::const_iterator i;
   for ( i = list.begin(); i != list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      if ( wait_for_sync_point_announced( sp ) ) {
         status = true;
      }
   }
   return status;
}

bool SyncPntManager::wait_for_sync_point_announced(
   SyncPnt *sp )
{
   // NOTE: We can't hold the mutex lock while waiting, otherwise
   // it will cause deadlock.

   if ( sp == NULL ) {
      // ERROR
   }

   //TODO: Add implementation.
   return false;
}

bool SyncPntManager::is_sync_point_achieved(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_achieved() );
}

bool SyncPntManager::achieve_sync_point(
   wstring const &label )
{
   return achieve_sync_point( get_sync_point( label ) );
}

bool SyncPntManager::achieve_all_sync_points(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   bool           status = false;
   SyncPntListVec list   = get_sync_point_list( list_name );

   SyncPntListVec::const_iterator i;
   for ( i = list.begin(); i != list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      if ( achieve_sync_point( sp ) ) {
         status = true;
      }
   }
   return status;
}

bool SyncPntManager::achieve_sync_point(
   SyncPnt *sp )
{
   MutexProtection auto_unlock_mutex( &mutex );

   //TODO: Add implementation.
   return false;
}

bool SyncPntManager::is_sync_point_synchronized(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_synchronized() );
}

bool SyncPntManager::wait_for_sync_point_synchronized(
   wstring const &label )
{
   return wait_for_sync_point_synchronized( get_sync_point( label ) );
}

bool SyncPntManager::wait_for_all_sync_points_synchronized(
   string const &list_name )
{
   // NOTE: We can't hold the mutex lock while waiting, otherwise
   // it will cause deadlock.

   bool           status = false;
   SyncPntListVec list   = get_sync_point_list( list_name );

   SyncPntListVec::const_iterator i;
   for ( i = list.begin(); i != list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      if ( wait_for_sync_point_synchronized( sp ) ) {
         status = true;
      }
   }
   return status;
}

bool SyncPntManager::wait_for_sync_point_synchronized(
   SyncPnt *sp )
{
   // NOTE: We can't hold the mutex lock while waiting, otherwise
   // it will cause deadlock.

   //TODO: Add implementation.
   return false;
}

// Callback from FedAmb.
void SyncPntManager::sync_point_registration_succeeded(
   wstring const &label )
{
   //TODO: Add implementation.
}

// Callback from FedAmb.
void SyncPntManager::sync_point_registration_failed(
   wstring const                    &label,
   SynchronizationPointFailureReason reason )
{
   //TODO: Add implementation.
}

// Callback from FedAmb.
void SyncPntManager::sync_point_announced(
   wstring const            &label,
   VariableLengthData const &user_supplied_tag )
{
   //TODO: Add implementation.
}

// Callback from FedAmb.
void SyncPntManager::sync_point_federation_synchronized(
   wstring const &label )
{
   //TODO: Add implementation.
}
