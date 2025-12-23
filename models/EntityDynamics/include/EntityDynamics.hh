/**
@ingroup EntityDynamics
@file models/EntityDynamics/include/EntityDynamics.hh
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

@python_module{TrickHLAModel}

@tldh
@trick_link_dependency{../../../source/SpaceFOM/SpaceTimeCoordinateData.cpp}
@trick_link_dependency{../../../source/SpaceFOM/QuaternionData.cpp}
@trick_link_dependency{../src/EntityDynamics.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, November 2023, --, SpaceFOM support and testing. }
@revs_end

*/

#ifndef SPACEFOM_ENTITY_DYNAMICS_HH
#define SPACEFOM_ENTITY_DYNAMICS_HH

// SpaceFOM includes.
#include "SpaceFOM/DynamicalEntityData.hh"
#include "SpaceFOM/PhysicalEntityData.hh"

namespace SpaceFOM
{

class EntityDynamics
{

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
   // Public constructors and destructors.
   explicit EntityDynamics(); // Default constructor.
   virtual ~EntityDynamics(); // Destructor.

   static void default_data();

   void initialize();

   void derivative();

   int integrate();

  public:
   // PhysicalEntity data.
   PhysicalEntityData  pe_data; ///< @trick_units{--} Basic entity propagation data.
   DynamicalEntityData de_data; ///< @trick_units{--} Parameters needed for active entity.

   double accel_env[3];          ///< @trick_units{m/s2} Computed environmental acceleration.
   double ang_accel_env[3];      ///< @trick_units{rad/s2} Computed environmental rotational acceleration.
   double ang_accel_inertial[3]; ///< @trick_units{rad/s2} Computed inertial rotational acceleration.

  protected:
   QuaternionData Q_dot; ///< @trick_units{--} Derivative of the attitude quaternion.

   double I_inv[3][3]; ///< @trick_units{--} Inverse of the inertia matrix.

   /*! @brief Load the integration state into the integrator. */
   void load();

   /*! @brief Unload the integration state into the integrator. */
   void unload();

  private:
   // This object is not copyable
   /*! @brief Copy constructor for EntityDynamics class.
    *  @details This constructor is private to prevent inadvertent copies. */
   EntityDynamics( EntityDynamics const &rhs );
   /*! @brief Assignment operator for EntityDynamics class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   EntityDynamics &operator=( EntityDynamics const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_ENTITY_DYNAMICS_HH: Do NOT put anything after this line!
