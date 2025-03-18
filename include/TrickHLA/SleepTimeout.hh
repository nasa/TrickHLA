/*!
@file TrickHLA/SleepTimeout.hh
@ingroup TrickHLA
@brief TrickHLA sleep timer for use in spin locks to detect a timeout.

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
@trick_link_dependency{../../source/TrickHLA/SleepTimeout.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, July 2020, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_SLEEP_TIMEOUT_HH
#define TRICKHLA_SLEEP_TIMEOUT_HH

// System include files.
#include <cstdint>
#include <time.h>

#define THLA_DEFAULT_SLEEP_TIMEOUT_IN_SEC ( (double)10.0 )
#define THLA_DEFAULT_SLEEP_WAIT_IN_MICROS ( (long)10000 )
#define THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS ( (long)25 )

namespace TrickHLA
{

class SleepTimeout
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SleepTimeout();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA SleepTimeout class. */
   SleepTimeout();

   /*! @brief Constructor for TrickHLA SleepTimeout class, uses default sleep time of 10,000 microsecond.
    *  @param timeout_seconds Timeout time in seconds. */
   explicit SleepTimeout( double const timeout_seconds );

   /*! @brief Constructor for TrickHLA SleepTimeout class, uses a default timeout of 10 seconds.
    *  @param sleep_micros Time to sleep in microseconds. */
   explicit SleepTimeout( long const sleep_micros );

   /*! @brief Constructor for the TrickHLA SleepTimeout class.
    *  @param timeout_seconds Timeout time in seconds.
    *  @param sleep_micros Time to sleep in microseconds. */
   SleepTimeout( double const timeout_seconds, long const sleep_micros );

   /*! @brief Destructor for the TrickHLA SleepTimeout class. */
   virtual ~SleepTimeout();

   /*! @brief Set the timeout and sleep times.
    *  @param timeout_seconds Timeout time in seconds.
    *  @param sleep_micros Time to sleep in microseconds with a minimum value of 0. */
   void set( double const timeout_seconds, long const sleep_micros );

   /*! @brief Sleep for the configured sleep time.
    *  @return Integer value of 0 for success, otherwise non-zero for an error. */
   int const sleep() const;

   /*! @brief Gets the wall clock time in microseconds.
    *  @return The wall clock time in microseconds. */
   int64_t const time() const;

   /*! @brief Determine if we cumulatively slept for the configured timeout time.
    *  @return True if timeout exceeded, false otherwise. */
   bool const timeout() const;

   /*! @brief Determine if we cumulatively slept for the configured timeout time.
    *  @param time_in_micros Wall clock time in microseconds.
    *  @return True if timeout exceeded, false otherwise. */
   bool const timeout( int64_t const time_in_micros ) const;

   /*! @brief Reset the internal timeout time. */
   void reset();

  protected:
   long long       timeout_time;       ///< @trick_io{**} Timeout elapsed time in microseconds.
   long long       timeout_clock_time; ///< @trick_io{**} Clock timeout time in microseconds.
   struct timespec sleep_time;         ///< @trick_io{**} Time-spec for the requested sleep time.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SleepTimeout class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SleepTimeout( SleepTimeout const &rhs );
   /*! @brief Assignment operator for SleepTimeout class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SleepTimeout &operator=( SleepTimeout const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_SLEEP_TIMEOUT_HH: Do NOT put anything after this line!
