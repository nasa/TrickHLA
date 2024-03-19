/*!
@file TrickHLA/SyncPntListBase.cpp
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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{SyncPnt.cpp}
@trick_link_dependency{SyncPntListBase.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
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
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPnt.hh"
#include "TrickHLA/SyncPntListBase.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPntListBase::SyncPntListBase()
   : mutex()
{
   return;
}

/*!
 * @details This is a pure virtual destructor.
 * @job_class{shutdown}
 */
SyncPntListBase::~SyncPntListBase()
{
   this->reset();

   // Make sure we destroy the mutex.
   mutex.destroy();
}

SyncPnt *SyncPntListBase::add_sync_point(
   wstring const &label )
{
   if ( contains( label ) ) {
      ostringstream errmsg;
      errmsg << "SyncPntListBase::add_sync_point():" << __LINE__
             << " ERROR: The sync-point label '" << label.c_str()
             << "' has already been added!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   SyncPnt *sp = new SyncPnt( label );

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );
   sync_point_list.push_back( sp );

   return sp;
}

SyncPnt *SyncPntListBase::get_sync_point(
   std::wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !sync_point_list.empty() ) {

      // Find the sync-point.
      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );

         // Check for a match.
         if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {
            // Return the sync-point.
            return sp;
         }
      }
   }

   // Must not have found the sync-point.
   return NULL;
}

SyncPnt *SyncPntListBase::register_sync_point(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb,
   wstring const                    &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   if ( ( sp != NULL ) && !sp->is_registered() ) {
      register_sync_point( RTI_amb, sp );
   }
   return sp;
}

SyncPnt *SyncPntListBase::register_sync_point(
   RTI1516_NAMESPACE::RTIambassador           &RTI_amb,
   RTI1516_NAMESPACE::FederateHandleSet const &federate_handle_set,
   wstring const                              &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt *sp = get_sync_point( label );
   if ( ( sp != NULL ) && !sp->is_registered() ) {
      register_sync_point( RTI_amb, federate_handle_set, sp );
   }
   return sp;
}

void SyncPntListBase::register_all_sync_points(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   // Iterate through all the synchronization points that have been added.
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      // Only add them if they are not already registered.
      if ( ( sp != NULL ) && !sp->is_registered() ) {

         // Register the synchronization point.
         register_sync_point( RTI_amb, sp );
      }
   }
}

void SyncPntListBase::register_all_sync_points(
   RTI1516_NAMESPACE::RTIambassador           &RTI_amb,
   RTI1516_NAMESPACE::FederateHandleSet const &federate_handle_set )
{
   if ( federate_handle_set.empty() ) {
      register_all_sync_points( RTI_amb );
   } else {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );

         // Only add them if they are not already registered.
         if ( ( sp != NULL ) && !sp->is_registered() ) {

            // Register the synchronization point with federate in set.
            register_sync_point( RTI_amb, federate_handle_set, sp );
         }
      }
   }
}

void SyncPntListBase::sync_point_registration_succeeded(
   wstring const &label )
{
   if ( mark_registered( label ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         send_hs( stdout, "SyncPntListBase::sync_point_registration_succeeded():%d Label:'%ls'%c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
      }
   }
}

void SyncPntListBase::sync_point_registration_failed(
   wstring const &label,
   bool const     not_unique )
{
   // Only handle the sync-points we know about.
   if ( contains( label ) ) {

      // If the reason for the failure is that the label is not unique then
      // this means the sync-point is registered with the RTI it just means
      // we did not do it.
      if ( not_unique ) {
         mark_registered( label );
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            send_hs( stdout, "SyncPntListBase::sync_point_registration_failed():%d Label:'%ls' already exists.%c",
                     __LINE__, label.c_str(), THLA_NEWLINE );
         }
      } else {
         string name;
         StringUtilities::to_string( name, label );
         ostringstream errmsg;
         errmsg << "SyncPntListBase::sync_point_registration_failed():" << __LINE__
                << " ERROR: '" << name << "'" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}

void SyncPntListBase::wait_for_all_announcements(
   Federate *fed_ptr )
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      send_hs( stdout, "SyncPntListBase::wait_for_all_announcements():%d Waiting...%c",
               __LINE__, THLA_NEWLINE );
      print_sync_points();
   }

   // Iterate through the synchronization point list.
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      // Wait for the sync-point announcement.
      wait_for_sync_point_announcement( fed_ptr, sp );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      print_sync_points();
   }
}

void SyncPntListBase::announce_sync_point(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb,
   wstring const                    &label,
   RTI1516_USERDATA const           &user_supplied_tag )
{
   // Check to see if the synchronization point is known and is in the list.
   if ( contains( label ) ) {

      // Mark sync-point as existing/announced.
      if ( mark_announced( label ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            send_hs( stdout, "SyncPntListBase::announce_sync_point():%d Synchronization point announced:'%ls'%c",
                     __LINE__, label.c_str(), THLA_NEWLINE );
         }
      }
   } else {
      // By default, achieve unrecognized synchronization point.

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         send_hs( stdout, "SyncPntListBase::announce_sync_point():%d Unrecognized synchronization point:'%ls', which will be achieved.%c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
      }

      // Unknown synchronization point so achieve it but don't wait for the
      // federation to be synchronized on it.
      achieve_sync_point( RTI_amb, label );
   }
}

void SyncPntListBase::wait_for_list_synchronization(
   Federate *federate )
{
   bool         achieved, valid_and_not_achieved;
   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   // Iterate through this SyncPntList's synchronization point list.
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      // Critical code section with a scope specific mutex lock.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &mutex );
         valid_and_not_achieved = ( sp != NULL ) && sp->is_valid() && !sp->is_achieved();
      }

      // Wait for a synchronization point if it is valid but not achieved.
      if ( valid_and_not_achieved ) {

         // Wait for the federation to synchronized on the sync-point.
         do {

            // Critical code section with a scope specific mutex lock.
            {
               // When auto_unlock_mutex goes out of scope it automatically
               // unlocks the mutex even if there is an exception.
               MutexProtection auto_unlock_mutex( &mutex );
               achieved = sp->is_achieved();
            }

            if ( !achieved ) {
               // Always check to see is a shutdown was received.
               federate->check_for_shutdown_with_termination();

               // Pause and release the processor for short sleep value.
               sleep_timer.sleep();

               // To be more efficient, we get the time once and share it.
               wallclock_time = sleep_timer.time();

               // Periodically check to make sure the federate is still part of
               // the federation execution.
               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "SyncPntListBase::wait_for_list_synchronization():" << __LINE__
                            << " ERROR: Unexpectedly the Federate is no longer an execution"
                            << " member. This means we are either not connected to the"
                            << " RTI or we are no longer joined to the federation"
                            << " execution because someone forced our resignation at"
                            << " the Central RTI Component (CRC) level!"
                            << THLA_ENDL;
                     DebugHandler::terminate_with_message( errmsg.str() );
                  }
               }

               if ( print_timer.timeout( wallclock_time ) ) {
                  print_timer.reset();
                  string name;
                  StringUtilities::to_string( name, sp->get_label() );
                  ostringstream msg;
                  msg << "SyncPntListBase::wait_for_synchronization():" << __LINE__
                      << " Synchronization-Point '" << name << "', waiting..." << THLA_ENDL;
                  send_hs( stdout, msg.str().c_str() );
               }
            }
         } while ( !achieved );
      }

      // Critical code section with a scope specific mutex lock.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &mutex );

         // Now that the sync-point is achieved, reset the state to EXISTS.
         if ( ( sp != NULL ) && !sp->is_achieved() ) {
            sp->set_state( SYNC_PT_STATE_EXISTS );
         }
      }
   }
}

void SyncPntListBase::achieve_and_wait_for_synchronization(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb,
   Federate                         *federate,
   std::wstring const               &label )
{
   string name;
   StringUtilities::to_string( name, label );

   // Check for the synchronization point by label.
   SyncPnt *sp = get_sync_point( label );

   // If the pointer is not NULL then we found it.
   if ( sp != NULL ) {

      // If the synchronization point is announced, then achieve it.
      if ( sp->is_announced() ) {

         // Achieve the synchronization point.
         achieve_sync_point( RTI_amb, sp );

      } else if ( sp->is_achieved() ) {

         // If the synchronization point is already achieved then print out
         // a message and move on to waiting for synchronization.
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            ostringstream errmsg;
            errmsg << "SyncPntListBase::achieve_and_wait_for_synchronization():"
                   << __LINE__
                   << " Synchronization-Point '" << name
                   << "' has already been achieved with the RTI!";
            send_hs( stderr, errmsg.str().c_str() );
         }

      } else if ( sp->is_synchronized() ) {

         // If the synchronization point is already synchronized, then print
         // out a message and return.
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            ostringstream errmsg;
            errmsg << "SyncPntListBase::achieve_and_wait_for_synchronization():"
                   << __LINE__
                   << " Synchronization-Point '" << name
                   << "' has already been synchronized with the RTI!";
            send_hs( stderr, errmsg.str().c_str() );
         }
         return;

      } else {

         // Okay, something is wrong here. Print a message and exit.
         ostringstream errmsg;
         errmsg << "SyncPntListBase::achieve_and_wait_for_synchronization():"
                << __LINE__
                << " ERROR: Synchronization-Point '" << name
                << "' has not been announced with the RTI!";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      bool achieved_and_not_synched;
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks
         // the mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &mutex );
         achieved_and_not_synched = sp->is_achieved() && !sp->is_synchronized();
      }
      // If achieved and not synchronized, then synchronize with the federation.
      if ( achieved_and_not_synched ) {
         // WARNING: Make sure we don't lock the mutex before we make this
         // call otherwise it will block the FedAmb callbacks.
         wait_for_synchronization( federate, sp );
         return;
      }

   } else {

      // Okay, we did not find the synchronization point. Print out an
      // error message and then exit.
      ostringstream errmsg;
      errmsg << "SyncPntListBase::achieve_and_wait_for_synchronization():"
             << __LINE__
             << " ERROR: Synchronization-Point '" << name
             << "' not found!";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::achieve_all_sync_points(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb )
{
   bool acknowledged = false;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   // Iterate through all the synchronization points.
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      if ( ( sp != NULL ) && sp->is_announced() ) {
         if ( achieve_sync_point( RTI_amb, sp ) ) {
            acknowledged = true;
         }
      }
   }
   return ( acknowledged );
}

SyncPtStateEnum SyncPntListBase::get_sync_point_state(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   SyncPnt const *sp = get_sync_point( label );
   if ( sp != NULL ) {
      return sp->get_state();
   }
   // We did not find the sync-point.
   return SYNC_PT_STATE_ERROR;
}

bool SyncPntListBase::is_sync_point_announced(
   wstring const &label )
{
   SyncPtStateEnum state = get_sync_point_state( label );
   return ( state == SYNC_PT_STATE_ANNOUNCED );
}

bool SyncPntListBase::clear_sync_point(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL ) && ( sp->is_achieved() ) && ( label.compare( sp->get_label() ) == 0 ) ) {

            // Extension class dependent code would go here.

            sync_point_list.erase( i );
            delete sp;
            i = sync_point_list.end();

            return true;
         }
      }
   }
   return false;
}

void SyncPntListBase::reset()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   while ( !sync_point_list.empty() ) {
      if ( *sync_point_list.begin() != NULL ) {
         delete ( *sync_point_list.begin() );
         sync_point_list.erase( sync_point_list.begin() );
      }
   }
   sync_point_list.clear();
}

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::contains(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt const *sp = ( *i );
         if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {
            return true;
         }
      }
   }
   return false;
}

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::mark_registered(
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

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::mark_announced(
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

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::mark_synchronized(
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

wstring SyncPntListBase::to_wstring()
{
   wstring result = L"Sync Points\n  state: ";

   // Extension class dependent code would go here.

   // Critical code section.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( sp != NULL ) {
            result += L"  " + sp->to_wstring() + L"\n";
         }
      }
   }
   result += L"\n";
   return result;
}

void SyncPntListBase::convert_sync_points(
   LoggableSyncPnt *sync_points )
{
   int loop = 0;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      ( *i )->convert( sync_points[loop++] );
   }
}

void SyncPntListBase::print_sync_points()
{
   ostringstream msg;
   msg << "SyncPntListBase::print_sync_points():" << __LINE__ << endl
       << "#############################" << endl
       << "Sync Point Dump: " << sync_point_list.size() << endl;

   // Critical code section.
   {
      string sync_point_label;

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         wstring spwl = ( *i )->to_wstring();
         sync_point_label.assign( spwl.begin(), spwl.end() );
         msg << sync_point_label << endl;
      }
   }
   msg << "#############################" << endl;
   send_hs( stdout, msg.str().c_str() );
}

void SyncPntListBase::register_sync_point(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb,
   SyncPnt                          *sp )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Register the sync-point label.
   try {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      RTI_amb.registerFederationSynchronizationPoint( sp->get_label(),
                                                      RTI1516_USERDATA( 0, 0 ) );
      // Mark the sync-point as registered.
      sp->set_state( SYNC_PT_STATE_REGISTERED );

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
}

void SyncPntListBase::register_sync_point(
   RTI1516_NAMESPACE::RTIambassador           &RTI_amb,
   RTI1516_NAMESPACE::FederateHandleSet const &federate_handle_set,
   SyncPnt                                    *sp )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Register the sync-point label.
   try {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      RTI_amb.registerFederationSynchronizationPoint( sp->get_label(),
                                                      RTI1516_USERDATA( 0, 0 ),
                                                      federate_handle_set );
      // Mark the sync-point as registered.
      sp->set_state( SYNC_PT_STATE_REGISTERED );

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
}

bool SyncPntListBase::wait_for_sync_point_announcement(
   Federate           *federate,
   std::wstring const &label )
{
   bool     announced = false;
   SyncPnt *sp        = get_sync_point( label );

   if ( sp != NULL ) {
      announced = wait_for_sync_point_announcement( federate, sp );
   } else {
      string name;
      StringUtilities::to_string( name, label );
      ostringstream errmsg;
      errmsg << "SyncPntListBase::wait_for_sync_point_announcement():" << __LINE__
             << " ERROR: Unknown sync-point '" << name
             << "', which means there is not an internal SyncPnt instance to"
             << " manage the state against!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   return announced;
}

bool SyncPntListBase::wait_for_sync_point_announcement(
   Federate *federate,
   SyncPnt  *sp )
{
   bool announced = false;
   if ( sp != NULL ) {

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
            errmsg << "SyncPntListBase::wait_for_sync_point_announcement():" << __LINE__
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
            message << "SyncPntListBase::wait_for_sync_point_announcement():" << __LINE__
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
                  errmsg << "SyncPntListBase::wait_for_sync_point_announcement():" << __LINE__
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
         message << "SyncPntListBase::wait_for_sync_point_announcement():" << __LINE__
                 << " Sync-point announced: " << sp_status << THLA_ENDL;
         send_hs( stdout, message.str().c_str() );
      }
   }
   return announced;
}

bool SyncPntListBase::achieve_sync_point(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb,
   wstring const                    &label )
{
   bool     achieved = false;
   SyncPnt *sp       = get_sync_point( label );

   if ( sp != NULL ) {

      // Achieve known sync-point, which will also update its state.
      achieved = achieve_sync_point( RTI_amb, sp );

   } else {

      // Achieve unknown sync-point and there is no state to update.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string name;
         StringUtilities::to_string( name, label );
         ostringstream msg;
         msg << "SyncPntListBase::achieve_sync_point():" << __LINE__
             << " Unknown Synchronization-Point '" << name << "'"
             << THLA_ENDL;
         send_hs( stdout, msg.str().c_str() );
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      try {
         RTI_amb.synchronizationPointAchieved( label );
         achieved = true;
      } catch ( SynchronizationPointLabelNotAnnounced const &e ) {
         // Error will result in us returning false.
      } catch ( FederateNotExecutionMember const &e ) {
         // Error will result in us returning false.
      } catch ( SaveInProgress const &e ) {
         // Error will result in us returning false.
      } catch ( RestoreInProgress const &e ) {
         // Error will result in us returning false.
      } catch ( NotConnected const &e ) {
         // Error will result in us returning false.
      } catch ( RTIinternalError const &e ) {
         // Error will result in us returning false.
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
   return achieved;
}

bool SyncPntListBase::achieve_sync_point(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb,
   SyncPnt                          *sp )
{
   bool achieved = false;

   if ( sp != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string name;
         StringUtilities::to_string( name, sp->get_label() );
         ostringstream msg;
         msg << "SyncPntListBase::achieve_sync_point():" << __LINE__
             << " Known Synchronization-Point '" << name << "', state:"
             << sp->get_state() << THLA_ENDL;
         send_hs( stdout, msg.str().c_str() );
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      try {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &mutex );

         RTI_amb.synchronizationPointAchieved( sp->get_label() );

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
   }
   return achieved;
}

bool SyncPntListBase::wait_for_synchronization(
   Federate *federate,
   SyncPnt  *sp )
{
   if ( sp != NULL ) {
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
            msg << "SyncPntListBase::wait_for_synchronization():" << __LINE__
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
                  errmsg << "SyncPntListBase::wait_for_synchronization():" << __LINE__
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
   return false;
}
