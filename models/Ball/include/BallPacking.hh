/*!
@file Ball/BallPacking.hh
@ingroup Ball
@brief Definition of the TrickHLA Ball packing class.

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
@trick_link_dependency{../../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../../source/TrickHLA/OpaqueBuffer.cpp}
@trick_link_dependency{../../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../src/BallPacking.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, April 2025, --, Initial version.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_BALL_PACKING_H
#define TRICKHLA_MODEL_BALL_PACKING_H

// System include files.
#include <iostream>

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/OpaqueBuffer.hh"
#include "TrickHLA/Packing.hh"

// Ball model include files.
#include "Ball/include/ball_state.h"

namespace TrickHLAModel
{

class BallPacking : public TrickHLA::Packing, public TrickHLA::OpaqueBuffer
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__BallPacking();

   // Make the Conditional class a friend.
   friend class BallConditionalBase;

   // Make the Lag Compensation class a friend.
   friend class BallLagCompBase;

  public:
   // Public constructors and destructors.
   BallPacking();                                     // Default constructor.
   explicit BallPacking( BallState &ball_state_ref ); // Initialization constructor.
   virtual ~BallPacking();                            // Destructor.

   // Default data.
   /*! @brief Sets up the attributes for a Ball using default values.
    *  @param sim_obj_name Name of SimObject containing this Ball.
    *  @param entity_obj_name Name of the Ball object in the SimObject.
    *  @param entity_name Name of the Ball instance.
    *  @param parent_ref_frame_name Name of the parent ReferenceFrame for this Ball instance.
    *  @param publishes Does this federate publish this Ball.
    *  @param mngr_object TrickHLA::Object associated with this Ball.
    *  */
   virtual void base_config( char const       *sim_obj_name,
                             char const       *ball_obj_name,
                             char const       *ball_name,
                             bool              publishes,
                             TrickHLA::Object *mngr_object = NULL );

   /*! @brief Function to begin the configuration/initialization of the
    *  Ball.
    *  This function needs to be called prior to TrickHLA initialization if
    *  the Ball object is not being configured with an
    *  initialization constructor. */
   void configure(); // cppcheck-suppress [duplInheritedMember]

   /*! @brief Entity instance initialization routine. */
   virtual void initialize();

   /*! @brief Initialization callback as part of the TrickHLA::Packing functions.
    *  @param obj Object associated with this packing class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

   // Access functions.
   /*! @brief Set the name of the Ball object instance.
    *  @param new_name Name of the Ball object instance. */
   virtual void set_name( char const *new_name );

   /*! @brief Get the name of the Ball object instance.
    *  @return Name of the Ball object instance. */
   virtual char const *get_name()
   {
      return name;
   }

   /*! @brief Set the Ball state position.
    *  @param new_pos New Ball position. */
   virtual void set_position( char const *new_pos )
   {
      position[0] = new_pos[0];
      position[1] = new_pos[1];
      return;
   }

   /*! @brief Get the position of the Ball.
    *  @param pos Current Ball position. */
   virtual void get_position( double *pos )
   {
      pos[0] = position[0];
      pos[1] = position[1];
      return;
   }

   /*! @brief Set the Ball state velocity.
    *  @param new_pos New Ball velocity. */
   virtual void set_velocity( char const *new_vel )
   {
      velocity[0] = new_vel[0];
      velocity[1] = new_vel[1];
      return;
   }

   /*! @brief Get the velocity of the Ball.
    *  @param vel Current Ball velocity. */
   virtual void get_velocity( double *vel )
   {
      vel[0] = velocity[0];
      vel[1] = velocity[1];
      return;
   }

   /*! @brief Set the Ball state acceleration.
    *  @param new_accel New Ball acceleration. */
   virtual void set_acceleration( char const *new_accel )
   {
      acceleration[0] = new_accel[0];
      acceleration[1] = new_accel[1];
      return;
   }

   /*! @brief Get the acceleration of the Ball.
    *  @param accel Current Ball acceleration. */
   virtual void get_acceleration( double *accel )
   {
      accel[0] = acceleration[0];
      accel[1] = acceleration[1];
      return;
   }

   /*! @brief Set the Ball state force.
    *  @param new_force New Ball force. */
   virtual void set_force( char const *new_force )
   {
      external_force[0] = new_force[0];
      external_force[1] = new_force[1];
      return;
   }

   /*! @brief Get the force of the Ball.
    *  @param vel Current Ball force. */
   virtual void set_force( double *force )
   {
      force[0] = external_force[0];
      force[1] = external_force[1];
      return;
   }

   /*! @brief Get the current scenario time associated with the Ball.
    *  @return Current time associated with the Ball. */
   virtual double const get_time()
   {
      return time;
   }

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

   /*! @brief Packs the packing data object from the working data object(s),
    *  @details Called from the pack() function to pack the data from the working
    *  data objects(s) into the packing_data object.  */
   virtual void pack_from_working_data();

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  packing_data object into the working data object(s). */
   virtual void unpack_into_working_data();

   /*! @brief Set the reference to the physical entity data.
    *  @param physical_data_ptr Pointer to the PhysicalEntity data instance. */
   virtual void set_data( BallState *ball_data_ptr )
   {
      ball_state = ball_data_ptr;
      return;
   }

   /*! @brief Get the reference to the physical entity data.
    *  @return Pointer to the PhysicalEntity data. */
   virtual BallState *get_data()
   {
      return ( ball_state );
   }

  public:
   bool debug; ///< @trick_units{--} Debug output flag.

  protected:
   // Setup Object Attribute references. These are set in initialize_callback
   // routine and used for efficiency and ownership transfer in unpack routines.
   TrickHLA::Attribute *name_attr;         ///< @trick_io{**} Name Attribute.
   TrickHLA::Attribute *time_attr;         ///< @trick_io{**} Time Attribute.
   TrickHLA::Attribute *position_attr;     ///< @trick_io{**} Position Attribute.
   TrickHLA::Attribute *velocity_attr;     ///< @trick_io{**} Velocity Attribute.
   TrickHLA::Attribute *acceleration_attr; ///< @trick_io{**} Acceleration Attribute.
   TrickHLA::Attribute *force_attr;        ///< @trick_io{**} Force Attribute.

   // Assign to these parameters when setting up the data associations for the
   // TrickHLAObject data for the Ball.
   char  *name;              ///< @trick_units{--}   Name of ball.
   double time;              ///< @trick_units{s}    Time.
   double position[2];       ///< @trick_units{m}    X(horizontal), Y(vertical) position.
   double velocity[2];       ///< @trick_units{m/s}  X,Y velocity.
   double acceleration[2];   ///< @trick_units{m/s2} X,Y acceleration.
   double external_force[2]; ///< @trick_units{N}    Total external force on ball.

   /*! @brief Print out the packing data debug information.
    *  @param stream Output stream. */
   virtual void debug_print( std::ostream &stream = std::cout ) const;

  protected:
   BallState *ball_state; ///< @trick_units{--} Ball state data.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for BallPacking class.
    *  @details This constructor is private to prevent inadvertent copies. */
   BallPacking( BallPacking const &rhs );
   /*! @brief Assignment operator for BallPacking class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   BallPacking &operator=( BallPacking const &rhs );
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_BALL_PACKING_H: Do NOT put anything after this line!
