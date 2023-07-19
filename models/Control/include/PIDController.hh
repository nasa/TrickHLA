/*****************************************************************************
PURPOSE:    ( PID Class H File )
*****************************************************************************/

#ifndef PIDController_HH
#define PIDController_HH

class PIDController {
public:
  double Kprop;
  double Kderv;
  double Kinteg;
  double Dt;
  double k;
  double error;
  double integral;
  double out_max;
  double out_min;
  double previous_error;
  double prev_setpoint_value;
  bool integration_enabled;

  PIDController(double kp, double ki, double kd, double omax, double omin, double dt, double tc);
  double getOutput(double setpoint_value, double measured_value);
};

#endif
