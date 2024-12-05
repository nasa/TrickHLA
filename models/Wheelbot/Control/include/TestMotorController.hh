#ifndef TEST_MOTOR_CONTROLLER_HH
#define TEST_MOTOR_CONTROLLER_HH

#include "../../Motor/include/MotorSpeedController.hh"

class TestMotorController : public MotorSpeedController
{

   // This class is for testing.
   // It plays the roll of a Controller and a Motor.
  public:
   TestMotorController() : motor_speed( 0.0 ) {}

   void set_commanded_speed( double speed_command )
   {
      commanded_speed = speed_command;
   }

  private:
   double commanded_speed; // rad/s
};
#endif
