/*!
@ingroup Ball
@file models/Ball/src/ball_ensemble_collision.c
@brief A simple routine to check for ball collisions with walls.

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
@trick_link_dependency{ball_ensemble_collision.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

/* System include files. */

/* Trick include files. */
#include "sim_services/Executive/include/exec_proto.h"
#include "sim_services/Message/include/message_proto.h"

/* Model include files. */
#include "../include/ball_proto.h"

/*!
 * @job_class{dynamic_event}
 */
double ball_ensemble_collision(
   BallWalls *walls,
   int        num_balls,
   BallState *states[] )
{
   double tgo;
   double event_tgo = BIG_TGO;

   /* NOTE: This routine only checks for collisions with walls not between balls. */

   /* Check for collisions with walls. */
   for ( int iinc = 0; iinc < num_balls; iinc++ ) {

      tgo = ball_ceiling( walls, states[iinc] );
      if ( tgo < event_tgo ) {
         event_tgo = tgo;
      }

      tgo = ball_floor( walls, states[iinc] );
      if ( tgo < event_tgo ) {
         event_tgo = tgo;
      }

      tgo = ball_left_wall( walls, states[iinc] );
      if ( tgo < event_tgo ) {
         event_tgo = tgo;
      }

      tgo = ball_right_wall( walls, states[iinc] );
      if ( tgo < event_tgo ) {
         event_tgo = tgo;
      }

      if ( event_tgo == 0.0 && walls->print_contact ) {
         message_publish( 0, "Ball %s: time = %8.2f; position = %12.6f , %12.6f; velocity = %12.6f , %12.6f\n",
                          states[iinc]->name, exec_get_sim_time(),
                          states[iinc]->output.position[0], states[iinc]->output.position[1],
                          states[iinc]->output.velocity[0], states[iinc]->output.velocity[1] );
      }
   }

   return ( event_tgo );
}
