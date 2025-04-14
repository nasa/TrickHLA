/**
@ingroup Ball
@file models/Ball/include/ball_proto.h
@brief Ball function prototype definitions.

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
@trick_link_dependency{../src/ball_ensemble_integ.c}
@trick_link_dependency{../src/ball_ensemble_collision.c}
@trick_link_dependency{../src/ball_force_default_data.c}
@trick_link_dependency{../src/ball_force_field.c}
@trick_link_dependency{../src/ball_print.c}
@trick_link_dependency{../src/ball_state_deriv.c}
@trick_link_dependency{../src/ball_state_init.c}
@trick_link_dependency{../src/ball_state_default_data.c}
@trick_link_dependency{../src/ball_walls.c}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2025, --, TrickHLA Tutorial. }
@revs_end

*/

#ifndef MODELS_TRICKHLA_BALL_PROTO_H_
#define MODELS_TRICKHLA_BALL_PROTO_H_

#include "ball_state.h"
#include "ball_force.h"
#include "ball_walls.h"


#ifdef __cplusplus
extern "C" {
#endif



/*! @brief A simple routine to integrate (propagate) the state of an array of balls.
 *  @param num_balls Number of balls in simulation.
 *  @param states List of ball states in simulation.
 *  @return Integration pass number used by integrator. */
int ball_ensemble_integ( int num_balls, BallState * states[] );

/*! @brief A simple routine to check for ball collisions with walls.
 *  @param walls Ball wall parameters for collision management.
 *  @param num_balls Number of balls in simulation.
 *  @param states List of ball states in simulation.
 *  @return Time to go to next predicted collision event. */
double ball_ensemble_collision( BallWalls * walls,
                                int         num_balls,
                                BallState * states[] );

/*! @brief Initializes the ball force model.
 *  @param force Force parameters for force model.
 *  @return Always returns zero. */
int ball_force_default_data( BallForce * force );

/*! @brief Compute forces based on ball position.
 *  @param force Force parameters for force model.
 *  @param position Current ball state.
 *  @return Always returns zero. */
int ball_force_field( BallForce * force, BallState * position );

/*! @brief Simple print out of ball state.
 *  @param state Ball state parameters.
 *  @return Always returns zero. */
int ball_print( BallState * state );

/*! @brief Initializes the ball state model.
 *  @param state Ball state parameters.
 *  @return Always returns zero. */
int ball_state_default_data( BallState * state );

/*! @brief A simple routine to compute the derivative of a ball state.
 *  @param state Ball state parameters.
 *  @return Always returns zero. */
int ball_state_deriv( BallState * state );

/*! @brief A simple routine to initialize the state of a ball.
 *  @param state Ball state parameters.
 *  @return Always returns zero. */
int ball_state_init( BallState * state );

/*
 * Wall contact testing routines.
 */
/*! @brief Regula Falsi routine for ceiling contact.
 *  @param walls Definition of wall constraints.
 *  @param ball_state Ball state for Regula Falsi testing.
 *  @return Time to go for Regula False logic. */
double ball_ceiling(
   BallWalls * walls,
   BallState * ball_state );

/*! @brief Regula Falsi routine for left wall contact.
 *  @param walls Definition of wall constraints.
 *  @param ball_state Ball state for Regula Falsi testing.
 *  @return Time to go for Regula False logic. */
double ball_left_wall(
   BallWalls * walls,
   BallState * ball_state );

/*! @brief Regula Falsi routine for floor contact.
 *  @param walls Definition of wall constraints.
 *  @param ball_state Ball state for Regula Falsi testing.
 *  @return Time to go for Regula False logic. */
double ball_floor(
   BallWalls * walls,
   BallState * ball_state );

/*! @brief Regula Falsi routine for right wall contact.
 *  @param walls Definition of wall constraints.
 *  @param ball_state Ball state for Regula Falsi testing.
 *  @return Time to go for Regula False logic. */
double ball_right_wall(
   BallWalls * walls,
   BallState * ball_state );

/*! @brief Regula Falsi routine for general wall contact testing.
 *  @param integ_time    Current integration time.
 *  @param wall_position Wall position in component axis.
 *  @param position      Ball position component.
 *  @param rf_state      Wall collision Regula Falsi state.
 *  @param velocity      Ball velocity component.
 *  @return Time to go for Regula False logic. */
double wall_contact(
   double         integ_time,
   double         wall_position,
   double         position,
   REGULA_FALSI * rf_state,
   double       * velocity     );

/*! @brief Regula Falsi routine for general wall contact testing.
 *  @param walls Walls configuration and REgula Falsi data.
 *  @return Always returns zero. */
int ball_walls_default_data( BallWalls * walls );

#ifdef __cplusplus
}
#endif


#endif /* MODELS_TRICKHLA_BALL_PROTO_H_ */
