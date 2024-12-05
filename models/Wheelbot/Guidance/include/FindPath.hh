#ifndef FINDPATH_HH
#define FINDPATH_HH

#include "Arena.hh"
#include "GridSquare.hh"
#include "Point.hh"

std::vector< Point > FindPath( GridSquare *origin, GridSquare *goal, Arena *arena );

#endif
