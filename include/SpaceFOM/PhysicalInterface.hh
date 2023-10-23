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

// System include files.

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/PhysicalInterfaceBase.hh"
#include "SpaceFOM/PhysicalInterfaceData.hh"

namespace TrickHLA
{
class Packing;
class OpaqueBuffer;
} // namespace TrickHLA

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
   PhysicalInterface();          // Default constructor.
   virtual ~PhysicalInterface(); // Destructor.

   // Initialize the packing object.
   /*! @brief Set the reference to the physical entity data. */
   virtual void initialize();

   // Initialize the packing object.
   /*! @brief Set the reference to the physical entity data.
    *  @param interface_data_ptr Pointer to the PhysicalInterface data instance. */
   virtual void initialize( PhysicalInterfaceData *interface_data_ptr );

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

   /*! @brief Set the reference to the physical entity data.
    *  @param interface_data_ptr Pointer to the PhysicalInterface data instance. */
   virtual void set_data( PhysicalInterfaceData *interface_data_ptr )
   {
      interface_data = interface_data_ptr;
      return;
   }

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
