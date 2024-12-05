#include <cmath>
#include <iostream>

#include "../include/DCMotor.hh"
#include "../include/PWM.hh"

DCMotor::DCMotor( const double initial_internal_resistance,
                  const double initial_motor_torque_constant )
   : motor_torque( 0.0 ),
     motor_current( 0.0 ),
     current_load( 0.0 ),
     internal_resistance( initial_internal_resistance ),
     motor_torque_constant( initial_motor_torque_constant )
{
   return;
}

void DCMotor ::update(
   const double motor_voltage )
{
   motor_current = motor_voltage / internal_resistance;
   motor_torque  = motor_current * motor_torque_constant;
   current_load  = std ::abs( motor_current );
}

void DCMotor::update(
   const PWM &pulse_width )
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
