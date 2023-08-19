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
@trick_link_dependency{RefFrameBase.cpp}
@trick_link_dependency{RefFrameTree.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Initial version.}
@revs_end

*/

// System include files.

// Trick include files.

// TrickHLA model include files.

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

/*! @brief Add a reference frame to the tree.
 *  @details This function is used to add a SpaceFOM reference frame into
 *  the reference frame tree.
 *  @param frame_ptr Pointer to the reference frame to add.
 *  @return Success or failure of the add. */
bool RefFrameTree::add_frame( RefFrameBase *frame_ptr )
{
   if ( frame_ptr != NULL ) {
      ref_frame_map.insert( pair< string, RefFrameBase * >( frame_ptr->name, frame_ptr ) );
      return ( true );
   }
   return ( true );
}

bool RefFrameTree::build_tree()
{
   return ( true );
}

bool RefFrameTree::check_tree()
{
   return ( true );
}

bool RefFrameTree::has_frame( char const *name )
{
   map< string, RefFrameBase * >::iterator map_iter;

   // Find the frame in the map.
   map_iter = ref_frame_map.find( string( name ) );
   if ( map_iter != ref_frame_map.end() ) {
      return ( true );
   }
   return ( false );
}

bool RefFrameTree::has_frame( string const &name )
{
   map< string, RefFrameBase * >::iterator map_iter;

   // Find the frame in the map.
   map_iter = ref_frame_map.find( name );
   if ( map_iter != ref_frame_map.end() ) {
      return ( true );
   }
   return ( false );
}

bool RefFrameTree::has_frame( RefFrameBase const *frame )
{
   return ( has_frame( frame->name ) );
}

RefFrameBase *RefFrameTree::find_frame( char const *name )
{
   map< string, RefFrameBase * >::iterator map_iter;

   // Find the frame in the map.
   map_iter = ref_frame_map.find( string( name ) );
   if ( map_iter != ref_frame_map.end() ) {
      return ( ref_frame_map[string( name )] );
   }
   return ( NULL );
}

RefFrameBase *RefFrameTree::find_frame( string const &name )
{
   map< string, RefFrameBase * >::iterator map_iter;

   // Find the frame in the map.
   map_iter = ref_frame_map.find( name );
   if ( map_iter != ref_frame_map.end() ) {
      return ( ref_frame_map[name] );
   }
   return ( NULL );
}

RefFrameBase *RefFrameTree::find_common_base(
   char const *child_1,
   char const *child_2 )
{
   return ( NULL );
}

RefFrameBase *RefFrameTree::find_common_base(
   string const &child_1,
   string const &child_2 )
{
   return ( NULL );
}

RefFrameBase *RefFrameTree::find_common_base(
   RefFrameBase *child_1,
   RefFrameBase *child_2 )
{
   return ( NULL );
}
