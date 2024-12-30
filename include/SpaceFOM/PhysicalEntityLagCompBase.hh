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

@tldh
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/LagCompensation.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityLagCompBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/QuaternionData.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_BASE_HH
#define SPACEFOM_PHYSICAL_ENTITY_LAG_COMP_BASE_HH

// System include files.
#include <iostream>

// Trick includes.

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/LagCompensation.hh"
#include "TrickHLA/Object.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityBase.hh"
#include "SpaceFOM/QuaternionData.hh"
#include "SpaceFOM/SpaceTimeCoordinateData.hh"

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
   explicit PhysicalEntityLagCompBase( PhysicalEntityBase &entity_ref ); // Initialization constructor.
   virtual ~PhysicalEntityLagCompBase();                                 // Destructor.

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

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  protected:
   PhysicalEntityBase &entity; ///< @trick_units{--} @trick_io{**}  PhysicalEntity to compensate.

   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *name_attr;         ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute *type_attr;         ///< @trick_io{**} Type type Attribute.
   TrickHLA::Attribute *status_attr;       ///< @trick_io{**} Status Attribute.
   TrickHLA::Attribute *parent_frame_attr; ///< @trick_io{**} Parent reference frame Attribute.
   TrickHLA::Attribute *state_attr;        ///< @trick_io{**} State Attribute.
   TrickHLA::Attribute *accel_attr;        ///< @trick_io{**} Acceleration Attribute.
   TrickHLA::Attribute *ang_accel_attr;    ///< @trick_io{**} Rotational acceleration Attribute.
   TrickHLA::Attribute *cm_attr;           ///< @trick_io{**} Center of mass Attribute.
   TrickHLA::Attribute *body_frame_attr;   ///< @trick_io{**} Body frame orientation Attribute.

   double compensate_dt; ///< @trick_units{s} Time difference between publish time and receive time.

   SpaceTimeCoordinateData lag_comp_data; ///< @trick_units{--} Compensated state data.
   QuaternionData          Q_dot;         ///< @trick_units{--} Computed attitude quaternion rate.
   double                  accel[3];      ///< @trick_units{m/s2} Entity acceleration vector.
   double                  ang_accel[3];  ///< @trick_units{rad/s2} Entity rotational acceleration vector.
   double                  cm[3];         ///< @trick_units{m} Position of the entity center of mass in the structural frame.

   QuaternionData body_wrt_struct; ///< @trick_units{--} Orientation of the body frame wrt. the structural frame.

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
   virtual void print_lag_comp_data( std::ostream &stream = std::cout );

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
