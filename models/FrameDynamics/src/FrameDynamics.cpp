/*!
@ingroup FrameDynamics
@file models/FrameDynamics/src/FrameDynamics.cpp
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
@trick_link_dependency{FrameDynamics.cpp}

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
#include "../include/FrameDynamics.hh"

/* GLOBAL Integrator. */
extern Trick::Integrator *trick_curr_integ;

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
FrameDynamics::FrameDynamics() // RETURN: -- None.
   : data()
{
   V_INIT( this->accel_env );
   V_INIT( this->ang_accel_env );
   Q_dot.initialize();
   return;
}

/*!
 * @job_class{shutdown}
 */
FrameDynamics::~FrameDynamics() // RETURN: -- None.
{
   return;
}

void FrameDynamics::default_data()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void FrameDynamics::initialize()
{

   // Use derivative evaluation to complete the state initialization.
   derivative();

   return;
}

/*!
 * @job_class{derivative}
 */
void FrameDynamics::derivative()
{

   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   Q_dot.derivative_first( data.state.att, data.state.ang_vel );

   //
   // Compute the translational dynamics.
   //
   data.accel[0] = this->accel_env[0];
   data.accel[1] = this->accel_env[1];
   data.accel[2] = this->accel_env[2];

   //
   // Compute the rotational dynamics.
   //
   data.ang_accel[0] = this->ang_accel_env[0];
   data.ang_accel[1] = this->ang_accel_env[1];
   data.ang_accel[2] = this->ang_accel_env[2];

   return;
}

/*!
 * @job_class{integration}
 */
int FrameDynamics::integrate()
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
   // Q_dot.first_derivative( data.state.att, data.state.ang_vel );

   // Return the Trick integrator integration step.
   return ( ipass );
}

/*!
 * @job_class{integration}
 */
void FrameDynamics::load()
{
   int istep = trick_curr_integ->intermediate_step;

   // Load state array: position and velocity.
   // Translational position
   trick_curr_integ->state[0] = data.state.pos[0];
   trick_curr_integ->state[1] = data.state.pos[1];
   trick_curr_integ->state[2] = data.state.pos[2];
   // Rotational position
   trick_curr_integ->state[3] = data.state.att.scalar;
   trick_curr_integ->state[4] = data.state.att.vector[0];
   trick_curr_integ->state[5] = data.state.att.vector[1];
   trick_curr_integ->state[6] = data.state.att.vector[2];
   // Translational velocity
   trick_curr_integ->state[7] = data.state.vel[0];
   trick_curr_integ->state[8] = data.state.vel[1];
   trick_curr_integ->state[9] = data.state.vel[2];
   // Rotational velocity
   trick_curr_integ->state[10] = data.state.ang_vel[0];
   trick_curr_integ->state[11] = data.state.ang_vel[1];
   trick_curr_integ->state[12] = data.state.ang_vel[2];

   // Load the integrator derivative references.
   // Translational position
   trick_curr_integ->deriv[istep][0] = data.state.vel[0];
   trick_curr_integ->deriv[istep][1] = data.state.vel[1];
   trick_curr_integ->deriv[istep][2] = data.state.vel[2];
   // Rotational position
   trick_curr_integ->deriv[istep][3] = Q_dot.scalar;
   trick_curr_integ->deriv[istep][4] = Q_dot.vector[0];
   trick_curr_integ->deriv[istep][5] = Q_dot.vector[1];
   trick_curr_integ->deriv[istep][6] = Q_dot.vector[2];
   // Translational velocity
   trick_curr_integ->deriv[istep][7] = data.accel[0];
   trick_curr_integ->deriv[istep][8] = data.accel[1];
   trick_curr_integ->deriv[istep][9] = data.accel[2];
   // Rotational velocity
   trick_curr_integ->deriv[istep][10] = data.ang_accel[0];
   trick_curr_integ->deriv[istep][11] = data.ang_accel[1];
   trick_curr_integ->deriv[istep][12] = data.ang_accel[2];

   // Return to calling routine.
   return;
}

/*!
 * @job_class{integration}
 */
void FrameDynamics::unload()
{
   int istep = trick_curr_integ->intermediate_step;

   // Translational position
   data.state.pos[0] = trick_curr_integ->state_ws[istep][0];
   data.state.pos[1] = trick_curr_integ->state_ws[istep][1];
   data.state.pos[2] = trick_curr_integ->state_ws[istep][2];
   // Rotational position
   data.state.att.scalar    = trick_curr_integ->state_ws[istep][3];
   data.state.att.vector[0] = trick_curr_integ->state_ws[istep][4];
   data.state.att.vector[1] = trick_curr_integ->state_ws[istep][5];
   data.state.att.vector[2] = trick_curr_integ->state_ws[istep][6];
   // Translational velocity
   data.state.vel[0] = trick_curr_integ->state_ws[istep][7];
   data.state.vel[1] = trick_curr_integ->state_ws[istep][8];
   data.state.vel[2] = trick_curr_integ->state_ws[istep][9];
   // Rotational velocity
   data.state.ang_vel[0] = trick_curr_integ->state_ws[istep][10];
   data.state.ang_vel[1] = trick_curr_integ->state_ws[istep][11];
   data.state.ang_vel[2] = trick_curr_integ->state_ws[istep][12];

   // Return to calling routine.
   return;
}
