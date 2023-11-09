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
@trick_link_dependency{../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{DynamicalEntityLagComp.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <string>
#include <float.h>

// Trick include files.
#include "trick/Integrator.hh"
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h" // for send_hs
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Attribute.hh"

// SpaceFOM include files.
#include "SpaceFOM/DynamicalEntityLagComp.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityLagComp::DynamicalEntityLagComp( DynamicalEntityBase & entity_ref ) // RETURN: -- None.
   : DynamicalEntityLagCompInteg( entity_ref )
{

   // Assign the integrator state references.
   // Translational position
   integ_states[0] = &(this->lag_comp_data.pos[0]);
   integ_states[1] = &(this->lag_comp_data.pos[1]);
   integ_states[2] = &(this->lag_comp_data.pos[2]);
   // Translational velocity
   integ_states[3] = &(this->lag_comp_data.vel[0]);
   integ_states[4] = &(this->lag_comp_data.vel[1]);
   integ_states[5] = &(this->lag_comp_data.vel[2]);
   // Rotational position
   integ_states[6] = &(this->lag_comp_data.att.scalar);
   integ_states[7] = &(this->lag_comp_data.att.vector[0]);
   integ_states[8] = &(this->lag_comp_data.att.vector[1]);
   integ_states[9] = &(this->lag_comp_data.att.vector[2]);
   // Rotational velocity
   integ_states[10] = &(this->lag_comp_data.ang_vel[0]);
   integ_states[11] = &(this->lag_comp_data.ang_vel[1]);
   integ_states[12] = &(this->lag_comp_data.ang_vel[2]);

}


/*!
 * @job_class{shutdown}
 */
DynamicalEntityLagComp::~DynamicalEntityLagComp() // RETURN: -- None.
{
   // Free up any allocated intergrator.
   if ( this->integrator != (Trick::Integrator *)NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->integrator ) ) ) {
         send_hs( stderr, "SpaceFOM::DynamicalEntityBase::~DynamicalEntityBase():%d ERROR deleting Trick Memory for 'this->integrator'%c",
                  __LINE__, THLA_NEWLINE );
      }
      this->integrator = NULL;
   }
}


/*!
 * @job_class{initialization}
 */
void DynamicalEntityLagComp::initialize()
{
   ostringstream errmsg;

   // Create and get a reference to the Trick Euler integrator.
   this->integrator = Trick::getIntegrator( Euler, 26, this->integ_dt);

   if ( this->integrator == (Trick::Integrator *)NULL ) {

      errmsg << "SpaceFOM::DynamicalEntityLagComp::initialize():" << __LINE__
             << " ERROR: Unexpected NULL Trick integrator!"<< THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );

   }

   // Call the base class initialize function.l
   DynamicalEntityLagCompInteg::initialize();

   // Return to calling routine.
   return;
}


/*!
 * @job_class(integration)
 */
void DynamicalEntityLagComp::update_time()
{
   this->lag_comp_data.time = this->integ_t;
   return;
}


/*!
 * @job_class(integration)
 */
void DynamicalEntityLagComp::load()
{
   int istep = integrator->intermediate_step;

   // Load state array: position and velocity.
   for( int iinc = 0; iinc < 13; iinc++ ){
      integrator->state[iinc] = *(integ_states[iinc]);
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
   this->integrator->deriv[istep][10] = this->rot_accel[0];
   this->integrator->deriv[istep][11] = this->rot_accel[1];
   this->integrator->deriv[istep][12] = this->rot_accel[2];

   // Return to calling routine.
   return;

}


/*!
 * @job_class{integration}
 */
void DynamicalEntityLagComp::unload()
{

   // Unload state array: position and velocity.
   for( int iinc = 0; iinc < 13; iinc++ ){
      *(integ_states[iinc]) = integrator->state[iinc];
   }

   // Normalize the propagated attitude quaternion.
   this->lag_comp_data.att.normalize();

   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   this->Q_dot.derivative_first( this->lag_comp_data.att,
                                 this->lag_comp_data.ang_vel );

   // Return to calling routine.
   return;

}


/*! @job_class{derivative}
 *  @brief Compute the first time derivative of the lag compensation state vector.
 *  @param user_data Any special user data needed to compute the derivative values. */
void DynamicalEntityLagComp::derivative_first(
   void * user_data )
{
   double accel_str[3];
   double rot_accel_str[3];
   double I_omega[3];
   double omega_X_I_omega[3];

   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   this->Q_dot.derivative_first( this->lag_comp_data.att,
                                 this->lag_comp_data.ang_vel );

   // Compute the translational acceleration in the structural frame.
   V_SCALE( accel_str, this->force, 1.0/this->mass );

   // Transform the translational acceleration into the body frame.
   this->body_wrt_struct.transform_vector( accel_str, this->accel );

   // Compute the rotational acceleration in the structural frame.
   // External torque acceleration.
   MxV( rot_accel_str, this->inertia_inv, this->torque );
   // Internal rotational accelerations.
   MxV( I_omega, this->inertia, this->lag_comp_data.ang_vel );
   V_CROSS( omega_X_I_omega, this->lag_comp_data.ang_vel, I_omega );
   rot_accel_str[0] += omega_X_I_omega[0];
   rot_accel_str[1] += omega_X_I_omega[1];
   rot_accel_str[2] += omega_X_I_omega[2];

   // Transform the rotational acceleration into the body frame.
   this->body_wrt_struct.transform_vector( rot_accel_str, this->rot_accel );

   return;
}
