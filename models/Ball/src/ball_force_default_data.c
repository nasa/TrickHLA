/*!
@ingroup Ball
@file models/Ball/src/ball_force_default_data.c
@brief Initializes the force model on the ball.

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
@trick_link_dependency{ball_force_default_data.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

/* System include files. */

/* Model include files. */
#include "Ball/include/ball_force.h"

/*!
 * @job_class{default_data}
 */
int ball_force_default_data(
  BallForce * force )
{

   force->input.origin[0] = 0.0 ;
   force->input.origin[1] = 2.0 ;
   force->input.force = 8.0 ;

   /* RETURN */
   return( 0 ) ;

}
