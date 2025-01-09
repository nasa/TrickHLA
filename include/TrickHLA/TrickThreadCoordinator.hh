/*!
@file TrickHLA/TrickThreadCoordinator.hh
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

@trick_parse{everything}

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../source/TrickHLA/TrickThreadCoordinator.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2023, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, January 2024, --, Added support for child \
thread data cycle time being longer than the main thread data cycle time.}
@revs_end
*/

#ifndef TRICKHLA_TRICK_THREAD_COORDINATOR_HH
#define TRICKHLA_TRICK_THREAD_COORDINATOR_HH

// System includes
#include <cstdint>
#include <string>

// TrickHLA include files.
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

// Forward Declared Classes: Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Federate;
class Manager;

class TrickThreadCoordinator
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__TrickThreadCoordinator();

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA TrickThreadCoordinator class. */
   TrickThreadCoordinator();
   /*! @brief Destructor for the TrickHLA TrickThreadCoordinator class. */
   virtual ~TrickThreadCoordinator();

   /*! @brief Setup the required class instance associations.
    *  @param federate  Associated TrickHLA::Federate class instance.
    *  @param manager   Associated TrickHLA::Manager class instance. */
   void setup( Federate &federate,
               Manager  &manager );

   /*! @brief Initialize the thread memory associated with the Trick child threads. */
   void initialize( double const main_thread_data_cycle_time );

   /*! @brief Associate a Trick child thread with TrickHLA. */
   void associate_to_trick_child_thread( unsigned int const thread_id,
                                         double const       data_cycle );

   /*! @brief Disable the comma separated list of Trick child thread IDs associated to TrickHLA. */
   void disable_trick_thread_associations( char const *thread_ids );

   /*! @brief Verify the threads IDs associated to objects in the input file. */
   void verify_trick_thread_associations();

   /*! @brief Announce to all the child threads the main thread has data available. */
   void announce_data_available();

   /*! @brief Announce to all the child threads the main thread sent the data. */
   void announce_data_sent();

   /*! @brief Wait to send data until all Trick child threads are ready. */
   void wait_to_send_data();

   /*! @brief Wait to receive data when the Trick main thread is ready. */
   void wait_to_receive_data();

   /*! @brief On receive boundary if sim-time is an integer multiple of a valid cycle-time. */
   bool const on_receive_data_cycle_boundary_for_obj( int const     obj_index,
                                                      int64_t const sim_time_in_base_time ) const;

   /*! @brief Get the data cycle time for the configured object index or return
    * the default data cycle time. */
   int64_t const get_data_cycle_base_time_for_obj( int const     obj_index,
                                                   int64_t const default_data_cycle_base_time ) const;

   /*! @brief Get the main thread data cycle in the base time. */
   int64_t const get_main_thread_data_cycle_base_time() const
   {
      return this->main_thread_data_cycle_base_time;
   }

   /*! @brief Verify the time constraints (i.e. Lookahead, LCTS, RT and dt). */
   virtual bool const verify_time_constraints();

   /*! @brief Verify the time constraints (i.e. Lookahead, LCTS, RT and dt). */
   virtual bool const verify_time_constraints( unsigned int const thread_id,
                                               int64_t const      data_cycle_base_time );

  protected:
   /*! @brief On receive boundary if the main thread simulation-time is an integer
    * multiple of a valid thread cycle-time.
    * Note: This is thread safe because this function is only local to this class
    * and it is called from a locked mutex critical section. */
   bool const on_receive_data_cycle_boundary_for_thread(
      unsigned int const thread_id,
      int64_t const      sim_time_in_base_time ) const
   {
      // On boundary if main thread sim-time is an integer multiple of a valid cycle-time.
      return ( ( this->any_child_thread_associated
                 && ( thread_id < this->thread_cnt )
                 && ( this->data_cycle_base_time_per_thread[thread_id] > 0LL ) )
                  ? ( ( sim_time_in_base_time % this->data_cycle_base_time_per_thread[thread_id] ) == 0LL )
                  : true );
   }

   /*! @brief On send boundary if the main thread simulation-time is an integer
    * multiple of a valid thread cycle-time for the send frame.
    * Note: This is thread safe because this function is only local to this class
    * and it is called from a locked mutex critical section. */
   bool const on_send_data_cycle_boundary_for_thread(
      unsigned int const thread_id,
      int64_t const      sim_time_in_base_time ) const
   {
      // Data from the child thread should be sent on the main thread frame that
      // corresponds to the end of the child thread frame.
      // Child |              |   child thread data cycle: 3
      //  Main |    |    |    |   main thread data cycle:  1
      //  Time 0    1    2    3
      //                   ^-- Check for child thread sending in main thread frame here.
      //                 ^-- (child_cycle - main_cycle) = ( 3 - 1 )
      //
      return ( ( this->any_child_thread_associated
                 && ( thread_id < this->thread_cnt )
                 && ( this->data_cycle_base_time_per_thread[thread_id] > 0LL ) )
                  ? ( ( ( sim_time_in_base_time - ( this->data_cycle_base_time_per_thread[thread_id] - this->main_thread_data_cycle_base_time ) )
                        % this->data_cycle_base_time_per_thread[thread_id] )
                      == 0LL )
                  : true );
   }

   /*! @brief Wait to send data for Trick main thread. */
   void wait_to_send_data_for_main_thread();

   /*! @brief Wait to send data for Trick child thread. */
   void wait_to_send_data_for_child_thread( unsigned int const thread_id );

   /*! @brief True if the specified thread ID is for an enabled Trick child thread association. */
   bool const is_enabled_child_thread_association( unsigned int const thread_id ) const
   {
      return ( ( this->thread_state[thread_id] != TrickHLA::THREAD_STATE_DISABLED )
               && ( this->thread_state[thread_id] != TrickHLA::THREAD_STATE_NOT_ASSOCIATED ) );
   }

  protected:
   Federate *federate; ///< @trick_units{--} Associated TrickHLA::Federate.
   Manager  *manager;  ///< @trick_units{--} Associated TrickHLA::Manager.

   MutexLock mutex; ///< @trick_units{--} TrickHLA thread state mutex.

   bool any_child_thread_associated; ///< @trick_units{--} True if at least one Trick Child thread is associated to TrickHLA.

   char *disable_thread_ids; ///< @trick_units{--} Comma separated list of thread ID's to disable association to TrickHLA.

   unsigned int thread_cnt; ///< @trick_units{--} Number of Trick child threads used for array sizes.

   unsigned int *thread_state; ///< @trick_units{--} TrickHLA state of trick child threads being used.

   long long *data_cycle_base_time_per_thread; ///< @trick_units{--} Data cycle times per thread in the base HLA Logical Time representation.
   long long *data_cycle_base_time_per_obj;    ///< @trick_units{--} Data cycle times per object instance in the base HLA Logical Time representation.

   long long main_thread_data_cycle_base_time; ///< @trick_units{--} Trick main thread data cycle time in the base HLA Logical Time representation.
};

} // namespace TrickHLA

#endif // TRICKHLA_TRICK_THREAD_COORDINATOR_HH: Do NOT put anything after this line!
