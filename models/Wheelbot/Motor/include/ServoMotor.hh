/********************************* TRICK HEADER *******************************
PURPOSE: (ServoMotor class)

LIBRARY DEPENDENCY:
    ((Motor/src/ServoMotor.cpp))
*******************************************************************************/
#ifndef SERVO_MOTOR_HH
#define SERVO_MOTOR_HH

namespace TrickHLAModel
{

class ServoMotor
{
  public:
   explicit ServoMotor( char side );

   double get_actual_speed( int pulse_width );

  private:
   ServoMotor();

   char   _side;
   int    _pulse_width;
   double actual_speed;
};

} // namespace TrickHLAModel
#endif
