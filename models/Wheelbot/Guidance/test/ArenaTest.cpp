#include <algorithm>
#include <gtest/gtest.h>

#define private public

#include "../include/Arena.hh"
#include "../include/GridSquare.hh"
#include "../include/Point.hh"

using namespace TrickHLAModel;

TEST( ArenaTest, one )
{
   // Attempt to create an arena
   Arena *arena;
   arena = new Arena( 5, 5 );
   EXPECT_NE( (void *)0, arena );
}

TEST( ArenaTest, two )
{
   // Make sure that arena size is what we specified.
   Arena arena( 5, 5 );
   EXPECT_EQ( arena.get_width(), 5 );
   EXPECT_EQ( arena.get_height(), 5 );
}

TEST( ArenaTest, three )
{
   // Make sure that arena size is what we specified.
   // Checks that x is width and y is height
   Arena arena( 10, 7 );
   EXPECT_EQ( arena.get_width(), 10 );
   EXPECT_EQ( arena.get_height(), 7 );
}

TEST( ArenaTest, four )
{
   // Make sure that arena size is what we specified.

   unsigned char ARENA_bits[] = {
      0x10, 0x00, 0x86, 0x00, 0xe8, 0x00, 0x28, 0x00, 0xe2, 0x00, 0x02, 0x00 };

   Arena arena( 10, 6, ARENA_bits );
   EXPECT_EQ( arena.get_width(), 10 );
   EXPECT_EQ( arena.get_height(), 6 );
}

TEST( ArenaTest, calcOffset_one )
{
   // Make sure that calcOffset properly adds its value to the variable offset
   Arena  arena( 10, 7 );
   size_t sz;
   arena.calc_offset( 1, 1, sz );
   EXPECT_EQ( sz, (size_t)11 );
}

TEST( ArenaTest, calcOffset_two )
{
   // Checks to make sure calcOffset function returns 1 if the if-statements
   // evaluate to false
   Arena  arena( 10, 7 );
   size_t sz;
   EXPECT_EQ( arena.calc_offset( 15, 12, sz ), 1 );
}

TEST( ArenaTest, calcOffset_three )
{
   // Checks to make sure calcOffset function returns 0 if the if-statements
   // evaluate to true
   Arena  arena( 10, 7 );
   size_t sz;
   EXPECT_EQ( arena.calc_offset( 5, 6, sz ), 0 );
}

TEST( ArenaTest, getGridSquare_one )
{
   // Tests to make sure getGridSquare gets the gridSquare pointer at the location specified
   // Which in this case is the first gridSquare
   Arena       arena( 10, 7 );
   GridSquare *agridsquare = arena.get_grid_square( 0, 0 );
   EXPECT_EQ( agridsquare, arena.grid );
}

TEST( ArenaTest, getGridSquare_two )
{
   // Tests if the gridSquare that getGridSquare picked up is the correct gridSquare
   // by counting the spaces in memory
   Arena       arena( 10, 7 );
   GridSquare *agridsquare = arena.get_grid_square( 2, 3 );
   EXPECT_EQ( agridsquare, arena.grid + 32 );
}

TEST( ArenaTest, getGridSquare_three )
{
   // failure case for if-statement within getGridSquare
   Arena arena( 10, 7 );
   EXPECT_EQ( arena.get_grid_square( 50, 70 ), ( (GridSquare *)0 ) );
}

TEST( ArenaTest, calcOffset2_one )
{
   // Checks to make sure calcOffset function returns 1 if the if-statements
   // evaluate to false
   // failure case for first if-statement in calcOffset function
   Arena       arena( 10, 7 );
   size_t      sz;
   GridSquare *gridSquare = (GridSquare *)( arena.grid - 1 );
   EXPECT_EQ( arena.calc_offset( gridSquare, sz ), 1 );
}

TEST( ArenaTest, calcOffset2_two )
{
   // Tests if calcOffset function returns 0 when all if statement conditions are met
   Arena       arena( 10, 7 );
   GridSquare *agridSquare = arena.get_grid_square( 2, 3 );
   size_t      sz;
   EXPECT_EQ( arena.calc_offset( agridSquare, sz ), 0 );
}

TEST( ArenaTest, calcOffset2_three )
{
   // Tests if offset value is calculated correctly and put in the correct memory location
   Arena       arena( 10, 7 );
   GridSquare *agridSquare = arena.get_grid_square( 9, 6 );
   size_t      sz;
   arena.calc_offset( agridSquare, sz );
   EXPECT_EQ( sz, 69 );
}

TEST( ArenaTest, calcOffset2_four )
{
   // failure case for second if-statement in calcOffset function
   Arena       arena( 10, 7 );
   GridSquare *agridSquare = arena.get_grid_square( 15, 10 );
   size_t      sz;
   EXPECT_EQ( arena.calc_offset( agridSquare, sz ), 1 );
}

TEST( ArenaTest, getGridSquareCoordinates_one )
{
   // failure case for if-statement in getGridSquareCoordinates function
   Arena       arena( 10, 7 );
   GridSquare *agridSquare = arena.get_grid_square( 15, 10 );
   Point       coordinate;
   std::cout << std::endl;
   std::cout << "The following error message is expected from this test." << std::endl;
   EXPECT_EQ( arena.get_grid_square_coordinates( agridSquare, coordinate ), 1 );
   std::cout << std::endl;
}

TEST( ArenaTest, getGridSquareCoordinates_two )
{
   // Tests if the if-statement evaluates to true that the remainder of code (for that function)
   // runs through
   Arena       arena( 10, 7 );
   GridSquare *agridSquare = arena.get_grid_square( 9, 6 );
   Point       coordinate;
   EXPECT_EQ( arena.get_grid_square_coordinates( agridSquare, coordinate ), 0 );
}

TEST( ArenaTest, getGridSquareCoordinates_three )
{
   // Tests the arithmetic within the getGridSquareCooordinates function
   // once the first if-statement evaluates to true
   Arena       arena( 10, 7 );
   GridSquare *agridSquare = arena.get_grid_square( 9, 6 );
   Point       coordinate;
   arena.get_grid_square_coordinates( agridSquare, coordinate );
   EXPECT_EQ( coordinate.getX(), 9 );
   EXPECT_EQ( coordinate.getY(), 6 );
}

TEST( ArenaTest, movementCostEstimate_one )
{
   // Failure case for the if-statement
   // Ensures that the movement_cost_estimate function fails appropiately
   Arena       arena( 10, 7 );
   GridSquare *agridSquare       = arena.get_grid_square( 11, 3 );
   GridSquare *anothergridSquare = arena.get_grid_square( 12, 4 );
   int         cost_estimate;
   std::cout << std::endl;
   std::cout << "The following error messages are expected from this test." << std::endl;
   EXPECT_EQ( arena.movement_cost_estimate( agridSquare, anothergridSquare, cost_estimate ), 1 );
   std::cout << std::endl;
}

TEST( ArenaTest, movementCostEstimate_two )
{
   // Tests the case that the if-statement evaluates to true and if the following lines of code
   // get executed in their entirety
   Arena       arena( 10, 7 );
   GridSquare *agridSquare       = arena.get_grid_square( 1, 2 );
   GridSquare *anothergridSquare = arena.get_grid_square( 3, 4 );
   int         cost_estimate;
   EXPECT_EQ( arena.movement_cost_estimate( agridSquare, anothergridSquare, cost_estimate ), 0 );
}

TEST( ArenaTest, movementCostEstimate_three )
{
   // Tests to ensure that movementCostEstimate function is calculating the movement cost correctly
   Arena       arena( 10, 7 );
   GridSquare *agridSquare       = arena.get_grid_square( 1, 2 );
   GridSquare *anothergridSquare = arena.get_grid_square( 3, 4 );
   int         cost_estimate;
   arena.movement_cost_estimate( agridSquare, anothergridSquare, cost_estimate );
   EXPECT_EQ( cost_estimate, 40 );
}

TEST( ArenaTest, distanceBetween_one )
{
   // Failure case for the if-statement
   // Ensures that the distanceBetween function fails appropiately
   Arena       arena( 10, 7 );
   GridSquare *agridSquare       = arena.get_grid_square( 11, 3 );
   GridSquare *anothergridSquare = arena.get_grid_square( 12, 4 );
   int         dist;
   std::cout << std::endl;
   std::cout << "The following error messages are expected from this test." << std::endl;
   EXPECT_EQ( arena.distance_between( agridSquare, anothergridSquare, dist ), 1 );
   std::cout << std::endl;
}

TEST( ArenaTest, distanceBetween_two )
{
   // Tests the case that the if-statement evaluates to true and if the following lines of code
   // get executed in their entirety
   Arena       arena( 10, 7 );
   GridSquare *agridSquare       = arena.get_grid_square( 1, 2 );
   GridSquare *anothergridSquare = arena.get_grid_square( 3, 4 );
   int         dist;
   EXPECT_EQ( arena.distance_between( agridSquare, anothergridSquare, dist ), 0 );
}

TEST( ArenaTest, distanceBetween_three )
{
   // Tests that the distanceBetween function properly calculates the distance between two gridSquares
   // using gridSquare pointers
   Arena       arena( 10, 7 );
   GridSquare *agridSquare       = arena.get_grid_square( 1, 2 );
   GridSquare *anothergridSquare = arena.get_grid_square( 3, 4 );
   int         dist;
   arena.distance_between( agridSquare, anothergridSquare, dist );
   EXPECT_EQ( dist, 28 );
}

TEST( ArenaTest, blockunblock_one )
{
   // Tests to ensure that the block and unblock functions change the isBlocked member
   // respective to their names
   Arena       arena( 10, 7 );
   GridSquare *agridSquare = arena.get_grid_square( 1, 2 );
   arena.block( 1, 2 );
   EXPECT_EQ( agridSquare->is_blocked, true );
   arena.unblock( 1, 2 );
   EXPECT_EQ( agridSquare->is_blocked, false );
}

TEST( ArenaTest, mark_one )
{
   // Tests to make sure that the mark function places a mark in the gridSquare desired
   Arena       arena( 10, 7 );
   GridSquare *agridSquare = arena.get_grid_square( 1, 2 );
   arena.mark( 1, 2, 'c' );
   EXPECT_EQ( agridSquare->mark, 'c' );
}

TEST( ArenaTest, getNeighbors_one )
{
   // Tests that error message displays when trying to
   std::cout << std::endl;
   std::cout << "The following error messages are expected from this test." << std::endl;
   Arena                       arena( 10, 7 );
   std::vector< GridSquare * > neighbors;
   GridSquare                 *agridSquare = arena.get_grid_square( 11, 22 );
   neighbors                               = arena.get_neighbors( agridSquare );
   int length                              = neighbors.size();
   std::cout << std::endl;
   std::cout << std::endl;
   EXPECT_EQ( length, 0 );
}

TEST( ArenaTest, getNeighbors_two )
{
   // Tests that getNeighbors returns the correct amount of neighbors within the vector
   Arena                       arena( 3, 3 );
   GridSquare                 *agridSquare = arena.get_grid_square( 1, 1 );
   std::vector< GridSquare * > neighbors;
   neighbors  = arena.get_neighbors( agridSquare );
   int length = neighbors.size();
   EXPECT_EQ( length, 8 );
}

TEST( ArenaTest, getNeighbors_three )
{
   // Tests that getNeighbors returns the correct amount of neighbors when
   // certain neighbors are blocked
   Arena                       arena( 3, 3 );
   GridSquare                 *agridSquare = arena.get_grid_square( 1, 1 );
   std::vector< GridSquare * > neighbors;
   arena.block( 0, 0 );
   arena.block( 2, 0 );
   arena.block( 2, 2 );
   neighbors  = arena.get_neighbors( agridSquare );
   int length = neighbors.size();
   EXPECT_EQ( length, 5 );
}

TEST( ArenaTest, getNeighbors_four )
{
   // Tests that getNeighbors returns the correct GridSquare pointers in the neighbors vector
   Arena                       arena( 3, 3 );
   GridSquare                 *agridSquare = arena.get_grid_square( 1, 1 );
   std::vector< GridSquare * > neighbors;

   arena.block( 0, 0 );
   GridSquare *n0_1 = arena.get_grid_square( 0, 1 ); // 0,1
   GridSquare *n0_2 = arena.get_grid_square( 0, 2 ); // 0,2
   GridSquare *n1_0 = arena.get_grid_square( 1, 0 ); // 1,0
   GridSquare *n1_2 = arena.get_grid_square( 1, 2 ); // 1,2
   arena.block( 2, 0 );
   GridSquare *n2_1 = arena.get_grid_square( 2, 1 ); // 2,1
   arena.block( 2, 2 );

   neighbors = arena.get_neighbors( agridSquare );
   std::vector< GridSquare * >::iterator neighbors_iterator;

   // Test for (0,1) gridSquare
   bool n0_1_found_flag = false;
   neighbors_iterator   = find( neighbors.begin(), neighbors.end(), n0_1 ); // search for neighbor (0,1) in neighbors
   if ( neighbors_iterator != neighbors.end() ) {                           // if the value is found
      n0_1_found_flag = true;                                               // change the found flag to true
   }

   Point point;
   arena.get_grid_square_coordinates( *neighbors_iterator, point );
   std::cout << point.getX() << " " << point.getY() << std::endl;

   EXPECT_EQ( n0_1_found_flag, true );

   // Test for (0,2) gridSquare
   bool n0_2_found_flag = false;
   neighbors_iterator   = find( neighbors.begin(), neighbors.end(), n0_2 ); // search for neighbor (0,2) in neighbors
   if ( neighbors_iterator != neighbors.end() ) {                           // if the value is found
      n0_2_found_flag = true;                                               // change the found flag to true
   }

   arena.get_grid_square_coordinates( *neighbors_iterator, point );
   std::cout << point.getX() << " " << point.getY() << std::endl;

   EXPECT_EQ( n0_2_found_flag, true );

   // Test for (1,0) gridSquare
   bool n1_0_found_flag = false;
   neighbors_iterator   = find( neighbors.begin(), neighbors.end(), n1_0 ); // search for neighbor (0,2) in neighbors
   if ( neighbors_iterator != neighbors.end() ) {                           // if the value is found
      n1_0_found_flag = true;                                               // change the found flag to true
   }

   arena.get_grid_square_coordinates( *neighbors_iterator, point );
   std::cout << point.getX() << " " << point.getY() << std::endl;

   EXPECT_EQ( n1_0_found_flag, true );

   // Test for (1,2) gridSquare
   bool n1_2_found_flag = false;
   neighbors_iterator   = find( neighbors.begin(), neighbors.end(), n1_2 ); // search for neighbor (1,2) in neighbors
   if ( neighbors_iterator != neighbors.end() ) {                           // if the value is found
      n1_2_found_flag = true;                                               // change the found flag to true
   }

   arena.get_grid_square_coordinates( *neighbors_iterator, point );
   std::cout << point.getX() << " " << point.getY() << std::endl;

   EXPECT_EQ( n1_2_found_flag, true );

   // Test for (2,1) gridSquare
   bool n2_1_found_flag = false;
   neighbors_iterator   = find( neighbors.begin(), neighbors.end(), n2_1 ); // search for neighbor (2,1) in neighbors
   if ( neighbors_iterator != neighbors.end() ) {                           // if the value is found
      n2_1_found_flag = true;                                               // change the found flag to true
   }

   arena.get_grid_square_coordinates( *neighbors_iterator, point );
   std::cout << point.getX() << " " << point.getY() << std::endl;

   EXPECT_EQ( n2_1_found_flag, true );
}
