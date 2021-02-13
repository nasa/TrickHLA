/*!
@file SpaceFOM/PhysicalEntityBase.cpp
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM Reference Frames.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../TrickHLA/CompileConfig.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{PhysicalEntityBase.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, SISO, Sept 2010, --, Smackdown implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <iostream>
#include <limits>
#include <math.h>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/matrix_macros.h"
#include "trick/message_proto.h"
#include "trick/vector_macros.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityBase.hh"

using namespace std;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityBase::PhysicalEntityBase() // RETURN: -- None.
   : name( NULL ),
     type( NULL ),
     status( NULL ),
     parent_ref_frame( NULL ),
     state( stc_encoder.get_data() ),
     body_wrt_struct( quat_encoder.get_data() )
{
   V_INIT( accel );
   V_INIT( rot_accel );
   V_INIT( cm );
}

/*!
 * @job_class{shutdown}
 */
PhysicalEntityBase::~PhysicalEntityBase() // RETURN: -- None.
{
   if ( this->name != (char *)NULL ) {
      trick_MM->delete_var( (void *)this->name );
      this->name = (char *)NULL;
   }
   if ( this->type != (char *)NULL ) {
      trick_MM->delete_var( (void *)this->type );
      this->type = (char *)NULL;
   }
   if ( this->status != (char *)NULL ) {
      trick_MM->delete_var( (void *)this->status );
      this->status = (char *)NULL;
   }
   if ( this->parent_ref_frame != (char *)NULL ) {
      trick_MM->delete_var( (void *)this->parent_ref_frame );
      this->parent_ref_frame = (char *)NULL;
   }
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityBase::initialize()
{
   ostringstream errmsg;

   // Must have federation instance name.
   if ( this->name == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntityBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL federation instance frame name!"
             << "  Setting frame name to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      this->name = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance type.
   if ( this->type == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntityBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL federation instance type!"
             << "  Setting type to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      this->type = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance status.
   if ( this->status == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntityBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL federation instance status!"
             << "  Setting status to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      this->status = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance parent_ref_frame.
   if ( this->parent_ref_frame == NULL ) {
      errmsg << "SpaceFOM::PhysicalEntityBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL federation instance parent_ref_frame!"
             << "  Setting parent_ref_frame to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      this->parent_ref_frame = trick_MM->mm_strdup( "" );
   }

   // Return to calling routine.
   return;
}

void PhysicalEntityBase::set_name( const char *new_name )
{
   if ( this->name != NULL ) {
      trick_MM->delete_var( (void *)this->name );
   }
   this->name = trick_MM->mm_strdup( new_name );
   return;
}

void PhysicalEntityBase::set_type( const char *new_name )
{
   if ( this->type != NULL ) {
      trick_MM->delete_var( (void *)this->type );
   }
   this->type = trick_MM->mm_strdup( new_name );
   return;
}

void PhysicalEntityBase::set_status( const char *new_name )
{
   if ( this->status != NULL ) {
      trick_MM->delete_var( (void *)this->status );
   }
   this->status = trick_MM->mm_strdup( new_name );
   return;
}

void PhysicalEntityBase::set_parent_ref_frame( const char *new_name )
{
   if ( this->parent_ref_frame != NULL ) {
      trick_MM->delete_var( (void *)this->parent_ref_frame );
   }
   this->parent_ref_frame = trick_MM->mm_strdup( new_name );

   return;
}

void PhysicalEntityBase::pack()
{
   stc_encoder.encode();
   quat_encoder.encode();
}

void PhysicalEntityBase::unpack()
{
   stc_encoder.decode();
   quat_encoder.decode();
}
