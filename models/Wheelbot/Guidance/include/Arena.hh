/********************************* TRICK HEADER *******************************
PURPOSE: (Arena class)

LIBRARY DEPENDENCY:
    ((Guidance/src/Arena.cpp))
*******************************************************************************/
#ifndef ARENA_H
#define ARENA_H

#include <iostream>
#include <vector>

#include "GridSquare.hh"
#include "Point.hh"

namespace TrickHLAModel
{

class Arena
{
  public:
   Arena( unsigned int width, unsigned int height );
   Arena( unsigned int width, unsigned int height, unsigned char const bits[] );
   virtual ~Arena();

   void block( unsigned int const x, unsigned int const y );

   void unblock( unsigned int const x, unsigned int const y );

   void mark( unsigned int x, unsigned int y, char c );

   std::vector< GridSquare * > get_neighbors( GridSquare const *grid_square_pointer );

   GridSquare *get_grid_square( unsigned int x, unsigned int y );

   int get_grid_square_coordinates( GridSquare const *grid_square_pointer, Point &coords );

   int movement_cost_estimate( GridSquare const *orig, GridSquare const *dest, int &cost );

   int distance_between( GridSquare const *orig, GridSquare const *dest, int &distance );

   int const get_height() { return height; }

   int const get_width() { return width; }

   friend std::ostream &operator<<( std::ostream &s, const Arena &arena );

  public:
   int         height;
   int         width;
   GridSquare *grid;

  private:
   int calc_offset( unsigned int x, unsigned int y, size_t &offset );
   int calc_offset( GridSquare const *grid_square, size_t &offset );
};

} // namespace TrickHLAModel
#endif
