#include <algorithm>
#include <gtest/gtest.h>
#define private public

#include "Wheelbot/Guidance/include/Navigator.hh"
#include "Wheelbot/Guidance/include/Point.hh"

#include "Wheelbot/Control/include/DifferentialDriveController.hh"
#include "Wheelbot/Control/include/TestMotorController.hh"
#include "Wheelbot/Control/include/VehicleController.hh"

#ifndef PI
#   define PI 3.1415926535
#endif
#define FLOAT_TOLERANCE 0.000001

using namespace TrickHLAModel;

/*
Test Fixture.
*/
class VehicleControllerTest : public ::testing::Test
{
  protected:
   std::vector< Point >         waypoint_queue;
   Navigator                   *navigator;
   TestMotorController         *right_motor_controller;
   TestMotorController         *left_motor_controller;
   DifferentialDriveController *drive_controller;
   VehicleController           *vehicle_controller;

   VehicleControllerTest()
   {
      Point waypoint( 1.0, 3.0 );
      waypoint_queue.push_back( waypoint );

      Point init_location( 0.0, 0.0 );
      right_motor_controller = new TestMotorController();
      left_motor_controller  = new TestMotorController();
      drive_controller       = new DifferentialDriveController( 0.183, 0.045, 8.880, 0.200,
                                                                *right_motor_controller,
                                                                *left_motor_controller );
      navigator              = new Navigator( 0.0, init_location );
      vehicle_controller     = new VehicleController( &waypoint_queue, *navigator, *drive_controller, 0.02 );
   }

   ~VehicleControllerTest()
   {
      delete navigator;
      delete driveController;
   }

   void SetUp() {}
   void TearDown() {}
};

TEST_F( VehicleControllerTest, one )
{
   Point current_destination;
   int   result = vehicle_controller->get_current_destination( current_destination );
   EXPECT_EQ( result, 0 );
   EXPECT_EQ( current_destination.getX(), 1 );
   EXPECT_EQ( current_destination.getY(), 3 );
}
