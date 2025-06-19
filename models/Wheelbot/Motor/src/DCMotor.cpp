#include <cmath>
#include <iostream>

#include "Wheelbot/Motor/include/DCMotor.hh"
#include "Wheelbot/Motor/include/PWM.hh"

using namespace TrickHLAModel;

DCMotor::DCMotor( double const initial_internal_resistance,
                  double const initial_motor_torque_constant )
   : motor_torque( 0.0 ),
     motor_current( 0.0 ),
     current_load( 0.0 ),
     internal_resistance( initial_internal_resistance ),
     motor_torque_constant( initial_motor_torque_constant )
{
   return;
}

void DCMotor ::update(
   double const motor_voltage )
{
   motor_current = motor_voltage / internal_resistance;
   motor_torque  = motor_current * motor_torque_constant;
   current_load  = std ::abs( motor_current );
}

void DCMotor::update(
   PWM const &pulse_width )
{
   update( pulse_width.get_average_voltage() );
}

double DCMotor ::get_torque()
{
   return motor_torque;
}

double DCMotor ::get_current_load()
{
   return current_load;
}
