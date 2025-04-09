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
   double r_frm_e[3]; /* Position vector of this frame with respect to its
                         current native frame (child) frame but expressed in
                         the desired express frame. */
   double v_frm_e[3]; /* Velocity vector of this frame with respect to its
                         current native frame (child) frame but expressed in
                         the desired express frame. */
   double a_frm_e[3]; /* Acceleration vector of this frame with respect to its
                         current native frame (child) frame but expressed in
                         the desired express frame. */

   // Working variables.
   // Translation.
   double wxr_t[3];
   double v_t[3];
   double axr_t[3];
   double two_w_t[3];
   double two_wxv_t[3];
   double wxwxr_t[3];
   double a_t[3];
   // Rotation.
   double w_e_t[3];
   double wdot_e_t[3];

   // Check for null reference to transformed frame data.
   if ( frame_out == NULL ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameDataState::transform_to_parent() ERROR:%d NULL transformed frame reference!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
   }
   // You cannot call this function on itself.
   if ( frame_out == this ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameDataState::transform_to_parent() ERROR:%d Computation on itself is not allowed!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
   }
   // You cannot write into the 'to' frame.
   if ( frame_out == &frame_to ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameDataState::transform_to_parent() ERROR:%d Not safe to compute into the 'to' frame!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
   }

   //**************************************************************************
   // Transform this frame expressed in its current native frame into another
   // specified frame.  Here, it is assumed that 'to' frame is the parent of
   // this frame.
   // See the Reference Frame Transformations section of the SpaceFOM
   // (Appendix E - E.4).
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
   // frame into the desired express frame.  This is still a vector
   // from the origin of the original parent frame to this frame's origin
   // but expressed in the express frame coordinates.
   frame_to.state.att.transform_vector( this->state.pos, r_frm_e );

   // Compute the output frame's position expressed in the express frame.
   V_ADD( frame_out->state.pos, frame_to.state.pos, r_frm_e )

   // Compute the attitude of this frame in the express frame.
   // Frame rotations and transformations can be accumulated through quaternion
   // multiplication.  Note q_02 = q_12 * q_01
   frame_out->state.att.multiply( frame_to.state.att, this->state.att );

   //
   // Velocity computations.
   //
   // Compute the apparent velocity of this frame with respect to the rotating
   // express frame but expressed in this frame.
   V_CROSS( wxr_t, frame_to.state.ang_vel, this->state.pos );

   // Add this frame's velocity to the apparent rotational velocity.
   V_ADD( v_t, this->state.vel, wxr_t );

   // Transform the combined velocity into the express frame.
   frame_to.state.att.transform_vector( v_t, v_frm_e );

   // Compute the output frame's total velocity in the express frame.
   V_ADD( frame_out->state.vel, frame_to.state.vel, v_frm_e );

   // Compute this frame's angular velocity wrt the express frame.
   // NOTE: Angular velocity is expressed in the 'body' frame, not the parent
   // frame.  Transform the express frame's angular velocity into this 'body'
   // frame.
   state.att.conjugate_transform_vector( frame_to.state.ang_vel, w_e_t );
   // Add the rotational velocity of the this frame to the angular velocity
   // of the express frame expressed in this frame.
   V_ADD( frame_out->state.ang_vel, w_e_t, this->state.ang_vel );

   //
   // Acceleration computations.
   //
   // Compute the apparent acceleration of this frame in a rotating child frame.
   V_CROSS( axr_t, frame_to.ang_accel, this->state.pos );
   V_SCALE( two_w_t, frame_to.state.ang_vel, 2.0 );
   V_CROSS( two_wxv_t, two_w_t, this->state.vel );
   V_CROSS( wxwxr_t, frame_to.state.ang_vel, wxr_t );

   // Add up the components of the rotationally induced acceleration.
   a_t[0] = this->accel[0] + wxwxr_t[0] + two_wxv_t[0] + axr_t[0];
   a_t[1] = this->accel[1] + wxwxr_t[1] + two_wxv_t[1] + axr_t[1];
   a_t[2] = this->accel[2] + wxwxr_t[2] + two_wxv_t[2] + axr_t[2];

   // Transform this frame's acceleration into the parent frame.
   frame_to.state.att.transform_vector( a_t, a_frm_e );

   // Compute this frame's acceleration expressed in the parent frame.
   V_ADD( frame_out->accel, frame_to.accel, a_frm_e );

   // Compute this frame's angular acceleration wrt the parent frame.
   // NOTE: Angular acceleration is expressed in the 'body' frame, not the parent frame.
   // Transform the child frame's angular acceleration wrt the parent frame into
   // this 'body' frame.
   state.att.conjugate_transform_vector( frame_to.ang_accel, wdot_e_t );
   // Add the rotational acceleration of this frame's child frame with respect
   // to the parent frame.
   V_ADD( frame_out->ang_accel, wdot_e_t, this->ang_accel );

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
   double dr_t[3];
   double dv_t[3];
   double dv_e[3];
   double wxr_e[3];
   double da_t[3];
   double da_e[3];
   double axr_e[3];
   double two_w_e[3];
   double two_wxv_e[3];
   double wxwxr_e[3];
   // Rotation.
   double w_e_bdy[3];
   double wdot_e_bdy[3];

   // Check for null reference to transformed frame data.
   if ( frame_out == NULL ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameDataState::transform_to_child() ERROR:%d NULL transformed frame reference!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
   }
   // You cannot call this function on itself.
   if ( frame_out == this ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameDataState::transform_to_parent() ERROR:%d Computation on itself is not allowed!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
   }
   // You cannot write into the 'to' frame.
   if ( frame_out == &frame_to ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameDataState::transform_to_parent() ERROR:%d Not safe to compute into the 'to' frame!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
   }

   //**************************************************************************
   // Transform this frame expressed in its current native frame into another
   // specified frame.  Here, it is assumed that 'to' frame is the child of
   // this frame.
   // See the Reference Frame Transformations section of the SpaceFOM
   // (Appendix E - E.4.2).
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
   // Compute the position vector from this frame to the desired output frame
   // expressed in this frame's parent frame.
   V_SUB( dr_t, this->state.pos, frame_to.state.pos )

   // Transform the position vector from this frame's parent frame into the
   // desired express frame.
   frame_to.state.att.transform_vector( dr_t, frame_out->state.pos );

   // Compute the conjugate of the express frame transformation quaternion.
   // Note that the express frame's parent is this frame or we would not be
   // calling this function.  The conjugate of attitude quaternion of the
   // express frame with respect to its parent frame (this) is the same
   // as the attitude quaterion of this frame with respect to the express frame.
   QuaternionData q_c_p;
   q_c_p.conjugate( frame_to.state.att );

   // Compute the attitude of the output frame in the express frame.
   // Frame rotations and transformations can be accumulated through quaternion
   // multiplication.  Note q_02 = q_12 * q_01
   frame_out->state.att.multiply( q_c_p, this->state.att );

   //
   // Velocity computations.
   //
   // Compute the difference between this frame's velocity and the express
   // frame's velocity expressed in this frame.
   V_SUB( dv_t, this->state.vel, frame_to.state.vel );

   // Transform the velocity difference into the express frame.
   frame_to.state.att.transform_vector( dv_t, dv_e );

   // Compute the apparent velocity of this frame in a rotating express frame.
   V_CROSS( wxr_e, frame_to.state.ang_vel, frame_out->state.pos );

   // Compute the total velocity of output frame in the rotating express frame.
   V_SUB( frame_out->state.vel, dv_e, wxr_e );

   // Compute output frame's angular velocity.
   // NOTE: Angular velocity is expressed in the 'body' frame, not the parent
   // frame.  Transform the express frame's angular velocity into the output
   // 'body' frame.
   frame_out->state.att.transform_vector( frame_to.state.ang_vel, w_e_bdy );

   // Note that the angular velocity of a parent frame wrt to its child is the
   // negative of the angular velocity of a child frame wrt its parent.
   // Therefore, subtract the rotational velocity of the express frame
   // expressed in the output frame (w_e_bdy) from the angular velocity of
   // this frame, which is already in the output body frame.
   V_SUB( frame_out->state.ang_vel, this->state.ang_vel, w_e_bdy );

   //
   // Acceleration computations.
   //
   // Compute the difference between this frame's acceleration and the express
   // frame's acceleration expressed in this frame.
   V_SUB( da_t, this->accel, frame_to.accel );

   // Transform the acceleration difference into the express frame.
   frame_to.state.att.transform_vector( da_t, da_e );

   // Compute the apparent acceleration of the output frame in a rotating
   // express frame.
   V_CROSS( axr_e, frame_to.ang_accel, frame_out->state.pos );
   V_SCALE( two_w_e, frame_to.state.ang_vel, 2.0 );
   V_CROSS( two_wxv_e, two_w_e, frame_out->state.vel );
   V_CROSS( wxwxr_e, frame_to.state.ang_vel, wxr_e );

   // Add up the components of the rotationally induced acceleration.
   frame_out->accel[0] = da_e[0] - wxwxr_e[0] - two_wxv_e[0] - axr_e[0];
   frame_out->accel[1] = da_e[1] - wxwxr_e[1] - two_wxv_e[1] - axr_e[1];
   frame_out->accel[2] = da_e[2] - wxwxr_e[2] - two_wxv_e[2] - axr_e[2];

   // Compute output frame's angular acceleration.
   // NOTE: Angular acceleration is expressed in the 'body' frame, not the
   // parent frame.  Transform the express frame's angular acceleration into
   // the output 'body' frame.
   frame_out->state.att.transform_vector( frame_to.ang_accel, wdot_e_bdy );

   // Note that the angular acceleration of a parent frame wrt to its child is
   // the negative of the angular acceleration of a child frame wrt its parent.
   // Therefore, subtract the rotational acceleration of the express frame
   // expressed in the output frame (wdot_e_bdy) from the angular acceleration
   // of this frame, which is already in the output body frame.
   V_SUB( frame_out->ang_accel, this->ang_accel, wdot_e_bdy );

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
