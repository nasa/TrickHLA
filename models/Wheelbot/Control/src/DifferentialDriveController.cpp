#include <iostream>
#include <math.h>

#ifndef PI
#   define PI 3.14159265
#endif

#include "../../Motor/include/MotorSpeedController.hh"

#include "../include/DifferentialDriveController.hh"

DifferentialDriveController::
   DifferentialDriveController( double                distance_between_wheels_,
                                double                wheel_radius_,
                                double                wheel_speed_limit_,
                                double                heading_rate_limit_,
                                double                slow_down_distance_,
                                MotorSpeedController &right_motor_controller_,
                                MotorSpeedController &left_motor_controller_ )
   : distance_between_wheels( distance_between_wheels_ ),
     wheel_radius( wheel_radius_ ),
     wheel_speed_limit( wheel_speed_limit_ ),
     heading_rate_limit( heading_rate_limit_ ),
     slow_down_distance( slow_down_distance_ ),
     right_motor_controller( right_motor_controller_ ),
     left_motor_controller( left_motor_controller_ ),
     right_motor_speed_command( 0.0 ),
     left_motor_speed_command( 0.0 ),
     desired_heading_rate( 0.0 ),
     desired_range_rate( 0.0 ),

     // PID Controller Initialization
     heading_ctrl( 1.0, 0.08, 0.5, heading_rate_limit, -heading_rate_limit, 0.1, 0.1 ),
     wheel_speed_ctrl( 1.0, 0.082, 0.5, wheel_speed_limit, -wheel_speed_limit, 0.1, 0.1 )
{
   return;
}

void DifferentialDriveController::stop()
{
   right_motor_controller.set_commanded_speed( 0.0 );
   left_motor_controller.set_commanded_speed( 0.0 );
}

int DifferentialDriveController::update( double distance_err, // m
                                         double heading_err ) // rad (-PI..+PI)
{
   right_motor_speed_command = 0.0;
   left_motor_speed_command  = 0.0;

   // If the vehicle heading is within 2 degrees of the target, then the
   // heading desired heading rate is proportional to the heading error.
   if ( cos( heading_err ) > cos( 2.0 * ( PI / 180.0 ) ) ) {
      // PID Controller output for heading
      desired_heading_rate = heading_ctrl.get_output( heading_rate_limit, heading_err );
   } else {
      if ( heading_err > 0.0 ) {
         desired_heading_rate = heading_rate_limit;
      } else {
         desired_heading_rate = -heading_rate_limit;
      }
   }

   double wheel_speed_for_heading_rate = ( desired_heading_rate * distance_between_wheels ) / ( 2.0 * wheel_radius );

   double available_wheel_speed_for_range_rate = wheel_speed_limit - fabs( wheel_speed_for_heading_rate );

   double wheel_speed_for_range_rate;

   if ( distance_err > slow_down_distance ) {
      wheel_speed_for_range_rate = available_wheel_speed_for_range_rate;
   } else {
      // PID Controller output for wheelspeed
      wheel_speed_for_range_rate = wheel_speed_ctrl.get_output( available_wheel_speed_for_range_rate, distance_err );
   }

   desired_range_rate = wheel_speed_for_range_rate * wheel_radius;

   double desired_right_wheel_speed = wheel_speed_for_range_rate + wheel_speed_for_heading_rate;
   double desired_left_wheel_speed  = wheel_speed_for_range_rate - wheel_speed_for_heading_rate;

   right_motor_speed_command = -desired_right_wheel_speed;
   left_motor_speed_command  = desired_left_wheel_speed;

   right_motor_controller.set_commanded_speed( right_motor_speed_command );
   left_motor_controller.set_commanded_speed( left_motor_speed_command );

   return 0;
}
