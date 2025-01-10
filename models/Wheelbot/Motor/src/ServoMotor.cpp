#include <math.h>

#include "../include/ServoMotor.hh"

using namespace TrickHLAModel;

ServoMotor::ServoMotor( char side )
   : _side( side ),
     _pulse_width( 0 ),
     actual_speed( 0.0 )
{
   return;
}

double ServoMotor::get_actual_speed( int pulse_width )
{
   this->_pulse_width = pulse_width;

   if ( _side == 'L' ) {
      actual_speed = -1.8147280722744906e+001 * pow( _pulse_width, 0 )
                     + -3.4553463215611258e-001 * pow( _pulse_width, 1 )
                     + 4.5593326051360884e-002 * pow( _pulse_width, 2 )
                     + -1.8392645176315394e-003 * pow( _pulse_width, 3 )
                     + 3.3261726281542813e-005 * pow( _pulse_width, 4 )
                     + -2.8937430901462806e-007 * pow( _pulse_width, 5 )
                     + 1.2003663411874751e-009 * pow( _pulse_width, 6 )
                     + -1.9140644089539568e-012 * pow( _pulse_width, 7 );
   } else if ( _side == 'R' ) {
      actual_speed = 1.8147280722744906e+001 * pow( _pulse_width, 0 )
                     + 3.4553463215611258e-001 * pow( _pulse_width, 1 )
                     + -4.5593326051360884e-002 * pow( _pulse_width, 2 )
                     + 1.8392645176315394e-003 * pow( _pulse_width, 3 )
                     + -3.3261726281542813e-005 * pow( _pulse_width, 4 )
                     + 2.8937430901462806e-007 * pow( _pulse_width, 5 )
                     + -1.2003663411874751e-009 * pow( _pulse_width, 6 )
                     + 1.9140644089539568e-012 * pow( _pulse_width, 7 );
   }
   return actual_speed;
}
