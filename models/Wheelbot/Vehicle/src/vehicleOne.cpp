/********************************* TRICK HEADER *******************************
PURPOSE: ( Simulate a two wheeled robotic vehicle.)
LIBRARY DEPENDENCY:
    ((VehicleOne.o)
     (Control/src/VehicleController.o)
     (Control/src/DifferentialDriveController.o)
     (Motor/src/DCMotorSpeedController.o)
     (Motor/src/DCMotor.o))
PROGRAMMERS:
    (((John M. Penn) (L3 Communications) (June 2015) (Trick Refresher Project)))
    (((Andrew W. Young) (NASA/ER7) (July 2023) (--) (TrickHLA familiarization project.)))
*******************************************************************************/
#include "Vehicle/include/VehicleOne.hh"
#include "Guidance/include/Navigator.hh"
#include "trick/MemoryManager.hh"
#include <iostream>
#include <math.h>
#include <chrono>
using namespace TrickHLAModel;

extern Trick::MemoryManager* trick_MM;

int VehicleOne::default_data() {

    distance_between_wheels = 0.183;
    wheel_radius = 0.045;
    vehicle_mass  = 2.0;
    double axel_radius = 0.5 * distance_between_wheels;
    z_axis_moment_of_inertia = 0.5 * vehicle_mass * axel_radius * axel_radius;

    // Vehicle Controller Parameters
    slow_down_distance = 0.75;
    arrival_distance = 0.1;
    wheel_speed_limit = 8.880;
    heading_rate_limit = M_PI/4;

    // DCMotor Parameters
    // At 5v the following parameters will result in a current of
    // 0.5 amperes, and a torque of 0.5 x 0.15 = 0.075 Newton-meters.
    DC_motor_internal_resistance = 10.0; // Ohms
    DC_motor_torque_constant     = 0.15; // Newton-meters / Ampere

   // Assuming torque = 0.075 Nm, then the wheel force due to
   // torque = 0.075 Nm / 0.045 = 1.67N. If we want the wheel force
   // to be 0 when wheel speed = 0.4 m/s
   // 0.075 = wheelDragConstant * 0.4
   // wheelDragConstant = 0.075/0.4 = 1.875
    wheel_drag_constant  = 1.875;
    corning_stiffness = 10.0;

    // SpaceTimeCoordinateState - 3 dimenstions of translational, 3 of rotational, and 1 of time. 
    // Really only using first two dimensions (x,y coordinates) for Wheelbot sim.
    // These are needed to publish/subscribe the position data using TrickHLA.

    position[0] = 0.0; // x-coordinate
    position[1] = 0.0; // y-coordinate
    position[2] = 0.0; // z-coordinate 
    position[3] = 0.0; // rotational , using this to share heading across RTI.
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

    velocity[0] = 0.0;
    velocity[1] = 0.0;

    acceleration[0] = 0.0;
    acceleration[1] = 0.0;

    heading     = 0.0;
    heading_rate = 0.0;
    heading_accel = 0.0;

    // Feedback to Motors
    right_motor_speed = 0.0;
    left_motor_speed  = 0.0;

    battery_voltage  = 5.0;

    // Initialize homing variables
    home_commanded = 0;
    end_of_homing = false;

    return 0;
}

int VehicleOne::state_init() {


    Point init_location( position[0],
                        position[1]);

    right_DC_motor = new DCMotor( DC_motor_internal_resistance, DC_motor_torque_constant );
    left_DC_motor  = new DCMotor( DC_motor_internal_resistance, DC_motor_torque_constant );

    // Note that right and left motor speeds are passed by reference.
    right_motor_controller = new DCMotorSpeedController(*right_DC_motor, .3, right_motor_speed, battery_voltage );
    left_motor_controller  = new DCMotorSpeedController(*left_DC_motor,  .3, left_motor_speed,  battery_voltage);

    drive_controller =
        new DifferentialDriveController(distance_between_wheels,
                                        wheel_radius,
                                        wheel_speed_limit,
                                        heading_rate_limit,
                                        slow_down_distance,
                                        *right_motor_controller,
                                        *left_motor_controller);

    navigator =
        new Navigator(heading, init_location);
    

    vehicle_controller =
        new VehicleController( &waypoint_queue, *navigator, *drive_controller, arrival_distance);

    // Register pointers with Trick Memory Manager
    trick_MM->declare_extern_var(navigator, "Navigator");
    trick_MM->declare_extern_var(right_DC_motor, "DCMotor");
    trick_MM->declare_extern_var(left_DC_motor, "DCMotor");
    trick_MM->declare_extern_var(right_motor_controller, "DCMotorSpeedController");
    trick_MM->declare_extern_var(left_motor_controller, "DCMotorSpeedController");
    trick_MM->declare_extern_var(drive_controller, "DifferentialDriveController");
    trick_MM->declare_extern_var(vehicle_controller, "VehicleController");

    return (0);
}

// Add waypoints to the waypoint queue
void VehicleOne::add_waypoint(double x, double y) {
    Point waypoint(x, y);
    waypoint_queue.push_back(waypoint);
}

// Controls the vehicle. If the vehicle is the subscriber, it will follow the vehicle publishing its
//location.
void VehicleOne::control() {

    // Perfect Sensors for now.
    navigator->set_heading(heading);
    navigator->set_location(position[0], position[1]);
    
    // Store heading in position array so it is published to the RTI.
    position[3] = heading;

    // Check to see if the variable server client input for homeCommanded has been activated
    // if so, go home and declare end of simulation
    if (home_commanded && !end_of_homing) {
        vehicle_controller->go_home();
        end_of_homing = true;
    }
    if(!subscriber){
        vehicle_controller->update();
    }else{
        vehicle_controller->follow();
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
   double turning_speed = 0.5 * distance_between_wheels * heading_rate;

   // Motorspeed: (radians/second), positive counter-clockwise.
   // Feedback to the motor controllers.
   right_motor_speed = -(speed + turning_speed) / wheel_radius;
   left_motor_speed  =  (speed - turning_speed) / wheel_radius;


   // Traction Force: Newtons positive forward
   double left_wheel_traction_force  =  left_DC_motor->get_torque()  / wheel_radius;
   double right_wheel_traction_force = -right_DC_motor->get_torque() / wheel_radius;
   double drive_force_mag = left_wheel_traction_force + right_wheel_traction_force;

   // Traction Torque
   vehicle_Z_torque = (right_wheel_traction_force - left_wheel_traction_force) * (0.5 * distance_between_wheels) ;

   drive_force[0] = cos(heading) * drive_force_mag;
   drive_force[1] = sin(heading) * drive_force_mag;

   // Lateral Tire (Turning) Force
   if (speed == 0.0) {
       lateral_tire_force[0] = 0.0;
       lateral_tire_force[1] = 0.0;
   } else {
       double tire_slip[2];
       tire_slip[0] = heading_unit[0] - velocity_unit[0];
       tire_slip[1] = heading_unit[1] - velocity_unit[1];

       lateral_tire_force[0] = corning_stiffness * tire_slip[0];
       lateral_tire_force[1] = corning_stiffness * tire_slip[1];
   }

   // Rolling Tire Resistance Force
   rolling_resist_force[0] = -velocity[0] * wheel_drag_constant;
   rolling_resist_force[1] = -velocity[1] * wheel_drag_constant;

   // Total Body Force
   force_total[0] = drive_force[0] + lateral_tire_force[0] + rolling_resist_force[0];
   force_total[1] = drive_force[1] + lateral_tire_force[1] + rolling_resist_force[1];

   // Body Rotational Acceleration
   heading_accel = vehicle_Z_torque / z_axis_moment_of_inertia;

   // If the simulation is at the end, the vehicle stops moving
   if (vehicle_controller->get_status() == true) {
      force_total[0] = 0;
      force_total[1] = 0;
      right_motor_speed = 0;
      left_motor_speed = 0;
      velocity[0] = 0;
      velocity[1] = 0;
      heading_rate = 0;
      heading_accel = 0;
   }

   // Body Linear Acceleration
   acceleration[0] = force_total[0] / vehicle_mass;
   acceleration[1] = force_total[1] / vehicle_mass;

   return 0;
}

#include "trick/integrator_c_intf.h"

int VehicleOne::state_integ() {
    int integration_step;

    load_state(
                &heading,
                &heading_rate,
                &position[0],
                &position[1],
                &velocity[0],
                &velocity[1],
                (double*)0
              );

    load_deriv(
                &heading_rate,
                &heading_accel,
                &velocity[0],
                &velocity[1],
                &acceleration[0],
                &acceleration[1],
                (double*)0
              );

    integration_step = integrate();

    unload_state(
                &heading,
                &heading_rate,
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
void VehicleOne::print_waypoints() {
    std::cout << "Waypoints:" << std::endl;
    for (std::vector<Point>::iterator it = waypoint_queue.begin(); it != waypoint_queue.end(); ++it) {
        Point& waypoint = *it;
        std::cout << "(" << waypoint.getX() << ", " << waypoint.getY() << ")" << std::endl;
    }
}

// Prints out STCS array - useful for debugging.
void VehicleOne::print_stcs(){
    std::cout << "STCS Array:" << std::endl;
    for (int i = 0; i < 7; i++) {
        std::cout << "STCS[" << i << "]: " << stcs[i] << std::endl;
    }
}

// Adds waypoint to queue using position data received from publishing vehicle.
void VehicleOne::add_waypoint_from_stcs() {
    double last_stcs[2] = { 0.0, 0.0 };
    // Read the first two elements of stcs
    double current_stcs[2];
    current_stcs[0] = stcs[0];
    current_stcs[1] = stcs[1];

    // Check if the current STCS values are new
    if (current_stcs[0] != last_stcs[0] || current_stcs[1] != last_stcs[1]) {
        // Update the last stored STCS values
        last_stcs[0] = current_stcs[0];
        last_stcs[1] = current_stcs[1];
    
        // Add the waypoint using the new STCS values
        add_waypoint(last_stcs[0], last_stcs[1]);
    }
}
