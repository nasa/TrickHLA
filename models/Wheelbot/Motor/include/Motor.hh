/********************************* TRICK HEADER *******************************
PURPOSE: (Motor class)

LIBRARY DEPENDENCY:
    ((Motor/src/Motor.cpp)
     (Motor/src/PWM.cpp))
*******************************************************************************/
#ifndef MOTOR
#define MOTOR

#include "PWM.hh"

namespace TrickHLAModel
{

class Motor
{
  public:
   Motor();

   virtual void update( const PWM &pulse_width ) = 0;
   virtual ~Motor();

   virtual double get_actual_speed();
};

} // namespace TrickHLAModel
#endif
