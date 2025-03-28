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
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
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
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/OpaqueBuffer.hh"
#include "TrickHLA/Packing.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalInterfaceData.hh"
#include "SpaceFOM/QuaternionEncoder.hh"

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

   // Make the Conditional class a friend.
   friend class PhysicalInterfaceConditionalBase;

   // Make the Lag Compensation class a friend.
   friend class PhysicalInterfaceLagCompBase;

  public:
   // Public constructors and destructors.
   PhysicalInterfaceBase();          // Default constructor.
   virtual ~PhysicalInterfaceBase(); // Destructor.

   // Default data.
   /*! @brief Sets up the attributes for a PhysicalInterface using default values.
    *  @param sim_obj_name          Name of SimObject containing this PhysicalInterface.
    *  @param interface_obj_name    Name of the PhysicalInterface object in the SimObject.
    *  @param interface_name        Name of the PhysicalInterface instance.
    *  @param interface_parent_name Name of the parent PhysicalEntity or PhysicalInterface for this PhysicalInterface instance.
    *  @param publishes             Does this federate publish this PhysicalInterface.
    *  @param mngr_object           TrickHLA::Object associated with this PhysicalInterface.
    *  */
   virtual void base_config( char const       *sim_obj_name,
                             char const       *interface_obj_name,
                             char const       *interface_name,
                             char const       *interface_parent_name,
                             bool              publishes,
                             TrickHLA::Object *mngr_object = NULL );

   // Pre-initialize the packing object.
   /*! @brief Function to begin the initialization/configuration of the
    *  PhysicalInterface.
    *  This function needs to be called prior to TrickHLA initialization if
    *  the PhysicalInterface object is not being configured with an
    *  initialization constructor. */
   void configure(); // cppcheck-suppress [duplInheritedMember]

   /*! @brief Interface instance initialization routine. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   // Access functions.
   /*! @brief Set the name of the PhysicalInterface object instance.
    *  @param new_name Name of the PhysicalInterface object instance. */
   virtual void set_name( char const *new_name );

   /*! @brief Get the name of the PhysicalInterface object instance.
    *  @return Name of the PhysicalInterface object instance. */
   virtual char const *get_name()
   {
      return packing_data.name;
   }

   /*! @brief Set the name of the parent reference frame for the PhysicalInterface.
    *  @param new_parent_name The name of the parent reference frame associated
    *  with the PhysicalInterface. */
   virtual void set_parent( char const *new_parent_name );

   /*! @brief Get the name of the parent reference frame associated with the PhysicalInterface.
    *  @return Name of the parent reference frame associated with the PhysicalInterface. */
   virtual char const *get_parent()
   {
      return packing_data.parent_name;
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

  protected:
   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *name_attr;     ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute *parent_attr;   ///< @trick_io{**} Parent entity or interface Attribute.
   TrickHLA::Attribute *position_attr; ///< @trick_io{**} Position Attribute.
   TrickHLA::Attribute *attitude_attr; ///< @trick_io{**} Attitude Attribute.

   // Assign to these parameters when setting up the data associations for the
   // SpaceFOM TrickHLAObject data for the PhysicalInterface.
   PhysicalInterfaceData packing_data; ///< @trick_units{--} Physical interface packing data.

   // Instantiate the attitude quaternion encoder.
   QuaternionEncoder quat_encoder; ///< @trick_units{--} Interface attitude quaternion encoder.

   /*! @brief Print out the interface data values.
    *  @param stream Output stream. */
   virtual void print_data( std::ostream &stream = std::cout ) const;

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
