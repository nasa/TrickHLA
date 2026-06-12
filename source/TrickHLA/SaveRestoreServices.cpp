/*!
@file TrickHLA/SaveRestoreServices.cpp
@ingroup TrickHLA
@brief This class provides basic HLA save and restore services for a federate.

@copyright Copyright 2026 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{SaveRestoreServices.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{utils/SleepTimeout.cpp}
@trick_link_dependency{utils/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2026, --, Refactor HLA Save and Restore services.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, May 2026, --, Reformulation for SaveRestore state machine architecture.}
@revs_end

*/

// System include files.
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

// Trick includes.
#include "trick/CheckPointRestart_c_intf.hh"
#include "trick/command_line_protos.h"
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/KnownFederate.hh"
#include "TrickHLA/SaveRestoreServices.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Time.hh"
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
#include "RTI/RTIambassadorFactory.h"
#include "RTI/Typedefs.h"
#include "RTI/time/HLAinteger64Time.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif // IEEE_1516_2025

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

//-----------------------------------------------------------
// Save Restore process state enumeration support functions.
//-----------------------------------------------------------
std::string TrickHLA::to_string( THLASaveProcessEnum save_state )
{
   switch ( save_state ) {
      case THLASaveProcessEnum::SAVE_NONE:
         return ( "SAVE_NONE" );
         break;
      case THLASaveProcessEnum::SAVE_INITIATED:
         return ( "SAVE_INITIATED" );
         break;
      case THLASaveProcessEnum::SAVE_REQUESTED:
         return ( "SAVE_REQUESTED" );
         break;
      case THLASaveProcessEnum::SAVE_IN_PROGRESS:
         return ( "SAVE_IN_PROGRESS" );
         break;
      case THLASaveProcessEnum::SAVE_COMPLETE:
         return ( "SAVE_COMPLETE" );
         break;
      case THLASaveProcessEnum::SAVE_FAILED:
         return ( "SAVE_FAILED" );
         break;
      case THLASaveProcessEnum::SAVE_UNSUPPORTED:
         return ( "SAVE_UNSUPPORTED" );
         break;
      default:
         return ( "SAVE_UNKNOWN" );
   }
   return ( "SAVE_UNKNOWN" );
}

std::string TrickHLA::to_string( THLARestoreProcessEnum restore_state )
{
   switch ( restore_state ) {
      case THLARestoreProcessEnum::RESTORE_NONE:
         return ( "RESTORE_NONE" );
         break;
      case THLARestoreProcessEnum::RESTORE_ACTIVATE:
         return ( "RESTORE_ACTIVATE" );
         break;
      case THLARestoreProcessEnum::RESTORE_REQUESTED:
         return ( "RESTORE_REQUESTED" );
         break;
      case THLARestoreProcessEnum::RESTORE_REQUEST_FAILED:
         return ( "RESTORE_REQUEST_FAILED" );
         break;
      case THLARestoreProcessEnum::RESTORE_REQUEST_SUCCEEDED:
         return ( "RESTORE_REQUEST_SUCCEEDED" );
         break;
      case THLARestoreProcessEnum::RESTORE_BEGUN:
         return ( "RESTORE_BEGUN" );
         break;
      case THLARestoreProcessEnum::RESTORE_IN_PROGRESS:
         return ( "RESTORE_IN_PROGRESS" );
         break;
      case THLARestoreProcessEnum::RESTORE_COMPLETE:
         return ( "RESTORE_COMPLETE" );
         break;
      case THLARestoreProcessEnum::RESTORE_FAILED:
         return ( "RESTORE_FAILED" );
         break;
      case THLARestoreProcessEnum::RESTORE_UNSUPPORTED:
         return ( "RESTORE_UNSUPPORTED" );
         break;
      default:
         return ( "RESTORE_UNKNOWN" );
   }
   return ( "RESTORE_UNKNOWN" );
}

/*!
 * @details NOTE: In most cases, we would allocate and set default names in
 * the constructor. However, since we want this class to be Input Processor
 * friendly, we cannot do that here since the Input Processor may not have
 * been initialized yet. So, we have to set the name information to NULL and
 * then allocate and set the defaults in the initialization job if not
 * already set in the input stream.
 *
 * @job_class{initialization}
 */
SaveRestoreServices::SaveRestoreServices( Federate &fed )
   : federate( &fed ),
     object_service( NULL ),
     time_management_service( NULL ),
     execution_control( NULL ),
     running_feds_count_at_time_of_restore( 0 ),
     joined_federates_file_name( "" ),
     HLA_save_directory( "" ),
     copy_run_directory( false ),
     unfreeze_after_save( false ),
     support_tcp_checkpoint( false ),
     save_state( THLASaveProcessEnum::SAVE_UNSUPPORTED ),
     save_label( L"" ),
     save_status_request_complete( false ),
     restore_state( THLARestoreProcessEnum::RESTORE_UNSUPPORTED ),
     restore_label( L"" ),
     restore_status_response_complete( false ),
     restore_status_process_response( false ),
     // FIXME:
     // Possibly deprecated after this.
     // Save variables.
     // Restore variables.
     restore_federation( false ),
     restore_file_name(),
     restore_name( L"" ),
     prev_restore_process( THLARestoreProcessEnum::RESTORE_UNSUPPORTED ),
     initiate_restore_flag( false ),
     restore_in_progress( false ),
     // restore_failed( false ),
     restore_is_imminent( false ),
     announce_restore( false ),
     restore_label_generated( false ),
     // restore_begun( false ),
     // restore_request_complete( false ),
     restore_completed( false ),
     federation_restore_failed_callback_complete( false ),
     federate_has_been_restarted( false ),
     start_to_restore( false ),
     restart_flag( false ),
     restart_cfg_flag( false )
{
   // Set the TrickHLA::Manager instance reference.
   this->object_service = fed.get_object_service();

   // Register the Time Management Services instance.
   time_management_service = fed.get_time_management_service();

   return;
}

/*!
 * @details Free up the Trick allocated memory associated with the attributes
 * of this class.
 * @job_class{shutdown}
 */
SaveRestoreServices::~SaveRestoreServices()
{
   // Clear the pending Save set.
   pending_save_queue.clear();

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::set_save_label( wstring const &label )
{
   // FIXME: May need some protections here.

   // Copy the label.
   save_label = label;

   return;
}

//----------------------------------------------------------------------------
// General SavRestoreService support functions.
//----------------------------------------------------------------------------

/*!
 *  @job_class{initialization}
 */
bool SaveRestoreServices::set_HLA_save_directory( std::string const &path )
{
   // If the save directory is not specified, set it to the current RUN directory
   if ( path.empty() ) {

      // Get the Trick provided RUN directory path information.
      string run_dir = command_line_args_get_output_dir();
      string def_dir = command_line_args_get_default_dir();

      // Build an absolute path to the RUN directory by combining default_dir
      // and run_dir from the EXECUTIVE.
      this->HLA_save_directory = def_dir + "/" + run_dir;

   } else {
      this->HLA_save_directory = path;
   }

   // Check if path is valid and return status.
   return ( this->check_HLA_save_directory() );
}

/*!
 *  @job_class{scheduled}
 */
bool SaveRestoreServices::check_HLA_save_directory()
{
   struct stat info;

   // Check for the existence of the path and that it is a directory.
   if ( stat( this->HLA_save_directory.c_str(), &info ) != 0 ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream msg;
         msg << "SaveRestoreServices::check_HLA_save_directory():" << __LINE__
             << " ERROR: Save directory path \'" << this->HLA_save_directory
             << "\' does NOT exist!";
         message_publish( MSG_ERROR, "%s\n", msg.str().c_str() );
      }

      return ( false );

   } else if ( ( info.st_mode & S_IFDIR ) == 0 ) { // NOLINT(misc-include-cleaner)

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream msg;
         msg << "SaveRestoreServices::check_HLA_save_directory():" << __LINE__
             << " ERROR: Save directory path \'" << this->HLA_save_directory
             << "\' exists but is NOT a directory!";
         message_publish( MSG_ERROR, "%s\n", msg.str().c_str() );
      }

      return ( false );
   }

   return ( true );
}

//----------------------------------------------------------------------------
// SaveRestoreService Save functions.
//----------------------------------------------------------------------------

/*!
 *  @job_class{scheduled}
 */
bool SaveRestoreServices::set_save_state( THLASaveProcessEnum state )
{
   // Check to make sure that Save and Restore is supported for this federate.
   if ( ( !execution_control->is_save_and_restore_supported() )
        && ( state != THLASaveProcessEnum::SAVE_UNSUPPORTED ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING,
                          "SaveRestoreServices::set_save_state():%d : HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }

      // Make sure that the service state reflects the unsupported state.
      this->save_state = THLASaveProcessEnum::SAVE_UNSUPPORTED;

      return ( false );
   }

   // Set the Save state.
   save_state = state;

   return ( true );
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::set_save_time(
   Int64Time const &time )
{
   // Make sure that we are in an appropriate state to set the Save time.
   if ( ( this->save_state != THLASaveProcessEnum::SAVE_NONE )
        && ( this->save_state != THLASaveProcessEnum::SAVE_REQUESTED )
        && ( this->save_state != THLASaveProcessEnum::SAVE_UNSUPPORTED ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::set_save_time():%d : Save already in progress: \'%s\'!\n",
                          __LINE__, TrickHLA::to_string( save_state ).c_str() );
      }
      return;
   }

   // Check to make sure the time hasn't already passed.
   Int64Time granted_time = time_management_service->get_granted_time();
   if ( time < granted_time ) {

      std::string label_str;
      StringUtilities::to_string( label_str, save_label );

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream msg;
         msg << "SaveRestoreServices::set_save_time():" << __LINE__
             << " : WARNING: Save time for label \'" << label_str
             << "\' in the past!";
         msg << " Save time is " << time.get_base_time()
             << " but Granted time is " << granted_time.get_base_time() << std::endl;
         message_publish( MSG_WARNING, "%s\n", msg.str().c_str() );
      }

      return;
   }

   // Set the Save time.
   save_time = time;

   return;
}

/*!
 * @details Trigger federation save, at current time.\n
 * NOTE: These routines do not coordinate a federation save via interactions
 * so make these internal routines so that the user does not accidentally call
 * them and mess things up.
 */
void SaveRestoreServices::save_request(
   wstring const &label )
{
   // If Federation SaveRestore is not supported then return without action.
   if ( save_state == THLASaveProcessEnum::SAVE_UNSUPPORTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d : HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return;
   }

   // Check the Federation Save state to ensure that a Save is applicable .
   if ( ( save_state != THLASaveProcessEnum::SAVE_NONE )
        && ( save_state != THLASaveProcessEnum::SAVE_UNSUPPORTED ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d : Save already in progress: \'%s\'!\n",
                          __LINE__, TrickHLA::to_string( save_state ).c_str() );
      }
      return;
   }

   // Check the Save label.
   if ( label.empty() ) {
      // If no label is passed in, then we must have a label already set.
      if ( this->save_label.empty() ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::save_request():" << __LINE__
                << " ERROR: No Save label set!" << endl;
         DebugHandler::terminate( errmsg.str() );
      }
      return;
   } else {
      this->save_label = label;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Make the RTI ambassador call to request a Federation Save.
   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, this->save_label );
         message_publish( MSG_NORMAL, "SaveRestoreServices::save_request():%d save_label: \'%s\'\n",
                          __LINE__, label_str.c_str() );
      }

      // Make the requestFederationSave call to the RTI Ambassador.
      federate->get_RTI_ambassador()->requestFederationSave( this->save_label );

   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Set the current Save state.
   save_state = THLASaveProcessEnum::SAVE_INITIATED;

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::save( wstring const &label )
{
   std::string label_str;
   std::string checkpoint_file_name;

   // If Federation SaveRestore is not supported then return without action.
   if ( save_state == THLASaveProcessEnum::SAVE_UNSUPPORTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::save():%d HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return;
   }

   // Do a little sanity checking.
   if ( save_state != THLASaveProcessEnum::SAVE_REQUESTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::save():" << __LINE__
                << " ERROR: Save state mismatch: "
                << TrickHLA::to_string( save_state ) << "!" << std::endl;
         message_publish( MSG_WARNING, "%s\n",
                          __LINE__, errmsg.str().c_str() );
      }
      return;
   }

   // Check the Save label.
   if ( label.empty() ) {
      // If no label is passed in, then we must have a label already set.
      if ( this->save_label.empty() ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::save():" << __LINE__
                << " ERROR: No Save label set!" << endl;
         DebugHandler::terminate( errmsg.str() );
      }
   } else {
      this->save_label = label;
   }

   // Convert the Save label to string.
   StringUtilities::to_string( label_str, save_label );

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;
   try {

      // Tell the federation that this federate has begun the Save.
      federate->get_RTI_ambassador()->federateSaveBegun();

   } catch ( SaveNotInitiated const &e ) {
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: SaveNotInitiated\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "ExecutionControlBase::setup_checkpoint():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Write out the list of currently joined federates.  We do this so that we
   // can enforce that only these federates exist when we restore.
   this->write_joined_federates_to_file( this->save_label );

   // Tell the object_service to setup the checkpoint data structures.
   object_service->convert_data_before_checkpoint();

   // Save any synchronization points.
   this->convert_sync_pts();

   // Mark that the Save state is SAVE_IN_PROGRESS
   save_state = THLASaveProcessEnum::SAVE_IN_PROGRESS;

   // Map the save label to the associated Trick checkpoint file names.
   checkpoint_file_name = execution_control->map_save_label_to_checkpoint_file_name( this->save_label );

   // Save the federate state using the Trick checkpoint mechanism.
   checkpoint( checkpoint_file_name.c_str() );

   //
   // Let the Federation know that our Save process is finished.
   //
   try {

      // Make the call to the RTI Ambassador to mark our Save as complete.
      federate->get_RTI_ambassador()->federateSaveComplete();

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::save():%d Federate Save Completed.\n",
                          __LINE__ );
      }

   } catch ( FederateHasNotBegunSave const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save():%d EXCEPTION: FederateHasNotBegunSave\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save():%d EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "SaveRestoreServices::post_checkpoint():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // FIXME: We probably need to better handle the Save state for exceptions.

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

/*!
 *  @job_class{scheduled}
 */
bool SaveRestoreServices::save_in_progress_check()
{
   // If Federation SaveRestore is not supported then return without action.
   if ( save_state == THLASaveProcessEnum::SAVE_UNSUPPORTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::save_in_progress_check():%d HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return ( false );
   }

   if ( save_state == THLASaveProcessEnum::SAVE_IN_PROGRESS ) {

      std::string label_str;
      StringUtilities::to_string( label_str, save_label );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING,
                          "SaveRestoreServices::save_in_progress_check():%d HLA Save for label \'%s\' in progress!\n",
                          __LINE__, label_str.c_str() );
      }
      return ( true );
   }

   return ( false );
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::save_succeded()
{
   std::string label_str;

   // If Federation SaveRestore is not supported then return without action.
   if ( save_state == THLASaveProcessEnum::SAVE_UNSUPPORTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::save_succeded():%d HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return;
   }

   // Convert the Save label to string.
   StringUtilities::to_string( label_str, save_label );

   // Do a little sanity checking.
   if ( save_state != THLASaveProcessEnum::SAVE_COMPLETE ) {
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::save_succeded():" << __LINE__
             << " ERROR: Save state mismatch: "
             << TrickHLA::to_string( save_state ) << "!" << std::endl;
      DebugHandler::terminate( errmsg.str() );
   }

   // Restore the base Save state.
   this->save_label = L"";
   this->save_state = THLASaveProcessEnum::SAVE_NONE;

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::save_failed()
{
   std::string label_str;

   // If Federation SaveRestore is not supported then return without action.
   if ( save_state == THLASaveProcessEnum::SAVE_UNSUPPORTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::save_failed():%d HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return;
   }

   // Convert the Save label to string.
   StringUtilities::to_string( label_str, save_label );

   // Do a little sanity checking.
   if ( save_state != THLASaveProcessEnum::SAVE_FAILED ) {
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::save_failed():" << __LINE__
             << " ERROR: Save state mismatch: "
             << TrickHLA::to_string( save_state ) << "!" << std::endl;
      DebugHandler::terminate( errmsg.str() );
   }

   // Print out an error message.
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      ostringstream msg;
      msg << "SaveRestoreServices::save_failed():" << __LINE__
          << " : Save for label \'" << label_str
          << "\' failed!" << std::endl;
      message_publish( MSG_ERROR, msg.str().c_str() );
   }

   // Restore the base Save state.
   this->save_label = L"";
   this->save_state = THLASaveProcessEnum::SAVE_NONE;

   return;
}

/*!
 *  @job_class{scheduled}
 */
bool SaveRestoreServices::write_joined_federates_to_file(
   wstring const &label )
{
   std::string    file_name;
   std::string    full_file_path;
   std::wofstream file;

   // Check the Save label.
   if ( label.empty() ) {
      // If no label is passed in, then we must have a label already set.
      if ( this->save_label.empty() ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::write_joined_federates_file():" << __LINE__
                << " ERROR: No Save label set!" << endl;
         DebugHandler::terminate( errmsg.str() );
      }
      // Get the joined federates file name from the ExecutionControl service.
      file_name = execution_control->map_save_label_to_federates_file_name( this->save_label );
   } else {
      // Get the joined federates file name from the ExecutionControl service.
      file_name = execution_control->map_save_label_to_federates_file_name( label );
   }

   // Form the full path.
   full_file_path = this->HLA_save_directory + "/" + file_name;

   // Open the joined federates file for writing.
   file.open( full_file_path.c_str(), ios::out ); // flawfinder: ignore

   // Check to make sure the file was successfully opened.
   if ( file.is_open() ) {

      // Start by writing the number of joined federates.
      file << federate->joined_federates_map.size() << std::endl;

      // Write the contents of running_feds into file...
      KnownFederateMap::iterator map_iter;
      for ( map_iter = federate->joined_federates_map.begin();
            map_iter != federate->joined_federates_map.end(); ++map_iter ) {

         // Get the associate joined federate reference.
         KnownFederate const *joined_federate = static_cast< KnownFederate * >( &( map_iter->second ) );

         // Write the federate information out to file.
         file << joined_federate->name << endl;
         file << joined_federate->type << endl;
         file << joined_federate->required << endl;
      }

      // Close the joined federates file.
      file.close();

   } else {

      ostringstream errmsg;
      errmsg << "SaveRestoreServices::write_joined_federates_file():" << __LINE__
             << " ERROR: Failed to open file '" << full_file_path << "' for writing!" << std::endl;
      message_publish( MSG_ERROR, errmsg.str().c_str() );

      return ( false );
   }

   return ( true );
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::print_save_failure_reason(
   SaveFailureReason reason )
{
   // dump the contents...
   ostringstream msg;

   if ( reason == RTI_UNABLE_TO_SAVE ) {
      msg << "SaveRestoreServices::print_save_failure_reason():" << __LINE__
          << " failure reason=\"RTI_UNABLE_TO_SAVE\"" << endl;
   }
   if ( reason == FEDERATE_REPORTED_FAILURE_DURING_SAVE ) {
      msg << "SaveRestoreServices::print_save_failure_reason():" << __LINE__
          << " failure reason=\"FEDERATE_REPORTED_FAILURE_DURING_SAVE\"" << endl;
   }
   if ( reason == FEDERATE_RESIGNED_DURING_SAVE ) {
      msg << "SaveRestoreServices::print_save_failure_reason():" << __LINE__
          << " failure reason=\"FEDERATE_RESIGNED_DURING_SAVE\"" << endl;
   }
   if ( reason == RTI_DETECTED_FAILURE_DURING_SAVE ) {
      msg << "SaveRestoreServices::print_save_failure_reason():" << __LINE__
          << " failure reason=\"=RTI_DETECTED_FAILURE_DURING_SAVE\"" << endl;
   }
   if ( reason == SAVE_TIME_CANNOT_BE_HONORED ) {
      msg << "SaveRestoreServices::print_save_failure_reason():" << __LINE__
          << " failure reason=\"SAVE_TIME_CANNOT_BE_HONORED\"" << endl;
   }
   message_publish( MSG_NORMAL, msg.str().c_str() );

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::request_federation_save_status() // cppcheck-suppress [functionStatic, unmatchedSuppression]
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::request_federation_save_status():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      federate->get_RTI_ambassador()->queryFederationSaveStatus();
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save_status():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save_status():%d EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save_status():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save_status():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

//----------------------------------------------------------------------------
// SaveRestoreService Restore functions.
//----------------------------------------------------------------------------

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::set_restore_label( wstring const &label )
{
   // FIXME: May need some protections here.

   // Copy the label.
   restore_label = label;

   return;
}

bool SaveRestoreServices::read_known_federates_from_file(
   wstring const &label )
{
   std::string    file_name;
   std::string    full_path;
   std::wifstream file; // Note that this is a wide string file stream.
   unsigned int   line_num;
   wstring        num_feds_wstr = L"";
   unsigned int   num_feds      = 0;

   // Check the Save label.
   if ( label.empty() ) {
      // If no label is passed in, then we must have a label already set.
      if ( this->restore_label.empty() ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::read_known_federates_from_file():" << __LINE__
                << " ERROR: No Restore label set!" << endl;
         DebugHandler::terminate( errmsg.str() );
      }
      // Get the joined federates file name from the ExecutionControl service.
      file_name = execution_control->map_save_label_to_federates_file_name( this->restore_label );
   } else {
      // Get the joined federates file name from the ExecutionControl service.
      file_name = execution_control->map_save_label_to_federates_file_name( label );
   }

   // Create the full path to the federates file.
   full_path = this->HLA_save_directory + "/" + file_name + ".feds";

   // Try to open the known federates file for reading.
   file.open( full_path.c_str(), ios::in ); // flawfinder: ignore

   // Check if the file is open for reading.
   if ( !file.is_open() ) {
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
             << " ERROR: Failed to open file '" << full_path << "'!" << endl;
      message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );
      return ( false );
   }

   //
   // We're going to place the data into the known federates list.
   //

   // Clear the know federates list.
   federate->known_federates.clear();

   // Read in the number of known federates from the joined federates Save file.
   if ( std::getline( file, num_feds_wstr ) ) {
      line_num = 1;
      try {
         num_feds = stoi( num_feds_wstr );
      } catch ( std::invalid_argument const &e ) {
         std::wcerr << L"Invalid input: No conversion could be performed." << std::endl;
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
                << " ERROR: Reading number of known federates:'"
                << " Invalid input: No conversion could be performed." << "'!" << endl;
         message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );
         file.close();
         return ( false );
      } catch ( std::out_of_range const &e ) {
         file.close();
         std::wcerr << L"Error: Number is out of range for an int." << std::endl;
         return ( false );
      }
   }

   // Sanity check.  There has to be at least 1 federate.
   if ( num_feds == 0 ) {
      file.close();
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
             << " ERROR: There has to be at least 1 federate.  Read in "
             << num_feds << " from '" << full_path << "'!" << endl;
      message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );
      return ( false );
   }

   // Now read in each federate entry.
   bool          read_error = false;
   unsigned int  fed_count  = 0;
   std::string   fed_name_str;
   wstring       required_wstr;
   KnownFederate known_federate;
   while ( std::getline( file, known_federate.name ) ) {

      // Get the federate name in string form for output.
      StringUtilities::to_string( fed_name_str, known_federate.name );

      // Increment the line count.
      ++line_num;

      // Read in the federate type.
      if ( std::getline( file, known_federate.type ) ) {

         // Increment the line count.
         ++line_num;

      } else {

         // There must have been an error reading the file.
         read_error = true;

         // Let the user know that something went wrong.
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
                << " ERROR: Error reading the type for known federate '"
                << fed_name_str << "' at line " << line_num
                << " from '" << full_path << "'!" << endl;
         message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );

         // Break out of the read while loop.
         break;
      }

      // Read in the federate required flag.
      ++line_num;
      if ( std::getline( file, required_wstr ) ) {

         // Increment the line count.
         ++line_num;

         // Convert to boolean value.
         int bool_val = 1;
         try {

            // Convert the string in the file to an integer.
            bool_val = stoi( required_wstr );

         } catch ( std::invalid_argument const &e ) {

            // Let the user know that something went wrong.
            ostringstream errmsg;
            errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
                   << " ERROR: Error reading if known federate '"
                   << fed_name_str << "' is required at line " << line_num
                   << " from '" << full_path << "'!" << std::endl;
            message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );

         } catch ( std::out_of_range const &e ) {

            // Let the user know that something went wrong.
            std::wcerr << L"Error: Number is out of range for an int." << std::endl;
         }

         // NOTE that a boolean conversion error is NOT a read error.
         // In the case of a conversion error, the federate is required.

         // Convert to boolean values.
         known_federate.required = ( bool_val != 0 ) ? true : false;

      } else {

         // There must have been an error reading the file.
         read_error = true;

         // Let the user know that something went wrong.
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
                << " ERROR: Error reading if known federate '"
                << fed_name_str << "' is required at line " << line_num
                << " from '" << full_path << "'!" << std::endl;
         message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );

         // Break out of the read while loop.
         break;
      }

      // Increment the fed count.
      ++fed_count;

      // Add the known federate to the known federates list.
      federate->known_federates.push_back( known_federate );
   }

   // We're done reading.  So, close the file.
   file.close();

   // Check for error while reading the known federates file.
   if ( read_error ) {

      // Clear the known federates list.
      federate->known_federates.clear();

      // Let the user know that something went wrong.
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
             << " ERROR: Error reading the known federates file '"
             << full_path << "'!" << std::endl;
      message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );

      return ( false );
   }

   // Check that the number of federates read in matches the number written.
   if ( fed_count != num_feds ) {

      // Let the user know that something went wrong.
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
             << " ERROR: Federate file specified " << num_feds
             << " but read in " << fed_count << "!" << std::endl;
      message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );

      // NOTE that we are NOT clearing the known federates file.  We are just
      // informing the user that there was a mismatch between the specified
      // number of federates and the number read.  We also return false to
      // indicate a discrepency.

      return ( false );
   }

   return ( true );
}

/*!
 *  @job_class{scheduled}
 */
bool SaveRestoreServices::set_restore_state( THLARestoreProcessEnum state )
{
   // Check to make sure that Save and Restore is supported for this federate.
   if ( ( !execution_control->is_save_and_restore_supported() )
        && ( state != THLARestoreProcessEnum::RESTORE_UNSUPPORTED ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING,
                          "SaveRestoreServices::set_restore_state():%d : HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }

      // Make sure that the service state reflects the unsupported state.
      this->restore_state = THLARestoreProcessEnum::RESTORE_UNSUPPORTED;

      return ( false );
   }

   // Set the Save state.
   restore_state = state;

   return ( true );
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_request_status()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::restore_request_status():" << __LINE__
                << " ERROR: SaveRestore NOT supported!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      }
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::restore_request_status():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Make the queryFederationRestoreStatus call to the RTI ambassador.
   try {

      // Clear the Federation Restore status response vector.
      restore_status_response.clear();

      // Make the RTI call.
      federate->get_RTI_ambassador()->queryFederationRestoreStatus();

      // Move the Restore process state to indicate that the Restore status has been requested.
      this->restore_state = THLARestoreProcessEnum::RESTORE_REQUEST_STATUS;

   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request_status():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request_status():%d EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request_status():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request_status():%d EXCEPTION: RTIinternalError\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_request_status_check()
{
   bool restore_conflict = false;

   // Just return if HLA save and restore is not supported by the simulation
   // execution control scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::restore_request_status_check():" << __LINE__
                << " ERROR: SaveRestore NOT supported!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Just return if we are not in a Restore process state waiting for a status response.
   if ( restore_state != THLARestoreProcessEnum::RESTORE_REQUEST_STATUS ) {
      return;
   }

   // Check to see if we have received a Restore status response.
   if ( !restore_status_response.empty() ) {

      // Iterate through the response vector to check for ongoing restores.
      for ( FederateRestoreStatus const &status : restore_status_response ) {

         if ( status.status != NO_RESTORE_IN_PROGRESS ) {
            restore_conflict = true;
            break;
         }
      }

   } else {
      // An empty Restore status response means we are still waiting for the
      // FedAmb::federationRestoreStatusResponse callback.
      return;
   }

   // Check for Restore response status conflict.
   if ( restore_conflict ) {
      ostringstream errmsg;
      errmsg << "ExecutionControlBase::restore_request_status_check():" << __LINE__
             << " ERROR: Federation NOT in state to Restore:" << endl;
      errmsg << to_string( restore_status_response ) << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      restore_failed();
   } else {
      restore_state = THLARestoreProcessEnum::RESTORE_STATUS_COMPLETE;
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream msg;
         msg << "SaveRestoreServices::restore_request_status_check():" << __LINE__
             << ": Restore status response complete." << endl;
         message_publish( MSG_WARNING, msg.str().c_str() );
      }
   }

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_request( wstring const &label )
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "ExecutionControlBase::restore_request():" << __LINE__
                << " ERROR: SaveRestore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      }
      return;
   }

   // Just return if we are not in the proper restore state.
   if ( restore_state != THLARestoreProcessEnum::RESTORE_STATUS_COMPLETE ) {
      return;
   }

   // Check the Restore label.
   if ( label.empty() ) {
      // If no label is passed in, then we must have a label already set.
      if ( this->restore_label.empty() ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_request():" << __LINE__
                << " ERROR: No Restore label set!" << endl;
         DebugHandler::terminate( errmsg.str() );
      }
   } else {
      this->restore_label = label;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      string name_str;
      StringUtilities::to_string( name_str, this->restore_label );
      message_publish( MSG_NORMAL, "SaveRestoreServices::restore_request():%d \
restore_process == RESTORE_BEGUN, Telling RTI to request federation \
restore with label '%s'.\n",
                       __LINE__, name_str.c_str() );
   }

   try {

      federate->get_RTI_ambassador()->requestFederationRestore( this->restore_label );
      this->restore_state = THLARestoreProcessEnum::RESTORE_REQUESTED;

      // Save the # of running_feds at the time federation restore is initiated.
      // this way, when the count decreases, we know someone has resigned!
      this->running_feds_count_at_time_of_restore = federate->joined_federates_map.size();

   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
      this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d EXCEPTION: SaveInProgress\n",
                       __LINE__ );
      this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
      this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

std::string SaveRestoreServices::to_string( RTI1516_NAMESPACE::FederateRestoreStatusVector const &response )
{
   ostringstream response_msg;

   // Iterate through the response vector to check for ongoing restores.
   for ( FederateRestoreStatus const &status : response ) {

      // Get the individual federate status string.
      response_msg << to_string( status );

      // Terminate the message string with a new line.
      response_msg << endl;
   }

   return ( response_msg.str() );
}

std::string SaveRestoreServices::to_string( rti1516e::FederateRestoreStatus const &restore_status )
{
   std::ostringstream restore_status_str;

   string id_name;
   StringUtilities::to_string( id_name, restore_status.preRestoreHandle );
   restore_status_str << "\tpre-restore fed_id: " << id_name << endl;
   StringUtilities::to_string( id_name, restore_status.postRestoreHandle );
   restore_status_str << "\tpost-restore fed_id: " << id_name << endl
                      << "\tstatus: ";

   // Print the appropriate status string.
   switch ( restore_status.status ) {

      case NO_RESTORE_IN_PROGRESS:
         restore_status_str << "NO_RESTORE_IN_PROGRESS";
         break;

      case FEDERATE_RESTORE_REQUEST_PENDING:
         restore_status_str << "FEDERATE_RESTORE_REQUEST_PENDING";
         break;

      case FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN:
         restore_status_str << "FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN";
         break;

      case FEDERATE_PREPARED_TO_RESTORE:
         restore_status_str << "FEDERATE_PREPARED_TO_RESTORE";
         break;

      case FEDERATE_RESTORING:
         restore_status_str << "FEDERATE_RESTORING";
         break;

      case FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE:
         restore_status_str << "FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE";
         break;

      default:
         restore_status_str << "UNKNOWN";
         break;
   }

   return ( restore_status_str.str() );
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_request_check()
{
   // FIXME:
   ostringstream errmsg;
   errmsg << "SaveRestoreServices::restore_request_check():" << __LINE__
          << " ERROR: Function not yet implemented!" << endl;
   DebugHandler::terminate( errmsg.str() );
   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_request_succeeded()
{
   // Set the Restore state to complete!
   restore_state = THLARestoreProcessEnum::RESTORE_COMPLETE;

   // FIXME:
   ostringstream errmsg;
   errmsg << "SaveRestoreServices::restore_request_succeeded():" << __LINE__
          << " ERROR: Function not yet implemented!" << endl;
   DebugHandler::terminate( errmsg.str() );
   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_request_failed()
{
   // Set the Restore state to failed!
   restore_state = THLARestoreProcessEnum::RESTORE_FAILED;

   // FIXME:
   ostringstream errmsg;
   errmsg << "SaveRestoreServices::restore_request_failed():" << __LINE__
          << " ERROR: Function not yet implemented!" << endl;
   DebugHandler::terminate( errmsg.str() );

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_begun()
{

   // We need to turn off all HLA data processing.
   federate->publish_data = false;

   // Set the Restore process state to RESTORE_IN_PROGRESS!
   restore_state = THLARestoreProcessEnum::RESTORE_IN_PROGRESS;

   return;
}

/*!
 *  @job_class{scheduled}
 */
bool SaveRestoreServices::restore_in_progress_check()
{
   // If Federation SaveRestore is not supported then return without action.
   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_UNSUPPORTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::restore_in_progress_check():%d HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return ( false );
   }

   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_IN_PROGRESS ) {

      std::string label_str;
      StringUtilities::to_string( label_str, restore_label );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING,
                          "SaveRestoreServices::restore_in_progress_check():%d HLA Restore for label \'%s\' in progress!\n",
                          __LINE__, label_str.c_str() );
      }
      return ( true );
   }

   return ( false );
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_succeded()
{
   // FIXME:
   ostringstream errmsg;
   errmsg << "SaveRestoreServices::restore_succeded():" << __LINE__
          << " ERROR: Function not yet implemented!" << endl;
   DebugHandler::terminate( errmsg.str() );

   // Set the Restore process state to RESTORE_COMPLETE!
   restore_state = THLARestoreProcessEnum::RESTORE_COMPLETE;

   // Resume publishing HLA data.
   federate->publish_data = true;

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_failed()
{
   // FIXME:
   ostringstream errmsg;
   errmsg << "SaveRestoreServices::restore_failed():" << __LINE__
          << " ERROR: Function not yet implemented!" << endl;
   DebugHandler::terminate( errmsg.str() );

   // Set the Restore process state to RESTORE_FAILED!
   restore_state = THLARestoreProcessEnum::RESTORE_FAILED;

   // Resume publishing HLA data.
   federate->publish_data = true;

   return;
}

//--------------------------------------------------------------------------
// Potentially deprecated SaveRestoreService functions.
//--------------------------------------------------------------------------

void SaveRestoreServices::restore_checkpoint(
   string const &file_name )
{
   string trick_filename = file_name;
   // Prepend federation name to the filename (if it's not already prepended)
   string federation_name_str = federate->get_federation_name();
   if ( trick_filename.compare( 0, federation_name_str.length(), federation_name_str ) != 0 ) {
      trick_filename = federation_name_str + "_" + file_name;
   }
   message_publish( MSG_NORMAL, "SaveRestoreServices::restore_checkpoint() Restoring checkpoint file %s\n",
                    trick_filename.c_str() );

   // Must init all data recording groups since we are restarting at init
   // time before Trick would normally do this. Prior to Trick 10.8, the only way
   // to do this is by calling each recording group init() routine in the S_define

   // This will run pre-load-checkpoint jobs, clear memory, read checkpoint
   // file, and run restart jobs.
   load_checkpoint( ( this->HLA_save_directory + "/" + trick_filename ).c_str() );

   load_checkpoint_job();

   // TODO: Load the checkpoint base time units into the Int64BaseTime class
   // so that all the HLA time representations use the correct base time.
   //
   // Refresh the HLA time constants given the HLA base time from the checkpoint.
   time_management_service->refresh_HLA_time_constants();

   // If exec_set_freeze_command(true) is in master fed's input.py file when
   // check-pointed, then restore starts up in freeze.
   // Clear non-master fed's freeze command so it does not cause
   // unnecessary freeze interaction to be sent.
   if ( !federate->get_execution_control()->is_master() ) {
      exec_set_freeze_command( false );
   }

   message_publish( MSG_NORMAL, "SaveRestoreServices::restore_checkpoint():%d Checkpoint file load complete.\n",
                    __LINE__ );

   // indicate that the restore was completed successfully
   this->restore_state = THLARestoreProcessEnum::RESTORE_COMPLETE;

   // make a copy of the 'restore_process' ENUM just in case it gets overwritten.
   this->prev_restore_process = this->restore_state;
}

void SaveRestoreServices::inform_RTI_of_restore_completion()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   if ( this->prev_restore_process == THLARestoreProcessEnum::RESTORE_COMPLETE ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::inform_RTI_of_restore_completion():%d Restore Complete.\n",
                          __LINE__ );
      }

      try {
         federate->get_RTI_ambassador()->federateRestoreComplete();
      } catch ( RestoreNotRequested const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: RestoreNotRequested\n",
                          __LINE__ );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: SaveInProgress\n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: NotConnected\n",
                          __LINE__ );
         federate->set_connection_lost();
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      }

   } else if ( this->prev_restore_process == THLARestoreProcessEnum::RESTORE_FAILED ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::inform_RTI_of_restore_completion():%d Restore Failed!\n",
                          __LINE__ );
      }

      try {
         federate->get_RTI_ambassador()->federateRestoreNotComplete();
      } catch ( RestoreNotRequested const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: RestoreNotRequested\n",
                          __LINE__ );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: SaveInProgress\n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: NotConnected\n",
                          __LINE__ );
         federate->set_connection_lost();
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         message_publish( MSG_WARNING, "SaveRestoreServices::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      }
   } else {
      message_publish( MSG_NORMAL, "SaveRestoreServices::inform_RTI_of_restore_completion():%d ERROR: \
Unexpected restore process %d, which is not 'RESTORE_COMPLETE' or 'Restore_Request_Failed'.\n",
                       __LINE__, restore_state );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Currently only used with IMSim initialization scheme; only for restore at simulation startup.
 *  @job_class{environment}
 */
void SaveRestoreServices::restart_checkpoint()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::restart_checkpoint():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      HLAinteger64Time fedTime;
      federate->get_RTI_ambassador()->queryLogicalTime( fedTime );
      time_management_service->set_granted_time( fedTime );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d queryLogicalTime EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d queryLogicalTime EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d queryLogicalTime EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d queryLogicalTime EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d queryLogicalTime EXCEPTION: RTIinternalError\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   time_management_service->set_requested_time_to_granted_time();
   this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;

   reinstate_logged_sync_pts();

   federation_restored();
}

/*!
 *  @job_class{freeze}
 */
void SaveRestoreServices::federation_restored()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::federation_restored():%d\n",
                       __LINE__ );
   }
   complete_restore();
   // this->start_to_restore    = false;
   // this->announce_restore    = false;
   // this->restore_begun       = false;
   // this->restore_is_imminent = false;
   this->restore_label = L"";
   this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
}

void SaveRestoreServices::wait_for_federation_restore_begun()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_begun():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   while ( this->restore_state != THLARestoreProcessEnum::RESTORE_BEGUN ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( this->restore_state != THLARestoreProcessEnum::RESTORE_BEGUN ) {

         // To be more efficient, we get the time once and share it.
         int64_t wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SaveRestoreServices::wait_for_federation_restore_begun():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_begun():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_begun():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::wait_until_federation_is_ready_to_restore()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_until_federation_is_ready_to_restore():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   while ( !this->start_to_restore ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->start_to_restore ) { // cppcheck-suppress [knownConditionTrueFalse]

         // To be more efficient, we get the time once and share it.
         int64_t wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SaveRestoreServices::wait_until_federation_is_ready_to_restore():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_until_federation_is_ready_to_restore():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_until_federation_is_ready_to_restore():%d Done.\n",
                       __LINE__ );
   }
}

string SaveRestoreServices::wait_for_federation_restore_to_complete()
{
   string return_string;

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_FAILED ) {
      return_string = "SaveRestoreServices::wait_for_federation_restore_to_complete() "
                      "Restore of federate failed\nTERMINATING SIMULATION!";
      return return_string;
   }

   if ( this->federation_restore_failed_callback_complete ) {
      return_string = "SaveRestoreServices::wait_for_federation_restore_to_complete() "
                      "Federation restore failed\nTERMINATING SIMULATION!";
      return return_string;
   }

   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_FAILED ) {
      // before we enter the blocking loop, the RTI informed us that it accepted
      // the failure of the the federate restore. build and return a message.
      return_string = "SaveRestoreServices::wait_for_federation_restore_to_complete() "
                      "Federation restore FAILED! Look at the message from the "
                      "SaveRestoreServices::print_restore_failure_reason() routine "
                      "for a reason why the federation restore failed.\n"
                      "TERMINATING SIMULATION!";
      return return_string;
   }

   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   // nobody reported any problems, wait until the restore is completed.
   while ( !this->restore_completed ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      if ( this->running_feds_count_at_time_of_restore > federate->joined_federates_map.size() ) {
         // someone has resigned since the federation restore has been initiated.
         // build a message detailing what happened and exit the routine.
         return_string = "SaveRestoreServices::wait_for_federation_restore_to_complete() "
                         "While waiting for restore of the federation "
                         "a federate resigned before the federation restore "
                         "completed!\nTERMINATING SIMULATION!";
         return return_string;
      } else {
         sleep_timer.sleep(); // sleep until RTI responds...

         if ( !this->restore_completed ) { // cppcheck-suppress [knownConditionTrueFalse]

            // To be more efficient, we get the time once and share it.
            int64_t wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "SaveRestoreServices::wait_for_federation_restore_to_complete():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!" << endl;
                  DebugHandler::terminate( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_to_complete():%d Waiting...\n",
                                __LINE__ );
            }
         }
      }
   }

   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_FAILED ) {
      // after this federate restore blocking loop has finished, check if the RTI
      // accepted the failure of the federate restore. build and return a message.
      return_string = "SaveRestoreServices::wait_for_federation_restore_to_complete() "
                      "Federation restore FAILED! Look at the message from the "
                      "SaveRestoreServices::print_restore_failure_reason() routine "
                      "for a reason why the federation restore failed.\n"
                      "TERMINATING SIMULATION!";
      return return_string;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_to_complete():%d Done.\n",
                       __LINE__ );
   }
   return return_string;
}

void SaveRestoreServices::wait_for_restore_request_callback()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_request_callback():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   while ( !has_restore_process_restore_request_failed() && !has_restore_process_restore_request_succeeded() ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !has_restore_process_restore_request_failed() && !has_restore_process_restore_request_succeeded() ) { // cppcheck-suppress [knownConditionTrueFalse]

         // To be more efficient, we get the time once and share it.
         int64_t wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SaveRestoreServices::wait_for_restore_request_callback():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_request_callback():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_request_callback():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::wait_for_restore_status_to_complete()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_status_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   while ( !this->restore_status_response_complete ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->restore_status_response_complete ) { // cppcheck-suppress [knownConditionTrueFalse]

         // To be more efficient, we get the time once and share it.
         int64_t wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SaveRestoreServices::wait_for_restore_status_to_complete():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_status_to_complete():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_status_to_complete():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   while ( !this->federation_restore_failed_callback_complete ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // if the federate has already been restored, do not wait for a signal
      // from the RTI that the federation restore failed, you'll never get it!
      if ( this->restore_completed ) {
         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete():%d Restore Complete, Done.\n",
                             __LINE__ );
         }
         return;
      }
      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->federation_restore_failed_callback_complete ) { // cppcheck-suppress [knownConditionTrueFalse]

         // To be more efficient, we get the time once and share it.
         int64_t wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::request_federation_restore_status() // cppcheck-suppress [functionStatic, unmatchedSuppression]
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::request_federation_restore_status():%d\n",
                       __LINE__ );
   }

   restore_status_response_complete = false;

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      federate->get_RTI_ambassador()->queryFederationRestoreStatus();
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_restore_status():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_restore_status():%d EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_restore_status():%d EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_restore_status():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_restore_status():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 *  @job_class{freeze}
 */
void SaveRestoreServices::requested_federation_restore_status(
   bool status )
{
   if ( !status ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::requested_federation_restore_status():%d\n",
                          __LINE__ );
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // FIXME: This call is gone and the logic is broken.
      // federate->get_fed_ambassador()->set_federation_restore_status_response_to_echo();
      restore_status_process_response = false;

      try {
         federate->get_RTI_ambassador()->queryFederationRestoreStatus();
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::requested_federation_restore_status():%d EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::requested_federation_restore_status():%d EXCEPTION: SaveInProgress\n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::requested_federation_restore_status():%d EXCEPTION: NotConnected\n",
                          __LINE__ );
         federate->set_connection_lost();
      } catch ( RTIinternalError const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::requested_federation_restore_status():%d EXCEPTION: RTIinternalError\n",
                          __LINE__ );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

void SaveRestoreServices::print_requested_federation_restore_status(
   FederateRestoreStatusVector const &status_vector )
{
   FederateRestoreStatusVector::const_iterator vector_iter;

   // dump the contents...
   ostringstream msg;
   // load the first element from 'theFederateRestoreStatusVector'.
   vector_iter = status_vector.begin();

   // Determine if were successful.
   while ( vector_iter != status_vector.end() ) {

      // dump the contents, for now...
      string id_name;
      StringUtilities::to_string( id_name, vector_iter->preRestoreHandle );
      msg << "SaveRestoreServices::print_requested_federation_restore_status() " << __LINE__
          << "pre-restore fed_id=" << id_name;
      StringUtilities::to_string( id_name, vector_iter->postRestoreHandle );
      msg << ", post-restore fed_id =" << id_name
          << ", status matrix:" << endl
          << "   NO_RESTORE_IN_PROGRESS="
          << ( vector_iter->status == NO_RESTORE_IN_PROGRESS ) << endl
          << "   FEDERATE_RESTORE_REQUEST_PENDING="
          << ( vector_iter->status == FEDERATE_RESTORE_REQUEST_PENDING ) << endl
          << "   FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN="
          << ( vector_iter->status == FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN ) << endl
          << "   FEDERATE_PREPARED_TO_RESTORE="
          << ( vector_iter->status == FEDERATE_PREPARED_TO_RESTORE ) << endl
          << "   FEDERATE_RESTORING="
          << ( vector_iter->status == FEDERATE_RESTORING ) << endl
          << "   FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE="
          << ( vector_iter->status == FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE )
          << endl;
      // Load the next element from 'theFederateRestoreStatusVector'.
      ++vector_iter;
   }
   message_publish( MSG_NORMAL, msg.str().c_str() );
}

void SaveRestoreServices::process_requested_federation_restore_status(
   FederateRestoreStatusVector const &status_vector )
{
   FederateRestoreStatusVector::const_iterator vector_iter;
   FederateRestoreStatusVector::const_iterator vector_end;
   vector_iter = status_vector.begin();
   vector_end  = status_vector.end();

   // If any of our federates have a restore in progress, we will NOT initiate restore
   this->initiate_restore_flag = true;

   // while there are elements in Federate Restore Status Vector...
   while ( vector_iter != vector_end ) {
      if ( vector_iter->status != NO_RESTORE_IN_PROGRESS ) {
         this->initiate_restore_flag = false;
         break;
      }
      ++vector_iter;
   }

   // only initiate if all federates do not have restore in progress
   if ( this->initiate_restore_flag ) {
      this->restore_state = THLARestoreProcessEnum::RESTORE_BEGUN;
   }

   // indicate that the request has completed...
   restore_status_response_complete = true;
}

void SaveRestoreServices::print_restore_failure_reason(
   RestoreFailureReason reason )
{
   // dump the contents...
   ostringstream msg;

   if ( reason == RTI_UNABLE_TO_RESTORE ) {
      msg << "SaveRestoreServices::print_restore_failure_reason():" << __LINE__
          << " failure reason=\"RTI_UNABLE_TO_RESTORE\"" << endl;
   }
   if ( reason == FEDERATE_REPORTED_FAILURE_DURING_RESTORE ) {
      msg << "SaveRestoreServices::print_restore_failure_reason():" << __LINE__
          << " failure reason=\"FEDERATE_REPORTED_FAILURE_DURING_RESTORE\"" << endl;
   }
   if ( reason == FEDERATE_RESIGNED_DURING_RESTORE ) {
      msg << "SaveRestoreServices::print_restore_failure_reason():" << __LINE__
          << " failure reason=\"FEDERATE_RESIGNED_DURING_RESTORE\"" << endl;
   }
   if ( reason == RTI_DETECTED_FAILURE_DURING_RESTORE ) {
      msg << "SaveRestoreServices::print_restore_failure_reason():" << __LINE__
          << " failure reason=\"RTI_DETECTED_FAILURE_DURING_RESTORE\"" << endl;
   }
   message_publish( MSG_NORMAL, msg.str().c_str() );

   this->federation_restore_failed_callback_complete = true;
}

void SaveRestoreServices::initiate_restore_announce(
   wstring const &restore_name_label )
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      return;
   }

   this->restore_label = restore_name_label;

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // figure out if anybody else requested a RESTORE before initiating the RESTORE!
   // change context to process for the status request...
   // this->restore_request_complete = false;
   // federate->get_fed_ambassador()->set_federation_restore_status_response_to_process();
   restore_status_process_response = true;
   request_federation_restore_status();
   wait_for_restore_status_to_complete();

   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_BEGUN ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string name_str;
         StringUtilities::to_string( name_str, this->restore_label );
         message_publish( MSG_NORMAL, "SaveRestoreServices::initiate_restore_announce():%d \
restore_process == RESTORE_BEGUN, Telling RTI to request federation \
restore with label '%s'.\n",
                          __LINE__, name_str.c_str() );
      }
      try {
         federate->get_RTI_ambassador()->requestFederationRestore( this->restore_label );
         this->restore_state = THLARestoreProcessEnum::RESTORE_IN_PROGRESS;

         // Save the # of running_feds at the time federation restore is initiated.
         // this way, when the count decreases, we know someone has resigned!
         this->running_feds_count_at_time_of_restore = federate->joined_federates_map.size();
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_restore_announce():%d EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
         this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_restore_announce():%d EXCEPTION: SaveInProgress\n",
                          __LINE__ );
         this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_restore_announce():%d EXCEPTION: RestoreInProgress\n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_restore_announce():%d EXCEPTION: NotConnected\n",
                          __LINE__ );
         federate->set_connection_lost();
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_restore_announce():%d EXCEPTION: RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
         this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_restore_announce():%d \
After communicating with RTI, restore_process != RESTORE_BEGUN, \
Something went WRONG!\n",
                          __LINE__ );
      }
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void SaveRestoreServices::complete_restore()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::complete_restore():%d\n",
                       __LINE__ );
   }

   if ( this->restore_state != THLARestoreProcessEnum::RESTORE_IN_PROGRESS ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::complete_restore():%d Restore Process != RESTORE_IN_PROGRESS.\n",
                          __LINE__ );
      }
      return;
   }

   if ( !this->start_to_restore ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::complete_restore():%d Start to restore flag is false so set restore_completed = true.\n",
                          __LINE__ );
      }
      restore_completed = true;
   }
}

/*!
 *  @job_class{checkpoint}
 */
void SaveRestoreServices::convert_sync_pts()
{
   // Dispatch to the ExecutionControl specific process.
   federate->get_execution_control()->convert_loggable_sync_pts();
}

void SaveRestoreServices::reinstate_logged_sync_pts()
{
   // Dispatch to the ExecutionControl specific process.
   federate->get_execution_control()->reinstate_logged_sync_pts();
}
