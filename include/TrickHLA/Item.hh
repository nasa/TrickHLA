/*!
@file TrickHLA/Item.hh
@ingroup TrickHLA
@brief This class represents a item to be held in an Item Queue.

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
@trick_link_dependency{../source/TrickHLA/Item.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, Feb 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_ITEM_HH_
#define _TRICKHLA_ITEM_HH_

namespace TrickHLA
{

class Item
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Item();

  public:
   Item *next; ///< @trick_units{--} Next item in linked-list.

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA Item class. */
   Item();
   /*! @brief Destructor for the TrickHLA Item class. */
   virtual ~Item();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Item class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Item( const Item &rhs );
   /*! @brief Assignment operator for Item class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Item &operator=( const Item &rhs );
};

} // namespace TrickHLA

#endif // _TRICKHLA_ITEM_HH_
