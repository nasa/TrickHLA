/*!
@file TrickHLA/time/TimeManagement.hh
@ingroup TrickHLA
@brief This class provides basic services for HLA time management.

@copyright Copyright 2019 United States Government as represented by the
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
@trick_link_dependency{../../../source/TrickHLA/time/TimeManagement.cpp}
@trick_link_dependency{../../../source/TrickHLA/time/Int64Time.cpp}
@trick_link_dependency{../../../source/TrickHLA/time/TrickThreadCoordinator.cpp}
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/FedAmb.cpp}
@trick_link_dependency{../../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../../source/TrickHLA/MutexLock.cpp}
@trick_link_dependency{../../../source/TrickHLA/MutexProtection.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, October 2025, --, Refactor from Federate class.}
@revs_end

*/

#ifndef TRICKHLA_TIME_MANAGEMENT_HH
#define TRICKHLA_TIME_MANAGEMENT_HH

// System includes.
#include <map>
#include <memory>
#include <string>

// Trick includes.
#include "trick/Flag.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Interval.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/time/TrickThreadCoordinator.hh"

#if defined( IEEE_1516_2025 )
#   include "TrickHLA/FedAmbHLA4.hh"
#else
#   include "TrickHLA/FedAmbHLA3.hh"
#endif // IEEE_1516_2025

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Enums.h"
#include "RTI/Handle.h"
#include "RTI/RTI1516.h"
#include "RTI/Typedefs.h"
#include "RTI/VariableLengthData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

class TimeManagement : public TrickThreadCoordinator
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__TimeManagement();

   //----------------------------- USER VARIABLES -----------------------------
   // The variables below are configured by the user in the input files.
   //--------------------------------------------------------------------------
  public:
   // FIXME: Is this really needed?
   // This is only used for checkpointing and restart.
   double lookahead_time; ///< @trick_units{s} The HLA lookahead time in seconds.

   bool time_regulating;  ///< @trick_units{--} HLA Time Regulation flag (default: true).
   bool time_constrained; ///< @trick_units{--} HLA Time Constrained flag (default: true).
   bool time_management;  ///< @trick_units{--} Enable HLA Time Management flag (default: true).

   //--------------------------------------------------------------------------

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA TimeManagement class. */
   explicit TimeManagement( Federate *fed );
   /*! @brief Destructor for the TrickHLA TimeManagement class. */
   virtual ~TimeManagement();

   /*! @brief Get the HLA time advance cycle time.
    *  @return HLA cycle time in seconds. */
   double const get_HLA_cycle_time()
   {
      return this->HLA_cycle_time;
   }

   /*! @brief Get the HLA time advance cycle time.
    *  @return HLA cycle time in base time unit. */
   int64_t const get_HLA_cycle_time_in_base_time()
   {
      return this->HLA_cycle_time_in_base_time;
   }

   //
   // Time management initialization functions.
   //
   /*! @brief Enable time constrained.
    *  @param time the granted HLA Logical time */
   void set_time_constrained_enabled( RTI1516_NAMESPACE::LogicalTime const &time );

   /*! @brief Setup this federate's constrained time management. */
   void setup_time_constrained();

   /*! @brief Enable time regulating.
    *  @param time the granted HLA Logical time */
   void set_time_regulation_enabled( RTI1516_NAMESPACE::LogicalTime const &time );

   /*! @brief Setup this federate's regulate time management. */
   void setup_time_regulation();

   /*! @brief Setup this federate's time management. */
   void setup_time_management();

   //
   // Executive execution loop time functions.
   //
   /*! @brief Increment the requested time by the lookahead time and make a
    *  HLA time advance request. */
   void time_advance_request();

   /*! @brief Moves the federates time to the Greatest Available Logical Time
    * (GALT) that is an integer multiple of the Least-Common-Time-Step (LCTS)
    * time if we are time constrained and Not time regulating. */
   void time_advance_request_to_GALT();

   /*! @brief Move the requested time to an integer multiple of the Greatest
    *  Available Logical Time (GALT) and Least Common Time Step (LCTS). */
   void time_advance_request_to_GALT_LCTS_multiple();

   /*! @brief Wait for a HLA time-advance grant. */
   void wait_for_time_advance_grant();

   /*! @brief Initialize the thread memory associated with the Trick child threads. */
   void initialize_thread_state( double const main_thread_data_cycle_time );

   /*! @brief Verify the time constraints (i.e. Lookahead, LCTS, RT and dt). */
   virtual bool const verify_time_constraints();

   /*! @brief Shutdown this federate's time management. */
   void shutdown_time_management();

   // TODO: Consider renaming these "shutdown" routines to disable.
   /*! @brief Shutdown this federate's time constrained time management. */
   void shutdown_time_constrained();

   /*! @brief Shutdown this federate's time regulating time management. */
   void shutdown_time_regulating();

   //
   // Routines to return federation state values.
   //
   /*! @brief Get the pointer to the associated HLA RTI Ambassador instance.
    *  @return Pointer to associated RTI Ambassador. */
   RTI1516_NAMESPACE::RTIambassador *get_RTI_ambassador() // cppcheck-suppress [functionStatic, unmatchedSuppression]
   {
      return RTI_ambassador.get();
   }

   /*! @brief Get the current granted HLA federation execution time.
    *  @return Reference to current granted HLA federation execution time. */
   Int64Time const &get_granted_time() const
   {
      return this->granted_time;
   }

   /*! @brief Get the current granted HLA federation execution time in the base HLA Logical Time representation.
    *  @return Reference to current granted HLA federation execution time. */
   double const get_granted_base_time() const
   {
      return granted_time.get_base_time();
   }

   /*! @brief Get the requested HLA federation execution time.
    *  @return Reference to requested HLA federation execution time. */
   Int64Time const &get_requested_time() const
   {
      return this->requested_time;
   }

   /*! @brief Get the current federate lookahead time.
    *  @return Reference to current federate lookahead time. */
   Int64Interval const &get_lookahead() const
   {
      return this->lookahead;
   }

   /*! @brief Get the current federate lookahead time in seconds.
    *  @return Current federate lookahead time in the base time. */
   int64_t const get_lookahead_in_base_time() const
   {
      return lookahead.get_base_time();
   }

   /*! @brief Query of federate has a zero lookahead time.
    *  @return True if lookahead time is zero; False otherwise. */
   bool const is_zero_lookahead_time() const
   {
      return ( lookahead.get_base_time() <= 0LL );
   }

   /*! @brief Query if time advance has been granted.
    *  @return True if time advance has been granted; False otherwise. */
   bool is_time_advance_granted()
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      return ( this->time_adv_state == TIME_ADVANCE_GRANTED );
   }

   /*! @brief Sets the granted time from the specified double.
    *  @param time Granted time in seconds. */
   void set_granted_time( double const time );

   /*! @brief Sets the granted time from the specified LogicalTime.
    *  @param time Granted time in HLA logical time. */
   void set_granted_time( RTI1516_NAMESPACE::LogicalTime const &time );

   /*! @brief Set the time advance as granted. */
   void set_time_advance_granted( RTI1516_NAMESPACE::LogicalTime const &time );

   /*! @brief Query if the federate is in a time regulating state.
    *  @return True if time regulating; False otherwise. */
   bool in_time_regulating_state() const
   {
      return this->time_regulating_state;
   }

   /*! @brief Set the state of time regulation.
    *  @param regulation_state Desired state of time regulation for this federate. */
   void set_time_regulation_state( bool const regulation_state )
   {
      this->time_regulating_state = regulation_state;
   }

   /*! @brief Set the state of time constraint.
    *  @param constrained_state Desired state of time constraint for this federate. */
   void set_time_constrained_state( bool const constrained_state )
   {
      this->time_constrained_state = constrained_state;
   }

   /*! @brief Sets the requested time from the specified double.
    *  @param time Requested time in seconds. */
   void set_requested_time( double const time );

   /*! @brief Sets the requested time from the specified LogicalTime.
    *  @param time Requested time in HLA logical time. */
   void set_requested_time( RTI1516_NAMESPACE::LogicalTime const &time );

   /*! @brief Sets the HLA base time unit. */
   static HLABaseTimeEnum get_HLA_base_time_unit();

   /*! @brief Sets the HLA base time unit.
    *  @param base_time_unit HLA base time unit. */
   void set_HLA_base_time_unit( HLABaseTimeEnum const base_time_unit );

   /*! @brief Sets the HLA base time unit and scales the Trick tics multiplier.
    *  @param base_time_unit HLA base time unit. */
   void set_HLA_base_time_unit_and_scale_trick_tics( HLABaseTimeEnum const base_time_unit );

   /*! @brief Sets the HLA base time multiplier.
    *  @param multiplier HLA base time multiplier. */
   void set_HLA_base_time_multiplier( int64_t const multiplier );

   /*! @brief Sets the HLA base time multiplier and scales the Trick tics multiplier.
    *  @param multiplier HLA base time multiplier. */
   void set_HLA_base_time_multiplier_and_scale_trick_tics( int64_t const multiplier );

   /*! @brief Refresh the HLA time constants for the current base time unit. */
   void refresh_HLA_time_constants();

   /*! @brief Scale the Trick Time Tic value given the HLA base time multiplier. */
   static void scale_trick_tics_to_HLA_base_time_multiplier();

   /*! @brief Sets the HLA lookahead time.
    *  @param value HLA lookahead time in seconds. */
   void set_lookahead( double const value );

   /*! @brief Refresh the HLA lookahead base time, which needs to be done if the HLA base time unit change. */
   void refresh_lookahead();

   /*! @brief Query if time management is enabled.
    *  @return True if time management is enabled; False otherwise. */
   bool is_time_management_enabled() const
   {
      // Time management is enabled if the local time-management flag is set.
      return this->time_management;
   }

   // Checkpoint restart initialization.
   /*! @brief Perform initialization after a restart. */
   virtual void restart_initialization();

  protected:
   /*! @brief Make the HLA time-advance request using the current requested_time value. */
   void perform_time_advance_request();

   /*! @brief Make time-advance request available and wait for time advance grant with zero lookahead. */
   void wait_for_zero_lookahead_TARA_TAG();

  protected:
   Int64Interval lookahead; ///< @trick_units{--} Lookahead time for data.

   double  HLA_cycle_time;              ///< @trick_units{--} HLA cycle time in seconds.
   int64_t HLA_cycle_time_in_base_time; ///< @trick_io{**} HLA time advance cycle delta time step in base time unit.

   Int64Time granted_time;   ///< @trick_units{--} HLA time given by RTI
   Int64Time requested_time; ///< @trick_units{--} requested/desired HLA time
   double    HLA_time;       ///< @trick_io{*io} @trick_units{s} Current HLA time to allow for plotting.

   TimeAdvanceStateEnum time_adv_state;       ///< @trick_units{--} HLA Time advance state.
   MutexLock            time_adv_state_mutex; ///< @trick_units{--} HLA Time advance state mutex lock.

   bool time_regulating_state;  ///< @trick_units{--} Internal flag, federates HLA Time Regulation state (default: false).
   bool time_constrained_state; ///< @trick_units{--} Internal flag, federates HLA Time Constrained state (default: false).

   uint64_t tag_wait_sum;   ///< @trick_units{--} Sum of all the TAG wait wallclock times.
   uint64_t tag_wait_count; ///< @trick_units{--} Number of times we waited for TAG.

   // Federation required associations.
   //
#if defined( IEEE_1516_2025 )
#   if !defined( SWIG )
   std::unique_ptr< RTI1516_NAMESPACE::RTIambassador > RTI_ambassador; ///< @trick_units{--} RTI ambassador.
#   endif                                                              // SWIG
#else
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
   std::auto_ptr< RTI1516_NAMESPACE::RTIambassador > RTI_ambassador; ///< @trick_units{--} RTI ambassador.
#   pragma GCC diagnostic pop
#endif // IEEE_1516_2025

  private:
   // Do not allow the copy constructor or assignment operator.
   TimeManagement();
   /*! @brief Copy constructor for TimeManagement class.
    *  @details This constructor is private to prevent inadvertent copies. */
   TimeManagement( TimeManagement const &rhs );

   /*! @brief Assignment operator for TimeManagement class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   TimeManagement &operator=( TimeManagement const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_TIME_MANAGEMENT_HH
