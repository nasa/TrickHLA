/*!
@file TrickHLA/ReflectedAttributesQueue.cpp
@ingroup TrickHLA
@brief This is a protected queue class to hold reflected attributes.

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
@trick_link_dependency{ReflectedAttributesQueue.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, Feb 2019, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.

// TrickHLA include files.
#include "TrickHLA/ReflectedAttributesQueue.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/StandardsSupport.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ReflectedAttributesQueue::ReflectedAttributesQueue()
   : queue_mutex(),
     attribute_map_queue()
{
   return;
}

/*!
 * @details Frees the Trick allocated memory.
 * @job_class{shutdown}
 */
ReflectedAttributesQueue::~ReflectedAttributesQueue()
{
   // Empty the queue without using a queue_mutex lock.
   while ( !attribute_map_queue.empty() ) {
      attribute_map_queue.pop();
   }

   // Make sure we destroy the queue_mutex.
   queue_mutex.destroy();
}

bool ReflectedAttributesQueue::empty()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &queue_mutex );

   return attribute_map_queue.empty();
}

void ReflectedAttributesQueue::push(
   AttributeHandleValueMap const &theAttributes )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &queue_mutex );

   attribute_map_queue.push( theAttributes );
}

void ReflectedAttributesQueue::pop()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &queue_mutex );

   attribute_map_queue.pop();
}

AttributeHandleValueMap const &ReflectedAttributesQueue::front()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &queue_mutex );

   AttributeHandleValueMap const &theAttributes = attribute_map_queue.front();
   return theAttributes;
}

void ReflectedAttributesQueue::clear()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &queue_mutex );

   while ( !attribute_map_queue.empty() ) {
      attribute_map_queue.pop();
   }
}
