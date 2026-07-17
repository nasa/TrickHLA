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
@trick_link_dependency{SyncPoint.cpp}
@trick_link_dependency{SyncPointList.cpp}
@trick_link_dependency{SyncPointTimed.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{time/Int64Time.cpp}
@trick_link_dependency{utils/MutexLock.cpp}
@trick_link_dependency{utils/MutexProtection.cpp}
@trick_link_dependency{utils/SleepTimeout.cpp}
@trick_link_dependency{utils/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2024, --, Initial implementation.}
@revs_end

*/

// System includes.
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/SyncPoint.hh"
#include "TrickHLA/SyncPointList.hh"
#include "TrickHLA/SyncPointTimed.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/utils/MutexLock.hh"
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
SyncPointList::SyncPointList()
   : list(),
     list_name(),
     mutex( NULL ),
     federate( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPointList::SyncPointList(
   std::string const &name,
   MutexLock         &mtx,
   Federate          *fed )
   : list(),
     list_name( name ),
     mutex( &mtx ),
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

void SyncPointList::setup(
   Federate *fed )
{
   if ( this->mutex == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::setup():" << __LINE__
             << " ERROR: Unexpected NULL mutex for sync-point list named '"
             << this->list_name << "'! Make sure to call the set_mutex()"
             << " function for this SyncPointList instance." << endl;
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   MutexProtection const auto_unlock_mutex( mutex );

   set_federate( fed );
}

void SyncPointList::set_list_name(
   std::string const &name )
{
   this->list_name = name;
}

string &SyncPointList::get_list_name()
{
   return this->list_name;
}

void SyncPointList::set_mutex(
   MutexLock &mtx )
{
   this->mutex = &mtx;
}

void SyncPointList::set_federate(
   Federate *fed )
{
   this->federate = fed;
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::set_federate():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer for sync-point list named '"
             << this->list_name << "'!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return;
   }
}

SyncPtStateEnum SyncPointList::get_state(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );
   SyncPoint const      *sp = get( label );
   return ( sp != NULL ) ? sp->get_state() : TrickHLA::SYNC_PT_STATE_UNKNOWN;
}

void SyncPointList::clear()
{

   // Clear/remove everything from the list.
   while ( !list.empty() ) {
      TMM_delete_var_a( list.back() );
      list.back() = NULL;
      list.pop_back();
   }

   return;
}

SyncPoint *SyncPointList::get(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );

   for ( SyncPoint *sync_pnt : list ) {
      if ( label.compare( sync_pnt->get_label() ) == 0 ) {
         return sync_pnt;
      }
   }
   return NULL;
}

bool SyncPointList::add(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );

   if ( contains( label ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointList::add():" << __LINE__
             << " ERROR: The sync-point label '" << label_str
             << "' has already been added!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Add the sync-point to the corresponding named list.
   //
   SyncPoint *sync_pnt = (SyncPoint *)TMM_declare_var_1d( "TrickHLA::SyncPoint", 1 );
   sync_pnt->set_label( label );
   list.push_back( sync_pnt );

   return true;
}

bool SyncPointList::add(
   wstring const   &label,
   Int64Time const &time )
{
   MutexProtection const auto_unlock_mutex( mutex );

   if ( contains( label ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointList::add():" << __LINE__
             << " ERROR: The sync-point label '" << label_str
             << "' has already been added!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Add the sync-point to the corresponding named list.
   //
   SyncPointTimed *sync_pnt = (SyncPointTimed *)TMM_declare_var_1d( "TrickHLA::SyncPointTimed", 1 );
   sync_pnt->set_label( label );
   sync_pnt->set_time( time );
   list.push_back( sync_pnt );

   return true;
}

bool SyncPointList::contains(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );

   for ( SyncPoint const *sync_pnt : list ) {
      if ( label.compare( sync_pnt->get_label() ) == 0 ) {
         return true;
      }
   }
   return false;
}

bool SyncPointList::is_registered(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint const *sp = get( label );
   return ( ( sp != NULL ) && sp->is_registered() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPointList::mark_registered(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint *sp = get( label );
   if ( sp != NULL ) {
      sp->set_state( TrickHLA::SYNC_PT_STATE_REGISTERED );
      return true;
   }
   return false;
}

bool SyncPointList::register_sync_point(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint *sp = get( label );

   // If the sync-point is null then it is unknown.
   if ( sp == NULL ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Did not find sync-point '" << label_str
             << "' in the '" << get_list_name() << "' list!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   return register_sync_point( sp );
}

bool SyncPointList::register_sync_point(
   wstring const           &label,
   FederateHandleSet const &handle_set )
{
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint *sp = get( label );

   // If the sync-point is null then it is unknown.
   if ( sp == NULL ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Did not find sync-point '" << label_str
             << "' in the '" << get_list_name() << "' list!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   return register_sync_point( sp, handle_set );
}

// True if at least one sync-point is registered.
bool SyncPointList::register_all()
{
   MutexProtection const auto_unlock_mutex( mutex );

   bool status = false;
   for ( SyncPoint *sync_pnt : list ) {
      if ( register_sync_point( sync_pnt ) ) {
         status = true;
      }
   }
   return status;
}

bool SyncPointList::register_all(
   FederateHandleSet const &handle_set )
{
   MutexProtection const auto_unlock_mutex( mutex );

   bool status = false;
   for ( SyncPoint *sync_pnt : list ) {
      if ( register_sync_point( sync_pnt, handle_set ) ) {
         status = true;
      }
   }
   return status;
}

bool SyncPointList::register_sync_point(
   SyncPoint *sp )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << endl;
      DebugHandler::terminate( errmsg.str() );
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
      MutexProtection const auto_unlock_mutex( mutex );

      RTI_amb->registerFederationSynchronizationPoint( sp->get_label(),
                                                       sp->encode_user_supplied_tag() );
      // Mark the sync-point as registered.
      sp->set_state( TrickHLA::SYNC_PT_STATE_REGISTERED );

      registered = true;

   } catch ( RTI1516_NAMESPACE::Exception const &e ) {

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string label_str;
      StringUtilities::to_string( label_str, sp->get_label() );
      ostringstream errmsg;
      errmsg << "SyncPointListBase::register_sync_point():" << __LINE__
             << " ERROR: Failed to register '" << label_str
             << "' synchronization point with RTI!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return registered;
}

bool SyncPointList::register_sync_point(
   SyncPoint               *sp,
   FederateHandleSet const &handle_set )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::register_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << endl;
      DebugHandler::terminate( errmsg.str() );
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
      MutexProtection const auto_unlock_mutex( mutex );

      RTI_amb->registerFederationSynchronizationPoint( sp->get_label(),
                                                       sp->encode_user_supplied_tag(),
                                                       handle_set );
      // Mark the sync-point as registered.
      sp->set_state( TrickHLA::SYNC_PT_STATE_REGISTERED );

      registered = true;

   } catch ( RTI1516_NAMESPACE::Exception const &e ) {

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string label_str;
      StringUtilities::to_string( label_str, sp->get_label() );
      ostringstream errmsg;
      errmsg << "SyncPointListBase::register_sync_point():" << __LINE__
             << " ERROR: Failed to register '" << label_str
             << "' synchronization point with RTI!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return registered;
}

bool SyncPointList::is_announced(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint const *sp = get( label );
   return ( ( sp != NULL ) && sp->is_announced() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPointList::mark_announced(
   wstring const            &label,
   VariableLengthData const &user_supplied_tag )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint *sp = get( label );
   if ( sp != NULL ) {
      sp->set_state( TrickHLA::SYNC_PT_STATE_ANNOUNCED );
      sp->decode_user_supplied_tag( user_supplied_tag );
      return true;
   }
   return false;
}

bool SyncPointList::wait_for_announced(
   wstring const &label )
{
   SyncPoint *sp;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection const auto_unlock_mutex( mutex );

      sp = get( label );

      // If the sync-point is null then it is unknown.
      if ( sp == NULL ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointList::wait_for_announced():" << __LINE__
                << " ERROR: Could not find sync-point '" << label_str
                << "' in the '" << get_list_name() << "' list!" << endl;
         DebugHandler::terminate( errmsg.str() );
         return false;
      }
   }

   return wait_for_announced( sp );
}

bool SyncPointList::wait_for_all_announced()
{
   // NOTE: Locking the mutex while waiting can cause deadlock for callbacks.

   bool status = false;
   for ( SyncPoint *sync_pnt : list ) {
      if ( wait_for_announced( sync_pnt ) ) {
         status = true;
      }
   }
   return status;
}

bool SyncPointList::wait_for_announced(
   SyncPoint *sp )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::wait_for_announced():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::wait_for_announced():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   bool announced = false;

   // Critical code section.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( mutex );
      announced = sp->is_announced();

      if ( !announced && !sp->is_valid() ) {
         ostringstream errmsg;
         errmsg << "SyncPointList::wait_for_announced():" << __LINE__
                << " ERROR: Bad sync-point state for sync-point!"
                << " Sync-point: " << sp->to_string() << endl;
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
         message << "SyncPointList::wait_for_announced():" << __LINE__
                 << " Sync-point: " << sp->to_string() << endl;
         message_publish( MSG_NORMAL, message.str().c_str() );
      }

      // Always check to see is a shutdown was received.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep();

      // Critical code section.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection const auto_unlock_mutex( mutex );
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
               errmsg << "SyncPointList::wait_for_announced():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution"
                      << " member. This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
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
      message << "SyncPointList::wait_for_announced():" << __LINE__
              << " Sync-point announced: " << sp->to_string() << endl;
      message_publish( MSG_NORMAL, message.str().c_str() );
   }

   return announced;
}

bool SyncPointList::is_achieved(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint const *sp = get( label );
   return ( ( sp != NULL ) && sp->is_achieved() );
}

bool SyncPointList::achieve(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint *sp = get( label );

   // If the sync-point is null then it is unknown.
   if ( sp == NULL ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      ostringstream errmsg;
      errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
             << " ERROR: Could not find sync-point '" << label_str
             << "' in the '" << get_list_name() << "' list!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   return achieve_sync_point( sp );
}

bool SyncPointList::achieve_all()
{
   MutexProtection const auto_unlock_mutex( mutex );

   bool status = false;
   for ( SyncPoint const *sync_pnt : list ) {
      if ( achieve( sync_pnt->get_label() ) ) {
         status = true;
      }
   }
   return status;
}

bool SyncPointList::achieve_sync_point(
   SyncPoint *sp )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "SyncPointList::achieve_sync_point():" << __LINE__
          << " Known Sync-point " << sp->to_string() << endl;
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
         MutexProtection const auto_unlock_mutex( mutex );

         RTI_amb->synchronizationPointAchieved( sp->get_label() );

         // Mark the sync-point as achieved.
         sp->set_state( TrickHLA::SYNC_PT_STATE_ACHIEVED );

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
         msg << "SyncPointList::achieve_sync_point():" << __LINE__
             << " For Known Sync-point " << sp->to_string()
             << ", Not Connected to RTI!" << endl;
         message_publish( MSG_WARNING, msg.str().c_str() );
         if ( federate != NULL ) {
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
         errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
                << " Sync-point '" << label_str
                << "' has already been achieved with the RTI!" << endl;
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
         errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
                << " Sync-point '" << label_str
                << "' has already been synchronized with the RTI!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }

      achieved = true;

   } else if ( sp->is_registered() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
                << " WARNING: Sync-point '" << label_str
                << "' is registered but has not been announced by the RTI!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   } else if ( sp->is_known() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
                << " WARNING: Sync-point '" << label_str
                << "' is known but has not been registered or announced!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   } else {
      // Sync-point is unknown.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, sp->get_label() );
         ostringstream errmsg;
         errmsg << "SyncPointList::achieve_sync_point():" << __LINE__
                << " WARNING: Sync-point '" << label_str
                << "' is unknown!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }

   return achieved;
}

bool SyncPointList::is_synchronized(
   wstring const &label )
{
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint const *sp = get( label );
   return ( ( sp != NULL ) && sp->is_synchronized() );
}

bool SyncPointList::is_all_synchronized()
{
   MutexProtection const auto_unlock_mutex( mutex );

   for ( SyncPoint const *sync_pnt : list ) {
      if ( !sync_pnt->is_synchronized() ) {
         return false;
      }
   }
   // Can only be synchronized if the list was not empty and we had no
   // unsynchronized sync-points.
   return ( !empty() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPointList::mark_synchronized(
   wstring const &label )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint *sp = get( label );
   if ( sp != NULL ) {

      // Mark the synchronization point at achieved which indicates the
      // federation is synchronized on the synchronization point.
      sp->set_state( TrickHLA::SYNC_PT_STATE_SYNCHRONIZED );
      return true;
   }
   return false;
}

bool SyncPointList::wait_for_synchronized(
   wstring const &label )
{
   SyncPoint const *sp;
   {
      // Scope this mutex lock because locking over the blocking wait call
      // below will cause deadlock.
      MutexProtection const auto_unlock_mutex( mutex );

      sp = get( label );

      // If the sync-point is null then it is unknown.
      if ( sp == NULL ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SyncPointList::wait_for_synchronized():" << __LINE__
                << " ERROR: Could not find sync-point '" << label_str
                << "' in the '" << get_list_name() << "' list!" << endl;
         DebugHandler::terminate( errmsg.str() );
         return false;
      }
   }

   return wait_for_synchronized( sp );
}

bool SyncPointList::wait_for_all_synchronized()
{
   // NOTE: Locking the mutex while waiting can cause deadlock for callbacks.

   bool status = false;
   for ( SyncPoint const *sync_pnt : list ) {
      if ( wait_for_synchronized( sync_pnt ) ) {
         status = true;
      }
   }
   return status;
}

bool SyncPointList::wait_for_synchronized(
   SyncPoint const *sp )
{
   if ( sp == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::wait_for_synchronized():" << __LINE__
             << " ERROR: Unexpected NULL SyncPoint!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return false;
   }
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::wait_for_synchronized():" << __LINE__
             << " ERROR: Unexpected NULL federate pointer!" << endl;
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
         msg << "SyncPointList::wait_for_synchronized():" << __LINE__
             << " Sync-point '" << label_str << "'" << endl;
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      // Critical code section.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks
         // the mutex even if there is an exception.
         MutexProtection const auto_unlock_mutex( mutex );
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
               errmsg << "SyncPointList::wait_for_synchronized():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution"
                      << " member. This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
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

std::string SyncPointList::to_string()
{
   unsigned int list_index = 0;

   // Scope this mutex lock because locking over the blocking wait call
   // below will cause deadlock.
   MutexProtection const auto_unlock_mutex( mutex );

   ostringstream msg;

   msg << "SyncPointList::to_string():" << __LINE__
       << " List:'" << get_list_name() << "' List-size:" << list.size() << endl;
   for ( SyncPoint *sync_pnt : list ) {
      msg << list_index++ << ":'" << get_list_name() << "' Sync-point:"
          << sync_pnt->to_string() << endl;
   }
   return msg.str();
}

std::string SyncPointList::to_string(
   wstring const &label )
{
   // Scope this mutex lock because locking over the blocking wait call
   // below will cause deadlock.
   MutexProtection const auto_unlock_mutex( mutex );

   SyncPoint *sp = get( label );
   if ( sp != NULL ) {
      return sp->to_string();
   }

   string label_str;
   StringUtilities::to_string( label_str, label );
   ostringstream msg;
   msg << "SyncPointList::to_string():" << __LINE__
       << " Unknown sync-point label: '" << label_str << "'" << endl;
   return msg.str();
}

/*! @brief Encode the variables to a form Trick can checkpoint. */
void SyncPointList::convert_data_before_checkpoint()
{
   for ( SyncPoint *sync_pnt : list ) {
      sync_pnt->convert_data_before_checkpoint();
   }
}

/*! @brief Decode the state of this class from the Trick checkpoint. */
void SyncPointList::restore_data_after_checkpoint()
{
   for ( SyncPoint *sync_pnt : list ) {
      sync_pnt->restore_data_after_checkpoint();
   }
}

/*! @brief Free/release the memory used for the checkpoint data structures. */
void SyncPointList::free_converted_data_for_checkpoint()
{
   for ( SyncPoint *sync_pnt : list ) {
      sync_pnt->free_converted_data_for_checkpoint();
   }
}
