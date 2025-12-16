/*!
@file SpaceFOM/PhysicalInterface.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM PhysicalInterface type.

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
@trick_link_dependency{../../source/SpaceFOM/PhysicalInterfaceBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalInterface.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_INTERFACE_HH
#define SPACEFOM_PHYSICAL_INTERFACE_HH

// SpaceFOM includes.
#include "SpaceFOM/PhysicalInterfaceBase.hh"

namespace SpaceFOM
{

class PhysicalInterface : virtual public SpaceFOM::PhysicalInterfaceBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalInterface();

  public:
   // Public constructors and destructors.
   PhysicalInterface();                                                     // Default constructor.
   explicit PhysicalInterface( PhysicalInterfaceData &interface_data_ref ); // Initialization constructor.
   virtual ~PhysicalInterface();                                            // Destructor.

   // Initialize the packing object.
   /*! @brief Set the reference to the physical entity data. */
   virtual void initialize();

   // Access functions.
   /*! @brief Set the name of the PhysicalInterface object instance.
    *  @param new_name Name of the PhysicalInterface object instance. */
   virtual void set_name( std::string const &new_name );

   /*! @brief Set the name of the parent reference frame for the PhysicalInterface.
    *  @param new_parent_name The name of the parent reference frame associated
    *  with the PhysicalInterface. */
   virtual void set_parent( std::string const &new_parent_name );

   /*! @brief Packs the packing data object from the working data object(s),
    *  @details Called from the pack() function to pack the data from the working
    *  data objects(s) into the pe_packing_data object.  */
   virtual void pack_from_working_data();

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  pe_packing_data object into the working data object(s). */
   virtual void unpack_into_working_data();

   /*! @brief Set the reference to the physical entity data.
    *  @param interface_data_ptr Pointer to the PhysicalInterface data instance. */
   virtual void set_data( PhysicalInterfaceData *interface_data_ptr );

   /*! @brief Get the reference to the physical entity data.
    *  @return Pointer to the PhysicalInterface data. */
   virtual PhysicalInterfaceData *get_data()
   {
      return ( interface_data );
   }

  protected:
   PhysicalInterfaceData *interface_data; ///< @trick_units{--} Physical entity data.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for PhysicalInterface class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalInterface( PhysicalInterface const &rhs );
   /*! @brief Assignment operator for PhysicalInterface class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalInterface &operator=( PhysicalInterface const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_INTERFACE_HH: Do NOT put anything after this line!
