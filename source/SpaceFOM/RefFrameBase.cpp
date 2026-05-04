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
@trick_link_dependency{SpaceTimeCoordinateConfig.cpp}
@trick_link_dependency{SpaceTimeCoordinateData.cpp}
@trick_link_dependency{SpaceTimeCoordinateEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, SISO, Sept 2010, --, Smackdown implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
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

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM includes.
#include "SpaceFOM/RefFrameBase.hh"
#include "SpaceFOM/SpaceTimeCoordinateConfig.hh"
#include "SpaceFOM/SpaceTimeCoordinateData.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

#define REF_FRAME_PACKING_DEBUG 0
#define REF_FRAME_PACKING_EXTRA_DEBUG 0

/*!
 * @job_class{initialization}
 */
RefFrameBase::RefFrameBase()
   : TrickHLA::Packing( "RefFrameBase" ),
     parent_frame( NULL ),
     name_attr( NULL ),
     parent_name_attr( NULL ),
     state_attr( NULL ),
     packing_data()
#if defined( USE_SPACEFOM_ENCODERS )
     ,
     stc_encoder( packing_data.state )
#endif // USE_SPACEFOM_ENCODERS
{
   return;
}

/*!
 * @job_class{shutdown}
 */
RefFrameBase::~RefFrameBase()
{
   return;
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{initialization}
 */
void RefFrameBase::base_config(
   bool               create,
   std::string const &sim_obj_name,
   std::string const &ref_frame_pkg_name,
   std::string const &ref_frame_fed_name,
   TrickHLA::Object  *mngr_object,
   bool const         publish,
   bool const         subscribe )
{
   string ref_frame_full_name = sim_obj_name + "." + ref_frame_pkg_name;

   // Make sure that the TrickHLA::Object pointer is not NULL.
   // If NULL, this it means this object has not been allocated yet.
   // If not allocated, there are two options:
   // 1). We are configuring in the input file, which is okay.
   // 2). We are configuring in default_data but forgot to allocate and
   //     assign the associated object in the 'create_connections()' routine.
   if ( mngr_object == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         ostringstream errmsg;
         errmsg << "RefFrameBase::base_config() Warning: " << endl
                << "\tThe TrickHLA::Object associated with object \'" << ref_frame_fed_name << "\' is NULL." << endl
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

   // Set the frame name.
   if ( !ref_frame_fed_name.empty() ) {
      set_name( ref_frame_fed_name );
   } else {
      ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameBase::base_config():" << __LINE__
             << " ERROR: Unexpected empty federation instance frame name!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine the publish and subscribe attribute values.
   bool publish_attr   = create || publish;
   bool subscribe_attr = !create || subscribe;

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   object->FOM_name            = "ReferenceFrame";
   object->name                = ref_frame_fed_name;
   object->create_HLA_instance = create;
   object->packing             = this;
   // Allocate the attributes for the RefFrameBase HLA object.
   object->attr_count = 3;
   object->attributes = static_cast< TrickHLA::Attribute * >( trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count ) );

   //
   // Specify the Reference Frame attributes.
   //
   object->attributes[0].FOM_name      = "name";
   object->attributes[0].trick_name    = ref_frame_full_name + string( ".packing_data.name" );
   object->attributes[0].config        = TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC;
   object->attributes[0].publish       = publish_attr;
   object->attributes[0].subscribe     = subscribe_attr;
   object->attributes[0].locally_owned = create;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

   object->attributes[1].FOM_name      = "parent_name";
   object->attributes[1].trick_name    = ref_frame_full_name + string( ".packing_data.parent_name" );
   object->attributes[1].config        = TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC;
   object->attributes[1].publish       = publish_attr;
   object->attributes[1].subscribe     = subscribe_attr;
   object->attributes[1].locally_owned = create;
   object->attributes[1].rti_encoding  = TrickHLA::ENCODING_UNICODE_STRING;

#if defined( USE_SPACEFOM_ENCODERS )
   object->attributes[2].FOM_name      = "state";
   object->attributes[2].trick_name    = ref_frame_full_name + string( ".stc_encoder.buffer" );
   object->attributes[2].config        = TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC;
   object->attributes[2].publish       = publish_attr;
   object->attributes[2].subscribe     = subscribe_attr;
   object->attributes[2].locally_owned = create;
   object->attributes[2].rti_encoding  = TrickHLA::ENCODING_NONE;
#else
   string const fom_name   = "state";
   string const trick_name = ref_frame_full_name + string( ".packing_data.state" );

   SpaceTimeCoordinateConfig::configure(
      &object->attributes[2],
      fom_name,
      trick_name,
      TrickHLA::CONFIG_INITIALIZE_AND_CYCLIC,
      publish_attr,
      subscribe_attr,
      create );
#endif

   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::initialize()
{
   // Must have federation instance name.
   if ( this->packing_data.name.empty() ) {
      ostringstream errmsg;

      string trick_name = ( name_attr != NULL ) ? name_attr->get_trick_name() : "";
      string fom_name   = ( name_attr != NULL ) ? name_attr->get_FOM_name() : "";

      errmsg << "SpaceFOM::RefFrameBase::initialize():" << __LINE__
             << " ERROR: For RefFrame object '"
             << ( ( object != NULL ) ? object->get_name() : "" )
             << "' with Attribute Trick name '" << trick_name
             << "' and FOM name '" << fom_name
             << "', detected unexpected NULL federation instance name!"
             << endl;

      // Print message and terminate.
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Should have federation instance parent frame name or empty name for root.
   if ( this->packing_data.parent_name.empty() ) {

      // Print message.
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_NO_MODULES ) ) {

         ostringstream errmsg;

         string trick_name = ( name_attr != NULL ) ? name_attr->get_trick_name() : "";
         string fom_name   = ( name_attr != NULL ) ? name_attr->get_FOM_name() : "";

         errmsg << "SpaceFOM::RefFrameBase::initialize():" << __LINE__
                << " WARNING: For RefFrame '" << this->packing_data.name
                << "' and object '" << ( ( object != NULL ) ? object->get_name() : "" )
                << "' with Attribute Trick name '" << trick_name
                << "' and FOM name '" << fom_name
                << "', detected unexpected empty federation instance parent frame name!"
                << endl;

         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }

      // Mark as root reference frame.
      this->is_root_node = true;

   } else {

      // Mark as NOT a root reference frame.
      this->is_root_node = false;
   }

   // Check to see if the parent reference frame has been set if this frame
   // is NOT the root frame.
   if ( !this->packing_data.parent_name.empty() && ( this->parent_frame == NULL ) ) {
      ostringstream errmsg;

      string trick_name = ( name_attr != NULL ) ? name_attr->get_trick_name() : "";
      string fom_name   = ( name_attr != NULL ) ? name_attr->get_FOM_name() : "";

      errmsg << "SpaceFOM::RefFrameBase::initialize():" << __LINE__
             << " ERROR: For RefFrame object '"
             << ( ( object != NULL ) ? object->get_name() : "" )
             << "' with Attribute Trick name '" << trick_name
             << "' and FOM name '" << fom_name
             << "', detected unexpected NULL parent frame reference!" << endl;

      // Print message and terminate.
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Associate the instantiated Manager object with this packing object.
   if ( this->object == NULL ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameBase::initialize():" << __LINE__
             << " ERROR: Unexpected NULL THLAManager object for ReferenceFrame \""
             << this->packing_data.name << "\"!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Initialize from the initial state of the working data.
   pack_from_working_data();

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
void RefFrameBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   TrickHLA::Packing::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   name_attr        = get_attribute_and_validate( "name" );
   parent_name_attr = get_attribute_and_validate( "parent_name" );
   state_attr       = get_attribute_and_validate( "state" );

   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::set_name( std::string const &new_name )
{
   // Check for initialization.
   if ( initialized ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameBase::set_name():" << __LINE__
             << " ERROR: The initialize() function has already been called" << endl;
      // Print message and terminate.
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Set the names.
   this->packing_data.name = new_name;
   this->name              = new_name;

   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::set_parent_name( std::string const &name )
{
   // Check for initialization.
   if ( initialized ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameBase::set_parent_name():" << __LINE__
             << " ERROR: The initialize() function has already been called" << endl;
      // Print message and terminate.
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Set the parent frame name appropriately.
   this->packing_data.parent_name = name;
   if ( this->packing_data.parent_name.empty() ) {
      this->is_root_node = true;
   } else {
      this->is_root_node = false;
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::set_parent_frame( RefFrameBase *pframe_ptr )
{
   // Check for initialization.
   if ( initialized ) {
      ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameBase::set_parent_frame():" << __LINE__
             << " ERROR: The initialize() function has already been called" << endl;
      // Print message and terminate.
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Set the parent frame reference pointer.
   this->parent_frame = pframe_ptr;
   this->parent       = pframe_ptr;

   // Set the parent frame name.
   if ( this->parent_frame != NULL ) {
      set_parent_name( this->parent_frame->packing_data.name );
   } else {
      set_parent_name( "" );
   }

   return;
}

/*!
 * @job_class{initialization}
 */
bool RefFrameBase::set_root( bool root_status )
{
   // If setting as root reference frame.
   if ( root_status ) {

      // Check to make sure predicates are satisfied.
      if ( this->parent_frame == NULL ) {

         // Check to make sure the parent name is empty.
         if ( this->packing_data.parent_name.empty() ) {
            // Set the is_root_node state to true.
            this->is_root_node = true;
            return ( true );
         } else {
            // Note that we DO NOT change the is_root_node state.
            return ( false );
         }

      } // Parent frame is not null.  Automatic fail.
      else {
         // Note that we DO NOT change the is_root_node state.
         return ( false );
      }

   } else {

      // If setting is NOT a root reference frame.

      // Check to make sure predicates are satisfied.
      if ( this->parent_frame != NULL ) {

         // Check for empty parent name string.
         if ( this->packing_data.parent_name.empty() ) {
            // Note that we DO NOT change the is_root_node state.
            return ( false );
         } else {
            // Set the is_root_node state to false.
            this->is_root_node = false;
            return ( true );
         }

      } else {
         // Parent frame is NULL.  Automatic fail.

         // Note that we DO NOT change the is_root_node state.
         return ( false );
      }
   }

   return ( true );
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::publish()
{
   if ( this->initialized ) {
      ostringstream errmsg;
      errmsg << "RefFrameBase::publish():" << __LINE__
             << " WARNING: Ignoring, reference frame already initialized!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } else {
      if ( object == NULL ) {
         ostringstream errmsg;
         errmsg << "RefFrameBase::publish():" << __LINE__
                << " ERROR: Unexpected NULL Object reference!" << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }
      if ( object->attributes == NULL ) {
         ostringstream errmsg;
         errmsg << "RefFrameBase::publish():" << __LINE__
                << " ERROR: For Object '" << object->get_name()
                << "', unexpected NULL object attribute reference!" << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }
      if ( object->attr_count <= 0 ) {
         ostringstream errmsg;
         errmsg << "RefFrameBase::publish():" << __LINE__
                << " ERROR: For Object '" << object->get_name()
                << "', unexpected non-zero object attribute count ("
                << object->attr_count << ")" << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      object->create_HLA_instance = true;
      for ( int i = 0; i < object->attr_count; ++i ) {
         object->attributes[i].publish       = true;
         object->attributes[i].subscribe     = false;
         object->attributes[i].locally_owned = true;
      }
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::subscribe()
{
   if ( this->initialized ) {
      ostringstream errmsg;
      errmsg << "RefFrameBase::subscribe():" << __LINE__
             << " WARNING: Ignoring, reference frame already initialized!" << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } else {
      if ( object == NULL ) {
         ostringstream errmsg;
         errmsg << "RefFrameBase::subscribe():" << __LINE__
                << " ERROR: Unexpected NULL Object reference!" << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }
      if ( object->attributes == NULL ) {
         ostringstream errmsg;
         errmsg << "RefFrameBase::subscribe():" << __LINE__
                << " ERROR: For Object '" << object->get_name()
                << "', unexpected NULL object attribute reference!" << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }
      if ( object->attr_count <= 0 ) {
         ostringstream errmsg;
         errmsg << "RefFrameBase::subscribe():" << __LINE__
                << " ERROR: For Object '" << object->get_name()
                << "', unexpected non-zero object attribute count ("
                << object->attr_count << ")" << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      object->create_HLA_instance = false;
      for ( int i = 0; i < object->attr_count; ++i ) {
         object->attributes[i].publish       = false;
         object->attributes[i].subscribe     = true;
         object->attributes[i].locally_owned = false;
      }
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameBase::pack()
{
   // Check for initialization.
   if ( !initialized ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_PACKING ) ) {
         ostringstream errmsg;
         errmsg << "RefFrameBase::pack() Warning: The initialize() function has not"
                << " been called!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }

   // Check for latency/lag compensation.
   if ( this->object->lag_comp == NULL ) {
      pack_from_working_data();
   }

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "RefFrameBase::pack():" << __LINE__ << endl;
      print_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

#if defined( USE_SPACEFOM_ENCODERS )
   // Encode the data into the buffer.
   stc_encoder.encode();
#endif

   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameBase::unpack()
{
   if ( !initialized ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_PACKING ) ) {
         ostringstream errmsg;
         errmsg << "RefFrameBase::unpack():" << __LINE__
                << " Warning: The initialize() function has not been called!" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }

#if defined( USE_SPACEFOM_ENCODERS )
   // Use the HLA encoder helpers to decode the PhysicalEntity fixed record.
   stc_encoder.decode();
#endif

   // Transfer the packing data into the working data.
   unpack_into_working_data();

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "RefFrameBase::unpack():" << __LINE__ << endl;
      print_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void RefFrameBase::print_data( std::ostream &stream ) const
{
   // Set the print precision.
   stream.precision( 15 );

   stream << "         Object-Name: '" << object->get_name() << "'" << endl;
   stream << "                time: " << packing_data.state.time << endl;
   packing_data.print_data( stream );
   stream << endl;

   return;
}
