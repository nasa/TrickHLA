/*
 * PURPOSE: (VehicleController class)
 */
#ifndef VEHICLE_CONTROLLER_HH
#define VEHICLE_CONTROLLER_HH

#include <vector>
#include "Guidance/include/Point.hh"
#include "Guidance/include/Navigator.hh"
#include "Control/include/DifferentialDriveController.hh"

#ifndef PI
#define PI 3.141592653589793
#endif

class VehicleController {
    public:
    VehicleController(std::vector<Point>* waypoint_queue,
                      Navigator& navigator,
                      DifferentialDriveController& drive_controller,
                      double arrival_distance);

    int get_current_destination(Point& current_destination);
    void set_waypoint_queue( std::vector<Point>* waypoint_queue );
    void print_destination();
    void update();
    void follow();

    // Homing Functions
    // Commands wheelbot to navigate to home
    void go_home();
    // Returns the value of the variable endofWaypoints
    bool get_status();

    private:
    // Do not allow the default constructor to be used.
    VehicleController();

    std::vector<Point>* waypoint_queue;
    std::vector<Point>::iterator destination;
    Point departure;
    Navigator& navigator;
    DifferentialDriveController& drive_controller;

    // Homing variables
    // Records if end of simulation
    bool end_of_waypoints;
    // Records if told to go home
    bool home_commanded;

    double arrival_distance;
};
#endif