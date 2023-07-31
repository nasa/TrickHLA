/*
 * PURPOSE: (MotorSpeedController class)
 */
#ifndef MOTOR_SPEED_CONTROLLER_HH
#define MOTOR_SPEED_CONTROLLER_HH

class MotorSpeedController {

    public:
    virtual ~MotorSpeedController() {}

    virtual void set_commanded_speed( double commanded_speed ) {
        commanded_speed = commanded_speed;
    }
    double get_commanded_speed() { return commanded_speed; }

    protected:
    double commanded_speed;
};

#endif
