/*!
@file SpaceFOM/DynamicalEntityBase.cpp
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM DynamicalEntities.

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
@trick_link_dependency{DynamicalEntityBase.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, SISO, Sept 2010, --, Smackdown implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2023, --, Cleaned up and filled out.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Refactored.}
@revs_end

*/

// System includes.
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/matrix_macros.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/vector_macros.h"

// SpaceFOM includes.
#include "SpaceFOM/DynamicalEntityBase.hh"
#include "SpaceFOM/PhysicalEntityBase.hh"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"

// using namespace std;
using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityBase::DynamicalEntityBase() // RETURN: -- None.
   : force_attr( NULL ),
     torque_attr( NULL ),
     mass_attr( NULL ),
     mass_rate_attr( NULL ),
     inertia_attr( NULL ),
     inertia_rate_attr( NULL )
{
   //
   // Initialize the additional DynamicalEntity packing data structure.
   //
   de_packing_data.mass      = 0.0;
   de_packing_data.mass_rate = 0.0;
   V_INIT( de_packing_data.force );
   V_INIT( de_packing_data.torque );
   M_IDENT( de_packing_data.inertia );
   M_INIT( de_packing_data.inertia_rate );
}

/*!
 * @job_class{shutdown}
 */
DynamicalEntityBase::~DynamicalEntityBase() // RETURN: -- None.
{
   return;
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{initialization}
 */
void DynamicalEntityBase::base_config(
   bool               create,
   std::string const &sim_obj_name,
   std::string const &entity_pkg_name,
   std::string const &entity_fed_name,
   TrickHLA::Object  *mngr_object )
{
   string entity_full_name_str = sim_obj_name + "." + entity_pkg_name;

   // Make sure that the TrickHLA::Object pointer is not NULL.
   // If NULL, this it means this object has not been allocated yet.
   // If not allocated, there are two options:
   // 1). We are configuring in the input file, which is okay.
   // 2). We are configuring in default_data but forgot to allocate and
   //     assign the associated object in the 'create_connections()' routine.
   if ( mngr_object == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         ostringstream errmsg;
         errmsg << "DynamicalEntityBase::base_config() Warning: " << std::endl
                << "\tThe TrickHLA::Object associated with object \'" << entity_fed_name << "\' is NULL." << std::endl
                << "\tEither of the two things are possible:" << std::endl
                << "\t1). We are configuring in the input file, which is okay." << std::endl
                << "\t2). We are configuring in default_data but forgot to allocate and" << std::endl
                << "\t    assign the associated object in the 'create_connections()' routine.";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   } else {
      // Associate the instantiated Manager object with this packing object.
      this->object = mngr_object;
   }

   // Set the entity name from the entity federation name.
   if ( entity_fed_name.empty() ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::DynamicalEntityBase::default_data():" << __LINE__
             << " WARNING: Unexpected empty federation instance DynamicalEntity name!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {
      set_name( entity_fed_name );
   }

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   object->FOM_name            = "DynamicalEntity";
   object->name                = entity_fed_name;
   object->create_HLA_instance = create;
   object->packing             = this;
   // Allocate the attributes for the DynamicalEntity HLA object.
   object->attr_count = 15;
   object->attributes = static_cast< TrickHLA::Attribute * >( trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count ) );

   //
   // Specify the Reference Frame attributes.
   //
   object->attributes[0].FOM_name   = "name";
   object->attributes[0].trick_name = entity_full_name_str + string( ".pe_packing_data.name" );
   ;
   object->attributes[0].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[0].publish       = create;
   object->attributes[0].subscribe     = !create;
   object->attributes[0].locally_owned = create;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[1].FOM_name   = "type";
   object->attributes[1].trick_name = entity_full_name_str + string( ".pe_packing_data.type" );
   ;
   object->attributes[1].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[1].publish       = create;
   object->attributes[1].subscribe     = !create;
   object->attributes[1].locally_owned = create;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[2].FOM_name   = "status";
   object->attributes[2].trick_name = entity_full_name_str + string( ".pe_packing_data.status" );
   ;
   object->attributes[2].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[2].publish       = create;
   object->attributes[2].subscribe     = !create;
   object->attributes[2].locally_owned = create;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[3].FOM_name   = "parent_reference_frame";
   object->attributes[3].trick_name = entity_full_name_str + string( ".pe_packing_data.parent_frame" );
   ;
   object->attributes[3].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[3].publish       = create;
   object->attributes[3].subscribe     = !create;
   object->attributes[3].locally_owned = create;
   object->attributes[3].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[4].FOM_name   = "state";
   object->attributes[4].trick_name = entity_full_name_str + string( ".stc_encoder.buffer" );
   ;
   object->attributes[4].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[4].publish       = create;
   object->attributes[4].subscribe     = !create;
   object->attributes[4].locally_owned = create;
   object->attributes[4].rti_encoding  = TrickHLA::ENCODING_NONE;

   object->attributes[5].FOM_name   = "acceleration";
   object->attributes[5].trick_name = entity_full_name_str + string( ".pe_packing_data.accel" );
   ;
   object->attributes[5].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[5].publish       = create;
   object->attributes[5].subscribe     = !create;
   object->attributes[5].locally_owned = create;
   object->attributes[5].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[6].FOM_name   = "rotational_acceleration";
   object->attributes[6].trick_name = entity_full_name_str + string( ".pe_packing_data.ang_accel" );
   ;
   object->attributes[6].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[6].publish       = create;
   object->attributes[6].subscribe     = !create;
   object->attributes[6].locally_owned = create;
   object->attributes[6].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[7].FOM_name   = "center_of_mass";
   object->attributes[7].trick_name = entity_full_name_str + string( ".pe_packing_data.cm" );
   ;
   object->attributes[7].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[7].publish       = create;
   object->attributes[7].subscribe     = !create;
   object->attributes[7].locally_owned = create;
   object->attributes[7].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[8].FOM_name   = "body_wrt_structural";
   object->attributes[8].trick_name = entity_full_name_str + string( ".quat_encoder.buffer" );
   ;
   object->attributes[8].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[8].publish       = create;
   object->attributes[8].subscribe     = !create;
   object->attributes[8].locally_owned = create;
   object->attributes[8].rti_encoding  = TrickHLA::ENCODING_NONE;

   object->attributes[9].FOM_name   = "force";
   object->attributes[9].trick_name = entity_full_name_str + string( ".de_packing_data.force" );
   ;
   object->attributes[9].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[9].publish       = create;
   object->attributes[9].subscribe     = !create;
   object->attributes[9].locally_owned = create;
   object->attributes[9].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[10].FOM_name   = "torque";
   object->attributes[10].trick_name = entity_full_name_str + string( ".de_packing_data.torque" );
   ;
   object->attributes[10].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[10].publish       = create;
   object->attributes[10].subscribe     = !create;
   object->attributes[10].locally_owned = create;
   object->attributes[10].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[11].FOM_name   = "mass";
   object->attributes[11].trick_name = entity_full_name_str + string( ".de_packing_data.mass" );
   ;
   object->attributes[11].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[11].publish       = create;
   object->attributes[11].subscribe     = !create;
   object->attributes[11].locally_owned = create;
   object->attributes[11].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[12].FOM_name   = "mass_rate";
   object->attributes[12].trick_name = entity_full_name_str + string( ".de_packing_data.mass_rate" );
   ;
   object->attributes[12].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[12].publish       = create;
   object->attributes[12].subscribe     = !create;
   object->attributes[12].locally_owned = create;
   object->attributes[12].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[13].FOM_name   = "inertia";
   object->attributes[13].trick_name = entity_full_name_str + string( ".de_packing_data.inertia" );
   ;
   object->attributes[13].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[13].publish       = create;
   object->attributes[13].subscribe     = !create;
   object->attributes[13].locally_owned = create;
   object->attributes[13].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   object->attributes[14].FOM_name   = "inertia_rate";
   object->attributes[14].trick_name = entity_full_name_str + string( ".de_packing_data.inertia_rate" );
   ;
   object->attributes[14].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[14].publish       = create;
   object->attributes[14].subscribe     = !create;
   object->attributes[14].locally_owned = create;
   object->attributes[14].rti_encoding  = TrickHLA::ENCODING_LITTLE_ENDIAN;

   return;
}

/*!
 * @job_class{initialization}
 */
void DynamicalEntityBase::initialize()
{
   // Only need to call the PhysicalEntityBase initialization function.
   PhysicalEntityBase::initialize();

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
void DynamicalEntityBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   PhysicalEntityBase::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   force_attr        = get_attribute_and_validate( "force" );
   torque_attr       = get_attribute_and_validate( "torque" );
   mass_attr         = get_attribute_and_validate( "mass" );
   mass_rate_attr    = get_attribute_and_validate( "mass_rate" );
   inertia_attr      = get_attribute_and_validate( "inertia" );
   inertia_rate_attr = get_attribute_and_validate( "inertia_rate" );

   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntityBase::pack()
{
   // Check for initialization.
   if ( !initialized ) {
      ostringstream errmsg;
      errmsg << "DynamicalEntityBase::pack() ERROR: The initialize() function has not"
             << " been called!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "DynamicalEntityBase::pack():" << __LINE__ << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Call the PhysicalEntityBase pack routine.
   PhysicalEntityBase::pack();

   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntityBase::unpack()
{
   if ( !initialized ) {
      ostringstream errmsg;
      errmsg << "DynamicalEntityBase::unpack():" << __LINE__
             << " ERROR: The initialize() function has not been called!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "DynamicalEntityBase::unpack():" << __LINE__ << '\n'
          << "DynamicalEntity: lag comp type: " << this->object->lag_comp_type << '\n';
      message_publish( MSG_WARNING, msg.str().c_str() );
   }

   // Call the PhysicalEntityBase unpack routine.
   PhysicalEntityBase::unpack();

   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntityBase::debug_print(
   std::ostream &stream ) const
{
   // Call the PhysicalEntity print routine first.
   PhysicalEntityBase::debug_print( stream );

   de_packing_data.print_data();

   stream.precision( 15 );

   return;
}
