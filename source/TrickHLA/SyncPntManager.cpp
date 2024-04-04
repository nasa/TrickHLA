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
     sync_pnt_lists(),
     federate( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPntManager::SyncPntManager(
   Federate *fed )
   : mutex(),
     sync_pnt_lists(),
     federate( fed )
{
   return;
}

/*!
 * @details This is a pure virtual destructor.
 * @job_class{shutdown}
 */
SyncPntManager::~SyncPntManager()
{
   // Clear/remove everything from the lists.
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
      status                    = true;
   }
   return status;
}

SyncPnt *SyncPntManager::get_sync_point(
   wstring const &label )
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
   // function instead (i.e. [] or at()) and the named list does not exist.
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
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPntListMap::const_iterator iter;
   for ( iter = sync_pnt_lists.begin(); iter != sync_pnt_lists.end(); ++iter ) {
      if ( list_name.compare( iter->first ) == 0 ) {
         SyncPntListVec list = iter->second;
         while ( !list.empty() ) {
            if ( *list.begin() != NULL ) {
               delete ( *list.begin() );
               list.erase( list.begin() );
            }
         }
         list.clear();
         return true;
      }
   }
   return false;
}

bool SyncPntManager::add_sync_point(
   wstring const &label,
   string const  &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   if ( contains_sync_point( label ) ) {
      string name;
      StringUtilities::to_string( name, label );
      ostringstream errmsg;
      errmsg << "SyncPntManager::add_sync_point():" << __LINE__
             << " ERROR: The sync-point label '" << name
             << "' has already been added!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   if ( !contains_sync_point_list_name( list_name ) ) {
      if ( !add_sync_point_list( list_name ) ) {
         ostringstream errmsg;
         errmsg << "SyncPntManager::add_sync_point():" << __LINE__
                << " ERROR: Could not add the named sync-point list for '"
                << list_name << "'!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
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
         SyncPnt const *sp = ( *i );
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
   // function instead (i.e. [] or at()) and the named list does not exist.
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

/*!
 * @job_class{initialization}
 */
bool SyncPntManager::mark_sync_point_registered(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   if ( sp != NULL ) {
      sp->set_state( SYNC_PT_STATE_REGISTERED );
      return true;
   }
   return false;
}

bool SyncPntManager::register_sync_point(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );

   // If the sync-point is null then it is unknown.
   if ( sp == NULL ) {
      // Add the unknown sync-point to the unknown list so it can be registered.
      if ( !add_sync_point( label, UNKNOWN_SYNC_PNT_LIST ) ) {
         string name;
         StringUtilities::to_string( name, label );
         ostringstream errmsg;
         errmsg << "SyncPntManager::register_sync_point():" << __LINE__
                << " ERROR: Failed to add sync-point '" << name
                << "' to '" << UNKNOWN_SYNC_PNT_LIST << "' list!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      sp = get_sync_point( label );
   }
   return register_sync_point( sp );
}

bool SyncPntManager::register_sync_point(
   wstring const           &label,
   FederateHandleSet const &handle_set )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );

   // If the sync-point is null then it is unknown.
   if ( sp == NULL ) {
      // Add the unknown sync-point to the unknown list so it can be registered.
      if ( !add_sync_point( label, UNKNOWN_SYNC_PNT_LIST ) ) {
         string name;
         StringUtilities::to_string( name, label );
         ostringstream errmsg;
         errmsg << "SyncPntManager::register_sync_point():" << __LINE__
                << " ERROR: Failed to add sync-point '" << name
                << "' to '" << UNKNOWN_SYNC_PNT_LIST << "' list!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      sp = get_sync_point( label );
   }
   return register_sync_point( sp, handle_set );
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

bool SyncPntManager::register_sync_point(
   SyncPnt *sp )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL SyncPnt!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *RTI_amb = federate->get_RTI_ambassador();

   // Register the sync-point label.
   bool registered = false;
   try {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      RTI_amb->registerFederationSynchronizationPoint( sp->get_label(),
                                                       RTI1516_USERDATA( 0, 0 ) );
      // Mark the sync-point as registered.
      sp->set_state( SYNC_PT_STATE_REGISTERED );

      registered = true;

   } catch ( RTI1516_EXCEPTION const &e ) {

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string name;
      StringUtilities::to_string( name, sp->get_label() );
      ostringstream errmsg;
      errmsg << "SyncPntListBase::register_sync_point():" << __LINE__
             << " ERROR: Failed to register '" << name
             << "' synchronization point with RTI!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return registered;
}

bool SyncPntManager::register_sync_point(
   SyncPnt                 *sp,
   FederateHandleSet const &handle_set )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL SyncPnt!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *RTI_amb = federate->get_RTI_ambassador();

   // Register the sync-point label.
   bool registered = false;
   try {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      RTI_amb->registerFederationSynchronizationPoint( sp->get_label(),
                                                       RTI1516_USERDATA( 0, 0 ),
                                                       handle_set );
      // Mark the sync-point as registered.
      sp->set_state( SYNC_PT_STATE_REGISTERED );

      registered = true;

   } catch ( RTI1516_EXCEPTION const &e ) {

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string name;
      StringUtilities::to_string( name, sp->get_label() );
      ostringstream errmsg;
      errmsg << "SyncPntListBase::register_sync_point():" << __LINE__
             << " ERROR: Failed to register '" << name
             << "' synchronization point with RTI!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return registered;
}

bool SyncPntManager::is_sync_point_announced(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_announced() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPntManager::mark_sync_point_announced(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   if ( sp != NULL ) {
      sp->set_state( SYNC_PT_STATE_ANNOUNCED );
      return true;
   }
   return false;
}

bool SyncPntManager::wait_for_sync_point_announced(
   wstring const &label )
{
   SyncPnt *sp;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection auto_unlock_mutex( &mutex );

      sp = get_sync_point( label );

      // If the sync-point is null then it is unknown.
      if ( sp == NULL ) {
         // Add the unknown sync-point to the unknown list so it can be tracked.
         if ( !add_sync_point( label, UNKNOWN_SYNC_PNT_LIST ) ) {
            string name;
            StringUtilities::to_string( name, label );
            ostringstream errmsg;
            errmsg << "SyncPntManager::wait_for_sync_point_announced():" << __LINE__
                   << " ERROR: Failed to add sync-point '" << name
                   << "' to '" << UNKNOWN_SYNC_PNT_LIST << "' list!" << THLA_ENDL;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         sp = get_sync_point( label );
      }
   }

   return wait_for_sync_point_announced( sp );
}

bool SyncPntManager::wait_for_all_sync_points_announced(
   string const &list_name )
{
   // NOTE: Locking the mutex while waiting can cause deadlock for callbacks.

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
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::wait_for_sync_point_announced():" << __LINE__
             << " ERROR: Unexpected NULL SyncPnt!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::wait_for_sync_point_announced():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   bool announced = false;

   // Critical code section.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );
      announced = sp->is_announced();

      // The sync-point state must be SYNC_PT_STATE_REGISTERED.
      if ( !sp->exists() && !sp->is_registered() && !announced ) {
         string sp_status;
         StringUtilities::to_string( sp_status, sp->to_wstring() );

         ostringstream errmsg;
         errmsg << "SyncPntManager::wait_for_sync_point_announced():" << __LINE__
                << " ERROR: Bad sync-point state for sync-point!"
                << " The sync-point state: " << sp_status << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   bool         print_summary = DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE );
   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   // Wait for the sync-point to be announced.
   while ( !announced ) {

      if ( print_summary ) {
         print_summary = false;

         // Get the current sync-point status.
         string sp_status;
         StringUtilities::to_string( sp_status, sp->to_wstring() );

         ostringstream message;
         message << "SyncPntManager::wait_for_sync_point_announced():" << __LINE__
                 << " Sync-point: " << sp_status << THLA_ENDL;
         send_hs( stdout, message.str().c_str() );
      }

      // Always check to see is a shutdown was received.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep();

      // Critical code section.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &mutex );
         announced = sp->is_announced();
      }

      if ( !announced ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         // Check to make sure we're still a member of the federation execution.
         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SyncPntManager::wait_for_sync_point_announced():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution"
                      << " member. This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << THLA_ENDL;
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         // Determine if we should print a summary.
         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            print_summary = true;
         }
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      // Get the current sync-point status.
      string sp_status;
      StringUtilities::to_string( sp_status, sp->to_wstring() );

      ostringstream message;
      message << "SyncPntManager::wait_for_sync_point_announced():" << __LINE__
              << " Sync-point announced: " << sp_status << THLA_ENDL;
      send_hs( stdout, message.str().c_str() );
   }

   return announced;
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
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );

   // If the sync-point is null then it is unknown.
   if ( sp == NULL ) {
      // Add the unknown sync-point to the unknown list so it can be achieved.
      if ( !add_sync_point( label, UNKNOWN_SYNC_PNT_LIST ) ) {
         string name;
         StringUtilities::to_string( name, label );
         ostringstream errmsg;
         errmsg << "SyncPntManager::achieve_sync_point():" << __LINE__
                << " ERROR: Failed to add sync-point '" << name
                << "' to '" << UNKNOWN_SYNC_PNT_LIST << "' list!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      sp = get_sync_point( label );
   }

   return achieve_sync_point( sp );
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
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::achieve_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL SyncPnt!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::achieve_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string name;
      StringUtilities::to_string( name, sp->get_label() );
      ostringstream msg;
      msg << "SyncPntManager::achieve_sync_point():" << __LINE__
          << " Known Synchronization-Point '" << name << "', state:"
          << sp->get_state() << THLA_ENDL;
      send_hs( stdout, msg.str().c_str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *RTI_amb = federate->get_RTI_ambassador();

   bool achieved = false;
   try {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      RTI_amb->synchronizationPointAchieved( sp->get_label() );

      // Mark the sync-point as achieved.
      sp->set_state( SYNC_PT_STATE_ACHIEVED );

      achieved = true;

   } catch ( SynchronizationPointLabelNotAnnounced const &e ) {
      // Keep sync-point state the same, and return false.
   } catch ( FederateNotExecutionMember const &e ) {
      // Keep sync-point state the same, and return false.
   } catch ( SaveInProgress const &e ) {
      // Keep sync-point state the same, and return false.
   } catch ( RestoreInProgress const &e ) {
      // Keep sync-point state the same, and return false.
   } catch ( NotConnected const &e ) {
      // Keep sync-point state the same, and return false.
   } catch ( RTIinternalError const &e ) {
      // Keep sync-point state the same, and return false.
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return achieved;
}

bool SyncPntManager::is_sync_point_synchronized(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_synchronized() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPntManager::mark_sync_point_synchronized(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   if ( sp != NULL ) {

      // Mark the synchronization point at achieved which indicates the
      // federation is synchronized on the synchronization point.
      sp->set_state( SYNC_PT_STATE_SYNCHRONIZED );
      return true;
   }
   return false;
}

bool SyncPntManager::wait_for_sync_point_synchronized(
   wstring const &label )
{
   SyncPnt *sp;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection auto_unlock_mutex( &mutex );

      sp = get_sync_point( label );

      // If the sync-point is null then it is unknown.
      if ( sp == NULL ) {
         // Add the unknown sync-point to the unknown list so it can be tracked.
         if ( !add_sync_point( label, UNKNOWN_SYNC_PNT_LIST ) ) {
            string name;
            StringUtilities::to_string( name, label );
            ostringstream errmsg;
            errmsg << "SyncPntManager::wait_for_sync_point_synchronized():" << __LINE__
                   << " ERROR: Failed to add sync-point '" << name
                   << "' to '" << UNKNOWN_SYNC_PNT_LIST << "' list!" << THLA_ENDL;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         sp = get_sync_point( label );
      }
   }

   return wait_for_sync_point_synchronized( sp );
}

bool SyncPntManager::wait_for_all_sync_points_synchronized(
   string const &list_name )
{
   // NOTE: Locking the mutex while waiting can cause deadlock for callbacks.

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
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::wait_for_sync_point_synchronized():" << __LINE__
             << " ERROR: Unexpected NULL SyncPnt!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPntManager::wait_for_sync_point_synchronized():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   bool         print_summary = DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE );
   bool         synchronized;
   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   // Wait for the federation to synchronize on the sync-point.
   do {
      if ( print_summary ) {
         print_summary = false;
         string name;
         StringUtilities::to_string( name, sp->get_label() );
         ostringstream msg;
         msg << "SyncPntManager::wait_for_sync_point_synchronized():" << __LINE__
             << " Synchronization-Point '" << name << "'" << THLA_ENDL;
         send_hs( stdout, msg.str().c_str() );
      }

      // Critical code section.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks
         // the mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &mutex );

         synchronized = sp->is_synchronized();
         if ( synchronized ) {
            // Now that the federation is synchronized on the synchronization point,
            // return to SYNC_PT_STATE_EXISTS state.
            sp->set_state( SYNC_PT_STATE_EXISTS );
         }
      }

      if ( !synchronized ) {

         // Always check to see if a shutdown was received.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         // Check to make sure we're still a member of the federation execution.
         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SyncPntManager::wait_for_sync_point_synchronized():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution"
                      << " member. This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << THLA_ENDL;
               DebugHandler::terminate_with_message( errmsg.str() );
               return false;
            }
         }

         // Print a summary if we timeout waiting.
         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            print_summary = true;
         }
      }
   } while ( !synchronized );

   return true;
}

// Callback from FedAmb.
void SyncPntManager::sync_point_registration_succeeded(
   wstring const &label )
{
   if ( mark_sync_point_registered( label ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         send_hs( stdout, "SyncPntManager::sync_point_registration_succeeded():%d Label:'%ls'%c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
      }
   } else {
      string name;
      StringUtilities::to_string( name, label );
      ostringstream errmsg;
      errmsg << "SyncPntManager::sync_point_registration_succeeded():" << __LINE__
             << " ERROR: Failed to mark sync-point '" << name
             << "' as registered!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

// Callback from FedAmb.
void SyncPntManager::sync_point_registration_failed(
   wstring const                    &label,
   SynchronizationPointFailureReason reason )
{
   // Only handle the sync-points we know about.
   if ( contains_sync_point( label ) ) {

      // If the reason for the failure is that the label is not unique then
      // this means the sync-point is registered with the RTI it just means
      // we did not do it.
      if ( reason == SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE ) {
         if ( mark_sync_point_registered( label ) ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
               string name;
               StringUtilities::to_string( name, label );
               send_hs( stdout, "SyncPntManager::sync_point_registration_failed():%d Label:'%s' already exists.%c",
                        __LINE__, name.c_str(), THLA_NEWLINE );
            }
         } else {
            string name;
            StringUtilities::to_string( name, label );
            ostringstream errmsg;
            errmsg << "SyncPntManager::sync_point_registration_failed():" << __LINE__
                   << " ERROR: Failed to mark sync-point '" << name
                   << "' as registered." << THLA_ENDL;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      } else {
         string name;
         StringUtilities::to_string( name, label );
         ostringstream errmsg;
         errmsg << "SyncPntManager::sync_point_registration_failed():" << __LINE__
                << " ERROR: Failed to register sync-point label '" << name
                << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   } else {
      // The registration failed, which means we should know this sync-point
      // but we don't, so add the unknown sync-point to the unknown sync-point
      // list to track it.
      if ( !add_sync_point( label, UNKNOWN_SYNC_PNT_LIST ) ) {
         string name;
         StringUtilities::to_string( name, label );
         ostringstream errmsg;
         errmsg << "SyncPntManager::sync_point_registration_failed():" << __LINE__
                << " ERROR: Failed to add sync-point '" << name
                << "' to '" << UNKNOWN_SYNC_PNT_LIST << "' list!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}

// Callback from FedAmb.
void SyncPntManager::sync_point_announced(
   wstring const            &label,
   VariableLengthData const &user_supplied_tag )
{
   // Check to see if the synchronization point is known and is in the list.
   if ( contains_sync_point( label ) ) {

      // Mark sync-point as existing/announced.
      if ( mark_sync_point_announced( label ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            string name;
            StringUtilities::to_string( name, label );
            send_hs( stdout, "SyncPntManager::sync_point_announced():%d Synchronization point announced:'%s'%c",
                     __LINE__, name.c_str(), THLA_NEWLINE );
         }
      } else {
         string name;
         StringUtilities::to_string( name, label );
         ostringstream errmsg;
         errmsg << "SyncPntManager::sync_point_announced():" << __LINE__
                << " ERROR: Failed to mark sync-point '" << name
                << "' as announced." << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   } else {
      // By default, achieve unrecognized synchronization point.

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string name;
         StringUtilities::to_string( name, label );
         send_hs( stdout, "SyncPntManager::sync_point_announced():%d Unrecognized synchronization point:'%s', which will be achieved.%c",
                  __LINE__, name.c_str(), THLA_NEWLINE );
      }

      // Unknown synchronization point so achieve it but don't wait for the
      // federation to be synchronized on it.
      if ( !achieve_sync_point( label ) ) {
         string name;
         StringUtilities::to_string( name, label );
         send_hs( stderr, "SyncPntManager::sync_point_announced():%d Failed to achieve unrecognized synchronization point:'%s'.%c",
                  __LINE__, name.c_str(), THLA_NEWLINE );
      }
   }
}

// Callback from FedAmb.
void SyncPntManager::sync_point_federation_synchronized(
   wstring const &label )
{
   // Mark the sync-point as synchronized.
   if ( mark_sync_point_synchronized( label ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string name;
         StringUtilities::to_string( name, label );
         send_hs( stdout, "SyncPntManager::sync_point_announced():%d Synchronization point synchronized:'%s'%c",
                  __LINE__, name.c_str(), THLA_NEWLINE );
      }
   } else {
      // Sync-point should have been announced and at least managed in the
      // unknown sync-point list.
      string name;
      StringUtilities::to_string( name, label );
      ostringstream errmsg;
      errmsg << "SyncPntManager::sync_point_federation_synchronized():" << __LINE__
             << " ERROR: Unexpected unmanaged sync-point '" << name << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}
