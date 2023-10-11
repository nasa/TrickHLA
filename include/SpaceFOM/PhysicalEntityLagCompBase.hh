/*!
@file SpaceFOM/PhysicalEntityLagCompBase.hh
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

@tldh:q
@trick_link_dependency{../../source/TrickHLA/LagCompensation.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityLagCompBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_BASE_HH
#define SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_BASE_HH

// System include files.

// Trick includes.

// TrickHLA include files.
#include "TrickHLA/LagCompensation.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityBase.hh"
#include "SpaceFOM/SpaceTimeCoordinateData.h"

namespace SpaceFOM
{

class PhysicalEntityLagCompBase : public TrickHLA::LagCompensation
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__PhysicalEntityLagCompBase();

  public:
   // Public constructors and destructors.
   PhysicalEntityLagCompBase( PhysicalEntityBase & entity_ref ); // Initialization constructor.
   virtual ~PhysicalEntityLagCompBase(); // Destructor.

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

   /*! @brief Initialize the callback object to the supplied Object pointer.
    *  @param obj Associated object for this class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   /*! @brief Initialization integration states. */
   virtual void initialize_states();

   /*! @brief Sending side latency compensation callback interface from the
    *  TrickHLALagCompensation class. */
   virtual void send_lag_compensation() = 0;

   /*! @brief Receive side latency compensation callback interface from the
    *  TrickHLALagCompensation class. */
   virtual void receive_lag_compensation() = 0;

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  protected:

   PhysicalEntityBase & entity; ///< @trick_units{--} @trick_io{**}  PhysicalEntity to compensate.

   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *name_attr;         ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute *type_attr;         ///< @trick_io{**} Type type Attribute.
   TrickHLA::Attribute *status_attr;       ///< @trick_io{**} Status Attribute.
   TrickHLA::Attribute *parent_frame_attr; ///< @trick_io{**} Parent reference frame Attribute.
   TrickHLA::Attribute *state_attr;        ///< @trick_io{**} State Attribute.
   TrickHLA::Attribute *accel_attr;        ///< @trick_io{**} Acceleration Attribute.
   TrickHLA::Attribute *rot_accel_attr;    ///< @trick_io{**} Rotational acceleration Attribute.
   TrickHLA::Attribute *cm_attr;           ///< @trick_io{**} Center of mass Attribute.
   TrickHLA::Attribute *body_frame_attr;   ///< @trick_io{**} Body frame orientation Attribute.

   double compensate_dt; ///< @trick_units{s} Time difference between publish time and receive time.

   SpaceTimeCoordinateData lag_comp_data; ///< @trick_units{--} Compensated state data.
   QuaternionData          Q_dot;         ///< @trick_units{--} Computed attitude quaternion rate.
   double accel[3];     ///< @trick_units{m/s2} Entity acceleration vector.
   double rot_accel[3]; ///< @trick_units{rad/s2} Entity rotational acceleration vector.

   /*! @brief Computer the rate of the attitude quaterion.
    *  @param stc Space time coordinate state data.
    *  @param Q_dot Reference to the quaternion rate. */
   static void compute_Q_dot(
      const double   quat_scalar,
      const double   quat_vector[3],
      const double   omega[3],
            double & qdot_scalar,
            double   qdot_vector[3] );

   /*! @brief Compensate the state data from the data time to the current scenario time.
    *  @param t_begin Scenario time at the start of the compensation step.
    *  @param t_end   Scenario time at the end of the compensation step. */
   virtual int compensate(
      const double t_begin,
      const double t_end   ) = 0;

   /*! @brief Copy the lag compensation state into the entity state. */
   virtual void copy_state_to_entity();

   /*! @brief Copy the entity state into the lag compensation state. */
   virtual void copy_state_from_entity();

   /*! @brief Print out the lag compensation data values. */
   virtual void print_lag_comp_data();

  private:
   // This object is not copyable
   /*! @brief Copy constructor for PhysicalEntityLagCompBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   PhysicalEntityLagCompBase( PhysicalEntityLagCompBase const &rhs );
   /*! @brief Assignment operator for PhysicalEntityLagCompBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   PhysicalEntityLagCompBase &operator=( PhysicalEntityLagCompBase const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_BASE_HH: Do NOT put anything after this line!
