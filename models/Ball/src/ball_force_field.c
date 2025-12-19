/*!
@ingroup Ball
@file models/Ball/src/ball_force_field.c
@brief Compute forces based on ball position.

@detail
- computes a relative vector from the ball to the force field origin
- computes the unit vector in the direction of this relative vector
- scales the unit vector by the magnitude of the constant force field)

\par<b>ASSUMPTIONS AND LIMITATIONS:</b>
- 2 dimensional space\n
- Positive X is horizontal to the right\n
- Positive Y is vertical and up
- Resulting force is 'collect'ed in the S_define file

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
#include "../include/ball_force.h"
#include "../include/ball_proto.h"

/*!
 * @job_class{derivative}
 */
int ball_force_field(
   BallForce *force,
   BallState *state )
{

   /* GET SHORTHAND NOTATION FOR DATA STRUCTURES */
   BallForce_In  *FI = &( force->input );
   BallForce_Out *FO = &( force->output );

   /* LOCAL VARIABLE DECLARATIONS */
   double mag;
   double rel[2];
   double unit[2];

   /* GET RELATIVE VECTOR FROM BALL TO FORCE ORIGIN */
   rel[0] = FI->origin[0] - state->output.position[0];
   rel[1] = FI->origin[1] - state->output.position[1];

   /* GET UNIT VECTOR AND POSITION MAGNITUDE FROM BALL TO FORCE ORIGIN */
   mag     = sqrt( rel[0] * rel[0] + rel[1] * rel[1] );
   unit[0] = rel[0] / mag;
   unit[1] = rel[1] / mag;

   /* COMPUTE EXTERNAL FORCE ON BALL IN THE DIRECTION OF THE UNIT VECTOR */
   FO->force[0] = FI->force * unit[0];
   FO->force[1] = FI->force * unit[1];

   /* RETURN */
   return ( 0 );
}
