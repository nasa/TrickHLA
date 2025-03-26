/*!
@file TrickHLA/TrickThreadCoordinator.cpp
@ingroup TrickHLA
@brief This class handles the coordination of Trick Child Threads with the
HLA asynchronous data exchanges and time management.

@copyright Copyright 2023 United States Government as represented by the
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
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{TrickThreadCoordinator.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2023, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, January 2024, --, Added support for child \
thread data cycle time being longer than the main thread data cycle time.}
@revs_end
*/

// System include files.
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/Threads.hh"
#include "trick/exec_proto.h"
#include "trick/exec_proto.hh"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/realtimesync_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/TrickThreadCoordinator.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
TrickThreadCoordinator::TrickThreadCoordinator() // RETURN: -- None.
   : federate( NULL ),
     manager( NULL ),
     mutex(),
     any_child_thread_associated( false ),
     disable_thread_ids( NULL ),
     thread_cnt( 0 ),
     thread_state( NULL ),
     data_cycle_base_time_per_thread( NULL ),
     data_cycle_base_time_per_obj( NULL ),
     main_thread_data_cycle_base_time( 0LL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
TrickThreadCoordinator::~TrickThreadCoordinator() // RETURN: -- None.
{
   if ( this->disable_thread_ids != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->disable_thread_ids ) ) ) {
         message_publish( MSG_WARNING, "TrickThreadCoordinator::~TrickThreadCoordinator():%d WARNING failed to delete Trick Memory for 'this->disable_thread_ids'\n",
                          __LINE__ );
      }
      this->disable_thread_ids = NULL;
   }

   // Release the arrays.
   if ( this->thread_state != NULL ) {
      this->thread_cnt = 0;
      if ( trick_MM->delete_var( static_cast< void * >( this->thread_state ) ) ) {
         message_publish( MSG_WARNING, "TrickThreadCoordinator::~TrickThreadCoordinator():%d WARNING failed to delete Trick Memory for 'this->thread_state'\n",
                          __LINE__ );
      }
      this->thread_state = NULL;
   }
   if ( this->data_cycle_base_time_per_thread != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->data_cycle_base_time_per_thread ) ) ) {
         message_publish( MSG_WARNING, "TrickThreadCoordinator::~TrickThreadCoordinator():%d WARNING failed to delete Trick Memory for 'this->data_cycle_base_time_per_thread'\n",
                          __LINE__ );
      }
      this->data_cycle_base_time_per_thread = NULL;
   }
   if ( this->data_cycle_base_time_per_obj != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->data_cycle_base_time_per_obj ) ) ) {
         message_publish( MSG_WARNING, "TrickThreadCoordinator::~TrickThreadCoordinator():%d WARNING failed to delete Trick Memory for 'this->data_cycle_base_time_per_obj'\n",
                          __LINE__ );
      }
      this->data_cycle_base_time_per_obj = NULL;
   }

   // Make sure we destroy the mutex.
   mutex.destroy();
}

/*!
 * @brief Setup the required class instance associations.
 * @param federate Associated TrickHLA::Federate class instance.
 * @param manager  Associated TrickHLA::Manager class instance.
 */
void TrickThreadCoordinator::setup(
   Federate &federate,
   Manager  &manager )
{
   // Set the associated TrickHLA Federate and Manager references.
   this->federate = &federate;
   this->manager  = &manager;
}

/*!
 * @brief Initialize the thread memory associated with the Trick child threads.
 */
void TrickThreadCoordinator::initialize(
   double const main_thread_data_cycle_time )
{
   // Determine if the main_thread_data_cycle_time time needs a resolution that
   // exceeds the configured base time.
   if ( Int64BaseTime::exceeds_base_time_resolution( main_thread_data_cycle_time ) ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
             << " ERROR: The main_thread_data_cycle_time specified (thread-id:0, "
             << setprecision( 18 ) << main_thread_data_cycle_time
             << " seconds) requires more resolution"
             << " than whole " << Int64BaseTime::get_units()
             << ". The HLA Logical Time is a 64-bit integer"
             << " representing " << Int64BaseTime::get_units()
             << " and cannot represent the Trick main thread data-cycle time of "
             << setprecision( 18 )
             << ( main_thread_data_cycle_time * Int64BaseTime::get_base_time_multiplier() )
             << " " << Int64BaseTime::get_units() << ". You can adjust the"
             << " base HLA Logical Time resolution by setting"
             << " 'THLA.federate.HLA_time_base_units = trick."
             << Int64BaseTime::get_units_string(
                   Int64BaseTime::best_base_time_resolution( main_thread_data_cycle_time ) )
             << "' or 'federate.set_HLA_base_time_units( "
             << Int64BaseTime::get_units_string(
                   Int64BaseTime::best_base_time_resolution( main_thread_data_cycle_time ) )
             << " )' in your input.py file. The current HLA base time resolution is "
             << Int64BaseTime::get_units_string( Int64BaseTime::get_base_units() )
             << ". You also need to update both the Federation Execution"
             << " Specific Federation Agreement (FESFA) and Federate Compliance"
             << " Declaration (FCD) documents for your Federation to document"
             << " the change in timing class resolution.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine if the Trick time Tic can represent the job cycle time.
   if ( Int64BaseTime::exceeds_base_time_resolution( main_thread_data_cycle_time, exec_get_time_tic_value() ) ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
             << " ERROR: The main_thread_data_cycle_time specified (thread-id:0, "
             << setprecision( 18 ) << main_thread_data_cycle_time
             << " seconds) requires more resolution"
             << " than the Trick time Tic value (" << exec_get_time_tic_value()
             << "). Please update the Trick time tic value in your input.py"
             << " file (i.e. by calling 'trick.exec_set_time_tic_value()').\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( this->thread_state != NULL ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
             << " ERROR: This function can only be called once. Detected the"
             << " this->thread_state variable is already allocated memory and"
             << " is not NULL.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Set the Trick main thread data cycle time.
   this->main_thread_data_cycle_base_time = Int64BaseTime::to_base_time( main_thread_data_cycle_time );

   // Verify the thread state data cycle time.
   if ( this->main_thread_data_cycle_base_time <= 0 ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
             << " ERROR: main_thread_data_cycle_time time ("
             << setprecision( 18 ) << main_thread_data_cycle_time
             << ") must be > 0.0!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine the total number of Trick threads (main + child).
   this->thread_cnt = exec_get_num_threads();

   // Protect against the thread count being unexpectedly zero and should be
   // at least 1 for the Trick main thread.
   if ( this->thread_cnt == 0 ) {
      this->thread_cnt = 1;
   }

   // Allocate the thread state array for all the Trick threads (main + child).
   this->thread_state = static_cast< unsigned int * >( TMM_declare_var_1d( "unsigned int", this->thread_cnt ) );
   if ( this->thread_state == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
             << " ERROR: Could not allocate memory for 'thread_state'"
             << " for requested size " << this->thread_cnt
             << "!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // We don't know if the Child threads are running TrickHLA jobs yet so
   // mark them all as not associated.
   for ( unsigned int thread_id = 0; thread_id < this->thread_cnt; ++thread_id ) {
      this->thread_state[thread_id] = TrickHLA::THREAD_STATE_NOT_ASSOCIATED;
   }

   // Disable Trick thread ID associations as configured in the input file.
   // This will override the compile-time associations in the S_define file.
   if ( this->disable_thread_ids != NULL ) {
      // Break up the comma separated thread-IDs list into a vector.
      std::vector< std::string > thread_id_vec;
      StringUtilities::tokenize( this->disable_thread_ids, thread_id_vec, "," );

      // Process each of the thread-ID's and convert from a string to an integer.
      for ( unsigned int k = 0; k < thread_id_vec.size(); ++k ) {

         // Convert the string to an integer.
         stringstream sstream;
         sstream << thread_id_vec[k];
         long long thread_id;
         sstream >> thread_id;

         if ( ( thread_id >= 1 ) && ( thread_id < this->thread_cnt ) ) {
            // Disable any Trick child thread associations for this ID
            // and including any API's for this child thread.
            this->thread_state[thread_id] = TrickHLA::THREAD_STATE_DISABLED;

            if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
               message_publish( MSG_NORMAL, "TrickThreadCoordinator::initialize():%d Disabled Trick child thread association (thread-id:%d).\n",
                                __LINE__, thread_id );
            }
         } else if ( thread_id == 0 ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
                   << " ERROR: The Trick thread-ID '" << thread_id_vec[k]
                   << "' specified in the input file for 'federate.disable_associated_thread_ids'"
                   << " is not valid because the Trick main thread (id:0) cannot"
                   << " be disabled!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         } else {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
                   << " ERROR: The Trick child thread-ID '" << thread_id_vec[k]
                   << "' specified in the input file for 'federate.disable_associated_thread_ids'"
                   << " is not valid because this Trick child thread does not"
                   << " exist in the S_define file! Valid Trick child thread-ID"
                   << " range is 1 to " << ( this->thread_cnt - 1 )
                   << "!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      }
   }

   // Allocate memory for the data cycle times per each thread.
   this->data_cycle_base_time_per_thread = static_cast< long long * >( TMM_declare_var_1d( "long long", this->thread_cnt ) );
   if ( this->data_cycle_base_time_per_thread == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
             << " ERROR: Could not allocate memory for 'data_cycle_base_time_per_thread'"
             << " for requested size " << this->thread_cnt
             << "!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   for ( unsigned int thread_id = 0; thread_id < this->thread_cnt; ++thread_id ) {
      this->data_cycle_base_time_per_thread[thread_id] = 0LL;
   }

   // Allocate memory for the data cycle times per each object instance.
   if ( manager->obj_count > 0 ) {
      this->data_cycle_base_time_per_obj = static_cast< long long * >( TMM_declare_var_1d( "long long", manager->obj_count ) );
      if ( this->data_cycle_base_time_per_obj == NULL ) {
         ostringstream errmsg;
         errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
                << " ERROR: Could not allocate memory for 'data_cycle_base_time_per_obj'"
                << " for requested size " << manager->obj_count
                << "'!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      for ( int obj_index = 0; obj_index < manager->obj_count; ++obj_index ) {
         this->data_cycle_base_time_per_obj[obj_index] = 0LL;
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::initialize():%d Trick main thread (id:0, data_cycle:%.9f).\n",
                       __LINE__, main_thread_data_cycle_time );
   }

   // Associate the Trick main thread. TrickHLA will maintain data coherency
   // for the HLA object instances specified in the input file over the data
   // cycle time specified.
   associate_to_trick_child_thread( 0, main_thread_data_cycle_time );

   if ( !verify_time_constraints( 0, this->main_thread_data_cycle_base_time ) ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize():" << __LINE__
             << " ERROR: Invalid HLA cycle time ("
             << setprecision( 18 ) << main_thread_data_cycle_time
             << " seconds)!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*!
 * @brief Disable the comma separated list of Trick child thread IDs associated to TrickHLA.
 */
void TrickThreadCoordinator::disable_trick_thread_associations(
   char const *thread_ids )
{
   if ( this->disable_thread_ids != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->disable_thread_ids ) ) ) {
         message_publish( MSG_WARNING, "TrickThreadCoordinator::disable_trick_thread_associations():%d WARNING failed to delete Trick Memory for 'this->disable_thread_ids'\n",
                          __LINE__ );
      }
   }
   this->disable_thread_ids = ( thread_ids != NULL ) ? trick_MM->mm_strdup( thread_ids ) : NULL;
}

/*!
 * @brief Associate a Trick child thread with TrickHLA.
 */
void TrickThreadCoordinator::associate_to_trick_child_thread(
   unsigned int const thread_id,
   double const       data_cycle )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   // Verify the TrickThreadCoordinator::initialize() function was called as
   // required before this function is called by checking if the thread count
   // was initialized.
   if ( ( this->thread_cnt == 0 ) || ( this->thread_state == NULL ) ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: Federate::initialize_thread_state() must be called"
             << " once before calling this function.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify the total Trick thread count (main + child).
   if ( this->thread_cnt != exec_get_num_threads() ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: The total number of Trick threads "
             << exec_get_num_threads() << " (main + child threads) does"
             << " not match the number (" << this->thread_cnt
             << ") we initialized to in TrickThreadCoordinator::initialize()"
             << " for the specified thread-id:" << thread_id << '\n';
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify the child thread-id specified is in range.
   if ( thread_id >= this->thread_cnt ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: Total Trick thread count " << this->thread_cnt
             << " (main + child threads), Invalid specified "
             << ( ( thread_id == 0 ) ? "main" : "child" )
             << " thread-id:" << thread_id << '\n';
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Just return if this trick child thread association has been disabled.
   if ( this->thread_state[thread_id] == TrickHLA::THREAD_STATE_DISABLED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
         message_publish( MSG_NORMAL, "TrickThreadCoordinator::associate_to_trick_child_thread():%d Disabled thread_id:%d.\n",
                          __LINE__, thread_id );
      }
      return;
   }

   // We do not support more than one thread association to the same thread-id.
   if ( this->thread_state[thread_id] != TrickHLA::THREAD_STATE_NOT_ASSOCIATED ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: You can not associate the same Trick "
             << ( ( thread_id == 0 ) ? "main" : "child" )
             << " thread (thread-id:" << thread_id
             << ") more than once with TrickHLA!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine if the data_cycle time needs a resolution that exceeds the
   // configured base time.
   if ( Int64BaseTime::exceeds_base_time_resolution( data_cycle ) ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: The data_cycle time specified (thread-id:" << thread_id
             << ", data_cycle:" << setprecision( 18 ) << data_cycle
             << " seconds) requires more resolution than whole "
             << Int64BaseTime::get_units()
             << ". The HLA Logical Time is a 64-bit integer representing "
             << Int64BaseTime::get_units()
             << " and cannot represent the Trick child thread data-cycle time of "
             << setprecision( 18 )
             << ( data_cycle * Int64BaseTime::get_base_time_multiplier() )
             << " " << Int64BaseTime::get_units() << ". You can adjust the"
             << " base HLA Logical Time resolution by setting"
             << " 'THLA.federate.HLA_time_base_units = trick."
             << Int64BaseTime::get_units_string(
                   Int64BaseTime::best_base_time_resolution( data_cycle ) )
             << "' in your input.py file. The current HLA base time resolution is "
             << Int64BaseTime::get_units_string( Int64BaseTime::get_base_units() )
             << ". You also need to update both the Federation Execution"
             << " Specific Federation Agreement (FESFA) and Federate Compliance"
             << " Declaration (FCD) documents for your Federation to document"
             << " the change in timing class resolution.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine if the Trick time Tic can represent the job cycle time.
   if ( Int64BaseTime::exceeds_base_time_resolution( data_cycle, exec_get_time_tic_value() ) ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: The data_cycle time specified (thread-id:" << thread_id
             << ", data_cycle:" << setprecision( 18 ) << data_cycle
             << " seconds) requires more resolution than the Trick time Tic value ("
             << setprecision( 18 ) << exec_get_time_tic_value()
             << "). Please update the Trick time tic value in your input"
             << " file (i.e. by calling 'trick.exec_set_time_tic_value()').\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   int64_t const data_cycle_base_time = Int64BaseTime::to_base_time( data_cycle );

   // Verify all the time constraints for this thread data cycle time are valid.
   if ( !verify_time_constraints( thread_id, data_cycle_base_time ) ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: The data_cycle time specified (thread-id:" << thread_id
             << ", data_cycle:" << setprecision( 18 ) << data_cycle
             << " seconds) requires more resolution than the Trick time Tic value ("
             << setprecision( 18 ) << exec_get_time_tic_value()
             << "). Please update the Trick time tic value in your input"
             << " file (i.e. by calling 'trick.exec_set_time_tic_value()').\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   ostringstream summary;
   summary << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
           << " Summary:\n";

   // Search all the objects for a thread ID match and configure data arrays.
   bool any_valid_thread_id_found = false;
   for ( int obj_index = 0; obj_index < manager->obj_count; ++obj_index ) {

      // Is this thread associated to this object.
      if ( manager->objects[obj_index].is_thread_associated( thread_id ) ) {

         if ( ( this->data_cycle_base_time_per_thread[thread_id] > 0LL )
              && ( this->data_cycle_base_time_per_thread[thread_id] != data_cycle_base_time ) ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
                   << " ERROR: For the object instance name '"
                   << manager->objects[obj_index].get_name() << "', the Trick "
                   << ( ( thread_id == 0 ) ? "main" : "child" )
                   << " thread (thread-id:" << thread_id << ", data_cycle:"
                   << setprecision( 18 )
                   << Int64BaseTime::to_seconds( this->data_cycle_base_time_per_thread[thread_id] )
                   << ") does not match the data cycle time specified:"
                   << setprecision( 18 ) << data_cycle << ". A Trick "
                   << ( ( thread_id == 0 ) ? "" : "child" )
                   << " thread must use the same data cycle time across all"
                   << " associated objects so that TrickHLA can properly"
                   << " ensure data coherency.\n";
            DebugHandler::terminate_with_message( errmsg.str() );

         } else if ( ( this->data_cycle_base_time_per_obj[obj_index] > 0LL )
                     && ( this->data_cycle_base_time_per_obj[obj_index] != data_cycle_base_time ) ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
                   << " ERROR: For the object instance name '"
                   << manager->objects[obj_index].get_name()
                   << "', an existing entry for this"
                   << " object (thread-id:" << thread_id << ", data_cycle:"
                   << setprecision( 18 )
                   << Int64BaseTime::to_seconds( this->data_cycle_base_time_per_thread[thread_id] )
                   << ") has a data cycle time that does not match the"
                   << " data cycle time specified:" << data_cycle
                   << ". An object instance must use the same data cycle"
                   << " time across all threads so that TrickHLA can properly"
                   << " ensure data coherency.\n";
            DebugHandler::terminate_with_message( errmsg.str() );

         } else {
            summary << "  thread-id:" << thread_id
                    << "  data_cycle:" << setprecision( 18 ) << data_cycle
                    << "  obj-instance:'" << manager->objects[obj_index].get_name()
                    << "'\n";

            any_valid_thread_id_found = true;

            this->data_cycle_base_time_per_thread[thread_id] = data_cycle_base_time;
            this->data_cycle_base_time_per_obj[obj_index]    = data_cycle_base_time;
         }
      }
   }

   if ( !any_valid_thread_id_found ) {
      summary << "  (No objects explicitly associated to thread-id:"
              << thread_id << ")\n";
   }
   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }

   // If the data cycle time for this child thread does not match the
   // main thread data cycle time then the user must specify all the valid
   // HLA object instance names associated to this child thread.
   if ( ( data_cycle_base_time != this->main_thread_data_cycle_base_time )
        && !any_valid_thread_id_found ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: For the Trick " << ( ( thread_id == 0 ) ? "main" : "child" )
             << " thread (thread-id:" << thread_id
             << ") specified, you have specified a data cycle time ("
             << setprecision( 18 ) << data_cycle
             << ") that differs from the Trick main thread data"
             << " cycle time ("
             << setprecision( 18 )
             << Int64BaseTime::to_seconds( this->main_thread_data_cycle_base_time )
             << "). This requires you to specify all the HLA object instance"
             << " names associated with this Trick "
             << ( ( thread_id == 0 ) ? "main" : "child" ) << " thread so that TrickHLA"
             << " can properly ensure data coherency.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make sure we mark the thread state as reset now that we associated to it.
   this->thread_state[thread_id] = TrickHLA::THREAD_STATE_RESET;

   if ( thread_id == 0 ) {
      // Ensure we set the data cycle time for the main thread even if no
      // object instance names were specified.
      this->data_cycle_base_time_per_thread[0] = this->main_thread_data_cycle_base_time;
   } else {
      // We now have at least one Trick child thread associated to TrickHLA.
      this->any_child_thread_associated = true;

      Trick::Threads const *child_thread = exec_get_exec_cpp()->get_thread( thread_id );

      // TrickHLA only supports certain Trick child thread process-types.
      switch ( child_thread->process_type ) {
         case Trick::PROCESS_TYPE_SCHEDULED: {
            // Supported but may result in unintended overruns depending on
            // how the users thread job cycle times are configured versus the
            // data cycle time specified in this thread association. We trust
            // the user knows what they are doing even though AMF threads may
            // be a better option.
            break;
         }
         case Trick::PROCESS_TYPE_ASYNC_CHILD: {
            // TrickHLA does not support Trick Asynchronous child threads
            // because the job scheduling is not compatible.
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
                   << " ERROR: The Trick child thread (thread-id:" << thread_id
                   << ") specified is configured as an Asynchronous Trick child"
                   << " thread, which is not compatible with job scheduling needs"
                   << " of TrickHLA to coordinate HLA data between threads at"
                   << " cycle boundaries. Consider configuring the Trick child"
                   << " thread as Asynchronous Must Finish (AMF) instead. Please"
                   << " add or update directives like the following in your"
                   << " input.py file to configure the Trick child thread with"
                   << " an AMF process-type and an AMF cycle time that matches"
                   << " the data cycle time specified for this thread association:\n"
                   << "trick.exec_set_thread_process_type( " << thread_id
                   << ", trick.PROCESS_TYPE_AMF_CHILD )\n"
                   << "trick.exec_set_thread_amf_cycle_time( " << thread_id << ", "
                   << setprecision( 18 ) << data_cycle << " )\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         case Trick::PROCESS_TYPE_AMF_CHILD: {
            // For Asynchronous Must Finish (AMF) configured threads, the AMF cycle
            // time needs to match the data cycle time specified in the association.
            if ( child_thread->amf_cycle != data_cycle ) {
               ostringstream errmsg;
               errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
                      << " ERROR: The Trick child thread (thread-id:" << thread_id
                      << ") specified is configured as Asynchronous Must Finish"
                      << " (AMF) with an AMF cycle time of "
                      << setprecision( 18 ) << child_thread->amf_cycle
                      << " seconds. However, this Trick child thread was"
                      << " associated to TrickHLA with a data cycle time of "
                      << setprecision( 18 ) << data_cycle
                      << " seconds. Please add or update directives like the"
                      << " following in your input.py file to configure the"
                      << " Trick child thread with an AMF process-type and an"
                      << " AMF cycle time that matches the data cycle time"
                      << " specified for this thread association:\n"
                      << "trick.exec_set_thread_process_type( " << thread_id
                      << ", trick.PROCESS_TYPE_AMF_CHILD )\n"
                      << "trick.exec_set_thread_amf_cycle_time( " << thread_id << ", "
                      << setprecision( 18 ) << data_cycle << " )\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }
            break;
         }
         default: {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
                   << " ERROR: Unknown Trick child thread type (process_type:"
                   << child_thread->process_type << ") for (thread-id:"
                   << thread_id << ")!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
      }
   }
}

/*!
 * @brief Verify the threads IDs associated to objects in the input file.
 * */
void TrickThreadCoordinator::verify_trick_thread_associations()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      ostringstream summary;
      summary << "TrickThreadCoordinator::verify_trick_thread_associations():"
              << __LINE__;

      if ( !this->any_child_thread_associated ) {
         // This status message only makes sense only if we have simulation
         // with Trick child threads.
         if ( this->thread_cnt > 1 ) {
            summary << " Detected no Trick child threads associated to any"
                    << " object instances configured in the input file.";
         }
         summary << '\n';
      } else {
         summary << " Summary of object instance and thread associations configured"
                 << " in the input file:\n";

         // Summary of the Object-instances per thread-ID.
         summary << "ThreadID  Cycle  Object-Instances\n";
         for ( unsigned int thread_id = 0; thread_id < this->thread_cnt; ++thread_id ) {
            summary << thread_id << "\t  ";

            switch ( this->thread_state[thread_id] ) {
               case TrickHLA::THREAD_STATE_DISABLED: {
                  summary << "(Disabled thread associated to TrickHLA)";
                  break;
               }
               case TrickHLA::THREAD_STATE_NOT_ASSOCIATED: {
                  summary << "(Thread not associated to TrickHLA)";
                  break;
               }
               default: {
                  summary << setprecision( 18 )
                          << Int64BaseTime::to_seconds( this->data_cycle_base_time_per_thread[thread_id] )
                          << "\t ";
                  for ( int obj_index = 0; obj_index < manager->obj_count; ++obj_index ) {
                     if ( manager->objects[obj_index].is_thread_associated( thread_id ) ) {
                        summary << "'" << manager->objects[obj_index].get_name() << "' ";
                     }
                  }
                  break;
               }
            }
            summary << '\n';
         }

         // Summary of the thread-ID's per object instance.
         summary << "Object-Instance   ThreadIDs\n";
         for ( int obj_index = 0; obj_index < manager->obj_count; ++obj_index ) {
            summary << "'" << manager->objects[obj_index].get_name() << "'\t  ";
            bool printed_thread_id = false;
            for ( unsigned int thread_id = 0; thread_id < manager->objects[obj_index].thread_ids_array_count; ++thread_id ) {
               if ( manager->objects[obj_index].thread_ids_array[thread_id] ) {
                  if ( printed_thread_id ) {
                     summary << ", ";
                  }
                  summary << thread_id;
                  printed_thread_id = true;
               }
            }
            summary << '\n';
         }
      }
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }

   // Verify every thread ID specified in the input file for each object has a
   // Trick child thread association made in the S_define file.
   for ( int obj_index = 0; obj_index < manager->obj_count; ++obj_index ) {
      for ( unsigned int thread_id = 0; thread_id < manager->objects[obj_index].thread_ids_array_count; ++thread_id ) {

         if ( ( this->thread_state[thread_id] != TrickHLA::THREAD_STATE_DISABLED )
              && manager->objects[obj_index].thread_ids_array[thread_id]
              && ( this->data_cycle_base_time_per_thread[thread_id] == 0LL ) ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::verify_trick_thread_associations():"
                   << __LINE__ << " ERROR: Object instance '"
                   << manager->objects[obj_index].get_name()
                   << "' specified a Trick thread-ID:" << thread_id << ", but no thread"
                   << " with this ID was associated in the S_define file!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      }
   }
}

/*!
 * @brief Announce all the HLA data was sent.
 */
void TrickThreadCoordinator::announce_data_available()
{
   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::announce_data_available():%d\n",
                       __LINE__ );
   }

   // Process Trick child thread states associated to TrickHLA.
   if ( this->any_child_thread_associated ) {

      int64_t const sim_time_base_time = Int64BaseTime::to_base_time( exec_get_sim_time() );

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      // Process all the Trick child threads associated to TrickHLA first
      // and only for threads on the data cycle time boundary.
      for ( unsigned int thread_id = 1; thread_id < this->thread_cnt; ++thread_id ) {
         if ( is_enabled_child_thread_association( thread_id )
              && on_receive_data_cycle_boundary_for_thread( thread_id, sim_time_base_time ) ) {

            this->thread_state[thread_id] = TrickHLA::THREAD_STATE_READY_TO_RECEIVE;
         }
      }

      // Set the state of the Trick main thread last.
      this->thread_state[0] = TrickHLA::THREAD_STATE_READY_TO_RECEIVE;
   }
}

/*!
 * @brief Announce all the HLA data was sent.
 */
void TrickThreadCoordinator::announce_data_sent()
{
   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::announce_data_sent():%d\n",
                       __LINE__ );
   }

   // Process Trick child thread states associated to TrickHLA.
   if ( this->any_child_thread_associated ) {

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      // Set the state of the main thread as ready to send.
      this->thread_state[0] = TrickHLA::THREAD_STATE_READY_TO_SEND;
   }
}

/*!
 * @brief Wait for the HLA data to be sent if a Trick child thread or if the
 * calling thread is the Trick main thread then wait for all associated Trick
 * child threads to have called this function.
 */
void TrickThreadCoordinator::wait_to_send_data()
{

   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_send_data():%d\n",
                       __LINE__ );
   }

   // Only wait to send data for child threads if any are associated.
   if ( this->any_child_thread_associated ) {

      // Get the ID of the thread that called this function.
      unsigned int const thread_id = exec_get_process_id();

      // Determine if this is the main thread (id = 0) or a child thread. The
      // main thread will wait for all the child threads to be ready to send
      // before returning.
      if ( thread_id == 0 ) {
         wait_to_send_data_for_main_thread();
      } else {
         wait_to_send_data_for_child_thread( thread_id );
      }
   }
}

/*!
 * @brief Trick main thread will wait for all associated Trick child threads
 * to have called the wait_to_send_data() function from S_define sim-object
 * to indicate they are ready to send data.
 */
void TrickThreadCoordinator::wait_to_send_data_for_main_thread()
{
   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_send_data_for_main_thread():%d Waiting...\n",
                       __LINE__ );
   }

   // The main thread will wait for all the child threads to be ready to send
   // before returning.

   // Simulation time of the main thread.
   int64_t const sim_time_in_base_time = Int64BaseTime::to_base_time( exec_get_sim_time() );

   // Don't check the Trick main thread (id = 0), only check child threads.
   unsigned int thread_id = 1;

   // Trick Main Thread: Take a quick first look to determine if all the
   // Trick child threads associated to TrickHLA are ready to send data.
   // If all the child threads are ready to send data then this quick look
   // will return faster than the more involved spin-lock code section
   // further below with the sleep code.
   bool all_ready_to_send = true;
   {
      // When auto_unlock_mutex goes out of scope it automatically
      // unlocks the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      // Check all the associated thread-id's.
      while ( ( thread_id < this->thread_cnt ) && all_ready_to_send ) {

         // If the state is THREAD_STATE_DISABLED or
         // THREAD_STATE_NOT_ASSOCIATED then there are no TrickHLA jobs on this
         // thread, so move on to the next thread-id. If the state is
         // THREAD_STATE_READY_TO_SEND then this thread-id is ready, so check
         // the next ID. Otherwise we are not ready to send and don't move on
         // from the current thread-id. Skip this child thread if it is not
         // scheduled to run at the same time as the main thread for this job.
         // Also skip if this thread is not on a data cycle boundary.
         if ( ( this->thread_state[thread_id] == TrickHLA::THREAD_STATE_READY_TO_SEND )
              || ( this->thread_state[thread_id] == TrickHLA::THREAD_STATE_DISABLED )
              || ( this->thread_state[thread_id] == TrickHLA::THREAD_STATE_NOT_ASSOCIATED )
              || !on_send_data_cycle_boundary_for_thread( thread_id, sim_time_in_base_time ) ) {

            // Move to the next thread-id because the current ID is
            // ready. This results in checking all the ID's just once.
            ++thread_id;
         } else {
            // Stay on the current ID and indicate not ready to send.
            all_ready_to_send = false;
         }
      }
   }

   // If the quick look was not successful do a more involved spin-lock with
   // sleeps, which adds more wait latency.
   if ( !all_ready_to_send ) {

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for all Trick child threads associated to TrickHLA to be
      // ready to send data.
      do {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         // Determine if all the Trick child threads are ready to send data.
         all_ready_to_send = true;
         {
            // When auto_unlock_mutex goes out of scope it automatically
            // unlocks the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &mutex );

            // Check all the associated thread-id's.
            while ( ( thread_id < this->thread_cnt ) && all_ready_to_send ) {

               // If the state is THREAD_STATE_DISABLED or
               // THREAD_STATE_NOT_ASSOCIATED then there are no TrickHLA jobs
               // on this thread, so move on to the next thread-id. If the state
               // is THREAD_STATE_READY_TO_SEND then this thread-id is ready,
               // so check the next ID. Otherwise we are not ready to send and
               // don't move on from the current thread-id. Skip this child
               // thread if it is not scheduled to run at the same time as the
               // main thread for this job.
               // Also skip if this thread is not on a data cycle boundary.
               if ( ( this->thread_state[thread_id] == TrickHLA::THREAD_STATE_READY_TO_SEND )
                    || ( this->thread_state[thread_id] == TrickHLA::THREAD_STATE_DISABLED )
                    || ( this->thread_state[thread_id] == TrickHLA::THREAD_STATE_NOT_ASSOCIATED )
                    || !on_send_data_cycle_boundary_for_thread( thread_id, sim_time_in_base_time ) ) {

                  // Move to the next thread-id because the current ID is
                  // ready. This results in checking all the ID's just once.
                  ++thread_id;
               } else {
                  // Stay on the current ID and indicate not ready to send.
                  all_ready_to_send = false;
               }
            }
         }

         if ( !all_ready_to_send ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "TrickThreadCoordinator::wait_to_send_data_for_main_thread():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_send_data_for_main_thread():%d Trick main thread, waiting on child thread %d...\n",
                                __LINE__, thread_id );
            }
         }
      } while ( !all_ready_to_send );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_send_data_for_main_thread():%d Done\n",
                       __LINE__ );
   }
}

/*!
 * @brief Wait for the HLA data to be sent for all Trick child threads.
 */
void TrickThreadCoordinator::wait_to_send_data_for_child_thread(
   unsigned int const thread_id )
{
   // Just return if this thread association is disabled.
   if ( this->thread_state[thread_id] == TrickHLA::THREAD_STATE_DISABLED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
         message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_send_data_for_child_thread():%d Child Thread:%d, Disabled, Done\n",
                          __LINE__, thread_id );
      }
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_send_data_for_child_thread():%d Child Thread:%d, waiting...\n",
                       __LINE__, thread_id );
   }

   // Trick Child Threads associated to TrickHLA need to wait for the Trick
   // main thread to send all the HLA data.
   //
   // Do a quick look to determine if the Trick main thread has sent all
   // the HLA data.
   bool sent_data;
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      // Mark this child thread as ready to send.
      this->thread_state[thread_id] = TrickHLA::THREAD_STATE_READY_TO_SEND;

      // Determine if all the data has been sent by the main thread.
      sent_data = ( this->thread_state[0] == TrickHLA::THREAD_STATE_READY_TO_SEND );
   }

   // If the quick look to see if the main thread has announced it has sent
   // the data has not succeeded then do a more involved spin-lock with a
   // sleep. This will have more wait latency.
   if ( !sent_data ) {

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the main thread to have sent the data.
      do {
         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            // When auto_unlock_mutex goes out of scope it automatically
            // unlocks the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &mutex );

            sent_data = ( this->thread_state[0] == TrickHLA::THREAD_STATE_READY_TO_SEND );
         }

         if ( !sent_data ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "TrickThreadCoordinator::wait_to_send_data_for_child_thread():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_send_data_for_child_thread():%d Child Thread:%d, waiting...\n",
                                __LINE__, thread_id );
            }
         }
      } while ( !sent_data );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_send_data_for_child_thread():%d Child Thread:%d, Done\n",
                       __LINE__, thread_id );
   }
}

/*! @brief Wait to receive data when the Trick main thread is ready. */
void TrickThreadCoordinator::wait_to_receive_data()
{
   // Don't process Trick child thread states associated to TrickHLA if none exist.
   if ( !this->any_child_thread_associated ) {
      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
         message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_receive_data():%d Done\n",
                          __LINE__ );
      }
      return;
   }

   // Get the ID of the thread that called this function.
   unsigned int const thread_id = exec_get_process_id();

   // Just return if this thread association is disabled.
   if ( this->thread_state[thread_id] == TrickHLA::THREAD_STATE_DISABLED ) {
      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
         message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_receive_data():%d %s Thread:%d, Disabled, Done\n",
                          __LINE__, ( ( thread_id == 0 ) ? "Main" : "Child" ),
                          thread_id );
      }
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_receive_data():%d %s Thread:%d, waiting...\n",
                       __LINE__, ( ( thread_id == 0 ) ? "Main" : "Child" ),
                       thread_id );
   }

   bool ready_to_receive;
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      ready_to_receive = ( this->thread_state[0] == TrickHLA::THREAD_STATE_READY_TO_RECEIVE );
   }

   // See if the main thread has announced it has received data.
   if ( !ready_to_receive ) {

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the main thread to receive data.
      do {
         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            // When auto_unlock_mutex goes out of scope it automatically
            // unlocks the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &mutex );

            ready_to_receive = ( this->thread_state[0] == TrickHLA::THREAD_STATE_READY_TO_RECEIVE );
         }

         if ( !ready_to_receive ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "TrickThreadCoordinator::wait_to_receive_data():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_receive_data():%d %s Thread:%d, waiting...\n",
                                __LINE__, ( ( thread_id == 0 ) ? "Main" : "Child" ),
                                thread_id );
            }
         }
      } while ( !ready_to_receive );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_THREAD_COORDINATOR ) ) {
      message_publish( MSG_NORMAL, "TrickThreadCoordinator::wait_to_receive_data():%d %s Thread:%d, Done\n",
                       __LINE__, ( ( thread_id == 0 ) ? "Main" : "Child" ),
                       thread_id );
   }
}

/*! @brief On boundary if sim-time is an integer multiple of a valid cycle-time. */
bool const TrickThreadCoordinator::on_receive_data_cycle_boundary_for_obj(
   int const     obj_index,
   int64_t const sim_time_in_base_time ) const
{
   // On boundary if sim-time is an integer multiple of a valid cycle-time.
   return ( ( this->any_child_thread_associated
              && ( obj_index < manager->obj_count )
              && ( this->data_cycle_base_time_per_obj[obj_index] > 0LL ) )
               ? ( ( sim_time_in_base_time % this->data_cycle_base_time_per_obj[obj_index] ) == 0LL )
               : true );
}

/*! @brief Get the data cycle time for the specified object index, otherwise
 * return the default data cycle time. */
int64_t const TrickThreadCoordinator::get_data_cycle_base_time_for_obj(
   int const     obj_index,
   int64_t const default_data_cycle_base_time ) const
{
   return ( this->any_child_thread_associated
            && ( obj_index < manager->obj_count )
            && ( this->data_cycle_base_time_per_obj[obj_index] > default_data_cycle_base_time ) )
             ? this->data_cycle_base_time_per_obj[obj_index]
             : default_data_cycle_base_time;
}

bool const TrickThreadCoordinator::verify_time_constraints()
{
   bool verified = true;
   for ( unsigned int thread_id = 0; thread_id < this->thread_cnt; ++thread_id ) {
      if ( !verify_time_constraints( thread_id, this->data_cycle_base_time_per_thread[thread_id] ) ) {
         verified = false;
      }
   }
   return verified;
}

/*!
 * @brief The following relationships between the Trick realtime-frame (RT),
 * Least Common Time Step (LCTS), and lookahead (L) times must hold True
 * and we advance HLA logical time with a dt time step per child thread:
 *
 * (LCTS ≥ RT) ∧ (LCTS % RT = 0) ∧
 * N
 * ∧ (dt[i] > 0) ∧ (dt[i] ≥ L[i]) ∧ (LCTS ≥ dt[i]) ∧ (LCTS % dt[i] = 0)
 * i=1
 */
bool const TrickThreadCoordinator::verify_time_constraints(
   unsigned int const thread_id,
   int64_t const      data_cycle_base_time )
{
   // Verify the child thread-id specified is in range.
   if ( thread_id >= this->thread_cnt ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
             << " ERROR: Total Trick thread count " << this->thread_cnt
             << " (main + child threads), Invalid specified "
             << ( ( thread_id == 0 ) ? "main" : "child" )
             << " thread-id:" << thread_id << '\n';
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Lookahead and LCTS times in the integer base time.
   int64_t const lookahead_base_time = federate->get_lookahead_in_base_time();
   int64_t const lcts_base_time      = manager->get_execution_control()->get_least_common_time_step();

   // Verify the lookahead time.
   if ( lookahead_base_time < 0 ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
             << " ERROR: Lookahead time must be greater than or equal zero (lookahead:"
             << setprecision( 18 ) << Int64BaseTime::to_seconds( lookahead_base_time )
             << " seconds)!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return false;
   }

   // Perform the time checks if this is the main thread or if the child thread
   // is associated to TrickHLA and is enabled.
   if ( ( thread_id == 0 ) || is_enabled_child_thread_association( thread_id ) ) {

      // Time constraint: (dt[i] > 0)
      if ( data_cycle_base_time <= 0 ) {
         ostringstream errmsg;
         errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                << " ERROR: The data cycle time for the Trick "
                << ( ( thread_id == 0 ) ? "main" : "child" )
                << " thread (thread-id:" << thread_id << ", data_cycle:"
                << setprecision( 18 ) << Int64BaseTime::to_seconds( data_cycle_base_time )
                << " seconds) cannot be less than or equal zero (0)!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }

      // Time constraint: (dt[i] ≥ L[i])
      if ( data_cycle_base_time < lookahead_base_time ) {
         ostringstream errmsg;
         errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                << " ERROR: The data cycle time for the Trick "
                << ( ( thread_id == 0 ) ? "main" : "child" )
                << " thread (thread-id:" << thread_id << ", data_cycle:"
                << setprecision( 18 ) << Int64BaseTime::to_seconds( data_cycle_base_time )
                << " seconds) cannot be less than the lookahead time ("
                << setprecision( 18 ) << Int64BaseTime::to_seconds( lookahead_base_time )
                << " seconds)!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }

      // The child thread data cycle time cannot be less than (i.e. faster)
      // than the main thread cycle time.
      if ( data_cycle_base_time < this->main_thread_data_cycle_base_time ) {
         ostringstream errmsg;
         errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                << " ERROR: The data cycle time for the Trick "
                << ( ( thread_id == 0 ) ? "main" : "child" ) << " thread (thread-id:"
                << thread_id << ", data_cycle:" << setprecision( 18 )
                << Int64BaseTime::to_seconds( data_cycle_base_time )
                << " seconds) cannot be less than the Trick main thread data cycle"
                << " time (data_cycle:" << setprecision( 18 )
                << Int64BaseTime::to_seconds( this->main_thread_data_cycle_base_time )
                << " seconds)!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }

      // Only allow child threads with a data cycle time that is an integer
      // multiple of the Trick main thread cycle time.
      if ( data_cycle_base_time % this->main_thread_data_cycle_base_time != 0 ) {
         ostringstream errmsg;
         errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                << " ERROR: The data cycle time for the Trick "
                << ( ( thread_id == 0 ) ? "main" : "child" ) << " thread (thread-id:"
                << thread_id << ", data_cycle:" << setprecision( 18 )
                << Int64BaseTime::to_seconds( data_cycle_base_time )
                << " seconds) must be an integer multiple of the Trick main"
                << " thread data cycle time (data_cycle:" << setprecision( 18 )
                << Int64BaseTime::to_seconds( this->main_thread_data_cycle_base_time )
                << " seconds)!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return false;
      }

      // Perform LCTS checks provided a value has been set. This handles the
      // case where the ExCO has not been received yet.
      if ( lcts_base_time > 0 ) {

         // Thread data cycle time must be less than the LCTS to be valid.
         // Time constraint: (LCTS ≥ dt[i])
         if ( lcts_base_time < data_cycle_base_time ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                   << " ERROR: The Least Common Time Step (LCTS:"
                   << setprecision( 18 ) << Int64BaseTime::to_seconds( lcts_base_time )
                   << " seconds) must be greater or equal to the data cycle time"
                   << " for the Trick " << ( ( thread_id == 0 ) ? "main" : "child" )
                   << " thread (thread-id:" << thread_id << ", data_cycle:"
                   << setprecision( 18 ) << Int64BaseTime::to_seconds( data_cycle_base_time )
                   << " seconds)! Please adjust the LCTS set in your input file"
                   << " as appropriate.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }

         // Child thread data cycle time must be an integer multiple of the LCTS.
         // Time constraint: (LCTS % dt[i] = 0)
         if ( lcts_base_time % data_cycle_base_time != 0 ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                   << " ERROR: The Least Common Time Step (LCTS:"
                   << setprecision( 18 ) << Int64BaseTime::to_seconds( lcts_base_time )
                   << " seconds) must be an integer multiple of the data cycle"
                   << " time for the Trick " << ( ( thread_id == 0 ) ? "main" : "child" )
                   << " thread (thread-id:" << thread_id << ", data_cycle:"
                   << setprecision( 18 ) << Int64BaseTime::to_seconds( data_cycle_base_time )
                   << " seconds)! Please adjust the LCTS set in your input"
                   << " file as appropriate.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }
      }
   }

   // We only need to verify these time constraints once so only check for
   // the main thread.
   if ( thread_id == 0 ) {

      // Freeze happens as an end_of_frame job where the frame is defined by
      // the Trick software/realtime frame time so all federates need a Trick
      // software frame time that meets constraints. This also handles the
      // case where the ExCO has not been received yet.
      // (LCTS ≥ RT) ∧ (LCTS % RT = 0)
      if ( lcts_base_time > 0 ) {

         // The Real-Time frame time is the Trick software frame time.
         double const RT_frame = exec_get_software_frame();

         // The Trick software frame must be configured.
         if ( RT_frame <= 0.0 ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                   << " ERROR: The Trick software frame ("
                   << setprecision( 18 ) << RT_frame
                   << ( ( RT_frame == 1.0 ) ? " second)" : " seconds)" )
                   << " cannot be less than or equal to zero!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }

         // Verify the RT frame can be represented in base time.
         if ( Int64BaseTime::exceeds_base_time_resolution( RT_frame ) ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                   << " ERROR: The Trick software frame ("
                   << setprecision( 18 ) << RT_frame
                   << ( ( RT_frame == 1.0 ) ? " second)" : " seconds)" )
                   << " cannot be represented in base-time because it"
                   << " exceeds the base-time resolution of "
                   << Int64BaseTime::get_units() << "!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }

         int64_t const RT_base_time = Int64BaseTime::to_base_time( RT_frame );

         // Time Constraint: (LCTS ≥ RT)
         if ( lcts_base_time < RT_base_time ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                   << " ERROR: The Least Common Time Step (LCTS:"
                   << setprecision( 18 ) << Int64BaseTime::to_seconds( lcts_base_time )
                   << " seconds) must be greater than or equal to the Trick"
                   << " software frame time (" << setprecision( 18 ) << RT_frame
                   << ( ( RT_frame == 1.0 ) ? " second)" : " seconds)" )
                   << " because Trick Real Time is enabled! One solution is to"
                   << " set the Trick software frame to the same time as the"
                   << " LCTS time of " << setprecision( 18 )
                   << Int64BaseTime::to_seconds( lcts_base_time ) << " seconds.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }

         // Time Constraint: (LCTS % RT = 0)
         if ( lcts_base_time % RT_base_time != 0 ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                   << " ERROR: The Least Common Time Step (LCTS:"
                   << setprecision( 18 ) << Int64BaseTime::to_seconds( lcts_base_time )
                   << " seconds) must be an integer multiple of the Trick"
                   << " software frame time (" << setprecision( 18 ) << RT_frame
                   << ( ( RT_frame == 1.0 ) ? " second)" : " seconds)" )
                   << " because Trick Real Time is enabled! One solution is to"
                   << " set the Trick software frame to the same time as the"
                   << " LCTS time of " << setprecision( 18 )
                   << Int64BaseTime::to_seconds( lcts_base_time ) << " seconds.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }
      }

      // For the master federate, verify additional time constraints.
      // Determine if the LCTS is enabled for the Master federate. SpaceFOM
      // requires the use of the LCTS while the TrickHLA simple-init and IMSim
      // schemes do not.
      if ( manager->get_execution_control()->is_master()
           && manager->get_execution_control()->is_enabled_least_common_time_step() ) {

         // Time Constraints: (LCTS > 0)
         if ( lcts_base_time <= 0 ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                   << " ERROR: For this Master federate, the ExCO Least Common"
                   << " Time Step (LCTS:"
                   << lcts_base_time << " " << Int64BaseTime::get_units()
                   << ") must be greater than zero!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }

         // Convert the padding time in seconds to the base-time as an integer.
         int64_t const pad_base_time = Int64BaseTime::to_base_time(
            manager->get_execution_control()->get_time_padding() );

         // Verify the Padding time if one was set.
         if ( pad_base_time > 0 ) {

            // At a minimum the Padding time must be >= LCTS.
            if ( pad_base_time < lcts_base_time ) {
               ostringstream errmsg;
               errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                      << " ERROR: Mode transition padding time ("
                      << pad_base_time << " " << Int64BaseTime::get_units()
                      << ") can not be less than the ExCO Least Common Time Step (LCTS:"
                      << lcts_base_time << " " << Int64BaseTime::get_units()
                      << ")!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
               return false;
            }

            // Padding time must be an integer multiple of the LCTS.
            if ( pad_base_time % lcts_base_time != 0 ) {
               ostringstream errmsg;
               errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                      << " ERROR: Mode transition padding time ("
                      << pad_base_time << " " << Int64BaseTime::get_units()
                      << ") is not an integer multiple of the ExCO"
                      << " Least Common Time Step (LCTS:"
                      << lcts_base_time << " " << Int64BaseTime::get_units()
                      << ")!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
               return false;
            }

            // The Master federate padding time must be at least 3 or more
            // times the Least Common Time Step (LCTS) or two seconds. This
            // will give commands time to propagate through the system and
            // still have time for mode transitions.
            if ( ( pad_base_time < Int64BaseTime::to_base_time( THLA_PADDING_DEFAULT ) )
                 && ( pad_base_time < ( 3 * lcts_base_time ) ) ) {
               ostringstream errmsg;
               errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                      << " ERROR: Mode transition padding time ("
                      << pad_base_time << " " << Int64BaseTime::get_units()
                      << ") is not a multiple of 3 or more of the ExCO"
                      << " Least Common Time Step (LCTS:"
                      << lcts_base_time << " " << Int64BaseTime::get_units()
                      << ") when the time padding is less than "
                      << THLA_PADDING_DEFAULT << " seconds!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
               return false;
            }
         }
      }

      // For a Master federate using CTE, we need to make sure the padding
      // time is at least two times the freeze frame time. Otherwise we can
      // not coordinate a go to run in time.
      if ( manager->get_execution_control()->is_master()
           && manager->get_execution_control()->does_cte_timeline_exist() ) {

         double time_padding = manager->get_execution_control()->get_time_padding();

         if ( time_padding <= ( 2.0 * exec_get_freeze_frame() ) ) {
            ostringstream errmsg;
            errmsg << "TrickThreadCoordinator::verify_time_constraints():" << __LINE__
                   << " ERROR: Mode transition padding time (" << time_padding
                   << " seconds) must be more than two times the Trick freeze"
                   << " frame time (" << exec_get_freeze_frame() << " seconds)!"
                   << " In your input.py file, please update the padding time"
                   << " and/or the Trick freeze frame time using directives"
                   << " like the following:\n"
                   << "   federate.set_time_padding( pad )\n"
                   << "   trick.exec_set_freeze_frame( frame_time )\n"
                   << "For example, adjusting the freeze frame time for the"
                   << " given time padding:\n";
            if ( time_padding > ( 2.0 * exec_get_software_frame() ) ) {
               // Example using the Trick software frame time for the freeze frame.
               errmsg << "   federate.set_time_padding( " << time_padding << " )\n"
                      << "   trick.exec_set_freeze_frame( " << exec_get_software_frame() << " )\n";
            } else {
               errmsg << "   federate.set_time_padding( " << time_padding << " )\n"
                      << "   trick.exec_set_freeze_frame( " << ( time_padding / 4 ) << " )\n";
            }
            DebugHandler::terminate_with_message( errmsg.str() );
            return false;
         }
      }
   }

   return true;
}
