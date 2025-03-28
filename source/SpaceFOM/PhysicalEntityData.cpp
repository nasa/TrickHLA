/*!
@file SpaceFOM/PhysicalEntityData.cpp
@ingroup SpaceFOM
@brief A simple structure that contains the date fields required to encode
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
@trick_link_dependency{PhysicalEntityData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2024, --, Initial version }
@revs_end

*/

// C includes.
#include <string.h>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"

// Trick HLA includes.
#include "TrickHLA/CompileConfig.hh"

// SpaceFOM includes.
#include "SpaceFOM/PhysicalEntityData.hh"

using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityData::PhysicalEntityData()
   : name( NULL ),
     type( NULL ),
     status( NULL ),
     parent_frame( NULL )
{
   for ( unsigned int iinc = 0; iinc < 3; iinc++ ) {
      this->accel[iinc]     = 0.0;
      this->ang_accel[iinc] = 0.0;
      this->cm[iinc]        = 0.0;
   }
}

/*!
 * @job_class{initialization}
 */
PhysicalEntityData::PhysicalEntityData( PhysicalEntityData const &source )
   : name( NULL ),
     type( NULL ),
     status( NULL ),
     parent_frame( NULL )
{
   this->copy( source );
}

/*!
 * @job_class{shutdown}
 */
PhysicalEntityData::~PhysicalEntityData()
{
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalEntityData::~PhysicalEntityData():%d ERROR deleting Trick Memory for 'this->name'\n",
                          __LINE__ );
      }
      this->name = NULL;
   }
   if ( this->type != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->type ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalEntityData::~PhysicalEntityData():%d ERROR deleting Trick Memory for 'this->type'\n",
                          __LINE__ );
      }
      this->type = NULL;
   }
   if ( this->status != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->status ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalEntityData::~PhysicalEntityData():%d ERROR deleting Trick Memory for 'this->status'\n",
                          __LINE__ );
      }
      this->status = NULL;
   }
   if ( this->parent_frame != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->parent_frame ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalEntityData::~PhysicalEntityData():%d ERROR deleting Trick Memory for 'this->parent_frame'\n",
                          __LINE__ );
      }
      this->parent_frame = NULL;
   }
}

/***********************************************************************
 * PhysicalEntityData methods.
 ***********************************************************************/

/*!
 * @job_class{scheduled}
 */
PhysicalEntityData &PhysicalEntityData::operator=(
   PhysicalEntityData const &rhs )
{
   copy( rhs );

   return ( *this );
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntityData::copy( PhysicalEntityData const &source )
{
   // Copy the string based parameters.
   if ( this->name != NULL ) {
      trick_MM->delete_var( static_cast< void * >( this->name ) );
   }
   if ( source.name != NULL ) {
      this->name = trick_MM->mm_strdup( source.name );
   } else {
      this->name = NULL;
   }

   if ( this->type != NULL ) {
      trick_MM->delete_var( static_cast< void * >( this->type ) );
   }
   if ( source.type != NULL ) {
      this->type = trick_MM->mm_strdup( source.type );
   } else {
      this->type = NULL;
   }

   if ( this->status != NULL ) {
      trick_MM->delete_var( static_cast< void * >( this->status ) );
   }
   if ( source.status != NULL ) {
      this->status = trick_MM->mm_strdup( source.status );
   } else {
      this->status = NULL;
   }

   if ( this->parent_frame != NULL ) {
      trick_MM->delete_var( static_cast< void * >( this->parent_frame ) );
   }
   if ( source.parent_frame != NULL ) {
      this->parent_frame = trick_MM->mm_strdup( source.parent_frame );
   } else {
      this->parent_frame = NULL;
   }

   // Copy the state.
   this->state = source.state;

   // Copy the accelerations and CM vectors.
   for ( int iinc = 0; iinc < 3; iinc++ ) {
      this->accel[iinc]     = source.accel[iinc];
      this->ang_accel[iinc] = source.ang_accel[iinc];
      this->cm[iinc]        = source.cm[iinc];
   }

   // Copy the body attitude quaternion.
   this->body_wrt_struct = source.body_wrt_struct;

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityData::set_name( char const *new_name )
{
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalEntityData::set_name():%d WARNING deleting Trick Memory for 'this->name'\n",
                          __LINE__ );
      }
   }
   name = trick_MM->mm_strdup( new_name );
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityData::set_type( char const *new_type )
{
   if ( this->type != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->type ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalEntityData::set_type():%d WARNING deleting Trick Memory for 'this->type'\n",
                          __LINE__ );
      }
   }
   type = trick_MM->mm_strdup( new_type );
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityData::set_status( char const *new_status )
{
   if ( this->status != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->status ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalEntityData::set_status():%d WARNING deleting Trick Memory for 'this->status'\n",
                          __LINE__ );
      }
   }
   this->status = trick_MM->mm_strdup( new_status );
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityData::set_parent_frame( char const *new_frame )
{
   if ( this->parent_frame != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->parent_frame ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalEntityData::set_parent_frame():%d WARNING deleting Trick Memory for 'this->parent_frame'\n",
                          __LINE__ );
      }
   }
   parent_frame = trick_MM->mm_strdup( new_frame );

   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntityData::print_data( std::ostream &stream ) const
{

   // Set the print precision.
   stream.precision( 15 );

   stream << "\tname:         '" << ( name != NULL ? name : "" ) << "'\n"
          << "\ttype:         '" << ( type != NULL ? type : "" ) << "'\n"
          << "\tstatus:       '" << ( status != NULL ? status : "" ) << "'\n"
          << "\tparent_frame: '" << ( parent_frame != NULL ? parent_frame : "" ) << "'\n";

   state.print_data( stream );

   stream << "\tacceleration: "
          << "\t\t" << accel[0] << ", "
          << "\t\t" << accel[1] << ", "
          << "\t\t" << accel[2] << '\n';

   stream << "\tangular acceleration: "
          << "\t\t" << ang_accel[0] << ", "
          << "\t\t" << ang_accel[1] << ", "
          << "\t\t" << ang_accel[2] << '\n';

   stream << "\tcenter of mass (cm): "
          << "\t\t" << cm[0] << ", "
          << "\t\t" << cm[1] << ", "
          << "\t\t" << cm[2] << '\n';

   stream << "\tBody frame orientation:\n";
   body_wrt_struct.print_data();

   return;
}
