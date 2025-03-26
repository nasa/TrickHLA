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
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{PhysicalEntityLagComp.cpp}


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
#include "SpaceFOM/PhysicalEntityLagComp.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityLagComp::PhysicalEntityLagComp( PhysicalEntityBase &entity_ref ) // RETURN: -- None.
   : PhysicalEntityLagCompInteg( entity_ref )
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
PhysicalEntityLagComp::~PhysicalEntityLagComp() // RETURN: -- None.
{
   // Free up any allocated intergrator.
   if ( this->integrator != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->integrator ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalEntityBase::~PhysicalEntityBase():%d WARNING failed to delete Trick Memory for 'this->integrator'\n",
                          __LINE__ );
      }
      this->integrator = NULL;
   }
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityLagComp::initialize()
{
   // Create and get a reference to the Trick Euler integrator.
   this->integrator = Trick::getIntegrator( Euler, 26, this->integ_dt );

   if ( this->integrator == NULL ) {
      ostringstream errmsg;

      errmsg << "SpaceFOM::PhysicalEntityLagComp::initialize():" << __LINE__
             << " ERROR: Unexpected NULL Trick integrator!\n";
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
void PhysicalEntityLagComp::update_time()
{
   this->lag_comp_data.time = this->integ_t;
   return;
}

/*!
 * @job_class{integration}
 */
void PhysicalEntityLagComp::load()
{
   int istep = integrator->intermediate_step;

   // Load state array: position and velocity.
   for ( int iinc = 0; iinc < 13; ++iinc ) {
      integrator->state[iinc] = *( integ_states[iinc] );
   }

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
void PhysicalEntityLagComp::unload()
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

/*! @job_class{derivative} */
void PhysicalEntityLagComp::derivative_first(
   void *user_data )
{
   // Compute the derivative of the attitude quaternion from the
   // angular velocity vector.
   Q_dot.derivative_first( this->lag_comp_data.att, this->lag_comp_data.ang_vel );

   return;
}
