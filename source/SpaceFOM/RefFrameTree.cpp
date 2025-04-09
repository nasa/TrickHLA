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
#include <sstream>
#include <string.h>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"

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
   return ( add_node( frame_ptr ) );
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
void RefFrameTree::print_tree( std::ostream &stream ) const
{
   if ( debug || DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
      message_publish( MSG_NORMAL, "RefFrameTree::print_tree():%d\n", __LINE__ );
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
   return ( has_node( name ) );
}

/*!
 * @job_class{scheduled}
 */
bool RefFrameTree::has_frame( string const &name )
{
   return ( has_node( name ) );
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
bool RefFrameTree::build_transform(
   RefFrameBase const *source_frame,
   RefFrameBase const *express_frame,
   RefFrameData       *transform_data )
{
   RefFrameBase *current_frame = NULL;
   RefFrameBase *next_frame    = NULL;

   LRTreeNodeVector::iterator path_itr;

   RefFrameDataState in_frame_data;
   RefFrameDataState out_frame_data;

   // Check for a NULL allocation.
   if ( transform_data == NULL ) {
      message_publish( MSG_WARNING, "SpaceFOM::RefFrameTree::build_transform: %d ERROR NULL transform data!\n",
                       __LINE__ );
      return ( false );
   }

   // Check for the degenerate case.  If the source frame is the express frame
   // then the transformation is an identity transformation.
   // NOTE: this case is captured below but we can handle it here trivially.
   if ( source_frame == express_frame ) {

      // Initialize the transform to an identity transform.
      transform_data->initialize();

      // Set the time stamp.
      transform_data->set_time( source_frame->packing_data.state.time );

      // Return success.
      return ( true );
   }

   // Check for the trivial case where the source frame's parent is already
   // the desired express frame.  In this case, the source is the transform.
   // NOTE: this case is captured below but we can handle it here trivially.
   if ( source_frame->parent_frame == express_frame ) {

      // Copy the source frame data as the transform.
      transform_data->copy( source_frame->packing_data );

      // Return success.
      return ( true );
   }

   //------------------------------------------------------------------------
   // This is not one of the above trivial cases.  So, proceed with the
   // general case by iterating through the transform path accumulating
   // the transformation data from the source to the express frame.
   //------------------------------------------------------------------------

   // Get the transformation path from source to the express frame.
   LRTreeNodeVector &path = this->paths[source_frame->node_id][express_frame->node_id];

   // Initialize the transform to an identity transform.
   out_frame_data.initialize();

   // Set the time stamp.
   out_frame_data.set_time( source_frame->packing_data.state.time );

   // Initialize the output data frame to cover trivial transformation chains.
   out_frame_data.copy( in_frame_data );

   // Start with the first element in the path.
   current_frame = static_cast< RefFrameBase * >( *( path_itr = path.begin() ) );

   // Now iterate through the remainder of the path vector.
   for ( ++path_itr; path_itr < path.end(); ++path_itr ) {

      // Copy the out data transformed into the in data.  This will effectively
      // chain the transformations together by accumulation on each subsequent
      // loop.
      in_frame_data.copy( out_frame_data );

      // Get the reference to the next frame
      next_frame = static_cast< RefFrameBase * >( *( path_itr ) );

      // Check if we are moving up the tree.
      if ( current_frame->parent_frame == next_frame ) {

         // Add transformation into the current frame's parent frame.
         if ( !in_frame_data.transform_to_parent( current_frame->packing_data, &out_frame_data ) ) {
            // Print Error message
            message_publish( MSG_WARNING, "SpaceFOM::RefFrameTree::build_transform: %d ERROR calling 'transform_to_parent'!\n",
                             __LINE__ );
            // Error return.
            return ( false );
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
               ostringstream errmsg;
               errmsg << "SpaceFOM::RefFrameTree::build_transform -> transform_to_parent:\n"
                      << "\tfrom " << current_frame->get_name() << " to " << next_frame->get_name() << "\n";
               out_frame_data.print_data( errmsg );
               message_publish( MSG_NORMAL, errmsg.str().c_str() );
            }
         }

      }
      // Check if we are moving down the tree.
      else if ( next_frame->parent_frame == current_frame ) {

         // Use the reverse transformation to transform into the next frame.
         if ( !in_frame_data.transform_to_child( next_frame->packing_data, &out_frame_data ) ) {
            // Print Error message
            message_publish( MSG_WARNING, "SpaceFOM::RefFrameTree::build_transform: %d ERROR calling 'transform_to_child'!\n",
                             __LINE__ );
            // Error return.
            return ( false );
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
               ostringstream errmsg;
               errmsg << "SpaceFOM::RefFrameTree::build_transform -> transform_to_child:\n"
                      << "\tfrom " << current_frame->get_name() << " to " << next_frame->get_name() << "\n";
               out_frame_data.print_data( errmsg );
               message_publish( MSG_NORMAL, errmsg.str().c_str() );
            }
         }
      }

      // Make the next frame the current frame.
      current_frame = next_frame;
   }

   // Copy the working data into the transform data.
   transform_data->copy( out_frame_data );

   // Set the transform data name (source) and new parent frame (express).
   transform_data->set_name( source_frame->name );
   transform_data->set_parent_name( express_frame->name );

   // Return success.
   return ( true );
}
