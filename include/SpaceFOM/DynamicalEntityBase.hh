/*!
@file SpaceFOM/DynamicalEntityBase.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM DynamicalEntity type.

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
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntityBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Refactored.}
@revs_end

*/

#ifndef SPACEFOM_DYNAMICAL_ENTITY_BASE_HH
#define SPACEFOM_DYNAMICAL_ENTITY_BASE_HH

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Object.hh"

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityData.hh"
#include "SpaceFOM/PhysicalEntityBase.hh"

namespace SpaceFOM
{

class DynamicalEntityBase : virtual public SpaceFOM::PhysicalEntityBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__DynamicalEntityBase();

   // Make the Conditional class a friend.
   friend class DynamicalEntityConditionalBase;

   // Make the Lag Compensation class a friend.
   friend class DynamicalEntityLagCompBase;

  public:
   // Public constructors and destructors.
   DynamicalEntityBase();          // Default constructor.
   virtual ~DynamicalEntityBase(); // Destructor.

   // Default data.
   /*! @brief Sets up the attributes for a DynamicalEntity using default values.
    *  @param sim_obj_name Name of SimObject containing this DynamicalEntity.
    *  @param entity_obj_name Name of the DynamicalEntity object in the SimObject.
    *  @param entity_name Name of the DynamicalEntity instance.
    *  @param parent_ref_frame_name Name of the parent ReferenceFrame for this DynamicalEntity instance.
    *  @param publishes Does this federate publish this DynamicalEntity.
    *  @param mngr_object TrickHLA::Object associated with this DynamicalEntity.
    *  */
   virtual void base_config( char const       *sim_obj_name,
                             char const       *entity_obj_name,
                             char const       *entity_name,
                             char const       *parent_ref_frame_name,
                             bool              publishes,
                             TrickHLA::Object *mngr_object = NULL );

   /*! @brief Function to begin the configuration/initialization of the
    *  DynamicalEntity.
    *  This function needs to be called prior to TrickHLA initialization if
    *  the DynamicalEntity object is not being configured with an
    *  initialization constructor. */
   void configure(); // cppcheck-suppress [duplInheritedMember]

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

   /*! @brief Packs the packing data object from the working data object(s),
    *  @details Called from the pack() function to pack the data from the working
    *  data objects(s) into the pe_packing_data object.  */
   virtual void pack_from_working_data() = 0;

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  pe_packing_data object into the working data object(s). */
   virtual void unpack_into_working_data() = 0;

  protected:
   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *force_attr;        ///< @trick_io{**} Force Attribute.
   TrickHLA::Attribute *torque_attr;       ///< @trick_io{**} Torque Attribute.
   TrickHLA::Attribute *mass_attr;         ///< @trick_io{**} Mass Attribute.
   TrickHLA::Attribute *mass_rate_attr;    ///< @trick_io{**} Mass rate Attribute.
   TrickHLA::Attribute *inertia_attr;      ///< @trick_io{**} Inertia matrix Attribute.
   TrickHLA::Attribute *inertia_rate_attr; ///< @trick_io{**} Inertia rate Attribute.

   // Assign to these parameters when setting up the additional data
   // associations for the SpaceFOM TrickHLAObject data for the DynamicalEntity.
   DynamicalEntityData de_packing_data; ///< @trick_units{--} Additional dynamical entity packing data.

   /*! @brief Print out the packing data debug information. */
   virtual void debug_print( std::ostream &stream ) const;

  private:
   // This object is not copyable
   DynamicalEntityBase( DynamicalEntityBase const & );
   DynamicalEntityBase &operator=( DynamicalEntityBase const & );
};

} // namespace SpaceFOM

#endif // SPACEFOM_DYNAMICAL_ENTITY_BASE_HH: Do NOT put anything after this line!
