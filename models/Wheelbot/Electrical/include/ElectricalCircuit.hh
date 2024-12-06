/********************************* TRICK HEADER *******************************
PURPOSE: (Electrical Model)

LIBRARY DEPENDENCY:
    ((Electrical/src/ElectricalCircuit.cpp)
     (Battery/src/DCBattery.cpp)
     (Motor/src/DCMotor.cpp))
*******************************************************************************/
#ifndef ELECTRICAL_CIRCUIT_H
#define ELECTRICAL_CIRCUIT_H

#include "../../Battery/include/DCBattery.hh"
#include "../../Motor/include/DCMotor.hh"

namespace TrickHLAModel
{

class ElectricalCircuit
{
  public:
   DCMotor   *motor1;
   DCMotor   *motor2;
   DCBattery *battery;
   ElectricalCircuit();
   void init( DCMotor *motor1, DCMotor *motor2, DCBattery *battery );
   void update();

  private:
   double motors_current;
};

} // namespace TrickHLAModel
#endif
