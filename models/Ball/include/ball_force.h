/**
@ingroup Ball
@file models/Ball/include/ball_force.h
@brief Ball force parameter definition.

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
@trick_link_dependency{../src/ball_force_field.c}
@trick_link_dependency{../src/ball_force_default_data.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

#ifndef MODELS_TRICKHLA_BALL_FORCE_H_
#define MODELS_TRICKHLA_BALL_FORCE_H_

/** @struct BallForce_In
    @brief ball force input parameters
 */
typedef struct {

   double origin[2]; /**< (m) Origin of force center */
   double force;     /**< (N) Force magnitude */

} BallForce_In;

/** @struct BallForce_Out
    @brief ball force output parameters
 */
typedef struct {

   double force[2]; /**< (N) Resulting X,Y force on ball */

} BallForce_Out;

/** @struct BallForce
    @brief ball force parameters
 */
typedef struct {

   BallForce_In  input;  /**< (--) User inputs. */
   BallForce_Out output; /**< (--) User outputs. */

} BallForce;

#endif /* MODELS_TRICKHLA_BALL_FORCE_H_ */
