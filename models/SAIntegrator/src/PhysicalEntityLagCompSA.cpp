/*!
@ingroup SAIntegrator
@file models/SAIntegrator/src/PhysicalEntityLagCompSA.cpp
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
@trick_link_dependency{PhysicalEntityLagCompSA.cpp}


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
#include "../include/PhysicalEntityLagCompSA.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityLagCompSA::PhysicalEntityLagCompSA( PhysicalEntityBase &entity_ref ) // RETURN: -- None.
   : PhysicalEntityLagCompBase( entity_ref ),
     integrator( this->integ_dt, 13, this->integ_states, this->integ_states, this->derivatives, this )
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
   integ_states[7] = &( this->lag_comp_data.vel[0] );
   integ_states[8] = &( this->lag_comp_data.vel[1] );
   integ_states[9] = &( this->lag_comp_data.vel[2] );
   // Rotational velocity
   integ_states[10] = &( this->lag_comp_data.ang_vel[0] );
   integ_states[11] = &( this->lag_comp_data.ang_vel[1] );
   integ_states[12] = &( this->lag_comp_data.ang_vel[2] );
}

/*!
 * @job_class{shutdown}
 */
PhysicalEntityLagCompSA::~PhysicalEntityLagCompSA() // RETURN: -- None.
{
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityLagCompSA::initialize()
{

   // Call the base class initialize function.l
   PhysicalEntityLagCompBase::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @job_class{derivative}
 */
void PhysicalEntityLagCompSA::derivatives(
   double t,
   double states[],
   double derivs[],
   void  *udata )
{
   QuaternionData qdot;

   // Cast the user data to a PhysicalEntityLagCompSA2 instance.
   PhysicalEntityLagCompSA *lag_comp_data_ptr = static_cast< PhysicalEntityLagCompSA * >( udata );

   // Copy the time derivative of the attitude quaternion.
   qdot.derivative_first( states[3], &( states[4] ), &( states[10] ) );

   // Compute the derivatives based on time, state, and user data.
   // Translational velocity.
   derivs[0] = states[7];
   derivs[1] = states[8];
   derivs[2] = states[9];

   // Rotational velocity in quaternion form.
   derivs[3] = qdot.scalar;
   derivs[4] = qdot.vector[0];
   derivs[5] = qdot.vector[1];
   derivs[6] = qdot.vector[2];

   // Translational acceleration.
   derivs[7] = lag_comp_data_ptr->accel[0];
   derivs[8] = lag_comp_data_ptr->accel[1];
   derivs[9] = lag_comp_data_ptr->accel[2];

   // Rotational acceleration.
   derivs[10] = lag_comp_data_ptr->ang_accel[0];
   derivs[11] = lag_comp_data_ptr->ang_accel[1];
   derivs[12] = lag_comp_data_ptr->ang_accel[2];

   // Return to calling routine.
   return;
}

/*!
 * @job_class{integration}
 */
void PhysicalEntityLagCompSA::load()
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
void PhysicalEntityLagCompSA::unload()
{

   // Unload the integrated states and derivatives.
   integrator.unload();

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
int PhysicalEntityLagCompSA::integrate(
   double const t_begin,
   double const t_end )
{
   double compensate_dt = t_end - t_begin;
   double dt_go         = compensate_dt;

   // Use the inherited debug-handler to allow debug comments to be turned
   // on and off from a setting in the input file.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_LAG_COMPENSATION ) ) {
      ostringstream msg;
      msg << "**** PhysicalEntityLagCompSA::integrate(): "
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
         msg << "****** PhysicalEntityLagCompSA::integrate(): "
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
         integrator.variable_step( this->integ_dt );
      } else {
         // Near the end; so, integrate to the end of the compensation step.
         integrator.variable_step( dt_go );
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

   // Compute the lag compensated value for the attitude quaternion rate.
   derivative_first();

   return ( 0 );
}

/*! @job_class{derivative} */
void PhysicalEntityLagCompSA::derivative_first(
   void *user_data )
{
   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   Q_dot.derivative_first( this->lag_comp_data.att, this->lag_comp_data.ang_vel );

   return;
}
