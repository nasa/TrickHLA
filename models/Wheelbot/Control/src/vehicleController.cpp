// This file contains functions for controlling the movement of the vehicles.

#include "Control/include/vehicleController.hh"
#include <iostream>

VehicleController::VehicleController( std::vector< Point >        *waypoints,
                                      Navigator                   &the_navigator,
                                      DifferentialDriveController &the_drive_controller,
                                      double                       arrival_distance_ )
   : waypoint_queue( waypoints ),
     navigator( the_navigator ),
     drive_controller( the_drive_controller ),
     end_of_waypoints( false ),
     home_commanded( false )
{
   // Enforce minimum arrival distance.
   if ( arrival_distance_ > 0.01 ) {
      arrival_distance = arrival_distance_;
   } else {
      arrival_distance = 0.01;
   }
   // waypoint_queue = waypoints;
   destination = waypoint_queue->begin();
}

void VehicleController::set_waypoint_queue( std::vector< Point > *waypoints )
{
   waypoint_queue = waypoints;
   destination    = waypoint_queue->begin();
}

int VehicleController::get_current_destination( Point &current_destination )
{
   if ( destination != waypoint_queue->end() ) {
      current_destination = *destination;
      return 0;
   }
   return 1;
}

// Commands wheelbot to navigate to home
void VehicleController::go_home()
{
   waypoint_queue->empty();
   waypoint_queue->push_back( Point( 0.0, 0.0 ) );
   destination    = waypoint_queue->end();
   home_commanded = true;
}

// Prints destination - useful for debugging.
void VehicleController::print_destination()
{
   if ( destination != waypoint_queue->end() ) {
      Point &dest = *destination; // Get a reference to the destination Point
      std::cout << "Destination = (" << dest.getX() << "," << dest.getY() << ")." << std::endl;
   } else {
      std::cout << "No Destination." << std::endl;
   }
}

// Returns the value of the variable endofWaypoints
bool VehicleController::get_status()
{
   return end_of_waypoints;
}

// This is how the publishing vehicle will update its navigation.
void VehicleController::update()
{
   if ( destination == waypoint_queue->end() && end_of_waypoints == false ) {
      if ( home_commanded == true ) {
         // go home
         if ( navigator.distance_to( *destination ) > arrival_distance ) {
            double heading_err = navigator.bearing_to( *destination );
            drive_controller.update( navigator.distance_to( *destination ), heading_err );
         } else {
            std::cout << "Vehicle reached home. End of simulation." << std::endl;
            end_of_waypoints = true;
            drive_controller.update( 0.0, 0.0 );
         }
      } else {
         std::cout << "Vehicle reached the last waypoint. End of simulation." << std::endl;
         end_of_waypoints = true;
      }
   } else {
      double distance_err = navigator.distance_to( *destination );
      if ( distance_err > arrival_distance ) {
         double heading_err = navigator.bearing_to( *destination );
         drive_controller.update( distance_err, heading_err );
      } else {
         if ( end_of_waypoints != true ) {
            std::cout << "Arrived at Destination." << std::endl;
            destination++;
            if ( destination == waypoint_queue->end() ) {
               std::cout << "Vehicle reached the last waypoint. End of simulation." << std::endl;
               end_of_waypoints = true;
               drive_controller.update( 0.0, 0.0 );
            } else {
            }
         }
      }
   }
}

// Function for subscribing vehicle to update its navigation based on the position data
// received from the publishing vehicle.
void VehicleController::follow()
{
   std::cout << "VehicleController:following publishing wheelbot..." << std::endl;

   if ( !waypoint_queue->empty() ) {
      if ( home_commanded ) {
         if ( navigator.distance_to( *destination ) > arrival_distance ) {
            double heading_err = navigator.bearing_to( *destination );
            drive_controller.update( navigator.distance_to( *destination ), heading_err );
         } else {
            std::cout << "Vehicle reached home. End of simulation.1" << std::endl;
            end_of_waypoints = true;
            drive_controller.update( 0.0, 0.0 );
         }
      } else {
         destination = waypoint_queue->end() - 1; // Set destination to the last entry in waypointQueue.
         print_destination();

         double distance_err = navigator.distance_to( *destination );
         if ( distance_err > arrival_distance ) {
            double heading_err = navigator.bearing_to( *destination );
            drive_controller.update( distance_err, heading_err );
         } else {
            std::cout << "Arrived at Destination." << std::endl;
         }
      }

   } else {
      std::cout << "No waypoints in the queue." << std::endl;
   }
}
