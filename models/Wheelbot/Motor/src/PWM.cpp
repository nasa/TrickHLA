#include <cmath>
#include <iostream>

#include "Wheelbot/Motor/include/PWM.hh"

using namespace TrickHLAModel;

PWM::PWM( double high_voltage_,
          double low_voltage_,
          double duty_cycle_ )
   : high_voltage( high_voltage_ ),
     low_voltage( low_voltage_ )
{
   set_duty_cycle( duty_cycle_ );
}

void PWM::set_duty_cycle( double duty_cycle_ )
{
   if ( ( duty_cycle_ >= 0.0 ) && ( duty_cycle_ <= 1.0 ) ) {
      duty_cycle = duty_cycle_;
   } else {
      throw std::logic_error( "PWM::PWM(): DutyCycle must be between 0.0 .. 1.0." );
   }
}

double PWM ::get_duty_cycle() const
{
   return duty_cycle;
}

double PWM::get_average_voltage() const
{
   return ( ( high_voltage * duty_cycle + low_voltage * ( 1 - duty_cycle ) ) / 2 );
}
