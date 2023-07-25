/********************************* TRICK HEADER *******************************
PURPOSE: ( Simulate a two wheeled robotic vehicle.)
LIBRARY DEPENDENCY:
    ((vehicleOne.o)
     (Control/src/vehicleController.o)
     (Control/src/differentialDriveController.o)
     (Motor/src/DCMotorSpeedController.o)
     (Motor/src/DCMotor.o))
PROGRAMMERS:
    (((John M. Penn) (L3 Communications) (June 2015) (Trick Refresher Project)))
    (((Andrew W. Young) (NASA/ER7) (July 2023) (--) (TrickHLA familiarization project.)))
*******************************************************************************/
#include "Vehicle/include/vehicleOne.hh"
#include "Guidance/include/navigator.hh"
#include "../../../../trick/include/trick/MemoryManager.hh"
#include <iostream>
#include <math.h>
#include <chrono>
using namespace TrickHLAModel;

extern Trick::MemoryManager* trick_MM;

int VehicleOne::default_data() {

    distanceBetweenWheels = 0.183;
    wheelRadius = 0.045;
    vehicleMass  = 2.0;
    double axelRadius = 0.5 * distanceBetweenWheels;
    ZAxisMomentofInertia = 0.5 * vehicleMass * axelRadius * axelRadius;

    // Vehicle Controller Parameters
    slowDownDistance = 0.75;
    if(subscriber){
        arrivalDistance = 0.4;
    }else{
        arrivalDistance  = 0.1;
    }
    wheelSpeedLimit = 8.880;
    headingRateLimit = M_PI/4;

    // DCMotor Parameters
    // At 5v the following parameters will result in a current of
    // 0.5 amperes, and a torque of 0.5 x 0.15 = 0.075 Newton-meters.
    DCMotorInternalResistance = 10.0; // Ohms
    DCMotorTorqueConstant     = 0.15; // Newton-meters / Ampere

   // Assuming torque = 0.075 Nm, then the wheel force due to
   // torque = 0.075 Nm / 0.045 = 1.67N. If we want the wheel force
   // to be 0 when wheel speed = 0.4 m/s
   // 0.075 = wheelDragConstant * 0.4
   // wheelDragConstant = 0.075/0.4 = 1.875
    wheelDragConstant  = 1.875;
    corningStiffness = 10.0;

    // SpaceTimeCoordinateState - 3 dimenstions of translational, 3 of rotational, and 1 of time. 
    // Really only using first two dimensions (x,y coordinates) for Wheelbot sim.
    // These are needed to publish/subscribe the position data using TrickHLA.
if(!subscriber){
    position[0] = 0.0; // x-coordinate
    position[1] = 0.0; // y-coordinate
}else{
    position[0] = 1.0;
    position[1] = -1.5;
}
    position[2] = 0.0; // z-coordinate, in this sim, using this to share heading across RTI.
    position[3] = 0.0; // rotational
    position[4] = 0.0; // rotational
    position[5] = 0.0; // rotational
    position[6] = 0.0; // time

    // Place to store SpaceTimeCoordinateState in subscribing Wheelbot. This will be used to update
    // subscribing Wheelbot's waypoints so it can follow the publishing Wheelbot. 
    // Only x and y coordinates are really used in this sim, but all are needed to store 
    // SpaceTimeCoordinateState data.

    stcs[0] = 0.0; // x-coordinate
    stcs[1] = 0.0; // y-coordinate
    stcs[2] = 0.0; // z-coordinate
    stcs[3] = 0.0; // rotational
    stcs[4] = 0.0; // rotational
    stcs[5] = 0.0; // rotational
    stcs[6] = 0.0; // time

    // Place for publisher to store other wheelbot's location to be used for updating the graphics 
    // display.
    tracker[0] = 0.0;
    tracker[1] = 0.0;
    tracker[2] = 0.0;
    tracker[3] = 0.0;
    tracker[4] = 0.0;
    tracker[5] = 0.0;
    tracker[6] = 0.0;

    velocity[0] = 0.0;
    velocity[1] = 0.0;

    acceleration[0] = 0.0;
    acceleration[1] = 0.0;

    heading     = 0.0;
    headingRate = 0.0;
    headingAccel = 0.0;

    // Feedback to Motors
    rightMotorSpeed = 0.0;
    leftMotorSpeed  = 0.0;

    batteryVoltage  = 5.0;

    // Initialize homing variables
    homeCommanded = 0;
    endofHoming = false;

    return 0;
}

int VehicleOne::state_init() {


    Point initLocation( position[0],
                        position[1]);

    rightDCMotor = new DCMotor( DCMotorInternalResistance, DCMotorTorqueConstant );
    leftDCMotor  = new DCMotor( DCMotorInternalResistance, DCMotorTorqueConstant );

    // Note that right and left motor speeds are passed by reference.
    rightMotorController = new DCMotorSpeedController(*rightDCMotor, .3, rightMotorSpeed, batteryVoltage );
    leftMotorController  = new DCMotorSpeedController(*leftDCMotor,  .3, leftMotorSpeed,  batteryVoltage);

    driveController =
        new DifferentialDriveController(distanceBetweenWheels,
                                        wheelRadius,
                                        wheelSpeedLimit,
                                        headingRateLimit,
                                        slowDownDistance,
                                        *rightMotorController,
                                        *leftMotorController);

    navigator =
        new Navigator(heading, initLocation);
    

    vehicleController =
        new VehicleController( &waypointQueue, *navigator, *driveController, arrivalDistance);

    // Register pointers with Trick Memory Manager
    trick_MM->declare_extern_var(navigator, "Navigator");
    trick_MM->declare_extern_var(rightDCMotor, "DCMotor");
    trick_MM->declare_extern_var(leftDCMotor, "DCMotor");
    trick_MM->declare_extern_var(rightMotorController, "DCMotorSpeedController");
    trick_MM->declare_extern_var(leftMotorController, "DCMotorSpeedController");
    trick_MM->declare_extern_var(driveController, "DifferentialDriveController");
    trick_MM->declare_extern_var(vehicleController, "VehicleController");

    return (0);
}

// Add waypoints to the waypoint queue
void VehicleOne::add_waypoint(double x, double y) {
    Point wayPoint(x, y);
    waypointQueue.push_back(wayPoint);
    printWaypoints();
    
}

// Controls the vehicle. If the vehicle is the subscriber, it will follow the vehicle publishing its
//location.
void VehicleOne::control() {

    // Perfect Sensors for now.
    navigator->setHeading(heading);
    navigator->setLocation(position[0], position[1]);

    position[2] = heading;

    // Check to see if the variable server client input for homeCommanded has been activated
    // if so, go home and declare end of simulation
    if (homeCommanded && !endofHoming) {
        vehicleController->gohome();
        endofHoming = true;
    }
    if(!subscriber){
        vehicleController->update();
    }else{
        vehicleController->follow();
    }
    
}


int VehicleOne::state_deriv() {

   double speed = sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]);

   // Direction that the vehicle is moving.
   double velocity_unit[2];
   velocity_unit[0] = velocity[0] / speed;
   velocity_unit[1] = velocity[1] / speed;

   // Direction that the vehicle is pointing.
   double heading_unit[2];
   heading_unit[0] =  cos(heading);
   heading_unit[1] =  sin(heading);

   // Meters/second, positive forward
   double turningSpeed = 0.5 * distanceBetweenWheels * headingRate;

   // Motorspeed: (radians/second), positive counter-clockwise.
   // Feedback to the motor controllers.
   rightMotorSpeed = -(speed + turningSpeed) / wheelRadius;
   leftMotorSpeed  =  (speed - turningSpeed) / wheelRadius;


   // Traction Force: Newtons positive forward
   double leftWheelTractionForce  =  leftDCMotor->getTorque()  / wheelRadius;
   double rightWheelTractionForce = -rightDCMotor->getTorque() / wheelRadius;
   double driveForceMag = leftWheelTractionForce + rightWheelTractionForce;

   // Traction Torque
   vehicleZTorque = (rightWheelTractionForce - leftWheelTractionForce) * (0.5 * distanceBetweenWheels) ;

   driveForce[0] = cos(heading) * driveForceMag;
   driveForce[1] = sin(heading) * driveForceMag;

   // Lateral Tire (Turning) Force
   if (speed == 0.0) {
       lateralTireForce[0] = 0.0;
       lateralTireForce[1] = 0.0;
   } else {
       double tireSlip[2];
       tireSlip[0] = heading_unit[0] - velocity_unit[0];
       tireSlip[1] = heading_unit[1] - velocity_unit[1];

       lateralTireForce[0] = corningStiffness * tireSlip[0];
       lateralTireForce[1] = corningStiffness * tireSlip[1];
   }

   // Rolling Tire Resistance Force
   rollingResistForce[0] = -velocity[0] * wheelDragConstant;
   rollingResistForce[1] = -velocity[1] * wheelDragConstant;

   // Total Body Force
   forceTotal[0] = driveForce[0] + lateralTireForce[0] + rollingResistForce[0];
   forceTotal[1] = driveForce[1] + lateralTireForce[1] + rollingResistForce[1];

   // Body Rotational Acceleration
   headingAccel = vehicleZTorque / ZAxisMomentofInertia;

   // If the simulation is at the end, the vehicle stops moving
   if (vehicleController->getStatus() == true) {
      forceTotal[0] = 0;
      forceTotal[1] = 0;
      rightMotorSpeed = 0;
      leftMotorSpeed = 0;
      velocity[0] = 0;
      velocity[1] = 0;
      headingRate = 0;
      headingAccel = 0;
   }

   // Body Linear Acceleration
   acceleration[0] = forceTotal[0] / vehicleMass;
   acceleration[1] = forceTotal[1] / vehicleMass;

   return 0;
}

#include "trick/integrator_c_intf.h"

int VehicleOne::state_integ() {
    int integration_step;

    load_state(
                &heading,
                &headingRate,
                &position[0],
                &position[1],
                &velocity[0],
                &velocity[1],
                (double*)0
              );

    load_deriv(
                &headingRate,
                &headingAccel,
                &velocity[0],
                &velocity[1],
                &acceleration[0],
                &acceleration[1],
                (double*)0
              );

    integration_step = integrate();

    unload_state(
                &heading,
                &headingRate,
                &position[0],
                &position[1],
                &velocity[0],
                &velocity[1],
                (double*)0
              );

    if (!integration_step) {
        if (heading < -M_PI) heading +=  2*M_PI;
        if (heading >  M_PI) heading += -2*M_PI;
    }

    return(integration_step);

}

// Prints out waypoint queue - useful for debugging.
void VehicleOne::printWaypoints() {
    std::cout << "Waypoints:" << std::endl;
    for (std::vector<Point>::iterator it = waypointQueue.begin(); it != waypointQueue.end(); ++it) {
        Point& waypoint = *it;
        std::cout << "(" << waypoint.getX() << ", " << waypoint.getY() << ")" << std::endl;
    }
}

// Prints out STCS array - useful for debugging.
void VehicleOne::printStcs(){
    std::cout << "STCS Array:" << std::endl;
    for (int i = 0; i < 7; i++) {
        std::cout << "STCS[" << i << "]: " << stcs[i] << std::endl;
    }
}

// Adds waypoint to queue using position data received from publishing vehicle.
void VehicleOne::addWaypointFromSTCS() {
    double lastSTCS[2] = { 0.0, 0.0 };
    // Read the first two elements of stcs
    double currentSTCS[2];
    currentSTCS[0] = stcs[0];
    currentSTCS[1] = stcs[1];

    // Check if the current STCS values are new
    if (currentSTCS[0] != lastSTCS[0] || currentSTCS[1] != lastSTCS[1]) {
        // Update the last stored STCS values
        lastSTCS[0] = currentSTCS[0];
        lastSTCS[1] = currentSTCS[1];
    
        // Add the waypoint using the new STCS values
        add_waypoint(lastSTCS[0], lastSTCS[1]);
    }
    printStcs();
}
