/*!
@file TrickHLA/SyncPnt.cpp
@ingroup TrickHLA
@brief This class provides a sync-point implementation for storing and managing
TrickHLA synchronization points.

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
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{SyncPnt.cpp}
@trick_link_dependency{Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA JSC ER7, TrickHLA, Jan 2019, --, Create from old TrickHLASyncPtsBase class.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

//System includes.

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA includes.
#include "TrickHLA/Federate.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/SyncPnt.hh"
#include "TrickHLA/Utilities.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPnt::SyncPnt()
   : label( L"" ),
     state( SYNC_PT_STATE_EXISTS )
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPnt::SyncPnt(
   std::wstring const &l )
   : label( l ),
     state( SYNC_PT_STATE_EXISTS )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SyncPnt::~SyncPnt()
{
   return;
}

void SyncPnt::register_sync_point(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Register the sync-point label.
   try {
      RTI_amb.registerFederationSynchronizationPoint( this->label,
                                                      RTI1516_USERDATA( 0, 0 ) );

      // Mark the sync-point as registered.
      this->set_state( SYNC_PT_STATE_REGISTERED );

   } catch ( RTI1516_EXCEPTION &e ) {

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      send_hs( stderr, "SyncPnt::register_sync_point():%d Failed to register '%ls' synchronization point with RTI!%c",
               __LINE__, this->label.c_str(), THLA_NEWLINE );
      exit( 0 );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Return now that we registered the sync-point.
   return;
}

void SyncPnt::register_sync_point(
   RTI1516_NAMESPACE::RTIambassador &          RTI_amb,
   RTI1516_NAMESPACE::FederateHandleSet const &federate_handle_set )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Register the sync-point label.
   try {
      RTI_amb.registerFederationSynchronizationPoint( this->label,
                                                      RTI1516_USERDATA( 0, 0 ),
                                                      federate_handle_set );

      // Mark the sync-point as registered.
      this->set_state( SYNC_PT_STATE_REGISTERED );

   } catch ( RTI1516_EXCEPTION &e ) {

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      send_hs( stderr, "SyncPnt::register_sync_point():%d Failed to register '%ls' synchronization point with RTI!%c",
               __LINE__, this->label.c_str(), THLA_NEWLINE );
      exit( 0 );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Return now that we registered the sync-point.
   return;
}

bool SyncPnt::wait_for_announce(
   Federate *federate )
{
   // The sync-point state must be SYNC_PT_STATE_REGISTERED.
   if ( !this->exists() && !this->is_registered() && !this->is_announced() ) {
      string sp_status;
      StringUtilities::to_string( sp_status, this->to_wstring() );

      ostringstream errmsg;
      errmsg << "SyncPnt::wait_for_announce():" << __LINE__
             << " Bad sync-point state for sync-point!"
             << " The sync-point state is: " << sp_status << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   SleepTimeout sleep_timer;

   // Wait for the federation to synchronize on the sync-point.
   while ( !this->is_announced() ) {

      // Always check to see is a shutdown was received.
      federate->check_for_shutdown_with_termination();

      (void)sleep_timer.sleep();

      // Check to make sure we're still a member of the federation execution.
      if ( !this->is_announced() && sleep_timer.timeout() ) {
         sleep_timer.reset();
         if ( !federate->is_execution_member() ) {
            ostringstream errmsg;
            errmsg << "SyncPnt::wait_for_announce():" << __LINE__
                   << " Unexpectedly the Federate is no longer an execution"
                   << " member. This means we are either not connected to the"
                   << " RTI or we are no longer joined to the federation"
                   << " execution because someone forced our resignation at"
                   << " the Central RTI Component (CRC) level!" << THLA_ENDL;
            send_hs( stderr, (char *)errmsg.str().c_str() );
            exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            return false;
         }
      }
   }

   // Mark the sync-point as announced.
   this->set_state( SYNC_PT_STATE_ANNOUNCED );

   return true;
}

void SyncPnt::achieve_sync_point(
   RTI1516_NAMESPACE::RTIambassador &RTI_amb )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Achieve this synchronization point.
   try {
      RTI_amb.synchronizationPointAchieved( this->label );
   } catch ( SynchronizationPointLabelNotAnnounced &e ) {
      throw; // Rethrow the exception.
   } catch ( FederateNotExecutionMember &e ) {
      throw; // Rethrow the exception.
   } catch ( SaveInProgress &e ) {
      throw; // Rethrow the exception.
   } catch ( RestoreInProgress &e ) {
      throw; // Rethrow the exception.
   } catch ( NotConnected &e ) {
      throw; // Rethrow the exception.
   } catch ( RTIinternalError &e ) {
      throw; // Rethrow the exception.
   }

   // Mark the sync-point as achieved.
   this->set_state( SYNC_PT_STATE_ACHIEVED );

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

bool SyncPnt::wait_for_synchronization(
   Federate *federate )
{
   SleepTimeout sleep_timer;

   // Wait for the federation to synchronize on the sync-point.
   while ( !this->is_synchronized() ) {

      // Always check to see is a shutdown was received.
      federate->check_for_shutdown_with_termination();

      (void)sleep_timer.sleep();

      // Check to make sure we're still a member of the federation execution.
      if ( !this->is_synchronized() && sleep_timer.timeout() ) {
         sleep_timer.reset();
         if ( !federate->is_execution_member() ) {
            ostringstream errmsg;
            errmsg << "SyncPnt::wait_for_synchronization():" << __LINE__
                   << " Unexpectedly the Federate is no longer an execution"
                   << " member. This means we are either not connected to the"
                   << " RTI or we are no longer joined to the federation"
                   << " execution because someone forced our resignation at"
                   << " the Central RTI Component (CRC) level!" << THLA_ENDL;
            send_hs( stderr, (char *)errmsg.str().c_str() );
            exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            return false;
         }
      }
   }

   // Now that the federation is synchronized on the synchronization point,
   // return to SYNC_PT_STATE_EXISTS state.
   this->set_state( SYNC_PT_STATE_EXISTS );

   return true;
}

bool SyncPnt::is_valid()
{
   return ( ( this->state == SYNC_PT_STATE_EXISTS )
            || ( this->state == SYNC_PT_STATE_REGISTERED )
            || ( this->state == SYNC_PT_STATE_ANNOUNCED )
            || ( this->state == SYNC_PT_STATE_ACHIEVED )
            || ( this->state == SYNC_PT_STATE_SYNCHRONIZED ) );
}

bool SyncPnt::exists()
{
   return ( this->state == SYNC_PT_STATE_EXISTS );
}

bool SyncPnt::is_registered()
{
   return ( this->state == SYNC_PT_STATE_REGISTERED );
}

bool SyncPnt::is_announced()
{
   return ( this->state == SYNC_PT_STATE_ANNOUNCED );
}

bool SyncPnt::is_achieved()
{
   return ( this->state == SYNC_PT_STATE_ACHIEVED );
}

bool SyncPnt::is_synchronized()
{
   return ( this->state == SYNC_PT_STATE_SYNCHRONIZED );
}

bool SyncPnt::is_error()
{
   return ( ( this->state != SYNC_PT_STATE_EXISTS )
            && ( this->state != SYNC_PT_STATE_REGISTERED )
            && ( this->state != SYNC_PT_STATE_ANNOUNCED )
            && ( this->state != SYNC_PT_STATE_ACHIEVED )
            && ( this->state != SYNC_PT_STATE_SYNCHRONIZED ) );
}

std::wstring SyncPnt::to_wstring()
{
   wstring result = L"[" + label + L"] -- ";
   switch ( this->state ) {

      case SYNC_PT_STATE_ERROR:
         result += L"SYNC_PT_STATE_ERROR";
         break;

      case SYNC_PT_STATE_EXISTS:
         result += L"SYNC_PT_STATE_EXISTS";
         break;

      case SYNC_PT_STATE_REGISTERED:
         result += L"SYNC_PT_STATE_REGISTERED";
         break;

      case SYNC_PT_STATE_ANNOUNCED:
         result += L"SYNC_PT_STATE_ANNOUNCED";
         break;

      case SYNC_PT_STATE_ACHIEVED:
         result += L"SYNC_PT_STATE_ACHIEVED";
         break;

      case SYNC_PT_STATE_SYNCHRONIZED:
         result += L"SYNC_PT_STATE_SYNCHRONIZED";
         break;

      default:
         result += L"SYNC_PT_STATE_UNKNOWN";
   }
   return result;
}

void SyncPnt::convert( LoggableSyncPnt &log_sync_pnt )
{
   log_sync_pnt.label = StringUtilities::ip_strdup_wstring( this->label );
   log_sync_pnt.state = this->state;
   return;
}
