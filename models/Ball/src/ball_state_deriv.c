/*!
@ingroup Ball
@file models/Ball/src/ball_state_deriv.c
@brief A simple routine to compute the derivative of a ball state.

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
@trick_link_dependency{ball_state_deriv.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

/* Trick include files. */
#include "sim_services/include/collect_macros.h"


/* Model include files. */
#include "../include/ball_state.h"


/*!
 * @job_class{derivative}
 */
int ball_state_deriv(
   BallState * state )
{

   /* GET SHORTHAND NOTATION FOR DATA STRUCTURES */
   BallState_In   *SI = &(state->input);
   BallState_Out  *SO = &(state->output);
   BallState_Work *SW = &(state->work);

   /* LOCAL VARIABLE DECLARATIONS */
   double **collected_forces;

   /* COLLECT EXTERNAL FORCES ON THE BALL  --  TRUST US ON THIS ONE */
   collected_forces = (double**)(SW->external_force);
   SO->external_force[0] = 0.0;
   SO->external_force[1] = 0.0;
   for( int iinc = 0; iinc < NUM_COLLECT(collected_forces); iinc++ ) {
      SO->external_force[0] += collected_forces[iinc][0];
      SO->external_force[1] += collected_forces[iinc][1];
   }

   /* SOLVE FOR THE X AND Y ACCELERATIONS OF THE BALL */
   SO->acceleration[0] = SO->external_force[0] / SI->mass; /* X acceleration */
   SO->acceleration[1] = SO->external_force[1] / SI->mass; /* Y acceleration */

   /* RETURN */
   return(0);
}
