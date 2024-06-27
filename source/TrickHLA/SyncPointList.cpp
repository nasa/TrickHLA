/*!
@file TrickHLA/SyncPointList.cpp
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
@trick_link_dependency{SyncPoint.cpp}
@trick_link_dependency{SyncPointList.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2024, --, Initial implementation.}
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
#include "TrickHLA/SyncPoint.hh"
#include "TrickHLA/SyncPointList.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPointList::SyncPointList(
   std::string const &name,
   MutexLock         &mtx,
   Federate          *fed )
   : list(),
     list_name( name ),
     mutex( mtx ),
     federate( fed )
{
   return;
}

/*!
 * @details This is a pure virtual destructor.
 * @job_class{shutdown}
 */
SyncPointList::~SyncPointList()
{
   clear();
}

string &SyncPointList::get_list_name()
{
   return this->list_name;
}

SyncPtStateEnum const SyncPointList::get_state(
   std::wstring const &label )
{
   MutexProtection  auto_unlock_mutex( &mutex );
   SyncPoint const *sp = get_sync_point( label );
   return ( sp != NULL ) ? sp->get_state() : TrickHLA::SYNC_PT_STATE_UNKNOWN;
}

void SyncPointList::clear()
{
   // Clear/remove everything from the list.
   while ( !list.empty() ) {
      if ( *list.begin() != NULL ) {
         delete ( *list.begin() );
         list.erase( list.begin() );
      }
   }
   list.clear();
}

void SyncPointList::setup(
   Federate *fed )
{
   MutexProtection auto_unlock_mutex( &mutex );
   this->federate = fed;
}

SyncPoint *SyncPointList::get_sync_point(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   for ( int i = 0; i < list.size(); ++i ) {
      if ( label.compare( list[i]->get_label() ) == 0 ) {
         return list[i];
      }
   }
   return NULL;
}

bool const SyncPointList::add(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   if ( contains( label ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointList::add_sync_point():" << __LINE__
             << " ERROR: The sync-point label '" << label_str
             << "' has already been added!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Add the sync-point to the corresponding named list.
   list.push_back( new SyncPoint( label ) );
   return true;
}

bool const SyncPointList::add(
   wstring const &label,
   Int64Time      time )
{
   /*
   if ( time < 0.0 ) {
      // No time specified
      execution_control->register_sync_point( label );
   } else {
      // DANNY2.7 convert time and encode in a buffer to send to RTI
      int64_t       _value = Int64BaseTime::to_base_time( time );
      unsigned char buf[8];
      buf[0] = (unsigned char)( ( _value >> 56 ) & 0xFF );
      buf[1] = (unsigned char)( ( _value >> 48 ) & 0xFF );
      buf[2] = (unsigned char)( ( _value >> 40 ) & 0xFF );
      buf[3] = (unsigned char)( ( _value >> 32 ) & 0xFF );
      buf[4] = (unsigned char)( ( _value >> 24 ) & 0xFF );
      buf[5] = (unsigned char)( ( _value >> 16 ) & 0xFF );
      buf[6] = (unsigned char)( ( _value >> 8 ) & 0xFF );
      buf[7] = (unsigned char)( ( _value >> 0 ) & 0xFF );
      //TODO: RTI_ambassador->registerFederationSynchronizationPoint( label, RTI1516_USERDATA( buf, 8 ) );

      execution_control->register_sync_point( label, time );
   }
   */

   // TODO: Add as a timed sync point.
   return add( label );
}

bool const SyncPointList::contains(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   for ( int i = 0; i < list.size(); ++i ) {
      if ( label.compare( list[i]->get_label() ) == 0 ) {
         return true;
      }
   }
   return false;
}

bool const SyncPointList::is_registered(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint const *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_registered() );
}

/*!
 * @job_class{initialization}
 */
bool const SyncPointList::mark_registered(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );
   if ( sp != NULL ) {
      sp->set_state( SYNC_PT_STATE_REGISTERED );
      return true;
   }
   return false;
}

bool const SyncPointList::register_sync_point(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );

   // If the sync-point is null then it is unknown.
   if ( sp == NULL ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Did not find sync-point '" << label_str
             << "' in the '" << get_list_name() << "' list!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   return register_sync_point( sp );
}

bool const SyncPointList::register_sync_point(
   wstring const           &label,
   FederateHandleSet const &handle_set )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );

   // If the sync-point is null then it is unknown.
   if ( sp == NULL ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Did not find sync-point '" << label_str
             << "' in the '" << get_list_name() << "' list!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   return register_sync_point( sp, handle_set );
}

// True if at least one sync-point is registered.
bool const SyncPointList::register_all()
{
   MutexProtection auto_unlock_mutex( &mutex );

   bool status = false;
   for ( int i = 0; i < list.size(); ++i ) {
      if ( register_sync_point( list[i] ) ) {
         status = true;
      }
   }
   return status;
}

bool const SyncPointList::register_all(
   FederateHandleSet const &handle_set )
{
   MutexProtection auto_unlock_mutex( &mutex );

   bool status = false;
   for ( int i = 0; i < list.size(); ++i ) {
      if ( register_sync_point( list[i], handle_set ) ) {
         status = true;
      }
   }
   return status;
}

bool const SyncPointList::register_sync_point(
   SyncPoint *sp )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
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

      string label_str;
      StringUtilities::to_string( label_str, sp->get_label() );
      ostringstream errmsg;
      errmsg << "SyncPointListBase::register_sync_point():" << __LINE__
             << " ERROR: Failed to register '" << label_str
             << "' synchronization point with RTI!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return registered;
}

bool const SyncPointList::register_sync_point(
   SyncPoint               *sp,
   FederateHandleSet const &handle_set )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
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

      string label_str;
      StringUtilities::to_string( label_str, sp->get_label() );
      ostringstream errmsg;
      errmsg << "SyncPointListBase::register_sync_point():" << __LINE__
             << " ERROR: Failed to register '" << label_str
             << "' synchronization point with RTI!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return registered;
}

bool const SyncPointList::is_announced(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint const *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_announced() );
}

/*!
 * @job_class{initialization}
 */
bool const SyncPointList::mark_announced(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );
   if ( sp != NULL ) {
      sp->set_state( SYNC_PT_STATE_ANNOUNCED );
      return true;
   }
   return false;
}

bool const SyncPointList::wait_for_announced(
   wstring const &label )
{
   SyncPoint *sp;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection auto_unlock_mutex( &mutex );

      sp = get_sync_point( label );

      // If the sync-point is null then it is unknown.
      if ( sp == NULL ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointList::wait_for_announced():" << __LINE__
                << " ERROR: Could not find sync-point '" << label_str
                << "' in the '" << get_list_name() << "' list!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }
   }

   return wait_for_announced( sp );
}

bool const SyncPointList::wait_for_all_announced()
{
   // NOTE: Locking the mutex while waiting can cause deadlock for callbacks.

   bool status = false;
   for ( int i = 0; i < list.size(); ++i ) {
      if ( wait_for_announced( list[i] ) ) {
         status = true;
      }
   }
   return status;
}

bool const SyncPointList::wait_for_announced(
   SyncPoint *sp )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::wait_for_announced():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::wait_for_announced():" << __LINE__
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
         ostringstream errmsg;
         errmsg << "SyncPointList::wait_for_announced():" << __LINE__
                << " ERROR: Bad sync-point state for sync-point!"
                << " The sync-point state: " << sp->to_string() << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
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
         ostringstream message;
         message << "SyncPointList::wait_for_announced():" << __LINE__
                 << " Sync-point: " << sp->to_string() << THLA_ENDL;
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
               errmsg << "SyncPointList::wait_for_announced():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution"
                      << " member. This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << THLA_ENDL;
               DebugHandler::terminate_with_message( errmsg.str() );
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
      message << "SyncPointList::wait_for_announced():" << __LINE__
              << " Sync-point announced: " << sp->to_string() << THLA_ENDL;
      send_hs( stdout, message.str().c_str() );
   }

   return announced;
}

bool const SyncPointList::is_achieved(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint const *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_achieved() );
}

bool const SyncPointList::achieve(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );

   // If the sync-point is null then it is unknown.
   if ( sp == NULL ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
             << " ERROR: Could not find sync-point '" << label_str
             << "' in the '" << get_list_name() << "' list!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   return achieve_sync_point( sp );
}

bool const SyncPointList::achieve_all()
{
   MutexProtection auto_unlock_mutex( &mutex );

   bool status = false;
   for ( int i = 0; i < list.size(); ++i ) {
      if ( achieve( list[i]->get_label() ) ) {
         status = true;
      }
   }
   return status;
}

bool const SyncPointList::achieve_sync_point(
   SyncPoint *sp )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string label_str;
      StringUtilities::to_string( label_str, sp->get_label() );
      ostringstream msg;
      msg << "SyncPointList::achieve_sync_point():" << __LINE__
          << " Known Synchronization-Point '" << label_str << "', state:"
          << sp->get_state() << THLA_ENDL;
      send_hs( stdout, msg.str().c_str() );
   }

   bool achieved = false;

   if ( sp->is_announced() ) {

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      RTIambassador *RTI_amb = federate->get_RTI_ambassador();

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

   } else if ( sp->is_achieved() ) {

      // If the synchronization point is already achieved then print out
      // a message.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
                << " Synchronization-Point '" << label_str
                << "' has already been achieved with the RTI!";
         send_hs( stderr, errmsg.str().c_str() );
      }

      achieved = true;

   } else if ( sp->is_synchronized() ) {
      // If the synchronization point is already synchronized, then print
      // out a message and return.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
                << " Synchronization-Point '" << label_str
                << "' has already been synchronized with the RTI!";
         send_hs( stderr, errmsg.str().c_str() );
      }

   } else {

      // Something went wrong. Print a message and exit.
      string label_str;
      StringUtilities::to_string( label_str, sp->get_label() );
      ostringstream errmsg;
      errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
             << " ERROR: Synchronization-Point '" << label_str
             << "' has not been announced with the RTI!";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return achieved;
}

bool const SyncPointList::is_synchronized(
   wstring const &label )
{
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint const *sp = get_sync_point( label );
   return ( ( sp != NULL ) && sp->is_synchronized() );
}

/*!
 * @job_class{initialization}
 */
bool const SyncPointList::mark_synchronized(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );
   if ( sp != NULL ) {

      // Mark the synchronization point at achieved which indicates the
      // federation is synchronized on the synchronization point.
      sp->set_state( SYNC_PT_STATE_SYNCHRONIZED );
      return true;
   }
   return false;
}

bool const SyncPointList::wait_for_synchronized(
   wstring const &label )
{
   SyncPoint *sp;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection auto_unlock_mutex( &mutex );

      sp = get_sync_point( label );

      // If the sync-point is null then it is unknown.
      if ( sp == NULL ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointList::wait_for_synchronized():" << __LINE__
                << " ERROR: Could not find sync-point '" << label_str
                << "' in the '" << get_list_name() << "' list!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }
   }

   return wait_for_synchronized( sp );
}

bool const SyncPointList::wait_for_all_synchronized()
{
   // NOTE: Locking the mutex while waiting can cause deadlock for callbacks.

   bool status = false;
   for ( int i = 0; i < list.size(); ++i ) {
      if ( wait_for_synchronized( list[i] ) ) {
         status = true;
      }
   }
   return status;
}

bool const SyncPointList::wait_for_synchronized(
   SyncPoint *sp )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::wait_for_synchronized():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::wait_for_synchronized():" << __LINE__
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
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream msg;
         msg << "SyncPointList::wait_for_synchronized():" << __LINE__
             << " Synchronization-Point '" << label_str << "'" << THLA_ENDL;
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
               errmsg << "SyncPointList::wait_for_synchronized():" << __LINE__
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

std::string SyncPointList::to_string()
{
   // Scope this mutex lock because locking over the blocking wait call
   // below will cause deadlock.
   MutexProtection auto_unlock_mutex( &mutex );

   ostringstream msg;
   msg << "SyncPointList::to_string():" << __LINE__ << THLA_ENDL
       << " List-Size:" << list.size() << THLA_ENDL;

   for ( int i = 0; i < list.size(); ++i ) {
      string label_str;
      StringUtilities::to_string( label_str, list[i]->get_label() );
      msg << i << ": List:'" << get_list_name() << "'"
          << " Sync-Point Label:'" << label_str << "'" << THLA_ENDL;
   }
   return msg.str();
}

std::string SyncPointList::to_string(
   std::wstring const &label )
{
   // Scope this mutex lock because locking over the blocking wait call
   // below will cause deadlock.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPoint *sp = get_sync_point( label );
   if ( sp != NULL ) {
      return sp->to_string();
   }

   string label_str;
   StringUtilities::to_string( label_str, label );
   ostringstream msg;
   msg << "SyncPointList::to_string():" << __LINE__
       << " Unknown sync-point label: '" << label_str << "'" << THLA_ENDL;
   return msg.str();
}
