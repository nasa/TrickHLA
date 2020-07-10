/*!
@file TrickHLA/ReflectedAttributesQueue.hh
@ingroup TrickHLA
@brief TrickHLA Queue of AttributeHandleValueMap.

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
@trick_link_dependency{../source/TrickHLA/ReflectedAttributesQueue.cpp}
@trick_link_dependency{../source/TrickHLA/MutexLock.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER6, TrickHLA, Feb 2019, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_REFLECTED_ATTRIBUTES_QUEUE_HH_
#define _TRICKHLA_REFLECTED_ATTRIBUTES_QUEUE_HH_

// TrickHLA include files
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"

// HLA include files.
#include RTI1516_HEADER

namespace TrickHLA
{

class ReflectedAttributesQueue
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ReflectedAttributesQueue();

  public:
   MutexLock queue_mutex; ///< @trick_io{**} Mutex to lock thread over critical code sections.

   // TODO: Update this class so that we can create a Trick checkpoint by adding
   // serialize and deserialize capability for the attribute_map_queue data type.
   // Trick can not checkpoint an STL Queue container containing a Map of
   // AttributeHandles and VariableLength values. DDexter 2/28/2019

   HLAAttributeMapQueue attribute_map_queue; ///< @trick_io{**} Queue of AttributeHandleValueMap from attribute reflections.

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA ReflectedAttributesQueue class. */
   ReflectedAttributesQueue();
   /*! @brief Destructor for the TrickHLA ReflectedAttributesQueue class. */
   virtual ~ReflectedAttributesQueue();

  public:
   /*! @brief Determine if the queue is empty.
    *  @return True if queue is empty, False otherwise. */
   bool empty();

   /*! @brief Push the attributes onto the queue.
    *  @param theAttributes The reflected attributes. */
   void push( const RTI1516_NAMESPACE::AttributeHandleValueMap &theAttributes );

   /*! @brief Pop the front value off the queue and the destructor for the
    * value will be called. */
   void pop();

   /*! @brief Get the first/oldest item in the queue.
    *  @return The first/oldest item in the queue. */
   const RTI1516_NAMESPACE::AttributeHandleValueMap &front();

   /*! @brief Clear the queue of all values. */
   void clear();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for ReflectedAttributesQueue class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ReflectedAttributesQueue( const ReflectedAttributesQueue &rhs );
   /*! @brief Assignment operator for ReflectedAttributesQueue class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ReflectedAttributesQueue &operator=( const ReflectedAttributesQueue &rhs );
};

} // namespace TrickHLA

#endif // _TRICKHLA_REFLECTED_ATTRIBUTES_QUEUE_HH_: Do NOT put anything after this line!
