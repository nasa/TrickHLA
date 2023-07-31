#ifndef SERVO_MOTOR_HH
#define SERVO_MOTOR_HH

class ServoMotor
{
  public:
    ServoMotor (char side);
    double get_actual_speed( int pulse_width );
  private:
    ServoMotor();
    char _side;
    int _pulse_width;
    double actual_speed;
}

#endif