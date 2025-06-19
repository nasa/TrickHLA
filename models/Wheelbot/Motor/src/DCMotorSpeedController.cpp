#include <iostream>

#include "Wheelbot/Motor/include/DCMotor.hh"
#include "Wheelbot/Motor/include/DCMotorSpeedController.hh"
#include "Wheelbot/Motor/include/MotorSpeedController.hh"

using namespace TrickHLAModel;

DCMotorSpeedController::DCMotorSpeedController(
   DCMotor      &dc_motor,
   double        motor_gain,
   double const &actual_speed,
   double const &supply_voltage )
   : motor_voltage( 0.0 ),
     motor( dc_motor ),
     gain( motor_gain ),
     actual_speed( actual_speed ),
     supply_voltage( supply_voltage )
{
   this->commanded_speed = 0.0;
}

void DCMotorSpeedController::set_commanded_speed(
   double cmd_speed )
{
   MotorSpeedController::set_commanded_speed( cmd_speed );
   motor_voltage = supply_voltage * gain * ( cmd_speed - actual_speed );
   motor_voltage = std ::min( supply_voltage, motor_voltage );
   motor_voltage = std ::max( -supply_voltage, motor_voltage );
   motor.update( motor_voltage );
}

double DCMotorSpeedController::get_motor_voltage()
{
   return motor_voltage;
}
