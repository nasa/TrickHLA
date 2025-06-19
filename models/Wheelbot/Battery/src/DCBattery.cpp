#include <iostream>

#include "Wheelbot/Battery/include/DCBattery.hh"

using namespace TrickHLAModel;

DCBattery::DCBattery( double initial_ideal_voltage, double initial_internal_resistance )
   : actual_voltage( 0 ),
     current( 0 ),
     ideal_voltage( initial_ideal_voltage ),
     internal_resistance( initial_internal_resistance )
{
   return;
}

void DCBattery ::update()
{
   actual_voltage = ideal_voltage - internal_resistance * current;
   actual_voltage = std ::min( ideal_voltage, actual_voltage );
   actual_voltage = std ::max( 0.0, actual_voltage );
}
double DCBattery ::get_actual_voltage()
{
   return actual_voltage;
}
void DCBattery ::set_current( double value )
{
   current = value;
}
