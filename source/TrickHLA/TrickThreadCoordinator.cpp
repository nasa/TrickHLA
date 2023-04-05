/*!
@file TrickHLA/TrickThreadCoordinator.cpp
@ingroup TrickHLA
@brief This class handles the coordination of Trick Child Threads with the
HLA asynchronous data exchanges and time management.

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
@revs_end

*/

// System include files.
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/SleepTimeout.hh"
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
     thread_state( NULL ),
     thread_state_cnt( 0 ),
     data_cycle_micros_per_thread( NULL ),
     data_cycle_micros_per_obj( NULL ),
     any_thread_associated( false ),
     main_thread_data_cycle_micros( 0LL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
TrickThreadCoordinator::~TrickThreadCoordinator() // RETURN: -- None.
{
   // Make sure we unlock the mutex.
   (void)this->mutex.unlock();

   // Release the arrays.
   if ( this->thread_state != NULL ) {
      this->thread_state_cnt = 0;
      if ( TMM_is_alloced( (char *)this->thread_state ) ) {
         TMM_delete_var_a( this->thread_state );
      }
      this->thread_state = NULL;
   }
   if ( this->data_cycle_micros_per_thread != NULL ) {
      if ( TMM_is_alloced( (char *)this->data_cycle_micros_per_thread ) ) {
         TMM_delete_var_a( this->data_cycle_micros_per_thread );
      }
      this->data_cycle_micros_per_thread = NULL;
   }
   if ( this->data_cycle_micros_per_obj != NULL ) {
      if ( TMM_is_alloced( (char *)this->data_cycle_micros_per_obj ) ) {
         TMM_delete_var_a( this->data_cycle_micros_per_obj );
      }
      this->data_cycle_micros_per_obj = NULL;
   }
}

/*!
 * @brief Setup the required class instance associations.
 *  @param federate Associated TrickHLA::Federate class instance.
 *  @param manager  Associated TrickHLA::Manager class instance.
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
void TrickThreadCoordinator::initialize_thread_state(
   double const main_thread_data_cycle_time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   // Set the Trick main thread data cycle time.
   this->main_thread_data_cycle_micros = Int64Interval::to_microseconds( main_thread_data_cycle_time );

   // Verify the thread state data cycle time.
   if ( this->main_thread_data_cycle_micros <= 0 ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize_thread_state():" << __LINE__
             << " ERROR: main_thread_data_cycle_time time ("
             << main_thread_data_cycle_time
             << ") must be > 0.0!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine the total number of Trick threads (main + child).
   this->thread_state_cnt = exec_get_num_threads();

   // Protect against the thread count being unexpectedly zero and should be
   // at least 1 for the Trick main thread.
   if ( this->thread_state_cnt == 0 ) {
      this->thread_state_cnt = 1;
   }

   // Allocate the thread state array for all the Trick threads (main + child).
   this->thread_state = (unsigned int *)TMM_declare_var_1d( "unsigned int", this->thread_state_cnt );
   if ( this->thread_state == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize_thread_state():" << __LINE__
             << " ERROR: Could not allocate memory for 'thread_state'"
             << " for requested size " << this->thread_state_cnt
             << "'!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      exit( 1 );
   }

   //  Trick Main thread runs the core TrickHLA jobs.
   this->thread_state[0] = THREAD_STATE_RESET;

   // We don't know if the Child threads are running TrickHLA jobs yet so
   // mark them all as not associated.
   for ( unsigned int id = 1; id < this->thread_state_cnt; ++id ) {
      this->thread_state[id] = THREAD_STATE_NOT_ASSOCIATED;
   }

   // Allocate memory for the data cycle times per each thread.
   this->data_cycle_micros_per_thread = (long long *)TMM_declare_var_1d( "long long", this->thread_state_cnt );
   if ( this->data_cycle_micros_per_thread == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize_thread_state():" << __LINE__
             << " ERROR: Could not allocate memory for 'data_cycle_micros_per_thread'"
             << " for requested size " << this->thread_state_cnt
             << "'!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      exit( 1 );
   }
   for ( unsigned int id = 0; id < this->thread_state_cnt; ++id ) {
      this->data_cycle_micros_per_thread[id] = 0LL;
   }

   // Allocate memory for the data cycle times per each object instance.
   this->data_cycle_micros_per_obj = (long long *)TMM_declare_var_1d( "long long", this->manager->obj_count );
   if ( this->data_cycle_micros_per_obj == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::initialize_thread_state():" << __LINE__
             << " ERROR: Could not allocate memory for 'data_cycle_micros_per_obj'"
             << " for requested size " << this->manager->obj_count
             << "'!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
      exit( 1 );
   }
   for ( unsigned int obj_index = 0; obj_index < this->manager->obj_count; ++obj_index ) {
      this->data_cycle_micros_per_obj[obj_index] = 0LL;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      send_hs( stdout, "TrickThreadCoordinator::initialize_thread_state():%d Trick main thread (id:0, data_cycle:%.3f).%c",
               __LINE__, main_thread_data_cycle_time, THLA_NEWLINE );
   }
}

/*!
 * @brief Associate a Trick child thread with TrickHLA.
 */
void TrickThreadCoordinator::associate_to_trick_child_thread(
   unsigned int const thread_id,
   double const       data_cycle,
   string const      &obj_insance_names )
{
   int64_t data_cycle_micros = Int64Interval::to_microseconds( data_cycle );

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
          << " Trick child thread (thread-id:" << thread_id
          << ", data_cycle:" << data_cycle << ")";
      if ( obj_insance_names.empty() ) {
         msg << " with no associated object instances." << THLA_ENDL;
      } else {
         msg << " with associated object instances:'" << obj_insance_names
             << "'." << THLA_ENDL;
      }
      send_hs( stdout, (char *)msg.str().c_str() );
   }

   // Verify the TrickThreadCoordinator::initialize_thread_state() function was called as
   // required before this function is called by checking if the thread count
   // was initialized.
   if ( this->thread_state_cnt == 0 ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: Federate::initialize_thread_state()"
             << " must be called once before calling this function."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify the total Trick thread count (main + child).
   if ( this->thread_state_cnt != exec_get_num_threads() ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: The total number of Trick threads "
             << exec_get_num_threads() << " (main + child threads) does"
             << " not match the number (" << this->thread_state_cnt
             << ") we initialized to in TrickThreadCoordinator::initialize_thread_state()"
             << " for the specified thread-id:" << thread_id << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify the child thread-id specified is in range.
   if ( thread_id >= this->thread_state_cnt ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: Total Trick thread count " << this->thread_state_cnt
             << " (main + child threads), Invalid specified child thread-id:"
             << thread_id << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // We do not support more than one thread association to the same thread-id.
   if ( this->thread_state[thread_id] != THREAD_STATE_NOT_ASSOCIATED ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: You can not associate the same Trick child thread (thread-id:"
             << thread_id << ") more than once with TrickHLA!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // The child thread data cycle time cannot be less than (i.e. faster)
   // than the main thread cycle time.
   if ( data_cycle_micros < this->main_thread_data_cycle_micros ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: The data cycle time for the Trick child thread (thread-id:"
             << thread_id << ", data_cycle:" << data_cycle
             << ") cannot be less than the Trick main thread data cycle"
             << " time (data_cycle:"
             << Int64Interval::to_seconds( this->main_thread_data_cycle_micros )
             << ")!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Only allow child threads with a data cycle time that is an integer
   // multiple of the Trick main thread cycle time.
   if ( ( data_cycle_micros % main_thread_data_cycle_micros ) != 0LL ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: The data cycle time for the Trick child thread (thread-id:"
             << thread_id << ", data_cycle:" << data_cycle
             << ") must be an integer multiple of the Trick main thread data"
             << " cycle time (data_cycle:"
             << Int64Interval::to_seconds( this->main_thread_data_cycle_micros )
             << ")!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Parse the HLA object instance names (as a comma separated list) that are
   // associated with this Trick child thread index.
   bool             any_valid_instance_name = false;
   vector< string > obj_instance_names_vec;
   StringUtilities::tokenize( obj_insance_names, obj_instance_names_vec, "," );
   for ( unsigned int i = 0; i < obj_instance_names_vec.size(); ++i ) {

      string obj_instance_name = obj_instance_names_vec.at( i );

      // Search all the objects for an instance name match.
      bool found_object = false;
      for ( unsigned int obj_index = 0; obj_index < this->manager->obj_count; ++obj_index ) {

         if ( obj_instance_name == this->manager->objects[obj_index].get_name_string() ) {

            if ( ( this->data_cycle_micros_per_thread[thread_id] > 0LL )
                 && ( this->data_cycle_micros_per_thread[thread_id] != data_cycle_micros ) ) {
               ostringstream errmsg;
               errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
                      << " ERROR: For the object instance name specified:'"
                      << obj_instance_name << "', the Trick child thread (thread-id:"
                      << thread_id << ", data_cycle:"
                      << Int64Interval::to_seconds( this->data_cycle_micros_per_thread[thread_id] )
                      << ") does not match the data cycle time specified:"
                      << data_cycle << ". A Trick child thread must use the"
                      << " same data cycle time across all associated objects"
                      << " so that TrickHLA can properly ensure data coherency."
                      << THLA_ENDL;
               DebugHandler::terminate_with_message( errmsg.str() );

            } else if ( ( this->data_cycle_micros_per_obj[obj_index] > 0LL )
                        && ( this->data_cycle_micros_per_obj[obj_index] != data_cycle_micros ) ) {
               ostringstream errmsg;
               errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
                      << " ERROR: For the object instance name specified:'"
                      << obj_instance_name << "' an existing entry for this"
                      << " object (thread-id:" << thread_id
                      << ", data_cycle:"
                      << Int64Interval::to_seconds( this->data_cycle_micros_per_thread[thread_id] )
                      << ") has a data cycle time that does not match the"
                      << " data cycle time specified:" << data_cycle
                      << ". An object instance must use the same data cycle"
                      << " time across all threads so that TrickHLA can properly"
                      << " ensure data coherency." << THLA_ENDL;
               DebugHandler::terminate_with_message( errmsg.str() );

            } else {
               found_object            = true;
               any_valid_instance_name = true;

               this->data_cycle_micros_per_thread[thread_id] = data_cycle_micros;
               this->data_cycle_micros_per_obj[obj_index]    = data_cycle_micros;
            }
         }
      }

      if ( !found_object ) {
         ostringstream errmsg;
         errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
                << " ERROR: For the Trick child thread (thread-id:" << thread_id
                << ") specified, the object instance name provided \""
                << obj_instance_name
                << "\" does not have a corresponding TrickHLA-Object configured."
                << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // If the data cycle time for this child thread does not match the
   // main thread data cycle time then the user must specify all the valid
   // HLA object instance names associated to this child thread.
   if ( ( data_cycle_micros != this->main_thread_data_cycle_micros ) && !any_valid_instance_name ) {
      ostringstream errmsg;
      errmsg << "TrickThreadCoordinator::associate_to_trick_child_thread():" << __LINE__
             << " ERROR: For the Trick child thread (thread-id:" << thread_id
             << ") specified, you have specified a data cycle time ("
             << data_cycle << ") that differs from the Trick main thread data"
             << " cycle time ("
             << Int64Interval::to_seconds( this->main_thread_data_cycle_micros )
             << "). This requires you to specify all the HLA object instance"
             << " names associated with this Trick child thread so that TrickHLA"
             << " can properly ensure data coherency." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make sure we mark the thread state as reset now that we associated to it.
   this->thread_state[thread_id] = THREAD_STATE_RESET;

   // We now have at least one Trick child thread associated to TrickHLA.
   this->any_thread_associated = true;
}

/*!
 * @brief Announce all the HLA data was sent.
 */
void TrickThreadCoordinator::announce_data_available()
{
   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      send_hs( stdout, "TrickThreadCoordinator::announce_data_available():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Process Trick child thread states associated to TrickHLA.
   if ( this->any_thread_associated ) {

      int64_t const sim_time_micros = Int64Interval::to_microseconds( exec_get_sim_time() );

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      // Process all the Trick child threads associated to TrickHLA first
      // and only for threads on the data cycle time boundary.
      for ( unsigned int id = 1; id < this->thread_state_cnt; ++id ) {
         if ( ( this->thread_state[id] != THREAD_STATE_NOT_ASSOCIATED )
              && on_data_cycle_boundary_for_thread( id, sim_time_micros ) ) {

            this->thread_state[id] = THREAD_STATE_READY_TO_RECEIVE;
         }
      }

      // Make sure we set the state of the Trick main thread last.
      this->thread_state[0] = THREAD_STATE_READY_TO_RECEIVE;
   }
}

/*!
 * @brief Announce all the HLA data was sent.
 */
void TrickThreadCoordinator::announce_data_sent()
{
   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      send_hs( stdout, "TrickThreadCoordinator::announce_data_sent():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Process Trick child thread states associated to TrickHLA.
   if ( this->any_thread_associated ) {

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      // Set the state of the main thread as ready to send.
      this->thread_state[0] = THREAD_STATE_READY_TO_SEND;
   }
}

/*!
 * @brief Wait for the HLA data to be sent if a Trick child thread or if the
 * calling thread is the Trick main thread then wait for all associated Trick
 * child threads to have called this function.
 */
void TrickThreadCoordinator::wait_to_send_data()
{
   // Don't process Trick child thread states associated to TrickHLA if none exist.
   if ( !this->any_thread_associated ) {
      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         send_hs( stdout, "TrickThreadCoordinator::wait_to_send_data():%d%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   // Get the ID of the thread that called this function.
   unsigned int const thread_id = exec_get_process_id();

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      send_hs( stdout, "TrickThreadCoordinator::wait_to_send_data():%d %s thread:%d, waiting...%c",
               __LINE__, ( ( thread_id == 0 ) ? "Main" : "Child" ),
               thread_id, THLA_NEWLINE );
   }

   // Determine if this is the main thread (id = 0) or a child thread. The main
   // thread will wait for all the child threads to be ready to send before
   // returning.
   if ( thread_id == 0 ) {

      int64_t const sim_time_micros = Int64Interval::to_microseconds( exec_get_sim_time() );

      // Don't check the Trick main thread (id = 0), only check child threads.
      unsigned int id = 1;

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
         while ( ( id < this->thread_state_cnt ) && all_ready_to_send ) {

            // If the state is THREAD_STATE_NOT_ASSOCIATED then there are
            // no TrickHLA jobs on this thread, so move on to the next
            // thread-id. If the state is THREAD_STATE_READY_TO_SEND then
            // this thread-id is ready, so check the next ID. Otherwise we
            // are not ready to send and don't move on from the current
            // thread-id. Skip this child thread if it is not scheduled to
            // run at the same time as the main thread for this job.
            // Also skip if this thread is not on a data cycle boundary.
            if ( ( this->thread_state[id] == THREAD_STATE_READY_TO_SEND )
                 || ( this->thread_state[id] == THREAD_STATE_NOT_ASSOCIATED )
                 || !on_data_cycle_boundary_for_thread( id, sim_time_micros ) ) {
               // Move to the next thread-id because the current ID is
               // ready. This results in checking all the ID's just once.
               ++id;
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
         SleepTimeout print_timer( this->federate->wait_status_time );
         SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

         // Wait for all Trick child threads associated to TrickHLA to be
         // ready to send data.
         do {

            // Check for shutdown.
            this->federate->check_for_shutdown_with_termination();

            (void)sleep_timer.sleep();

            // Determine if all the Trick child threads are ready to send data.
            all_ready_to_send = true;
            {
               // When auto_unlock_mutex goes out of scope it automatically
               // unlocks the mutex even if there is an exception.
               MutexProtection auto_unlock_mutex( &mutex );

               // Check all the associated thread-id's.
               while ( ( id < this->thread_state_cnt ) && all_ready_to_send ) {

                  // If the state is THREAD_STATE_NOT_ASSOCIATED then there are
                  // no TrickHLA jobs on this thread, so move on to the next
                  // thread-id. If the state is THREAD_STATE_READY_TO_SEND then
                  // this thread-id is ready, so check the next ID. Otherwise we
                  // are not ready to send and don't move on from the current
                  // thread-id. Skip this child thread if it is not scheduled to
                  // run at the same time as the main thread for this job.
                  // Also skip if this thread is not on a data cycle boundary.
                  if ( ( this->thread_state[id] == THREAD_STATE_READY_TO_SEND )
                       || ( this->thread_state[id] == THREAD_STATE_NOT_ASSOCIATED )
                       || !on_data_cycle_boundary_for_thread( id, sim_time_micros ) ) {
                     // Move to the next thread-id because the current ID is
                     // ready. This results in checking all the ID's just once.
                     ++id;
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
                  if ( !this->federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "TrickThreadCoordinator::wait_to_send_data():" << __LINE__
                            << " Unexpectedly the Federate is no longer an execution"
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
                  send_hs( stdout, "TrickThreadCoordinator::wait_to_send_data():%d Trick main thread, waiting on child thread %d...%c",
                           __LINE__, id, THLA_NEWLINE );
               }
            }
         } while ( !all_ready_to_send );
      }
   } else {

      // Trick Child Threads associated to TrickHLA need to wait for the Trick
      // main thread to send all the HLA data.

      // Do a quick look to determine if the Trick main thread has sent all
      // the HLA data.
      bool sent_data;
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks
         // the mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &mutex );

         // Mark this child thread as ready to send.
         this->thread_state[thread_id] = THREAD_STATE_READY_TO_SEND;

         // Determine if all the data has been sent by the main thread.
         sent_data = ( this->thread_state[0] == THREAD_STATE_READY_TO_SEND );
      }

      // If the quick look to see if the main thread has announced it has sent
      // the data has not succeeded then do a more involved spin-lock with a
      // sleep. This will have more wait latency.
      if ( !sent_data ) {

         int64_t      wallclock_time;
         SleepTimeout print_timer( this->federate->wait_status_time );
         SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

         // Wait for the main thread to have sent the data.
         do {
            // Check for shutdown.
            this->federate->check_for_shutdown_with_termination();

            (void)sleep_timer.sleep();

            {
               // When auto_unlock_mutex goes out of scope it automatically
               // unlocks the mutex even if there is an exception.
               MutexProtection auto_unlock_mutex( &mutex );

               sent_data = ( this->thread_state[0] == THREAD_STATE_READY_TO_SEND );
            }

            if ( !sent_data ) {

               // To be more efficient, we get the time once and share it.
               wallclock_time = sleep_timer.time();

               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();
                  if ( !this->federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "TrickThreadCoordinator::wait_to_send_data():" << __LINE__
                            << " Unexpectedly the Federate is no longer an execution"
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
                  send_hs( stdout, "TrickThreadCoordinator::wait_to_send_data():%d Child thread: %d, waiting...%c",
                           __LINE__, thread_id, THLA_NEWLINE );
               }
            }
         } while ( !sent_data );
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      send_hs( stdout, "TrickThreadCoordinator::wait_to_send_data():%d %s Thread:%d, Done%c",
               __LINE__, ( ( thread_id == 0 ) ? "Main" : "Child" ),
               thread_id, THLA_NEWLINE );
   }
}

/*! @brief Wait to receive data when the Trick main thread is ready. */
void TrickThreadCoordinator::wait_to_receive_data()
{
   // Don't process Trick child thread states associated to TrickHLA if none exist.
   if ( !this->any_thread_associated ) {
      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         send_hs( stdout, "TrickThreadCoordinator::wait_to_receive_data():%d%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   // Get the ID of the thread that called this function.
   unsigned int const thread_id = exec_get_process_id();

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      send_hs( stdout, "TrickThreadCoordinator::wait_to_receive_data():%d %s Thread:%d, waiting...%c",
               __LINE__, ( ( thread_id == 0 ) ? "Main" : "Child" ),
               thread_id, THLA_NEWLINE );
   }

   bool ready_to_receive;
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &mutex );

      ready_to_receive = ( this->thread_state[0] == THREAD_STATE_READY_TO_RECEIVE );
   }

   // See if the main thread has announced it has received data.
   if ( !ready_to_receive ) {

      int64_t      wallclock_time;
      SleepTimeout print_timer( this->federate->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the main thread to receive data.
      do {
         // Check for shutdown.
         this->federate->check_for_shutdown_with_termination();

         (void)sleep_timer.sleep();

         {
            // When auto_unlock_mutex goes out of scope it automatically
            // unlocks the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &mutex );

            ready_to_receive = ( this->thread_state[0] == THREAD_STATE_READY_TO_RECEIVE );
         }

         if ( !ready_to_receive ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !this->federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "TrickThreadCoordinator::wait_to_receive_data():" << __LINE__
                         << " Unexpectedly the Federate is no longer an execution"
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
               send_hs( stdout, "TrickThreadCoordinator::wait_to_receive_data():%d %s Thread:%d, waiting...%c",
                        __LINE__, ( ( thread_id == 0 ) ? "Main" : "Child" ),
                        thread_id, THLA_NEWLINE );
            }
         }
      } while ( !ready_to_receive );
   }
   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      send_hs( stdout, "TrickThreadCoordinator::wait_to_receive_data():%d %s Thread:%d, Done%c",
               __LINE__, ( ( thread_id == 0 ) ? "Main" : "Child" ),
               thread_id, THLA_NEWLINE );
   }
}

/*! @brief On boundary if sim-time is an integer multiple of a valid cycle-time. */
bool const TrickThreadCoordinator::on_data_cycle_boundary_for_thread(
   unsigned int const thread_id,
   int64_t const      sim_time_micros ) const
{
   // On boundary if sim-time is an integer multiple of a valid cycle-time.
   return ( ( this->any_thread_associated
              && ( thread_id < this->thread_state_cnt )
              && ( this->data_cycle_micros_per_thread[thread_id] > 0LL ) )
               ? ( ( sim_time_micros % this->data_cycle_micros_per_thread[thread_id] ) == 0LL )
               : true );
}

/*! @brief On boundary if sim-time is an integer multiple of a valid cycle-time. */
bool const TrickThreadCoordinator::on_data_cycle_boundary_for_obj(
   unsigned int const obj_index,
   int64_t const      sim_time_micros ) const
{
   // On boundary if sim-time is an integer multiple of a valid cycle-time.
   return ( ( this->any_thread_associated
              && ( obj_index < this->manager->obj_count )
              && ( this->data_cycle_micros_per_obj[obj_index] > 0LL ) )
               ? ( ( sim_time_micros % this->data_cycle_micros_per_obj[obj_index] ) == 0LL )
               : true );
}

/*! @brief Get the data cycle time for the specified object index, otherwise
 * return the default data cycle time. */
int64_t const TrickThreadCoordinator::get_data_cycle_time_micros_for_obj(
   unsigned int const obj_index,
   int64_t const      default_data_cycle_micros ) const
{
   return ( this->any_thread_associated
            && ( obj_index < this->manager->obj_count )
            && ( this->data_cycle_micros_per_obj[obj_index] > default_data_cycle_micros ) )
             ? this->data_cycle_micros_per_obj[obj_index]
             : default_data_cycle_micros;
}
