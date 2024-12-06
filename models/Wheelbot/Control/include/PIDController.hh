/********************************* TRICK HEADER *******************************
PURPOSE: (PID class)

LIBRARY DEPENDENCY:
    ((Control/src/PIDController.cpp))
*******************************************************************************/
#ifndef PIDController_HH
#define PIDController_HH

namespace TrickHLAModel
{

class PIDController
{
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
   bool   integration_enabled;

   PIDController( double kp, double ki, double kd, double omax, double omin, double dt, double tc );
   double get_output( double setpoint_value, double measured_value );
};

} // namespace TrickHLAModel
#endif
