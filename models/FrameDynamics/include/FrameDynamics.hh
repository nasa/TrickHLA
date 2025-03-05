/**
@ingroup FrameDynamics
@file models/FrameDynamics/include/FrameDynamics.hh
@brief A class to perform a simple propagation of a SpaceFOM Reference Frame
for testing.

@copyright Copyright 2025 United States Government as represented by the
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
@trick_link_dependency{../src/FrameDynamics.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, February 2025, --, SpaceFOM support and testing. }
@revs_end

*/

#ifndef SPACEFOM_FRAME_DYNAMICS_HH
#define SPACEFOM_FRAME_DYNAMICS_HH

// SpaceFOM includes.
#include "SpaceFOM/RefFrameData.hh"

namespace SpaceFOM
{

class FrameDynamics
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__FrameDynamics();

  public:
   // Public constructors and destructors.
   explicit FrameDynamics(); // Default constructor.
   virtual ~FrameDynamics(); // Destructor.

   void default_data();

   void initialize();

   void derivative();

   int integrate();

  public:
   // PhysicalEntity data.
   RefFrameData data; ///< @trick_units{--} Basic frame propagation data.

   double accel_env[3];     ///< @trick_units{m/s2} Environmental acceleration.
   double ang_accel_env[3]; ///< @trick_units{rad/s2} Environmental rotational acceleration.

  protected:
   QuaternionData Q_dot; ///< @trick_units{--} Derivative of the attitude quaternion.

   /*! @brief Load the integration state into the integrator. */
   void load();

   /*! @brief Unload the integration state into the integrator. */
   void unload();

  private:
   // This object is not copyable
   /*! @brief Copy constructor for FrameDynamics class.
    *  @details This constructor is private to prevent inadvertent copies. */
   FrameDynamics( FrameDynamics const &rhs );
   /*! @brief Assignment operator for FrameDynamics class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   FrameDynamics &operator=( FrameDynamics const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_FRAME_DYNAMICS_HH: Do NOT put anything after this line!
