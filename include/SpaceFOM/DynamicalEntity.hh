/*!
@file SpaceFOM/DynamicalEntity.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM physical entity type.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the DynamicalEntity object.

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
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntity.cpp}
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntityBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntity.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Refactored design.}
@revs_end

*/

#ifndef SPACEFOM_DYNAMICAL_ENTITY_HH
#define SPACEFOM_DYNAMICAL_ENTITY_HH

// System include files.

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityBase.hh"
#include "SpaceFOM/DynamicalEntityData.hh"
#include "SpaceFOM/PhysicalEntity.hh"

namespace TrickHLA
{
class Packing;
class OpaqueBuffer;
} // namespace TrickHLA

namespace SpaceFOM
{

class DynamicalEntity : public SpaceFOM::PhysicalEntity, public SpaceFOM::DynamicalEntityBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__DynamicalEntity();

  public:
   // Public constructors and destructors.
   DynamicalEntity(); // Default constructor.
   DynamicalEntity( PhysicalEntityData  &physical_data_ref,
                    DynamicalEntityData &dynamics_data_ref ); // Initialization constructor.
   virtual ~DynamicalEntity(); // Destructor.

   /*! @brief Set the reference to the physical entity data.
    *  @param physical_data_ptr Pointer to the PhysicalEntityData instance.
    *  @param dynamics_data_ptr Pointer to the DynamicalEntityData instance. */
   void pre_initialize( PhysicalEntityData  *physical_data_ptr,
                        DynamicalEntityData *dynamics_data_ptr );

   /*! @brief Initialize the packing object. */
   virtual void initialize();

   /*! @brief Packs the packing data object from the working data object(s),
    *  @details Called from the pack() function to pack the data from the working
    *  data objects(s) into the pe_packing_data object.  */
   virtual void pack_from_working_data();

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  pe_packing_data object into the working data object(s). */
   virtual void unpack_into_working_data();

  protected:
   DynamicalEntityData *dynamical_data; ///< @trick_units{--} Dynamical entity data.

  private:
   using PhysicalEntity::initialize; // Tell compiler we want all initialize functions.

   // This object is not copyable
   /*! @brief Copy constructor for DynamicalEntity class.
    *  @details This constructor is private to prevent inadvertent copies. */
   DynamicalEntity( DynamicalEntity const &rhs );
   /*! @brief Assignment operator for DynamicalEntity class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   DynamicalEntity &operator=( DynamicalEntity const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_DYNAMICAL_ENTITY_HH: Do NOT put anything after this line!
