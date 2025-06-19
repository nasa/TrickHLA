/*!
@file TrickHLA/ItemQueue.hh
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

@trick_parse{everything}

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../source/TrickHLA/Item.cpp}
@trick_link_dependency{../../source/TrickHLA/ItemQueue.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Feb 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, April 2025, --, Updated thread safety.}
@revs_end

*/

#ifndef TRICKHLA_ITEM_QUEUE_HH
#define TRICKHLA_ITEM_QUEUE_HH

// System includes.
#include <string>

// TrickHLA includes.
#include "Item.hh"
#include "MutexLock.hh"

namespace TrickHLA
{

class ItemQueue
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ItemQueue();

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA ItemQueue class. */
   ItemQueue();
   /*! @brief Destructor for the TrickHLA ItemQueue class. */
   virtual ~ItemQueue();

  public:
   /*! @brief Query if the item queue is empty.
    *  @return True if empty; False otherwise. */
   bool const empty();

   /*! @brief Get the size of the item queue.
    *  @return Number of elements in the queue. */
   unsigned int const size();

   /*! @brief Gets the front item on the item queue.
    *  @return The front or head item on the item queue. */
   Item *front();

   /*! @brief Gets the last item on the item queue.
    *  @return The last item on the item queue. */
   Item *back();

   /*! @brief Pop an item off the queue. */
   void pop();

   /*! @brief Push the item onto the queue.
    *  @param item Item to put into the queue. */
   void push( Item *item );

   /*! @brief Prints the 'head' pointers for all elements in the queue.
    *  @param name Name of the caller. */
   void dump_linked_list( std::string const &name );

   MutexLock mutex; ///< @trick_io{**} Mutex to lock thread over critical code sections.

  private:
   unsigned int count; ///< @trick_units{count} Number of elements in the queue.

   Item *head; ///< @trick_units{--} First item in linked-list queue.
   Item *tail; ///< @trick_units{--} Last item in linked-list queue.

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for ItemQueue class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ItemQueue( ItemQueue const &rhs );
   /*! @brief Assignment operator for ItemQueue class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ItemQueue &operator=( ItemQueue const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_ITEM_QUEUE_HH
