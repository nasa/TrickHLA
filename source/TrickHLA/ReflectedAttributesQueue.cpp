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

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, Feb 2019, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.

// TrickHLA include files.
#include "TrickHLA/ReflectedAttributesQueue.hh"

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ReflectedAttributesQueue::ReflectedAttributesQueue()
   : attribute_map_queue()
{
   // Initialize the queue_mutex.
   pthread_mutex_init( &queue_mutex, NULL );
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
   pthread_mutex_destroy( &queue_mutex );
}

bool ReflectedAttributesQueue::empty()
{
   pthread_mutex_lock( &queue_mutex );
   bool queue_is_empty = attribute_map_queue.empty();
   pthread_mutex_unlock( &queue_mutex );
   return queue_is_empty;
}

void ReflectedAttributesQueue::push(
   const AttributeHandleValueMap &theAttributes )
{
   pthread_mutex_lock( &queue_mutex );
   attribute_map_queue.push( theAttributes );
   pthread_mutex_unlock( &queue_mutex );
}

void ReflectedAttributesQueue::pop()
{
   pthread_mutex_lock( &queue_mutex );
   attribute_map_queue.pop();
   pthread_mutex_unlock( &queue_mutex );
}

const AttributeHandleValueMap &ReflectedAttributesQueue::front()
{
   pthread_mutex_lock( &queue_mutex );
   AttributeHandleValueMap &theAttributes = attribute_map_queue.front();
   pthread_mutex_unlock( &queue_mutex );
   return theAttributes;
}

void ReflectedAttributesQueue::clear()
{
   pthread_mutex_lock( &queue_mutex );
   while ( !attribute_map_queue.empty() ) {
      attribute_map_queue.pop();
   }
   pthread_mutex_unlock( &queue_mutex );
}
