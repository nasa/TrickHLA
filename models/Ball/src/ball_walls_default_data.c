/*!
@ingroup Ball
@file models/Ball/src/ball_walls_default_data.c
@brief Initializes the wall positions.

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
@trick_link_dependency{ball_walls_default_data.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

/* System include files. */

/* Model include files. */
#include "Ball/include/ball_walls.h"

int ball_walls_default_data(
   BallWalls *walls )
{

   /* Turn off wall contact printing by default. */
   walls->print_contact = false;

   /* Set the wall positions */
   walls->floor_y_pos      = -10.0;
   walls->right_wall_x_pos = 10.0;
   walls->ceiling_y_pos    = 10.0;
   walls->left_wall_x_pos  = -10.0;

   return ( 0 );
}
