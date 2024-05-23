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
#include <string.h>

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
