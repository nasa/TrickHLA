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
@trick_link_dependency{../../source/TrickHLA/OpaqueBuffer.cpp}
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/SpaceTimeCoordinateEncoder.cpp}
@trick_link_dependency{../../source/SpaceFOM/QuaternionEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_BASE_HH
#define SPACEFOM_PHYSICAL_ENTITY_BASE_HH

// System include files.

// TrickHLA include files.
#include "TrickHLA/OpaqueBuffer.hh"
#include "TrickHLA/Packing.hh"

// SpaceFOM include files.
#include "SpaceFOM/QuaternionEncoder.hh"
#include "SpaceFOM/SpaceTimeCoordinateEncoder.hh"

namespace TrickHLA
{
class Packing;
class OpaqueBuffer;
} // namespace TrickHLA

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

  public:
   // Public constructors and destructors.
   PhysicalEntityBase();          // Default constructor.
   virtual ~PhysicalEntityBase(); // Destructor.

   // Default data.
   /*! @brief Sets up the attributes for a PhysicalEntity using default values.
    *  @param object TrickHLA::Object associated with this PhysicalEntity.
    *  @param sim_obj_name Name of SimObject containing this PhysicalEntity.
    *  @param entity_obj_name Name of the ReferenceFrame object in the SimObject.
    *  @param entity_name Name of the PhysicalEntity instance.
    *  @param parent_ref_frame_name Name of the parent ReferenceFrame for this PhysicalEntity instance.
    *  @param publishes Does this federate publish this PhysicalEntity.
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

   // Access functions.
   /*! @brief Set the name of the PhysicalEntity object instance.
    *  @param name Name of the PhysicalEntity object instance. */
   virtual void set_name( char const *name );

   /*! @brief Get the name of the PhysicalEntity object instance.
    *  @return Name of the PhysicalEntity object instance. */
   virtual char const *get_name()
   {
      return name;
   }

   /*! @brief Set the type string of the PhysicalEntity.
    *  @param type Type string associated with the PhysicalEntity. */
   virtual void set_type( char const *type );

   /*! @brief Get the type string associated with the PhysicalEntity.
    *  @return Type string associated with the PhysicalEntity. */
   virtual char const *get_type()
   {
      return type;
   }

   /*! @brief Set the status string of the PhysicalEntity.
    *  @param status Status string associated with the PhysicalEntity. */
   virtual void set_status( char const *status );

   /*! @brief Get the status string associated with the PhysicalEntity.
    *  @return Status string associated with the PhysicalEntity. */
   virtual char const *get_status()
   {
      return status;
   }

   /*! @brief Set the name of the parent reference frame for the PhysicalEntity.
    *  @param parent_ref_frame The name of the parent reference frame associated
    *  with the PhysicalEntity. */
   virtual void set_parent_ref_frame( char const *parent_ref_frame );

   /*! @brief Get the name of the parent reference frame associated with the PhysicalEntity.
    *  @return Name of the parent reference frame associated with the PhysicalEntity. */
   virtual char const *get_parent_ref_frame()
   {
      return parent_frame;
   }

   /*! @brief Get the current scenario time associated with the PhysicalEntity.
    *  @return Name of the parent reference frame associated with the PhysicalEntity. */
   double const get_time()
   {
      return state.time;
   }

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack() = 0;

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack() = 0;

   // Access to protected data.
   virtual void set_object( TrickHLA::Object *mngr_obj );

   // Access to protected data.
   virtual TrickHLA::Object *get_object()
   {
      return object;
   }

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  protected:
   bool initialized; ///< @trick_units{--} Initialization indication flag.

   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *name_attr;         ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute *type_attr;         ///< @trick_io{**} Type type Attribute.
   TrickHLA::Attribute *status_attr;       ///< @trick_io{**} Status Attribute.
   TrickHLA::Attribute *parent_frame_attr; ///< @trick_io{**} Parent reference frame Attribute.
   TrickHLA::Attribute *state_attr;        ///< @trick_io{**} State Attribute.
   TrickHLA::Attribute *accel_attr;        ///< @trick_io{**} Acceleration Attribute.
   TrickHLA::Attribute *rot_accel_attr;    ///< @trick_io{**} Rotational acceleration Attribute.
   TrickHLA::Attribute *cm_attr;           ///< @trick_io{**} Center of mass Attribute.
   TrickHLA::Attribute *body_frame_attr;   ///< @trick_io{**} Body frame orientation Attribute.

   // Instantiate the Space/Time Coordinate encoder
   SpaceTimeCoordinateEncoder stc_encoder;  ///< @trick_units{--} Entity state encoder.
   QuaternionEncoder          quat_encoder; ///< @trick_units{--} Attitude quaternion encoder.

   char *name;         ///< @trick_units{--} Name of the physical entity.
   char *type;         ///< @trick_units{--} String use to define entity type.
   char *status;       ///< @trick_units{--} String use to define entity status.
   char *parent_frame; ///< @trick_units{--} Parent frame for state representation.

   SpaceTimeCoordinateData &state; ///< @trick_units{--} Space time coordinate state.

   double accel[3];     ///< @trick_units{m/s2} Entity acceleration vector.
   double rot_accel[3]; ///< @trick_units{rad/s2} Entity rotational acceleration vector.
   double cm[3];        ///< @trick_units{m} Position of the entity center of mass in the structural frame.

   QuaternionData &body_wrt_struct; ///< @trick_units{--} Orientation of the body frame wrt. the structural frame.

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
