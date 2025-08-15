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

// System includes.
#include <cstring>
#include <ostream>
#include <sstream>

// Trick includes.
#include "trick/message_proto.h"
#include "trick/message_type.h"

// SpaceFOM includes.
#include "SpaceFOM/RefFrameData.hh"
#include "SpaceFOM/RefFrameDataState.hh"

using namespace std;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RefFrameData::RefFrameData()
{
   return;
}

/*!
 * @job_class{initialization}
 */
RefFrameData::RefFrameData( RefFrameData const &source )
   : RefFrameDataState( source )
{
   this->copy( source ); // NOLINT
}

/*!
 * @job_class{shutdown}
 */
RefFrameData::~RefFrameData()
{
   return;
}

/***********************************************************************
 * RefFrameData methods.
 ***********************************************************************/

/*!
 * @job_class{scheduled}
 */
RefFrameData &RefFrameData::operator=(
   RefFrameData const &rhs )
{
   copy( rhs );

   return ( *this );
}

/*!
 * @job_class{scheduled}
 */
void RefFrameData::copy( RefFrameData const &source )
{
   // Copy the names.
   this->name        = source.name;
   this->parent_name = source.parent_name;

   // Copy the reference frame data state.
   RefFrameDataState::copy( source );

   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameData::initialize()
{
   // Initialize the reference frame data state.
   RefFrameDataState::initialize();

   return;
}

/*!
 * @job_class{scheduled}
 */
bool RefFrameData::transform_to_parent(
   RefFrameData const &frame_to,
   RefFrameData       *frame_out )
{
   // Check for null reference to transformed frame data.
   if ( frame_out == NULL ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_parent() ERROR:%d NULL transformed frame reference!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
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
   if ( this->name != frame_out->name ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_parent() ERROR:%d Frame name mismatch: %s/%s!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, this->name.c_str(), frame_out->name.c_str() );
      return ( false );
   }
   if ( this->parent_name != frame_to.name ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_parent() ERROR:%d Frame incompatibility: %s/%s!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, this->parent_name.c_str(), frame_to.name.c_str() );
      return ( false );
   }
   if ( frame_to.parent_name != frame_out->parent_name ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_parent() ERROR:%d Frame parent: %s/%s!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, frame_to.parent_name.c_str(), frame_out->parent_name.c_str() );
      return ( false );
   }

   // Call the reference frame data state version of this function.
   return ( RefFrameDataState::transform_to_parent( frame_to, frame_out ) );
}

/*!
 * @job_class{scheduled}
 */
bool RefFrameData::transform_to_child(
   RefFrameData const &frame_to,
   RefFrameData       *frame_out )
{
   // Check for null reference to transformed frame data.
   if ( frame_out == NULL ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_child() ERROR:%d NULL transformed frame reference!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__ );
      return ( false );
   }

   //**************************************************************************
   // Transform this frame expressed in its current parent frame into one of
   // its specified child frames.
   // See the Reference Frame Transformations section of the SpaceFOM
   // (Appendix E).
   //**************************************************************************

   // Check for frame compatibility.
   if ( this->name != frame_out->name ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_child() ERROR:%d Frame name mismatch: %s/%s!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, this->name.c_str(), frame_out->name.c_str() );
      return ( false );
   }
   if ( this->name != frame_to.parent_name ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_child() ERROR:%d Frame incompatibility: %s/%s!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, this->name.c_str(), frame_to.parent_name.c_str() );
      return ( false );
   }
   if ( frame_to.name != frame_out->parent_name ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_child() ERROR:%d Frame parent: %s/%s!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, frame_to.name.c_str(), frame_out->parent_name.c_str() );
      return ( false );
   }

   // Call the reference frame data state version of this function.
   return ( RefFrameDataState::transform_to_child( frame_to, frame_out ) );
}

/*!
 * @job_class{scheduled}
 */
void RefFrameData::print_data( std::ostream &stream ) const
{
   // Set the print precision.
   stream.precision( 15 );

   stream << "\tname:   '" << name << "'" << endl
          << "\tparent: '" << parent_name << "'" << endl;
   RefFrameDataState::print_data( stream );

   return;
}
