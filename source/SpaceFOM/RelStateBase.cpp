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
<<<<<<< HEAD
@trick_link_dependency{SpaceTimeCoordinateData.cpp}
=======
>>>>>>> develop
@trick_link_dependency{RefFrameBase.cpp}
@trick_link_dependency{RefFrameTree.cpp}
@trick_link_dependency{RelStateBase.cpp}

@revs_title
@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2024, --, Initial version }
@revs_end

*/

// System include files.
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/constant.h"
#include "trick/message_proto.h"
#include "trick/trick_math.h"

// TrickHLA model include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityData.hh"
#include "SpaceFOM/RefFrameBase.hh"
#include "SpaceFOM/RefFrameTree.hh"
#include "SpaceFOM/RelStateBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RelStateBase::RelStateBase(
   RefFrameBase &wrt_frame,
   RefFrameTree &tree )
   : express_frame( &wrt_frame ),
     frame_tree( &tree )
{
   this->name         = NULL;
   this->type         = NULL;
   this->status       = NULL;
   this->parent_frame = NULL;

   this->accel[0] = 0.0;
   this->accel[1] = 0.0;
   this->accel[2] = 0.0;

   this->ang_accel[0] = 0.0;
   this->ang_accel[1] = 0.0;
   this->ang_accel[2] = 0.0;

   this->cm[0] = 0.0;
   this->cm[1] = 0.0;
   this->cm[2] = 0.0;

   return;
}

/*!
 * @job_class{shutdown}
 */
RelStateBase::~RelStateBase()
{
   this->express_frame = NULL;
   this->frame_tree    = NULL;

   return;
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::set_frame(
   char const *wrt_frame )
{
   RefFrameBase *lookup_frame;

   // Look up the frame by name.
   lookup_frame = frame_tree->find_frame( wrt_frame );
   if ( lookup_frame ) {
      express_frame = lookup_frame;
      return ( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
      ostringstream errmsg;
      errmsg << "RelStateBase::set_frame() Warning: Reference frame "
             << wrt_frame << " not found!" << endl;
      send_hs( stderr, errmsg.str().c_str() );
   }

   return ( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::set_frame(
   std::string const &wrt_frame )
{
   RefFrameBase *lookup_frame;

   // Look up the frame by name.
   lookup_frame = frame_tree->find_frame( wrt_frame );
   if ( lookup_frame ) {
      express_frame = lookup_frame;
      return ( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
      ostringstream errmsg;
      errmsg << "RelStateBase::set_frame() Warning: Reference frame "
             << wrt_frame << " not found!" << endl;
      send_hs( stderr, errmsg.str().c_str() );
   }

   return ( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData const *entity )
{
   RefFrameData path_transform; /* The reference frame transformation data
                                   needed to transform from the entity's
                                   parent frame into a desired express frame. */

   double r_ent_c_p[3]; /* Position vector of the entity with respect to its
                           current frame (child) but expressed in the desired
                           parent frame. */
   double v_ent_c_p[3]; /* Velocity vector of the entity with respect to its
                           current frame (child) but expressed in the desired
                           parent frame. */
   double a_ent_c_p[3]; /* Acceleration vector of the entity with respect to its
                           current frame (child) but expressed in the desired
                           parent frame. */

   // Working variables.
   // Translation
   double wxr_c[3];
   double v_c[3];
   double axr_c[3];
   double two_w_c[3];
   double two_wxv_c[3];
   double wxwxr_c[3];
   double a_c[3];
   // Rotation
   double w_c_p_bdy[3];
   double wdot_c_p_bdy[3];

   // Check for NULL frame.
   if ( entity == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: PhysicalEntityData NULL!" << endl;
         send_hs( stderr, errmsg.str().c_str() );
      }
      return ( false );
   }

   // Find the entity parent frame in the tree.
   RefFrameBase *entity_parent_frame = frame_tree->find_frame( entity->parent_frame );
   if ( entity_parent_frame == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: Could not find parent frame: %s!" << endl;
         send_hs( stderr, entity->parent_frame, errmsg.str().c_str() );
      }
      return ( false );
   }

   // Check for trivial transformation.
   if ( entity_parent_frame == express_frame ) {

      // Just copy the state and return.
      this->copy( *entity );

      return ( true );
   }

   //**************************************************************************
   // Build the reference frame transformation from the reference frame path.
   // See the Reference Frame Transformations section of the SpaceFOM
   // (Appendix E).
   //**************************************************************************

   // Ask the Reference Frame Tree to build the transformation for the entity
   // parent reference frame with respect to the desired express frame.
   if ( !frame_tree->build_transform( entity_parent_frame, express_frame, &path_transform ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: Could not build frame transformation: %s/%s!" << endl;
         send_hs( stderr, entity->parent_frame, express_frame->name, errmsg.str().c_str() );
      }
      return ( false );
   }

   //**************************************************************************
   // Compute the state of the entity with respect to a new express (parent)
   // frame.
   //**************************************************************************

   //
   // Position computations.
   //
   // Transform the entity position vector expressed in its current frame
   // (child) into the desired express frame (parent).  This is still a vector
   // from the origin of the original child frame to the entity but expressed
   // in the new parent frame coordinates.
   path_transform.state.att.transform_vector( entity->state.pos, r_ent_c_p );

   // Compute entity position expressed in the express frame.
   V_ADD( this->state.pos, path_transform.state.pos, r_ent_c_p )

   // Compute the entity attitude in the express frame.
   this->state.att.multiply( path_transform.state.att, entity->state.att );

   //
   // Velocity computations.
   //
   // Compute the apparent velocity of the entity in a rotating parent frame.
   V_CROSS( wxr_c, path_transform.state.ang_vel, entity->state.pos );

   // Compute the total velocity of the entity in the rotating parent frame.
   V_ADD( v_c, entity->state.vel, wxr_c );

   // Transform the entity velocity into the express frame.
   path_transform.state.att.transform_vector( v_c, v_ent_c_p );

   // Compute entity velocity expressed in the express frame.
   V_ADD( this->state.vel, path_transform.state.vel, v_ent_c_p );

   // Compute this entity's angular velocity wrt the parent frame.
   // NOTE: Angular velocity is expressed in the 'body' frame, not the parent frame.
   // Transform the child frame's angular velocity wrt the parent frame into
   // this entity's 'body' frame.
   this->state.att.conjugate_transform_vector( path_transform.state.ang_vel, w_c_p_bdy );
   // Add the rotational velocity of the entity's current frame (child) with
   // respect to the new parent frame.
   V_ADD( this->state.ang_vel, w_c_p_bdy, entity->state.ang_vel );

   //
   // Acceleration computations.
   //
   // Compute the apparent acceleration of the entity in a rotating parent frame.
   V_CROSS( axr_c, path_transform.ang_accel, entity->state.pos );
   V_SCALE( two_w_c, path_transform.state.ang_vel, 2.0 );
   V_CROSS( two_wxv_c, two_w_c, entity->state.vel );
   V_CROSS( wxwxr_c, path_transform.state.ang_vel, wxr_c );

   // Add up the components of the rotationally induced acceleration.
   a_c[0] = entity->accel[0] + wxwxr_c[0] + two_wxv_c[0] + axr_c[0];
   a_c[1] = entity->accel[1] + wxwxr_c[1] + two_wxv_c[1] + axr_c[1];
   a_c[2] = entity->accel[2] + wxwxr_c[2] + two_wxv_c[2] + axr_c[2];

   // Transform the entity acceleration into the parent frame.
   path_transform.state.att.transform_vector( a_c, a_ent_c_p );

   // Compute entity acceleration expressed in the parent frame.
   V_ADD( this->accel, path_transform.accel, a_ent_c_p );

   // Compute this entity's angular acceleration wrt the parent frame.
   // NOTE: Angular acceleration is expressed in the 'body' frame, not the parent frame.
   // Transform the current frame's angular acceleration wrt the parent frame
   // into the entity 'body' frame.
   entity->state.att.conjugate_transform_vector( path_transform.state.ang_vel, wdot_c_p_bdy );
   // Add the rotational acceleration of the entity frame with respect
   // to the parent frame.
   V_ADD( this->ang_accel, wdot_c_p_bdy, entity->ang_accel );

   return ( true );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData const *entity,
   const char               *wrt_frame )
{

   // Set the frame in which to express the state.
   if ( this->set_frame( wrt_frame ) ) {
      // Call the base function.
      return ( this->compute_state( entity, express_frame ) );
   }

   return ( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData const *entity,
   std::string const        &wrt_frame )
{

   // Set the frame in which to express the state.
   if ( this->set_frame( wrt_frame ) ) {
      // Call the base function.
      return ( this->compute_state( entity, express_frame ) );
   }

   return ( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData const *entity,
   RefFrameBase const       *wrt_frame )
{
   // Check for NULL frame.
   if ( wrt_frame == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: Reference frame NULL!" << endl;
         send_hs( stderr, errmsg.str().c_str() );
      }
      return ( false );
   }

   // Set the frame in which to express the state.
   if ( this->set_frame( *wrt_frame ) ) {
      // Call the base function to compute the state.
      return ( this->compute_state( entity ) );
   }

   return ( false );
}
