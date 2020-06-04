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
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{SyncPnt.cpp}
@trick_link_dependency{SyncPntListBase.cpp}

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
#include "TrickHLA/SyncPntListBase.hh"
#include "TrickHLA/Utilities.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPntListBase::SyncPntListBase()
   : read_locks( 0 ),
     write_locks( 0 )
{
}

/*!
 * @details This is a pure virtual destructor.
 * @job_class{shutdown}
 */
SyncPntListBase::~SyncPntListBase()
{
   SyncPntListBase::reset();
}

void SyncPntListBase::add_sync_pnt(
   wstring const &label )
{
   SyncPnt *sp = new SyncPnt( label );
   lock_read_write();
   sync_point_list.push_back( sp );
   unlock_read_write();
}

SyncPnt *SyncPntListBase::get_sync_pnt(
   std::wstring const &label )
{

   // Mark the read lock.
   lock_read_only();

   // Find the sync-point.
   if ( !sync_point_list.empty() ) {

      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {

         SyncPnt *sp = ( *i );

         // Check for a match.
         if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {

            // Release the read lock.
            unlock_read_only();

            // Return the sync-point.
            return sp;
         }
      }
   }

   // Release the read lock.
   unlock_read_only();

   // Must not have found the sync-point.
   return NULL;
}

SyncPnt *SyncPntListBase::register_sync_pnt(
   RTI1516_NAMESPACE::RTIambassador &rti_amb,
   wstring const &                   label )
{
   lock_read_only();
   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {
            // Check to see if the sync-point is already registered.
            if ( sp->is_registered() ) {
               unlock_read_only();
               return sp;
            }
            sp->register_sync_point( rti_amb );
            unlock_read_only();
            return sp;
         }
      }
   }
   unlock_read_only();

   // Must not have found the sync-point.
   return NULL;
}

SyncPnt *SyncPntListBase::register_sync_pnt(
   RTI1516_NAMESPACE::RTIambassador &          rti_amb,
   RTI1516_NAMESPACE::FederateHandleSet const &federate_handle_set,
   wstring const &                             label )
{
   lock_read_only();
   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {
            // Check to see if the sync-point is already registered.
            if ( sp->is_registered() ) {
               unlock_read_only();
               return sp;
            }
            sp->register_sync_point( rti_amb, federate_handle_set );
            unlock_read_only();
            return sp;
         }
      }
   }
   unlock_read_only();

   // Must not have found the sync-point.
   return NULL;
}

void SyncPntListBase::register_all_sync_pnts(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Iterate through all the synchronization points that have been added.
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      // Only add them if they are not already registered.
      if ( ( sp != NULL ) && !sp->is_registered() ) {
         // Register the synchronization point.
         sp->register_sync_point( rti_ambassador );
      }
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

void SyncPntListBase::register_all_sync_pnts(
   RTI1516_NAMESPACE::RTIambassador &          rti_ambassador,
   RTI1516_NAMESPACE::FederateHandleSet const &federate_handle_set )
{
   if ( federate_handle_set.empty() ) {
      register_all_sync_pnts( rti_ambassador );
   } else {
      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         // Only add them if they are not already registered.
         if ( ( sp != NULL ) && !sp->is_registered() ) {
            // Register the synchronization point with federate in set.
            sp->register_sync_point( rti_ambassador, federate_handle_set );
         }
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
   return;
}

void SyncPntListBase::sync_point_registration_succeeded(
   wstring const &label )
{
   if ( this->mark_registered( label ) ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         send_hs( stdout, "SyncPntListBase::sync_point_registration_succeeded():%d Label:'%ls'%c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
      }
   }
   return;
}

void SyncPntListBase::sync_point_registration_failed(
   wstring const &label,
   bool           not_unique )
{

   // Only handle the sync-points we know about.
   if ( this->contains( label ) ) {

      // If the reason for the failure is that the label is not unique then
      // this means the sync-point is registered with the RTI it just means
      // we did not do it.
      if ( not_unique ) {
         this->mark_registered( label );
         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            send_hs( stdout, "SyncPntListBase::sync_point_registration_failed():%d Label:'%ls' already exists.%c",
                     __LINE__, label.c_str(), THLA_NEWLINE );
         }
      } else {
         send_hs( stderr, "SyncPntListBase::sync_point_registration_failed():%d '%ls' Synchronization Point failure, federate shutting down!%c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
         exit( 1 );
      }
   }

   return;
}

void SyncPntListBase::wait_for_announcement(
   Federate *     federate,
   wstring const &label )
{
   SyncPnt *sp = this->get_sync_pnt( label );
   if ( sp != NULL ) {
      sp->wait_for_announce( federate );
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream message;
         message
            << "SyncPntListBase::wait_for_announcement():"
            << __LINE__
            << ": Sync-point announced: " << sp->to_string().c_str() << THLA_ENDL;
         send_hs( stderr, (char *)message.str().c_str() );
      }
   }
   return;
}

void SyncPntListBase::wait_for_all_announcements(
   Federate *fed_ptr )
{
   vector< SyncPnt * >::const_iterator i;

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      send_hs( stdout, "SyncPntListBase::wait_for_all_registrations():%d Waiting...%c",
               __LINE__, THLA_NEWLINE );
      this->print_sync_pnts();
   }

   // Iterate through the synchronization point list.
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      if ( sp != NULL ) {
         // Wait for the synchronization point announcement.
         sp->wait_for_announce( fed_ptr );
      }
   }

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      this->print_sync_pnts();
   }

   return;
}

void SyncPntListBase::announce_sync_point(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   wstring const &                   label,
   RTI1516_USERDATA const &          user_supplied_tag )
{

   // Check to see if the synchronization point is known?
   if ( this->contains( label ) ) {

      // Mark initialization sync-point as existing/announced.
      if ( this->mark_announced( label ) ) {
         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            send_hs( stdout, "SyncPntListBase::announce_sync_point():%d Synchronization point announced:'%ls'%c",
                     __LINE__, label.c_str(), THLA_NEWLINE );
         }
      }

   } // By default, mark an unrecognized synchronization point is achieved.
   else {

      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         send_hs( stdout, "SyncPntListBase::announce_sync_point():%d Unrecognized synchronization point:'%ls', which will be achieved.%c",
                  __LINE__, label.c_str(), THLA_NEWLINE );
      }

      // Unknown synchronization point so achieve it but don't wait for the
      // federation to be synchronized on it.
      this->achieve_sync_pnt( rti_ambassador, label );
   }
   return;
}

bool SyncPntListBase::achieve_sync_pnt(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   wstring const &                   label )
{
   lock_read_only();
   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {
            // Check to see if the sync-point is already achieved.
            if ( sp->is_achieved() ) {
               unlock_read_only();
               return false;
            }
            bool returnValue = this->achieve_sync_pnt( rti_ambassador, sp );
            unlock_read_only();
            return returnValue;
         }
      }
   }
   unlock_read_only();
   return false;
}

bool SyncPntListBase::achieve_sync_pnt(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   SyncPnt *                         sync_pnt )
{
   sync_pnt->set_state( SYNC_PNT_STATE_ACHIEVED );
   try {
      rti_ambassador.synchronizationPointAchieved( sync_pnt->get_label() );
   } catch ( SynchronizationPointLabelNotAnnounced &e ) {
      unlock_read_only();
      throw; // Rethrow the exception.
   } catch ( FederateNotExecutionMember &e ) {
      unlock_read_only();
      throw; // Rethrow the exception.
   } catch ( SaveInProgress &e ) {
      unlock_read_only();
      throw; // Rethrow the exception.
   } catch ( RestoreInProgress &e ) {
      unlock_read_only();
      throw; // Rethrow the exception.
   } catch ( NotConnected &e ) {
      unlock_read_only();
      throw; // Rethrow the exception.
   } catch ( RTIinternalError &e ) {
      unlock_read_only();
      throw; // Rethrow the exception.
   }
   return true;
}

void SyncPntListBase::wait_for_list_synchronization(
   Federate *federate )
{
   // Iterate through this SyncPntList's synchronization point list.
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      // Wait for a synchronization point if it is not already achieved.
      if ( ( sp != NULL ) && sp->is_valid() && !sp->is_achieved() ) {

         unsigned int sleep_micros = 1000;
         unsigned int wait_count   = 0;
         unsigned int wait_check   = 10000000 / sleep_micros; // Number of wait cycles for 10 seconds

         // Wait for the federation to synchronized on the sync-point.
         while ( !sp->is_achieved() ) {

            // Always check to see is a shutdown was received.
            federate->check_for_shutdown_with_termination();

            // Pause and release the processor for short sleep value.
            usleep( sleep_micros );

            // Periodically check to make sure the federate is still part of
            // the federation exectuion.
            if ( ( !sp->is_achieved() ) && ( ( ++wait_count % wait_check ) == 0 ) ) {
               wait_count = 0;
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "SyncPntListBase::wait_for_all_sync_pnts_synchronization:" << __LINE__
                         << " Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!"
                         << THLA_ENDL;
                  send_hs( stderr, (char *)errmsg.str().c_str() );
                  exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
               }
            }
         }
      }

      // Now that the sync-point is achieved, reset the state to EXISTS.
      sp->set_state( SYNC_PNT_STATE_EXISTS );
   }

   return;
}

void SyncPntListBase::achieve_and_wait_for_synchronization(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
   Federate *                        federate,
   std::wstring const &              label ) throw( RTI1516_NAMESPACE::SynchronizationPointLabelNotAnnounced,
                                      RTI1516_NAMESPACE::FederateNotExecutionMember,
                                      RTI1516_NAMESPACE::SaveInProgress,
                                      RTI1516_NAMESPACE::RestoreInProgress,
                                      RTI1516_NAMESPACE::NotConnected,
                                      RTI1516_NAMESPACE::RTIinternalError )
{
   string        name;
   ostringstream errmsg;

   // Check for the synchronization point by label.
   SyncPnt *sp = this->get_sync_pnt( label );

   // If the pointer is not NULL then we found it.
   if ( sp != NULL ) {

      // If the synchronization point is announced, then achieve it.
      if ( sp->is_announced() ) {

         // Achieve the synchronization point.
         sp->achieve_sync_point( rti_ambassador );

      } else if ( sp->is_achieved() ) {

         // If the synchronization point is already achieved then print out
         // a message and move on to waiting for synchronization.
         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            StringUtilities::to_string( name, sp->get_label() );
            errmsg << "SyncPntListBase::achieve_and_wait_for_synchronization():"
                   << __LINE__
                   << " Synchronization-Point '" << name
                   << "' has already been achieved with the RTI!";
            send_hs( stderr, (char *)errmsg.str().c_str() );
         }

      } else if ( sp->is_synchronized() ) {

         // If the synchronization point is already synchronized, then print
         // out a message and return.
         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            StringUtilities::to_string( name, sp->get_label() );
            errmsg << "SyncPntListBase::achieve_and_wait_for_synchronization():"
                   << __LINE__
                   << " Synchronization-Point '" << name
                   << "' has already been synchronized with the RTI!";
            send_hs( stderr, (char *)errmsg.str().c_str() );
         }
         return;

      } else {

         // Okay, something is wrong here.  Print a message and exit.
         StringUtilities::to_string( name, sp->get_label() );
         errmsg << "SyncPntListBase::achieve_and_wait_for_synchronization():"
                << __LINE__
                << " Synchronization-Point '" << name
                << "' has not been announced with the RTI!";
         send_hs( stderr, (char *)errmsg.str().c_str() );
         exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
         return;
      }

      // If achieved and not synchronized, then synchronize with the federation.
      if ( sp->is_achieved() && !sp->is_synchronized() ) {
         sp->wait_for_synchronization( federate );
         return;
      }

   } else {

      // Okay, we did not find the synchronization point.  Print out an
      // error message and then exit.
      StringUtilities::to_string( name, label );
      errmsg << "SyncPntListBase::achieve_and_wait_for_synchronization():"
             << __LINE__
             << " Synchronization-Point '" << name
             << "' not found!";
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
      return;
   }

   return;
}

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::achieve_all_sync_pnts(
   RTI1516_NAMESPACE::RTIambassador &rti_ambassador ) throw( RTI1516_NAMESPACE::SynchronizationPointLabelNotAnnounced,
                                                             RTI1516_NAMESPACE::FederateNotExecutionMember,
                                                             RTI1516_NAMESPACE::SaveInProgress,
                                                             RTI1516_NAMESPACE::RestoreInProgress,
                                                             RTI1516_NAMESPACE::NotConnected,
                                                             RTI1516_NAMESPACE::RTIinternalError )
{
   bool wasAcknowledged = false;

   // Iterate through ALL the synchronization points.
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      if ( ( sp != NULL ) && sp->is_valid() && !sp->is_achieved() ) {
         if ( this->achieve_sync_pnt( rti_ambassador, sp ) ) {
            wasAcknowledged = true;
         }
      }
   }

   return ( wasAcknowledged );
}

SyncPntStateEnum SyncPntListBase::get_sync_pnt_state(
   wstring const &label )
{
   lock_read_write();
   if ( !sync_point_list.empty() ) {

      // Iterate through the sync-point list.
      vector< SyncPnt * >::iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );

         // Check if this is the sync-point that we're looking for.
         if ( ( sp != NULL ) && ( label.compare( sp->get_label() ) == 0 ) ) {

            // This is the sync-point we're looking for.
            sync_point_list.end();
            unlock_read_write();
            return sp->get_state();
         }
      }
   }
   unlock_read_write();

   // Hmmm. . . We did not find the sync-point.
   return SYNC_PNT_STATE_ERROR;
}

bool SyncPntListBase::is_sync_pnt_announced(
   wstring const &label )
{

   SyncPntStateEnum state = get_sync_pnt_state( label );
   if ( state == SYNC_PNT_STATE_ANNOUNCED ) {
      return true;
   }
   return false;
}

bool SyncPntListBase::clear_sync_pnt(
   wstring const &label )
{
   lock_read_write();
   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL ) && ( sp->is_achieved() ) && ( label.compare( sp->get_label() ) == 0 ) ) {

            // Extension class dependent code would go here.

            sync_point_list.erase( i );
            delete sp;
            i = sync_point_list.end();

            unlock_read_write();
            return true;
         }
      }
   }
   unlock_read_write();
   return false;
}

void SyncPntListBase::reset()
{
   lock_read_write();
   while ( !sync_point_list.empty() ) {
      if ( *sync_point_list.begin() != NULL ) {
         delete ( *sync_point_list.begin() );
         sync_point_list.erase( sync_point_list.begin() );
      }
   }
   sync_point_list.clear();
   unlock_read_write();
}

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::contains(
   wstring const &label )
{
   lock_read_only();
   if ( !sync_point_list.empty() ) {
      vector< SyncPnt * >::const_iterator i;
      for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
         SyncPnt *sp = ( *i );
         if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {
            unlock_read_only();
            return true;
         }
      }
   }
   unlock_read_only();
   return false;
}

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::mark_registered(
   wstring const &label )
{
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {
         sp->set_state( SYNC_PNT_STATE_REGISTERED );
         return true;
      }
   }
   return false;
}

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::mark_announced(
   wstring const &label )
{
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {
         sp->set_state( SYNC_PNT_STATE_ANNOUNCED );
         return true;
      }
   }
   return false;
}

/*!
 * @job_class{initialization}
 */
bool SyncPntListBase::mark_synchronized(
   wstring const &label )
{
   vector< SyncPnt * >::const_iterator i;
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );

      // Mark the synchronization point at achieved which indicates the
      // federation is synchronized on the synchronization point.
      if ( ( sp != NULL ) && ( sp->get_label().compare( label ) == 0 ) ) {
         sp->set_state( SYNC_PNT_STATE_SYNCHRONIZED );
         return true;
      }
   }
   return false;
}

void SyncPntListBase::lock_read_only()
{
   while ( write_locks > 0 ) {
      RELEASE_1();
   }
   read_locks++;
}

void SyncPntListBase::lock_read_write()
{
   while ( ( write_locks + read_locks ) > 0 ) {
      RELEASE_1();
   }
   write_locks++;
}

void SyncPntListBase::unlock_read_only()
{
   read_locks--;
}

void SyncPntListBase::unlock_read_write()
{
   write_locks--;
}

wstring SyncPntListBase::to_string()
{
   wstring result;

   result = L"Sync Points\n  state: ";

   // Extension class dependent code would go here.

   vector< SyncPnt * >::const_iterator i;
   lock_read_only();
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      SyncPnt *sp = ( *i );
      if ( sp != NULL ) {
         result += L"  " + sp->to_string() + L"\n";
      }
   }
   result += L"\n";
   unlock_read_only();
   return result;
}

void SyncPntListBase::convert_sync_pts(
   LoggableSyncPnt *sync_points )
{
   int                                 loop = 0;
   vector< SyncPnt * >::const_iterator i;

   lock_read_only();
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      ( *i )->convert( sync_points[loop++] );
   }

   unlock_read_only();
}

void SyncPntListBase::print_sync_pnts()
{
   vector< SyncPnt * >::const_iterator i;
   string                              sync_point_label;
   wstring                             spwl;

   ostringstream msg;
   msg << "SyncPntListBase::print_sync_pnts():" << __LINE__ << endl
       << "#############################" << endl
       << "Sync Point Dump: " << sync_point_list.size() << endl;
   lock_read_only();
   for ( i = sync_point_list.begin(); i != sync_point_list.end(); ++i ) {
      spwl = ( *i )->to_string();
      sync_point_label.assign( spwl.begin(), spwl.end() );
      msg << sync_point_label << endl;
   }
   msg << "#############################" << endl;
   send_hs( stdout, (char *)msg.str().c_str() );

   unlock_read_only();
}
