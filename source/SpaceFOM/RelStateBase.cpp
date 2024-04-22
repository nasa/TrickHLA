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
#include "trick/constant.h"
#include "trick/trick_math.h"

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
   PhysicalEntityData * entity )
{
   RefFrameData * path_transform; /* The reference frame transformation data
                                     needed to transform from the entity's
                                     parent frame into a desired express frame. */

   double r_ent_p_exp[3]; /* Position vector of the entity with respect to its
                             parent frame but expressed in the desired express frame. */
   double v_ent_p_exp[3]; /* Velocity vector of the entity with respect to its
                             parent frame but expressed in the desired express frame. */
   double a_ent_p_exp[3]; /* Acceleration vector of the entity with respect to its
                             parent frame but expressed in the desired express frame. */

   // Working variables.
   double wxr_p[3];
   double v_p[3];
   double axr_p[3];
   double two_w_p[3];
   double two_wxv_p[3];
   double wxwxr_p[3];
   double a_p[3];

   // Check for NULL frame.
   if ( entity == NULL ){
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: PhysicalEntityData NULL!" << endl;
         send_hs( stderr, errmsg.str().c_str() );
      }
      return( false );
   }

   // Find the entity parent frame in the tree.
   RefFrameBase * entity_parent_frame = frame_tree->find_frame( entity->parent_frame );
   if ( entity_parent_frame == NULL ){
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: Could not find parent frame: %s!" << endl;
         send_hs( stderr, entity->parent_frame, errmsg.str().c_str() );
      }
      return( false );
   }

   // Check for trivial transformation.
   if ( entity_parent_frame == express_frame ){

      // Just copy the state and return.
      this->copy( *entity );

      return( true );
   }

   //**************************************************************************
   // Build the reference frame transformation from the reference frame path.
   // See the Reference Frame Transformations section of the SpaceFOM
   // (Appendix E).
   //**************************************************************************

   // Ask the Reference Frame Tree to build the transformation for the entity
   // parent reference frame with respect to the desired express frame.
   path_transform = frame_tree->build_transform( entity_parent_frame, express_frame );

   //
   // Position computations.
   //
   // Transform the entity position vector expressed in its parent frame
   // into the desired express frame.  This is still a vector from the origin
   // of the original parent frame to the entity but expressed in the express
   // frame's orientation.
   path_transform->state.att.transform_vector( entity->state.pos, r_ent_p_exp );

   // Compute entity position expressed in the express frame.
   V_ADD( this->state.pos, path_transform->state.pos, r_ent_p_exp )

   // Compute the entity attitude in the express frame.
   path_transform->state.att.transform_quat( entity->state.att, this->state.att );

   //
   // Velocity computations.
   //
   // Compute the apparent velocity of the entity in a rotating parent frame.
   V_CROSS( wxr_p, path_transform->state.ang_vel, entity->state.pos );

   // Compute the total velocity of the entity in the rotating parent frame.
   V_ADD( v_p, entity->state.vel, wxr_p );

   // Transform the entity velocity into the express frame.
   path_transform->state.att.transform_vector( v_p, v_ent_p_exp );

   // Compute entity velocity expressed in the express frame.
   V_ADD( this->state.vel, path_transform->state.vel, v_ent_p_exp );

   // Compute the angular velocity in the express frame.


   //
   // Acceleration computations.
   //
   // Compute the apparent acceleration of the entity in a rotating parent frame.
   V_CROSS( axr_p, path_transform->ang_accel, entity->state.pos );
   V_SCALE( two_w_p, 2.0, path_transform->state.ang_vel );
   V_CROSS( two_wxv_p, two_w_p, entity->state.vel );
   V_CROSS( wxwxr_p, path_transform->state.ang_vel, wxr_p );

   // Add up the components of the rotationally induced acceleration.
   a_p[0] = path_transform->accel[0] + wxwxr_p[0] + two_wxv_p[0] + axr_p[0];
   a_p[1] = path_transform->accel[1] + wxwxr_p[1] + two_wxv_p[1] + axr_p[1];
   a_p[2] = path_transform->accel[2] + wxwxr_p[2] + two_wxv_p[2] + axr_p[2];

   // Transform the entity acceleration into the express frame.
   path_transform->state.att.transform_vector( a_p, a_ent_p_exp );

   // Compute entity acceleration expressed in the express frame.
   V_ADD( this->accel, path_transform->accel, a_ent_p_exp );

   return( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData * entity,
   const char * wrt_frame )
{

   // Set the frame in which to express the state.
   if ( this->set_frame( wrt_frame ) ) {
      // Call the base function.
      return( this->compute_state( entity, express_frame ) );
   }

   return( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData * entity,
   std::string & wrt_frame )
{

   // Set the frame in which to express the state.
   if ( this->set_frame( wrt_frame ) ) {
      // Call the base function.
      return( this->compute_state( entity, express_frame ) );
   }

   return( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData * entity,
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
      return( this->compute_state( entity ) );
   }

   return( false );
}

