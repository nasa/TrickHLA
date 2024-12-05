#include <algorithm> // std::find
#include <stdlib.h>
#include <vector> // std::vector

#include "../include/Arena.hh"
#include "../include/FindPath.hh"
#include "../include/GridSquare.hh"
#include "../include/Point.hh"

std::vector< Point > FindPath( GridSquare *origin, GridSquare const *goal, Arena *arena )
{
   std::vector< GridSquare * > open_set;
   std::vector< GridSquare * > closed_set;
   std::vector< Point >        failure;

   int estimated_cost_to_goal; // h(n)

   if ( arena == (Arena *)0 ) {
      return failure;
   }
   Point orig_pt, goal_pt;
   if ( arena->get_grid_square_coordinates( origin, orig_pt ) != 0 ) {
      std::cerr << "FindPath: Bad Origin.";
      return failure;
   }
   if ( arena->get_grid_square_coordinates( goal, goal_pt ) != 0 ) {
      std::cerr << "FindPath: Bad Goal.";
      return failure;
   }

   origin->parent  = (GridSquare *)0;
   origin->g_score = 0;

   arena->movement_cost_estimate( origin, goal, estimated_cost_to_goal );
   origin->f_score = estimated_cost_to_goal;

   open_set.push_back( origin );

   while ( !open_set.empty() ) {

      // Get the item in the openSet that has the lowest f_score.
      std::vector< GridSquare * >::iterator curr, best;
      for ( best = curr = open_set.begin(); curr != open_set.end(); ++curr ) {
         if ( ( *curr )->f_score < ( *best )->f_score ) {
            best = curr;
         }
      }

      // Get and remove the best from the open set. This is current.
      GridSquare *current = *best;
      open_set.erase( best );

      // if the current position is the goal then backtrack, build
      // and return the path to get here.
      if ( current == goal ) {
         std::vector< Point > path;
         while ( current != (GridSquare *)0 ) {
            Point coordinates;
            if ( arena->get_grid_square_coordinates( current, coordinates ) == 0 ) {
               path.insert( path.begin(), coordinates );
            }
            current = current->parent;
         }
         return path;
      }

      // Add current to the closed set.
      closed_set.push_back( current );

      // Get the neighbors of current.
      std::vector< GridSquare * > neighbors = arena->get_neighbors( current );

      // For each of the neighbors
      std::vector< GridSquare * >::iterator neighbors_iterator;
      for ( neighbors_iterator = neighbors.begin();
            neighbors_iterator != neighbors.end();
            ++neighbors_iterator ) {

         GridSquare *neighbor = *neighbors_iterator;

         // Search for the neighbor in the closed set.
         std::vector< GridSquare * >::iterator closed_set_iterator =
            find( closed_set.begin(), closed_set.end(), neighbor );

         // if neighbor is not in the closed set.
         if ( closed_set_iterator == closed_set.end() ) {

            // Calculate a g_score for this neighbor.
            int cost_to_move_to_neighbor;
            arena->distance_between( current, neighbor, cost_to_move_to_neighbor );
            int neighbor_g_score = current->g_score + cost_to_move_to_neighbor;

            // Search for the neighbor in the openset.
            std::vector< GridSquare * >::iterator open_set_iterator =
               find( open_set.begin(), open_set.end(), neighbor );

            // if neighbor is in the openset and the tentative g score is better than the current score
            if ( ( open_set_iterator == open_set.end() ) || ( neighbor_g_score < neighbor->g_score ) ) {

               neighbor->parent  = current;
               neighbor->g_score = neighbor_g_score;
               arena->movement_cost_estimate( neighbor, goal, estimated_cost_to_goal );
               neighbor->f_score = neighbor->g_score + estimated_cost_to_goal;

               if ( open_set_iterator == open_set.end() ) {
                  open_set.push_back( neighbor );
               }
            }
         }
      }
   }
   std::cerr << "Failed to find a path to the goal.";
   return failure;
}
