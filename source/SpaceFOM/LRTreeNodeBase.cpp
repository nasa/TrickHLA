/*!
@file SpaceFOM/LRTreeNodeBase.cpp
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

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, February 2024, --, Initial version.}
@revs_end

*/

// System include files.
#include <cstring>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"

/* Global singleton pointer to the memory manager. */
extern Trick::MemoryManager *trick_MM;

// TrickHLA model include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"

// SpaceFOM include files.
#include "SpaceFOM/LRTreeNodeBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
LRTreeNodeBase::LRTreeNodeBase()
   : name( NULL ),
     parent( NULL ),
     debug( false ),
     is_root_node( false ),
     node_id( 0 )
{
   return;
}

/*!
 * @job_class{initialization}
 */
LRTreeNodeBase::LRTreeNodeBase(
   char const     *node_name,
   LRTreeNodeBase *node_parent )
   : name( NULL ),
     parent( NULL ),
     debug( false ),
     is_root_node( false ),
     node_id( 0 )
{
   // Copy the name.
   if ( trick_MM != NULL ) {
      name = trick_MM->mm_strdup( node_name );
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         message_publish( MSG_NORMAL,
                          "mm_strdup called before MemoryManager instantiation. Setting to name to NULL.\n",
                          __LINE__ );
      }
   }

   // Assign the parent node.
   parent = node_parent;

   // Determine root node status.
   if ( node_parent == NULL ) {
      is_root_node = true;
   } else {
      is_root_node = false;
   }

   return;
}

/*!
 * @job_class{shutdown}
 */
LRTreeNodeBase::~LRTreeNodeBase()
{
   // Free the allocated node name.
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::LRTreeNodeBase::~LRTreeNodeBase:%d WARNING failed to delete Trick Memory for 'this->name'\n",
                          __LINE__ );
      }

      // Set the node name to NULL.
      name = NULL;
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void LRTreeNodeBase::set_name( char const *node_name )
{
   // Check to see if a node name has already been set.
   if ( name == NULL ) {

      // It has not been set; so, only copy the new node name if it is not NULL.
      if ( node_name != NULL ) {
         name = trick_MM->mm_strdup( node_name );
      }

   } // A node name is set; so, make sure that the new node name is NOT NULL.
   else if ( node_name != NULL ) {

      // New node name not NULL; so, checking for a match to the existing name.
      if ( strcmp( node_name, this->name ) ) {

         // New node name differs from existing node name.

         // Free the existing name.
         if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
            message_publish( MSG_WARNING, "SpaceFOM::LRTreeNodeBase::set_name():%d WARNING failed to delete Trick Memory for 'this->name'\n",
                             __LINE__ );
         }

         // Copy the new node name.
         name = trick_MM->mm_strdup( node_name );
      }

   } // The new node name is NULL; so, reset the node name.
   else {

      // Free the existing name.
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::LRTreeNodeBase::set_name():%d WARNING failed to delete Trick Memory for 'this->name'\n",
                          __LINE__ );
      }

      // Set the node name to NULL.
      name = NULL;
   }

   return;
}

/*!
 * @job_class{initialization}
 */
bool LRTreeNodeBase::set_root( bool root_status )
{
   // If setting as root node.
   if ( root_status ) {

      // Check to make sure predicates are satisfied.
      if ( this->parent == NULL ) {
         // Parent frame is NULL so set the root state.
         this->is_root_node = true;
         return ( true );
      } else {
         // Parent frame is not null.  Automatic fail.
         // Note that we DO NOT change the is_root_node state.
         return ( false );
      }

   } // If setting is NOT a root node.
   else {

      // Check to make sure predicates are satisfied.
      if ( this->parent != NULL ) {

         // Set the is_root_node state to false.
         this->is_root_node = false;
         return ( true );

      } else {

         // Parent frame is NULL.  Automatic fail.
         // Note that we DO NOT change the is_root_node state.
         return ( false );
      }
   }

   return ( true );
}

/*!
 * @job_class{scheduled}
 */
void LRTreeNodeBase::print_node( std::ostream &stream ) const
{
   stream << "Node Name: '" << this->name << "'\n"
          << "\tID: " << this->node_id << '\n';

   if ( parent == NULL ) {
      stream << "\tParent: <NULL>\n";
   } else {
      stream << "\tParent: '" << this->parent->name << "'\n";
   }
   if ( is_root_node ) {
      stream << "\tIs Root: True\n";
   } else {
      stream << "\tIs Root: False\n";
   }

   return;
}
