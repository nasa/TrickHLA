/*!
@file SpaceFOM/PhysicalEntityBase.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM physical entity type.

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
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/OpaqueBuffer.cpp}
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/QuaternionEncoder.cpp}
@trick_link_dependency{../../source/SpaceFOM/SpaceTimeCoordinateEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Refactored.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_BASE_HH
#define SPACEFOM_PHYSICAL_ENTITY_BASE_HH

// System include files.
#include <iostream>

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/OpaqueBuffer.hh"
#include "TrickHLA/Packing.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityData.hh"
#include "SpaceFOM/QuaternionEncoder.hh"
#include "SpaceFOM/SpaceTimeCoordinateEncoder.hh"

namespace SpaceFOM
{

class PhysicalEntityBase : public TrickHLA::Packing, public TrickHLA::OpaqueBuffer
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalEntityBase();

   // Make the Conditional class a friend.
   friend class PhysicalEntityConditionalBase;

   // Make the Lag Compensation class a friend.
   friend class PhysicalEntityLagCompBase;

  public:
   // Public constructors and destructors.
   PhysicalEntityBase();          // Default constructor.
   virtual ~PhysicalEntityBase(); // Destructor.

   // Default data.
   /*! @brief Sets up the attributes for a PhysicalEntity using default values.
    *  @param sim_obj_name Name of SimObject containing this PhysicalEntity.
    *  @param entity_obj_name Name of the PhysicalEntity object in the SimObject.
    *  @param entity_name Name of the PhysicalEntity instance.
    *  @param parent_ref_frame_name Name of the parent ReferenceFrame for this PhysicalEntity instance.
    *  @param publishes Does this federate publish this PhysicalEntity.
    *  @param mngr_object TrickHLA::Object associated with this PhysicalEntity.
    *  */
   virtual void base_config( char const       *sim_obj_name,
                             char const       *entity_obj_name,
                             char const       *entity_name,
                             char const       *parent_ref_frame_name,
                             bool              publishes,
                             TrickHLA::Object *mngr_object = NULL );

   /*! @brief Function to begin the configuration/initialization of the
    *  PhysicalEntity.
    *  This function needs to be called prior to TrickHLA initialization if
    *  the PhysicalEntity object is not being configured with an
    *  initialization constructor. */
   void configure(); // cppcheck-suppress [duplInheritedMember]

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   // Access functions.
   /*! @brief Set the name of the PhysicalEntity object instance.
    *  @param new_name Name of the PhysicalEntity object instance. */
   virtual void set_name( char const *new_name );

   /*! @brief Get the name of the PhysicalEntity object instance.
    *  @return Name of the PhysicalEntity object instance. */
   virtual char const *get_name()
   {
      return pe_packing_data.name;
   }

   /*! @brief Set the type string of the PhysicalEntity.
    *  @param new_type Type string associated with the PhysicalEntity. */
   virtual void set_type( char const *new_type );

   /*! @brief Get the type string associated with the PhysicalEntity.
    *  @return Type string associated with the PhysicalEntity. */
   virtual char const *get_type()
   {
      return pe_packing_data.type;
   }

   /*! @brief Set the status string of the PhysicalEntity.
    *  @param new_status Status string associated with the PhysicalEntity. */
   virtual void set_status( char const *new_status );

   /*! @brief Get the status string associated with the PhysicalEntity.
    *  @return Status string associated with the PhysicalEntity. */
   virtual char const *get_status()
   {
      return pe_packing_data.status;
   }

   /*! @brief Set the name of the parent reference frame for the PhysicalEntity.
    *  @param new_frame The name of the parent reference frame associated
    *  with the PhysicalEntity. */
   virtual void set_parent_frame( char const *new_frame );

   /*! @brief Get the name of the parent reference frame associated with the PhysicalEntity.
    *  @return Name of the parent reference frame associated with the PhysicalEntity. */
   virtual char const *get_parent_frame()
   {
      return pe_packing_data.parent_frame;
   }

   /*! @brief Get the current scenario time associated with the PhysicalEntity.
    *  @return Current time associated with the PhysicalEntity. */
   virtual double const get_time()
   {
      return pe_packing_data.state.time;
   }

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

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

   /*! @brief Get a constant reference to the PhysicalEntity packing data.
    *  @return A constant reference to the PhysicalEntity packing data. */
   PhysicalEntityData const &get_packing_data() { return ( pe_packing_data ); };

  protected:
   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *name_attr;         ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute *type_attr;         ///< @trick_io{**} Type type Attribute.
   TrickHLA::Attribute *status_attr;       ///< @trick_io{**} Status Attribute.
   TrickHLA::Attribute *parent_frame_attr; ///< @trick_io{**} Parent reference frame Attribute.
   TrickHLA::Attribute *state_attr;        ///< @trick_io{**} State Attribute.
   TrickHLA::Attribute *accel_attr;        ///< @trick_io{**} Acceleration Attribute.
   TrickHLA::Attribute *ang_accel_attr;    ///< @trick_io{**} Angular acceleration Attribute.
   TrickHLA::Attribute *cm_attr;           ///< @trick_io{**} Center of mass Attribute.
   TrickHLA::Attribute *body_frame_attr;   ///< @trick_io{**} Body frame orientation Attribute.

   // Assign to these parameters when setting up the data associations for the
   // SpaceFOM TrickHLAObject data for the PhysicalEntity.
   PhysicalEntityData pe_packing_data; ///< @trick_units{--} Physical entity packing data.

   // Instantiate the aggregate data encoders
   SpaceTimeCoordinateEncoder stc_encoder;  ///< @trick_units{--} Entity state encoder.
   QuaternionEncoder          quat_encoder; ///< @trick_units{--} Attitude quaternion encoder.

   /*! @brief Print out the packing data debug information.
    *  @param stream Output stream. */
   virtual void debug_print( std::ostream &stream = std::cout ) const;

  private:
   // This object is not copyable
   /*! @brief Copy constructor for PhysicalEntityBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalEntityBase( PhysicalEntityBase const &rhs );
   /*! @brief Assignment operator for PhysicalEntityBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalEntityBase &operator=( PhysicalEntityBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_ENTITY_BASE_HH: Do NOT put anything after this line!
