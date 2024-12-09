/********************************* TRICK HEADER *******************************
PURPOSE: (MotorSpeedController class)
*******************************************************************************/
#ifndef MOTOR_SPEED_CONTROLLER_HH
#define MOTOR_SPEED_CONTROLLER_HH

namespace TrickHLAModel
{

class MotorSpeedController
{

  public:
   virtual ~MotorSpeedController() {}

   virtual void set_commanded_speed( double commanded_speed )
   {
      this->commanded_speed = commanded_speed;
   }

   double get_commanded_speed()
   {
      return this->commanded_speed;
   }

  protected:
   double commanded_speed;
};

} // namespace TrickHLAModel
#endif
