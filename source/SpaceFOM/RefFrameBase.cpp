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
using namespace TrickHLA;
using namespace SpaceFOM;

#define REF_FRAME_PACKING_DEBUG 0
#define REF_FRAME_PACKING_EXTRA_DEBUG 0

/*!
 * @job_class{initialization}
 */
RefFrameBase::RefFrameBase()
   : debug( false ),
     initialized( false ),
     name_attr( NULL ),
     parent_name_attr( NULL ),
     state_attr( NULL ),
     time( 0.0 ),
     name( NULL ),
     parent_name( NULL ),
     parent_frame( NULL ),
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
   char const       *sim_obj_name,
   char const       *ref_frame_obj_name,
   char const       *ref_frame_parent_name,
   char const       *ref_frame_name,
   bool              publishes,
   TrickHLA::Object *mngr_object )
{
   string ref_frame_name_str = string( sim_obj_name ) + "." + string( ref_frame_obj_name );
   string trick_name_str;

   // Associate the instantiated Manager object with this packing object.
   if ( mngr_object == NULL ) {
      if ( this->object == NULL ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::RefFrameBase::default_data():" << __LINE__
                << " WARNING: Unexpected NULL THLAManager object for ReferenceFrame \""
                << ref_frame_name << "\"!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      // If the mngr_object is not set but the object is, use that.
   }
   else {
      if ( this->object == NULL ) {
         // If the object is not already set, use the passed in mngr_object.
         this->object = mngr_object;
      }
      else{
         ostringstream errmsg;
         errmsg << "SpaceFOM::RefFrameBase::default_data():" << __LINE__
                << " WARNING: THLAManager object for ReferenceFrame \""
                << ref_frame_name << "\" is already set!" << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }


   // Set the frame name and parent frame name.
   if( ref_frame_parent_name != NULL ){
      this->parent_name = trick_MM->mm_strdup( ref_frame_parent_name );
   }
   else{
      this->parent_name = trick_MM->mm_strdup( "" );
   }
   if( ref_frame_name != NULL ){
      this->name = trick_MM->mm_strdup( ref_frame_name );
   }
   else{
      ostringstream errmsg;
      errmsg << "SpaceFOM::RefFrameBase::default_data():" << __LINE__
             << " WARNING: Unexpected NULL federation instance frame name!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

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
         send_hs( stderr, (char *)errmsg.str().c_str() );
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
         send_hs( stderr, (char *)errmsg.str().c_str() );
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
   ostringstream errmsg;

   // Check for initialization.
   if ( initialized ) {
      errmsg << "SpaceFOM::RefFrameBase::set_parent_name():" << __LINE__
             << " ERROR: The initialize() function has already been called" << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Set the parent frame name appropriately.
   if ( this->parent_name != NULL ) {
      trick_MM->delete_var( (void *)this->parent_name );
   }
   if ( name != NULL ) {
      this->parent_name = trick_MM->mm_strdup( name );
   }
   else {
      this->parent_name = NULL;
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::set_parent_frame( RefFrameBase * pframe_ptr )
{
   ostringstream errmsg;

   // Check for initialization.
   if ( initialized ) {
      errmsg << "SpaceFOM::RefFrameBase::set_parent_frame():" << __LINE__
             << " ERROR: The initialize() function has already been called" << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Set the parent frame reference pointer.
   this->parent_frame = pframe_ptr;

   // Set the parent frame name.
   if ( this->parent_frame != NULL ){
      this->set_parent_name( this->parent_frame->name );
   }
   else {
      this->set_parent_name( NULL );
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::publish()
{
   ostringstream errmsg;

   if ( this->initialized ){
      ostringstream errmsg;
      errmsg << "RefFrameBase::publish():" << __LINE__
             << " WARNING: Ignoring, reference frame already initialized!" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   }
   else {
      object->create_HLA_instance         = true;
      object->attributes[0].publish       = true;
      object->attributes[0].subscribe     = false;
      object->attributes[0].locally_owned = true;
      object->attributes[1].publish       = true;
      object->attributes[1].subscribe     = false;
      object->attributes[1].locally_owned = true;
      object->attributes[2].publish       = true;
      object->attributes[2].subscribe     = false;
      object->attributes[2].locally_owned = true;
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameBase::subscribe()
{
   ostringstream errmsg;

   if ( this->initialized ){
      ostringstream errmsg;
      errmsg << "RefFrameBase::publish():" << __LINE__
             << " WARNING: Ignoring, reference frame already initialized!" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   }
   else {
      object->create_HLA_instance         = false;
      object->attributes[0].publish       = false;
      object->attributes[0].subscribe     = true;
      object->attributes[0].locally_owned = false;
      object->attributes[1].publish       = false;
      object->attributes[1].subscribe     = true;
      object->attributes[1].locally_owned = false;
      object->attributes[2].publish       = false;
      object->attributes[2].subscribe     = true;
      object->attributes[2].locally_owned = false;
   }

   return;
}

/*!
 * @job_class{default_data}
 */
void RefFrameBase::set_object( TrickHLA::Object * mngr_obj )
{
   ostringstream errmsg;

   // Check for initialization.
   if ( initialized ) {
      errmsg << "SpaceFOM::RefFrameBase::set_object():" << __LINE__
             << " ERROR: The initialize() function has already been called" << THLA_ENDL;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Assign the object.
   this->object = mngr_obj;

   return;
}

