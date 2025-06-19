/**
@ingroup Ball
@file models/Ball/include/ball_state.h
@brief Ball model EOM state parameter definition.

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
@trick_link_dependency{../src/ball_state_default_data.c}
@trick_link_dependency{../src/ball_state_deriv.c}
@trick_link_dependency{../src/ball_state_init.c}
@trick_link_dependency{../src/ball_ensemble_collision.c}
@trick_link_dependency{../src/ball_ensemble_integ.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

#ifndef MODELS_TRICKHLA_BALL_STATE_H_
#define MODELS_TRICKHLA_BALL_STATE_H_

/* System include files. */
#include <stdbool.h>

/* Trick includes. */
#include "trick/compat/sim_services/Integrator/include/regula_falsi.h"

/** @struct BallState_In
    @brief Ball state input parameters
 */
typedef struct {

   /*=== Initial Ball States ===*/
   bool   print_state; /**< (--)  Ball state print control flag. */
   double mass ;       /**< (kg)  Total mass. */
   double position[2]; /**< (m)   X(horizontal),Y(vertical) position. */
   double speed;       /**< (m/s) Linear speed. */
   double elevation;   /**< (rad) Trajectory angle with respect to the horizontal. */

} BallState_In;

/** @struct BallState_Out
    @brief Ball state output parameters
 */
typedef struct {

   double position[2];       /**< (m)    X(horizontal), Y(vertical) position. */
   double velocity[2];       /**< (m/s)  X,Y velocity. */
   double acceleration[2];   /**< (m/s2) X,Y acceleration. */
   double external_force[2]; /**< (N)    Total external force on ball. */

} BallState_Out;

/** @struct BallState_Work
    @brief Ball state work parameters
 */
typedef struct {

   void ** external_force; /**< ** (N) External forces, from 'collect'. */

   REGULA_FALSI floor;      /**< (--) Dynamic event parameters for floor impact. */
   REGULA_FALSI right_wall; /**< (--) Dynamic event parameters for right wall impact. */
   REGULA_FALSI ceiling;    /**< (--) Dynamic event parameters for ceiling impact. */
   REGULA_FALSI left_wall;  /**< (--) Dynamic event parameters for left wall impact. */

} BallState_Work;

/** @struct BallState
    @brief Ball state structure
 */
typedef struct {

   char           * name;   /**< (--)    Name of ball. */
   unsigned int     id;     /**< (count) Ball ID. */
   BallState_In     input;  /**< (--)    User inputs. */
   BallState_Out    output; /**< (--)    User outputs. */
   BallState_Work   work;   /**< (--)    EOM workspace. */

} BallState;


#endif /* MODELS_TRICKHLA_BALL_STATE_H_ */
