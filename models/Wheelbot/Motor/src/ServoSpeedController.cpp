#include <iostream>
#include <math.h>

#include "Wheelbot/Motor/include/ServoMotor.hh"
#include "Wheelbot/Motor/include/ServoSpeedController.hh"

using namespace TrickHLAModel;

ServoSpeedController::ServoSpeedController(
   ServoMotor &servo_motor,
   char        side )
   : servo( servo_motor ),
     _side( side ),
     _pulse_width( 0.0 )
{
   this->commanded_speed = 0.0;
}

void ServoSpeedController::set_commanded_speed( double commanded_speed )
{
   this->commanded_speed = commanded_speed;

   if ( _side == 'L' ) {
      std::cout << "The left wheel desired speed is: " << commanded_speed << '\n';

      double pulse_width = 9.1296697267545980e+001 * pow( commanded_speed, 0 )
                           + 1.3551549019843796e+000 * pow( commanded_speed, 1 )
                           + -2.5748263162935388e-002 * pow( commanded_speed, 2 )
                           + -3.7691759514032080e-003 * pow( commanded_speed, 3 )
                           + 3.8490572015823302e-004 * pow( commanded_speed, 4 )
                           + 4.5526955758039407e-005 * pow( commanded_speed, 5 )
                           + -6.7622608926425730e-007 * pow( commanded_speed, 6 );
      std::cout << "The pulse width for the side above is " << pulse_width << '\n';

      _pulse_width = (int)pulse_width;
   } else if ( _side == 'R' ) {

      std::cout << "The right wheel desired speed is: " << commanded_speed << '\n';

      double pulse_width = 9.1296697267545980e+001 * pow( commanded_speed, 0 )
                           + -1.3551549019843796e+000 * pow( commanded_speed, 1 )
                           + -2.5748263162935388e-002 * pow( commanded_speed, 2 )
                           + 3.7691759514032080e-003 * pow( commanded_speed, 3 )
                           + 3.8490572015823302e-004 * pow( commanded_speed, 4 )
                           + -4.5526955758039407e-005 * pow( commanded_speed, 5 )
                           + -6.7622608926425730e-007 * pow( commanded_speed, 6 );
      std::cout << "The pulse width for the side above is " << pulse_width << '\n';

      _pulse_width = (int)pulse_width;
   }

   if ( _pulse_width > 180 ) {
      _pulse_width = 180;
   } else if ( _pulse_width < 0 ) {
      _pulse_width = 0;
   }

   if ( _side == 'L' ) {
      std::cout << "Left Servo angle is: " << _pulse_width << '\n';
   } else if ( _side == 'R' ) {
      std::cout << "Right Servo angle is: " << _pulse_width << '\n';
   }
}
