/*!
@file TrickHLA/MutexProtection.cpp
@ingroup TrickHLA
@brief Mutex protection, automatically unlocks mutex when this object goes out of scope.

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
@trick_link_dependency{MutexProtection.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, July 2020, --, Initial implementation.}
@revs_end

*/

// TrickHLA include files.
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/MutexLock.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
MutexProtection::MutexProtection(
   TrickHLA::MutexLock *mutex_lock )
   : mutex( mutex_lock )
{
   mutex->lock();
}

/*!
 * @details Unlocks the mutex.
 * @job_class{shutdown}
 */
MutexProtection::~MutexProtection()
{
   mutex->unlock();
   mutex = NULL;
}
