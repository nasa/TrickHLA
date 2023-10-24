/*!
@file SpaceFOM/PhysicalEntityLagComp.cpp
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
@trick_link_dependency{PhysicalEntityLagComp.cpp}


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
#include "SpaceFOM/PhysicalEntityLagComp.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityLagComp::PhysicalEntityLagComp( PhysicalEntityBase & entity_ref ) // RETURN: -- None.
   : PhysicalEntityLagCompInteg( entity_ref ),
     integrator(NULL)
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
   integ_states[6] = &(this->lag_comp_data.quat.scalar);
   integ_states[7] = &(this->lag_comp_data.quat.vector[0]);
   integ_states[8] = &(this->lag_comp_data.quat.vector[1]);
   integ_states[9] = &(this->lag_comp_data.quat.vector[2]);
   // Rotational velocity
   integ_states[10] = &(this->lag_comp_data.ang_vel[0]);
   integ_states[11] = &(this->lag_comp_data.ang_vel[1]);
   integ_states[12] = &(this->lag_comp_data.ang_vel[2]);

}


/*!
 * @job_class{shutdown}
 */
PhysicalEntityLagComp::~PhysicalEntityLagComp() // RETURN: -- None.
{
   // Free up any allocated intergrator.
   if ( this->integrator != (Trick::Integrator *)NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->integrator ) ) ) {
         send_hs( stderr, "SpaceFOM::PhysicalEntityBase::~PhysicalEntityBase():%d ERROR deleting Trick Memory for 'this->integrator'%c",
                  __LINE__, THLA_NEWLINE );
      }
      this->integrator = NULL;
   }
}


/*!
 * @job_class{initialization}
 */
void PhysicalEntityLagComp::initialize()
{
   ostringstream errmsg;

   // Create and get a reference to the Trick Euler integrator.
   this->integrator = Trick::getIntegrator( Euler, 26, this->integ_dt);

   if ( this->integrator == (Trick::Integrator *)NULL ) {

      errmsg << "SpaceFOM::PhysicalEntityLagComp::initialize():" << __LINE__
             << " ERROR: Unexpected NULL Trick integrator!"<< THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );

   }

   // Call the base class initialize function.l
   PhysicalEntityLagCompInteg::initialize();

   // Return to calling routine.
   return;
}


/*!
 * @job_class{integration}
 */
int PhysicalEntityLagComp::compensate(
   const double t_begin,
   const double t_end   )
{
   int ipass;
   double dt_go  = t_end - t_begin;

   if ( debug ) {
      cout<< "Compensate: t_begin, t_end, dt_go: " << t_begin << ", " << t_end
          << ", " << dt_go << endl;
   }

   // Propagate the current PhysicalEntity state to the desired time.
   // Set the current integration time for the integrator.
   this->integ_t = t_begin;
   this->integrator->time = this->integ_t;
   // Compute and save the size of this compensation step.
   this->compensate_dt = dt_go;

   // Loop through integrating the state forward to the current scenario time.
   while( (dt_go >= 0.0) && (fabs(dt_go) > this->integ_tol) ) {

      // Print out debug information if requested.
      if ( debug ) {
         cout << "Integ dt, tol, t, dt_go: " << this->integ_dt << ", "
              << this->integ_tol << ", " << integ_t << ", " << dt_go << endl;
      }

      // Integration inner loop.
      // Step through the integrator's integration steps.
      do {

         // Initialize the integration pass.
         ipass = 0;

         // Load the integration states and derivatives.
         this->load();

         // Perform the integration propagation one integration step.
         if ( dt_go > this->integ_dt ){
            // Not near the end; so, use the defined integration step size.
            this->integrator->dt = this->integ_dt;
         }
         else {
            // Near the end; so, use the remainder of the time step.
            this->integrator->dt = dt_go;
         }

         // Call the integrator.
         ipass |= this->integrator->integrate();

         // Unload the integrated states.
         this->unload();

         // Normalize the propagated attitude quaternion.
         QuaternionData::normalize_quaternion( &(this->lag_comp_data.quat.scalar),
                                               this->lag_comp_data.quat.vector     );

      } while ( ipass );

      // Update the integration time.
      this->integ_t = this->integrator->time;

      // Compute the remaining time in the compensation step.
      dt_go = t_end - this->integ_t;

   }

   // Update the lag compensated time,
   lag_comp_data.time = integ_t;

   // Compute the lag compensated value for the attitude quaternion rate.
   QuaternionData::compute_quat_dot( this->lag_comp_data.quat.scalar,
                                     this->lag_comp_data.quat.vector,
                                     this->lag_comp_data.ang_vel,
                                     &(this->Q_dot.scalar),
                                     this->Q_dot.vector );

   return( 0 );
}


/*!
 * @job_class(integration)
 */
void PhysicalEntityLagComp::load()
{
   int istep = integrator->intermediate_step;

   // Load state array: position and velocity.
   for( int iinc = 0; iinc < 12; iinc++ ){
      integrator->state[iinc] = *(integ_states[iinc]);
   }

   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   QuaternionData::compute_quat_dot( this->integrator->state[6],
                                     &(this->integrator->state[7]),
                                     &(this->integrator->state[10]),
                                     &(this->Q_dot.scalar),
                                     this->Q_dot.vector );

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
void PhysicalEntityLagComp::unload()
{

   // Unload state array: position and velocity.
   for( int iinc = 0; iinc < 12; iinc++ ){
      *(integ_states[iinc]) = integrator->state[iinc];
   }

   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   QuaternionData::compute_quat_dot( *(integ_states[6]),
                                     integ_states[7],
                                     integ_states[10],
                                     &(this->Q_dot.scalar),
                                     this->Q_dot.vector );

   // Return to calling routine.
   return;

}
