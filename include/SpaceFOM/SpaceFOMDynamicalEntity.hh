/*!
@file SpaceFOM/SpaceFOMDynamicalEntity.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM physical entity type.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the DynamicalEntity object.

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

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntityBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/SpaceFOMDynamicalEntity.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef SPACEFOM_DYNAMICAL_ENTITY_HH
#define SPACEFOM_DYNAMICAL_ENTITY_HH

// System include files.

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityData.h"
#include "SpaceFOM/DynamicalEntityBase.hh"
#include "SpaceFOM/SpaceFOMPhysicalEntity.hh"

namespace TrickHLA
{
class Packing;
class OpaqueBuffer;
} // namespace TrickHLA

namespace SpaceFOM
{

class SpaceFOMDynamicalEntity : public SpaceFOM::SpaceFOMPhysicalEntity, public SpaceFOM::DynamicalEntityBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__SpaceFOMDynamicalEntity();

  public:
   // Public constructors and destructors.
   SpaceFOMDynamicalEntity();          // Default constructor.
   virtual ~SpaceFOMDynamicalEntity(); // Destructor.

   // Initialize the packing object.
   /*! @brief Set the reference to the physical entity data.
    *  @param ref_frame_data_ptr Pointer to the RefFrameData instance. */
   void initialize( PhysicalEntityData  * physical_data_ptr,
                    DynamicalEntityData * dynamics_data_ptr );

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();


  protected:
   DynamicalEntityData *dynamical_data; ///< @trick_units{--} Physical entity data.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for SpaceFOMDynamicalEntity class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SpaceFOMDynamicalEntity( SpaceFOMDynamicalEntity const &rhs );
   /*! @brief Assignment operator for SpaceFOMDynamicalEntity class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SpaceFOMDynamicalEntity &operator=( SpaceFOMDynamicalEntity const &rhs );

};

} // namespace SpaceFOM

#endif // SPACEFOM_DYNAMICAL_ENTITY_HH: Do NOT put anything after this line!
