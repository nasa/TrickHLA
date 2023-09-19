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
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntityBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_DYNAMICAL_ENTITY_BASE_HH
#define SPACEFOM_DYNAMICAL_ENTITY_BASE_HH

// SpaceFOM include files.
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

  public:
   // Public constructors and destructors.
   DynamicalEntityBase();          // Default constructor.
   virtual ~DynamicalEntityBase(); // Destructor.

   // Default data.
   /*! @brief Sets up the attributes for a DynamicalEntity using default values.
    *  @param object TrickHLA::Object associated with this DynamicalEntity.
    *  @param sim_obj_name Name of SimObject containing this DynamicalEntity.
    *  @param entity_obj_name Name of the DynamicalEntity object in the SimObject.
    *  @param entity_name Name of the DynamicalEntity instance.
    *  @param parent_ref_frame_name Name of the parent ReferenceFrame for this DynamicalEntity instance.
    *  @param publishes Does this federate publish this DynamicalEntity.
    *  */
   virtual void default_data( TrickHLA::Object *mngr_object,
                              char const       *sim_obj_name,
                              char const       *entity_obj_name,
                              char const       *entity_name,
                              char const       *parent_ref_frame_name,
                              bool              publishes );

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack() = 0;

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack() = 0;

  protected:
   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *force_attr;        ///< @trick_io{**} Force Attribute.
   TrickHLA::Attribute *torque_attr;       ///< @trick_io{**} Torque Attribute.
   TrickHLA::Attribute *mass_attr;         ///< @trick_io{**} Mass Attribute.
   TrickHLA::Attribute *mass_rate_attr;    ///< @trick_io{**} Mass rate Attribute.
   TrickHLA::Attribute *inertia_attr;      ///< @trick_io{**} Inertia matrix Attribute.
   TrickHLA::Attribute *inertia_rate_attr; ///< @trick_io{**} Inertia rate Attribute.

   double force[3];  ///< @trick_units{N} Entity force vector.
   double torque[3]; ///< @trick_units{N*m} Entity torque vector.
   double mass;      ///< @trick_units{kg} Entity mass.
   double mass_rate; ///< @trick_units{kg/s} Entity mass rate.

   double inertia[3][3];      ///< @trick_units{kg*m2} Entity inertia matrix.
   double inertia_rate[3][3]; ///< @trick_units{kg*m2/s} Entity inertia rate matrix.

  private:
   // This object is not copyable
   DynamicalEntityBase( DynamicalEntityBase const & );
   DynamicalEntityBase &operator=( DynamicalEntityBase const & );
};

} // namespace SpaceFOM

#endif // SPACEFOM_DYNAMICAL_ENTITY_BASE_HH: Do NOT put anything after this line!
