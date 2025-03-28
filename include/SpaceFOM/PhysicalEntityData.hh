/**
@file SpaceFOM/PhysicalEntityData.hh
@ingroup SpaceFOM
@brief A simple structure that contains the date fields required to encode
and decode a SISO Space Reference FOM PhysicalEntity data type.

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
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, July 2023, --, Initial version }
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, October 2023, --, Made into full class.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_DATA_HH
#define SPACEFOM_PHYSICAL_ENTITY_DATA_HH

// System include files.
#include <iostream>

// SpaceFOM include files.
#include "SpaceFOM/QuaternionData.hh"
#include "SpaceFOM/SpaceTimeCoordinateData.hh"

namespace SpaceFOM
{

class PhysicalEntityData
{

  public:
   char *name;         ///< @trick_units{--} Name of the physical entity.
   char *type;         ///< @trick_units{--} String use to define entity type.
   char *status;       ///< @trick_units{--} String use to define entity status.
   char *parent_frame; ///< @trick_units{--} Parent frame for state representation.

   SpaceTimeCoordinateData state; ///< @trick_units{--} Space time coordinate state.

   double accel[3];     ///< @trick_units{m/s^2} Entity acceleration vector.
   double ang_accel[3]; ///< @trick_units{rad/s^2} Entity angular acceleration vector.
   double cm[3];        ///< @trick_units{m} Position of the entity center of mass in the structural frame.

   QuaternionData body_wrt_struct; ///< @trick_units{--} Orientation of the body frame wrt. the structural frame.

   // Default constructor.
   PhysicalEntityData();

   // Copy constructor.
   /*! @brief Copy constructor for PhysicalEntityData class.
    *  @param source Source data to copy from. */
   PhysicalEntityData( PhysicalEntityData const &source );

   // Destructor.
   /*! @brief Destructor for the SpaceFOM PhysicalEntityData class. */
   virtual ~PhysicalEntityData();

   /*! @brief Assignment operator for PhysicalEntityData class.
    *  @param rhs Right operand data to copy from. */
   virtual PhysicalEntityData &operator=( PhysicalEntityData const &rhs );

   /*! @brief Copy the PhysicalEntityData.
    *  @param source Source PhysicalEntityData to copy from. */
   virtual void copy( PhysicalEntityData const &source );

   // Access functions.
   /*! @brief Set the name of the PhysicalEntityData instance.
    *  @param new_name Name of the PhysicalEntityData instance. */
   virtual void set_name( char const *new_name );

   /*! @brief Set the type string of the PhysicalEntityData.
    *  @param new_type Type string associated with the PhysicalEntityData. */
   virtual void set_type( char const *new_type );

   /*! @brief Set the status string of the PhysicalEntityData.
    *  @param new_status Status string associated with the PhysicalEntityData. */
   virtual void set_status( char const *new_status );

   /*! @brief Set the name of the parent reference frame for the PhysicalEntityData.
    *  @param new_frame The name of the parent reference frame associated
    *  with the PhysicalEntityData. */
   virtual void set_parent_frame( char const *new_frame );

   /*! @brief Print out the data values.
    *  @param stream Output stream. */
   virtual void print_data( std::ostream &stream = std::cout ) const;
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_ENTITY_DATA_HH: Do NOT put anything after this line!
