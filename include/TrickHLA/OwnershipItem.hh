/*!
@file TrickHLA/OwnershipItem.hh
@ingroup TrickHLA
@brief This class represents a queue item for holding ownership transfers of
an attribute.

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

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3, TS21, July 2009, --, Checkpoint / restart of TrickHLA.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_OWNERSHIP_ITEM_HH
#define TRICKHLA_OWNERSHIP_ITEM_HH

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Item.hh"
#include "TrickHLA/StandardsSupport.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

namespace TrickHLA
{

class OwnershipItem : public Item
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__OwnershipItem();

  public:
   double time;     ///< @trick_units{--} Federation time when this attribute's ownership is to be transferred.
   char  *FOM_name; ///< @trick_units{--} FOM name for the attribute.

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA OwnershipItem class. */
   OwnershipItem() : time( 0.0 ), FOM_name( NULL )
   {
      return;
   }
   /*! @brief Destructor for the TrickHLA OwnershipItem class. */
   virtual ~OwnershipItem()
   {
      clear();
   }

   /*! @brief Clear the Trick allocated memory for the FOM name. */
   void clear()
   {
      if ( this->FOM_name != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( this->FOM_name ) ) ) {
            message_publish( MSG_WARNING, "OwnershipItem::clear():%d WARNING failed to delete Trick Memory for 'this->FOM_name'\n",
                             __LINE__ );
         }
         this->FOM_name = NULL;
      }
   }

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for OwnershipItem class.
    *  @details This constructor is private to prevent inadvertent copies. */
   OwnershipItem( OwnershipItem const &rhs );
   /*! @brief Assignment operator for OwnershipItem class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   OwnershipItem &operator=( OwnershipItem const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_OWNERSHIP_ITEM_HH
