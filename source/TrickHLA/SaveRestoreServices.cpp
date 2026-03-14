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
@revs_end

*/

// System include files.
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <float.h>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/CheckPointRestart_c_intf.hh"
#include "trick/Flag.h"
#include "trick/MemoryManager.hh"
#include "trick/command_line_protos.h"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
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
SaveRestoreServices::SaveRestoreServices(
   Federate *fed )
   : restore_federation( false ),
     restore_file_name(),
     initiated_a_federation_save( false ),
     federate( fed ),
     save_name( L"" ),
     restore_name( L"" ),
     HLA_save_directory( "" ),
     initiate_save_flag( false ),
     restore_process( NO_RESTORE ),
     prev_restore_process( NO_RESTORE ),
     initiate_restore_flag( false ),
     restore_in_progress( false ),
     restore_failed( false ),
     restore_is_imminent( false ),
     save_label( "" ),
     announce_save( false ),
     save_label_generated( false ),
     save_request_complete( false ),
     save_completed( false ),
     restore_label( "" ),
     announce_restore( false ),
     restore_label_generated( false ),
     restore_begun( false ),
     restore_request_complete( false ),
     restore_completed( false ),
     federation_restore_failed_callback_complete( false ),
     federate_has_been_restarted( false ),
     publish_data( true ),
     running_feds_count( 0 ),
     running_feds( NULL ),
     running_feds_count_at_time_of_restore( 0 ),
     checkpoint_file_name( "" ),
     checkpoint_rt_itimer( Off ),
     execution_has_begun( false ),
     start_to_save( false ),
     start_to_restore( false ),
     restart_flag( false ),
     restart_cfg_flag( false )
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
   // Free the memory used by the array of running Federates for the Federation.
   clear_running_feds();
}

/*!
 * @details Trigger federation save, at current time or user-specified time.\n
 * NOTE: These routines do not coordinate a federation save via interactions
 * so make these internal routines so that the user does not accidentally call
 * them and mess things up.
 */
void SaveRestoreServices::initiate_federation_save(
   string const &file_name )
{
   set_checkpoint_file_name( file_name );
   initiate_save_announce();

   this->initiated_a_federation_save = true;
}

void SaveRestoreServices::start_federation_save(
   string const &file_name )
{
   start_federation_save_at_scenario_time( -DBL_MAX, file_name );
}

void SaveRestoreServices::start_federation_save_at_sim_time(
   double        freeze_sim_time,
   string const &file_name )
{
   start_federation_save_at_scenario_time(
      federate->get_execution_control()->convert_sim_time_to_scenario_time( freeze_sim_time ),
      file_name );
}

void SaveRestoreServices::start_federation_save_at_scenario_time(
   double        freeze_scenario_time,
   string const &file_name )
{
   // Call the ExecutionControl method.
   federate->get_execution_control()->start_federation_save_at_scenario_time(
      freeze_scenario_time, file_name );
}

void SaveRestoreServices::load_and_print_running_federate_names()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::load_and_print_running_federate_names():%d started.\n",
                       __LINE__ );
   }

   // Make sure the MOM handles get initialized before we try to use them.
   if ( !federate->MOM_HLAfederation_class_handle.isValid() ) {
      federate->initialize_MOM_handles();
   }

   AttributeHandleSet fedMomAttributes;
   fedMomAttributes.insert( federate->MOM_HLAfederatesInFederation_handle );
   federate->subscribe_attributes( federate->MOM_HLAfederation_class_handle, fedMomAttributes );

   AttributeHandleSet requestedAttributes;
   requestedAttributes.insert( federate->MOM_HLAfederatesInFederation_handle );
   federate->request_attribute_update( federate->MOM_HLAfederation_class_handle, requestedAttributes );

   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   while ( this->running_feds_count <= 0 ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // Sleep a little while to wait for the information to update.
      sleep_timer.sleep();

      if ( this->running_feds_count <= 0 ) { // cppcheck-suppress [knownConditionTrueFalse]

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SaveRestoreServices::load_and_print_running_federate_names():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::load_and_print_running_federate_names():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }

   // Only unsubscribe from the attributes we subscribed to in this function.
   federate->unsubscribe_attributes( federate->MOM_HLAfederation_class_handle, fedMomAttributes );

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::load_and_print_running_federate_names():%d \
MOM just informed us that there are %d federates currently running in the federation.\n",
                       __LINE__, running_feds_count );
   }

   federate->ask_MOM_for_federate_names();

   int joinedFedCount = 0;

   // Wait for all the required federates to join.
   federate->all_federates_joined = false;

   print_timer.reset();
   sleep_timer.reset();

   while ( !federate->all_federates_joined ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // Sleep a little while to wait for more federates to join.
      sleep_timer.sleep();

      // Determine what federates have joined only if the joined federate
      // count has changed.
      if ( joinedFedCount != (int)federate->joined_federate_names.size() ) {
         joinedFedCount = federate->joined_federate_names.size();

         if ( joinedFedCount >= running_feds_count ) {
            federate->all_federates_joined = true;
         }
      }
      if ( !federate->all_federates_joined ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SaveRestoreServices::load_and_print_running_federate_names():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::load_and_print_running_federate_names():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }

   // Execute a blocking loop until the RTI responds with information for all
   // running federates
   print_timer.reset();
   sleep_timer.reset();
   while ( federate->joined_federate_names.size() < (unsigned int)running_feds_count ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep();

      if ( federate->joined_federate_names.size() < (unsigned int)running_feds_count ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SaveRestoreServices::load_and_print_running_federate_names():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::load_and_print_running_federate_names():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }

   // Now, copy the new information into my data stores and restore the saved
   // information back to what is was before this routine ran (so we can get a
   // valid checkpoint).
   clear_running_feds();
   update_running_feds();

   // Print out a list of the Running Federates.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      // Build the federate summary as an output string stream.
      ostringstream summary;
      unsigned int  cnt = 0;

      summary << "SaveRestoreServices::load_and_print_running_federate_names():"
              << __LINE__ << endl
              << "'running_feds' data structure contains these "
              << running_feds_count << " federates:";

      // Summarize the required federates first.
      for ( int i = 0; i < running_feds_count; ++i ) {
         ++cnt;
         summary << endl
                 << "    " << cnt
                 << ": Found running federate '"
                 << running_feds[i].name << "'";
      }
      summary << endl;

      // Display the federate summary.
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }

   // Clear the entry since it was absorbed into running_feds...
   federate->joined_federate_name_map.clear();

   fedMomAttributes.clear();
   requestedAttributes.clear();

   // Do not un-subscribe to this MOM data; we DO want updates as federates
   // join / resign the federation!

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::load_and_print_running_federate_names():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::update_running_feds()
{
   // Make a copy of the updated known feds before restoring the saved copy...
   running_feds = reinterpret_cast< KnownFederate * >(
      alloc_type( running_feds_count, "TrickHLA::KnownFederate" ) );

   if ( running_feds == NULL ) {
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::update_running_feds():" << __LINE__
             << " ERROR: Could not allocate memory for running_feds!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( (int)federate->joined_federate_name_map.size() != running_feds_count ) {
      // Show the contents of 'joined_federate_name_map'
      TrickHLAObjInstanceNameMap::const_iterator map_iter;
      for ( map_iter = federate->joined_federate_name_map.begin();
            map_iter != federate->joined_federate_name_map.end();
            ++map_iter ) {
         string fed_name_str;
         StringUtilities::to_string( fed_name_str, federate->MOM_HLAfederate_instance_name_map[map_iter->first] );
         string obj_name_str;
         StringUtilities::to_string( obj_name_str, map_iter->second );
         message_publish( MSG_NORMAL, "SaveRestoreServices::update_running_feds():%d joined_federate_name_map[%s]=%s\n",
                          __LINE__, fed_name_str.c_str(), obj_name_str.c_str() );
      }

      for ( int i = 0; i < running_feds_count; ++i ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::update_running_feds():%d running_feds[%d]=%s\n",
                          __LINE__, i, running_feds[i].name.c_str() );
      }

      // Terminate the execution since the counters are out of sync...
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::update_running_feds():" << __LINE__
             << " FATAL_ERROR: joined_federate_name_map contains "
             << federate->joined_federate_name_map.size()
             << " entries but running_feds_count = " << running_feds_count
             << "!!!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Loop through joined_federate_name_map to build the running_feds list
   unsigned int index = 0;

   TrickHLAObjInstanceNameMap::const_iterator map_iter;
   for ( map_iter = federate->joined_federate_name_map.begin();
         map_iter != federate->joined_federate_name_map.end(); ++map_iter ) {

      running_feds[index].name = StringUtilities::mm_strdup_wstring( map_iter->second.c_str() );

      running_feds[index].MOM_instance_name = StringUtilities::mm_strdup_wstring(
         federate->MOM_HLAfederate_instance_name_map[map_iter->first].c_str() );

      // If the federate was running at the time of the checkpoint, it must be
      // a 'required' federate in the restore, regardless if it is was required
      // when the federation originally started up.
      running_feds[index].required = true;

      ++index;
   }
}

void SaveRestoreServices::clear_running_feds()
{
   if ( this->running_feds != NULL ) {
      for ( int i = 0; i < running_feds_count; ++i ) {
         running_feds[i].MOM_instance_name = "";
         running_feds[i].name              = "";
      }
      if ( trick_MM->delete_var( static_cast< void * >( this->running_feds ) ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::clear_running_feds():%d WARNING failed to delete Trick Memory for 'this->running_feds'\n",
                          __LINE__ );
      }
      this->running_feds = NULL;
   }
}

void SaveRestoreServices::add_a_single_entry_into_running_feds()
{
   // Allocate a new structure to absorb the original values plus the new one.
   KnownFederate *temp_feds;
   temp_feds = reinterpret_cast< KnownFederate * >(
      alloc_type( running_feds_count + 1, "TrickHLA::KnownFederate" ) );

   if ( temp_feds == NULL ) {
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::add_a_single_entry_into_running_feds():" << __LINE__
             << " ERROR: Could not allocate memory for temp_feds when attempting to add"
             << " an entry into running_feds!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      // copy current running_feds entries into temporary structure...
      for ( int i = 0; i < running_feds_count; ++i ) {
         temp_feds[i].MOM_instance_name = running_feds[i].MOM_instance_name;
         temp_feds[i].name              = running_feds[i].name;
         temp_feds[i].required          = running_feds[i].required;
      }

      TrickHLAObjInstanceNameMap::const_iterator map_iter = federate->joined_federate_name_map.begin();
      StringUtilities::to_string( temp_feds[running_feds_count].MOM_instance_name,
                                  federate->MOM_HLAfederate_instance_name_map[map_iter->first] );
      StringUtilities::to_string( temp_feds[running_feds_count].name, map_iter->second );
      temp_feds[running_feds_count].required = true;

      // delete running_feds data structure.
      clear_running_feds();

      // assign temp_feds into running_feds
      this->running_feds = temp_feds;

      ++running_feds_count; // make the new running_feds_count size permanent
   }

#if 0 // TODO: Update for IMSim.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::add_a_single_entry_into_running_feds():%d Exiting routine, here is what running_feds contains:\n",
               __LINE__, '\n');
      for ( int t = 0; t < running_feds_count; t++) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::add_a_single_entry_into_running_feds():%d running_feds[%d].MOM_instance_name='%s'\n",
                  __LINE__, t, running_feds[t].MOM_instance_name, '\n');
         message_publish( MSG_NORMAL, "SaveRestoreServices::add_a_single_entry_into_running_feds():%d running_feds[%d].name='%s'\n",
                  __LINE__, t, running_feds[t].name, '\n');
         message_publish( MSG_NORMAL, "SaveRestoreServices::add_a_single_entry_into_running_feds():%d running_feds[%d].required=%d\n",
                  __LINE__, t, running_feds[t].required, '\n');
      }
   }
#endif
}

void SaveRestoreServices::write_running_feds_file(
   string const &file_name )
{
   string   full_path;
   ofstream file;

   full_path = this->HLA_save_directory + "/" + file_name + ".running_feds";
   file.open( full_path.c_str(), ios::out ); // flawfinder: ignore

   if ( file.is_open() ) {
      file << this->running_feds_count << endl;

      // echo the contents of running_feds into file...
      for ( int i = 0; i < this->running_feds_count; ++i ) {
         file << running_feds[i].MOM_instance_name << endl;
         file << running_feds[i].name << endl;
         file << running_feds[i].required << endl;
      }

      file.close(); // close the file.

   } else {
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::write_running_feds_file():" << __LINE__
             << " ERROR: Failed to open file '" << full_path << "' for writing!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*!
 *  @job_class{freeze}
 */
void SaveRestoreServices::request_federation_save()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !federate->get_execution_control()->is_save_and_restore_supported() ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string name_str;
         StringUtilities::to_string( name_str, this->save_name );
         message_publish( MSG_NORMAL, "SaveRestoreServices::request_federation_save():%d save_name:%s\n",
                          __LINE__, name_str.c_str() );
      }
      federate->get_RTI_ambassador()->requestFederationSave( this->save_name );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save():%d EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save():%d EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
      federate->set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "SaveRestoreServices::request_federation_save():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

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
   federate->refresh_HLA_time_constants();

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
   this->restore_process = RESTORE_COMPLETE;

   // make a copy of the 'restore_process' ENUM just in case it gets overwritten.
   this->prev_restore_process = this->restore_process;
}

void SaveRestoreServices::inform_RTI_of_restore_completion()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   if ( this->prev_restore_process == RESTORE_COMPLETE ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
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

   } else if ( this->prev_restore_process == RESTORE_FAILED ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
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
                       __LINE__, restore_process );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void SaveRestoreServices::read_running_feds_file(
   string const &file_name )
{
   string   full_path;
   ifstream file;

   // Prepend federation name to the filename (if it's not already prepended)
   string federation_name_str = federate->get_federation_name();
   if ( file_name.compare( 0, federation_name_str.length(), federation_name_str ) == 0 ) {
      // Already prepended
      full_path = this->HLA_save_directory + "/" + file_name + ".running_feds";
   } else {
      // Prepend it here
      full_path = this->HLA_save_directory + "/" + federation_name_str + "_" + file_name + ".running_feds";
   }

   file.open( full_path.c_str(), ios::in ); // flawfinder: ignore
   if ( file.is_open() ) {

      federate->clear_known_feds();

      file >> federate->known_feds_count;

      // Re-allocate it...
      federate->known_feds = reinterpret_cast< KnownFederate * >(
         alloc_type( federate->known_feds_count, "TrickHLA::KnownFederate" ) );
      if ( federate->known_feds == NULL ) {
         ostringstream errmsg;
         errmsg << "SaveRestoreServices::read_running_feds_file():" << __LINE__
                << " ERROR: Could not allocate memory for known_feds!" << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      string current_line;
      for ( int i = 0; i < federate->known_feds_count; ++i ) {
         file >> current_line;
         federate->known_feds[i].MOM_instance_name = current_line;

         file >> current_line;
         federate->known_feds[i].name = current_line;

         file >> current_line;
         federate->known_feds[i].required = ( atoi( current_line.c_str() ) != 0 ); // flawfinder: ignore
      }

      file.close(); // Close the file before exiting
   } else {
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::read_running_feds_file()" << __LINE__
             << " ERROR: Failed to open file '" << full_path << "'!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

void SaveRestoreServices::copy_running_feds_into_known_feds()
{
   federate->clear_known_feds();

   // Re-allocate it...
   federate->known_feds = reinterpret_cast< KnownFederate * >(
      alloc_type( running_feds_count, "TrickHLA::KnownFederate" ) );
   if ( federate->known_feds == NULL ) {
      ostringstream errmsg;
      errmsg << "SaveRestoreServices::copy_running_feds_into_known_feds():" << __LINE__
             << " ERROR: Could not allocate memory for known_feds!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Copy everything from running_feds into known_feds...
   federate->known_feds_count = 0;
   for ( int i = 0; i < this->running_feds_count; ++i ) {
      federate->known_feds[federate->known_feds_count].MOM_instance_name = running_feds[i].MOM_instance_name;
      federate->known_feds[federate->known_feds_count].name              = running_feds[i].name;
      federate->known_feds[federate->known_feds_count].required          = running_feds[i].required;
      federate->known_feds_count++;
   }
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Currently only used with IMSim initialization scheme; only for restore at simulation startup.
 *  @job_class{environment}
 */
void SaveRestoreServices::restart_checkpoint()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::restart_checkpoint():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      HLAinteger64Time fedTime;
      federate->get_RTI_ambassador()->queryLogicalTime( fedTime );
      federate->set_granted_time( fedTime );
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

   federate->set_requested_time_to_granted_time();
   this->restore_process = NO_RESTORE;

   reinstate_logged_sync_pts();

   federation_restored();
}

/*!
 *  @job_class{freeze}
 */
void SaveRestoreServices::federation_saved()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::federation_saved():%d\n", __LINE__ );
   }
   this->announce_save         = false;
   this->save_label_generated  = false;
   this->save_request_complete = false;
   this->save_label            = "";
   this->save_name             = L"";
   this->checkpoint_file_name  = "";

   if ( federate->unfreeze_after_save ) {
      // This keeps from generating the RUNFED_v2 sync point since it's not needed
      federate->get_execution_control()->set_freeze_announced( false );

      // Exit freeze mode.
      federate->un_freeze();
   }
}

/*!
 *  @job_class{freeze}
 */
void SaveRestoreServices::federation_restored()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::federation_restored():%d\n",
                       __LINE__ );
   }
   complete_restore();
   this->start_to_restore     = false;
   this->announce_restore     = false;
   this->save_label_generated = false;
   this->restore_begun        = false;
   this->restore_is_imminent  = false;
   this->restore_label        = "";
   this->restore_process      = NO_RESTORE;
}

void SaveRestoreServices::wait_for_federation_restore_begun()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_begun():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !this->restore_begun ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->restore_begun ) { // cppcheck-suppress [knownConditionTrueFalse]

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
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_begun():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_begun():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::wait_until_federation_is_ready_to_restore()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_until_federation_is_ready_to_restore():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer( federate->wait_status_time );
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
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_until_federation_is_ready_to_restore():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_until_federation_is_ready_to_restore():%d Done.\n",
                       __LINE__ );
   }
}

string SaveRestoreServices::wait_for_federation_restore_to_complete()
{
   string return_string;

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   if ( this->restore_failed ) {
      return_string = "SaveRestoreServices::wait_for_federation_restore_to_complete() "
                      "Restore of federate failed\nTERMINATING SIMULATION!";
      return return_string;
   }

   if ( this->federation_restore_failed_callback_complete ) {
      return_string = "SaveRestoreServices::wait_for_federation_restore_to_complete() "
                      "Federation restore failed\nTERMINATING SIMULATION!";
      return return_string;
   }

   if ( this->restore_process == RESTORE_FAILED ) {
      // before we enter the blocking loop, the RTI informed us that it accepted
      // the failure of the the federate restore. build and return a message.
      return_string = "SaveRestoreServices::wait_for_federation_restore_to_complete() "
                      "Federation restore FAILED! Look at the message from the "
                      "SaveRestoreServices::print_restore_failure_reason() routine "
                      "for a reason why the federation restore failed.\n"
                      "TERMINATING SIMULATION!";
      return return_string;
   }

   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   // nobody reported any problems, wait until the restore is completed.
   while ( !this->restore_completed ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      if ( this->running_feds_count_at_time_of_restore > this->running_feds_count ) {
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
                  DebugHandler::terminate_with_message( errmsg.str() );
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

   if ( this->restore_process == RESTORE_FAILED ) {
      // after this federate restore blocking loop has finished, check if the RTI
      // accepted the failure of the federate restore. build and return a message.
      return_string = "SaveRestoreServices::wait_for_federation_restore_to_complete() "
                      "Federation restore FAILED! Look at the message from the "
                      "SaveRestoreServices::print_restore_failure_reason() routine "
                      "for a reason why the federation restore failed.\n"
                      "TERMINATING SIMULATION!";
      return return_string;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_to_complete():%d Done.\n",
                       __LINE__ );
   }
   return return_string;
}

void SaveRestoreServices::wait_for_restore_request_callback()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_request_callback():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer( federate->wait_status_time );
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
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_request_callback():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_request_callback():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::wait_for_restore_status_to_complete()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_status_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !this->restore_request_complete ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->restore_request_complete ) { // cppcheck-suppress [knownConditionTrueFalse]

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
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_status_to_complete():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_restore_status_to_complete():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::wait_for_save_status_to_complete()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_save_status_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !this->save_request_complete ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->save_request_complete ) { // cppcheck-suppress [knownConditionTrueFalse]

         // To be more efficient, we get the time once and share it.
         int64_t wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "SaveRestoreServices::wait_for_save_status_to_complete():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_save_status_to_complete():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_save_status_to_complete():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !this->federation_restore_failed_callback_complete ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // if the federate has already been restored, do not wait for a signal
      // from the RTI that the federation restore failed, you'll never get it!
      if ( this->restore_completed ) {
         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
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
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::wait_for_federation_restore_failed_callback_to_complete():%d Done.\n",
                       __LINE__ );
   }
}

void SaveRestoreServices::request_federation_save_status() // cppcheck-suppress [functionStatic, unmatchedSuppression]
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
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
}

void SaveRestoreServices::request_federation_restore_status() // cppcheck-suppress [functionStatic, unmatchedSuppression]
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::request_federation_restore_status():%d\n",
                       __LINE__ );
   }

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
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::requested_federation_restore_status():%d\n",
                          __LINE__ );
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      federate->get_fed_ambassador()->set_federation_restore_status_response_to_echo();
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
      this->restore_process = INITIATE_RESTORE;
   }

   // indicate that the request has completed...
   restore_request_complete = true;
}

void SaveRestoreServices::process_requested_federation_save_status(
   FederateHandleSaveStatusPairVector const &status_vector )
{
   FederateHandleSaveStatusPairVector::const_iterator vector_iter;
   FederateHandleSaveStatusPairVector::const_iterator vector_end;
   vector_iter = status_vector.begin();
   vector_end  = status_vector.end();

   // If any of our federates have a save in progress, we will NOT initiate save
   initiate_save_flag = true;

   // while there are elements in Federate Save Status Vector...
   while ( initiate_save_flag && ( vector_iter != vector_end ) ) {
      if ( vector_iter->second != RTI1516_NAMESPACE::NO_SAVE_IN_PROGRESS ) {
         initiate_save_flag = false;
      }
      ++vector_iter;
   }

   // indicate that the request has completed...
   save_request_complete = true;
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
}

/*!
 *  @job_class{environment}
 */
void SaveRestoreServices::set_checkpoint_file_name(
   string const &name ) // IN: -- checkpoint file name
{
   this->checkpoint_file_name = name;
   StringUtilities::to_wstring( this->save_name, name );
}

/*!
 *  @job_class{environment}
 */
void SaveRestoreServices::initiate_save_announce()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !federate->get_execution_control()->is_save_and_restore_supported() ) {
      return;
   }

   if ( this->save_label_generated ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_save_announce():%d save_label already generated for federate '%s'\n",
                          __LINE__, federate->get_federate_name().c_str() );
      }
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::initiate_save_announce():%d Checkpoint filename:'%s'\n",
                       __LINE__, checkpoint_file_name.c_str() );
   }

   // Save the checkpoint_file_name into 'save_label'
   this->save_label = this->checkpoint_file_name;

   this->save_label_generated = true;
}

void SaveRestoreServices::initiate_restore_announce(
   string const &restore_name_label )
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !federate->get_execution_control()->is_save_and_restore_supported() ) {
      return;
   }

   this->restore_label = restore_name_label;

   // Wide String restore label
   wstring ws_restore_label;
   StringUtilities::to_wstring( ws_restore_label, this->restore_label );

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // figure out if anybody else requested a RESTORE before initiating the RESTORE!
   // change context to process for the status request...
   this->restore_request_complete = false;
   federate->get_fed_ambassador()->set_federation_restore_status_response_to_process();
   request_federation_restore_status();
   wait_for_restore_status_to_complete();

   if ( this->restore_process == INITIATE_RESTORE ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string name_str;
         StringUtilities::to_string( name_str, ws_restore_label );
         message_publish( MSG_NORMAL, "SaveRestoreServices::initiate_restore_announce():%d \
restore_process == INITIATE_RESTORE, Telling RTI to request federation \
restore with label '%s'.\n",
                          __LINE__, name_str.c_str() );
      }
      try {
         federate->get_RTI_ambassador()->requestFederationRestore( ws_restore_label );
         this->restore_process = RESTORE_IN_PROGRESS;

         // Save the # of running_feds at the time federation restore is initiated.
         // this way, when the count decreases, we know someone has resigned!
         this->running_feds_count_at_time_of_restore = this->running_feds_count;
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_restore_announce():%d EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
         this->restore_process = NO_RESTORE;
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_restore_announce():%d EXCEPTION: SaveInProgress\n",
                          __LINE__ );
         this->restore_process = NO_RESTORE;
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
         this->restore_process = NO_RESTORE;
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "SaveRestoreServices::initiate_restore_announce():%d \
After communicating with RTI, restore_process != INITIATE_RESTORE, \
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
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "SaveRestoreServices::complete_restore():%d\n",
                       __LINE__ );
   }

   if ( this->restore_process != RESTORE_IN_PROGRESS ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "SaveRestoreServices::complete_restore():%d Restore Process != RESTORE_IN_PROGRESS.\n",
                          __LINE__ );
      }
      return;
   }

   if ( !this->start_to_restore ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
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

void SaveRestoreServices::check_HLA_save_directory()
{
   // If the save directory is not specified, set it to the current RUN directory
   if ( HLA_save_directory.empty() ) {

      string run_dir = command_line_args_get_output_dir();
      string def_dir = command_line_args_get_default_dir();

      // build a absolute path to the RUN directory by combining default_dir
      // and run_dir from the EXECUTIVE.
      this->HLA_save_directory = def_dir + "/" + run_dir;
   }
}

/*! @brief Set the federate has begun execution state. */
void SaveRestoreServices::set_federate_has_begun_execution()
{
   execution_has_begun = true;
   federate->joined_federate_name_map.clear(); // clear out joined federate names
   check_HLA_save_directory();
}
