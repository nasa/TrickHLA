/*!
@file SpaceFOM/RefFrameDataState.cpp
@ingroup SpaceFOM
@brief A simple class that contains the date fields required to encode
and decode a SISO Space Reference FOM PhysicalEntity data type.

@copyright Copyright 2024 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{RefFrameDataState.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2024, --, Initial version }
@revs_end

*/

// C includes.
#include <cstring>
#include <sstream>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"

// Trick HLA includes.
#include "TrickHLA/CompileConfig.hh"

// SpaceFOM includes.
#include "SpaceFOM/RefFrameDataState.hh"

using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RefFrameDataState::RefFrameDataState()
{
   V_INIT( this->accel );
   V_INIT( this->ang_accel );
}

/*!
 * @job_class{initialization}
 */
RefFrameDataState::RefFrameDataState( RefFrameDataState const &source )
{
   this->copy( source );
}

/*!
 * @job_class{shutdown}
 */
RefFrameDataState::~RefFrameDataState()
{
   return;
}

/***********************************************************************
 * RefFrameDataState methods.
 ***********************************************************************/

/*!
 * @job_class{scheduled}
 */
RefFrameDataState &RefFrameDataState::operator=(
   RefFrameDataState const &rhs )
{
   copy( rhs );

   return ( *this );
}

/*!
 * @job_class{scheduled}
 */
void RefFrameDataState::copy( RefFrameDataState const &source )
{
   // Copy the state.
   this->state = source.state;

   // Copy the accelerations.
   for ( int iinc = 0; iinc < 3; iinc++ ) {
      this->accel[iinc]     = source.accel[iinc];
      this->ang_accel[iinc] = source.ang_accel[iinc];
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameDataState::initialize()
{
   state.initialize();
   V_INIT( this->accel );
   V_INIT( this->ang_accel );
   return;
}

/*!
 * @job_class{scheduled}
 */
bool RefFrameDataState::transform_to_parent(
   RefFrameDataState const &frame_to,
   RefFrameDataState       *frame_out )
{
   double r_frm_p[3]; /* Position vector of this frame with respect to its
                         current native frame (child) frame but expressed in
                         the desired parent frame. */
   double v_frm_p[3]; /* Velocity vector of this frame with respect to its
                         current native frame (child) frame but expressed in
                         the desired parent frame. */
   double a_frm_p[3]; /* Acceleration vector of this frame with respect to its
                         current native frame (child) frame but expressed in
                         the desired parent frame. */

   // Working variables.
   // Translation.
   double wxr_c[3];
   double v_c[3];
   double axr_c[3];
   double two_w_c[3];
   double two_wxv_c[3];
   double wxwxr_c[3];
   double a_c[3];
   // Rotation.
   double w_c_p_bdy[3];
   double wdot_c_p_bdy[3];

   // Check for null reference to transformed frame data.
   if ( frame_out == NULL ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameDataState::transform_to_parent() ERROR:%d NULL transformed frame reference!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
   }

   //**************************************************************************
   // Transform this frame expressed in its current native frame (child) into
   // a new desired frame (parent).
   // See the Reference Frame Transformations section of the SpaceFOM
   // (Appendix E).
   //**************************************************************************

   //**************************************************************************
   // Note: All frames are rotationally expressed using a left transformation
   // quaternion.  This transformation quaternion can be used to transform a
   // vector expressed in the native (child) frame into a vector expressed in
   // a frame's parent frame.
   //**************************************************************************

   //
   // Position computations.
   //
   // Transform this frame's position vector expressed in its current native
   // frame (child) into the desired parent frame.  This is still a vector
   // from the origin of the original frame (child) to this frame's origin
   // but expressed in the new parent frame coordinates.
   frame_to.state.att.transform_vector( this->state.pos, r_frm_p );

   // Compute this frame's position expressed in the new parent frame.
   V_ADD( frame_out->state.pos, frame_to.state.pos, r_frm_p )

   // Compute the attitude of this frame in the new parent frame.
   // Frame rotations and transformations can be accumulated through quaternion
   // multiplication.  Note q_02 = q_12 * q_01
   frame_out->state.att.multiply( frame_to.state.att, this->state.att );

   //
   // Velocity computations.
   //
   // Compute the apparent velocity of this frame in a rotating child frame.
   V_CROSS( wxr_c, frame_to.state.ang_vel, this->state.pos );

   // Compute the total velocity of this frame in the rotating child frame.
   V_ADD( v_c, this->state.vel, wxr_c );

   // Transform the frame velocity into the parent frame.
   frame_to.state.att.transform_vector( v_c, v_frm_p );

   // Compute this frame's velocity expressed in the parent frame.
   V_ADD( frame_out->state.vel, frame_to.state.vel, v_frm_p );

   // Compute this frame's angular velocity wrt the parent frame.
   // NOTE: Angular velocity is expressed in the 'body' frame, not the parent
   // frame.  Transform the child frame's angular velocity wrt the parent frame
   // into this new parent 'body' frame.
   state.att.conjugate_transform_vector( frame_to.state.ang_vel, w_c_p_bdy );
   // Add the rotational velocity of the this frame's child frame with respect
   // to the parent frame.
   V_ADD( frame_out->state.ang_vel, w_c_p_bdy, this->state.ang_vel );

   //
   // Acceleration computations.
   //
   // Compute the apparent acceleration of this frame in a rotating child frame.
   V_CROSS( axr_c, frame_to.ang_accel, this->state.pos );
   V_SCALE( two_w_c, frame_to.state.ang_vel, 2.0 );
   V_CROSS( two_wxv_c, two_w_c, this->state.vel );
   V_CROSS( wxwxr_c, frame_to.state.ang_vel, wxr_c );

   // Add up the components of the rotationally induced acceleration.
   a_c[0] = this->accel[0] + wxwxr_c[0] + two_wxv_c[0] + axr_c[0];
   a_c[1] = this->accel[1] + wxwxr_c[1] + two_wxv_c[1] + axr_c[1];
   a_c[2] = this->accel[2] + wxwxr_c[2] + two_wxv_c[2] + axr_c[2];

   // Transform this frame's acceleration into the parent frame.
   frame_to.state.att.transform_vector( a_c, a_frm_p );

   // Compute this frame's acceleration expressed in the parent frame.
   V_ADD( frame_out->accel, frame_to.accel, a_frm_p );

   // Compute this frame's angular acceleration wrt the parent frame.
   // NOTE: Angular acceleration is expressed in the 'body' frame, not the parent frame.
   // Transform the child frame's angular acceleration wrt the parent frame into
   // this 'body' frame.
   state.att.conjugate_transform_vector( frame_to.ang_accel, wdot_c_p_bdy );
   // Add the rotational acceleration of this frame's child frame with respect
   // to the parent frame.
   V_ADD( frame_out->ang_accel, wdot_c_p_bdy, this->ang_accel );

   return ( true );
}

/*!
 * @job_class{scheduled}
 */
bool RefFrameDataState::transform_to_child(
   RefFrameDataState const &frame_to,
   RefFrameDataState       *frame_out )
{
   // Working variables.
   // Translation.
   double dr_p[3];
   double dv_p[3];
   double dv_c[3];
   double wxr_c[3];
   double da_p[3];
   double da_c[3];
   double axr_c[3];
   double two_w_c[3];
   double two_wxv_c[3];
   double wxwxr_c[3];
   // Rotation.
   double w_c_bdy[3];
   double wdot_c_bdy[3];

   // Check for null reference to transformed frame data.
   if ( frame_out == NULL ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameDataState::transform_to_child() ERROR:%d NULL transformed frame reference!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
   }

   //**************************************************************************
   // Transform this frame expressed in its current parent frame into one of
   // its specified child frames.
   // See the Reference Frame Transformations section of the SpaceFOM
   // (Appendix E).
   //**************************************************************************

   //**************************************************************************
   // Note: All frames are rotationally expressed using a left transformation
   // quaternion.  This transformation quaternion can be used to transform a
   // vector expressed in the native (child) frame into a vector expressed in
   // a frame's parent frame.  As a consequence of quaternion mathematics, the
   // conjugate of the transformation quaternion can be used to transform a
   // vector expressed in the parent frame into a vector expressed in the
   // native (child) frame.
   //**************************************************************************

   //
   // Position computations.
   //
   // Compute the position difference between this frame and the child frame
   // expressed in parent frame coordinates.
   // FIXME: Is there a missing transformation here?
   V_SUB( dr_p, this->state.pos, frame_to.state.pos )

   // Transform the position vector into the child frame coordinates.
   frame_to.state.att.transform_vector( dr_p, frame_out->state.pos );

   // Compute the conjugate of the parent to child transformation quaternion.
   // This is the child to parent transformation quaternion.
   QuaternionData q_c_p;
   q_c_p.conjugate( frame_to.state.att );

   // Compute the attitude of this frame in the child frame.
   // Frame rotations and transformations can be accumulated through quaternion
   // multiplication.  Note q_02 = q_12 * q_01
   frame_out->state.att.multiply( q_c_p, this->state.att );

   //
   // Velocity computations.
   //
   // Compute the difference between this frame velocity and the child frame
   // velocity expressed in the parent frame.
   V_SUB( dv_p, this->state.vel, frame_to.state.vel );

   // Transform the velocity difference into the child frame.
   frame_to.state.att.transform_vector( dv_p, dv_c );

   // Compute the apparent velocity of this frame in a rotating child frame.
   V_CROSS( wxr_c, frame_to.state.ang_vel, frame_out->state.pos );

   // Compute the total velocity of this frame in the rotating child frame.
   V_SUB( frame_out->state.vel, dv_c, wxr_c );

   // Compute this frame's angular velocity wrt the child frame.
   // NOTE: Angular velocity is expressed in the 'body' frame, not the parent frame.
   // Transform the child frame's angular velocity wrt the parent frame into
   // this 'body' frame.
   frame_out->state.att.transform_vector( frame_to.state.ang_vel, w_c_bdy );

   // Subtract the rotational velocity of the child frame with respect to the
   // parent frame expressed in the body frame to the rotational velocity of
   // this  body frame with respect to the parent expressed in the body frame.
   V_SUB( frame_out->state.ang_vel, this->state.ang_vel, w_c_bdy );

   //
   // Acceleration computations.
   //
   // Compute the difference between the parent frame acceleration and the
   // child frame acceleration expressed in the parent frame.
   V_SUB( da_p, this->accel, frame_to.accel );

   // Transform the acceleration difference into the child frame.
   frame_to.state.att.transform_vector( da_p, da_c );

   // Compute the apparent acceleration of this frame in a rotating child frame.
   V_CROSS( axr_c, frame_to.ang_accel, frame_out->state.pos );
   V_SCALE( two_w_c, frame_to.state.ang_vel, 2.0 );
   V_CROSS( two_wxv_c, two_w_c, frame_out->state.vel );
   V_CROSS( wxwxr_c, frame_to.state.ang_vel, wxr_c );

   // Add up the components of the rotationally induced acceleration.
   frame_out->accel[0] = da_c[0] - wxwxr_c[0] - two_wxv_c[0] - axr_c[0];
   frame_out->accel[1] = da_c[1] - wxwxr_c[1] - two_wxv_c[1] - axr_c[1];
   frame_out->accel[2] = da_c[2] - wxwxr_c[2] - two_wxv_c[2] - axr_c[2];

   // Compute this frame's angular acceleration wrt the child frame.
   // NOTE: Angular acceleration is expressed in the 'body' frame, not the parent frame.
   // Transform the child frame's angular acceleration wrt the parent frame into
   // this 'body' frame.
   frame_out->state.att.transform_vector( frame_to.ang_accel, wdot_c_bdy );
   // Subtract the rotational acceleration of this frame's child frame with
   // respect to the parent frame expressed in the body frame.
   V_SUB( frame_out->ang_accel, this->ang_accel, wdot_c_bdy );

   return ( true );
}

/*!
 * @job_class{scheduled}
 */
void RefFrameDataState::print_data( std::ostream &stream ) const
{
   // Set the print precision.
   stream.precision( 15 );

   stream << "\tposition: "
          << "\t\t" << state.pos[0] << ", "
          << "\t\t" << state.pos[1] << ", "
          << "\t\t" << state.pos[2] << '\n';
   stream << "\tvelocity: "
          << "\t\t" << state.vel[0] << ", "
          << "\t\t" << state.vel[1] << ", "
          << "\t\t" << state.vel[2] << '\n';
   stream << "\tacceleration: "
          << "\t\t" << accel[0] << ", "
          << "\t\t" << accel[1] << ", "
          << "\t\t" << accel[2] << '\n';
   state.att.print_data( stream );
   stream << "\tangular velocity: "
          << "\t\t" << state.ang_vel[0] << ", "
          << "\t\t" << state.ang_vel[1] << ", "
          << "\t\t" << state.ang_vel[2] << '\n';
   stream << "\tangular acceleration: "
          << "\t\t" << ang_accel[0] << ", "
          << "\t\t" << ang_accel[1] << ", "
          << "\t\t" << ang_accel[2] << '\n';

   return;
}
