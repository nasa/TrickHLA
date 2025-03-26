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

// System include files.
#include <cstdio>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"

using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
MutexProtection::MutexProtection(
   TrickHLA::MutexLock *mutex_lock )
   : mutex( mutex_lock )
{
   if ( mutex->lock() != 0 ) {
      message_publish( MSG_ERROR, "MutexProtection::MutexProtection():%d ERROR Locking the MutexLock!\n",
                       __LINE__ );
   }
}

/*!
 * @details Unlocks the mutex.
 * @job_class{shutdown}
 */
MutexProtection::~MutexProtection()
{
   // Only unlock once because the code creating instances of MutexProtection
   // could be recursive and we must only unlock once per constructor call.
   mutex->unlock();

   // Make sure to NULL the pointer so this class can be reclaimed.
   this->mutex = NULL;
}
