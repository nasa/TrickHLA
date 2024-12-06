/********************************* TRICK HEADER *******************************
LIBRARY DEPENDENCY:
    ((Electrical/src/ElectricalCircuit.cpp)
     (Battery/src/DCBattery.cpp)
     (Motor/src/DCMotor.cpp))
*******************************************************************************/
#include <iostream>

#include "../../Battery/include/DCBattery.hh"
#include "../../Motor/include/DCMotor.hh"

#include "../include/ElectricalCircuit.hh"

using namespace TrickHLAModel;

ElectricalCircuit::ElectricalCircuit()
   : motors_current( 0 ),
     motor1( 0 ),
     motor2( 0 ),
     battery( 0 )
{
   return;
}

void ElectricalCircuit ::init(
   DCMotor   *motorOne,
   DCMotor   *motorTwo,
   DCBattery *battery1 )
{
   motor1  = motorOne;
   motor2  = motorTwo;
   battery = battery1;
}

void ElectricalCircuit ::update()
{
   motors_current = motor1->get_current_load() + motor2->get_current_load();
   battery->set_current( motors_current );
}
