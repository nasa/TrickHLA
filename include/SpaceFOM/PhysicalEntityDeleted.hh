/*!
@file SpaceFOM/PhysicalEntityDeleted.hh
@ingroup SpaceFOM
@brief Callback class the user writes to do something once the object has been
deleted from the RTI.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/ObjectDeletedHandler.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityDeleted.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_DELETED_HH
#define SPACEFOM_PHYSICAL_ENTITY_DELETED_HH

// TrickHLA includes.
#include "TrickHLA/ObjectDeletedHandler.hh"

namespace SpaceFOM
{

class PhysicalEntityDeleted : public TrickHLA::ObjectDeletedHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalEntityDeleted();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the SpaceFOM PhysicalEntityDeleted class. */
   PhysicalEntityDeleted();
   /*! @brief Destructor for the SpaceFOM PhysicalEntityDeleted class. */
   virtual ~PhysicalEntityDeleted();

   //-----------------------------------------------------------------
   // These functions must be defined to make this a concrete class.
   //-----------------------------------------------------------------

   /*! @brief Callback routine implementation to report that this object has
    *  been deleted from the RTI. */
   virtual void deleted();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for PhysicalEntityDeleted class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalEntityDeleted( PhysicalEntityDeleted const &rhs );
   /*! @brief Assignment operator for PhysicalEntityDeleted class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalEntityDeleted &operator=( PhysicalEntityDeleted const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_ENTITY_DELETED_HH: Do NOT put anything after this line!
