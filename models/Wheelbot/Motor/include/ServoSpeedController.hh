#ifndef SERVO_SPEED_CONTROLLER_HH
#define SERVO_SPEED_CONTROLLER_HH

#include "MotorSpeedController.hh"
#include "ServoMotor.hh"

class ServoSpeedController : public MotorSpeedController
{
  public:
   ServoSpeedController( ServoMotor &servo_motor, char side );
   void set_commanded_speed( double commanded_speed );

  private:
   ServoSpeedController();
   ServoMotor &servo;
   char        _side;
   double      commanded_speed;
   double      _pulse_width;
}

#endif
