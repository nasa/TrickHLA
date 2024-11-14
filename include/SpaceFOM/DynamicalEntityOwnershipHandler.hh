/*!
@file SpaceFOM/DynamicalEntityOwnershipHandler.hh
@ingroup SpaceFOM
@brief Ownership transfer for the HLA object attributes of a SpaceFOM
DynamicalEntity.

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
@trick_link_dependency{../../source/TrickHLA/OwnershipHandler.cpp}
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntityOwnershipHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_DYNAMICAL_ENTITY_OWNERSHIP_HANDLER_HH
#define SPACEFOM_DYNAMICAL_ENTITY_OWNERSHIP_HANDLER_HH

// TrickHLA include files.
#include "TrickHLA/Object.hh"
#include "TrickHLA/OwnershipHandler.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityOwnershipHandler.hh"

namespace SpaceFOM
{

class DynamicalEntityOwnershipHandler : public PhysicalEntityOwnershipHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__DynamicalEntityOwnershipHandler();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the SpaceFOM DynamicalEntityOwnershipHandler class. */
   DynamicalEntityOwnershipHandler();
   /*! @brief Destructor for the SpaceFOM DynamicalEntityOwnershipHandler class. */
   virtual ~DynamicalEntityOwnershipHandler();

   /*! @brief Initialization callback as part of the TrickHLA::OwnershipHandler functions.
    *  @param obj Object associated with this OwnershipHandler class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for DynamicalEntityOwnershipHandler class.
    *  @details This constructor is private to prevent inadvertent copies. */
   DynamicalEntityOwnershipHandler( DynamicalEntityOwnershipHandler const &rhs );
   /*! @brief Assignment operator for DynamicalEntityOwnershipHandler class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   DynamicalEntityOwnershipHandler &operator=( DynamicalEntityOwnershipHandler const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_DYNAMICAL_ENTITY_OWNERSHIP_HANDLER_HH: Do NOT put anything after this line!
