/*!
@file JEOD/JEODPhysicalInterface.hh
@ingroup JEOD
@brief This class provides data packing for the SpaceFOM PhysicalInterface and
the interface with a JEOD MassPointState instance.

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
@trick_link_dependency{../../source/JEOD/JEODPhysicalInterface.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_JEOD_PHYSICAL_INTERFACE_HH
#define SPACEFOM_JEOD_PHYSICAL_INTERFACE_HH

// SpaceFOM includes.
#include "SpaceFOM/PhysicalInterfaceBase.hh"

namespace jeod
{
class BodyRefFrame;
class DynBody;
} /* namespace jeod */

namespace TrickHLA
{
class Packing;
class OpaqueBuffer;
} // namespace TrickHLA

namespace SpaceFOM
{

class JEODPhysicalInterface : virtual public SpaceFOM::PhysicalInterfaceBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__JEODPhysicalInterface();

  public:
   // Public constructors and destructors.
   JEODPhysicalInterface();                                       // Default constructor.
   explicit JEODPhysicalInterface( jeod::DynBody &dyn_body_ref ); // Initialization constructor.
   JEODPhysicalInterface( jeod::DynBody      &dyn_body_ref,
                          jeod::BodyRefFrame &vehicle_point_ref ); // Initialization constructor.
   virtual ~JEODPhysicalInterface();                               // Destructor.

   /*! @brief Make sure that the JEODPhysicalInterface is configured properly. */
   virtual void configure();

   // Initialize the packing object.
   /*! @brief Initialize the class assuming that the vehicle point data is set. */
   virtual void initialize();

   /*! @brief Packs the packing data object from the working data object(s),
    *  @details Called from the pack() function to pack the data from the working
    *  data objects(s) into the pe_packing_data object.  */
   virtual void pack_from_working_data();

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  pe_packing_data object into the working data object(s). */
   virtual void unpack_into_working_data();

   /*! @brief Set the ID string of the vehicle point for the JEOD Vehicle Point.
    *  @param new_id The ID string for vehicle point associated with the JEOD
    *  DynBody to be associated with this SpaceFOM::PhysicalInterface. */
   virtual void set_vehicle_point_id( char const *new_id );

   /*! @brief Set the DynBody references.
    *  @param dyn_body_ptr Pointer to the associated DynBody. */
   void set_data( jeod::DynBody *dyn_body_ptr );

   /*! @brief Set the Vehicle Point references.
    *  @param vehicle_point_ptr Pointer to the associated BodyRefFrame. */
   void set_data( jeod::BodyRefFrame *vehicle_point_ptr );

   /*! @brief Set the DynBody and Vehicle Point references.
    *  @param dyn_body_ptr      Pointer to the associated DynBody.
    *  @param vehicle_point_ptr Pointer to the associated BodyRefFrame. */
   void set_data( jeod::DynBody      *dyn_body_ptr,
                  jeod::BodyRefFrame *vehicle_point_ptr );

   /*! @brief Get the reference to the JEOD vehicle point data.
    *  @return Pointer to the vehicle point data. */
   virtual jeod::BodyRefFrame const *get_data()
   {
      return ( vehicle_point_data );
   }

  protected:
   jeod::DynBody            *dyn_body;           ///< @trick_units{--} JEOD DynBody..
   char                     *vehicle_point_id;   ///< @trick_units{--} JEOD vehicle point ID.
   jeod::BodyRefFrame const *vehicle_point_data; ///< @trick_units{--} JEOD vehicle point data.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for JEODPhysicalInterface class.
    *  @details This constructor is private to prevent inadvertent copies. */
   JEODPhysicalInterface( JEODPhysicalInterface const &rhs );
   /*! @brief Assignment operator for JEODPhysicalInterface class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   JEODPhysicalInterface &operator=( JEODPhysicalInterface const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_JEOD_PHYSICAL_INTERFACE_HH: Do NOT put anything after this line!
