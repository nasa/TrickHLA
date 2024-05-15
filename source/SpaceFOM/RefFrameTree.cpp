/*!
@file SpaceFOM/RefFrameTree.cpp
@ingroup SpaceFOM
@brief This class provides an extensible base class for a SpaceFOM Reference
Frames tree.

@copyright Copyright 2023 United States Government as represented by the
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
@trick_link_dependency{RefFrameBase.cpp}
@trick_link_dependency{RefFrameTree.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Initial version.}
@revs_end

*/

// System include files.

// Trick include files.
#include "trick/message_proto.h"
#include "trick/MemoryManager.hh"

// TrickHLA model include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"

// SpaceFOM include files.
#include "SpaceFOM/RefFrameTree.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RefFrameTree::RefFrameTree()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
RefFrameTree::~RefFrameTree()
{
   return;
}

/*!
 * @job_class{initialization}
 */
bool RefFrameTree::add_frame( RefFrameBase *frame_ptr )
{
   return ( this->add_node( frame_ptr ) );
}

/*!
 * @job_class{initialization}
 */
bool RefFrameTree::build_tree()
{
   return ( LRTreeBase::build_tree() );
}

/*!
 * @job_class{initialization}
 */
bool RefFrameTree::check_tree()
{
   return ( LRTreeBase::check_tree() );
}

/*!
 * @job_class{scheduled}
 */
void RefFrameTree::print_tree( std::ostream &stream )
{
   if ( debug || DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
      send_hs( stdout,
               "RefFrameTree::print_tree():%d\n",
               __LINE__, THLA_NEWLINE );
      print_nodes( stream );
   }
   if ( debug || DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
      print_paths( stream );
   }
   return;
}

/*!
 * @job_class{scheduled}
 */
bool RefFrameTree::has_frame( char const *name )
{
   return ( this->has_node( name ) );
}

/*!
 * @job_class{scheduled}
 */
bool RefFrameTree::has_frame( string const &name )
{
   return ( this->has_node( name ) );
}

/*!
 * @job_class{scheduled}
 */
bool RefFrameTree::has_frame( RefFrameBase const *frame )
{
   return ( has_node( frame ) );
}

/*!
 * @job_class{scheduled}
 */
RefFrameBase *RefFrameTree::find_frame( char const *name )
{
   return ( static_cast< RefFrameBase * >( find_node( name ) ) );
}

/*!
 * @job_class{scheduled}
 */
RefFrameBase *RefFrameTree::find_frame( string const &name )
{
   return ( static_cast< RefFrameBase * >( find_node( name ) ) );
}

/*!
 * @job_class{scheduled}
 */
RefFrameData *RefFrameTree::build_transform(
   RefFrameBase const * source_frame,
   RefFrameBase const * express_frame )
{
   RefFrameData * transform_data = NULL;
   LRTreeNodeBase * current_node = NULL;
   RefFrameBase * current_frame = NULL;
   RefFrameBase * next_frame = NULL;
   LRTreeNodeVector::iterator path_itr;

   // Allocate the frame transformation.
   transform_data = static_cast< SpaceFOM::RefFrameData * >( trick_MM->declare_var( "SpaceFOM::RefFrameData" ) );
   // Check for a NULL allocation.
   if ( transform_data != NULL ){
      send_hs( stderr, "SpaceFOM::RefFrameTree::build_transform: %d ERROR allocating Trick Memory!",
               __LINE__, THLA_NEWLINE );
      return( NULL );
   }

   // First, let's get the transformation path from source to the express frame.
   LRTreeNodeVector & path = this->paths[source_frame->node_id][express_frame->node_id];

   // Now the work begins . . .

   // Get the first node/frame in the path.
   current_frame = static_cast<RefFrameBase*>(path[0]);
   next_frame    = static_cast<RefFrameBase*>(path[1]);
   // Check for parent.
   if ( current_frame->parent_frame != NULL ){
      // Check if we are going up the tree.
      if ( current_frame->parent_frame == next_frame ) {

      }
      // Check to see if we are going down the tree.
      else if ( next_frame->parent_frame == current_frame ){

      }
   }

   // Iterate through the path.
   for ( path_itr = path.begin(); path_itr != path.end(); ++path_itr ) {

      // Access the Reference Frame data.
      current_frame = static_cast<RefFrameBase*>(*path_itr);

      // Check if this is a node on the up path or the down path.


   }

   return( transform_data );
}
