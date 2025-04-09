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
   return;
}

/*!
 * @job_class{initialization}
 */
RefFrameData::RefFrameData( RefFrameData const &source )
   : RefFrameDataState( source ),
     name( NULL ),
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
         message_publish( MSG_WARNING, "SpaceFOM::RefFrameData::~RefFrameData():%d ERROR deleting Trick Memory for 'this->name'\n",
                          __LINE__ );
      }
      this->name = NULL;
   }
   if ( this->parent_name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->parent_name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::RefFrameData::~RefFrameData():%d ERROR deleting Trick Memory for 'this->parent_name'\n",
                          __LINE__ );
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
   if ( this->name != NULL ) {
      trick_MM->delete_var( static_cast< void * >( this->name ) );
   }
   if ( source.name != NULL ) {
      this->name = trick_MM->mm_strdup( source.name );
   } else {
      this->name = NULL;
   }

   if ( this->parent_name != NULL ) {
      trick_MM->delete_var( static_cast< void * >( this->parent_name ) );
   }
   if ( source.parent_name != NULL ) {
      this->parent_name = trick_MM->mm_strdup( source.parent_name );
   } else {
      this->parent_name = NULL;
   }

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
 * @job_class{initialization}
 */
void RefFrameData::set_name( char const *new_name )
{
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::RefFrameData::set_name():%d ERROR deleting Trick Memory for 'this->name'\n",
                          __LINE__ );
      }
   }
   this->name = trick_MM->mm_strdup( new_name );
   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameData::set_parent_name( char const *name )
{
   // Set the parent frame name appropriately.
   if ( this->parent_name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->parent_name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::RefFrameData::set_parent_name():%d ERROR deleting Trick Memory for 'this->name'\n",
                          __LINE__ );
      }
   }
   if ( name != NULL ) {
      this->parent_name = trick_MM->mm_strdup( name );
   }

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
      errmsg << "SpaceFOM::RefFrameData::transform_to_parent() ERROR:%d NULL transformed frame reference!" << std::endl;
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
   if ( strcmp( this->name, frame_out->name ) ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_parent() ERROR:%d Frame name mismatch: %s/%s!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, this->name, frame_out->name );
      return ( false );
   }
   if ( strcmp( this->parent_name, frame_to.name ) ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_parent() ERROR:%d Frame incompatibility: %s/%s!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, this->parent_name, frame_to.name );
      return ( false );
   }
   if ( strcmp( frame_to.parent_name, frame_out->parent_name ) ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_parent() ERROR:%d Frame parent: %s/%s!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, frame_to.parent_name, frame_out->parent_name );
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
      errmsg << "SpaceFOM::RefFrameData::transform_to_child() ERROR:%d NULL transformed frame reference!" << std::endl;
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
   if ( strcmp( this->name, frame_out->name ) ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_child() ERROR:%d Frame name mismatch: %s/%s!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, this->name, frame_out->name );
      return ( false );
   }
   if ( strcmp( this->name, frame_to.parent_name ) ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_child() ERROR:%d Frame incompatibility: %s/%s!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, this->name, frame_to.parent_name );
      return ( false );
   }
   if ( strcmp( frame_to.name, frame_out->parent_name ) ) {
      std::ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameData::transform_to_child() ERROR:%d Frame parent: %s/%s!" << std::endl;
      message_publish( MSG_WARNING, errmsg.str().c_str(), __LINE__, frame_to.name, frame_out->parent_name );
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

   stream << "\tname:   '" << ( name != NULL ? name : "" ) << "'\n"
          << "\tparent: '" << ( parent_name != NULL ? parent_name : "" ) << "'\n";
   RefFrameDataState::print_data( stream );

   return;
}
