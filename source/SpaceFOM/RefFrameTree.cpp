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

/*! @brief Add a reference frame to the tree.
 *  @details This function is used to add a SpaceFOM reference frame into
 *  the reference frame tree.
 *  @param frame_ptr Pointer to the reference frame to add.
 *  @return Success or failure of the add. */
bool RefFrameTree::add_frame( RefFrameBase *frame_ptr )
{
   return ( this->add_node( frame_ptr ) );
}

bool RefFrameTree::build_tree()
{
   return ( LRTreeBase::build_tree() );
}

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

bool RefFrameTree::has_frame( char const *name )
{
   return ( this->has_node( name ) );
}

bool RefFrameTree::has_frame( string const &name )
{
   return ( this->has_node( name ) );
}

bool RefFrameTree::has_frame( RefFrameBase const *frame )
{
   return ( has_node( frame ) );
}

RefFrameBase *RefFrameTree::find_frame( char const *name )
{
   return ( static_cast< RefFrameBase * >( find_node( name ) ) );
}

RefFrameBase *RefFrameTree::find_frame( string const &name )
{
   return ( static_cast< RefFrameBase * >( find_node( name ) ) );
}
