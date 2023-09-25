/*!
@file SpaceFOM/PhysicalInterfaceBase.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM physical interface type.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the PhysicalInterface object.

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
@trick_link_dependency{../../source/SpaceFOM/PhysicalInterfaceBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/QuaternionEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial vesrion.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_INTERFACE_BASE_HH
#define SPACEFOM_PHYSICAL_INTERFACE_BASE_HH

// System include files.

// TrickHLA include files.
#include "TrickHLA/OpaqueBuffer.hh"
#include "TrickHLA/Packing.hh"

// SpaceFOM include files.
#include "SpaceFOM/QuaternionEncoder.hh"
#include "SpaceFOM/SpaceTimeCoordinateEncoder.hh"

namespace SpaceFOM
{

class PhysicalInterfaceBase : public TrickHLA::Packing, public TrickHLA::OpaqueBuffer
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalInterfaceBase();

  public:
   // Public constructors and destructors.
   PhysicalInterfaceBase();          // Default constructor.
   virtual ~PhysicalInterfaceBase(); // Destructor.

   // Default data.
   /*! @brief Sets up the attributes for a PhysicalInterface using default values.
    *  @param object TrickHLA::Object associated with this PhysicalInterface.
    *  @param sim_obj_name Name of SimObject containing this PhysicalInterface.
    *  @param interface_obj_name Name of the PhysicalInterface object in the SimObject.
    *  @param interface_name Name of the PhysicalInterface instance.
    *  @param parent_name Name of the parent PhysicalEntity or PhysicalInterface for this PhysicalInterface instance.
    *  @param publishes Does this federate publish this PhysicalInterface.
    *  */
   virtual void default_data( TrickHLA::Object *mngr_object,
                              char const       *sim_obj_name,
                              char const       *interface_obj_name,
                              char const       *interface_name,
                              char const       *parent_name,
                              bool              publishes );

   /*! @brief interface instance initialization routine. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   // Access functions.
   /*! @brief Set the name of the PhysicalInterface object instance.
    *  @param name Name of the PhysicalInterface object instance. */
   virtual void set_name( char const *new_name );

   /*! @brief Get the name of the PhysicalInterface object instance.
    *  @return Name of the PhysicalInterface object instance. */
   virtual char const *get_name()
   {
      return name;
   }

   /*! @brief Set the name of the parent reference frame for the PhysicalInterface.
    *  @param parent_ref_frame The name of the parent reference frame associated
    *  with the PhysicalInterface. */
   virtual void set_parent( char const *new_parent_name );

   /*! @brief Get the name of the parent reference frame associated with the PhysicalInterface.
    *  @return Name of the parent reference frame associated with the PhysicalInterface. */
   virtual char const *get_parent()
   {
      return parent_name;
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
   TrickHLA::Attribute *name_attr;     ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute *parent_attr;   ///< @trick_io{**} Parent entity or interface Attribute.
   TrickHLA::Attribute *position_attr; ///< @trick_io{**} Position Attribute.
   TrickHLA::Attribute *attitude_attr; ///< @trick_io{**} Attitude Attribute.

   char   *name;        ///< @trick_units{--}   Name of the physical interface.
   char   *parent_name; ///< @trick_units{--}   Parent PhysicalEntity or PhysicalInterface.
   double  position[3]; ///< @trick_units{m/s2} Interface position vector.

   // Instantiate the attitude quaternion encoder.
   QuaternionEncoder quat_encoder; ///< @trick_units{--} Interface attitude quaternion encoder.

   // Note: This is a reference that gets assignment from the encoder in the
   // initialization list of the class constructor.  Essentially this is a short cut.
   QuaternionData &attitude; ///< @trick_units{--} Orientation of the interface wrt. the parent.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for PhysicalInterfaceBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalInterfaceBase( PhysicalInterfaceBase const &rhs );
   /*! @brief Assignment operator for PhysicalInterfaceBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalInterfaceBase &operator=( PhysicalInterfaceBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_INTERFACE_BASE_HH: Do NOT put anything after this line!
