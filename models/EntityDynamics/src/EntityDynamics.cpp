/*!
@ingroup EntityDynamics
@file models/EntityDynamics/src/EntityDynamics.cpp
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
@trick_link_dependency{EntityDynamics.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, October 2023, --, SpaceFOM support and testing. }
@revs_end

*/

// System includes.
#include <iostream>
#include <sstream>

// Trick includes.
#include "trick/Integrator.hh"
#include "trick/trick_math.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"

// Model includes.
#include "../include/EntityDynamics.hh"

/* GLOBAL Integrator. */
extern Trick::Integrator *trick_curr_integ;

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
EntityDynamics::EntityDynamics() // RETURN: -- None.
   : pe_data(),
     de_data()
{
   V_INIT( this->accel_env );
   V_INIT( this->ang_accel_env );
   V_INIT( this->ang_accel_inertial );
   M_INIT( this->I_inv );
   Q_dot.initialize();
   return;
}

/*!
 * @job_class{shutdown}
 */
EntityDynamics::~EntityDynamics() // RETURN: -- None.
{
   return;
}

void EntityDynamics::default_data()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void EntityDynamics::initialize()
{
   // Compute the inverse of the inertia matrix.
   if ( dm_invert_symm( I_inv, de_data.inertia ) != TM_SUCCESS ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalEntityBase::set_object():" << __LINE__
             << " ERROR: The initialize() function has already been called\n";
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   return;
}

/*!
 * @job_class{derivative}
 */
void EntityDynamics::derivative()
{
   double force_bdy[3];
   double torque_bdy[3];
   double accel_force_bdy[3];
   double ang_accel_torque_bdy[3];
   double I_omega[3];

   /*************************************************************************
    * NOTE: While the force and torque values are expressed in the structural
    * reference frame, we are also assuming that the force and torque are
    * summed and computed to be applied at the entity center of mass (CM).
    * This is important in the fact that the sum of the torques generated by
    * individual forces applied away from the CM are not the same as a torque
    * generated from the SUM of the forces applied at the origin of the
    * structural reference frame.
    **************************************************************************/

   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   Q_dot.derivative_first( pe_data.state.att, pe_data.state.ang_vel );

   //
   // Compute the translational dynamics.
   //
   // Transform the force into the body frame.
   pe_data.body_wrt_struct.transform_vector( de_data.force, force_bdy );

   // Compute the force contribution to the translational acceleration.
   V_SCALE( accel_force_bdy, force_bdy, 1.0 / de_data.mass );

   // Compute the total acceleration acceleration.
   V_ADD( pe_data.accel, this->accel_env, accel_force_bdy );

   //
   // Compute the rotational dynamics.
   //
   // Transform the torque into the body frame.
   pe_data.body_wrt_struct.transform_vector( de_data.torque, torque_bdy );

   // External torque acceleration.
   MxV( ang_accel_torque_bdy, this->I_inv, torque_bdy );

   // Inertial rotational accelerations (omega X I omega).
   MxV( I_omega, de_data.inertia, pe_data.state.ang_vel );
   V_CROSS( this->ang_accel_inertial, pe_data.state.ang_vel, I_omega );

   // Compute the total angular acceleration.
   pe_data.ang_accel[0] = this->ang_accel_env[0] + ang_accel_torque_bdy[0] + this->ang_accel_inertial[0];
   pe_data.ang_accel[1] = this->ang_accel_env[1] + ang_accel_torque_bdy[1] + this->ang_accel_inertial[1];
   pe_data.ang_accel[2] = this->ang_accel_env[2] + ang_accel_torque_bdy[2] + this->ang_accel_inertial[2];

   return;
}

/*!
 * @job_class{integration}
 */
int EntityDynamics::integrate()
{
   int ipass;

   // Load the states and derivatives into the integrator.
   load();

   // Call the Trick integration routine.
   ipass = trick_curr_integ->integrate();

   // Unload the states from the integrator.
   unload();

   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   // Q_dot.first_derivative( pe_data.state.att, pe_data.state.ang_vel );

   // Return the Trick integrator integration step.
   return ( ipass );
}

/*!
 * @job_class{integration}
 */
void EntityDynamics::load()
{
   int istep = trick_curr_integ->intermediate_step;

   // Load state array: position and velocity.
   // Translational position
   trick_curr_integ->state[0] = pe_data.state.pos[0];
   trick_curr_integ->state[1] = pe_data.state.pos[1];
   trick_curr_integ->state[2] = pe_data.state.pos[2];
   // Rotational position
   trick_curr_integ->state[3] = pe_data.state.att.scalar;
   trick_curr_integ->state[4] = pe_data.state.att.vector[0];
   trick_curr_integ->state[5] = pe_data.state.att.vector[1];
   trick_curr_integ->state[6] = pe_data.state.att.vector[2];
   // Translational velocity
   trick_curr_integ->state[7] = pe_data.state.vel[0];
   trick_curr_integ->state[8] = pe_data.state.vel[1];
   trick_curr_integ->state[9] = pe_data.state.vel[2];
   // Rotational velocity
   trick_curr_integ->state[10] = pe_data.state.ang_vel[0];
   trick_curr_integ->state[11] = pe_data.state.ang_vel[1];
   trick_curr_integ->state[12] = pe_data.state.ang_vel[2];

   // Load the integrator derivative references.
   // Translational position
   trick_curr_integ->deriv[istep][0] = pe_data.state.vel[0];
   trick_curr_integ->deriv[istep][1] = pe_data.state.vel[1];
   trick_curr_integ->deriv[istep][2] = pe_data.state.vel[2];
   // Rotational position
   trick_curr_integ->deriv[istep][3] = Q_dot.scalar;
   trick_curr_integ->deriv[istep][4] = Q_dot.vector[0];
   trick_curr_integ->deriv[istep][5] = Q_dot.vector[1];
   trick_curr_integ->deriv[istep][6] = Q_dot.vector[2];
   // Translational velocity
   trick_curr_integ->deriv[istep][7] = pe_data.accel[0];
   trick_curr_integ->deriv[istep][8] = pe_data.accel[1];
   trick_curr_integ->deriv[istep][9] = pe_data.accel[2];
   // Rotational velocity
   trick_curr_integ->deriv[istep][10] = pe_data.ang_accel[0];
   trick_curr_integ->deriv[istep][11] = pe_data.ang_accel[1];
   trick_curr_integ->deriv[istep][12] = pe_data.ang_accel[2];

   // Return to calling routine.
   return;
}

/*!
 * @job_class{integration}
 */
void EntityDynamics::unload()
{
   int istep = trick_curr_integ->intermediate_step;

   // Translational position
   pe_data.state.pos[0] = trick_curr_integ->state_ws[istep][0];
   pe_data.state.pos[1] = trick_curr_integ->state_ws[istep][1];
   pe_data.state.pos[2] = trick_curr_integ->state_ws[istep][2];
   // Rotational position
   pe_data.state.att.scalar    = trick_curr_integ->state_ws[istep][3];
   pe_data.state.att.vector[0] = trick_curr_integ->state_ws[istep][4];
   pe_data.state.att.vector[1] = trick_curr_integ->state_ws[istep][5];
   pe_data.state.att.vector[2] = trick_curr_integ->state_ws[istep][6];
   // Translational velocity
   pe_data.state.vel[0] = trick_curr_integ->state_ws[istep][7];
   pe_data.state.vel[1] = trick_curr_integ->state_ws[istep][8];
   pe_data.state.vel[2] = trick_curr_integ->state_ws[istep][9];
   // Rotational velocity
   pe_data.state.ang_vel[0] = trick_curr_integ->state_ws[istep][10];
   pe_data.state.ang_vel[1] = trick_curr_integ->state_ws[istep][11];
   pe_data.state.ang_vel[2] = trick_curr_integ->state_ws[istep][12];

   // Return to calling routine.
   return;
}
