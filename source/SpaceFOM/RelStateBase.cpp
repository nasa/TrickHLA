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

#define REL_STATE_EXTRA_DEBUG 0

/*!
 * @job_class{initialization}
 */
RelStateBase::RelStateBase(
   RefFrameBase &wrt_frame,
   RefFrameTree &tree )
   : debug( false ),
     express_frame( &wrt_frame ),
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
      set_parent_frame( express_frame->name );
      return ( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
      ostringstream errmsg;
      errmsg << "RelStateBase::set_frame() Warning: Reference frame "
             << wrt_frame << " not found!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
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
      set_parent_frame( express_frame->name );
      return ( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
      ostringstream errmsg;
      errmsg << "RelStateBase::set_frame() Warning: Reference frame "
             << wrt_frame << " not found!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   return ( false );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData const *entity )
{
   double r_ent_s_t[3]; /* Position vector of the entity with respect to its
                           current frame (subject) but expressed in the desired
                           target frame. */
   double v_ent_s_t[3]; /* Velocity vector of the entity with respect to its
                           current frame (subject) but expressed in the desired
                           target frame. */
   double a_ent_s_t[3]; /* Acceleration vector of the entity with respect to its
                           current frame (subject) but expressed in the desired
                           target frame. */

   QuaternionData q_s_t; /* The transformation quaternion from subject into target frame. */

   // Working variables.
   // Translation
   double wxr_s[3];
   double v_s[3];
   double axr_s[3];
   double two_w_s[3];
   double two_wxv_s[3];
   double wxwxr_s[3];
   double a_s[3];
   // Rotation
   double w_s_t_bdy[3];    /* Angular velocity of the subject frame with respect to
                              the target frame expressed in the entity body frame. */
   double wdot_s_t_bdy[3]; /* Angular acceleration of the subject frame with respect to
                              the target frame expressed in the entity body frame. */

   // Check for NULL frame.
   if ( entity == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: PhysicalEntityData NULL!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return ( false );
   }

   // Find the entity parent frame in the tree.
   RefFrameBase const *entity_subject_frame = frame_tree->find_frame( entity->parent_frame );
   if ( entity_subject_frame == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: Could not find subject frame: %s!" << endl;
         message_publish( MSG_WARNING, entity->parent_frame, errmsg.str().c_str() );
      }
      return ( false );
   }

   // Check for trivial transformation.
   if ( entity_subject_frame == express_frame ) {

      // Just copy the state and return.
      copy( *entity );

      // Print out the path transformation if debug is set.
      if ( debug ) {
         ostringstream msg;
         msg << "SpaceFOM::RelStateBase::compute_state():" << __LINE__ << "\n";
         msg << "Path transformation for " << entity->name << "\n";
         path_transform.print_data( msg );
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      return ( true );
   }

   //**************************************************************************
   // Build the reference frame transformation from the reference frame path.
   // See the Reference Frame Transformations section of the SpaceFOM
   // (Appendix E).
   //**************************************************************************

   // Ask the Reference Frame Tree to build the transformation for the entity
   // subject reference frame with respect to the desired express (target) frame.
   if ( !frame_tree->build_transform( entity_subject_frame, express_frame, &path_transform ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_0_TRACE, DEBUG_SOURCE_ALL_MODULES ) ) {
         ostringstream errmsg;
         errmsg << "RelStateBase::compute_state() Warning: Could not build frame transformation: %s/%s!" << endl;
         message_publish( MSG_WARNING, entity->parent_frame, express_frame->name, errmsg.str().c_str() );
      }
      return ( false );
   }

   // Copy over the identification data strings.
   set_name( entity->name );
   set_type( entity->type );
   set_status( entity->status );
   set_parent_frame( express_frame->name );

   //**************************************************************************
   // Compute the state of the entity with respect to a new express (parent)
   // frame.
   //**************************************************************************

   //
   // Position computations.
   //
   // Transform the entity position vector expressed in its current frame
   // (subject) into the desired express frame (target).  This is still a vector
   // from the origin of the subject frame to the entity but expressed
   // in the new target frame coordinates.
   path_transform.state.att.conjugate_transform_vector( entity->state.pos, r_ent_s_t );

   // Compute entity position expressed in the target frame.
   V_ADD( this->state.pos, path_transform.state.pos, r_ent_s_t )

   // Compute the entity attitude in the target frame.
   // Frame rotations and transformations can be accumulated through quaternion
   // multiplication.  Note q_02 = q_12 * q_01
   state.att.multiply( entity->state.att, path_transform.state.att );

   //
   // Velocity computations.
   //
   // Compute the apparent velocity of the entity in a rotating target frame.
   V_CROSS( wxr_s, path_transform.state.ang_vel, entity->state.pos );

   // Compute the total velocity of the entity in the rotating target frame.
   V_ADD( v_s, entity->state.vel, wxr_s );

   // Transform the entity velocity into the express (target) frame.
   path_transform.state.att.conjugate_transform_vector( v_s, v_ent_s_t );

   // Compute entity velocity expressed in the express (target) frame.
   V_ADD( this->state.vel, path_transform.state.vel, v_ent_s_t );

   // Compute this entity's angular velocity wrt the target frame.
   // NOTE: Angular velocity is expressed in the 'body' frame, not the target frame.
   // Transform the subject frame's angular velocity wrt the target frame into
   // this entity's 'body' frame.
   entity->state.att.conjugate_transform_vector( path_transform.state.ang_vel, w_s_t_bdy );
   // Add the rotational velocity of the entity's current frame (subject) with
   // respect to the desired target frame.
   V_ADD( this->state.ang_vel, w_s_t_bdy, entity->state.ang_vel );

   //
   // Acceleration computations.
   //
   // Compute the apparent acceleration of the entity in a rotating target frame.
   V_CROSS( axr_s, path_transform.ang_accel, entity->state.pos );
   V_SCALE( two_w_s, path_transform.state.ang_vel, 2.0 );
   V_CROSS( two_wxv_s, two_w_s, entity->state.vel );
   V_CROSS( wxwxr_s, path_transform.state.ang_vel, wxr_s );

   // Add up the components of the rotationally induced acceleration.
   a_s[0] = entity->accel[0] + wxwxr_s[0] + two_wxv_s[0] + axr_s[0];
   a_s[1] = entity->accel[1] + wxwxr_s[1] + two_wxv_s[1] + axr_s[1];
   a_s[2] = entity->accel[2] + wxwxr_s[2] + two_wxv_s[2] + axr_s[2];

   // Transform the entity acceleration into the target frame.
   path_transform.state.att.conjugate_transform_vector( a_s, a_ent_s_t );

   // Compute entity acceleration expressed in the target frame.
   V_ADD( this->accel, path_transform.accel, a_ent_s_t );

   // Compute this entity's angular acceleration wrt the target frame.
   // NOTE: Angular acceleration is expressed in the 'body' frame, not the subject frame.
   // Transform the current frame's angular acceleration wrt the target frame
   // into the entity 'body' frame.
   entity->state.att.conjugate_transform_vector( path_transform.ang_accel, wdot_s_t_bdy );
   // Add the rotational acceleration of the entity subject frame with respect
   // to the target frame.
   V_ADD( this->ang_accel, wdot_s_t_bdy, entity->ang_accel );

   // Print out the path transformation if debug is set.
   if ( debug ) {
      ostringstream msg;
      msg << "SpaceFOM::RelStateBase::compute_state():" << __LINE__ << "\n";
      msg << "\tPath transform: \n";
      path_transform.print_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return ( true );
}

/*!
 * @job_class{scheduled}
 */
bool RelStateBase::compute_state(
   PhysicalEntityData const *entity,
   char const               *wrt_frame )
{
   // Set the frame in which to express the state.
   if ( set_frame( wrt_frame ) ) {
      // Call the base function.
      return ( compute_state( entity, express_frame ) );
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
   if ( set_frame( wrt_frame ) ) {
      // Call the base function.
      return ( compute_state( entity, express_frame ) );
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
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return ( false );
   }

   // Set the frame in which to express the state.
   if ( set_frame( *wrt_frame ) ) { // cppcheck-suppress [knownConditionTrueFalse]
      // Call the base function to compute the state.
      return ( compute_state( entity ) );
   }

   return ( false );
}

/*!
 * @job_class{scheduled}
 */
void RelStateBase::print_path_transform( std::ostream &stream ) const
{
   // Call the path transformation print function.
   path_transform.print_data( stream );

   return;
}
