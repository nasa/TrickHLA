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
@trick_link_dependency{SyncPointList.cpp}
@trick_link_dependency{SyncPointManagerBase.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{time/Int64Time.cpp}
@trick_link_dependency{utils/MutexLock.cpp}
@trick_link_dependency{utils/MutexProtection.cpp}
@trick_link_dependency{utils/SleepTimeout.cpp}
@trick_link_dependency{utils/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2024, --, Initial implementation.}
@revs_end

*/

// System includes.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/MemoryServices.hh"
#include "TrickHLA/SyncPoint.hh"
#include "TrickHLA/SyncPointList.hh"
#include "TrickHLA/SyncPointManagerBase.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/utils/MutexProtection.hh"
#include "TrickHLA/utils/SleepTimeout.hh"
#include "TrickHLA/utils/StringUtilities.hh"
#include "TrickHLA/utils/Utilities.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Enums.h"
#include "RTI/Exception.h"
#include "RTI/RTIambassador.h"
#include "RTI/Typedefs.h"
#include "RTI/VariableLengthData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPointManagerBase::SyncPointManagerBase()
   : mutex(),
     sync_pnt_lists(),
     federate( nullptr )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPointManagerBase::SyncPointManagerBase(
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

   if ( this->federate == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::setup():" << __LINE__
             << " ERROR: Unexpected nullptr federate pointer.\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }
}

void SyncPointManagerBase::clear()
{
   // Clear/remove everything from the list.
   while ( !sync_pnt_lists.empty() ) {
      MemoryServices::delete_var( sync_pnt_lists.back() );
      sync_pnt_lists.back() = nullptr;
      sync_pnt_lists.pop_back();
   }
   sync_pnt_lists.clear();
}

SyncPtStateEnum SyncPointManagerBase::get_sync_point_state(
   wstring const &label )
{
   SyncPoint const *sp = get_sync_point( label );
   return ( ( sp != nullptr ) ? sp->get_state() : TrickHLA::SYNC_PT_STATE_UNKNOWN );
}

bool SyncPointManagerBase::add_sync_point_list(
   string const &list_name )
{

   MutexProtection const auto_unlock_mutex( &mutex );

   // Create the named list only if it does not already exist.
   if ( !contains_sync_point_list_name( list_name ) ) {

      // Allocate a new sync point list and add it to the sync_pnt_lists.
      // FIXME: We need to use a named allocation to keep Trick STL checkpoint happy.
      size_t         cdims[]              = { 1 };
      string const   sync_point_list_name = string( "SyncPointList_" ) + list_name;
      SyncPointList *list                 = nullptr;
      list                                = MemoryServices::declare_var( list,
                                                                         "TrickHLA::SyncPointList",
                                                                         0,
                                                                         sync_point_list_name,
                                                                         1,
                                                                         cdims );

      if ( list == nullptr ) {
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::add_sync_point_list():" << __LINE__
                << " ERROR: Cannot allocate Trick Managed Memory for TrickHLA::SyncPointList named '"
                << list_name << "'!\n";
         DebugHandler::terminate( errmsg.str() );
         return false;
      }

      list->set_list_name( list_name );
      sync_pnt_lists.push_back( list );

      return true;
   }

   return false;
}

bool SyncPointManagerBase::add_sync_point(
   wstring const &label,
   string const  &list_name )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   // Check for the label being in any of the lists.
   if ( contains_sync_point( label ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
             << " ERROR: The sync-point label '" << label_str
             << "' has already been added!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   SyncPointList *sp_list = get_sync_point_list( list_name );
   if ( sp_list == nullptr ) {
      if ( !add_sync_point_list( list_name ) ) {
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
                << " ERROR: Could not add the named sync-point list for '"
                << list_name << "'!\n";
         DebugHandler::terminate( errmsg.str() );
         return false;
      }
      sp_list = get_sync_point_list( list_name );
   }

   if ( ( sp_list != nullptr ) && sp_list->add( label ) ) {
      return true;
   }

   string label_str;
   StringUtilities::to_string( label_str, label );
   ostringstream errmsg;
   errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
          << " ERROR: Could not add the sync-point label '"
          << label_str << "' to the named sync-point list '"
          << list_name << "'!\n";
   DebugHandler::terminate( errmsg.str() );
   return false;
}

bool SyncPointManagerBase::add_sync_point(
   wstring const   &label,
   string const    &list_name,
   Int64Time const &time )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   if ( contains_sync_point( label ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
             << " ERROR: The sync-point label '" << label_str
             << "' has already been added!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Add the named list if it does not exist.
   SyncPointList *sp_list = get_sync_point_list( list_name );
   if ( sp_list == nullptr ) {
      if ( !add_sync_point_list( list_name ) ) {
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
                << " ERROR: Could not add the named sync-point list for '"
                << list_name << "'!\n";
         DebugHandler::terminate( errmsg.str() );
         return false;
      }
      sp_list = get_sync_point_list( list_name );
   }

   // Add the sync-point label with time to the named list.
   if ( ( sp_list != nullptr ) && sp_list->add( label, time ) ) {
      return true;
   }

   string label_str;
   StringUtilities::to_string( label_str, label );
   ostringstream errmsg;
   errmsg << "SyncPointManagerBase::add_sync_point():" << __LINE__
          << " ERROR: Could not add the sync-point label '"
          << label_str << "' to the named sync-point list '"
          << list_name << "'!\n";
   DebugHandler::terminate( errmsg.str() );
   return false;
}

bool SyncPointManagerBase::contains_sync_point(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( &mutex );

#if defined( TRICKHLA_USE_STL_ALGORITHM )
   return ( std::any_of( sync_pnt_lists.begin(), sync_pnt_lists.end(),
                         [&label]( SyncPointList *sync_pnt_list ) -> bool {
                            return sync_pnt_list->contains( label );
                         } ) );
#else
   for ( SyncPointList *sync_pnt_list : sync_pnt_lists ) {
      if ( sync_pnt_list->contains( label ) ) {
         return true;
      }
   }
   return false;
#endif // TRICKHLA_USE_STL_ALGORITHM
}

/*
 * Does the names list contain the sync-point label.
 */
bool SyncPointManagerBase::contains_sync_point(
   wstring const     &label,
   std::string const &list_name )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPointList *sp_list = get_sync_point_list( list_name );
   return ( ( sp_list != nullptr ) && sp_list->contains( label ) );
}

bool SyncPointManagerBase::contains_sync_point_list_name(
   string const &list_name )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   return ( get_sync_point_list( list_name ) != nullptr );
}

bool SyncPointManagerBase::is_sync_point_list_empty(
   string const &list_name )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPointList *sp_list = get_sync_point_list( list_name );
   return ( ( sp_list == nullptr ) || sp_list->empty() );
}

bool SyncPointManagerBase::is_sync_point_registered(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint const *sp = get_sync_point( label );
   return ( ( sp != nullptr ) && sp->is_registered() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPointManagerBase::mark_sync_point_registered(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );
   if ( sp != nullptr ) {
      sp->mark_registered();
      return true;
   }
   return false;
}

bool SyncPointManagerBase::register_sync_point(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );

   // Unknown sync point if it is not contained in any list.
   if ( sp == nullptr ) {
      // Add the unknown sync-point to the unknown list so it can be registered.
      if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
                << " ERROR: Failed to add sync-point '" << label_str
                << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
         DebugHandler::terminate( errmsg.str() );
         return false;
      }
      sp = get_sync_point( label );
   }
   return register_sync_point( sp );
}

bool SyncPointManagerBase::register_sync_point(
   wstring const           &label,
   FederateHandleSet const &handle_set )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );

   // Unknown sync point if it is not contained in any list.
   if ( sp == nullptr ) {
      // Add the unknown sync-point to the unknown list so it can be registered.
      if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
                << " ERROR: Failed to add sync-point '" << label_str
                << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
         DebugHandler::terminate( errmsg.str() );
         return false;
      }
      sp = get_sync_point( label );
   }
   return register_sync_point( sp, handle_set );
}

bool SyncPointManagerBase::register_sync_point(
   SyncPoint *sp )
{
   if ( sp == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
             << " ERROR: Unexpected nullptr SyncPoint!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
             << " ERROR: Unexpected nullptr federate pointer!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *RTI_amb = federate->get_RTI_ambassador();

   // Register the sync-point label.
   bool registered = false;
   try {
      RTI_amb->registerFederationSynchronizationPoint( sp->get_label(),
                                                       sp->encode_user_supplied_tag() );
      // Mark the sync-point as registered.
      sp->mark_registered();

      registered = true;

   } catch ( RTI1516_NAMESPACE::Exception const &e ) {

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string label_str;
      StringUtilities::to_string( label_str, sp->get_label() );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
             << " ERROR: Failed to register '" << label_str
             << "' synchronization point with RTI!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return registered;
}

bool SyncPointManagerBase::register_sync_point(
   SyncPoint               *sp,
   FederateHandleSet const &handle_set )
{
   if ( sp == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
             << " ERROR: Unexpected nullptr SyncPoint!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
             << " ERROR: Unexpected nullptr federate pointer!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *RTI_amb = federate->get_RTI_ambassador();

   // Register the sync-point label.
   bool registered = false;
   try {
      RTI_amb->registerFederationSynchronizationPoint( sp->get_label(),
                                                       sp->encode_user_supplied_tag(),
                                                       handle_set );
      // Mark the sync-point as registered.
      sp->mark_registered();

      registered = true;

   } catch ( RTI1516_NAMESPACE::Exception const &e ) {

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string label_str;
      StringUtilities::to_string( label_str, sp->get_label() );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::register_sync_point():" << __LINE__
             << " ERROR: Failed to register '" << label_str
             << "' synchronization point with RTI!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return registered;
}

// True if at least one sync-point is registered.
bool SyncPointManagerBase::register_all_sync_points(
   string const &list_name )
{
   bool                  status = false;
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPointList const *sp_list = get_sync_point_list( list_name );

   if ( sp_list != nullptr ) {
      for ( SyncPoint *sp : sp_list->list ) {
         if ( register_sync_point( sp ) ) {
            status = true;
         }
      }
   }
   return status;
}

bool SyncPointManagerBase::register_all_sync_points(
   string const            &list_name,
   FederateHandleSet const &handle_set )
{
   bool                  status = false;
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPointList const *sp_list = get_sync_point_list( list_name );

   if ( sp_list != nullptr ) {
      for ( SyncPoint *sp : sp_list->list ) {
         if ( register_sync_point( sp, handle_set ) ) {
            status = true;
         }
      }
   }
   return status;
}

bool SyncPointManagerBase::is_sync_point_announced(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint const *sp = get_sync_point( label );
   return ( ( sp != nullptr ) && sp->is_announced() );
}

bool SyncPointManagerBase::mark_sync_point_announced(
   wstring const            &label,
   VariableLengthData const &user_supplied_tag )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );
   if ( sp != nullptr ) {
      sp->mark_announced( user_supplied_tag );
      return true;
   }
   return false;
}

bool SyncPointManagerBase::wait_for_sync_point_announced(
   wstring const &label )
{
   SyncPoint *sp;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection const auto_unlock_mutex( &mutex );

      sp = get_sync_point( label );

      // If the sync-point index is negative it is unknown.
      if ( sp == nullptr ) {
         // Add the unknown sync-point to the unknown list so it can be tracked.
         if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
            string label_str;
            StringUtilities::to_string( label_str, label );
            ostringstream errmsg;
            errmsg << "SyncPointManagerBase::wait_for_sync_point_announced():" << __LINE__
                   << " ERROR: Failed to add sync-point '" << label_str
                   << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
            DebugHandler::terminate( errmsg.str() );
            return false;
         }
         sp = get_sync_point( label );
      }
   }

   return wait_for_announced( sp );
}

bool SyncPointManagerBase::wait_for_announced(
   SyncPoint *sp )
{
   if ( sp == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::wait_for_announced():" << __LINE__
             << " ERROR: Unexpected nullptr SyncPoint!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::wait_for_announced():" << __LINE__
             << " ERROR: Unexpected nullptr federate pointer!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   bool announced = false;

   // Critical code section.
   {
      MutexProtection const auto_unlock_mutex( &mutex );

      announced = sp->is_announced();

      if ( !announced && !sp->is_valid() ) {
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::wait_for_announced():" << __LINE__
                << " ERROR: Bad sync-point state for sync-point!"
                << " Sync-point: " << sp->to_string() << "\n";
         DebugHandler::terminate( errmsg.str() );
         return false;
      }
   }

   bool         print_summary = DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE );
   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   // Wait for the sync-point to be announced.
   while ( !announced ) {

      if ( print_summary ) {
         print_summary = false;

         // Get the current sync-point status.
         ostringstream message;
         message << "SyncPointManagerBase::wait_for_announced():" << __LINE__
                 << " Sync-point: " << sp->to_string() << "\n";
         message_publish( MSG_NORMAL, message.str().c_str() );
      }

      // Always check to see is a shutdown was received.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep();

      // Critical code section.
      {
         MutexProtection const auto_unlock_mutex( &mutex );
         announced = sp->is_announced();
      }

      if ( !announced ) {

         // To be more efficient, we get the time once and share it.
         int64_t const wallclock_time = sleep_timer.time();

         // Check to make sure we're still a member of the federation execution.
         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SyncPointManagerBase::wait_for_announced():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution"
                      << " member. This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!\n";
               DebugHandler::terminate( errmsg.str() );
               return false;
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
      ostringstream message;
      message << "SyncPointManagerBase::wait_for_announced():" << __LINE__
              << " Sync-point announced: " << sp->to_string() << "\n";
      message_publish( MSG_NORMAL, message.str().c_str() );
   }

   return announced;
}

bool SyncPointManagerBase::wait_for_all_sync_points_announced(
   string const &list_name )
{
   // NOTE: Locking mutex while waiting can cause deadlock for FedAmb callbacks.
   bool status = false;

   SyncPointList const *sp_list = get_sync_point_list( list_name );

   if ( sp_list != nullptr ) {
      for ( SyncPoint *sp : sp_list->list ) {
         if ( wait_for_announced( sp ) ) {
            status = true;
         }
      }
   }
   return status;
}

bool SyncPointManagerBase::is_sync_point_achieved(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint const *sp = get_sync_point( label );
   return ( ( sp != nullptr ) && sp->is_achieved() );
}

bool SyncPointManagerBase::achieve_sync_point(
   wstring const &label )
{
   return achieve_sync_point( label, TrickHLA::EMPTY_USER_SUPPLIED_TAG );
}

bool SyncPointManagerBase::achieve_sync_point(
   wstring const            &label,
   VariableLengthData const &user_supplied_tag )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );

   // If the sync-point is nullptr it is unknown.
   if ( sp == nullptr ) {
      // Add the unknown sync-point to the Unknown list so it will be achieved.
      if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
                << " ERROR: Failed to add sync-point '" << label_str
                << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
         DebugHandler::terminate( errmsg.str() );
         return false;
      }
      sp = get_sync_point( label );
      if ( sp != nullptr ) {
         // Mark unknown sync-point as announced otherwise it will not be achieved.
         sp->mark_announced( user_supplied_tag );
      }
   } else if ( contains_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
      // Mark any sync-point already in the Unknown list as announced so that
      // it will be achieved.
      sp->mark_announced( user_supplied_tag );
   }

   return achieve_sync_point( sp );
}

bool SyncPointManagerBase::achieve_sync_point(
   SyncPoint *sp )
{
   if ( sp == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
             << " ERROR: Unexpected nullptr SyncPoint!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
             << " ERROR: Unexpected nullptr federate pointer!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
          << " Known Sync-point " << sp->to_string() << "\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   bool achieved = false;

   if ( sp->is_announced() ) {

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      RTIambassador *RTI_amb = federate->get_RTI_ambassador();

      try {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection const auto_unlock_mutex( &mutex );

         RTI_amb->synchronizationPointAchieved( sp->get_label() );

         // Mark the sync-point as achieved.
         sp->mark_achieved();

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
         ostringstream msg;
         msg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
             << " For Known Sync-point " << sp->to_string()
             << ", Not Connected to RTI!\n";
         message_publish( MSG_WARNING, msg.str().c_str() );
         if ( federate != nullptr ) {
            federate->set_connection_lost();
         }
      } catch ( RTIinternalError const &e ) {
         // Keep sync-point state the same, and return false.
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   } else if ( sp->is_achieved() ) {

      // If the synchronization point is already achieved then print out
      // a message.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
                << " Sync-point '" << label_str
                << "' has already been achieved with the RTI!\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }

      achieved = true;

   } else if ( sp->is_synchronized() ) {
      // If the synchronization point is already synchronized, then print
      // out a message and return.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
                << " Sync-point '" << label_str
                << "' has already been synchronized with the RTI!\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }

      achieved = true;

   } else if ( sp->is_registered() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
                << " WARNING: Sync-point '" << label_str
                << "' is registered but has not been announced by the RTI!\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   } else if ( sp->is_known() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
                << " WARNING: Sync-point '" << label_str
                << "' is known but has not been registered or announced!\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   } else {
      // Sync-point is unknown.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::achieve_sync_point():" << __LINE__
                << " WARNING: Sync-point '" << label_str
                << "' is unknown!\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }

   return achieved;
}

bool SyncPointManagerBase::achieve_all_sync_points(
   string const &list_name )
{
   bool status = false;

   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPointList const *sp_list = get_sync_point_list( list_name );

   if ( sp_list != nullptr ) {
      for ( SyncPoint *sp : sp_list->list ) {
         if ( achieve_sync_point( sp ) ) {
            status = true;
         }
      }
   }
   return status;
}

bool SyncPointManagerBase::is_sync_point_synchronized(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint const *sp = get_sync_point( label );
   return ( ( sp != nullptr ) && sp->is_synchronized() );
}

bool SyncPointManagerBase::is_all_sync_points_synchronized(
   std::string const &list_name )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPointList *sp_list = get_sync_point_list( list_name );
   return ( ( sp_list != nullptr ) && sp_list->is_all_synchronized() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPointManagerBase::mark_sync_point_synchronized(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );
   if ( sp != nullptr ) {
      sp->mark_synchronized();
      return true;
   }
   return false;
}

bool SyncPointManagerBase::wait_for_sync_point_synchronized(
   wstring const &label )
{
   SyncPoint const *sp;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection const auto_unlock_mutex( &mutex );

      sp = get_sync_point( label );

      // If the sync-point is nullptr then it is unknown.
      if ( sp == nullptr ) {
         // Add the unknown sync-point to the unknown list so it can be tracked.
         if ( !add_sync_point( label, TrickHLA::UNKNOWN_SYNC_POINT_LIST ) ) {
            string label_str;
            StringUtilities::to_string( label_str, label );
            ostringstream errmsg;
            errmsg << "SyncPointManagerBase::wait_for_sync_point_synchronized():" << __LINE__
                   << " ERROR: Failed to add sync-point '" << label_str
                   << "' to '" << TrickHLA::UNKNOWN_SYNC_POINT_LIST << "' list!\n";
            DebugHandler::terminate( errmsg.str() );
            return false;
         }
         sp = get_sync_point( label );
      }
   }

   return wait_for_synchronized( sp );
}

bool SyncPointManagerBase::wait_for_all_sync_points_synchronized(
   string const &list_name )
{
   // NOTE: Locking mutex while waiting can cause deadlock for FedAmb callbacks.
   bool status = false;

   SyncPointList const *sp_list = get_sync_point_list( list_name );

   // First check to insure that the list exists.
   if ( sp_list != nullptr ) {
      // The list exists; so, wait for all the sync points to synchronize.
      for ( SyncPoint const *sp : sp_list->list ) {
         if ( wait_for_synchronized( sp ) ) {
            status = true;
         }
      }
   }
   return status;
}

bool SyncPointManagerBase::wait_for_synchronized(
   SyncPoint const *sp )
{
   if ( sp == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::wait_for_synchronized():" << __LINE__
             << " ERROR: Unexpected nullptr SyncPoint!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::wait_for_synchronized():" << __LINE__
             << " ERROR: Unexpected nullptr federate pointer!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   bool         print_summary = DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE );
   bool         synchronized;
   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   // Wait for the federation to synchronize on the sync-point.
   do {
      if ( print_summary ) {
         print_summary = false;
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream msg;
         msg << "SyncPointManagerBase::wait_for_synchronized():" << __LINE__
             << " Sync-point '" << label_str << "'\n";
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      // Critical code section.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks
         // the mutex even if there is an exception.
         MutexProtection const auto_unlock_mutex( &mutex );
         synchronized = sp->is_synchronized();
      }

      if ( !synchronized ) {

         // Always check to see if a shutdown was received.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         // To be more efficient, we get the time once and share it.
         int64_t const wallclock_time = sleep_timer.time();

         // Check to make sure we're still a member of the federation execution.
         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SyncPointManagerBase::wait_for_synchronized():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution"
                      << " member. This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!\n";
               DebugHandler::terminate( errmsg.str() );
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

bool SyncPointManagerBase::achieve_sync_point_and_wait_for_synchronization(
   wstring const &label )
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
         DebugHandler::terminate( errmsg.str() );
         return false;
      }
   } else {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointManagerBase::achieve_sync_point_and_wait_for_synchronization():" << __LINE__
             << " ERROR: Failed to achieve sync-point '" << label_str << "'\n";
      DebugHandler::terminate( errmsg.str() );
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
      MutexProtection const auto_unlock_mutex( &mutex );

      msg << "SyncPointManagerBase::to_string():" << __LINE__
          << " Number of Sync-Point Lists:" << sync_pnt_lists.size() << "\n";

      for ( SyncPointList *sync_pnt_list : sync_pnt_lists ) {
         msg << sync_pnt_list->to_string();
      }
   }
   return msg.str();
}

string SyncPointManagerBase::to_string(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );
   if ( sp != nullptr ) {
      return sp->to_string();
   }

   string label_str;
   StringUtilities::to_string( label_str, label );
   ostringstream msg;
   msg << "SyncPointManagerBase::to_string():" << __LINE__
       << " Unknown sync-point label:'" << label_str << "'\n";
   return msg.str();
}

string SyncPointManagerBase::to_string(
   string const &list_name )
{
   MutexProtection const auto_unlock_mutex( &mutex );

   SyncPointList *sp_list = get_sync_point_list( list_name );
   if ( sp_list != nullptr ) {
      return sp_list->to_string();
   }

   return "SyncPointManagerBase::to_string():" + std::to_string( __LINE__ )
          + " Unknown list name '" + list_name + "'\n";
}

void SyncPointManagerBase::print_sync_points()
{
   ostringstream msg;
   msg << "SyncPointManagerBase::print_sync_points():" << __LINE__ << "\n"
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
      DebugHandler::terminate( errmsg.str() );
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
            DebugHandler::terminate( errmsg.str() );
         }
      } else {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointManagerBase::sync_point_registration_failed():" << __LINE__
                << " ERROR: Failed to register sync-point label '" << label_str
                << "'\n";
         DebugHandler::terminate( errmsg.str() );
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
         DebugHandler::terminate( errmsg.str() );
      }
   }
}

// Callback from FedAmb.
void SyncPointManagerBase::sync_point_announced(
   wstring const            &label,
   VariableLengthData const &user_supplied_tag )
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
         DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
   }
}

SyncPoint *SyncPointManagerBase::get_sync_point(
   wstring const &label )
{
   for ( SyncPointList *sp_list : sync_pnt_lists ) {
      if ( sp_list != nullptr ) {
         SyncPoint *sp = sp_list->get( label );
         if ( sp != nullptr ) {
            return sp;
         }
      }
   }
   return nullptr;
}

SyncPointList *SyncPointManagerBase::get_sync_point_list(
   string const &list_name )
{
   for ( SyncPointList *sp_list : sync_pnt_lists ) {
      if ( ( sp_list != nullptr ) && list_name.compare( sp_list->get_list_name() ) == 0 ) {
         return sp_list;
      }
   }
   return nullptr;
}

//-------------------------------------------------------------------------
// CheckpointConversionBase Interface.
//-------------------------------------------------------------------------

/*! @brief Encode the variables to a form Trick can checkpoint. */
void SyncPointManagerBase::convert_data_before_checkpoint()
{
   for ( SyncPointList *sync_pnt_list : sync_pnt_lists ) {
      sync_pnt_list->convert_data_before_checkpoint();
   }
}

/*! @brief Decode the state of this class from the Trick checkpoint. */
void SyncPointManagerBase::restore_data_after_checkpoint()
{
   for ( SyncPointList *sync_pnt_list : sync_pnt_lists ) {
      sync_pnt_list->restore_data_after_checkpoint();
   }
}

/*! @brief Free/release the memory used for the checkpoint data structures. */
void SyncPointManagerBase::free_converted_data_for_checkpoint()
{
   for ( SyncPointList *sync_pnt_list : sync_pnt_lists ) {
      sync_pnt_list->free_converted_data_for_checkpoint();
   }
}

/*!
 *  @job_class{restart}
 */
void SyncPointManagerBase::checkpoint_restart()
{
   return;
}
