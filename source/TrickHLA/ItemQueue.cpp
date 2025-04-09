/*!
@file TrickHLA/ItemQueue.cpp
@ingroup TrickHLA
@brief This class represents a queue for holding Items.

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
@trick_link_dependency{Item.cpp}
@trick_link_dependency{ItemQueue.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Feb 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdio> // needed for std::FILE used in trick/message_proto.h
#include <sstream>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Item.hh"
#include "TrickHLA/ItemQueue.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ItemQueue::ItemQueue()
   : mutex(),
     count( 0 ),
     head( NULL ),
     tail( NULL ),
     original_head( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
ItemQueue::~ItemQueue()
{
   // Empty the queue by popping items off of it.
   while ( !empty() ) {
      pop();
   }

   // Make sure we destroy the mutex.
   mutex.destroy();
}

/*!
 * @brief Prints the 'head' pointers for all elements in the queue.
 * @param name Name of the caller.
 * */
void ItemQueue::dump_head_pointers(
   char const *name )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   ostringstream msg;
   msg << "ItemQueue::dump_head_pointers(" << name << "):" << __LINE__ << " ";
   for ( Item *item = head; item != NULL; item = item->next ) {
      msg << item << "->";
   }
   msg << "NULL\n";

   message_publish( MSG_NORMAL, msg.str().c_str() );
}

/*!
 * @brief Sets head to the passed-in element's next value.
 * @param item Item to extract the 'next' data pointer.
 */
void ItemQueue::next(
   Item *item )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   // Adjust to the next item off the stack in a thread-safe way.
   if ( item->next != NULL ) { // is this not the end of the queue

      // If this is the first call to the routine, capture the head pointer so
      // it can be restored once we are done with walking the queue...
      if ( original_head == NULL ) {
         original_head = head;
      }

      // Adjust the "head" to point to the next item in the linked-list.
      head = item->next;
   }
}

/*!
 * @brief Pop an item off the queue.
 */
void ItemQueue::pop()
{
   // Pop the item off the stack in a thread-safe way.

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   if ( !empty() ) {
      Item *item = head;

      // Adjust the "head" to point to the next item in the linked-list.
      if ( head == tail ) {
         head = NULL;
         tail = NULL;
      } else {
         head = item->next;
      }

      // Make sure we delete the Item we created when it was pushed on the queue.
      delete item;
      --count;
   }
}

/*!
 * @brief Push the item onto the queue.
 * @param item Item to put into the queue.
 */
void ItemQueue::push( // RETURN: -- None.
   Item *item )       // IN: -- Item to put into the queue.
{
   // Add the item to the queue in a thread-safe way.

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   // Add the item to the tail-end of the linked list.
   if ( tail == NULL ) {
      head = item;
      tail = item;
   } else {
      tail->next = item;
      tail       = item;
   }
   ++count;
}

/*!
 * @brief Re-established original 'head' queue pointer after the queue has
 *  been walked.
 */
void ItemQueue::rewind()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   // If the next() routine was ever executed to walk thru the queue without
   // popping, restore the queue's original 'head' pointer.
   if ( original_head != NULL ) {
      head          = original_head;
      original_head = NULL;
   }
}
