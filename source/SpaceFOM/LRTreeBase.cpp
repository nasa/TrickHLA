/*!
@file SpaceFOM/LRTreeBase.cpp
@ingroup SpaceFOM
@brief This class provides an reference implementation for a specialized
form of a Directed Acyclic Graph (DAG) called a Labeled Rooted Tree.

This is the base implementation for specialized Directed Acyclic Graph (DAG)
referred to as a Labeled Rooted Tree (LRTree).  In addition to the acyclic
characteristic of a DAG, the LRTree has a single defined root/top/bottom node
that defines the 'start' of the tree.  The root node will not have a parent
node; all other nodes will reference a single parent node that must also be in
the LRTree.  The resulting hierarchical tree will insure that one and only
one path exists through the LRTree connecting any two nodes.

This construct will be use in the SpaceFOM as the basis for the Reference
node Tree.  The paths between nodes will provide the information necessary
to compute transformations between nodes.

Note: this implementation does NOT manage node memory allocation.

@copyright Copyright 2024 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{LRTreeNodeBase.cpp}
@trick_link_dependency{LRTreeBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, February 2024, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA model include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"

// SpaceFOM include files.
#include "SpaceFOM/LRTreeBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
LRTreeBase::LRTreeBase()
   : debug( false ),
     nodes(),
     node_map(),
     root_node_id( 0 ),
     root_node_ptr( NULL ),
     paths( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
LRTreeBase::~LRTreeBase()
{
   // Free the paths matrix.
   this->free_paths();

   return;
}

/*!
 * @job_class{initialize}
 */
bool LRTreeBase::add_node( LRTreeNodeBase *node_ptr )
{
   // Check to see if the node pointer is NULL.
   if ( node_ptr == NULL ) {
      return ( false );
   }

   // Make sure that the node is not already in the tree.
   if ( this->has_node( node_ptr ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         send_hs( stdout, "LRTreeBase::add_node():%d ERROR: Node \'%s\' is already in the tree.\n",
                  __LINE__, node_ptr->name, THLA_NEWLINE );
      }
      return ( false );
   }

   // Check for NULL node name.  The node must have a name.
   if ( node_ptr->name == NULL ) {
      send_hs( stdout, "LRTreeBase::add_node():%d ERROR: NULL node name.\n",
               __LINE__, THLA_NEWLINE );
      return ( false );
   }

   // Make sure that the node name is unique.
   if ( this->has_node( node_ptr->name ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         send_hs( stdout, "LRTreeBase::add_node():%d ERROR: Node \'%s\' duplicate name is already in the tree.\n",
                  __LINE__, node_ptr->name, THLA_NEWLINE );
      }
      return ( false );
   }

   // The node ID for this node is now the node's position in the vector.
   node_ptr->node_id = nodes.size();

   // Push the node on the end of the vector.
   nodes.push_back( node_ptr );

   // Place the node in the map with the name as the key.
   node_map.insert( pair< string, LRTreeNodeBase * >( node_ptr->name, node_ptr ) );

   return ( true );
}

/*!
 * @job_class{initialize}
 */
bool LRTreeBase::build_tree()
{
   // Start by allocating the paths matrix for the tree.
   if ( !allocate_paths() ) {
      ostringstream errmsg;
      errmsg << "LRTreeBase::build_tree():" << __LINE__
             << " ERROR: Failed to allocate the tree paths matrix!\"" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Find the root node for this tree.
   LRTreeNodeBase const *root_node = this->find_root();
   if ( root_node == NULL ) {
      ostringstream errmsg;
      errmsg << "LRTreeBase::build_tree():" << __LINE__
             << " ERROR: Root node not found!\"" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Sweep through the nodes vector to populate the paths matrix.
   for ( unsigned int iinc = 0; iinc < this->nodes.size(); ++iinc ) {

      for ( unsigned int jinc = 0; jinc <= iinc; ++jinc ) {
         // Any diagonal elements just have themselves.
         if ( jinc == iinc ) {

            paths[iinc][jinc].push_back( this->nodes[iinc] );

         } else { // Fill in off-diaganol paths.

            // Find the path between iinc and jinc.
            LRTreeNodeVector *path = find_path( iinc, jinc );

            // Check to make sure that we found a path.
            if ( path != NULL ) {

               // Iterate through the path to build out the matrix.
               // Note that the upper triangle of matrix is the reverse of the lower.
               LRTreeNodeVector::iterator path_iter;
               for ( path_iter = path->begin(); path_iter < path->end(); ++path_iter ) {
                  // Set the lower diaganol values.
                  paths[iinc][jinc].push_back( *path_iter );
                  // Set the upper diaganol values.
                  paths[jinc][iinc].insert( paths[jinc][iinc].begin(), *path_iter );
               }

               // Clean up the path vector.
               path->clear();
               free( path );
               path = NULL;
            }
         }
      }
   }

   return ( true );
}

/*!
 * @job_class{initialize}
 */
bool LRTreeBase::check_tree()
{
   bool found_root  = false;
   bool check_state = true;

   // Iterate through all the nodes.
   for ( unsigned int iinc = 0; iinc < this->nodes.size(); ++iinc ) {

      // Get the reference to the node in the vector.
      LRTreeNodeBase *node_ptr = this->nodes[iinc];

      // Check for node ID consistency.
      if ( node_ptr->node_id != iinc ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
            send_hs( stdout, "LRTreeBase::check_tree():%d ERROR: Node ID (%d) mismatch with index [%d]: \'%s\'!\n",
                     __LINE__, iinc, node_ptr->node_id, node_ptr->name, THLA_NEWLINE );
         }
         // Mark check state false since node ID does not match index.
         check_state = false;
      }

      // Check for an unparented root node.
      if ( node_ptr->parent == NULL ) {

         // Check is this is marked as a root node.
         if ( !( node_ptr->is_root_node ) ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
               send_hs( stdout, "LRTreeBase::check_tree():%d ERROR: Root node not marked as root node: \'%s\'!\n",
                        __LINE__, node_ptr->name, THLA_NEWLINE );
            }
            // Mark check state false since this root node is not marked as a root node.
            check_state = false;
         }

         // Check for a single unparented root node.
         if ( found_root ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
               send_hs( stdout, "LRTreeBase::check_tree():%d ERROR: More than one root node found: \'%s\'!\n",
                        __LINE__, node_ptr->name, THLA_NEWLINE );
            }
            // Mark check state false since we already found a root node.
            check_state = false;
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
               send_hs( stdout, "LRTreeBase::check_tree():%d INFO: Root node found: \'%s\'!\n",
                        __LINE__, node_ptr->name, THLA_NEWLINE );
            }
            found_root = true;
         }

      } else { // Not a root node.

         // Make sure that the parent node exists within the tree.
         if ( this->has_node( node_ptr->parent ) ) {
            if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
               send_hs( stdout, "LRTreeBase::check_tree():%d INFO: Parent \'%s\' found for node \'%s\'!\n",
                        __LINE__, node_ptr->parent->name, node_ptr->name, THLA_NEWLINE );
            }
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
               send_hs( stdout, "LRTreeBase::check_tree():%d ERROR: Parent \'%s\' not found for node \'%s\'!\n",
                        __LINE__, node_ptr->parent->name, node_ptr->name, THLA_NEWLINE );
            }
            // Mark check state false since parent node is not in tree.
            check_state = false;
         }

         // Make sure this node is NOT marked as a root node.
         if ( node_ptr->is_root_node ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
               send_hs( stdout, "LRTreeBase::check_tree():%d ERROR: Branch node marked as root node: \'%s\'!\n",
                        __LINE__, node_ptr->name, THLA_NEWLINE );
            }
            // Mark check state false since branch node marked as root node.
            check_state = false;
         }

         // Need to check to make sure that tree is acyclic.
         if ( this->is_cyclic( node_ptr ) ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
               send_hs( stdout, "LRTreeBase::check_tree():%d ERROR: Branch node is part of a cyclic segment: \'%s\'!\n",
                        __LINE__, node_ptr->name, THLA_NEWLINE );
            }
            // Mark check state false since branch node marked as root node.
            check_state = false;
         }
      }
   }

   // Return false if no root node found.
   if ( !found_root ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         send_hs( stdout, "LRTreeBase::check_tree():%d ERROR: No root node found!\n",
                  __LINE__, THLA_NEWLINE );
      }
      // Mark check state false since no root node was found.
      check_state = false;
   }

   return ( check_state );
}

/*!
 * @job_class{scheduled}
 */
bool LRTreeBase::has_node( unsigned int const node_id )
{
   return ( node_id < this->nodes.size() );
}

/*!
 * @job_class{scheduled}
 */
bool LRTreeBase::has_node( char const *name )
{
   if ( name == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         send_hs( stdout, "LRTreeBase::has_node():%d ERROR: NULL node name.\n",
                  __LINE__, THLA_NEWLINE );
      }
      return ( false );
   }
   return ( has_node( string( name ) ) );
}

/*!
 * @job_class{scheduled}
 */
bool LRTreeBase::has_node( string const &name )
{
   // Returns true if the node was found, false otherwise.
   return ( node_map.find( name ) != node_map.end() );
}

/*!
 * @job_class{scheduled}
 */
bool LRTreeBase::has_node( LRTreeNodeBase const *node )
{
   vector< LRTreeNodeBase * >::iterator node_iter;

   // Find the node in the vector.
   for ( node_iter = nodes.begin(); node_iter < nodes.end(); ++node_iter ) {
      if ( *node_iter == node ) {
         return ( true );
      }
   }

   return ( false );
}

/*!
 * @job_class{scheduled}
 */
LRTreeNodeBase *LRTreeBase::find_node( unsigned int const node_id )
{
   return ( this->nodes[node_id] );
}

/*!
 * @job_class{scheduled}
 */
LRTreeNodeBase *LRTreeBase::find_node( char const *name )
{
   if ( name == NULL ) {
      send_hs( stdout, "LRTreeBase::has_node():%d ERROR: NULL node name.\n",
               __LINE__, THLA_NEWLINE );
      return ( NULL );
   }
   return ( find_node( string( name ) ) );
}

/*!
 * @job_class{scheduled}
 */
LRTreeNodeBase *LRTreeBase::find_node( string const &name )
{
   map< string, LRTreeNodeBase * >::iterator map_iter;

   // Find the node in the map.
   map_iter = node_map.find( name );
   if ( map_iter != node_map.end() ) {
      // The node was found; so, return it.
      return ( map_iter->second );
   }

   // The node was not found; so, return NULL.
   return ( NULL );
}

/*!
 * @job_class{scheduled}
 */
void LRTreeBase::print_nodes( std::ostream &stream )
{
   for ( int iinc = 0; iinc < nodes.size(); ++iinc ) {
      nodes[iinc]->print_node( stream );
   }
   return;
}

/*!
 * @job_class{scheduled}
 */
void LRTreeBase::print_paths( std::ostream &stream )
{
   // Check to see if a paths matrix has been allocated.
   if ( paths != NULL ) {

      // Get the size of the path matrix.
      int num_nodes = this->nodes.size();

      // Print out header tag.
      stream << "LRTreeBase::print_paths: " << THLA_NEWLINE;

      // Iterate through the rows.
      for ( int iinc = 0; iinc < num_nodes; ++iinc ) {

         // Check that the columns have be allocated.
         if ( paths[iinc] != NULL ) {

            // Iterate through the columns.
            for ( int jinc = 0; jinc < num_nodes; ++jinc ) {

               unsigned int path_size = paths[iinc][jinc].size();

               stream << "paths[" << iinc << "][" << jinc << "]: ";

               if ( path_size > 0 ) {
                  // Loop through the node path vector.
                  for ( int kinc = 0; kinc < path_size; ++kinc ) {
                     stream << paths[iinc][jinc][kinc]->node_id;
                     if ( kinc < path_size - 1 ) {
                        stream << ", ";
                     }
                  }
               } else {
                  stream << "None";
               }
               stream << THLA_NEWLINE;

            } // End column iteration.
         }

      } // End row iteration.
   } else {
      send_hs( stdout, "LRTreeBase::print_paths():%d Warning: No path matrix allocated.\n",
               __LINE__, THLA_NEWLINE );
   }

   return;
}

/*!
 * @job_class{initialization}
 */
bool LRTreeBase::allocate_paths()
{
   int num_nodes;

   // Free any old paths before allocating new paths.
   this->free_paths();

   // Size the path matrix.
   num_nodes = this->nodes.size();

   // Allocate the rows of the matrix.
   this->paths = new LRTreeNodeVector *[num_nodes];

   // Check for successful allocation.
   if ( this->paths == NULL ) {

      // Allocation failed.
      send_hs( stdout, "LRTreeBase::allocate_paths():%d Error: Failed allocation of paths matrix rows.\n",
               __LINE__, THLA_NEWLINE );
      return ( false );

   } else { // Row allocation succeeded.

      // Allocate the columns of the path matrix.
      // Note that this is always a square matrix.
      for ( int iinc = 0; iinc < num_nodes; ++iinc ) {

         // Allocate the columns of the matrix.
         this->paths[iinc] = new LRTreeNodeVector[num_nodes];

         // Check for column allocation failure.
         if ( this->paths[iinc] == NULL ) {
            send_hs( stdout, "LRTreeBase::allocate_paths():%d Error: Failed allocation of paths matrix columns for row %d.\n",
                     __LINE__, iinc, THLA_NEWLINE );
            return ( false );
         }
      }
   }

   return ( true );
}

/*!
 * @job_class{shutdown}
 */
void LRTreeBase::free_paths()
{
   // Check to see if a paths matrix has been allocated.
   if ( paths != NULL ) {

      // Size the path matrix.
      int num_nodes = this->nodes.size();

      // Iterate through and free the path matrix.
      // Iterate through the rows.
      for ( int iinc = 0; iinc < num_nodes; ++iinc ) {

         // Check that the columns have been allocated.
         if ( paths[iinc] != NULL ) {

            for ( int jinc = 0; jinc < num_nodes; ++jinc ) {

               // Clear the path vector.
               paths[iinc][jinc].clear();

            } // End column iteration.

            // Free the allocated path column.
            // Delete the memory.
            delete[] this->paths[iinc];
            // Set the path column to NULL.
            this->paths[iinc] = NULL;
         }

      } // End row iteration.

      // Free the allocated path rows.
      delete[] paths;

      // Mark as NULL.
      paths = NULL;
   }
   return;
}

/*
 * @job_class{scheduled}
 */
LRTreeNodeBase *LRTreeBase::find_root()
{
   unsigned int    num_nodes;
   LRTreeNodeBase *node_ptr;
   LRTreeNodeBase *root_node;
   unsigned int    root_id;
   bool            found_root = false;

   // Get the number of nodes in the tree.
   num_nodes = this->nodes.size();

   // Iterate through all the nodes.
   // for ( node_iter = nodes.begin(); node_iter < nodes.end(); ++node_iter ) {
   for ( unsigned int iinc = 0; iinc < num_nodes; ++iinc ) {

      // Get the reference to the node in the vector.
      node_ptr = this->nodes[iinc];

      // Check for an unparented root node.
      if ( node_ptr->parent == NULL ) {
         // Check for a single unparented root node.
         if ( found_root ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
               send_hs( stdout, "LRTreeBase::find_root():%d ERROR: More than one root node found: \'%s\'!\n",
                        __LINE__, node_ptr->name, THLA_NEWLINE );
            }
            // Return NULL since we already found a root node.
            return ( NULL );
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
               send_hs( stdout, "LRTreeBase::find_root():%d INFO: Root node found: \'%s\'!\n",
                        __LINE__, node_ptr->name, THLA_NEWLINE );
            }
            found_root = true;
            root_node  = node_ptr;
            root_id    = iinc;
         }
      }
   }

   // Return NULL if no root node found.
   if ( !found_root ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         send_hs( stdout, "LRTreeBase::find_root():%d ERROR: No root node found!\n",
                  __LINE__, THLA_NEWLINE );
      }
      return ( NULL );
   }

   // Set the root information.
   root_node->is_root_node = true;
   this->root_node_ptr     = root_node;
   this->root_node_id      = root_id;

   // Return the pointer to the root noode.
   return ( root_node );
}

/*
 * @job_class{scheduled}
 */
bool LRTreeBase::is_cyclic( LRTreeNodeBase const *node )
{
   unsigned int            level = 0;
   unsigned int            num_nodes;
   bool                    not_acyclic = false;
   LRTreeNodeSet           path_set;
   LRTreeNodeSet::iterator path_pos;
   const LRTreeNodeBase   *current_node = node;

   // Get the number of nodes in the tree.
   num_nodes = this->nodes.size();

   while ( level < num_nodes ) {

      // Check if the current node is in the path set.
      // If so, there is a cyclic relationship.
      path_pos = path_set.find( current_node );
      if ( path_pos == path_set.end() ) {
         not_acyclic = true;
         break;
      }

      // Add this node to the path set.
      path_set.insert( current_node );

      // Check to see if the parent is NULL.
      // If so, this is a root node and we are done.
      if ( node->parent != NULL ) {
         path_set.insert( node->parent );
      } else {
         break;
      }

      // Increment the path level.
      level++;
   }

   // Check to see if we ran off the end of the node list.
   if ( level >= num_nodes ) {
      not_acyclic = true;
   }

   // Clear the path_set before leaving.
   path_set.clear();

   return ( not_acyclic );
}

/*
 * @job_class{scheduled}
 */
LRTreeNodeVector *LRTreeBase::get_path_to_root( unsigned int const node_id )
{
   LRTreeNodeBase   *node_ptr;
   LRTreeNodeVector *return_vector = NULL;

   // Make sure the node is in the tree.
   node_ptr = this->find_node( node_id );
   if ( node_ptr != NULL ) {

      // Allocate the return node path vector;
      return_vector = new LRTreeNodeVector;

      // Climb the tree up to the root.
      while ( node_ptr != NULL ) {

         // Push the node onto the path vector.
         return_vector->push_back( node_ptr );

         // Move up the tree.
         node_ptr = node_ptr->parent;
      }
   }

   return ( return_vector );
}

/*
 * @job_class{scheduled}
 */
LRTreeNodeVector *LRTreeBase::get_path_to_root( LRTreeNodeBase const *node )
{

   // Protect against NULL pointers.
   if ( node == NULL ) {
      send_hs( stdout, "LRTreeBase::get_path_to_root():%d Error: NULL node pointer.\n",
               __LINE__, THLA_NEWLINE );
      return ( NULL );
   }

   // Call the indexed routine.
   return ( get_path_to_root( node->node_id ) );
}

/*!
 * @job_class{scheduled}
 */
LRTreeNodeVector *LRTreeBase::find_path( unsigned int const local,
                                         unsigned int const wrt )
{
   LRTreeNodeVector *up_path     = NULL;
   LRTreeNodeVector *down_path   = NULL;
   LRTreeNodeVector *return_path = NULL;

   LRTreeNodeVector::iterator up_itr;
   LRTreeNodeVector::iterator down_itr;

   // Make sure these nodes are in the tree.
   if ( this->has_node( local ) && this->has_node( wrt ) ) {

      // Get path from local node to root.
      up_path = this->get_path_to_root( local );

      // Get path from wrt node to root.
      down_path = this->get_path_to_root( wrt );

      // Check to find the common path.
      if ( ( up_path != NULL ) && ( down_path != NULL ) ) {

         // Find the common node between two paths.
         LRTreeNodeBase const *common_node = find_common_node( up_path, down_path );

         // Make sure that we found a common node.
         if ( common_node != NULL ) {

            // We found the up and down paths and the common node.
            // So, now build the path.

            // First allocate a return path vector.
            return_path = new LRTreeNodeVector;

            // Start moving up the tree from the local node.
            for ( up_itr = up_path->begin(); up_itr != up_path->end(); ++up_itr ) {
               // Add the up path to the return path up to and including the common node.
               return_path->push_back( *up_itr );
               if ( *up_itr == common_node ) {
                  break;
               }
            }

            // Find the common node position in the down path.
            for ( down_itr = down_path->begin(); down_itr != down_path->end(); ++down_itr ) {
               if ( *down_itr == common_node ) {
                  break;
               }
            }

            // Check for an aberrant condition where the common node isn't found.
            // This should NEVER happen!
            if ( down_itr == down_path->end() ) {
               send_hs( stdout, "LRTreeBase::find_path():%d Error: Failed to find common node in down path search.\n",
                        __LINE__, THLA_NEWLINE );
               return_path->clear();
               free( return_path );
               return_path = NULL;
            } else { // Everything looks to be okay.

               // Now work backwards from the current common node down to the wrt node.
               while ( down_itr != down_path->begin() ) {
                  --down_itr;
                  return_path->push_back( *down_itr );
               }
            }
         }
      }

      // Clear and free the path sets.
      if ( up_path != NULL ) {
         up_path->clear();
         free( up_path );
         up_path = NULL;
      }
      if ( down_path != NULL ) {
         down_path->clear();
         free( down_path );
         down_path = NULL;
      }
   }

   // Return the filled in path set.
   return ( return_path );
}

/*!
 * @job_class{scheduled}
 */
LRTreeNodeVector *LRTreeBase::find_path( LRTreeNodeBase const *local,
                                         LRTreeNodeBase const *wrt )
{

   // Protect against NULL pointers.
   if ( ( local == NULL ) || ( wrt == NULL ) ) {
      if ( local == NULL ) {
         send_hs( stdout, "LRTreeBase::find_path():%d Error: NULL \'local\' pointer.\n",
                  __LINE__, THLA_NEWLINE );
      }
      if ( wrt == NULL ) {
         send_hs( stdout, "LRTreeBase::find_path():%d Error: NULL \'wrt\' pointer.\n",
                  __LINE__, THLA_NEWLINE );
      }
      return ( NULL );
   }

   // Call the indexed routine.
   return ( find_path( local->node_id, wrt->node_id ) );
}

/*!
 * @job_class{scheduled}
 */
LRTreeNodeBase *LRTreeBase::find_common_node( unsigned int const local,
                                              unsigned int const wrt )
{
   LRTreeNodeVector *up_path         = NULL;
   LRTreeNodeVector *down_path       = NULL;
   LRTreeNodeBase   *common_node_ptr = NULL;

   // Make sure these nodes are in the tree.
   if ( this->has_node( local ) && this->has_node( wrt ) ) {

      // Get path from local node to root.
      up_path = this->get_path_to_root( local );

      // Get path from wrt node to root.
      down_path = this->get_path_to_root( wrt );

      // Now find the common node.
      common_node_ptr = find_common_node( up_path, down_path );
   }

   return ( common_node_ptr );
}

/*!
 * @job_class{scheduled}
 */
LRTreeNodeBase *LRTreeBase::find_common_node( LRTreeNodeVector *up_path,
                                              LRTreeNodeVector *down_path )
{
   LRTreeNodeVector::iterator up_iter;
   LRTreeNodeVector::iterator down_iter;
   LRTreeNodeBase            *common_node_ptr = NULL;

   // Protect against NULL pointers.
   if ( ( up_path == NULL ) || ( down_path == NULL ) ) {
      if ( up_path == NULL ) {
         send_hs( stdout, "LRTreeBase::find_common_node():%d Error: NULL \'up_path\' pointer.\n",
                  __LINE__, THLA_NEWLINE );
      }
      if ( down_path == NULL ) {
         send_hs( stdout, "LRTreeBase::find_common_node():%d Error: NULL \'down_path\' pointer.\n",
                  __LINE__, THLA_NEWLINE );
      }
      return ( NULL );
   }

   // Climb up the paths looking for a common node.
   for ( up_iter = up_path->begin(); up_iter != up_path->end(); ++up_iter ) {
      // See if we can find this node in the down_set.
      // The first one that we find should be the closest common node.
      for ( down_iter = down_path->begin(); down_iter != down_path->end(); ++down_iter ) {
         if ( *down_iter == *up_iter ) {
            common_node_ptr = *up_iter;
            break;
         }
      }

      // Exit the iteration loop if a common node was found.
      if ( common_node_ptr != NULL ) {
         break;
      }
   }

   return ( common_node_ptr );
}