// This file contains functions for controlling the movement of the vehicles.

#include "Control/include/vehicleController.hh"
#include <iostream>

VehicleController::VehicleController( std::vector<Point>* wayPoints,
                                      Navigator& theNavigator,
                                      DifferentialDriveController& theDriveController,
                                      double arrival_distance):
   navigator(theNavigator),
   driveController(theDriveController) {

    // Enforce minimum arrival distance.
    if (arrival_distance > 0.01) {
        arrivalDistance = arrival_distance;
    } else {
        arrivalDistance = 0.01;
    }
    waypointQueue = wayPoints;
    destination = waypointQueue->begin();

    // Initialize homing variables
    endofWaypoints = false;
    homeCommanded = false;

}

void VehicleController::setWayPointQueue( std::vector<Point>* wayPoints ) {
    waypointQueue = wayPoints;
    destination = waypointQueue->begin();
}

int VehicleController::getCurrentDestination(Point& currentDestination) {
    if (destination != waypointQueue->end()) {
        currentDestination = *destination;
        return 0;
    }
    return 1;
}

// Commands wheelbot to navigate to home
void VehicleController::gohome() {
    waypointQueue->empty();
    waypointQueue->push_back(Point(0.0,0.0));
    destination = waypointQueue->end();
    homeCommanded = true;
}

// Prints destination - useful for debugging.
void VehicleController::printDestination() {
    if (destination != waypointQueue->end()) {
        Point& dest = *destination; // Get a reference to the destination Point
        std::cout << "Destination = (" << dest.getX() << "," << dest.getY() << ")." << std::endl;
    }  else {
        std::cout << "No Destination." << std::endl;
    }
}

// Returns the value of the variable endofWaypoints
bool VehicleController::getStatus() {
    return endofWaypoints;
}

// This is how the publishing vehicle will update its navigation.
void VehicleController::update() {
  if (destination == waypointQueue->end() && endofWaypoints == false) {
      if (homeCommanded == true){
        //go home
        if(navigator.distanceTo(*destination)>arrivalDistance){
            double heading_err = navigator.bearingTo(*destination);
            driveController.update(navigator.distanceTo(*destination), heading_err);
        }else{
            std::cout << "Vehicle reached home. End of simulation." << std::endl;
            endofWaypoints = true;
            driveController.update(0.0,0.0);
        }
      }else{
        std::cout << "Vehicle reached the last waypoint. End of simulation." << std::endl;
        endofWaypoints = true;
      }
  }else{
    double distance_err = navigator.distanceTo(*destination);
    if(distance_err > arrivalDistance){
        double heading_err = navigator.bearingTo(*destination);
        driveController.update(distance_err, heading_err);
    }else{
        if(endofWaypoints != true){
            std::cout << "Arrived at Destination." << std::endl;
            destination++;
            if(destination == waypointQueue->end()){
                std::cout << "Vehicle reached the last waypoint. End of simulation." << std::endl;
                endofWaypoints = true;
                driveController.update(0.0,0.0);
            }else{
            }
        }
    }
  }
}

// Function for subscribing vehicle to update its navigation based on the position data
// received from the publishing vehicle.
void VehicleController::follow() {
    std::cout << "VehicleController:following publishing wheelbot..." << std::endl;

    if (!waypointQueue->empty()) {
        if (homeCommanded){
            if(navigator.distanceTo(*destination)>arrivalDistance){
                double heading_err = navigator.bearingTo(*destination);
                driveController.update(navigator.distanceTo(*destination), heading_err);
            }else{
                std::cout << "Vehicle reached home. End of simulation.1" << std::endl;
                endofWaypoints = true;
                driveController.update(0.0,0.0);
            }
        }else{
            destination = waypointQueue->end() - 1; // Set destination to the last entry in waypointQueue.
            printDestination();

            double distance_err = navigator.distanceTo(*destination);
            if (distance_err > arrivalDistance) {
                double heading_err = navigator.bearingTo(*destination);
                driveController.update(distance_err, heading_err);
            } else {
                std::cout << "Arrived at Destination." << std::endl;
            }
        }
        
    }else{
            std::cout << "No waypoints in the queue." << std::endl;
        }
}
