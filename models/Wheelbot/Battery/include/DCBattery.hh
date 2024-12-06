/******************************************
 PURPOSE:	(DCBattery)
 *******************************************/
#ifndef DCBattery_H
#define DCBattery_H

namespace TrickHLAModel
{

class DCBattery
{
  public:
   DCBattery( double initial_ideal_voltage, double initial_internal_resistance );
   void   update();
   double get_actual_voltage();
   void   set_current( double current );

  private:
   double ideal_voltage;
   double internal_resistance;
   double actual_voltage;
   double current;
};

} // namespace TrickHLAModel
#endif
