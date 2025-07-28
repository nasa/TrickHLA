/********************************* TRICK HEADER *******************************
PURPOSE: (DifferentialDriveController class)

LIBRARY DEPENDENCY:
    ((Control/src/DifferentialDriveController.cpp)
     (Control/src/PIDController.cpp))
*******************************************************************************/
#ifndef DIFFERENTIAL_DRIVE_CONTROLER_HH
#define DIFFERENTIAL_DRIVE_CONTROLER_HH

#include "../../Motor/include/MotorSpeedController.hh"
#include "PIDController.hh"

namespace TrickHLAModel
{

class DifferentialDriveController
{

  public:
   DifferentialDriveController( double                distance_between_wheels,
                                double                wheel_radius,
                                double                wheel_speed_limit,
                                double                heading_rate_limit,
                                double                slow_down_distance,
                                MotorSpeedController &right_motor_controller,
                                MotorSpeedController &left_motor_controller );

   int  update( double distance_err, double heading_err );
   void stop();

  private:
   // Do not allow the default constructor to be used.
   DifferentialDriveController();

   double distance_between_wheels;
   double wheel_radius;
   double wheel_speed_limit;
   double heading_rate_limit;
   double slow_down_distance;

   MotorSpeedController &right_motor_controller;
   MotorSpeedController &left_motor_controller;

   double right_motor_speed_command;
   double left_motor_speed_command;
   double desired_heading_rate;
   double desired_range_rate;

   // PID Controller
   PIDController heading_ctrl;
   PIDController wheel_speed_ctrl;
};

} // namespace TrickHLAModel
#endif
