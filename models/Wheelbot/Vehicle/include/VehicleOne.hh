/********************************* TRICK HEADER *******************************
PURPOSE: ( Simulate a two wheeled robotic vehicle.)
LIBRARY DEPENDENCY:
    ((Vehicle/src/VehicleOne.cpp)
     (Control/src/DifferentialDriveController.cpp)
     (Control/src/VehicleController.cpp)
     (Guidance/src/Navigator.cpp)
     (Motor/src/DCMotor.cpp)
     (Motor/src/DCMotorSpeedController.cpp))
PROGRAMMERS:
    (((John M. Penn) (L3 Communications) (June 2015) (Trick Refresher Project)))
    (((Andrew W. Young) (NASA/ER7) (July 2023) (--) (TrickHLA familiarization project.)))
*******************************************************************************/
#ifndef TEST_VEHICLE_HH
#define TEST_VEHICLE_HH

#include <chrono>

#include "../../Control/include/DifferentialDriveController.hh"
#include "../../Control/include/VehicleController.hh"
#include "../../Guidance/include/Navigator.hh"
#include "../../Guidance/include/Point.hh"
#include "../../Motor/include/DCMotor.hh"
#include "../../Motor/include/DCMotorSpeedController.hh"

namespace TrickHLAModel
{

class VehicleOne
{
  public:
   std::vector< Point >  waypoint_queue;
   Navigator            *navigator;
   MotorSpeedController *right_motor_controller;
   MotorSpeedController *left_motor_controller;
   DCMotor              *right_DC_motor;
   DCMotor              *left_DC_motor;

   DifferentialDriveController *drive_controller;
   VehicleController           *vehicle_controller;
   bool                         subscriber;
   double                       distance_between_wheels; /* m */
   double                       wheel_radius;            /* m */
   double                       vehicle_mass;            /* kg */
   double                       z_axis_moment_of_inertia;

   // Vehicle Controller Parameters
   double slow_down_distance;  /* m */
   double arrival_distance;    /* m */
   double wheel_speed_limit;   /* rad/s */
   double heading_rate_limit;  /* rad/s */
   double wheel_drag_constant; /* -- */
   double corning_stiffness;   /* -- */

   // DCMotor Parameters
   double DC_motor_internal_resistance; /* ohm */
   double DC_motor_torque_constant;     /* N*m/amp */

   double position[7];     /* m */
   double stcs[7];         /* m */
   double velocity[2];     /* m/s */
   double acceleration[2]; /* m/s2 */

   double heading;       /* rad */
   double heading_rate;  /* rad/s */
   double heading_accel; /* rad/s2 */

   double right_motor_speed; /* rad/s */
   double left_motor_speed;  /* rad/s */

   // Forces
   double drive_force[2];          /* N */
   double lateral_tire_force[2];   /* N */
   double rolling_resist_force[2]; /* N */
   double force_total[2];          /* N */
   double vehicle_Z_torque;        /* N*m */

   double battery_voltage;

   // Homing Button Variables
   // Get input from Trick server client for homing
   int home_commanded;
   // If Wheelbot was homed and end of simulation
   bool end_of_homing;

   void add_waypoint( double x, double y );

   int  default_data();
   int  state_init();
   void control();
   int  state_deriv();
   int  state_integ();
   void print_waypoints();
   void add_waypoint_from_stcs();
   void print_stcs();
};

} // namespace TrickHLAModel

#endif
