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
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, April 2025, --, Updated thread safety.}
@revs_end

*/

// System includes.
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include <trick/message_proto.h>
#include <trick/message_type.h>

// TrickHLA includes.
#include "TrickHLA/Item.hh"
#include "TrickHLA/ItemQueue.hh"
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
     tail( NULL )
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
 * @brief Query if the item queue is empty.
 * @return True if empty; False otherwise.
 */
bool const ItemQueue::empty()
{
   MutexProtection auto_unlock_mutex( &mutex );
   return ( head == NULL );
}

/*!
 * @brief Get the size of the item queue.
 * @return Number of elements in the queue.
 */
unsigned int const ItemQueue::size()
{
   MutexProtection auto_unlock_mutex( &mutex );
   return count;
}

/*!
 * @brief Gets the front item on the item queue.
 * @return The front or head item on the item queue.
 */
Item *ItemQueue::front()
{
   MutexProtection auto_unlock_mutex( &mutex );
   return head;
}

/*!
 * @brief Gets the last item on the item queue.
 * @return The last item on the item queue.
 */
Item *ItemQueue::back()
{
   MutexProtection auto_unlock_mutex( &mutex );
   return tail;
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

   if ( head != NULL ) {
      Item *item = head;

      // Adjust the "head" to point to the next item in the linked-list.
      if ( head == tail ) {
         head  = NULL;
         tail  = NULL;
         count = 0;
      } else {
         head = item->next;
         --count;
      }

      // Make sure we delete the Item we created when it was pushed on the queue.
      delete item;
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
   } else {
      tail->next = item;
   }
   tail = item;
   ++count;
}

/*!
 * @brief Prints the 'head' pointers for all elements in the queue.
 * @param name Name of the caller.
 * */
void ItemQueue::dump_linked_list(
   string const &name )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &mutex );

   ostringstream msg;
   msg << "ItemQueue::dump_linked_list(" << name << "):" << __LINE__ << " ";
   for ( Item *item = head; item != NULL; item = item->next ) {
      msg << item << "->";
   }
   msg << "NULL" << endl;

   message_publish( MSG_NORMAL, msg.str().c_str() );
}
