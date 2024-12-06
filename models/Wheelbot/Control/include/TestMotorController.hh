#ifndef TEST_MOTOR_CONTROLLER_HH
#define TEST_MOTOR_CONTROLLER_HH

#include "../../Motor/include/MotorSpeedController.hh"

namespace TrickHLAModel
{

class TestMotorController : public MotorSpeedController
{
   // This class is for testing.
   // It plays the roll of a Controller and a Motor.
  public:
   TestMotorController() : motor_speed( 0.0 ), commanded_speed( 0.0 ) {}

   void set_commanded_speed( double speed_command )
   {
      this->commanded_speed = speed_command;
   }
};

} // namespace TrickHLAModel
#endif
