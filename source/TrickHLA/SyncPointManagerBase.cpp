/*!
@file TrickHLA/SyncPointManagerBase.cpp
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
@trick_link_dependency{SyncPointList.cpp}
@trick_link_dependency{SyncPointManagerBase.cpp}
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
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/release.h"

// HLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPointList.hh"
#include "TrickHLA/SyncPointManagerBase.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPointManagerBase::SyncPointManagerBase()
   : mutex(),
#if SYNC_POINT_LIST_TMM_ARRAY
     sync_pnt_lists( NULL ),
     sync_pnt_lists_count( 0 ),
#else
     sync_pnt_lists(),
#endif
     federate( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPointManagerBase::SyncPointManagerBase(
   Federate *fed )
   : mutex(),
#if SYNC_POINT_LIST_TMM_ARRAY
     sync_pnt_lists( NULL ),
     sync_pnt_lists_count( 0 ),
#else
     sync_pnt_lists(),
#endif
     federate( fed )
{
   return;
}

/*!
 * @details This is a pure virtual destructor.
 * @job_class{shutdown}
 */
SyncPointManagerBase::~SyncPointManagerBase()
{
   // Clear/remove everything from the lists.
   clear();

   // Make sure we destroy the mutex.
   mutex.destroy();
}

void SyncPointManagerBase::setup(
   Federate *fed )
{
   this->federate = fed;

   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::setup():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

#if SYNC_POINT_LIST_TMM_ARRAY
   for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
#else
   for ( int index = 0; index < sync_pnt_lists.size(); ++index ) {
#endif
      sync_pnt_lists[index]->setup( this->federate );
   }
}

void SyncPointManagerBase::clear()
{
#if SYNC_POINT_LIST_TMM_ARRAY
   if ( sync_pnt_lists != NULL ) {
      for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
         if ( sync_pnt_lists[index] != NULL ) {
            sync_pnt_lists[index]->clear();
            TMM_delete_var_a( sync_pnt_lists[index] );
            sync_pnt_lists[index] = NULL;
         }
      }
      TMM_delete_var_a( sync_pnt_lists );
      sync_pnt_lists_count = 0;
      sync_pnt_lists       = NULL;
   }
#else
   // Clear/remove everything from the list.
   while ( !sync_pnt_lists.empty() ) {
      if ( *sync_pnt_lists.begin() != NULL ) {
         delete ( *sync_pnt_lists.begin() );
         sync_pnt_lists.erase( sync_pnt_lists.begin() );
      }
   }
   sync_pnt_lists.clear();
#endif
}

int const SyncPointManagerBase::get_list_index_for_sync_point(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

#if SYNC_POINT_LIST_TMM_ARRAY
   for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
#else
   for ( int index = 0; index < sync_pnt_lists.size(); ++index ) {
#endif
      if ( sync_pnt_lists[index]->contains( label ) ) {
         return index;
      }
   }
   return -1;
}

int const SyncPointManagerBase::get_list_index_for_list_name(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

#if SYNC_POINT_LIST_TMM_ARRAY
   for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
#else
   for ( int index = 0; index < sync_pnt_lists.size(); ++index ) {
#endif
      if ( list_name.compare( sync_pnt_lists[index]->get_list_name() ) == 0 ) {
         return index;
      }
   }
   return -1;
}

SyncPtStateEnum const SyncPointManagerBase::get_sync_point_state(
   std::wstring const &label )
{
#if SYNC_POINT_LIST_TMM_ARRAY
   for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
#else
   for ( int index = 0; index < sync_pnt_lists.size(); ++index ) {
#endif
      if ( sync_pnt_lists[index]->contains( label ) ) {
         return sync_pnt_lists[index]->get_state( label );
      }
   }
   return TrickHLA::SYNC_PT_STATE_UNKNOWN;
}

bool const SyncPointManagerBase::add_sync_point_list(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   // Create the named list only if it does not already exist.
   if ( !contains_sync_point_list_name( list_name ) ) {

#if SYNC_POINT_LIST_TMM_ARRAY
      // Use a Trick memory managed allocation so we can checkpoint it.
      if ( sync_pnt_lists == NULL ) {
         // Allocate the list.
         sync_pnt_lists = static_cast< SyncPointList ** >( TMM_declare_var_1d( "TrickHLA::SyncPointList *", 1 ) );
      } else {
         // Resize the list.
         sync_pnt_lists = static_cast< SyncPointList ** >( TMM_resize_array_1d_a( sync_pnt_lists, ( sync_pnt_lists_count + 1 ) ) );
      }
      if ( sync_pnt_lists == NULL ) {
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::add_sync_point_list():" << __LINE__
                << " ERROR: Could not allocate memory for the sync-point lists!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }

      // Allocate the new Sync-Point List and add it to the end of the array.
      sync_pnt_lists[sync_pnt_lists_count] = static_cast< SyncPointList * >( TMM_declare_var_1d( "TrickHLA::SyncPointList", 1 ) );
      if ( sync_pnt_lists[sync_pnt_lists_count] == NULL ) {
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::add_sync_point_list():" << __LINE__
                << " ERROR: Could not allocate memory for the sync-point list at array index:"
                << sync_pnt_lists_count << " for sync-point list '"
                << list_name << "'!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }
      sync_pnt_lists[sync_pnt_lists_count]->set_list_name( list_name );
      sync_pnt_lists[sync_pnt_lists_count]->set_mutex( this->mutex );
      sync_pnt_lists[sync_pnt_lists_count]->set_federate( this->federate );

      // Update the count to match the successful new array allocation.
      ++sync_pnt_lists_count;

#else
      sync_pnt_lists.push_back( new SyncPointList( list_name, this->mutex, this->federate ) );
#endif

      return true;
   }
   return false;
}

bool const SyncPointManagerBase::add_sync_point(
   wstring const &label,
   string const  &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   if ( contains_sync_point( label ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
             << " ERROR: The sync-point label '" << label_str
             << "' has already been added!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Add the named list if it does not exist.
   int index = get_list_index_for_list_name( list_name );
   if ( index < 0 ) {
      if ( !add_sync_point_list( list_name ) ) {
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
                << " ERROR: Could not add the named sync-point list for '"
                << list_name << "'!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }
      index = get_list_index_for_list_name( list_name );
   }

   // Add the sync-point label to the named list.
   if ( ( index >= 0 ) && sync_pnt_lists[index]->add( label ) ) {
      return true;
   }

   string label_str;
   StringUtilities::to_string( label_str, label );
   ostringstream errmsg;
   errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
          << " ERROR: Could not add the sync-point label '"
          << label_str << "' to the named sync-point list '"
          << list_name << "'!\n";
   DebugHandler::terminate_with_message( errmsg.str() );
   return false;
}

bool const SyncPointManagerBase::add_sync_point(
   wstring const   &label,
   string const    &list_name,
   Int64Time const &time )
{
   MutexProtection auto_unlock_mutex( &mutex );

   if ( contains_sync_point( label ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
             << " ERROR: The sync-point label '" << label_str
             << "' has already been added!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Add the named list if it does not exist.
   int index = get_list_index_for_list_name( list_name );
   if ( index < 0 ) {
      if ( !add_sync_point_list( list_name ) ) {
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
                << " ERROR: Could not add the named sync-point list for '"
                << list_name << "'!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }
      index = get_list_index_for_list_name( list_name );
   }

   // Add the sync-point label with time to the named list.
   if ( ( index >= 0 ) && sync_pnt_lists[index]->add( label, time ) ) {
      return true;
   }

   string label_str;
   StringUtilities::to_string( label_str, label );
   ostringstream errmsg;
   errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
          << " ERROR: Could not add the sync-point label '"
          << label_str << "' to the named sync-point list '"
          << list_name << "'!\n";
   DebugHandler::terminate_with_message( errmsg.str() );
   return false;
}

bool const SyncPointManagerBase::contains_sync_point(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

#if SYNC_POINT_LIST_TMM_ARRAY
   for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
#else
   for ( int index = 0; index < sync_pnt_lists.size(); ++index ) {
#endif
      if ( sync_pnt_lists[index]->contains( label ) ) {
         return true;
      }
   }
   return false;
}

/*
 * Does the names list contain the sync-point label.
 */
bool const SyncPointManagerBase::contains_sync_point(
   wstring const     &label,
   std::string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_list_name( list_name );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->contains( label ) );
}

bool const SyncPointManagerBase::contains_sync_point_list_name(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   return ( get_list_index_for_list_name( list_name ) >= 0 );
}

bool const SyncPointManagerBase::is_sync_point_registered(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_sync_point( label );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->is_registered( label ) );
}

/*!
 * @job_class{initialization}
 */
bool const SyncPointManagerBase::mark_sync_point_registered(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_sync_point( label );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->mark_registered( label ) );
}

bool const SyncPointManagerBase::register_sync_point(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int index = get_list_index_for_sync_point( label );

   // Unknown sync point if it is not contained in any list.
   if ( index < 0 ) {
      // Add the unknown sync-point to the unknown list so it can be registered.
      if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
                << " ERROR: Failed to add sync-point '" << label_str
                << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }
      index = get_list_index_for_sync_point( label );
   }

   return ( ( index >= 0 ) && sync_pnt_lists[index]->register_sync_point( label ) );
}

bool const SyncPointManagerBase::register_sync_point(
   wstring const           &label,
   FederateHandleSet const &handle_set )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int index = get_list_index_for_sync_point( label );

   // Unknown sync point if it is not contained in any list.
   if ( index < 0 ) {
      // Add the unknown sync-point to the unknown list so it can be registered.
      if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
                << " ERROR: Failed to add sync-point '" << label_str
                << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }
      index = get_list_index_for_sync_point( label );
   }

   return ( ( index >= 0 ) && sync_pnt_lists[index]->register_sync_point( label, handle_set ) );
}

// True if at least one sync-point is registered.
bool const SyncPointManagerBase::register_all_sync_points(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_list_name( list_name );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->register_all() );
}

bool const SyncPointManagerBase::register_all_sync_points(
   string const            &list_name,
   FederateHandleSet const &handle_set )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_list_name( list_name );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->register_all( handle_set ) );
}

bool const SyncPointManagerBase::is_sync_point_announced(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_sync_point( label );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->is_announced( label ) );
}

bool const SyncPointManagerBase::mark_sync_point_announced(
   wstring const          &label,
   RTI1516_USERDATA const &user_supplied_tag )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_sync_point( label );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->mark_announced( label, user_supplied_tag ) );
}

bool const SyncPointManagerBase::wait_for_sync_point_announced(
   wstring const &label )
{
   int index;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection auto_unlock_mutex( &mutex );

      index = get_list_index_for_sync_point( label );

      // If the sync-point index is negative it is unknown.
      if ( index < 0 ) {
         // Add the unknown sync-point to the unknown list so it can be tracked.
         if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
            string label_str;
            StringUtilities::to_string( label_str, label );
            ostringstream errmsg;
            errmsg << "SyncPointManagerBase::wait_for_sync_point_announced():" << __LINE__
                   << " ERROR: Failed to add sync-point '" << label_str
                   << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }
         index = get_list_index_for_sync_point( label );
      }
   }

   return ( ( index >= 0 ) && sync_pnt_lists[index]->wait_for_announced( label ) );
}

bool const SyncPointManagerBase::wait_for_all_sync_points_announced(
   string const &list_name )
{
   // NOTE: Locking the mutex while waiting can cause deadlock for callbacks.

   int const index = get_list_index_for_list_name( list_name );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->wait_for_all_announced() );
}

bool const SyncPointManagerBase::is_sync_point_achieved(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_sync_point( label );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->is_achieved( label ) );
}

bool const SyncPointManagerBase::achieve_sync_point(
   wstring const &label )
{
   return achieve_sync_point( label, RTI1516_USERDATA( 0, 0 ) );
}

bool const SyncPointManagerBase::achieve_sync_point(
   std::wstring const     &label,
   RTI1516_USERDATA const &user_supplied_tag )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int index = get_list_index_for_sync_point( label );

   // If the sync-point list index is negative it is unknown.
   if ( index < 0 ) {
      // Add the unknown sync-point to the Unknown list so it will be achieved.
      if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
                << " ERROR: Failed to add sync-point '" << label_str
                << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }
      index = get_list_index_for_sync_point( label );
      if ( index >= 0 ) {
         // Mark unknown sync-point as announced otherwise it will not be achieved.
         sync_pnt_lists[index]->mark_announced( label, user_supplied_tag );
      }
   } else if ( TrickHLA::UNKNOWN_SYNC_POINT_LIST.compare( sync_pnt_lists[index]->get_list_name() ) ) {
      // Mark any sync-point already in the Unknown list as announced so that
      // it will be achieved.
      sync_pnt_lists[index]->mark_announced( label, user_supplied_tag );
   }

   return ( ( index >= 0 ) && sync_pnt_lists[index]->achieve( label ) );
}

bool const SyncPointManagerBase::achieve_all_sync_points(
   string const &list_name )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_list_name( list_name );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->achieve_all() );
}

bool const SyncPointManagerBase::is_sync_point_synchronized(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_sync_point( label );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->is_synchronized( label ) );
}

/*!
 * @job_class{initialization}
 */
bool const SyncPointManagerBase::mark_sync_point_synchronized(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_sync_point( label );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->mark_synchronized( label ) );
}

bool const SyncPointManagerBase::wait_for_sync_point_synchronized(
   wstring const &label )
{
   int index;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection auto_unlock_mutex( &mutex );

      index = get_list_index_for_sync_point( label );

      // If the sync-point is null then it is unknown.
      if ( index < 0 ) {
         // Add the unknown sync-point to the unknown list so it can be tracked.
         if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
            string label_str;
            StringUtilities::to_string( label_str, label );
            ostringstream errmsg;
            errmsg << "SyncPointManagerBase::wait_for_sync_point_synchronized():" << __LINE__
                   << " ERROR: Failed to add sync-point '" << label_str
                   << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }
         index = get_list_index_for_sync_point( label );
      }
   }

   return ( ( index >= 0 ) && sync_pnt_lists[index]->wait_for_synchronized( label ) );
}

bool const SyncPointManagerBase::wait_for_all_sync_points_synchronized(
   string const &list_name )
{
   // NOTE: Locking the mutex while waiting can cause deadlock for callbacks.

   int const index = get_list_index_for_list_name( list_name );
   return ( ( index >= 0 ) && sync_pnt_lists[index]->wait_for_all_synchronized() );
}

bool const SyncPointManagerBase::achieve_sync_point_and_wait_for_synchronization(
   std::wstring const &label )
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      message_publish( MSG_NORMAL, "SyncPointManagerBase::achieve_sync_point_and_wait_for_synchronization():%d Label:'%s'\n",
                       __LINE__, label_str.c_str() );
   }

   if ( achieve_sync_point( label ) ) {
      if ( !wait_for_sync_point_synchronized( label ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::achieve_sync_point_and_wait_for_synchronization():" << __LINE__
                << " ERROR: Failed to wait for sync-point '" << label_str << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }
   } else {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::achieve_sync_point_and_wait_for_synchronization():" << __LINE__
             << " ERROR: Failed to achieve sync-point '" << label_str << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      print_sync_points();
   }
   return true;
}

string SyncPointManagerBase::to_string()
{
   ostringstream msg;

   // Critical code section.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

#if SYNC_POINT_LIST_TMM_ARRAY
      msg << "SyncPointManagerBase::to_string():" << __LINE__
          << " Number of Sync-Point Lists:" << sync_pnt_lists_count << '\n';

      for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
#else
      msg << "SyncPointManagerBase::to_string():" << __LINE__
          << " Number of Sync-Point Lists:" << sync_pnt_lists.size() << endl;

      for ( int index = 0; index < sync_pnt_lists.size(); ++index ) {
#endif
         msg << sync_pnt_lists[index]->to_string();
      }
   }
   return msg.str();
}

string SyncPointManagerBase::to_string(
   std::wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   int const index = get_list_index_for_sync_point( label );
   if ( index >= 0 ) {
      return sync_pnt_lists[index]->to_string( label );
   }

   string label_str;
   StringUtilities::to_string( label_str, label );
   ostringstream msg;
   msg << "SyncPointManagerBase::to_string():" << __LINE__
       << " Unknown sync-point label:'" << label_str << "'\n";
   return msg.str();
}

void SyncPointManagerBase::print_sync_points()
{
   ostringstream msg;
   msg << "SyncPointManagerBase::print_sync_points():" << __LINE__ << '\n'
       << to_string();
   message_publish( MSG_NORMAL, msg.str().c_str() );
}

// Callback from FedAmb.
void SyncPointManagerBase::sync_point_registration_succeeded(
   wstring const &label )
{
   if ( mark_sync_point_registered( label ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         message_publish( MSG_NORMAL, "SyncPointManagerBase::sync_point_registration_succeeded():%d Label:'%s'\n",
                          __LINE__, label_str.c_str() );
      }
   } else {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::sync_point_registration_succeeded():" << __LINE__
             << " ERROR: Failed to mark sync-point '" << label_str
             << "' as registered!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

// Callback from FedAmb.
void SyncPointManagerBase::sync_point_registration_failed(
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
               string label_str;
               StringUtilities::to_string( label_str, label );
               message_publish( MSG_NORMAL, "SyncPointManagerBase::sync_point_registration_failed():%d Label:'%s' already exists.\n",
                                __LINE__, label_str.c_str() );
            }
         } else {
            string label_str;
            StringUtilities::to_string( label_str, label );
            ostringstream errmsg;
            errmsg << "SyncPointManagerBase::sync_point_registration_failed():" << __LINE__
                   << " ERROR: Failed to mark sync-point '" << label_str
                   << "' as registered.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      } else {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::sync_point_registration_failed():" << __LINE__
                << " ERROR: Failed to register sync-point label '" << label_str
                << "'\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   } else {
      // The registration failed, which means we should know this sync-point
      // but we don't, so add the unknown sync-point to the unknown sync-point
      // list to track it.
      if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::sync_point_registration_failed():" << __LINE__
                << " ERROR: Failed to add sync-point '" << label_str
                << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}

// Callback from FedAmb.
void SyncPointManagerBase::sync_point_announced(
   wstring const          &label,
   RTI1516_USERDATA const &user_supplied_tag )
{
   // Unrecognized sync-point label if not seen before or if it is in the
   // Unknown list (i.e. seen before but still unrecognized).
   if ( !contains_sync_point( label ) || contains_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {

      // Unrecognized sync-point. Achieve all unrecognized sync-points.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONTROL ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         message_publish( MSG_NORMAL, "SyncPointManagerBase::sync_point_announced():%d Unrecognized sync-point:'%s', which will be achieved.\n",
                          __LINE__, label_str.c_str() );
      }

      // Achieve all Unrecognized sync-points but don't wait for the
      // federation to be synchronized on it.
      if ( !achieve_sync_point( label, user_supplied_tag ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         message_publish( MSG_WARNING, "SyncPointManagerBase::sync_point_announced():%d Failed to achieve unrecognized sync-point:'%s'.\n",
                          __LINE__, label_str.c_str() );
      }
   } else {
      // Known sync-point that is already in one of the sync-point lists.

      // Mark known sync-point as announced.
      if ( mark_sync_point_announced( label, user_supplied_tag ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            string label_str;
            StringUtilities::to_string( label_str, label );
            message_publish( MSG_NORMAL, "SyncPointManagerBase::sync_point_announced():%d Marked sync-point announced:'%s'\n",
                             __LINE__, label_str.c_str() );
         }
      } else {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::sync_point_announced():" << __LINE__
                << " ERROR: Failed to mark sync-point '" << label_str
                << "' as announced.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}

// Callback from FedAmb.
void SyncPointManagerBase::sync_point_federation_synchronized(
   wstring const &label )
{
   // Mark the sync-point as synchronized.
   if ( mark_sync_point_synchronized( label ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         message_publish( MSG_NORMAL, "SyncPointManagerBase::sync_point_federation_synchronized():%d Sync-point synchronized:'%s'\n",
                          __LINE__, label_str.c_str() );
      }
   } else {
      // Sync-point should have been announced and at least managed in the
      // unknown sync-point list.
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::sync_point_federation_synchronized():" << __LINE__
             << " ERROR: Unexpected unmanaged sync-point '" << label_str << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*! @brief Encode the variables to a form Trick can checkpoint. */
void SyncPointManagerBase::encode_checkpoint()
{
   free_checkpoint();

#if SYNC_POINT_LIST_TMM_ARRAY
   for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
#else
   for ( int index = 0; index < sync_pnt_lists.size(); ++index ) {
#endif
      sync_pnt_lists[index]->encode_checkpoint();
   }
}

/*! @brief Decode the state of this class from the Trick checkpoint. */
void SyncPointManagerBase::decode_checkpoint()
{
#if SYNC_POINT_LIST_TMM_ARRAY
   for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
#else
   for ( int index = 0; index < sync_pnt_lists.size(); ++index ) {
#endif
      sync_pnt_lists[index]->decode_checkpoint();
   }
}

/*! @brief Free/release the memory used for the checkpoint data structures. */
void SyncPointManagerBase::free_checkpoint()
{
#if SYNC_POINT_LIST_TMM_ARRAY
   for ( int index = 0; index < sync_pnt_lists_count; ++index ) {
#else
   for ( int index = 0; index < sync_pnt_lists.size(); ++index ) {
#endif
      sync_pnt_lists[index]->free_checkpoint();
   }
}
