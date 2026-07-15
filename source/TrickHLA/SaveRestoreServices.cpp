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
#include "trick/CheckPointRestart.hh"
#include "trick/command_line_protos.h"
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// Global singleton pointer to the CheckPointRestart.
extern Trick::CheckPointRestart *the_cpr;

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
#include "RTI/Handle.h"
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
     object_service( fed.get_object_service() ),
     interaction_service( fed.get_interaction_service() ),
     time_management_service( fed.get_time_management_service() ),
     execution_control( NULL ),
     joined_federates_file_name( "" ),
     HLA_save_directory( "" ),
     save_state( THLASaveProcessEnum::SAVE_UNSUPPORTED ),
     save_label( L"" ),
     restore_state( THLARestoreProcessEnum::RESTORE_UNSUPPORTED ),
     restore_label( L"" ),
     restore_name( L"" )
{
   return;
}

/*!
 * @details Free up the Trick allocated memory associated with the attributes
 * of this class.
 * @job_class{shutdown}
 */
SaveRestoreServices::~SaveRestoreServices()
{
   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::save_set_label( wstring const &label )
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
      string const run_dir = command_line_args_get_output_dir();
      string const def_dir = command_line_args_get_default_dir();

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
             << ": ERROR: Save directory path \'" << this->HLA_save_directory
             << "\' does NOT exist!";
         message_publish( MSG_ERROR, "%s\n", msg.str().c_str() );
      }

      return ( false );

   } else if ( ( info.st_mode & S_IFDIR ) == 0 ) { // NOLINT(misc-include-cleaner)

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream msg;
         msg << "SaveRestoreServices::check_HLA_save_directory():" << __LINE__
             << ": ERROR: Save directory path \'" << this->HLA_save_directory
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
bool SaveRestoreServices::save_set_state( THLASaveProcessEnum state )
{
   // Check to make sure that Save and Restore is supported for this federate.
   if ( ( !execution_control->is_save_and_restore_supported() )
        && ( state != THLASaveProcessEnum::SAVE_UNSUPPORTED ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING,
                          "SaveRestoreServices::save_set_label():%d: HLA SaveRestore NOT supported!\n",
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
void SaveRestoreServices::save_set_time(
   Int64Time const &time )
{
   // Make sure that we are in an appropriate state to set the Save time.
   if ( ( this->save_state != THLASaveProcessEnum::SAVE_NONE )
        && ( this->save_state != THLASaveProcessEnum::SAVE_REQUESTED )
        && ( this->save_state != THLASaveProcessEnum::SAVE_UNSUPPORTED ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::save_set_time():%d: Save already in progress: \'%s\'!\n",
                          __LINE__, TrickHLA::to_string( save_state ).c_str() );
      }
      return;
   }

   // Check to make sure the time hasn't already passed.
   Int64Time const granted_time = time_management_service->get_granted_time();
   if ( time < granted_time ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         std::string label_str;
         StringUtilities::to_string( label_str, save_label );
         ostringstream msg;
         msg << "SaveRestoreServices::save_set_time():" << __LINE__
             << " : WARNING: Save time for label \'" << label_str
             << "\' in the past!";
         msg << " Save time is " << time.get_base_time()
             << " but Granted time is " << granted_time.get_base_time() << endl;
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
         message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d: HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return;
   }

   // Check the Federation Save state to ensure that a Save is applicable .
   if ( ( save_state != THLASaveProcessEnum::SAVE_NONE )
        && ( save_state != THLASaveProcessEnum::SAVE_UNSUPPORTED ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d: Save already in progress: \'%s\'!\n",
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
                << ": ERROR: No Save label set!" << endl;
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
         message_publish( MSG_NORMAL, "SaveRestoreServices::save_request():%d: save_label: \'%s\'\n",
                          __LINE__, label_str.c_str() );
      }

      // Make the requestFederationSave call to the RTI Ambassador.
      federate->get_RTI_ambassador()->requestFederationSave( this->save_label );

   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d: EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d: EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d: EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d: EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::save_request():%d: EXCEPTION: RTIinternalError: '%s'\n",
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
         message_publish( MSG_WARNING, "SaveRestoreServices::save():%d: HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return;
   }

   // Do a little sanity checking.
   if ( save_state != THLASaveProcessEnum::SAVE_REQUESTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::save():" << __LINE__
                << ": WARNING: Save state mismatch: "
                << TrickHLA::to_string( save_state ) << "!" << endl;
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
                << ": ERROR: No Save label set!" << endl;
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
      message_publish( MSG_WARNING, "SaveRestoreServices::setup_checkpoint():%d: EXCEPTION: SaveNotInitiated\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::setup_checkpoint():%d: EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::setup_checkpoint():%d: EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::setup_checkpoint():%d: EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "SaveRestoreServices::setup_checkpoint():%d: EXCEPTION: RTIinternalError: '%s'\n",
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
   checkpoint_file_name = execution_control->map_label_to_checkpoint_file_name( this->save_label );

   // Save the federate state using the Trick checkpoint mechanism.
   the_cpr->checkpoint( checkpoint_file_name );

   //
   // Let the Federation know that our Save process is finished.
   //
   try {

      // Make the call to the RTI Ambassador to mark our Save as complete.
      federate->get_RTI_ambassador()->federateSaveComplete();

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::save():%d: Federate Save Completed.\n",
                          __LINE__ );
      }

   } catch ( FederateHasNotBegunSave const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save():%d: EXCEPTION: FederateHasNotBegunSave\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save():%d: EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save():%d: EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::save():%d: EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "SaveRestoreServices::post_checkpoint():%d: EXCEPTION: RTIinternalError: '%s'\n",
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
         message_publish( MSG_WARNING, "SaveRestoreServices::save_in_progress_check():%d: HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return ( false );
   }

   if ( save_state == THLASaveProcessEnum::SAVE_IN_PROGRESS ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         std::string label_str;
         StringUtilities::to_string( label_str, save_label );
         message_publish( MSG_WARNING, "SaveRestoreServices::save_in_progress_check():%d: HLA Save for label \'%s\' in progress!\n",
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
         message_publish( MSG_WARNING, "SaveRestoreServices::save_succeded():%d: HLA SaveRestore NOT supported!\n",
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
             << ": ERROR: Save state mismatch: "
             << TrickHLA::to_string( save_state ) << "!" << endl;
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
         message_publish( MSG_WARNING, "SaveRestoreServices::save_failed():%d: HLA SaveRestore NOT supported!\n",
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
             << ": ERROR: Save state mismatch: "
             << TrickHLA::to_string( save_state ) << "!" << endl;
      DebugHandler::terminate( errmsg.str() );
   }

   // Print out an error message.
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      ostringstream msg;
      msg << "SaveRestoreServices::save_failed():" << __LINE__
          << " : Save for label \'" << label_str
          << "\' failed!" << endl;
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
                << ": ERROR: No Save label set!" << endl;
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
      file << federate->joined_federates_map.size() << endl;

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
             << ": ERROR: Failed to open file '" << full_file_path << "' for writing!" << endl;
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
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save_status():%d: EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save_status():%d: EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save_status():%d: EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save_status():%d: EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

/*!
 *  @job_class{checkpoint}
 */
void SaveRestoreServices::convert_sync_pts()
{
   // Dispatch to the ExecutionControl specific process.
   federate->get_execution_control()->convert_loggable_sync_pts();
}

//----------------------------------------------------------------------------
// SaveRestoreService Restore functions.
//----------------------------------------------------------------------------

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_set_label( wstring const &label )
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
                << ": ERROR: No Restore label set!" << endl;
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
             << ": ERROR: Failed to open file '" << full_path << "'!" << endl;
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
         std::wcerr << L"Invalid input: No conversion could be performed." << endl;
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
                << ": ERROR: Reading number of known federates:'"
                << " Invalid input: No conversion could be performed." << "'!" << endl;
         message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );
         file.close();
         return ( false );
      } catch ( std::out_of_range const &e ) {
         file.close();
         std::wcerr << L"Error: Number is out of range for an int." << endl;
         return ( false );
      }
   }

   // Sanity check.  There has to be at least 1 federate.
   if ( num_feds == 0 ) {
      file.close();
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
             << ": ERROR: There has to be at least 1 federate.  Read in "
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
                << ": ERROR: Error reading the type for known federate '"
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
                   << ": ERROR: Error reading if known federate '"
                   << fed_name_str << "' is required at line " << line_num
                   << " from '" << full_path << "'!" << endl;
            message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );

         } catch ( std::out_of_range const &e ) {

            // Let the user know that something went wrong.
            std::wcerr << L"Error: Number is out of range for an int." << endl;
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
                << ": ERROR: Error reading if known federate '"
                << fed_name_str << "' is required at line " << line_num
                << " from '" << full_path << "'!" << endl;
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
             << ": ERROR: Error reading the known federates file '"
             << full_path << "'!" << endl;
      message_publish( MSG_ERROR, "%s\n", errmsg.str().c_str() );

      return ( false );
   }

   // Check that the number of federates read in matches the number written.
   if ( fed_count != num_feds ) {

      // Let the user know that something went wrong.
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::read_known_federates_from_file()" << __LINE__
             << ": ERROR: Federate file specified " << num_feds
             << " but read in " << fed_count << "!" << endl;
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
bool SaveRestoreServices::restore_set_state( THLARestoreProcessEnum state )
{
   // Check to make sure that Save and Restore is supported for this federate.
   if ( ( !execution_control->is_save_and_restore_supported() )
        && ( state != THLARestoreProcessEnum::RESTORE_UNSUPPORTED ) ) {

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::restore_set_state():%d: HLA SaveRestore NOT supported!\n",
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
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_request_status():" << __LINE__
                << ": WARNING: SaveRestore NOT supported!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
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
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request_status():%d: EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request_status():%d: EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request_status():%d: EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request_status():%d: EXCEPTION: RTIinternalError\n",
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
void SaveRestoreServices::restore_waiting_for_request_status()
{
   bool restore_conflict = false;

   // Just return if HLA save and restore is not supported by the simulation
   // execution control scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_waiting_for_request_status():" << __LINE__
                << ": WARNING: SaveRestore NOT supported!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Just return if we are not in a Restore process state waiting for a status response.
   if ( restore_state != THLARestoreProcessEnum::RESTORE_REQUEST_STATUS ) {
      return;
   }

   // Check to see if we have received a Restore status response.
   if ( restore_status_response.empty() ) {

      // Check to see if we want to print out a wait message.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         if ( execution_control->process_timer.timeout( execution_control->process_timer.time() ) ) {
            execution_control->process_timer.reset();
            message_publish( MSG_NORMAL,
                             "SaveRestoreServices::restore_waiting_for_request_status():%d: Waiting for Restore status response ...\n",
                             __LINE__ );
         }
      }

      // An empty Restore status response means we are still waiting for the
      // FedAmb::federationRestoreStatusResponse callback.
      return;

   } else {

      // Iterate through the response vector to check for ongoing restores.
      for ( FederateRestoreStatus const &status : restore_status_response ) {
         if ( status.status != NO_RESTORE_IN_PROGRESS ) {
            restore_conflict = true;
            break;
         }
      }
   }

   // Check for Restore response status conflict.
   if ( restore_conflict ) {

      // Reset the Restore state.
      restore_state = THLARestoreProcessEnum::RESTORE_NONE;

      ostringstream errmsg;
      errmsg << "SaveRestoreServices::restore_waiting_for_request_status():" << __LINE__
             << ": WARNING: Federation NOT in state to Restore:" << endl;
      errmsg << to_string( restore_status_response ) << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );

   } else {

      // Set the Restore status to indicate that the status request is complete.
      restore_state = THLARestoreProcessEnum::RESTORE_STATUS_COMPLETE;

      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         ostringstream msg;
         msg << "SaveRestoreServices::restore_waiting_for_request_status():" << __LINE__
             << ": Restore status response complete." << endl;
         message_publish( MSG_NORMAL, msg.str().c_str() );
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
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, label );
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_request():" << __LINE__
                << ": WARNING: SaveRestore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
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
                << ": ERROR: No Restore label set!" << endl;
         DebugHandler::terminate( errmsg.str() );
      }
   } else {
      this->restore_label = label;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      string name_str;
      StringUtilities::to_string( name_str, this->restore_label );
      message_publish( MSG_NORMAL,
                       "SaveRestoreServices::restore_request():%d: Requesting RTI to Restore Federation with label '%s'.\n",
                       __LINE__, name_str.c_str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {

      federate->get_RTI_ambassador()->requestFederationRestore( this->restore_label );
      this->restore_state = THLARestoreProcessEnum::RESTORE_REQUESTED;

   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d: EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
      this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d: EXCEPTION: SaveInProgress\n",
                       __LINE__ );
      this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d: EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d: EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::restore_request():%d: EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
      this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

std::string SaveRestoreServices::to_string( FederateRestoreStatusVector const &response )
{
   ostringstream response_msg;

   // Iterate through the response vector to check for ongoing restores.
   for ( FederateRestoreStatus const &status : response ) {

      // Get the individual federate status string.
      response_msg << to_string( status ) << endl;
   }

   return ( response_msg.str() );
}

std::string SaveRestoreServices::to_string( FederateRestoreStatus const &restore_status )
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
void SaveRestoreServices::restore_waiting_for_request()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, restore_label );
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_waiting_for_request():" << __LINE__
                << ": WARNING: SaveRestore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Just return if we are not in the proper restore state.
   if ( restore_state != THLARestoreProcessEnum::RESTORE_REQUESTED ) {
      return;
   }

   // We're still waiting for the FedAmb callback from the
   // RTIamb->requestFederationRestore call.

   // Check to see if we want to print out a wait message.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      if ( execution_control->process_timer.timeout( execution_control->process_timer.time() ) ) {
         execution_control->process_timer.reset();
         message_publish( MSG_NORMAL,
                          "SaveRestoreServices::restore_waiting_for_request():%d: Waiting for Restore request callback ...\n",
                          __LINE__ );
      }
   }

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_waiting_for_begun()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, restore_label );
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_waiting_for_begun():" << __LINE__
                << ": WARNING: SaveRestore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Just return if we are not in the proper restore state.
   if ( restore_state != THLARestoreProcessEnum::RESTORE_REQUEST_SUCCEEDED ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      if ( execution_control->process_timer.timeout( execution_control->process_timer.time() ) ) {

         execution_control->process_timer.reset();
         string label_str;
         StringUtilities::to_string( label_str, restore_label );
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_waiting_for_begun():" << __LINE__
                << " : Waiting for Restore to begin for Label: '" << label_str << "'!" << endl;
         message_publish( MSG_NORMAL, errmsg.str().c_str() );
      }
   }

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_request_failed()
{

   // Reset the Restore state.
   restore_state = THLARestoreProcessEnum::RESTORE_NONE;

   // Report the error.
   string label_str;
   StringUtilities::to_string( label_str, restore_label );
   ostringstream errmsg;
   errmsg << "SaveRestoreServices::restore_request_failed():" << __LINE__
          << ": ERROR: Restore failed for Label: '" << label_str << "'" << endl;
   message_publish( MSG_ERROR, errmsg.str().c_str() );

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_begun()
{
   // If Federation SaveRestore is not supported then return without action.
   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_UNSUPPORTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::restore_begun():%d: HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return;
   }

   // We need to turn off all HLA data processing.
   federate->publish_data = false;

   // Set the Restore state to RESTORE_BEGUN.
   restore_state = THLARestoreProcessEnum::RESTORE_BEGUN;

   // Now we wait for the FedAmb::initiateFederateRestore callback to actually
   // perform the federate Restore.

   return;
}

/*! @brief Function called after Restore begun while waiting for initiation. */
void SaveRestoreServices::restore_waiting_for_initiated()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, restore_label );
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_waiting_for_initiated():" << __LINE__
                << ": WARNING: SaveRestore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Just return if we are not in the proper restore state.
   if ( restore_state != THLARestoreProcessEnum::RESTORE_BEGUN ) {
      return;
   }

   // Check to see if we want to print out a wait message.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      if ( execution_control->process_timer.timeout( execution_control->process_timer.time() ) ) {
         execution_control->process_timer.reset();
         message_publish( MSG_NORMAL,
                          "SaveRestoreServices::restore_waiting_for_initiated():%d: Waiting for Restore initiated callback ...\n",
                          __LINE__ );
      }
   }

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_initiated(
#if defined( IEEE_1516_2025 )
   wstring const        &label,
   wstring const        &federate_name,
   FederateHandle const &new_federate_handle )
#else
   wstring const &label,
   wstring const &federate_name,
   FederateHandle new_federate_handle )
#endif // IEEE_1516_2025
{
   std::string restore_label_str;
   std::string checkpoint_file_name;
   struct stat temp_buf;

   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, restore_label );
         string fed_name_str;
         StringUtilities::to_string( fed_name_str, federate_name );
         string fed_handle_str;
         StringUtilities::to_string( fed_handle_str, new_federate_handle );
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_initiated():" << __LINE__
                << ": WARNING: SaveRestore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Just return if we are not in the proper restore state.
   if ( restore_state != THLARestoreProcessEnum::RESTORE_IN_PROGRESS ) {
      return;
   }

   // Set the Restore label.
   this->restore_label = label;

   // Map the Restore label to the associated Trick checkpoint file names.
   checkpoint_file_name              = execution_control->map_label_to_checkpoint_file_name( label );
   string const checkpoint_full_path = this->HLA_save_directory + "/" + checkpoint_file_name;

   // Make sure that the checkpoint file exists.
   if ( stat( checkpoint_full_path.c_str(), &temp_buf ) != 0 ) {

      // Mark this Restore as failed.
      this->restore_state = THLARestoreProcessEnum::RESTORE_FAILED;

      message_publish( MSG_ERROR,
                       "SaveRestoreServices::restore_initiated():%d: Could not find checkpoint file: \'%s\'!\n",
                       __LINE__, checkpoint_full_path.c_str() );

      StringUtilities::to_string( restore_label_str, restore_label );
      message_publish( MSG_ERROR,
                       "SaveRestoreServices::restore_initiated():%d: Restore failed for label: \'%s\'!\n",
                       __LINE__, restore_label_str.c_str() );

      // Notify the Federation that we could not complete the Restore.
      restore_failed_notification();

      return;
   }

   // Report status to the user.
   message_publish( MSG_NORMAL, "SaveRestoreServices::restore_initiated():%d: Restoring from checkpoint file %s\n",
                    __LINE__, checkpoint_file_name.c_str() );

   // Mark the Trick load_checkpoint process as pending completion.  We need to do this
   // because the Trick checkpoint process is not thread safe.  This routine is triggered
   // from the FedAmbassador call back on a separate thread.  Once the checkpoint file
   // name is set below,  Trick will automatically start the load checkpoint process at
   // the top of the next Run or Freeze frame.
   this->restore_checkpoint_pending = true;

   // Set the checkpoint file name.
   the_cpr->load_checkpoint( checkpoint_full_path, true );

   // Now Trick will initiate the load checkpoint process:
   // 1. The “preload_checkpoint” jobs are called. These job-classes allow you to prepare
   // your sim for a checkpoint-restore, in whatever way you see fit.
   // 2. init_from_checkpoint( <filename> ) is called, which:
   //    i.  Calls reset_memory to delete all dynamically allocated objects.
   //    ii. Calls read_checkpoint( <filename> ) to read, parse, and restore
   //        the state described in the checkpoint file.
   //        • Read the definitions section of the checkpoint file, and allocate all of the
   //          objects described there in.
   //        • Clear all of the objects to 0, as appropriate to the data-type.
   //        • Read the assignment statement section, and assign values to the objects.
   // 3. Run the “restart” jobs. These too are user-defined jobs that “tidy up” the simulation
   //    state. This is where the Federate::checkpoint_restart jobs are called.
   //
   // If this job where in the main Trick thread, we could call the the_cpr->load_checkpoint_job()
   // directly.  However, that is not the case here.  So we mark the state above and leave
   // it to Trick to call.

   // NOTE: Once the TrickHLA checkpoint_restart job is called, this will reset the 
   // this->restore_checkpoint_pending to false and allow the Restore process to
   // proceed.

   return;
}

/*!
 *  @job_class{scheduled}
 */
bool SaveRestoreServices::restore_waiting_for_completion()
{
   std::string restore_label_str;

   // If Federation SaveRestore is not supported then return without action.
   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_UNSUPPORTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::restore_waiting_for_completion():%d: HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return ( false );
   }

   // Check to see that the checkpoint_restart jobs ran successfully.
   // If so, the restore_state should be THLARestoreProcessEnum::RESTORE_IN_PROGRESS.
   // If the restore_state is THLARestoreProcessEnum::RESTORE_FAILED then
   // something went wrong and we need to notify the Federation.
   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_FAILED ) {

      StringUtilities::to_string( restore_label_str, restore_label );
      message_publish( MSG_ERROR,
                       "SaveRestoreServices::restore_failed_notification():%d: Restore failed for label: \'%s\'!\n",
                       __LINE__, restore_label_str.c_str() );

      // Notify the Federation that we could not complete the Restore.
      restore_failed_notification();

      return( false );
   }

   // Check to see if we are still waiting for the Trick checkpoint to load.
   // If so, the restore_state should be THLARestoreProcessEnum::RESTORE_IN_PROGRESS.
   // If not, then we are not in the Restore state we think we should be in.
   if ( this->restore_state != THLARestoreProcessEnum::RESTORE_IN_PROGRESS ) {

      StringUtilities::to_string( restore_label_str, restore_label );
      message_publish( MSG_ERROR,
                       "SaveRestoreServices::restore_failed_notification():%d: Unexpected Restore state for label: \'%s\'!\n",
                       __LINE__, restore_label_str.c_str() );

      return( false );

   }

   // Check for completion of the load_checkpoint_job call by Trick.
   if ( this->restore_checkpoint_pending ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         if ( execution_control->process_timer.timeout( execution_control->process_timer.time() ) ) {
            execution_control->process_timer.reset();
            StringUtilities::to_string( restore_label_str, restore_label );
            message_publish( MSG_NORMAL,
                             "SaveRestoreServices::restore_waiting_for_completion():%d: HLA Restore in progress for label \'%s\'!\n",
                             __LINE__, restore_label_str.c_str() );
         }
      }

      return( true );

   }

   // Rebuild HLA state after the checkpoint load.
   restore_after_checkpoint_load();

   // Notify the Federation that we successfully completed the Restore.
   restore_success_notification();

   return ( false );
}

/*!
 * @job_class{scheduled}
 */
void SaveRestoreServices::restore_after_checkpoint_load()
{
   // If Federation SaveRestore is not supported then return without action.
   if ( this->restore_state == THLARestoreProcessEnum::RESTORE_UNSUPPORTED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::restore_after_checkpoint_load():%d: HLA SaveRestore NOT supported!\n",
                          __LINE__ );
      }
      return;
   }

   // Make sure to reset the Save state.
   if ( save_state != THLASaveProcessEnum::SAVE_NONE ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, restore_label );
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_after_checkpoint_load():" << __LINE__
                << ": WARNING: Resetting Save state to THLASaveProcessEnum::SAVE_NONE!" << endl
                << " Label: '" << label_str << "'" << endl
                << " State: '" << TrickHLA::to_string( save_state ) << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      save_state = THLASaveProcessEnum::SAVE_NONE;
   }

   // Make sure we are in an appropriate Restore state.
   if ( restore_state == THLARestoreProcessEnum::RESTORE_IN_PROGRESS ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::restore_after_checkpoint_load():%d: Rebuilding HLA Handles.\n",
                          __LINE__ );
      }

      // Restore the data constructs from loading the checkpoint file.
      federate->restore_data_after_checkpoint();
      if ( execution_control != NULL ) {
         execution_control->restore_data_after_checkpoint();
      }

      //
      // Get us restarted again...
      //

      // Reset RTI data to the state it was in when checkpointed
      object_service->setup_object_ref_attributes();
      interaction_service->setup_interaction_ref_attributes();
      object_service->setup_object_RTI_handles();
      interaction_service->setup_interaction_RTI_handles();
      object_service->set_all_object_instance_handles_by_name();

      // FIXME: This will never be true with the current logic.
      if ( restore_state == THLARestoreProcessEnum::RESTORE_ACTIVATE ) {
         federate->set_all_federate_MOM_instance_handles_by_name();
         federate->restore_federate_handles_from_MOM();
      }

      // Restore interactions and sync points
      reinstate_logged_sync_pts();

      // FIXME: This should have already been done.
      // Restore ownership transfer data for all objects
      // Object *objects   = object_service->get_objects();
      // int     obj_count = object_service->get_object_count();
      // for ( int i = 0; i < obj_count; ++i ) {
      //   objects[i].restore_data_after_checkpoint();
      //}

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::restore_after_checkpoint_load():%d: Rebuilt HLA federate state.\n",
                          __LINE__ );
      }

   } else {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::restore_after_checkpoint_load():%d: Restore NOT in progress!\n",
                          __LINE__ );
      }
   }

   return;
}

/*!
 *  @job_class{freeze}
 */
void SaveRestoreServices::restore_success_notification()
{
   std::string restore_label_str;

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      StringUtilities::to_string( restore_label_str, restore_label );
      message_publish( MSG_NORMAL,
                       "SaveRestoreServices::restore_success_notification():%d: Restore succeeded for label: \'%s\'!\n",
                       __LINE__, restore_label_str.c_str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Tell the Federation that this federate completed the Restore.
   try {
      federate->get_RTI_ambassador()->federateRestoreComplete();
   } catch ( RestoreNotRequested const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_success_notification():%d: EXCEPTION: RestoreNotRequested\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_success_notification():%d: EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_success_notification():%d: EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_success_notification():%d: EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::restore_success_notification():%d: EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

/*!
 *  @job_class{freeze}
 */
void SaveRestoreServices::restore_failed_notification()
{
   std::string restore_label_str;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      StringUtilities::to_string( restore_label_str, restore_label );
      message_publish( MSG_ERROR,
                       "SaveRestoreServices::restore_failed_notification():%d: Restore failed for label: \'%s\'!\n",
                       __LINE__, restore_label_str.c_str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Tell the Federation that this federate could not complete the Restore.
   try {
      federate->get_RTI_ambassador()->federateRestoreNotComplete();
   } catch ( RestoreNotRequested const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_failed_notification():%d: EXCEPTION: RestoreNotRequested\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_failed_notification():%d: EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_failed_notification():%d: EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_failed_notification():%d: EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::restore_failed_notification():%d: EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_succeded()
{

   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, restore_label );
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_succeded():" << __LINE__
                << ": WARNING: SaveRestore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Just return if we are not in the proper restore state.
   if ( restore_state != THLARestoreProcessEnum::RESTORE_COMPLETE ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;
   try {
      HLAinteger64Time time;
      federate->get_RTI_ambassador()->queryLogicalTime( time );
      federate->time_management_service.set_granted_time( time );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_succeded():%d: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_succeded():%d: SaveInProgress\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_succeded():%d: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_succeded():%d: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restore_succeded():%d: RTIinternalError\n",
                       __LINE__ );
   }

   federate->time_management_service.set_requested_time_to_granted_time();

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // TODO: Load the checkpoint base time units into the Int64BaseTime class
   // so that all the HLA time representations use the correct base time.
   //
   // Refresh the HLA time constants given the HLA base time from the checkpoint.
   time_management_service->refresh_HLA_time_constants();

   // Resume publishing HLA data.
   federate->publish_data = true;

   // The Restore is successfully completed!  So, return to nominal state.
   restore_state = THLARestoreProcessEnum::RESTORE_NONE;

   return;
}

/*!
 *  @job_class{scheduled}
 */
void SaveRestoreServices::restore_failed()
{

   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !execution_control->is_save_and_restore_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
         string label_str;
         StringUtilities::to_string( label_str, restore_label );
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::restore_failed():" << __LINE__
                << ": WARNING: SaveRestore NOT supported!" << endl
                << " Label:'" << label_str << "'" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Just return if we are not in the proper restore state.
   if ( restore_state != THLARestoreProcessEnum::RESTORE_FAILED ) {
      return;
   }

   // Resume publishing HLA data.
   federate->publish_data = true;

   // Return to nominal state.
   restore_state = THLARestoreProcessEnum::RESTORE_NONE;

   // FIXME: Should we just terminate here?

   return;
}

void SaveRestoreServices::restore_failed_print_reason(
   RestoreFailureReason reason )
{
   // dump the contents...
   ostringstream msg;

   if ( reason == RTI_UNABLE_TO_RESTORE ) {
      msg << "SaveRestoreServices::restore_failed_print_reason():" << __LINE__
          << " failure reason=\"RTI_UNABLE_TO_RESTORE\"" << endl;
   }
   if ( reason == FEDERATE_REPORTED_FAILURE_DURING_RESTORE ) {
      msg << "SaveRestoreServices::restore_failed_print_reason():" << __LINE__
          << " failure reason=\"FEDERATE_REPORTED_FAILURE_DURING_RESTORE\"" << endl;
   }
   if ( reason == FEDERATE_RESIGNED_DURING_RESTORE ) {
      msg << "SaveRestoreServices::restore_failed_print_reason():" << __LINE__
          << " failure reason=\"FEDERATE_RESIGNED_DURING_RESTORE\"" << endl;
   }
   if ( reason == RTI_DETECTED_FAILURE_DURING_RESTORE ) {
      msg << "SaveRestoreServices::restore_failed_print_reason():" << __LINE__
          << " failure reason=\"RTI_DETECTED_FAILURE_DURING_RESTORE\"" << endl;
   }
   message_publish( MSG_NORMAL, msg.str().c_str() );

   return;
}

/*!
 *  @job_class{restart}
 */
void SaveRestoreServices::reinstate_logged_sync_pts()
{
   // Dispatch to the ExecutionControl specific process.
   execution_control->reinstate_logged_sync_pts();
}

//--------------------------------------------------------------------------
// FIXME: Potentially deprecated SaveRestoreService functions.
//--------------------------------------------------------------------------

void SaveRestoreServices::restore_checkpoint(
   string const &file_name )
{
   string trick_filename = file_name;
   // Prepend federation name to the filename (if it's not already prepended)
   string const federation_name_str = federate->get_federation_name();
   if ( trick_filename.compare( 0, federation_name_str.length(), federation_name_str ) != 0 ) {
      trick_filename = federation_name_str + "_" + file_name;
   }
   message_publish( MSG_NORMAL, "SaveRestoreServices::restore_checkpoint():%d: Restoring checkpoint file %s\n",
                    __LINE__, trick_filename.c_str() );

   // Must init all data recording groups since we are restarting at init
   // time before Trick would normally do this. Prior to Trick 10.8, the only way
   // to do this is by calling each recording group init() routine in the S_define

   // This will run pre-load-checkpoint jobs, clear memory, read checkpoint
   // file, and run restart jobs.
   the_cpr->load_checkpoint( this->HLA_save_directory + "/" + trick_filename );

   the_cpr->load_checkpoint_job();

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

   message_publish( MSG_NORMAL, "SaveRestoreServices::restore_checkpoint():%d: Checkpoint file load complete.\n",
                    __LINE__ );

   // indicate that the restore was completed successfully
   this->restore_state = THLARestoreProcessEnum::RESTORE_COMPLETE;

   return;
}

//--------------------------------------------------------------------------
// Potentially deprecated SaveRestoreService functions.
//--------------------------------------------------------------------------

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
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d: SaveInProgress\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::restart_checkpoint():%d: RTIinternalError\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   time_management_service->set_requested_time_to_granted_time();
   this->restore_state = THLARestoreProcessEnum::RESTORE_NONE;

   reinstate_logged_sync_pts();

   // FIXME: Reset the Save/Restore state.
   restore_label.clear();
   restore_state = THLARestoreProcessEnum::RESTORE_NONE;
}
