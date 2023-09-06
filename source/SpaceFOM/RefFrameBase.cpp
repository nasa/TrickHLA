/*!
@file SpaceFOM/RefFrameBase.cpp
@ingroup SpaceFOM
@brief This class provides an extendable base class for SpaceFOM Reference Frames packing.

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
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
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
#include <cstring>
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
#include "TrickHLA/DebugHandler.hh"
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

/*!
 * @job_class{initialization}
 */
RefFrameBase::RefFrameBase()
   : debug( false ),
     initialized( false ),
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
   TrickHLA::Object           *object,
   char const                 *sim_obj_name,
   char const                 *ref_frame_obj_name,
   char const                 *ref_frame_name,
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
void RefFrameBase::initialize()
{
   // Must have federation instance name.
   if ( this->name == NULL ) {
      if ( debug ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::RefFrameBase::initialize():" << __LINE__
                << " WARNING: Unexpected NULL federation instance frame name!"
                << "  Setting frame name to empty string." << THLA_ENDL;
         send_hs( stderr, errmsg.str().c_str() );
      }
      this->name = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance parent frame name.
   if ( this->parent_name == NULL ) {
      if ( debug ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::RefFrameBase::initialize():" << __LINE__
                << " WARNING: Unexpected NULL federation instance parent frame name!"
                << "  Setting parent frame name to empty string." << THLA_ENDL;
         send_hs( stderr, errmsg.str().c_str() );
      }
      this->parent_name = trick_MM->mm_strdup( "" );
   }

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
void RefFrameBase::set_name( char const *new_name )
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
void RefFrameBase::set_parent_name( char const *name )
{
   if ( this->parent_name != NULL ) {
      trick_MM->delete_var( (void *)this->parent_name );
   }
   this->parent_name = trick_MM->mm_strdup( name );
   return;
}

/*!
 * @job_class{default_data}
 */
char *RefFrameBase::allocate_input_string( // RETURN: -- None.
   char const *c_string )                  // IN: -- String to allocate.
{
   return allocate_input_string( string( c_string ) );
}

/*!
 * @job_class{default_data}
 */
char *RefFrameBase::allocate_input_string( // RETURN: -- None.
   string const &cpp_string )              // IN: -- String to allocate.
{
   char *new_c_str = static_cast< char * >( TMM_declare_var_1d( "char", cpp_string.length() + 1 ) );
   strncpy( new_c_str, cpp_string.c_str(), cpp_string.length() + 1 );

   return new_c_str;
}
