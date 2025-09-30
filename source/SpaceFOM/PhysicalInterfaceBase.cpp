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

// System includes.
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/reference_frame.h"
#include "trick/vector_macros.h"

// SpaceFOM includes.
#include "SpaceFOM/PhysicalInterfaceBase.hh"
#include "SpaceFOM/QuaternionData.hh"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalInterfaceBase::PhysicalInterfaceBase() // RETURN: -- None.
   : TrickHLA::Packing( "PhysicalInterfaceBase" ),
     debug( false ),
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
   bool               create,
   std::string const &sim_obj_name,
   std::string const &interface_pkg_name,
   std::string const &interface_fed_name,
   TrickHLA::Object  *mngr_object )
{
   string interface_full_name_str = sim_obj_name + "." + interface_pkg_name;

   // Make sure that the TrickHLA::Object pointer is not NULL.
   // If NULL, this it means this object has not been allocated yet.
   // If not allocated, there are two options:
   // 1). We are configuring in the input file, which is okay.
   // 2). We are configuring in default_data but forgot to allocate and
   //     assign the associated object in the 'create_connections()' routine.
   if ( mngr_object == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         ostringstream errmsg;
         errmsg << "PhysicalInterfaceBase::base_config() Warning: " << endl
                << "\tThe TrickHLA::Object associated with object \'" << interface_fed_name << "\' is NULL." << endl
                << "\tEither of the two things are possible:" << endl
                << "\t1). We are configuring in the input file, which is okay." << endl
                << "\t2). We are configuring in default_data but forgot to allocate and" << endl
                << "\t    assign the associated object in the 'create_connections()' routine.";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   } else {
      // Associate the instantiated Manager object with this packing object.
      this->object = mngr_object;
   }

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   object->FOM_name            = "PhysicalInterface";
   object->name                = interface_fed_name;
   object->create_HLA_instance = create;
   object->packing             = this;
   // Allocate the attributes for the PhysicalInterface HLA object.
   object->attr_count = 9;
   object->attributes = static_cast< TrickHLA::Attribute * >( trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count ) );

   //
   // Specify the Reference Frame attributes.
   //
   object->attributes[0].FOM_name   = "name";
   object->attributes[0].trick_name = interface_full_name_str + string( ".packing_data.name" );
   ;
   object->attributes[0].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[0].publish       = create;
   object->attributes[0].subscribe     = !create;
   object->attributes[0].locally_owned = create;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[1].FOM_name   = "parent_name";
   object->attributes[1].trick_name = interface_full_name_str + string( ".packing_data.parent_name" );
   ;
   object->attributes[1].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[1].publish       = create;
   object->attributes[1].subscribe     = !create;
   object->attributes[1].locally_owned = create;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[2].FOM_name   = "position";
   object->attributes[2].trick_name = interface_full_name_str + string( ".packing_data.position" );
   ;
   object->attributes[2].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[2].publish       = create;
   object->attributes[2].subscribe     = !create;
   object->attributes[2].locally_owned = create;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[3].FOM_name   = "attitude";
   object->attributes[3].trick_name = interface_full_name_str + string( ".quat_encoder.buffer" );
   ;
   object->attributes[3].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[3].publish       = create;
   object->attributes[3].subscribe     = !create;
   object->attributes[3].locally_owned = create;
   object->attributes[3].rti_encoding  = TrickHLA::ENCODING_NONE;

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::configure()
{
   // Check for a NULL object pointer.
   if ( this->object == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::configure():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA Object pointer!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Check for empty federation instance name.
   if ( this->object->create_HLA_instance
        && this->packing_data.name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::configure():" << __LINE__
             << " WARNING: Unexpected empty interface name!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Check for empty federation instance parent_ref_frame.
   if ( this->object->create_HLA_instance
        && this->packing_data.parent_name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::configure():" << __LINE__
             << " WARNING: Unexpected empty interface parent!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Call the base class.
   Packing::configure();

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::initialize()
{
   // Must have interface instance name.
   if ( this->object->create_HLA_instance
        && this->packing_data.name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " ERROR: Unexpected empty interface name!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Should have interface parent specified if creating this interface.
   if ( this->object->create_HLA_instance
        && this->packing_data.parent_name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " WARNING: Unexpected empty interface parent!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Mark this as initialized.
   Packing::initialize();

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
void PhysicalInterfaceBase::set_name( std::string const &new_name )
{
   if ( this->object != NULL
        && this->object->create_HLA_instance
        && new_name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::set_name():" << __LINE__
             << " WARNING: Unexpected empty interface name!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }
   this->packing_data.name = new_name;
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::set_parent( std::string const &new_parent_name )
{
   if ( this->object != NULL
        && this->object->create_HLA_instance
        && new_parent_name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::set_parent():" << __LINE__
             << " WARNING: Unexpected empty parent name!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }
   this->packing_data.parent_name = new_parent_name;

   return;
}

void PhysicalInterfaceBase::pack()
{
   // Check for initialization.
   if ( !initialized ) {
      ostringstream errmsg;
      errmsg << "PhysicalInterfaceBase::pack() ERROR: The initialize() function has not"
             << " been called!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Check for latency/lag compensation.
   if ( this->object->lag_comp == NULL ) {
      pack_from_working_data();
   }

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "PhysicalInterfaceBase::pack():" << __LINE__ << endl;
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
          << " ERROR: The initialize() function has not been called!" << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Use the HLA encoder helpers to decode the PhysicalInterface fixed record.
   quat_encoder.decode();

   // Transfer the packing data into the working data.
   unpack_into_working_data();

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "PhysicalInterfaceBase::unpack():" << __LINE__ << endl;
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

   stream << "\tObject-Name: '" << object->get_name() << "'" << endl
          << "\tname:   '" << packing_data.name << "'" << endl
          << "\tparent: '" << packing_data.parent_name << "'" << endl;
   stream << "\tposition: "
          << "\t\t" << packing_data.position[0] << ", "
          << "\t\t" << packing_data.position[1] << ", "
          << "\t\t" << packing_data.position[2] << endl;
   stream << "\tattitude (s,v): "
          << "\t\t" << packing_data.attitude.scalar << "; "
          << "\t\t" << packing_data.attitude.vector[0] << ", "
          << "\t\t" << packing_data.attitude.vector[1] << ", "
          << "\t\t" << packing_data.attitude.vector[2] << endl;
   stream << "\tattitude (RPY){deg}: "
          << "\t\t" << euler_angles[0] << ", "
          << "\t\t" << euler_angles[1] << ", "
          << "\t\t" << euler_angles[2] << endl;
   stream << endl;

   return;
}
