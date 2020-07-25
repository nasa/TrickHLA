/*!
@file SpaceFOM/RefFrameBase.cpp
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
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/Object.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
@trick_link_dependency{ExecutionControl.cpp}
@trick_link_dependency{RefFrameBase.cpp}

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
#include "trick/message_proto.h"

// TrickHLA model include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/ExecutionControl.hh"
#include "SpaceFOM/RefFrameBase.hh"

using namespace std;
using namespace SpaceFOM;

#define REF_FRAME_PACKING_DEBUG 0
#define REF_FRAME_PACKING_EXTRA_DEBUG 0

extern Trick::MemoryManager *trick_MM;

/*!
 * @job_class{initialization}
 */
RefFrameBase::RefFrameBase()
   : debug( false ),
     initialized( false ),
     ref_frame_data( NULL ),
     ref_frame_attr( NULL ),
     name( NULL ),
     parent_name( NULL ),
     stc_encoder(),
     stc_data( stc_encoder.get_data() )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
RefFrameBase::~RefFrameBase()
{
   if ( this->name != (char *)NULL ) {
      trick_MM->delete_var( (void *)this->name );
      this->name = (char *)NULL;
   }
   if ( this->parent_name != (char *)NULL ) {
      trick_MM->delete_var( (void *)this->parent_name );
      this->parent_name = (char *)NULL;
   }
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{default_data}
 */
void RefFrameBase::default_data(
   SpaceFOM::ExecutionControl *execution_control,
   TrickHLA::Object *          object,
   const char *                sim_obj_name,
   const char *                ref_frame_obj_name,
   const char *                ref_frame_name,
   bool                        publishes )
{
   string ref_frame_name_str = string( sim_obj_name ) + "." + string( ref_frame_obj_name );
   string trick_name_str;

   //
   // Assign the reference in the manager to this Reference Frame object.
   //
   execution_control->root_ref_frame = this;

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   object->FOM_name            = allocate_input_string( "ReferenceFrame" );
   object->name                = allocate_input_string( ref_frame_name );
   object->create_HLA_instance = publishes;
   object->packing             = this;
   // Allocate the attributes for the RefFrameBase HLA object.
   object->attr_count = 3;
   object->attributes = (TrickHLA::Attribute *)trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count );

   //
   // Specify the Reference Frame attributes.
   //
   // Setup the "root_frame_name" attribute.
   object->attributes[0].FOM_name      = allocate_input_string( "name" );
   trick_name_str                      = ref_frame_name_str + string( ".name" );
   object->attributes[0].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[0].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[0].publish       = publishes;
   object->attributes[0].subscribe     = !publishes;
   object->attributes[0].locally_owned = publishes;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[1].FOM_name      = allocate_input_string( "parent_name" );
   trick_name_str                      = ref_frame_name_str + string( ".parent_name" );
   object->attributes[1].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[1].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[1].publish       = publishes;
   object->attributes[1].subscribe     = !publishes;
   object->attributes[1].locally_owned = publishes;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[2].FOM_name      = allocate_input_string( "state" );
   trick_name_str                      = ref_frame_name_str + string( ".stc_encoder.buffer" );
   object->attributes[2].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[2].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[2].publish       = publishes;
   object->attributes[2].subscribe     = !publishes;
   object->attributes[2].locally_owned = publishes;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_OPAQUE_DATA;

   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::initialize(
   RefFrameData *ref_frame_data_ptr )
{
   ostringstream errmsg;

   // Must have federation instance name.
   if ( this->name == NULL ) {
      errmsg << "SpaceFOM::RefFrameBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL federation instance frame name!"
             << "  Setting frame name to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      this->name = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance parent frame name.
   if ( this->parent_name == NULL ) {
      errmsg << "SpaceFOM::RefFrameBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL federation instance parent frame name!"
             << "  Setting parent frame name to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      this->parent_name = trick_MM->mm_strdup( "" );
   }

   // Set the reference to the reference frame.
   if ( ref_frame_data_ptr == NULL ) {
      // Print message and terminate.
      errmsg << "SpaceFOM::RefFrameBase::initialize():" << __LINE__
             << " ERROR: Unexpected NULL reference frame: " << this->name << THLA_ENDL;
      Trick::Executive *trick_exec = exec_get_exec_cpp();
      send_hs( stderr, (char *)errmsg.str().c_str() );
      trick_exec->exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }
   this->ref_frame_data = ref_frame_data_ptr;

   // Mark this as initialized.
   this->initialized = true;

   // Return to calling routine.
   return;
}

/*!
 * @details From the TrickHLA::Packing class. We override this function so
 * that we can initialize references to the TrickHLA::Attribute's that are
 * used in the unpack function to handle attribute ownership and different
 * attribute data rates.
 *
 * Use the initialize callback function as a way to setup TrickHLA::Attribute
 * references which are use to determine ownership or if data for an attribute
 * was received.
 *
 * @job_class{initialization}
 */
void RefFrameBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   this->TrickHLA::Packing::initialize_callback( obj );

   // Get a reference to the TrickHLA::Attribute for the "state" FOM attribute.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   ref_frame_attr = get_attribute_and_validate( "state" );
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::set_name( const char *new_name )
{
   if ( this->name != NULL ) {
      trick_MM->delete_var( (void *)this->name );
   }
   this->name = trick_MM->mm_strdup( new_name );
   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::set_parent_name( const char *name )
{
   if ( this->parent_name != NULL ) {
      trick_MM->delete_var( (void *)this->parent_name );
   }
   this->parent_name = trick_MM->mm_strdup( name );
   return;
}

void RefFrameBase::pack()
{
   int iinc;

   // Check for initialization.
   if ( !initialized ) {
      cout << "RefFrameBase::pack() ERROR: The initialize() function has not"
           << " been called!" << endl;
   }

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Pack the data.
   // Position and velocity vectors.
   for ( iinc = 0; iinc < 3; iinc++ ) {
      stc_data.pos[iinc] = ref_frame_data->state.pos[iinc];
      stc_data.vel[iinc] = ref_frame_data->state.vel[iinc];
   }
   // Attitude quaternion.
   stc_data.quat_scalar = ref_frame_data->state.quat_scalar;
   for ( iinc = 0; iinc < 3; iinc++ ) {
      stc_data.quat_vector[iinc] = ref_frame_data->state.quat_vector[iinc];
      stc_data.ang_vel[iinc]     = ref_frame_data->state.ang_vel[iinc];
   }
   // Time tag for this state data.
   // stc_data.time = ref_frame->state.time;
   stc_data.time = ref_frame_data->state.time = get_scenario_time();

   // Print out debug information if desired.
   if ( debug ) {
      cout.precision( 15 );
      cout << "RefFrameBase::pack()" << endl
           << "\tObject-Name: '" << object->get_name() << "'" << endl
           << "\tname: '" << ( this->name != NULL ? this->name : "" ) << "'" << endl
           << "\tparent_name: '" << ( this->parent_name != NULL ? this->parent_name : "" ) << "'" << endl
           << "\ttime: " << stc_data.time << endl
           << "\tposition: " << endl
           << "\t\t" << stc_data.pos[0] << endl
           << "\t\t" << stc_data.pos[1] << endl
           << "\t\t" << stc_data.pos[2] << endl
           << endl;
   }

   // Encode the data into the reference frame buffer.
   stc_encoder.encode();

   return;
}

void RefFrameBase::unpack()
{
   //double dt; // Local vs. remote time difference.

   if ( !initialized ) {
      cout << "RefFrameBase::unpack() ERROR: The initialize() function has not"
           << " been called!" << endl;
   }

   // Use the HLA encoder helpers to decode the reference frame fixed record.
   stc_encoder.decode();

   // If the HLA phase attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value. If we locally own the "Phase" attribute then we do not
   // want to override it's value. If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the "phase_deg" local variable, which would cause data
   // corruption of the state. We always need to do this check because
   // ownership transfers could happen at any time or the data could be at a
   // different rate.
   if ( ref_frame_attr->is_received() ) {
      // Print out debug information if desired.
      if ( debug ) {
         cout.precision( 15 );
         cout << "RefFrameBase::unpack()" << endl
              << "\tObject-Name: '" << object->get_name() << "'" << endl
              << "\tname: '" << ( this->name != NULL ? this->name : "" ) << "'" << endl
              << "\tparent_name: '" << ( this->parent_name != NULL ? this->parent_name : "" ) << "'" << endl
              << "\ttime: " << stc_data.time << endl
              << "\tposition: " << endl
              << "\t\t" << stc_data.pos[0] << endl
              << "\t\t" << stc_data.pos[1] << endl
              << "\t\t" << stc_data.pos[2] << endl
              << endl;
      }

      // Unpack the data.
      // Position and velocity vectors.
      for ( int iinc = 0; iinc < 3; iinc++ ) {
         ref_frame_data->state.pos[iinc] = stc_data.pos[iinc];
         ref_frame_data->state.vel[iinc] = stc_data.vel[iinc];
      }
      // Attitude quaternion.
      ref_frame_data->state.quat_scalar = stc_data.quat_scalar;
      for ( int iinc = 0; iinc < 3; iinc++ ) {
         ref_frame_data->state.quat_vector[iinc] = stc_data.quat_vector[iinc];
         ref_frame_data->state.ang_vel[iinc]     = stc_data.ang_vel[iinc];
      }
      // Time tag for this state data.
      ref_frame_data->state.time = stc_data.time;

      // Set the frame name and parent name.
      if ( ref_frame_data->name != NULL ) {
         free( ref_frame_data->name );
         ref_frame_data->name = NULL;
      }
      ref_frame_data->name = strdup( this->name );

      if ( ref_frame_data->parent_name != NULL ) {
         free( ref_frame_data->parent_name );
         ref_frame_data->parent_name = NULL;
      }
      if ( this->parent_name != NULL ) {
         if ( this->parent_name[0] != '\0' ) {
            ref_frame_data->parent_name = strdup( this->parent_name );
         }
      }
   }

   return;
}

/*!
 * @job_class{default_data}
 */
char *RefFrameBase::allocate_input_string( // RETURN: -- None.
   const char *c_string )                  // IN: -- String to allocate.
{
   char *new_c_str;

   new_c_str = (char *)TMM_declare_var_1d( "char", strlen( c_string ) + 1 );
   strncpy( new_c_str, c_string, strlen( c_string ) + 1 );

   return new_c_str;
}

/*!
 * @job_class{default_data}
 */
char *RefFrameBase::allocate_input_string( // RETURN: -- None.
   string cpp_string )                     // IN: -- String to allocate.
{
   char *new_c_str;

   new_c_str = (char *)TMM_declare_var_1d( "char", cpp_string.length() + 1 );
   strncpy( new_c_str, cpp_string.c_str(), cpp_string.length() + 1 );

   return new_c_str;
}
