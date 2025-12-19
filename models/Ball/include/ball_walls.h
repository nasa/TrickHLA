/**
@ingroup Ball
@file models/Ball/include/ball_walls.h
@brief Ball walls parameter definition.

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
@trick_link_dependency{../src/ball_walls.c}
@trick_link_dependency{../src/ball_walls_default_data.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

#ifndef MODELS_TRICKHLA_BALL_WALLS_H_
#define MODELS_TRICKHLA_BALL_WALLS_H_

/* System include files. */
#include <stdbool.h>

/** @struct BallWalls
    @brief Ball walls parameters
 */
typedef struct {

   bool   print_contact;    /**< (--) Ball wall contact print control flag. */
   double floor_y_pos;      /**< (m)  Horizontal floor location on Y axis. */
   double right_wall_x_pos; /**< (m)  Vertical right wall location on X axis. */
   double ceiling_y_pos;    /**< (m)  Horizontal ceiling location on Y axis. */
   double left_wall_x_pos;  /**< (m)  Vertical left wall location on X axis. */

} BallWalls;

#endif /* MODELS_TRICKHLA_BALL_WALLS_H_ */
