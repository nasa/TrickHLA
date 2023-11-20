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

// System include files.

// Trick include files.

// JEOD include files.
#include "dynamics/dyn_body/include/dyn_body.hh"
#include "dynamics/dyn_body/include/body_ref_frame.hh"

// TrickHLA include files.

// SpaceFOM include files.
#include "SpaceFOM/PhysicalInterfaceBase.hh"

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
   JEODPhysicalInterface();          // Default constructor.
   virtual ~JEODPhysicalInterface(); // Destructor.

   // Initialize the packing object.
   /*! @brief Initialize the class assuming that the vehicle point data is set. */
   virtual void initialize();

   // Initialize the packing object.
   /*! @brief Initialize the class to reference to the JEOD vehicle point data.
    *  @param vehicle_point_ptr Pointer to the vehicle point data instance. */
   virtual void initialize( const jeod::BodyRefFrame *vehicle_point_ptr );

   // Initialize the packing object.
   /*! @brief Initialize the class using the JEOD vehicle point ID.
    *  @param dyn_body_ptr Pointer to the vehicle JEOD DynBody instance. */
   virtual void initialize( jeod::DynBody * dyn_body_ptr );

   /*! @brief Packs the packing data object from the working data object(s),
    *  @detail Called from the pack() function to pack the data from the working
    *  data objects(s) into the pe_packing_data object.  */
   virtual void pack_from_working_data();

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @detail Called from the unpack() function to unpack the data in the
    *  pe_packing_data object into the working data object(s). */
   virtual void unpack_into_working_data();

   /*! @brief Set the ID string of the vehicle point for the JEOD Vehicle Point.
    *  @param new_id The ID string for vehicle point associated with the JEOD
    *  DynBody to be associated with this SpaceFOM::PhysicalInterface. */
   virtual void set_vehicle_point_id( char const *new_id );

   /*! @brief Set the reference to the JEOD vehicle point data.
    *  @param vehicle_point_ptr Pointer to the vehicle point data instance. */
   virtual void set_data( const jeod::BodyRefFrame *vehicle_point_ptr )
   {
      vehicle_point_data = vehicle_point_ptr;
      return;
   }

   /*! @brief Get the reference to the JEOD vehicle point data.
    *  @return Pointer to the vehicle point data. */
   virtual const jeod::BodyRefFrame *get_data()
   {
      return ( vehicle_point_data );
   }

  protected:
   char                     * vehicle_point_id;   ///< @trick_units{--} JEOD vehicle point ID.
   const jeod::BodyRefFrame * vehicle_point_data; ///< @trick_units{--} JEOD vehicle point data.

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
