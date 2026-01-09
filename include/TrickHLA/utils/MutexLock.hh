/*!
@file TrickHLA/MutexLock.hh
@ingroup TrickHLA
@brief TrickHLA Mutex Lock wrapper.

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
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, July 2020, --, Initial implementation.}
@revs_end
*/

#ifndef TRICKHLA_MUTEX_LOCK_HH
#define TRICKHLA_MUTEX_LOCK_HH

// System includes.
#include <pthread.h>

namespace TrickHLA
{

class MutexLock
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__MutexLock();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA MutexLock class with mutex
    * attribute PTHREAD_MUTEX_RECURSIVE. */
   MutexLock();
   /*! @brief Destructor for the TrickHLA MutexLock class. */
   virtual ~MutexLock();

   /*! @brief Initialize the mutex.
    *  @return Integer value of 0 for success, otherwise non-zero for an error. */
   int const initialize();

   /*! @brief Lock the mutex.
    *  @return Integer value of 0 for success, otherwise non-zero for an error. */
   int const lock();

   /*! @brief Unlock the mutex.
    *  @return Integer value of 0 for success, otherwise non-zero for an error. */
   int const unlock();

   /*! @brief Destroy the mutex lock.
    *  @return Integer value of 0 for success, otherwise non-zero for an error. */
   int const destroy();

   pthread_mutex_t mutex; ///< @trick_io{**} Mutex to lock thread over critical code sections.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for MutexLock class.
    *  @details This constructor is private to prevent inadvertent copies. */
   MutexLock( MutexLock const &rhs );
   /*! @brief Assignment operator for MutexLock class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   MutexLock &operator=( MutexLock const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_MUTEX_LOCK_HH: Do NOT put anything after this line!
