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
@trick_link_dependency{../source/TrickHLA/SleepTimeout.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, July 2020, --, Initial implementation.}
@revs_end

*/

#ifndef _TRICKHLA_SLEEP_TIMEOUT_HH_
#define _TRICKHLA_SLEEP_TIMEOUT_HH_

// System include files.
#include <time.h>

#define THLA_DEFAULT_SLEEP_TIMEOUT_IN_SEC 10.0
#define THLA_DEFAULT_SLEEP_WAIT_IN_MICROS 10000
#define THLA_SLEEP_WAIT_IN_MICROS_FOR_PERFORMANCE 25

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

   /*! @brief Constructor for TrickHLA SleepTimeout class, uses a default timeout of 10 seconds.
    *  @param sleep_micros Time to sleep in microseconds. */
   SleepTimeout( long sleep_micros );

   /*! @brief Constructor for the TrickHLA SleepTimeout class.
    *  @param timeout_seconds Timeout time in seconds.
    *  @param sleep_micros Time to sleep in microseconds. */
   SleepTimeout( double timeout_seconds, long sleep_micros );

   /*! @brief Destructor for the TrickHLA SleepTimeout class. */
   virtual ~SleepTimeout();

   /*! @brief Set the timeout and sleep times.
    *  @param timeout_seconds Timeout time in seconds.
    *  @param sleep_micros Time to sleep in microseconds with a minimum value of 0. */
   void set( double timeout_seconds, long sleep_micros );

   /*! @brief Sleep for the configured sleep time.
    *  @return Integer value of 0 for success, otherwise non-zero for an error. */
   int sleep();

   /*! @brief Determine if we cumulatively slept for the configured timeout time.
    *  @return True if timeout exceeded, false otherwise. */
   const bool timeout() const;

   /*! @brief Reset the internal timeout time. */
   void reset();

  protected:
   long long       timeout_time;       ///< @trick_io{**} Timeout elapsed time in microseconds.
   long long       timeout_clock_time; ///< @trick_io{**} Clock timeout time in microseconds
   struct timespec sleep_time;         ///< @trick_io{**} Time-spec for the requested sleep time.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SleepTimeout class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SleepTimeout( const SleepTimeout &rhs );
   /*! @brief Assignment operator for SleepTimeout class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SleepTimeout &operator=( const SleepTimeout &rhs );
};

} // namespace TrickHLA

#endif // _TRICKHLA_SLEEP_TIMEOUT_HH_: Do NOT put anything after this line!
