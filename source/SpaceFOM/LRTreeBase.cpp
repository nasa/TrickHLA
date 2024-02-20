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
   : paths( NULL )
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

/*! @brief Add a reference node to the tree.
 *  @details This function is used to add a SpaceFOM reference node into
 *  the reference node tree.
 *  @param node_ptr Pointer to the reference node to add.
 *  @return Success or failure of the add. */
bool LRTreeBase::add_node( LRTreeNodeBase *node_ptr )
{
   if ( node_ptr != NULL ) {

      // Make sure that the node is not already in the tree.
      if ( has_node( node_ptr ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
            send_hs( stdout, "LRTreeBase::add_node():%d WARNING: Node \'%s\' is already in the tree.\n",
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

      // The node ID for this node is now the node's position in the vector.
      node_ptr->node_id = nodes.size();

      // Push the node on the end of the vector.
      nodes.push_back( node_ptr );

      // Place the node in the map with the name as the key.
      node_map.insert( pair< string, LRTreeNodeBase * >( node_ptr->name, node_ptr ) );

      return ( true );
   }
   return ( false );
}

bool LRTreeBase::build_tree()
{

   // Start by allocating the paths matrix for the tree.
   if ( !allocate_paths() ) {
      ostringstream errmsg;
      errmsg << "LRTreeBase::build_tree():" << __LINE__
             << " ERROR: Failed to allocate the tree paths matrix!\"" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return ( true );
}

bool LRTreeBase::check_tree()
{
   return ( true );
}

bool LRTreeBase::has_node( unsigned int node_id )
{
   return ( node_id < this->nodes.size() );
}

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

bool LRTreeBase::has_node( string const &name )
{
   // Returns true if the node was found, false otherwise.
   return ( node_map.find( name ) != node_map.end() );
}

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

LRTreeNodeBase *LRTreeBase::find_node( unsigned int node_id )
{
   return ( this->nodes[node_id] );
}

LRTreeNodeBase *LRTreeBase::find_node( char const *name )
{
   if ( name == NULL ) {
      send_hs( stdout, "LRTreeBase::has_node():%d ERROR: NULL node name.\n",
               __LINE__, THLA_NEWLINE );
      return ( NULL );
   }
   return ( find_node( string( name ) ) );
}

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

LRTreeNodeBase *LRTreeBase::find_common_base(
   char const *node_1,
   char const *node_2 )
{
   return ( NULL );
}

LRTreeNodeBase *LRTreeBase::find_common_base(
   string const &node_1,
   string const &node_2 )
{
   return ( NULL );
}

LRTreeNodeBase *LRTreeBase::find_common_base(
   LRTreeNodeBase const *node_1,
   LRTreeNodeBase const *node_2 )
{
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

   int num_nodes;

   //Check to see if a paths matrix has been allocated.
   if ( paths != NULL ) {

      // Get the size of the path matrix.
      num_nodes = this->nodes.size();

      // Iterate through the rows.
      for ( int iinc = 0 ; iinc < num_nodes ; iinc++ ) {

         // Check that the columns have be allocated.
         if ( paths[iinc] != NULL ) {

            // Iterate through the columns.
            for ( int jinc = 0 ; jinc < num_nodes ; jinc++ ) {

               // Loop through the node path vector.
               for ( int kinc = 0 ; kinc < nodes.size() ; kinc++ ){
                  stream << nodes[ kinc ]->node_id;
               }

            } // End column iteration.

         }

      } // End row iteration.

   }

   return;
}

/*!
 * @job_class{shutdown}
 */
bool LRTreeBase::allocate_paths()
{
   int num_nodes;

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
