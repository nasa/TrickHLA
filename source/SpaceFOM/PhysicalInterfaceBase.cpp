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
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/reference_frame.h"
#include "trick/vector_macros.h"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/MemoryServices.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM includes.
#include "SpaceFOM/PhysicalInterfaceBase.hh"
#include "SpaceFOM/QuaternionData.hh"
#if !defined( USE_SPACEFOM_OPAQUE_BUFFER_ENCODERS )
#   include "SpaceFOM/QuaternionConfig.hh"
#endif

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
     attitude_attr( NULL )
#if defined( USE_SPACEFOM_OPAQUE_BUFFER_ENCODERS )
     ,
     quat_encoder( packing_data.attitude )
#endif
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
   TrickHLA::Object  *mngr_object,
   bool const         publish,
   bool const         subscribe )
{
   string const interface_full_name_str = sim_obj_name + "." + interface_pkg_name;

   // Make sure that the TrickHLA::Object pointer is not NULL.
   // If NULL, this it means this object has not been allocated yet.
   // If not allocated, there are two options:
   // 1). We are configuring in the input file, which is okay.
   // 2). We are configuring in default_data but forgot to allocate and
   //     assign the associated object in the 'create_connections()' routine.
   if ( mngr_object == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SRC_OBJECT ) ) {
         ostringstream errmsg;
         errmsg << "PhysicalInterfaceBase::base_config() Warning: \n"
                << "\tThe TrickHLA::Object associated with object \'" << interface_fed_name << "\' is NULL.\n"
                << "\tEither of the two things are possible:\n"
                << "\t1). We are configuring in the input file, which is okay.\n"
                << "\t2). We are configuring in default_data but forgot to allocate and\n"
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
   object->attributes = MemoryServices::declare_var( object->attributes, object->attr_count );

   bool const publish_attr   = create || publish;
   bool const subscribe_attr = !create || subscribe;

   //
   // Specify the Reference Frame attributes.
   //
   object->attributes[0].FOM_name      = "name";
   object->attributes[0].trick_name    = interface_full_name_str + string( ".packing_data.name" );
   object->attributes[0].config        = TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC;
   object->attributes[0].publish       = publish_attr;
   object->attributes[0].subscribe     = subscribe_attr;
   object->attributes[0].locally_owned = create;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[1].FOM_name      = "parent_name";
   object->attributes[1].trick_name    = interface_full_name_str + string( ".packing_data.parent_name" );
   object->attributes[1].config        = TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC;
   object->attributes[1].publish       = publish_attr;
   object->attributes[1].subscribe     = subscribe_attr;
   object->attributes[1].locally_owned = create;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[2].FOM_name      = "position";
   object->attributes[2].trick_name    = interface_full_name_str + string( ".packing_data.position" );
   object->attributes[2].config        = TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC;
   object->attributes[2].publish       = publish_attr;
   object->attributes[2].subscribe     = subscribe_attr;
   object->attributes[2].locally_owned = create;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

#if defined( USE_SPACEFOM_OPAQUE_BUFFER_ENCODERS )
   object->attributes[3].FOM_name      = "attitude";
   object->attributes[3].trick_name    = interface_full_name_str + string( ".quat_encoder.buffer" );
   object->attributes[3].config        = TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC;
   object->attributes[3].publish       = publish_attr;
   object->attributes[3].subscribe     = subscribe_attr;
   object->attributes[3].locally_owned = create;
   object->attributes[3].rti_encoding  = TrickHLA::ENCODING_NONE;
#else
   string const quat_fom_name   = "attitude";
   string const quat_trick_name = interface_full_name_str + string( ".packing_data.attitude" );

   QuaternionConfig::configure(
      &object->attributes[3],
      quat_fom_name,
      quat_trick_name,
      TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC,
      publish_attr,
      subscribe_attr,
      create );
#endif // USE_SPACEFOM_OPAQUE_BUFFER_ENCODERS

   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceBase::initialize()
{
   // Check for a NULL object pointer.
   if ( this->object == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA Object pointer!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Must have interface instance name.
   if ( this->object->create_HLA_instance
        && this->packing_data.name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " ERROR: Unexpected empty interface name!\n";
      DebugHandler::terminate( errmsg.str() );
   }

   // Should have interface parent specified if creating this interface.
   if ( this->object->create_HLA_instance
        && this->packing_data.parent_name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::PhysicalInterfaceBase::initialize():" << __LINE__
             << " WARNING: Unexpected empty interface parent!\n";
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
             << " WARNING: Unexpected empty interface name!\n";
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
             << " WARNING: Unexpected empty parent name!\n";
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
      errmsg << "PhysicalInterfaceBase::pack():" << __LINE__
#if defined( TRICKHLA_ERROR_IF_NOT_INITIALIZED )
             << " ERROR: The initialize() function has not been called!\n";
      DebugHandler::terminate( errmsg.str() );
#else
             << " WARNING: The initialize() function has not been called!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
#endif
   }

   // Check for latency/lag compensation.
   if ( this->object->lag_comp == NULL ) {
      pack_from_working_data();
   }

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "PhysicalInterfaceBase::pack():" << __LINE__ << "\n";
      print_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

#if defined( USE_SPACEFOM_OPAQUE_BUFFER_ENCODERS )
   // Encode the data into the buffer.
   quat_encoder.encode();
#endif

   return;
}

void PhysicalInterfaceBase::unpack()
{
   if ( !initialized ) {
      ostringstream errmsg;
      errmsg << "PhysicalInterfaceBase::unpack():" << __LINE__
#if defined( TRICKHLA_ERROR_IF_NOT_INITIALIZED )
             << " ERROR: The initialize() function has not been called!\n";
      DebugHandler::terminate( errmsg.str() );
#else
             << " WARNING: The initialize() function has not been called!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
#endif
   }

#if defined( USE_SPACEFOM_OPAQUE_BUFFER_ENCODERS )
   // Use the HLA encoder helpers to decode the PhysicalInterface fixed record.
   quat_encoder.decode();
#endif

   // Transfer the packing data into the working data.
   unpack_into_working_data();

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "PhysicalInterfaceBase::unpack():" << __LINE__ << "\n";
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

   stream << "        Object-Name: '" << object->get_name() << "'\n"
          << "               name: '" << packing_data.name << "'\n"
          << "             parent: '" << packing_data.parent_name << "'\n"
          << "           position: "
          << "\t" << packing_data.position[0] << ", "
          << "\t\t" << packing_data.position[1] << ", "
          << "\t\t" << packing_data.position[2] << "\n"
          << "     attitude (s,v): "
          << "\t" << packing_data.attitude.scalar << "; "
          << "\t\t" << packing_data.attitude.vector[0] << ", "
          << "\t\t" << packing_data.attitude.vector[1] << ", "
          << "\t\t" << packing_data.attitude.vector[2] << "\n"
          << "attitude (RPY){deg}: "
          << "\t" << euler_angles[0] << ", "
          << "\t\t" << euler_angles[1] << ", "
          << "\t\t" << euler_angles[2] << "\n"
          << "\n";
   return;
}
