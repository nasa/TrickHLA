/*!
@file SpaceFOM/DynamicalEntityLagComp.cpp
@ingroup SpaceFOM
@brief This class provides the implementation for a TrickHLA SpaceFOM
DynamicalEntity latency/lag compensation class.

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
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{DynamicalEntityLagComp.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <float.h>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/Integrator.hh"
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityLagComp.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityLagComp::DynamicalEntityLagComp( DynamicalEntityBase &entity_ref ) // RETURN: -- None.
   : DynamicalEntityLagCompInteg( entity_ref )
{
   // Assign the integrator state references.
   // Translational position
   integ_states[0] = &( this->lag_comp_data.pos[0] );
   integ_states[1] = &( this->lag_comp_data.pos[1] );
   integ_states[2] = &( this->lag_comp_data.pos[2] );
   // Translational velocity
   integ_states[3] = &( this->lag_comp_data.vel[0] );
   integ_states[4] = &( this->lag_comp_data.vel[1] );
   integ_states[5] = &( this->lag_comp_data.vel[2] );
   // Rotational position
   integ_states[6] = &( this->lag_comp_data.att.scalar );
   integ_states[7] = &( this->lag_comp_data.att.vector[0] );
   integ_states[8] = &( this->lag_comp_data.att.vector[1] );
   integ_states[9] = &( this->lag_comp_data.att.vector[2] );
   // Rotational velocity
   integ_states[10] = &( this->lag_comp_data.ang_vel[0] );
   integ_states[11] = &( this->lag_comp_data.ang_vel[1] );
   integ_states[12] = &( this->lag_comp_data.ang_vel[2] );
}

/*!
 * @job_class{shutdown}
 */
DynamicalEntityLagComp::~DynamicalEntityLagComp() // RETURN: -- None.
{
   // Free up any allocated intergrator.
   if ( this->integrator != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->integrator ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::DynamicalEntityBase::~DynamicalEntityBase():%d WARNING failed to delete Trick Memory for 'this->integrator'\n",
                          __LINE__ );
      }
      this->integrator = NULL;
   }
}

/*!
 * @job_class{initialization}
 */
void DynamicalEntityLagComp::initialize()
{
   // Create and get a reference to the Trick Euler integrator.
   this->integrator = Trick::getIntegrator( Euler, 26, this->integ_dt );

   if ( this->integrator == NULL ) {
      ostringstream errmsg;

      errmsg << "SpaceFOM::DynamicalEntityLagComp::initialize():" << __LINE__
             << " ERROR: Unexpected NULL Trick integrator!\n";
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Call the base class initialize function.l
   DynamicalEntityLagCompInteg::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{integration}
 */
void DynamicalEntityLagComp::update_time()
{
   this->lag_comp_data.time = this->integ_t;
   return;
}

/*!
 * @job_class{integration}
 */
void DynamicalEntityLagComp::load()
{
   int istep = integrator->intermediate_step;

   // Load state array: position and velocity.
   for ( int iinc = 0; iinc < 13; ++iinc ) {
      integrator->state[iinc] = *( integ_states[iinc] );
   }

   /*************************************************************************
    * Note: We are not accounting for the time rate of change of the mass
    * properties in these equations.  We could add in the equations for both
    * mass and inertia but, with the exception of launch and ascent cases,
    * they will probably contribute little to the dynamics.  If these are
    * needed, the state vector will have to be expanded by at least 7 for
    * the mass and the symmetric inertia terms, total of 20 (13+7).
    *************************************************************************/

   // Load the integrator derivative references.
   // Translational position
   this->integrator->deriv[istep][0] = this->integrator->state[3];
   this->integrator->deriv[istep][1] = this->integrator->state[4];
   this->integrator->deriv[istep][2] = this->integrator->state[5];
   // Translational velocity
   this->integrator->deriv[istep][3] = this->accel[0];
   this->integrator->deriv[istep][4] = this->accel[1];
   this->integrator->deriv[istep][5] = this->accel[2];
   // Rotational position
   this->integrator->deriv[istep][6] = this->Q_dot.scalar;
   this->integrator->deriv[istep][7] = this->Q_dot.vector[0];
   this->integrator->deriv[istep][8] = this->Q_dot.vector[1];
   this->integrator->deriv[istep][9] = this->Q_dot.vector[2];
   // Rotational velocity
   this->integrator->deriv[istep][10] = this->ang_accel[0];
   this->integrator->deriv[istep][11] = this->ang_accel[1];
   this->integrator->deriv[istep][12] = this->ang_accel[2];

   // Return to calling routine.
   return;
}

/*!
 * @job_class{integration}
 */
void DynamicalEntityLagComp::unload()
{
   // Unload state array: position and velocity.
   for ( int iinc = 0; iinc < 13; ++iinc ) {
      *( integ_states[iinc] ) = integrator->state[iinc];
   }

   // Normalize the propagated attitude quaternion.
   lag_comp_data.att.normalize();

   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   Q_dot.derivative_first( this->lag_comp_data.att, this->lag_comp_data.ang_vel );

   // Return to calling routine.
   return;
}

/*!
 * @job_class{derivative}
 */
void DynamicalEntityLagComp::derivative_first(
   void *user_data )
{
   double force_bdy[3];
   double torque_bdy[3];
   double accel_force_bdy[3];
   double ang_accel_torque_bdy[3];
   double I_omega[3];
   double omega_X_I_omega[3];

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
   Q_dot.derivative_first( this->lag_comp_data.att, this->lag_comp_data.ang_vel );

   //
   // Compute the translational dynamics.
   //
   // Transform the force into the body frame.
   body_wrt_struct.transform_vector( this->force, force_bdy );

   // Compute the force contribution to the translational acceleration.
   V_SCALE( accel_force_bdy, force_bdy, 1.0 / this->mass );

   // Compute the total acceleration acceleration.
   V_ADD( this->accel, this->accel_env, accel_force_bdy );

   //
   // Compute the rotational dynamics.
   //
   // Transform the torque into the body frame.
   body_wrt_struct.transform_vector( this->torque, torque_bdy );

   // External torque acceleration.
   MxV( ang_accel_torque_bdy, this->inertia_inv, torque_bdy );

   // Inertial rotational accelerations (omega X I omega).
   MxV( I_omega, this->inertia, this->lag_comp_data.ang_vel );
   V_CROSS( omega_X_I_omega, this->lag_comp_data.ang_vel, I_omega );

   // Compute the total angular acceleration.
   this->ang_accel[0] = this->ang_accel_env[0] + ang_accel_torque_bdy[0] + omega_X_I_omega[0];
   this->ang_accel[1] = this->ang_accel_env[1] + ang_accel_torque_bdy[1] + omega_X_I_omega[1];
   this->ang_accel[2] = this->ang_accel_env[2] + ang_accel_torque_bdy[2] + omega_X_I_omega[2];

   return;
}
