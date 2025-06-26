/*!
@ingroup Ball
@file models/Ball/src/ball_force_field.c
@brief A simple routine to initialize the state of a ball.

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
@trick_link_dependency{ball_force_field.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

/* System include files. */
#include <math.h>

/* Trick include files. */

/* Model include files. */
#include "../include/ball_proto.h"
#include "../include/ball_state.h"

/*!
 * @job_class{initialization}
 */
int ball_state_init(
   BallState *state )
{

   /* GET SHORHAND NOTATION FOR DATA STRUCTURES */
   BallState_In  *BI = &( state->input );
   BallState_Out *BO = &( state->output );

   /* TRANSFER INPUT POSITION STATES TO OUTPUT POSITION STATES */
   BO->position[0] = BI->position[0]; /* X position */
   BO->position[1] = BI->position[1]; /* Y position */

   /* TRANSFER INPUT SPEED AND ELEVATION INTO THE VELOCITY VECTOR */
   BO->velocity[0] = BI->speed * cos( BI->elevation ); /* X velocity */
   BO->velocity[1] = BI->speed * sin( BI->elevation ); /* Y velocity */

   /* RETURN */
   return ( 0 );
}
