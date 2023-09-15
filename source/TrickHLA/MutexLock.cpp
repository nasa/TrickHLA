/*!
@file TrickHLA/MutexLock.cpp
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

@tldh
@trick_link_dependency{MutexLock.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, July 2020, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <pthread.h>

// TrickHLA include files.
#include "TrickHLA/MutexLock.hh"

using namespace TrickHLA;

/*!
 * @details Default constructor for the TrickHLA MutexLock class with mutex
 * attribute PTHREAD_MUTEX_RECURSIVE.
 * @job_class{initialization}
 */
MutexLock::MutexLock()
{
   pthread_mutexattr_t attr;
   pthread_mutexattr_init( &attr );
   pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
   pthread_mutex_init( &mutex, &attr );
}

/*!
 * @details Frees the allocated mutex.
 * @job_class{shutdown}
 */
MutexLock::~MutexLock()
{
   unlock();
   pthread_mutex_destroy( &mutex );
}

/*!
 * @brief Lock the mutex.
 * @return Integer value of 0 for success, otherwise non-zero for an error.
 */
int const MutexLock::lock()
{
   return pthread_mutex_lock( &mutex );
}

/*!
 * @brief Unlock the mutex.
 * @return Integer value of 0 for success, otherwise non-zero for an error.
 */
int const MutexLock::unlock()
{
   return pthread_mutex_unlock( &mutex );
}
