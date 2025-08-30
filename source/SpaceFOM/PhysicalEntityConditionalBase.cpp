/*!
@file SpaceFOM/PhysicalEntityConditionalBase.cpp
@ingroup SpaceFOM
@brief This is a base class for implementing the TrickHLA Conditional class
for SpaceFOM PhysicalEntity objects.

@copyright Copyright 2020 United States Government as represented by the
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
@trick_link_dependency{../TrickHLA/Conditional.cpp}
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Object.cpp}
@trick_link_dependency{PhysicalEntityConditionalBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System includes.
#include <cstring>
#include <ostream>
#include <sstream>

// SpaceFOM includes.
#include "SpaceFOM/PhysicalEntityBase.hh"
#include "SpaceFOM/PhysicalEntityConditionalBase.hh"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityConditionalBase::PhysicalEntityConditionalBase(
   PhysicalEntityBase &entity_ref )
   : TrickHLA::Conditional(),
     debug( false ),
     entity( entity_ref ),
     prev_data(),
     name_attr( NULL ),
     type_attr( NULL ),
     status_attr( NULL ),
     parent_frame_attr( NULL ),
     state_attr( NULL ),
     accel_attr( NULL ),
     ang_accel_attr( NULL ),
     cm_attr( NULL ),
     body_frame_attr( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
PhysicalEntityConditionalBase::~PhysicalEntityConditionalBase()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalEntityConditionalBase::initialize()
{
   // Call the base class function.
   TrickHLA::Conditional::initialize();

   // Return to calling routine.
   return;
}

/*!
 * @details From the TrickHLA::Packing class. We override this function so
 * that we can initialize references to the TrickHLA::Attribute's that are
 * used in the unpack function to handle attribute ownership and different
 * attribute data rates. Use the initialize callback function as a way to
 * setup TrickHLA-Attribute references which are used to determine ownership
 * or if data for an attribute was received.
 *
 * @job_class{initialization}
 */
void PhysicalEntityConditionalBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   Conditional::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object status.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   name_attr         = entity.name_attr;
   type_attr         = entity.type_attr;
   status_attr       = entity.status_attr;
   parent_frame_attr = entity.parent_frame_attr;
   state_attr        = entity.state_attr;
   accel_attr        = entity.accel_attr;
   ang_accel_attr    = entity.ang_accel_attr;
   cm_attr           = entity.cm_attr;
   body_frame_attr   = entity.body_frame_attr;

   // Mark this Conditional instance as initialized.
   this->initialized = true;

   return;
}

/*!
 * @job_class{scheduled}
 */
bool PhysicalEntityConditionalBase::should_send(
   TrickHLA::Attribute *attr )
{
   bool send_attr = false;

   // If there is simulation data to compare to and if the attribute FOM name
   // has been specified, check the value of the current simulation variable
   // versus the previous value. Return true if there was a change.
   //
   if ( attr == name_attr ) {

      // Check for Name change.
      if ( entity.pe_packing_data.name != prev_data.name ) {
         // Update the previous value.
         prev_data.name = entity.pe_packing_data.name;
         // Mark to send.
         send_attr = true;
      }

   } else if ( attr == type_attr ) {

      // Check for Type change.
      if ( entity.pe_packing_data.type != prev_data.type ) {
         // Update the previous value.
         prev_data.type = entity.pe_packing_data.type;
         // Mark to send.
         send_attr = true;
      }

   } else if ( attr == status_attr ) {

      // Check for Status change.
      if ( entity.pe_packing_data.status != prev_data.status ) {
         // Update the previous value.
         prev_data.status = entity.pe_packing_data.status;
         // Mark to send.
         send_attr = true;
      }

   } else if ( attr == parent_frame_attr ) {

      // Check for Parent Frame change.
      if ( entity.pe_packing_data.parent_frame != prev_data.parent_frame ) {
         // Update the previous value.
         prev_data.parent_frame = entity.pe_packing_data.parent_frame;
         // Mark to send.
         send_attr = true;
      }

   } // Check for change in state.
   else if ( attr == state_attr ) {

      if ( entity.pe_packing_data.state != prev_data.state ) {

         // Update the previous value.
         prev_data.state = entity.pe_packing_data.state;

         // Mark to send.
         send_attr = true;
      }

   } // Check for change in translational acceleration.
   else if ( attr == accel_attr ) {

      if ( ( entity.pe_packing_data.accel[0] != prev_data.accel[0] )
           || ( entity.pe_packing_data.accel[1] != prev_data.accel[1] )
           || ( entity.pe_packing_data.accel[2] != prev_data.accel[2] ) ) {

         // Update the previous value.
         for ( int iinc = 0; iinc < 3; ++iinc ) {
            prev_data.accel[iinc] = entity.pe_packing_data.accel[iinc];
         }

         // Mark to send.
         send_attr = true;
      }

   } // Check for change in angular acceleration.
   else if ( attr == ang_accel_attr ) {

      if ( ( entity.pe_packing_data.ang_accel[0] != prev_data.ang_accel[0] )
           || ( entity.pe_packing_data.ang_accel[1] != prev_data.ang_accel[1] )
           || ( entity.pe_packing_data.ang_accel[2] != prev_data.ang_accel[2] ) ) {

         // Update the previous value.
         for ( int iinc = 0; iinc < 3; ++iinc ) {
            prev_data.ang_accel[iinc] = entity.pe_packing_data.ang_accel[iinc];
         }

         // Mark to send.
         send_attr = true;
      }

   } // Check for change in center of mass (cm).
   else if ( attr == cm_attr ) {

      if ( ( entity.pe_packing_data.cm[0] != prev_data.cm[0] )
           || ( entity.pe_packing_data.cm[1] != prev_data.cm[1] )
           || ( entity.pe_packing_data.cm[2] != prev_data.cm[2] ) ) {

         // Update the previous value.
         for ( int iinc = 0; iinc < 3; ++iinc ) {
            prev_data.cm[iinc] = entity.pe_packing_data.cm[iinc];
         }

         // Mark to send.
         send_attr = true;
      }

   } // Check for change in body frame definition.
   else if ( attr == body_frame_attr ) {

      if ( entity.pe_packing_data.body_wrt_struct != prev_data.body_wrt_struct ) {

         // Update the previous value.
         prev_data.body_wrt_struct = entity.pe_packing_data.body_wrt_struct;

         // Mark to send.
         send_attr = true;
      }

   } else {
      ostringstream errmsg;

      errmsg << "PhysicalEntityConditionalBase::should_send("
             << attr->get_FOM_name() << "):" << __LINE__
             << "ERROR: Could not find the data for the specified FOM attribute!"
             << endl;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   return send_attr;
}
