/*!
@file SpaceFOM/PhysicalEntityBase.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM physical entity type.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the PhysicalEntity object.

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

   // Initialization routines.
   virtual void initialize();

   // Access functions.
   virtual void        set_name( char const *name );
   virtual char const *get_name()
   {
      return name;
   }

   virtual void        set_type( char const *type );
   virtual char const *get_type()
   {
      return type;
   }

   virtual void        set_status( char const *status );
   virtual char const *get_status()
   {
      return status;
   }

   virtual void        set_parent_ref_frame( char const *parent_ref_frame );
   virtual char const *get_parent_ref_frame()
   {
      return parent_ref_frame;
   }

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

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  protected:
   bool                 initialized; ///< @trick_units{--} Initialization indication flag.
   TrickHLA::Attribute *entity_attr; ///< @trick_io{**} TrickHLA entity Attribute.

   char  *name;             ///< @trick_units{--} Name of this entity(required).
   char  *type;             ///< @trick_units{--} True underlying type for this entity(optional).
   char  *status;           ///< @trick_units{--} Status string for this entity (optional).
   char  *parent_ref_frame; ///< @trick_units{--} Name of this entity's parent frame(required).
   double accel[3];         ///< @trick_units{m/s2} Vehicle inertial acceleration (optional).
   double rot_accel[3];     ///< @trick_units{rad/s2} Angular body accels, body referenced (optional).
   double cm[3];            ///< @trick_units{m} Center of mass location in vehicle structural frame (required).

   SpaceTimeCoordinateData &state;           ///< @trick_units{--} SpaceTimeCoordinate from encoder (required).
   QuaternionData          &body_wrt_struct; ///< @trick_units{--} Attitude quaternion for body frame w.r.t. structural frame.(optional)

   // Instantiate the Space/Time Coordinate encoder
   SpaceTimeCoordinateEncoder stc_encoder;  ///< @trick_units{--} Entity state encoder.
   QuaternionEncoder          quat_encoder; ///< @trick_units{--} Attitude quaternion encoder.

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
