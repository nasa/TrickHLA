/*!
@file SpaceFOM/PhysicalEntityLagCompSA2.cpp
@ingroup SpaceFOM
@brief This class provides the implementation for a TrickHLA SpaceFOM
PhysicalEntity latency/lag compensation class.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../source/TrickHLA/LagCompensation.cpp}
@trick_link_dependency{PhysicalEntityLagCompSA2.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <string>
#include <float.h>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h" // for send_hs
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Attribute.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityLagCompSA2.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityLagCompSA2::PhysicalEntityLagCompSA2( PhysicalEntityBase & entity_ref ) // RETURN: -- None.
   : PhysicalEntityLagCompInteg( entity_ref ),
     integrator( this->integ_dt, 7, this->integ_states, this->integ_derivs, this->derivatives, this )
{

   // Assign the integrator state references.
   // Translational position
   integ_states[0] = &(this->lag_comp_data.pos[0]);
   integ_states[1] = &(this->lag_comp_data.pos[1]);
   integ_states[2] = &(this->lag_comp_data.pos[2]);
   // Rotational position
   integ_states[3] = &(this->lag_comp_data.att.scalar);
   integ_states[4] = &(this->lag_comp_data.att.vector[0]);
   integ_states[5] = &(this->lag_comp_data.att.vector[1]);
   integ_states[6] = &(this->lag_comp_data.att.vector[2]);

   // Translational velocity
   integ_derivs[0] = &(this->lag_comp_data.vel[0]);
   integ_derivs[1] = &(this->lag_comp_data.vel[1]);
   integ_derivs[2] = &(this->lag_comp_data.vel[2]);
   // Rotational velocity
   integ_derivs[3] = &(this->Q_dot.scalar);
   integ_derivs[4] = &(this->Q_dot.vector[0]);
   integ_derivs[5] = &(this->Q_dot.vector[1]);
   integ_derivs[6] = &(this->Q_dot.vector[2]);

}


/*!
 * @job_class{shutdown}
 */
PhysicalEntityLagCompSA2::~PhysicalEntityLagCompSA2() // RETURN: -- None.
{

}


/*!
 * @job_class{initialization}
 */
void PhysicalEntityLagCompSA2::initialize()
{

   // Call the base class initialize function.l
   PhysicalEntityLagCompInteg::initialize();

   // Return to calling routine.
   return;
}


/*!
 * @job_class{derivative}
 */
void PhysicalEntityLagCompSA2::derivatives(
   double       t,
   double const pos[],
   double const vel[],
   double       accel[],
   void       * udata)
{
   double omega[3];
   double quat_scalar = pos[3];
   double quat_vector[3];
   double qdot_scalar = vel[3];
   double qdot_vector[3];

   // Cast the user data to a PhysicalEntityLagCompSA2 instance.
   PhysicalEntityLagCompSA2 * lag_comp_data_ptr = static_cast<PhysicalEntityLagCompSA2 *>(udata);

   //
   // Set the translational acceleration.
   //
   accel[0] = lag_comp_data_ptr->accel[0];
   accel[1] = lag_comp_data_ptr->accel[1];
   accel[2] = lag_comp_data_ptr->accel[2];

   //
   // Compute the rotational acceleration.
   //
   quat_vector[0] = pos[4];
   quat_vector[1] = pos[5];
   quat_vector[2] = pos[6];
   qdot_vector[0] = vel[4];
   qdot_vector[1] = vel[5];
   qdot_vector[2] = vel[6];

   // Compute the angular velocity vector.
   QuaternionData::compute_omega( qdot_scalar,
                  qdot_vector,
                  quat_scalar,
                  quat_vector,
                  omega );

   // Compute the second derivative of the attitude quaternion.
   QuaternionData::compute_quat_dotdot( quat_scalar,
                        quat_vector,
                        omega,
                        lag_comp_data_ptr->rot_accel,
                        &(accel[3]),
                        &(accel[4]) );

#ifdef DEBUG
   cout << "Q_dotdot: " << endl;
   cout << "\tScalar: " << accel[3] << endl;
   cout << "\tVector: "
        << "\t\t" << accel[4] << ", "
        << "\t\t" << accel[5] << ", "
        << "\t\t" << accel[6] << endl;
#endif

   // Return to calling routine.
   return;
}


/*!
 * @job_class{derivative}
 */
int PhysicalEntityLagCompSA2::compensate(
   const double t_begin,
   const double t_end   )
{
   double dt_go  = t_end - t_begin;

   // Propagate the current PhysicalEntity state to the desired time.
   // Set the current integration time for the integrator.
   this->integ_t = t_begin;
   this->integrator.setIndyVar( this->integ_t );
   // Compute and save the size of this compensation step.
   this->compensate_dt = dt_go;

   // Loop through integrating the state forward to the current scenario time.
   while( (dt_go >= 0.0) && (fabs(dt_go) > this->integ_tol) ) {

      // Print out debug information if requested.
      if ( debug ) {
         cout << "Integ dt, tol, t, dt_go: " << this->integ_dt << ", "
              << this->integ_tol << ", " << integ_t << ", " << dt_go << endl;
      }

      // Load the integration states and derivatives.
      this->integrator.load();

      // Perform the integration propagation one integration step.
      if ( dt_go > this->integ_dt ){
         // Not near the end; so, use the defined integration step size.
         this->integrator.step(this->integ_dt);
      }
      else {
         // Near the end; so, integrate to the end of the compensation step.
         this->integrator.step(dt_go);
      }

      // Unload the integrated states and derivatives.
      this->integrator.unload();

      // Normalize the propagated attitude quaternion.
      QuaternionData::normalize_quaternion( &(this->lag_comp_data.att.scalar),
                            this->lag_comp_data.att.vector     );

      // Update the integration time.
      this->integ_t = this->integrator.getIndyVar();

      // Compute the remaining time in the compensation step.
      dt_go = t_end - this->integ_t;

   }

   // Update the lag compensated time,
   lag_comp_data.time = integ_t;

   // Compute the angular velocity vector from the propagated attitude
   // quaternion and its derivative.
   QuaternionData::compute_omega( this->Q_dot.scalar,
                  this->Q_dot.vector,
                  this->lag_comp_data.att.scalar,
                  this->lag_comp_data.att.vector,
                  this->lag_comp_data.ang_vel );

   // Print out debug information if desired.
   if ( debug ) {
      cout << "\tOmega: "
           << "\t\t" << this->lag_comp_data.ang_vel[0] << ", "
           << "\t\t" << this->lag_comp_data.ang_vel[1] << ", "
           << "\t\t" << this->lag_comp_data.ang_vel[2] << endl;
   }

   return( 0 );
}



