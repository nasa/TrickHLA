/*!
@file QuaternionPacking.cpp
@ingroup SpaceFOM
@brief This class provides an example class for testing the encoding and
decoding of the SpaceFOM QuaternionState data type.

@copyright Copyright 2025 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../source/TrickHLA/Object.cpp}
@trick_link_dependency{../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../source/TrickHLA/Types.cpp}
@trick_link_dependency{models/src/QuaternionPacking.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, April 2025, --, Initial version.}
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
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/ExecutionControl.hh"

// Artemis FOM includes.
#include "models/include/QuaternionPacking.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
QuaternionPacking::QuaternionPacking()
   : debug( false ),
     test( true ),
     working_data( NULL ),
     quat_attr( NULL ),
     packing_data(),
     quat_encoder( packing_data )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
QuaternionPacking::~QuaternionPacking()
{
}

/*!
 * @details These can be overridden in the input file.
 * @job_class{default_data}
 */
void QuaternionPacking::base_config(
   bool                      publishes,
   char const               *sim_obj_name,
   char const               *packing_name,
   SpaceFOM::QuaternionData *working,
   TrickHLA::Object         *mngr_object  )
{
   string stc_name_str = string( sim_obj_name ) + "." + string( packing_name );
   string trick_name_str;

   // Associate the instantiated Manager object with this packing object.
   if ( mngr_object == NULL ) {
      if ( this->object == NULL ) {
         ostringstream errmsg;
         errmsg << "SpaceFOM::QuaternionPacking::base_config():" << __LINE__
                << " WARNING: Unexpected NULL THLAManager object!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      // If the mngr_object is not set but the object is, use that.
   } else {
      if ( this->object == NULL ) {
         // If the object is not already set, use the passed in mngr_object.
         this->object = mngr_object;
      } else {
         ostringstream errmsg;
         errmsg << "SpaceFOM::QuaternionPacking::base_config():" << __LINE__
                << " WARNING: THLAManager object is already set!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // Set the working data pointer if not NULL.
   if ( working != NULL ) {
      working_data = working;
   }

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the object.
   object->FOM_name            = "QuatTest";
   object->name                = "quat_test";
   object->create_HLA_instance = publishes;
   object->packing             = this;
   // Allocate the attributes for the QuaternionPacking HLA object.
   object->attr_count = 1;
   object->attributes = static_cast< TrickHLA::Attribute * >( trick_MM->declare_var( "TrickHLA::Attribute", object->attr_count ) );

   //
   // Specify the attributes.
   //
   object->attributes[0].FOM_name      = "quaternion";
   trick_name_str                      = stc_name_str + string( ".quat_encoder.buffer" );
   object->attributes[0].trick_name    = trick_name_str.c_str();
   object->attributes[0].config        = static_cast< TrickHLA::DataUpdateEnum >( TrickHLA::CONFIG_INITIALIZE + TrickHLA::CONFIG_CYCLIC );
   object->attributes[0].publish       = publishes;
   object->attributes[0].subscribe     = !publishes;
   object->attributes[0].locally_owned = publishes;
   object->attributes[0].rti_encoding  = TrickHLA::ENCODING_NONE;

   return;
}

/*!
 * @job_class{initialization}
 */
void QuaternionPacking::initialize()
{

   // Check to make sure the working data has been set.
   if ( working_data == NULL ){
      ostringstream errmsg;
      errmsg << "SpaceFOM::QuaternionPacking::initialize():" << __LINE__
             << " ERROR: NULL latitude reference!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Initialize from the initial state of the working data.
   pack_from_working_data();

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
void QuaternionPacking::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   TrickHLA::Packing::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object type.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   quat_attr = get_attribute_and_validate( "quaternion" );

   return;
}

/*!
 * @job_class{initialization}
 */
void QuaternionPacking::publish()
{
   if ( this->initialized ) {
      ostringstream errmsg;
      errmsg << "QuaternionPacking::publish():" << __LINE__
             << " WARNING: Ignoring, already initialized!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } else {
      object->create_HLA_instance         = true;
      object->attributes[0].publish       = true;
      object->attributes[0].subscribe     = false;
      object->attributes[0].locally_owned = true;
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void QuaternionPacking::subscribe()
{
   if ( this->initialized ) {
      ostringstream errmsg;
      errmsg << "QuaternionPacking::subscribe():" << __LINE__
             << " WARNING: Ignoring, already initialized!\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } else {
      object->create_HLA_instance         = false;
      object->attributes[0].publish       = false;
      object->attributes[0].subscribe     = true;
      object->attributes[0].locally_owned = false;
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionPacking::pack()
{
   // Check for initialization.
   if ( !initialized ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_PACKING ) ) {
         ostringstream errmsg;
         errmsg << "QuaternionPacking::pack() Warning: The initialize() function has not"
                << " been called!\n";
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
      msg << "QuaternionPacking::pack():" << __LINE__ << '\n';
      print_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Encode the data into the buffer.
   quat_encoder.encode();

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionPacking::unpack()
{
   if ( !initialized ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_PACKING ) ) {
         ostringstream errmsg;
         errmsg << "QuaternionPacking::unpack():" << __LINE__
                << " Warning: The initialize() function has not been called!\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }

   // Use the HLA encoder helpers to decode the Quaternion fixed record.
   quat_encoder.decode();

   // Transfer the packing data into the working data.
   unpack_into_working_data();

   // Check to see if testing incoming values.
   if ( test ) {
      unpack_test();
   }

   // Print out debug information if desired.
   if ( debug ) {
      ostringstream msg;
      msg << "QuaternionPacking::unpack():" << __LINE__ << '\n';
      print_data( msg );
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionPacking::pack_from_working_data()
{
   int iinc;

   // NOTE: Because TrickHLA handles the bundling of locally owned attributes
   // we do not need to check the ownership status of them here like we do
   // in the unpack() function, since we don't run the risk of corrupting our
   // state.

   // Pack the data.
   // Attitude quaternion.
   packing_data.scalar = working_data->scalar;
   for ( iinc = 0; iinc < 3; ++iinc ) {
      packing_data.vector[iinc] = working_data->vector[iinc];
   }

   // Return to the calling routine.
   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionPacking::unpack_into_working_data()
{
   // If the HLA attribute has changed and is remotely owned (i.e. is
   // coming from another federate) then override our simulation state with the
   // incoming value.  If we locally own the attribute then we do not want to
   // override it's value.  If we did not do this check then we would be
   // overriding state of something we own and publish with whatever value
   // happen to be in the local variable, which would cause data corruption of
   // the state.  We always need to do this check because ownership transfers
   // could happen at any time or the data could be at a different rate.

   // Unpack the space-time coordinate state.
   if ( quat_attr->is_received() ) {

      // Unpack the data.
      // Attitude quaternion.
      working_data->scalar = packing_data.scalar;
      for ( int iinc = 0; iinc < 3; ++iinc ) {
         working_data->vector[iinc] = packing_data.vector[iinc];
      }
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionPacking::unpack_test()
{
   double tol = 4.0 * std::numeric_limits< double >::min();

   // Scalar
   if ( abs( test_data.scalar - packing_data.scalar ) > tol ) {
      ostringstream msg;
      msg << "QuaternionPacking::unpack_test(): " << __LINE__
          << " : Failed scalar test!" << std::endl;
      message_publish( MSG_ERROR, msg.str().c_str() );
   } else {
      ostringstream msg;
      msg << "QuaternionPacking::unpack_test(): " << __LINE__
          << " : Passed scalar test!" << std::endl;
      message_publish( MSG_INFO, msg.str().c_str() );
   }

   // Vector
   if (     (abs( test_data.vector[0] - packing_data.vector[0] ) > tol )
         || (abs( test_data.vector[1] - packing_data.vector[1] ) > tol )
         || (abs( test_data.vector[2] - packing_data.vector[2] ) > tol ) ) {
      ostringstream msg;
      msg << "QuaternionPacking::unpack_test(): " << __LINE__
          << " : Failed vector test!" << std::endl;
      message_publish( MSG_ERROR, msg.str().c_str() );
   } else {
      ostringstream msg;
      msg << "QuaternionPacking::unpack_test(): " << __LINE__
          << " : Passed vector test!" << std::endl;
      message_publish( MSG_INFO, msg.str().c_str() );
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void QuaternionPacking::print_data( std::ostream &stream ) const
{
   // Set the print precision.
   stream.precision( 15 );

   stream << "\tObject-Name: '" << object->get_name() << "'\n";
   packing_data.print_data( stream );
   stream << '\n';

   return;
}
