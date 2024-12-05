#include <algorithm>
#include <gtest/gtest.h>

#define private public

#include "../include/DifferentialDriveController.hh"
#include "../include/TestMotorController.hh"

#ifndef PI
#   define PI 3.1415926535
#endif
#define FLOAT_TOLERANCE 0.000001

/*
    Test Fixture.
*/
class DifferentialDriveControllerTest : public ::testing::Test
{
  protected:
   TestMotorController         *right_motor_controller;
   TestMotorController         *left_motor_controller;
   DifferentialDriveController *drive_controller;

   DifferentialDriveControllerTest()
   {
      right_motor_controller =
         new TestMotorController();
      left_motor_controller =
         new TestMotorController();
      drive_controller =
         new DifferentialDriveController( 0.183, 0.045, 8.880, 0.200,
                                          *right_motor_controller, *left_motor_controller );
   }

   ~DifferentialDriveControllerTest()
   {
      delete drive_controller;
      delete right_motor_controller;
      delete left_motor_controller;
   }
};

TEST_F( DifferentialDriveControllerTest, constructor )
{
   EXPECT_NEAR( 0.045, drive_controller->wheel_radius, FLOAT_TOLERANCE );
   EXPECT_NEAR( 0.183, drive_controller->distance_between_wheels, FLOAT_TOLERANCE );
   EXPECT_NEAR( 8.880, drive_controller->wheel_speed_limit, FLOAT_TOLERANCE );
   EXPECT_NEAR( 0.200, drive_controller->slow_down_distance, FLOAT_TOLERANCE );
}

TEST_F( DifferentialDriveControllerTest, setDistanceBetweenWheels )
{
   int result;

   result = drive_controller->set_distance_between_wheels( 0.1 );
   EXPECT_EQ( 0, result );
   EXPECT_NEAR( 0.1, drive_controller->distance_between_wheels, FLOAT_TOLERANCE );

   result = drive_controller->set_distance_between_wheels( 0.2 );
   EXPECT_EQ( 0, result );
   EXPECT_NEAR( 0.2, drive_controller->distance_between_wheels, FLOAT_TOLERANCE );

   result = drive_controller->set_distance_between_wheels( -0.3 );
   EXPECT_EQ( 1, result );
   EXPECT_NEAR( 0.2, drive_controller->distance_between_wheels, FLOAT_TOLERANCE );
}

TEST_F( DifferentialDriveControllerTest, setWheelRadius )
{
   int result;
   result = drive_controller->set_wheel_radius( 0.059 );
   EXPECT_EQ( 0, result );
   EXPECT_NEAR( 0.059, drive_controller->wheel_radius, FLOAT_TOLERANCE );

   result = drive_controller->set_wheel_radius( 0.083 );
   EXPECT_EQ( 0, result );
   EXPECT_NEAR( 0.083, drive_controller->wheel_radius, FLOAT_TOLERANCE );

   result = drive_controller->set_wheel_radius( -0.075 );
   EXPECT_EQ( 1, result );
   EXPECT_NEAR( 0.083, drive_controller->wheel_radius, FLOAT_TOLERANCE );
}

TEST_F( DifferentialDriveControllerTest, setWheelRotationRateLimit )
{
   int result;
   result = drive_controller->set_wheel_speed_limit( 7.123 );
   EXPECT_EQ( 0, result );
   EXPECT_NEAR( 7.123, drive_Controller->wheel_speed_limit, FLOAT_TOLERANCE );

   result = drive_controller->set_wheel_speed_limit( 5.234 );
   EXPECT_EQ( 0, result );
   EXPECT_NEAR( 5.234, drive_controller->wheel_speed_limit, FLOAT_TOLERANCE );

   result = drive_controller->set_wheel_speed_limit( -4.987 );
   EXPECT_EQ( 1, result );
   EXPECT_NEAR( 5.234, drive_controller->wheel_speed_limit, FLOAT_TOLERANCE );
}

TEST_F( DifferentialDriveControllerTest, PositiveRangeErrorOnly )
{
   // If there is no heading error, and the distance error is non-zero,
   // then the speeds should be the same, and equal to the wheelRotationRateLimit.

   double distance_err = 1.0;
   double heading_err  = 0.0;
   drive_controller->update( distance_err, heading_err );

   double right_motor_speed_command, left_motor_speed_command;
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );

   // the speeds should be the same
   EXPECT_NEAR( right_motor_speed_command, left_motor_speed_command, FLOAT_TOLERANCE );
   // and equal to the setWheelRotationRateLimit.
   EXPECT_NEAR( right_motor_speed_command, 8.880, FLOAT_TOLERANCE );
}

TEST_F( DifferentialDriveControllerTest, positiveHeadingError )
{
   // If the heading error is positive, then we should turn to the right,
   // meaning that the left wheel should move faster than the right wheel.

   double right_motor_speed_command, left_motor_speed_command;

   drive_controller->update( 0.0, 0.1 * ( PI / 180.0 ) );
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( left_motor_speed_command, right_motor_speed_command );

   drive_controller->update( 50.0, 30 * ( PI / 180.0 ) );
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( left_motor_speed_command, right_motor_speed_command );

   drive_controller->update( 100.0, 60 * ( PI / 180.0 ) );
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( left_motor_speed_command, right_motor_speed_command );

   drive_controller->update( 0.0, 89 * ( PI / 180.0 ) );
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( left_motor_speed_command, right_motor_speed_command );

   drive_controller->update( 0.0, 90 * ( PI / 180.0 ) );
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( left_motor_speed_command, right_motor_speed_command );

   drive_controller->update( 50.0, 91 * ( PI / 180.0 ) );
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( left_motor_speed_command, right_motor_speed_command );

   drive_controller->update( 100.0, 120 * ( PI / 180.0 ) );
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( left_motor_speed_command, right_motor_speed_command );

   drive_controller->update( 0.0, 150 * ( PI / 180.0 ) );
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( left_motor_speed_command, right_motor_speed_command );

   drive_controller->update( 50.0, 179 * ( PI / 180.0 ) );
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( left_motor_speed_command, right_motor_speed_command );
}

TEST_F( DifferentialDriveControllerTest, negativeHeadingError )
{
   // If the heading error is negative, then we should turn to the left,
   // meaning that the right wheel should move faster than the left wheel.

   double distance_err = 0.0;
   double heading_err  = -30 * ( PI / 180.0 );
   drive_controller->update( distance_err, heading_err );

   double right_motor_speed_command, left_motor_speed_command;
   drive_controller->get_commanded_motor_speeds( left_motor_speed_command, right_motor_speed_command );
   EXPECT_GT( right_motor_speed_command, left_motor_speed_command );
}
