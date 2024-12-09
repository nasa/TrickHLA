/********************************* TRICK HEADER *******************************
PURPOSE: (DCMotor class)

LIBRARY DEPENDENCY:
    ((Motor/src/DCMotor.cpp)
     (Motor/src/PWM.cpp))
*******************************************************************************/
#ifndef DCMotor_H
#define DCMotor_H

#include "PWM.hh"

namespace TrickHLAModel
{

class DCMotor
{
  public:
   DCMotor( double const initial_internal_resistance,
            double const initial_motor_torque_constant );

   void   update( double const motor_voltage );
   void   update( PWM const &pulse_width );
   double get_torque();
   double get_current_load();

  private:
   double motor_torque;
   double motor_current;
   double current_load;
   double internal_resistance;
   double motor_torque_constant;
};

} // namespace TrickHLAModel
#endif
