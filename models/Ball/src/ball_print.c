/*!
@ingroup Ball
@file models/Ball/src/ball_print.c
@brief A simple routine to compute the derivative of a ball state.

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

/* Trick include files. */
#include "sim_services/Executive/include/exec_proto.h"
#include "sim_services/Message/include/message_proto.h"

/* Model include files. */
#include "../include/ball_proto.h"
#include "../include/ball_state.h"

/*!
 * @job_class{scheduled}
 */
int ball_print(
   BallState *state )
{
   /* Check if printing is active (True). */
   if ( state->input.print_state ) {

      /* GET SHORTHAND NOTATION FOR DATA STRUCTURES */
      BallState_Out *SO = &( state->output );

      message_publish( 0, "Ball %s: time = %8.2f; position = %12.6f , %12.6f; velocity = %12.6f , %12.6f\n",
                       state->name, exec_get_sim_time(),
                       SO->position[0], SO->position[1],
                       SO->velocity[0], SO->velocity[1] );
   }

   return ( 0 );
}
