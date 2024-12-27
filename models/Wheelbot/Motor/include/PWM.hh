/********************************* TRICK HEADER *******************************
PURPOSE: (PWM class)

LIBRARY DEPENDENCY:
    ((Motor/src/PWM.cpp))
*******************************************************************************/
#ifndef PWM_H
#define PWM_H

#include <stdexcept>

namespace TrickHLAModel
{

class PWM
{

  public:
   double high_voltage;
   double low_voltage;

   PWM( double high_voltage,
        double low_voltage,
        double duty_cycle );

   virtual ~PWM() {}

   void   set_duty_cycle( double duty_cycle );
   double get_duty_cycle() const;
   double get_average_voltage() const;

  private:
   PWM() : high_voltage( 0.0 ), low_voltage( 0.0 ), duty_cycle( 0.0 ) {};
   double duty_cycle;
};

} // namespace TrickHLAModel
#endif
