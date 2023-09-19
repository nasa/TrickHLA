/*!
@file SpaceFOM/PhysicalInterfaceBase.cpp
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM PhysicalInterfaces.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{../TrickHLA/CompileConfig.cpp}
@trick_link_dependency{PhysicalInterfaceBase.cpp}


@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, SISO, September 2023, --, Initial implementation.}
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
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/PhysicalInterfaceBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalInterfaceBase::PhysicalInterfaceBase() // RETURN: -- None.
   : debug( false ),
     initialized( false ),
     name_attr( NULL ),
     parent_attr( NULL ),
     position_attr( NULL ),
     attitude_attr( NULL ),
     quat_encoder(),
     name( NULL ),
     parent_name( NULL ),
     attitude( quat_encoder.get_data() )
{
   V_INIT( position );
}

/*!
 * @job_class{shutdown}
 */
PhysicalInterfaceBase::~PhysicalInterfaceBase() // RETURN: -- None.
{


   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         send_hs( stderr, "SpaceFOM::PhysicalInterfaceBase::~PhysicalInterfaceBase():%d ERROR deleting Trick Memory for 'this->name'%c",
                  __LINE__, THLA_NEWLINE );
      }
      this->name = NULL;
   }
   if ( this->parent_name != (char *)NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->parent_name ) ) ) {
         send_hs( stderr, "SpaceFOM::PhysicalInterfaceBase::~PhysicalInterfaceBase():%d ERROR deleting Trick Memory for 'this->parent'%c",
                  __LINE__, THLA_NEWLINE );
      }
      this->parent_name = NULL;

   }
   initialized   = false;
   name_attr     = NULL;
   position_attr = NULL;
   attitude_attr = NULL;
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{default_data}
 */
void PhysicalInterfaceBase::default_data(
   TrickHLA::Object *mngr_object,
   char const       *sim_obj_name,
   char const       *interface_obj_name,
   char const       *interface_name,
   char const       *parent_ref_frame_name,
   bool              publishes )
{
   string interface_name_str = string( sim_obj_name ) + "." + string( interface_obj_name );
   string trick_name_str;

   // Associate the instantiated Manager object with this packing object.
   this->object = mngr_object;

   // Set the parent frame name.
   if ( parent_ref_frame_name != NULL ) {
      parent_name = trick_MM->mm_strdup( parent_ref_frame_name );
   } else {
      parent_name = trick_MM->mm_strdup( "" );
   }
   if ( interface_name != NULL ) {
      name = trick_MM->mm_strdup( interface_name );
   } else {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::default_data():" << __LINE__
             << " WARNING: Unexpected NULL federation instance PhysicalInterface name!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   object->FOM_name            = allocate_input_string( "PhysicalInterface" );
   object->name                = allocate_input_string( interface_name );
   object->create_HLA_instance = publishes;
   object->packing             = this;
   // Allocate the attributes for the PhysicalInterface HLA object.
   object->attr_count = 9;
   object->attributes = (TrickHLA::Attribute *)trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count );

   //
   // Specify the Reference Frame attributes.
   //
   object->attributes[0].FOM_name      = allocate_input_string( "name" );
   trick_name_str                      = interface_name_str + string( ".name" );
   object->attributes[0].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[0].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[0].publish       = publishes;
   object->attributes[0].subscribe     = !publishes;
   object->attributes[0].locally_owned = publishes;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[1].FOM_name      = allocate_input_string( "parent_name" );
   trick_name_str                      = interface_name_str + string( ".parent_name" );
   object->attributes[1].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[1].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[1].publish       = publishes;
   object->attributes[1].subscribe     = !publishes;
   object->attributes[1].locally_owned = publishes;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[2].FOM_name      = allocate_input_string( "position" );
   trick_name_str                      = interface_name_str + string( ".position" );
   object->attributes[2].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[2].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[2].publish       = publishes;
   object->attributes[2].subscribe     = !publishes;
   object->attributes[2].locally_owned = publishes;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[3].FOM_name      = allocate_input_string( "attitude" );
   trick_name_str                      = interface_name_str + string( ".attitude.buffer" );
   object->attributes[3].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[3].config        = ( TrickHLA::DataUpdateEnum )( (int)TrickHLA::CONFIG_INITIALIZE + (int)TrickHLA::CONFIG_CYCLIC );
   object->attributes[3].publish       = publishes;
   object->attributes[3].subscribe     = !publishes;
   object->attributes[3].locally_owned = publishes;
   object->attributes[3].rti_encoding  = TrickHLA::ENCODING_OPAQUE_DATA;

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::initialize()
{
   ostringstream errmsg;

   // Must have federation instance name.
   if ( name == NULL ) {
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL interface name!"
             << "  Setting frame name to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      this->name = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance parent_ref_frame.
   if ( parent_name == NULL ) {
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL interface parent!"
             << "  Setting parent_ref_frame to empty string." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
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
void PhysicalInterfaceBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   this->TrickHLA::Packing::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   name_attr     = get_attribute_and_validate( "name" );
   parent_attr   = get_attribute_and_validate( "parent_name" );
   position_attr = get_attribute_and_validate( "position" );
   attitude_attr = get_attribute_and_validate( "attitude" );

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::set_name( char const *new_name )
{
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->name ) ) ) {
         send_hs( stderr, "SpaceFOM::PhysicalInterfaceBase::set_name():%d ERROR deleting Trick Memory for 'this->name'%c",
                  __LINE__, THLA_NEWLINE );
      }
   }
   name = trick_MM->mm_strdup( new_name );
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::set_parent( char const *new_parent_name )
{
   if ( this->parent_name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->parent_name ) ) ) {
         send_hs( stderr, "SpaceFOM::PhysicalInterfaceBase::set_parent():%d ERROR deleting Trick Memory for 'this->parent_frame'%c",
                  __LINE__, THLA_NEWLINE );
      }
   }
   parent_name = trick_MM->mm_strdup( new_parent_name );

   return;
}

/*!
 * @job_class{default_data}
 */
void PhysicalInterfaceBase::set_object( TrickHLA::Object *mngr_obj )
{
   ostringstream errmsg;

   // Check for initialization.
   if ( initialized ) {
      errmsg << "SpaceFOM::PhysicalInterfaceBase::set_object():" << __LINE__
             << " ERROR: The initialize() function has already been called" << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Assign the object.
   this->object = mngr_obj;

   return;
}
