/*!
@file JEOD/JEODPhysicalEntity.hh
@ingroup JEOD
@brief This class provides data packing for the SpaceFOM PhysicalEntity and
the interface with a JEOD DynBody instance.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the PhysicalEntity object.

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
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityBase.cpp}
@trick_link_dependency{../../source/JEOD/JEODPhysicalEntity.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef SPACEFOM_JEOD_PHYSICAL_ENTITY_HH
#define SPACEFOM_JEOD_PHYSICAL_ENTITY_HH

// System include files.

// Trick include files.

// JEOD include files.
#include "dynamics/dyn_body/include/dyn_body.hh"

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityBase.hh"

namespace TrickHLA
{
class Packing;
class OpaqueBuffer;
} // namespace TrickHLA

namespace SpaceFOM
{

class JEODPhysicalEntity : virtual public SpaceFOM::PhysicalEntityBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__JEODPhysicalEntity();

  public:
   // Public constructors and destructors.
   JEODPhysicalEntity();                                       // Default constructor.
   explicit JEODPhysicalEntity( jeod::DynBody &dyn_body_ref ); // Initialization constructor.
   virtual ~JEODPhysicalEntity();                              // Destructor.

   /*! @brief Set the reference to the physical entity data.
    *  @param dyn_body_data_ptr Pointer to the JEOD DynBody instance. */
   virtual void configure( jeod::DynBody *dyn_body_ptr );

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

   /*! @brief Set the reference to the physical entity data.
    *  @param dyn_body_data_ptr Pointer to the JEOD Dynbody instance. */
   virtual void set_data( jeod::DynBody *dyn_body_data_ptr )
   {
      dyn_body_data = dyn_body_data_ptr;
      return;
   }

   /*! @brief Get the reference to the physical entity data.
    *  @return Pointer to the PhysicalEntity data. */
   virtual jeod::DynBody *get_data()
   {
      return ( dyn_body_data );
   }

  protected:
   jeod::DynBody *dyn_body_data; ///< @trick_units{--} JEOD DynBody data.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for JEODPhysicalEntity class.
    *  @details This constructor is private to prevent inadvertent copies. */
   JEODPhysicalEntity( JEODPhysicalEntity const &rhs );
   /*! @brief Assignment operator for JEODPhysicalEntity class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   JEODPhysicalEntity &operator=( JEODPhysicalEntity const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_JEOD_PHYSICAL_ENTITY_HH: Do NOT put anything after this line!
