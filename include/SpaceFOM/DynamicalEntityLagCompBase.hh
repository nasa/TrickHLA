/*!
@file SpaceFOM/DynamicalEntityLagCompBase.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM physical entity latency/lag
compensation class.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the PhysicalEntity latency compensation object.

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
@trick_link_dependency{../../source/TrickHLA/LagCompensation.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntityLagCompBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/QuaternionData.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_DYNAMICAL_ENTITY_LAG_COMP_BASE_HH
#define SPACEFOM_DYNAMICAL_ENTITY_LAG_COMP_BASE_HH

// System include files.
#include <iostream>

// Trick includes.

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/LagCompensation.hh"
#include "TrickHLA/Object.hh"

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityBase.hh"
#include "SpaceFOM/PhysicalEntityLagCompBase.hh"
#include "SpaceFOM/QuaternionData.hh"
#include "SpaceFOM/SpaceTimeCoordinateData.hh"

namespace SpaceFOM
{

class DynamicalEntityLagCompBase : public PhysicalEntityLagCompBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__DynamicalEntityLagCompBase();

  public:
   // Public constructors and destructors.
   explicit DynamicalEntityLagCompBase( DynamicalEntityBase &entity_ref ); // Initialization constructor.
   virtual ~DynamicalEntityLagCompBase();                                  // Destructor.

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

   /*! @brief Initialize the callback object to the supplied Object pointer.
    *  @param obj Associated object for this class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   /*! @brief Initialization integration states. */
   virtual void initialize_states();

   /*! @brief Sending side latency compensation callback interface from the
    *  TrickHLALagCompensation class. */
   virtual void send_lag_compensation();

   /*! @brief Receive side latency compensation callback interface from the
    *  TrickHLALagCompensation class. */
   virtual void receive_lag_compensation();

   /*! @brief When lag compensation is disabled, this function is called to
    * bypass the send side lag compensation and your implementation must copy
    * the sim-data to the lag-comp data to effect the bypass. */
   virtual void bypass_send_lag_compensation();

   /*! @brief When lag compensation is disabled, this function is called to
    * bypass the receive side lag compensation and your implementation must
    * copy the lag-comp data to the sim-data to effect the bypass. You must
    * make sure to check the lag-comp data was received before copying to
    * the sim-data otherwise you will be copying stale data. */
   virtual void bypass_receive_lag_compensation();

  protected:
   DynamicalEntityBase &de_entity; ///< @trick_units{--} @trick_io{**}  PhysicalEntity to compensate.

   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *force_attr;        ///< @trick_io{**} Force Attribute.
   TrickHLA::Attribute *torque_attr;       ///< @trick_io{**} Torque Attribute.
   TrickHLA::Attribute *mass_attr;         ///< @trick_io{**} Mass Attribute.
   TrickHLA::Attribute *mass_rate_attr;    ///< @trick_io{**} Mass rate Attribute.
   TrickHLA::Attribute *inertia_attr;      ///< @trick_io{**} Inertia matrix Attribute.
   TrickHLA::Attribute *inertia_rate_attr; ///< @trick_io{**} Inertia rate Attribute.

   double force[3];  ///< @trick_units{N} Entity force vector in struct frame.
   double torque[3]; ///< @trick_units{N*m} Entity torque vector in struct frame.
   double mass;      ///< @trick_units{kg} Entity mass.
   double mass_rate; ///< @trick_units{kg/s} Entity mass rate.

   double inertia[3][3];      ///< @trick_units{kg*m2} Entity inertia matrix, in body frame.
   double inertia_rate[3][3]; ///< @trick_units{kg*m2/s} Entity inertia rate matrix.
   double inertia_inv[3][3];  ///< @trick_units{--} Inverse of the entity inertia matrix.

   double accel_env[3];          ///< @trick_units{m/s2} Computed environmental acceleration.
   double ang_accel_env[3];      ///< @trick_units{rad/s2} Computed environmental rotational acceleration.
   double ang_accel_inertial[3]; ///< @trick_units{rad/s2} Computed inertial rotational acceleration.

   /*! @brief Compensate the state data from the data time to the current scenario time.
    *  @param t_begin Scenario time at the start of the compensation step.
    *  @param t_end   Scenario time at the end of the compensation step. */
   virtual int compensate(
      double const t_begin,
      double const t_end ) = 0;

   /*! @brief Unload the lag compensation state into the packing data. */
   virtual void unload_lag_comp_data();

   /*! @brief Load the packing data into the lag compensation state. */
   virtual void load_lag_comp_data();

   /*! @brief Print out the lag compensation data values.
    *  @param stream Output stream. */
   virtual void print_lag_comp_data( std::ostream &stream = std::cout ) const;

  private:
   // This object is not copyable
   /*! @brief Copy constructor for DynamicalEntityLagCompBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   DynamicalEntityLagCompBase( DynamicalEntityLagCompBase const &rhs );
   /*! @brief Assignment operator for DynamicalEntityLagCompBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   DynamicalEntityLagCompBase &operator=( DynamicalEntityLagCompBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_DYNAMICAL_ENTITY_LAG_COMP_BASE_HH: Do NOT put anything after this line!
