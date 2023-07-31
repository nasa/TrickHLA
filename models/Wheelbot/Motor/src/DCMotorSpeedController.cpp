
#include "Motor/include/DCMotorSpeedController.hh"
#include <iostream>

DCMotorSpeedController::DCMotorSpeedController(
        DCMotor& dc_motor,
        double Gain,
        const double& actual_speed,
        const double& supply_voltage)
        : motor_voltage(0.0),
          motor(dc_motor),
          gain(Gain),
          actual_speed(actual_speed),
          supply_voltage(supply_voltage)
{};

void DCMotorSpeedController::set_commanded_speed ( double commanded_speed)
{
    MotorSpeedController::set_commanded_speed( commanded_speed);
    motor_voltage = supply_voltage * gain * (commanded_speed - actual_speed);
    motor_voltage = std :: min (supply_voltage, motor_voltage);
    motor_voltage = std :: max (-supply_voltage, motor_voltage);
    motor.update( motor_voltage);
}

double DCMotorSpeedController::get_motor_voltage()
{
    return motor_voltage;
}
