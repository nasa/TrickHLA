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
@trick_link_dependency{../TrickHLA/Attribute.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Object.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{../TrickHLA/Types.cpp}
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
     name_attr( NULL ),
     parent_attr( NULL ),
     position_attr( NULL ),
     attitude_attr( NULL ),
     quat_encoder( packing_data.attitude )
{
   V_INIT( packing_data.position );
}

/*!
 * @job_class{shutdown}
 */
PhysicalInterfaceBase::~PhysicalInterfaceBase() // RETURN: -- None.
{
   if ( this->packing_data.name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->packing_data.name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalInterfaceBase::~PhysicalInterfaceBase():%d WARNING failed to delete Trick Memory for 'this->packing_data.name'\n",
                          __LINE__ );
      }
      this->packing_data.name = NULL;
   }
   if ( this->packing_data.parent_name != (char *)NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->packing_data.parent_name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalInterfaceBase::~PhysicalInterfaceBase():%d WARNING failed to delete Trick Memory for 'this->parent'\n",
                          __LINE__ );
      }
      this->packing_data.parent_name = NULL;
   }
   initialized   = false;
   name_attr     = NULL;
   position_attr = NULL;
   attitude_attr = NULL;
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::base_config(
   char const       *sim_obj_name,
   char const       *interface_obj_name,
   char const       *interface_name,
   char const       *interface_parent_name,
   bool              publishes,
   TrickHLA::Object *mngr_object )
{
   string interface_name_str = string( sim_obj_name ) + "." + string( interface_obj_name );
   string trick_name_str;

   // Associate the instantiated Manager object with this packing object.
   this->object = mngr_object;

   // Set the parent frame name.
   if ( interface_parent_name != NULL ) {
      this->packing_data.parent_name = trick_MM->mm_strdup( interface_parent_name );
   } else {
      this->packing_data.parent_name = trick_MM->mm_strdup( "" );
   }
   if ( interface_name != NULL ) {
      this->packing_data.name = trick_MM->mm_strdup( interface_name );
   } else {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::default_data():" << __LINE__
             << " WARNING: Unexpected NULL federation instance PhysicalInterface name!\n";
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
   object->attributes = static_cast< TrickHLA::Attribute * >( trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count ) );

   //
   // Specify the Reference Frame attributes.
   //
   object->attributes[0].FOM_name      = allocate_input_string( "name" );
   trick_name_str                      = interface_name_str + string( ".packing_data.name" );
   object->attributes[0].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[0].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[0].publish       = publishes;
   object->attributes[0].subscribe     = !publishes;
   object->attributes[0].locally_owned = publishes;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[1].FOM_name      = allocate_input_string( "parent_name" );
   trick_name_str                      = interface_name_str + string( ".packing_data.parent_name" );
   object->attributes[1].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[1].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[1].publish       = publishes;
   object->attributes[1].subscribe     = !publishes;
   object->attributes[1].locally_owned = publishes;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[2].FOM_name      = allocate_input_string( "position" );
   trick_name_str                      = interface_name_str + string( ".packing_data.position" );
   object->attributes[2].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[2].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[2].publish       = publishes;
   object->attributes[2].subscribe     = !publishes;
   object->attributes[2].locally_owned = publishes;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[3].FOM_name      = allocate_input_string( "attitude" );
   trick_name_str                      = interface_name_str + string( ".quat_encoder.buffer" );
   object->attributes[3].trick_name    = allocate_input_string( trick_name_str );
   object->attributes[3].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[3].publish       = publishes;
   object->attributes[3].subscribe     = !publishes;
   object->attributes[3].locally_owned = publishes;
   object->attributes[3].rti_encoding  = TrickHLA::ENCODING_NONE;

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::configure()
{
   // Must have federation instance name.
   if ( this->packing_data.name == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL interface name!"
             << "  Setting frame name to empty string.\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      this->packing_data.name = trick_MM->mm_strdup( "" );
   }

   // Must have federation instance parent_ref_frame.
   if ( this->packing_data.parent_name == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL interface parent!"
             << "  Setting parent_ref_frame to empty string.\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      this->packing_data.parent_name = trick_MM->mm_strdup( "" );
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::initialize()
{
   // Must have interface instance name.
   if ( this->packing_data.name == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL interface name!"
             << "  Setting frame name to empty string.\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      this->packing_data.name = trick_MM->mm_strdup( "" );
   }

   // Should have interface parent specified if creating this interface.
   if ( this->object->create_HLA_instance
        && this->packing_data.parent_name == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " WARNING: Unexpected NULL interface parent!"
             << "  Setting parent_ref_frame to empty string.\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      this->packing_data.parent_name = trick_MM->mm_strdup( "" );
   }

   // Mark this as initialized.
   TrickHLA::Packing::initialize();

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
   TrickHLA::Packing::initialize_callback( obj );

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
   if ( this->packing_data.name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->packing_data.name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalInterfaceBase::set_name():%d WARNING failed to delete Trick Memory for 'this->packing_data.name'\n",
                          __LINE__ );
      }
   }
   this->packing_data.name = trick_MM->mm_strdup( new_name );
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::set_parent( char const *new_parent_name )
{
   if ( this->packing_data.parent_name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->packing_data.parent_name ) ) ) {
         message_publish( MSG_WARNING, "SpaceFOM::PhysicalInterfaceBase::set_parent():%d WARNING failed to delete Trick Memory for 'this->parent_frame'\n",
                          __LINE__ );
      }
   }
   this->packing_data.parent_name = trick_MM->mm_strdup( new_parent_name );

   return;
}

void PhysicalInterfaceBase::pack()
{
   // Check for initialization.
   if ( !initialized ) {
      ostringstream errmsg;
      errmsg << "PhysicalInterfaceBase::pack() ERROR: The initialize() function has not"
             << " been called!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Check for latency/lag compensation.
   if ( this->object->lag_comp == NULL ) {
      pack_from_working_data();
   }

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "PhysicalInterfaceBase::pack():" << __LINE__ << '\n';
      print_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Encode the data into the buffer.
   quat_encoder.encode();

   return;
}

void PhysicalInterfaceBase::unpack()
{
   if ( !initialized ) {
      ostringstream msg;
      msg << "PhysicalInterfaceBase::unpack():" << __LINE__
          << " ERROR: The initialize() function has not been called!\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Use the HLA encoder helpers to decode the PhysicalInterface fixed record.
   quat_encoder.decode();

   // Transfer the packing data into the working data.
   unpack_into_working_data();

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "PhysicalInterfaceBase::unpack():" << __LINE__ << '\n';
      print_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalInterfaceBase::print_data( std::ostream &stream ) const
{
   double euler_angles[3];

   // Compute the attitude Euler angles.
   packing_data.attitude.get_Euler_deg( Roll_Pitch_Yaw, euler_angles );

   // Set the print precision.
   stream.precision( 15 );

   stream << "\tObject-Name: '" << object->get_name() << "'\n"
          << "\tname:   '" << ( packing_data.name != NULL ? packing_data.name : "" ) << "'\n"
          << "\tparent: '" << ( packing_data.parent_name != NULL ? packing_data.parent_name : "" ) << "'\n";
   stream << "\tposition: "
          << "\t\t" << packing_data.position[0] << ", "
          << "\t\t" << packing_data.position[1] << ", "
          << "\t\t" << packing_data.position[2] << '\n';
   stream << "\tattitude (s,v): "
          << "\t\t" << packing_data.attitude.scalar << "; "
          << "\t\t" << packing_data.attitude.vector[0] << ", "
          << "\t\t" << packing_data.attitude.vector[1] << ", "
          << "\t\t" << packing_data.attitude.vector[2] << '\n';
   stream << "\tattitude (RPY){deg}: "
          << "\t\t" << euler_angles[0] << ", "
          << "\t\t" << euler_angles[1] << ", "
          << "\t\t" << euler_angles[2] << '\n';
   stream << '\n';

   return;
}
