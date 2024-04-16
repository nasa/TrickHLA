/*!
@file SpaceFOM/RelStateBase.cpp
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM Reference Frames.

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
@trick_link_dependency{SpaceTimeCoordinateData.cpp}
@trick_link_dependency{RefFrameBase.cpp}
@trick_link_dependency{RefFrameTree.cpp}
@trick_link_dependency{PhysicalEntityData.cpp}
@trick_link_dependency{RelStateBase.cpp}

@revs_title
@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2024, --, Initial version }
@revs_end

*/

// System include files.
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA model include files.
#include "TrickHLA/Types.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"

// SpaceFOM include files.
#include "SpaceFOM/RelStateBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RelStateBase::RelStateBase(
   RefFrameBase & wrt_frame,
   RefFrameTree & tree )
   : express_frame( &wrt_frame ),
     frame_tree( &tree )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
RelStateBase::~RelStateBase()
{
   express_frame = NULL;
   frame_tree = NULL;

   return;
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::set_frame(
   const char * wrt_frame )
{
   RefFrameBase * lookup_frame;

   // Look up the frame by name.
   lookup_frame = frame_tree->find_frame( wrt_frame );
   if ( lookup_frame ) {
      express_frame = lookup_frame;
      return( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
      ostringstream errmsg;
      errmsg << "RelStateBase::set_frame() Warning: Reference frame "
             << wrt_frame << " not found!" << endl;
      send_hs( stderr, errmsg.str().c_str() );
   }

   return( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::set_frame(
   std::string & wrt_frame )
{
   RefFrameBase * lookup_frame;

   // Look up the frame by name.
   lookup_frame = frame_tree->find_frame( wrt_frame );
   if ( lookup_frame ) {
      express_frame = lookup_frame;
      return( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
      ostringstream errmsg;
      errmsg << "RelStateBase::set_frame() Warning: Reference frame "
             << wrt_frame << " not found!" << endl;
      send_hs( stderr, errmsg.str().c_str() );
   }

   return( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData * source )
{
   // Check for NULL frame.
   if ( source == NULL ){
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: PhysicalEntityData NULL!" << endl;
         send_hs( stderr, errmsg.str().c_str() );
      }
      return( false );
   }

   // Find the source parent frame in the tree.
   RefFrameBase * source_parent_frame = frame_tree->find_frame( source->parent_frame );
   if ( source_parent_frame == NULL ){
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: Could not find parent frame: %s!" << endl;
         send_hs( stderr, source->parent_frame, errmsg.str().c_str() );
      }
      return( false );
   }

   // Check for trivial transformation.
   if ( source_parent_frame == express_frame ){

      // Just copy the state and return.
      strcpy( this->name, source->name );

      return( true );
   }

   // Now to do some linear algebra.
   // See SpaceFOM standard p 148.
   QuaternionData q_c_p;  // The attitude of the child frame with respect to the parent frame.
   double r_0_p[3]; /* Position vector giving the position of child reference
                       frame origin with respect to the parent reference frame
                       origin expressed in parent reference frame coordinates. */
   double r_p[3];   /* Position vector of the entity expressed in parent
                       reference frame coordinates. */
   double r_c_in_p[3]; /* Position vector in the child frame transformed into
                          parent frame orientation. */

   // Compute position.
   q_c_p.transform_vector( source->state.pos, r_c_in_p );
   r_p[0] = r_0_p[0] + r_c_in_p[0];
   r_p[1] = r_0_p[1] + r_c_in_p[1];
   r_p[2] = r_0_p[2] + r_c_in_p[2];

   return( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData * source,
   const char * wrt_frame )
{

   // Set the frame in which to express the state.
   if ( this->set_frame( wrt_frame ) ) {
      // Call the base function.
      return( this->compute_state( source, express_frame ) );
   }

   return( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData * source,
   std::string & wrt_frame )
{

   // Set the frame in which to express the state.
   if ( this->set_frame( wrt_frame ) ) {
      // Call the base function.
      return( this->compute_state( source, express_frame ) );
   }

   return( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData * source,
   RefFrameBase * wrt_frame )
{
   // Check for NULL frame.
   if ( wrt_frame == NULL ){
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: Reference frame NULL!" << endl;
         send_hs( stderr, errmsg.str().c_str() );
      }
      return( false );
   }

   // Set the frame in which to express the state.
   if ( this->set_frame( *wrt_frame ) ) {
      // Call the base function to compute the state.
      return( this->compute_state( source ) );
   }

   return( false );
}

