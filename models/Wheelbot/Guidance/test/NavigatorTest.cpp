#include <gtest/gtest.h>
#include <math.h>

#define private public

#include "../include/Navigator.hh"
#include "../include/Point.hh"

#define PI 3.14159265358979
#define FP_TOLERANCE 0.000000001

using namespace TrickHLAModel;

TEST( NavigatorTest, distanceTo_one )
{
   Point     location( 0.0, 0.0 );
   double    heading = 0.0;
   Navigator navigator( heading, location );
   Point     map_point( 3.0, 4.0 );
   double    distance = navigator.distance_to( map_point );
   EXPECT_NEAR( distance, 5.0, FP_TOLERANCE );
}

TEST( NavigatorTest, distanceTo_two )
{
   Point     location( 2.0, 2.0 );
   double    heading = 0.0;
   Navigator navigator( heading, location );
   Point     map_point( 5.0, 6.0 );
   double    distance = navigator.distance_to( map_point );
   EXPECT_NEAR( distance, 5.0, FP_TOLERANCE );
}

TEST( NavigatorTest, distanceTo_three )
{
   // Tests if distance is found correctly from a negative location to a positive map_point
   Point     location( -4, -5 );
   Navigator navigator( PI / 6, location );
   Point     map_point( 6, 9 );
   double    distance = navigator.distance_to( map_point );

   EXPECT_NEAR( distance, 17.204650534, FP_TOLERANCE );
}

TEST( NavigatorTest, convertMapToPlatform_one )
{
   // Tests if the map_point gets converted to platform correctly
   Point     location( 5, 4 );
   Navigator navigator( PI / 6, location );
   Point     map_point( 6, 9 );
   Point     platform_point;
   platform_point = navigator.convert_map_to_platform( map_point );

   EXPECT_EQ( platform_point.getX(), 1 );
   EXPECT_EQ( platform_point.getY(), 5 );
}

TEST( NavigatorTest, convertMapToPlatform_two )
{
   // Tests if the map_point gets converted to platform correctly
   // under slightly more strenuous conditions than the previous test
   Point     location( -8, -9 );
   Navigator navigator( 5 * PI / 6, location );
   Point     map_point( 3, -5 );
   Point     platform_point = navigator.convert_map_to_platform( map_point );

   EXPECT_EQ( platform_point.getX(), 11 );
   EXPECT_EQ( platform_point.getY(), 4 );
}

TEST( NavigatorTest, convertPlatformToBody_one )
{
   // If heading is 45 degrees then <1,0> in platform coordinates should be
   // <sqrt(2)/2, sqrt(2)/2> in body coordinates.
   double    heading = 45.0 * ( PI / 180.0 );
   Point     location( 0, 0 );
   Navigator navigator( heading, location );
   Point     platform_point( 1, 0 );
   Point     body_point = navigator.convert_platform_to_body( platform_point );

   double expected_result = sqrt( 2.0 ) / 2.0;
   EXPECT_NEAR( body_point.getX(), expected_result, FP_TOLERANCE );
   EXPECT_NEAR( body_point.getY(), expected_result, FP_TOLERANCE );
}

TEST( NavigatorTest, convertPlatformToBody_two )
{
   // If heading is 45 degrees, then <0,1> in platform coordinates
   // should be <sqrt(2)/2, sqrt(2)/2> in body coordinates.
   double    heading = 45.0 * ( PI / 180.0 );
   Point     location( 0, 0 );
   Navigator navigator( heading, location );
   Point     platform_point( 1, 0 );
   Point     body_point = navigator.convert_platform_to_body( platform_point );

   double expected_result = sqrt( 2.0 ) / 2.0;
   EXPECT_NEAR( body_point.getX(), expected_result, FP_TOLERANCE );
   EXPECT_NEAR( body_point.getY(), expected_result, FP_TOLERANCE );
}

TEST( NavigatorTest, convertBodyToPlatform_one )
{
   // This test is the inverse of convertPlatformToBody_one.
   double    heading = 45.0 * ( PI / 180.0 );
   Point     location( 0, 0 );
   Navigator navigator( heading, location );

   double H = sqrt( 2.0 ) / 2.0;
   Point  body_point( H, H );
   Point  platform_point = navigator.convert_body_to_platform( body_point );

   EXPECT_NEAR( platform_point.getX(), 1.0, 00001 );
   EXPECT_NEAR( platform_point.getY(), 0.0, 00001 );
}

TEST( NavigatorTest, convertPlatformToMap_one )
{
   // Tests if Platform points get converted to map_points correctly
   Point     location( -8, -9 );
   Navigator navigator( PI / 6, location );
   Point     platform_point( 11, 4 );
   Point     map_point;
   map_point = navigator.convert_platform_to_map( platform_point );

   EXPECT_EQ( map_point.getX(), 3 );
   EXPECT_EQ( map_point.getY(), -5 );
}

TEST( NavigatorTest, bearingTo_one )
{
   Point     location( 0, 0 );
   Navigator navigator( PI / 6, location );
   Point     map_point( 3, 0 );
   double    bearing;
   bearing = navigator.bearing_to( map_point );

   EXPECT_NEAR( bearing, ( -PI / 6 ), FP_TOLERANCE );
}

TEST( NavigatorTest, bearingTo_two )
{
   Point     location( 20, 0 );
   Navigator navigator( 0.0, location );
   Point     map_point( 20, 20 );
   double    bearing;
   bearing = navigator.bearing_to( map_point );
   std::cout << "bearing = " << bearing << std::endl;
   EXPECT_NEAR( bearing, ( -PI / 6 ), FP_TOLERANCE );
}
