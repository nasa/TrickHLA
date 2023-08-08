#ifndef DCMotor_H
#define DCMotor_H
/********************************* TRICK HEADER *******************************
PURPOSE: ()
LIBRARY DEPENDENCY:
    ((DCMotor.o))
*******************************************************************************/
#include "PWM.hh"

class DCMotor {
public:
    DCMotor(const double initial_internal_resistance, const double initial_motor_torque_constant);
    void update (const double motor_voltage);
    void update(const PWM& pulse_width);
    double get_torque ();
    double get_current_load ();

private:
    double motor_torque;
    double motor_current;
    double current_load;
    double internal_resistance;
    double motor_torque_constant;
};

#endif

