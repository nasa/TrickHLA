/*!
@file models/EntityDynamics/src/EntityDynamics.cpp
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
@trick_link_dependency{EntityDynamics.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, October 2023, --, SpaceFOM support and testing. }
@revs_end

*/

// System includes.
#include <iostream>
#include <sstream>

// Trick includes.
#include "trick/trick_math.h"
#include "trick/Integrator.hh"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Types.hh"

// Model includes.
#include "../include/EntityDynamics.hh"

/* GLOBAL Integrator. */
extern Trick::Integrator* trick_curr_integ ;

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

void EntityDynamics::default_data()
{
   return;
}


void EntityDynamics::initialize()
{
   ostringstream errmsg;

   // Compute the inverse of the inertia matrix.
   if ( dm_invert_symm( I_inv, de_data.inertia ) != TM_SUCCESS ) {
      errmsg << "SpaceFOM::PhysicalEntityBase::set_object():" << __LINE__
             << " ERROR: The initialize() function has already been called" << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   return;
}


void EntityDynamics::derivative()
{
   double accel_str[3];
   double rot_accel_str[3];
   double I_omega[3];
   double omega_X_I_omega[3];

   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   this->Q_dot.derivative_first( pe_data.state.att, pe_data.state.ang_vel );
   std::cout << pe_data.name << ".Q_dot: " << Q_dot.scalar << "; "
         << Q_dot.vector[0] << ", " << Q_dot.vector[1] << ", " << Q_dot.vector[2] << std::endl;

   // Compute the translational acceleration in the structural frame.
   V_SCALE( accel_str, de_data.force, 1.0/de_data.mass );

   // Transform the translational acceleration into the body frame.
   pe_data.body_wrt_struct.transform_vector( accel_str, pe_data.accel );

   // Compute the rotational acceleration in the structural frame.
   // External torque acceleration.
   MxV( rot_accel_str, I_inv, de_data.torque );
   // Internal rotational accelerations.
   MxV( I_omega, de_data.inertia, pe_data.state.ang_vel );
   V_CROSS( omega_X_I_omega, pe_data.state.ang_vel, I_omega );
   rot_accel_str[0] += omega_X_I_omega[0];
   rot_accel_str[1] += omega_X_I_omega[1];
   rot_accel_str[2] += omega_X_I_omega[2];

   // Transform the rotational acceleration into the body frame.
   pe_data.body_wrt_struct.transform_vector( rot_accel_str, pe_data.rot_accel );

   return;
}


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
   //this->Q_dot.first_derivative( pe_data.state.att, pe_data.state.ang_vel );

   // Return the Trick integrator integration step.
   return( ipass );
}


/*!
 * @job_class(integration)
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
   trick_curr_integ->deriv[istep][10] = pe_data.rot_accel[0];
   trick_curr_integ->deriv[istep][11] = pe_data.rot_accel[1];
   trick_curr_integ->deriv[istep][12] = pe_data.rot_accel[2];

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
   pe_data.state.att.scalar = trick_curr_integ->state_ws[istep][3];
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
