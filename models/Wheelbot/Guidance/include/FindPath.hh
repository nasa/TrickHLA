#ifndef FINDPATH_HH
#define FINDPATH_HH

#include "Arena.hh"
#include "GridSquare.hh"
#include "Point.hh"

namespace TrickHLAModel
{

std::vector< Point > FindPath( GridSquare *origin, GridSquare const *goal, Arena *arena );

}
#endif
