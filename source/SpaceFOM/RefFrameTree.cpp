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
#include <string.h>

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
   RefFrameBase const * express_frame,
   RefFrameData       * transform_data )
{
   RefFrameBase * current_frame = NULL;
   RefFrameBase * next_frame    = NULL;
   RefFrameBase * end_frame     = NULL;

   LRTreeNodeVector::iterator path_itr;

   RefFrameDataState in_frame_data;
   RefFrameDataState out_frame_data;

   bool allocated = false;

   // Allocate the frame transformation if NULL.
   if ( transform_data == NULL ){
      transform_data = static_cast< SpaceFOM::RefFrameData * >( trick_MM->declare_var( "SpaceFOM::RefFrameData" ) );
      // Check for a NULL allocation.
      if ( transform_data == NULL ){
         send_hs( stderr, "SpaceFOM::RefFrameTree::build_transform: %d ERROR allocating Trick Memory!",
               __LINE__, THLA_NEWLINE );
         return( NULL );
      }
      allocated = true;
   }

   // Check for the degenerate case.  If the source frame is the express frame
   // then the transformation is an identity transformation.
   // NOTE: this case is captured below but we can handle it here trivially.
   if ( source_frame == express_frame ) {

      // Initialize the transform to an identity transform.
      transform_data->initialize();

      // Set the time stamp.
      transform_data->set_time( source_frame->packing_data.state.time );

      // Return the identity transform reference.
      return( transform_data );

   }

   // Check for the trivial case where the source frame's parent is already
   // the desired express frame.  In this case, the source is the transform.
   // NOTE: this case is captured below but we can handle it here trivially.
   if ( source_frame->parent_frame == express_frame ) {

      // Copy the source frame data as the transform.
      transform_data->copy( source_frame->packing_data );

      // Return the transform reference.
      return( transform_data );

   }

   // Get the transformation path from source to the express frame.
   LRTreeNodeVector & path = this->paths[source_frame->node_id][express_frame->node_id];

   // Set the current frame to the starting frame.
   current_frame = static_cast<RefFrameBase*>(*(path_itr = path.begin()));
   next_frame    = static_cast<RefFrameBase*>(*(++path_itr));

   // Mark the ending frame for termination.
   end_frame = static_cast<RefFrameBase*>(*(path.end()));

   // Set the initial frame transformation depending on initial direction, up or down.
   if ( current_frame->parent_frame == next_frame ) {
      // When going up the tree we start with the first frame.
      in_frame_data.copy( current_frame->packing_data );
   }
   else {
      // When going down the tree, we start with an identity transform.
      // NOTE: This should already be identity but just to make sure ...
      in_frame_data.initialize();
   }

   //
   // Iterate up the tree.
   //
   // Note that when going up the tree we ignore the last node.  This is
   // because the transform from source to express frame does not include
   // transformation data for the express frame to its parent.
   while (    (current_frame->parent_frame == next_frame)
           && (next_frame != end_frame)
           && (path_itr < path.end())                     ) {

      // Add transformation into the parent frame.
      if( !in_frame_data.transform_to_parent( next_frame->packing_data, &out_frame_data ) ) {

         // Print Error message
         send_hs( stderr, "SpaceFOM::RefFrameTree::build_transform: %d ERROR calling 'transform_to_parent'!",
                  __LINE__, THLA_NEWLINE );

         // Free transform.
         if( allocated ) {
            if ( trick_MM->delete_var( static_cast< void * >( transform_data ) ) ) {
               send_hs( stderr, "SpaceFOM::RefFrameTree::build_transform:%d ERROR deleting Trick Memory for 'transform_data'%c",
                        __LINE__, THLA_NEWLINE );
            }
            transform_data = NULL;
         }

         // Return NULL pointer to indicate an error occurred.
         return( NULL );

      }

      // Copy the out data transformed in this loop into the in data for the next.
      // This effectively chains the transformations together by accumulation.
      in_frame_data.copy( out_frame_data );

      // Move to the next node in the path.
      current_frame = next_frame;
      next_frame = static_cast<RefFrameBase*>(*(++path_itr));

   }

   //
   // Check for NULL intermediate node.
   //
   // FIXME: Do we need to do anything here?
   //if ( current_frame->parent_frame == NULL ) {
   //
   //}

   //
   // Iterate down the tree.
   //
   while (    (next_frame != NULL )
           && (next_frame->parent_frame == current_frame)
           && (path_itr < path.end())                     ) {

      // Use the reverse transformation.
      if( !in_frame_data.transform_to_child( next_frame->packing_data, &out_frame_data ) ) {

         // Print Error message
         send_hs( stderr, "SpaceFOM::RefFrameTree::build_transform: %d ERROR calling 'transform_to_child'!",
               __LINE__, THLA_NEWLINE );

         // Free transform.
         if( allocated ) {
            if ( trick_MM->delete_var( static_cast< void * >( transform_data ) ) ) {
               send_hs( stderr, "SpaceFOM::RefFrameTree::build_transform:%d ERROR deleting Trick Memory for 'transform_data'%c",
                        __LINE__, THLA_NEWLINE );
            }
            transform_data = NULL;
         }

         // Return NULL pointer to indicate an error occurred.
         return( NULL );

      }

      // Copy the out data transformed in this loop into the in data for the next.
      // This effectively chains the transformations together by accumulation.
      in_frame_data.copy( out_frame_data );

      // Move to the next node in the path.
      current_frame = next_frame;
      if ( next_frame == end_frame ) {
         next_frame = NULL;
      }
      else {
         next_frame = static_cast<RefFrameBase*>(*(++path_itr));
      }

   }

   //
   // Check to make sure we reached the end of the list without error.
   //
   if ( path_itr != path.end() ){

      // Print Error message
      send_hs( stderr, "SpaceFOM::RefFrameTree::build_transform: %d Something is wrong with the transform path!",
               __LINE__, THLA_NEWLINE );

      // Free transform.
      if( allocated ) {
         if ( trick_MM->delete_var( static_cast< void * >( transform_data ) ) ) {
            send_hs( stderr, "SpaceFOM::RefFrameTree::build_transform:%d ERROR deleting Trick Memory for 'transform_data'%c",
                     __LINE__, THLA_NEWLINE );
         }
         transform_data = NULL;
      }

      // Return NULL pointer to indicate an error occurred.
      return( NULL );

   }

   // Copy the working data into the transform data.
   transform_data->copy( out_frame_data );

   // Set the transform data name (source) and new parent frame (express).
   transform_data->set_name( source_frame->name );
   transform_data->set_parent_name( express_frame->name );

   return( transform_data );

}
