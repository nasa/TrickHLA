/*!
@ingroup SAIntegrator
@file models/SAIntegrator/src/PhysicalEntityLagCompSA2.cpp
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
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{PhysicalEntityLagCompSA2.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, September 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <float.h>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "../include/PhysicalEntityLagCompSA2.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityLagCompSA2::PhysicalEntityLagCompSA2( PhysicalEntityBase &entity_ref ) // RETURN: -- None.
   : PhysicalEntityLagCompBase( entity_ref ),
     integrator( this->integ_dt, 7, this->integ_states, this->integ_derivs, this->derivatives, this )
{

   // Assign the integrator state references.
   // Translational position
   integ_states[0] = &( this->lag_comp_data.pos[0] );
   integ_states[1] = &( this->lag_comp_data.pos[1] );
   integ_states[2] = &( this->lag_comp_data.pos[2] );
   // Rotational position
   integ_states[3] = &( this->lag_comp_data.att.scalar );
   integ_states[4] = &( this->lag_comp_data.att.vector[0] );
   integ_states[5] = &( this->lag_comp_data.att.vector[1] );
   integ_states[6] = &( this->lag_comp_data.att.vector[2] );

   // Translational velocity
   integ_derivs[0] = &( this->lag_comp_data.vel[0] );
   integ_derivs[1] = &( this->lag_comp_data.vel[1] );
   integ_derivs[2] = &( this->lag_comp_data.vel[2] );
   // Rotational velocity
   integ_derivs[3] = &( this->Q_dot.scalar );
   integ_derivs[4] = &( this->Q_dot.vector[0] );
   integ_derivs[5] = &( this->Q_dot.vector[1] );
   integ_derivs[6] = &( this->Q_dot.vector[2] );
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
   PhysicalEntityLagCompBase::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{derivative}
 */
void PhysicalEntityLagCompSA2::derivatives(
   double t,
   double pos[], // cppcheck-suppress [constParameter]
   double vel[], // cppcheck-suppress [constParameter]
   double accel[],
   void  *udata )
{
   double omega[3];
   double quat_scalar;
   double quat_vector[3];
   double qdot_scalar;
   double qdot_vector[3];

   // Cast the user data to a PhysicalEntityLagCompSA2 instance.
   PhysicalEntityLagCompSA2 *lag_comp_data_ptr = static_cast< PhysicalEntityLagCompSA2 * >( udata );

   //
   // Set the translational acceleration.
   //
   accel[0] = lag_comp_data_ptr->accel[0];
   accel[1] = lag_comp_data_ptr->accel[1];
   accel[2] = lag_comp_data_ptr->accel[2];

   //
   // Compute the rotational acceleration.
   //
   quat_scalar    = pos[3];
   quat_vector[0] = pos[4];
   quat_vector[1] = pos[5];
   quat_vector[2] = pos[6];

   qdot_scalar    = vel[3];
   qdot_vector[0] = vel[4];
   qdot_vector[1] = vel[5];
   qdot_vector[2] = vel[6];

   // Compute the angular velocity vector.
   QuaternionData::compute_omega( quat_scalar,
                                  quat_vector,
                                  qdot_scalar,
                                  qdot_vector,
                                  omega );

   // Compute the second derivative of the attitude quaternion.
   QuaternionData::compute_2nd_derivative( quat_scalar,
                                           quat_vector,
                                           omega,
                                           lag_comp_data_ptr->ang_accel,
                                           &( accel[3] ),
                                           &( accel[4] ) );

   // Return to calling routine.
   return;
}

/*!
 * @job_class{integration}
 */
void PhysicalEntityLagCompSA2::load()
{

   // Compute the derivatives of the lag compensation state vector.
   // Note: The SAIntegrator does not require a pre-integration derivative
   // evaluation.  The integrator calls the derivative() routine.
   // derivative_first();

   // Load the integration states and derivatives.
   integrator.load();
   return;
}

/*!
 * @job_class{integration}
 */
void PhysicalEntityLagCompSA2::unload()
{

   // Unload the integrated states and derivatives.
   integrator.unload();

   // Normalize the propagated attitude quaternion.
   lag_comp_data.att.normalize();

   // Compute the angular velocity vector.
   Q_dot.compute_omega( this->lag_comp_data.att, this->lag_comp_data.ang_vel );

   // Return to calling routine.
   return;
}

/*!
 * @job_class{derivative}
 */
int PhysicalEntityLagCompSA2::integrate(
   double const t_begin,
   double const t_end )
{
   double compensate_dt = t_end - t_begin;
   double dt_go         = compensate_dt;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream msg;
      msg << "**** PhysicalEntityLagCompSA2::integrate(): "
          << "Compensate: t_begin, t_end, dt_go: "
          << t_begin << ", " << t_end << ", " << dt_go << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Propagate the current PhysicalEntity state to the desired time.
   // Set the current integration time for the integrator.
   this->integ_t = t_begin;
   integrator.setIndyVar( 0.0 );

   // Loop through integrating the state forward to the current scenario time.
   while ( ( dt_go >= 0.0 ) && ( fabs( dt_go ) > this->integ_tol ) ) {

      // Use the inherited debug-handler to allow debug comments to be turned
      // on and off from a setting in the input file.
      if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
         ostringstream msg;
         msg << "****** PhysicalEntityLagCompSA2::integrate(): "
             << "Integ dt, tol, t, dt_go: "
             << this->integ_dt << ", " << this->integ_tol << ", "
             << integ_t << ", " << dt_go << '\n';
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      // Load the integration states and derivatives.
      load();

      // Perform the integration propagation one integration step.
      if ( dt_go > this->integ_dt ) {
         // Not near the end; so, use the defined integration step size.
         integrator.step( this->integ_dt );
      } else {
         // Near the end; so, integrate to the end of the compensation step.
         integrator.step( dt_go );
      }

      // Unload the integrated states and derivatives.
      unload();

      // Update the integration time.
      this->integ_t = t_begin + integrator.getIndyVar();

      // Compute the remaining time in the compensation step.
      dt_go = compensate_dt - integrator.getIndyVar();
   }

   // Update the lag compensated time,
   update_time();

   // Compute the angular velocity vector from the propagated attitude
   // quaternion and its derivative.
   Q_dot.compute_omega( this->lag_comp_data.att, this->lag_comp_data.ang_vel );

   // Print out debug information if desired.
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream msg;
      msg << "\tOmega: "
          << "\t\t" << this->lag_comp_data.ang_vel[0] << ", "
          << "\t\t" << this->lag_comp_data.ang_vel[1] << ", "
          << "\t\t" << this->lag_comp_data.ang_vel[2] << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return ( 0 );
}

/*! @job_class{derivative} */
void PhysicalEntityLagCompSA2::derivative_first(
   void *user_data )
{
   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   Q_dot.derivative_first( this->lag_comp_data.att, this->lag_comp_data.ang_vel );

   return;
}
