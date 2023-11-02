/**
@file models/EntityDynamics/include/EntityDynamics.hh
@ingroup SpaceFOM
@brief A class to perform a simple propagation of a SpaceFOM PhysicalEntity
or DynamicalEntity for testing.

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
@trick_link_dependency{../../../source/SpaceFOM/SpaceTimeCoordinateData.cpp}
@trick_link_dependency{../../../source/SpaceFOM/QuaternionData.cpp}
@trick_link_dependency{../src/EntityDynamics.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, October 2023, --, SpaceFOM support and testing. }
@revs_end

*/

#ifndef SPACEFOM_ENTITY_DYNAMICS_HH
#define SPACEFOM_ENTITY_DYNAMICS_HH

// SpaceFOM includes.
#include "SpaceFOM/PhysicalEntityData.hh"
#include "SpaceFOM/DynamicalEntityData.hh"

namespace SpaceFOM {

class EntityDynamics{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__EntityDynamics();

  public:

   void default_data();

   void initialize();

   void derivative();

   int integrate();

  public:

   // PhysicalEntity data.
   PhysicalEntityData  pe_data; ///< @trick_units{--} Basic entity propagation data.
   DynamicalEntityData de_data; ///< @trick_units{--} Parameters needed for active entity.

  protected:

   QuaternionData Q_dot; ///< @trick_units{--} Derivative of the attitude quaternion.

   /*! @brief Load the integration state into the integrator. */
   void load();

   /*! @brief Unload the integration state into the integrator. */
   void unload();

};

} // namespace SpaceFOM

#endif // SPACEFOM_ENTITY_DYNAMICS_HH: Do NOT put anything after this line!
