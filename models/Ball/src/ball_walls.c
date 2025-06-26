/*!
@ingroup Ball
@file models/Ball/src/ball_walls.c
@brief Simple routines to check for ball collision with a wall.

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
@trick_link_dependency{ball_walls.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

/* System include files. */
#include <stdio.h>

/* Trick include files. */
#include "sim_services/Executive/include/exec_proto.h"
#include "sim_services/Integrator/include/integrator_c_intf.h"
#include "sim_services/Message/include/message_proto.h"

/* Model include files. */
#include "../include/ball_proto.h"
#include "../include/ball_state.h"
#include "../include/ball_walls.h"

/*!
 * @job_class{derivative}
 */
double wall_contact(
   double        integ_time,
   double        wall_position,
   double        position,
   REGULA_FALSI *rf_state,
   double       *velocity )
{

   double tgo;

   /* Compute the state error. */
   rf_state->error = position - wall_position;

   /* Call the Regula False routine. */
   tgo = regula_falsi( integ_time, rf_state );

   /* Test for 0 time to go. */
   if ( tgo == 0.0 ) {

      /* Reset the Regula False. */
      reset_regula_falsi( integ_time, rf_state );

      /* Reverse the velocity (elastic collision. */
      *velocity = -*velocity;
   }
   return ( tgo );
}

/*!
 * @job_class{derivative}
 */
double ball_ceiling(
   BallWalls *walls,
   BallState *ball_state )
{
   double integ_time;
   double tgo;

   /* Get the integration time. */
   integ_time = get_integ_time();

   tgo = wall_contact( integ_time,
                       walls->ceiling_y_pos,
                       ball_state->output.position[1],
                       &( ball_state->work.ceiling ),
                       &( ball_state->output.velocity[1] ) );
   if ( tgo == 0.0 && walls->print_contact ) {
      message_publish( 0, "%s hit Ceiling @ t = %g.\n", ball_state->name, integ_time );
   }

   return ( tgo );
}

/*!
 * @job_class{derivative}
 */
double ball_floor(
   BallWalls *walls,
   BallState *ball_state )
{
   double integ_time;
   double tgo;

   /* Get the integration time. */
   integ_time = get_integ_time();

   tgo = wall_contact( integ_time,
                       walls->floor_y_pos,
                       ball_state->output.position[1],
                       &( ball_state->work.floor ),
                       &( ball_state->output.velocity[1] ) );
   if ( tgo == 0.0 && walls->print_contact ) {
      message_publish( 0, "%s hit Floor @ t = %g.\n", ball_state->name, integ_time );
   }

   return ( tgo );
}

/*!
 * @job_class{derivative}
 */
double ball_left_wall(
   BallWalls *walls,
   BallState *ball_state )
{
   double integ_time;
   double tgo;

   /* Get the integration time. */
   integ_time = get_integ_time();

   tgo = wall_contact( integ_time,
                       walls->left_wall_x_pos,
                       ball_state->output.position[0],
                       &( ball_state->work.left_wall ),
                       &( ball_state->output.velocity[0] ) );
   if ( tgo == 0.0 && walls->print_contact ) {
      message_publish( 0, "%s hit Left Wall @ t = %g.\n", ball_state->name, integ_time );
   }

   return ( tgo );
}

/*!
 * @job_class{derivative}
 */
double ball_right_wall(
   BallWalls *walls,
   BallState *ball_state )
{
   double integ_time;
   double tgo;

   /* Get the integration time. */
   integ_time = get_integ_time();

   tgo = wall_contact( integ_time,
                       walls->right_wall_x_pos,
                       ball_state->output.position[0],
                       &( ball_state->work.right_wall ),
                       &( ball_state->output.velocity[0] ) );
   if ( tgo == 0.0 && walls->print_contact ) {
      message_publish( 0, "%s hit Right Wall @ t = %g.\n", ball_state->name, integ_time );
   }

   return ( tgo );
}
