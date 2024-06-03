/*!
@file SpaceFOM/RefFrameData.cpp
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
@trick_link_dependency{RefFrameData.cpp}

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
#include "SpaceFOM/RefFrameData.hh"

using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RefFrameData::RefFrameData()
: name( NULL ),
  parent_name( NULL )
{
   V_INIT( this->accel );
   V_INIT( this->ang_accel );
}

/*!
 * @job_class{initialization}
 */
RefFrameData::RefFrameData( const RefFrameData &source )
: name( NULL ),
  parent_name( NULL )
{
   this->copy( source );
}

/*!
 * @job_class{shutdown}
 */
RefFrameData::~RefFrameData()
{
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         send_hs( stderr, "SpaceFOM::RefFrameData::~RefFrameData():%d ERROR deleting Trick Memory for 'this->name'%c",
                  __LINE__, THLA_NEWLINE );
      }
      this->name = NULL;
   }
   if ( this->parent_name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->parent_name ) ) ) {
         send_hs( stderr, "SpaceFOM::RefFrameData::~RefFrameData():%d ERROR deleting Trick Memory for 'this->parent_name'%c",
                  __LINE__, THLA_NEWLINE );
      }
      this->parent_name = NULL;
   }

}

/***********************************************************************
 * RefFrameData methods.
 ***********************************************************************/

/*!
 * @job_class{scheduled}
 */
RefFrameData &RefFrameData::operator=(
   const RefFrameData &rhs )
{

   this->copy( rhs );

   return ( *this );
}

/*!
 * @job_class{scheduled}
 */
void RefFrameData::copy( const RefFrameData &source )
{
   // Copy the names.
   if ( this->name != NULL ){
      trick_MM->delete_var( static_cast< void * >( this->name ) );
   }
   if ( source.name != NULL ) {
      this->name = trick_MM->mm_strdup( source.name );
   }
   else{
      this->name = NULL;
   }

   if ( this->parent_name != NULL ){
      trick_MM->delete_var( static_cast< void * >( this->parent_name ) );
   }
   if ( source.parent_name != NULL ) {
      this->parent_name = trick_MM->mm_strdup( source.parent_name );
   }
   else{
      this->parent_name = NULL;
   }

   // Copy the state.
   this->state = source.state;

   // Copy the accelerations.
   for( int iinc = 0 ; iinc < 3 ; iinc++ ){
      this->accel[iinc]     = source.accel[iinc];
      this->ang_accel[iinc] = source.ang_accel[iinc];
   }

   return;
}


/*!
 * @job_class{scheduled}
 */
void RefFrameData::initialize()
{
   state.initialize();
   V_INIT( this->accel );
   V_INIT( this->ang_accel );
   return;
}


/*!
 * @job_class{scheduled}
 */
bool RefFrameData::transform_to_parent(
   RefFrameData const & transform_c_p,
   RefFrameData       * frame_p )
{

   double r_frm_c_p[3]; /* Position vector of this frame with respect to its
                           child frame but expressed in the desired parent frame. */
   double v_frm_c_p[3]; /* Velocity vector of this frame with respect to its
                           child frame but expressed in the desired parent frame. */
   double a_frm_c_p[3]; /* Acceleration vector of this frame with respect to its
                           child frame but expressed in the desired parent frame. */

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
   if ( frame_p == NULL ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform() ERROR:%d NULL transformed frame reference!" << std::endl;
      send_hs( stderr, errmsg.str().c_str(), __LINE__ );
      return( false );
   }

   //**************************************************************************
   // Transform this frame expressed in its current frame (child) into a new
   // desired frame (parent).  Note that the current frame must match the name
   // of the child to parent frame transformation.
   // See the Reference Frame Transformations section of the SpaceFOM
   // (Appendix E).
   //**************************************************************************

   // Check for frame compatibility.  The parent frame name for this frame
   // must match the name of the transformation.  The parent frame name for
   // the transformation will indicate the new parent frame for the transformed
   // frame.
   if ( strcmp( this->name, frame_p->name ) ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform() ERROR:%d Frame name mismatch: %s/%s!" << std::endl;
      send_hs( stderr, errmsg.str().c_str(), __LINE__, this->name, frame_p->name );
      return( false );
   }
   if ( strcmp( this->parent_name, transform_c_p.name ) ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform() ERROR:%d Frame incompatibility: %s/%s!" << std::endl;
      send_hs( stderr, errmsg.str().c_str(), __LINE__, this->parent_name, transform_c_p.name );
      return( false );
   }
   if ( strcmp( transform_c_p.parent_name, frame_p->parent_name ) ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform() ERROR:%d Frame parent: %s/%s!" << std::endl;
      send_hs( stderr, errmsg.str().c_str(), __LINE__, transform_c_p.parent_name, frame_p->parent_name );
      return( false );
   }

   //
   // Position computations.
   //
   // Transform this frame's position vector expressed in its current frame
   // (child) into the desired parent frame.  This is still a vector from
   // the origin of the original frame (child) to this frame's origin but
   // expressed in the new parent frame coordinates.
   transform_c_p.state.att.transform_vector( this->state.pos, r_frm_c_p );

   // Compute this frame's position expressed in the new parent frame.
   V_ADD( frame_p->state.pos, transform_c_p.state.pos, r_frm_c_p )

   // Compute the attitude of this frame in the new parent frame.
   frame_p->state.att.multiply( transform_c_p.state.att, this->state.att );

   //
   // Velocity computations.
   //
   // Compute the apparent velocity of this frame in a rotating child frame.
   V_CROSS( wxr_c, transform_c_p.state.ang_vel, this->state.pos );

   // Compute the total velocity of this frame in the rotating child frame.
   V_ADD( v_c, this->state.vel, wxr_c );

   // Transform the frame velocity into the parent frame.
   transform_c_p.state.att.transform_vector( v_c, v_frm_c_p );

   // Compute this frame's velocity expressed in the parent frame.
   V_ADD( frame_p->state.vel, transform_c_p.state.vel, v_frm_c_p );

   // Compute this frame's angular velocity wrt the parent frame.
   // NOTE: Angular velocity is expressed in the 'body' frame, not the parent frame.
   // Transform the child frame's angular velocity wrt the parent frame into
   // this 'body' frame.
   this->state.att.conjugate_transform_vector( transform_c_p.state.ang_vel, w_c_p_bdy );
   // Add the rotational velocity of the this frame's child frame with respect
   // to the parent frame.
   V_ADD( frame_p->state.ang_vel, w_c_p_bdy, this->state.ang_vel );

   //
   // Acceleration computations.
   //
   // Compute the apparent acceleration of this frame in a rotating child frame.
   V_CROSS( axr_c, transform_c_p.ang_accel, this->state.pos );
   V_SCALE( two_w_c, transform_c_p.state.ang_vel, 2.0 );
   V_CROSS( two_wxv_c, two_w_c, this->state.vel );
   V_CROSS( wxwxr_c, transform_c_p.state.ang_vel, wxr_c );

   // Add up the components of the rotationally induced acceleration.
   a_c[0] = this->accel[0] + wxwxr_c[0] + two_wxv_c[0] + axr_c[0];
   a_c[1] = this->accel[1] + wxwxr_c[1] + two_wxv_c[1] + axr_c[1];
   a_c[2] = this->accel[2] + wxwxr_c[2] + two_wxv_c[2] + axr_c[2];

   // Transform this frame's acceleration into the parent frame.
   transform_c_p.state.att.transform_vector( a_c, a_frm_c_p );

   // Compute this frame's acceleration expressed in the parent frame.
   V_ADD( frame_p->accel, transform_c_p.accel, a_frm_c_p );

   // Compute this frame's angular acceleration wrt the parent frame.
   // NOTE: Angular acceleration is expressed in the 'body' frame, not the parent frame.
   // Transform the child frame's angular acceleration wrt the parent frame into
   // this 'body' frame.
   this->state.att.conjugate_transform_vector( transform_c_p.state.ang_vel, wdot_c_p_bdy );
   // Add the rotational acceleration of this frame's child frame with respect
   // to the parent frame.
   V_ADD( frame_p->ang_accel, wdot_c_p_bdy, this->ang_accel );

   return( true );

}


/*!
 * @job_class{scheduled}
 */
bool RefFrameData::transform_to_child(
   RefFrameData const & transform_c_p,
   RefFrameData       * frame_c )
{

   double r_frm_c_p[3]; /* Position vector of this frame with respect to its
                           child frame but expressed in the desired parent frame. */
   double v_frm_c_p[3]; /* Velocity vector of this frame with respect to its
                           child frame but expressed in the desired parent frame. */
   double a_frm_c_p[3]; /* Acceleration vector of this frame with respect to its
                           child frame but expressed in the desired parent frame. */

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
   if ( frame_c == NULL ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform() ERROR:%d NULL transformed frame reference!" << std::endl;
      send_hs( stderr, errmsg.str().c_str(), __LINE__ );
      return( false );
   }

   // FIXME: Work to do here.

   return( false );

}
