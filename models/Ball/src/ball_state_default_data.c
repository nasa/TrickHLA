/*!
@ingroup Ball
@file models/Ball/src/ball_state_default_data.c
@brief Initializes the ball state.

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
@trick_link_dependency{ball_state_default_data.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

/* System include files. */
#include <math.h>

/* Trick includes. */

/* Model include files. */
#include "Ball/include/ball_state.h"


/*!
 * @job_class{default_data}
 */
int ball_state_default_data(
  BallState * state )
{
   /* Setup ball initial state. */
   state->input.print_state = false;
   state->input.mass        = 10.0 ;
   state->input.speed       = 3.5 ;
   state->input.elevation   = (0.0174532925199433) * 45.0;
   state->input.position[0] = 5.0;
   state->input.position[1] = 5.0;

   /* Initialize the ball wall contact Regula Falsi working data structures.
    * These have to go with the ball since it is associated with the
    * propagated state of the ball and not really the wall.
    */
   state->work.floor.lower_set = 0;
   state->work.floor.upper_set = 0;
   state->work.floor.iterations = 0;
   state->work.floor.fires = 0;
   state->work.floor.x_lower = BIG_TGO;
   state->work.floor.t_lower = BIG_TGO;
   state->work.floor.x_upper = BIG_TGO;
   state->work.floor.t_upper = BIG_TGO;
   state->work.floor.delta_time = BIG_TGO;
   state->work.floor.error_tol = 1.0e-6;
   state->work.floor.mode = Decreasing;

   state->work.right_wall.lower_set = 0;
   state->work.right_wall.upper_set = 0;
   state->work.right_wall.iterations = 0;
   state->work.right_wall.fires = 0;
   state->work.right_wall.x_lower = BIG_TGO;
   state->work.right_wall.t_lower = BIG_TGO;
   state->work.right_wall.x_upper = BIG_TGO;
   state->work.right_wall.t_upper = BIG_TGO;
   state->work.right_wall.delta_time = BIG_TGO;
   state->work.right_wall.error_tol = 1.0e-6;
   state->work.right_wall.mode = Increasing;

   state->work.ceiling.lower_set = 0;
   state->work.ceiling.upper_set = 0;
   state->work.ceiling.iterations = 0;
   state->work.ceiling.fires = 0;
   state->work.ceiling.x_lower = BIG_TGO;
   state->work.ceiling.t_lower = BIG_TGO;
   state->work.ceiling.x_upper = BIG_TGO;
   state->work.ceiling.t_upper = BIG_TGO;
   state->work.ceiling.delta_time = BIG_TGO;
   state->work.ceiling.error_tol = 1.0e-6;
   state->work.ceiling.mode = Increasing;

   state->work.left_wall.lower_set = 0;
   state->work.left_wall.upper_set = 0;
   state->work.left_wall.iterations = 0;
   state->work.left_wall.fires = 0;
   state->work.left_wall.x_lower = BIG_TGO;
   state->work.left_wall.t_lower = BIG_TGO;
   state->work.left_wall.x_upper = BIG_TGO;
   state->work.left_wall.t_upper = BIG_TGO;
   state->work.left_wall.delta_time = BIG_TGO;
   state->work.left_wall.error_tol = 1.0e-6;
   state->work.left_wall.mode = Decreasing;

   return( 0 );

}
