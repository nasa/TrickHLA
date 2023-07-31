#ifndef PWM_H
#define PWM_H
/********************************* TRICK HEADER *******************************
PURPOSE: ()
LIBRARY DEPENDENCY:
    ((PWM.o))
*******************************************************************************/

#include<stdexcept>

class PWM {

  public:

    double high_voltage;
    double low_voltage;

    PWM( double high_voltage,
         double low_voltage,
         double duty_cycle);

    ~PWM() {}

    void   set_duty_cycle( double duty_cycle);
    double get_duty_cycle() const;
    double get_average_voltage() const;

  private:

    PWM(){};
    double duty_cycle;
};

#endif
