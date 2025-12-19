/*!
@ingroup Ball
@file models/Ball/src/ball_ensemble_integ.c
@brief A simple routine to integrate (propagate) the state of an array of balls.

\par<b>ASSUMPTIONS AND LIMITATIONS:</b>
- 2 dimensional space\n
- Positive X is horizontal to the right\n
- Positive Y is vertical and up

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
@trick_link_dependency{ball_ensemble_integ.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

/* Trick include files. */
#include "sim_services/Integrator/include/integrator_c_intf.h"

/* Model include files. */
#include "../include/ball_state.h"

/*!
 * @job_class{integration}
 */
int ball_ensemble_integ( int num_balls, BallState *states[] )
{

   int ipass;

   /* LOAD THE POSITION AND VELOCITY STATES */
   for ( int iinc = 0; iinc < num_balls; iinc++ ) {
      load_indexed_state( ( iinc * 4 ), states[iinc]->output.position[0] );
      load_indexed_state( ( iinc * 4 ) + 1, states[iinc]->output.position[1] );
      load_indexed_state( ( iinc * 4 ) + 2, states[iinc]->output.velocity[0] );
      load_indexed_state( ( iinc * 4 ) + 3, states[iinc]->output.velocity[1] );
   }

   /* LOAD THE POSITION AND VELOCITY STATE DERIVATIVES */
   for ( int iinc = 0; iinc < num_balls; iinc++ ) {
      load_indexed_deriv( ( iinc * 4 ), states[iinc]->output.velocity[0] );
      load_indexed_deriv( ( iinc * 4 ) + 1, states[iinc]->output.velocity[1] );
      load_indexed_deriv( ( iinc * 4 ) + 2, states[iinc]->output.acceleration[0] );
      load_indexed_deriv( ( iinc * 4 ) + 3, states[iinc]->output.acceleration[1] );
   }

   /* CALL THE TRICK INTEGRATION SERVICE */
   ipass = integrate();

   /* UNLOAD THE NEW POSITION AND VELOCITY STATES */
   for ( int iinc = 0; iinc < num_balls; iinc++ ) {
      states[iinc]->output.position[0] = unload_indexed_state( iinc * 4 );
      states[iinc]->output.position[1] = unload_indexed_state( ( iinc * 4 ) + 1 );
      states[iinc]->output.velocity[0] = unload_indexed_state( ( iinc * 4 ) + 2 );
      states[iinc]->output.velocity[1] = unload_indexed_state( ( iinc * 4 ) + 3 );
   }

   /* RETURN */
   return ( ipass );
}
