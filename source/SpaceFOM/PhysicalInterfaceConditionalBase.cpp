/*!
@file SpaceFOM/PhysicalInterfaceConditionalBase.cpp
@ingroup SpaceFOM
@brief This is a base class for implementing the TrickHLA Conditional class
for SpaceFOM PhysicalInterface objects.

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
@trick_link_dependency{PhysicalInterfaceConditionalBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"

// Model include files.
#include "SpaceFOM/PhysicalInterfaceConditionalBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalInterfaceConditionalBase::PhysicalInterfaceConditionalBase(
   PhysicalInterfaceBase &interface_ref )
   : TrickHLA::Conditional(),
     debug( false ),
     interface( interface_ref ),
     prev_data(),
     name_attr( NULL ),
     parent_attr( NULL ),
     position_attr( NULL ),
     attitude_attr( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
PhysicalInterfaceConditionalBase::~PhysicalInterfaceConditionalBase()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void PhysicalInterfaceConditionalBase::initialize()
{
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
void PhysicalInterfaceConditionalBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   Conditional::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object status.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   name_attr     = interface.name_attr;
   parent_attr   = interface.parent_attr;
   position_attr = interface.position_attr;
   attitude_attr = interface.attitude_attr;

   // Mark this Conditional instance as initialized.
   this->initialized = true;

   return;
}

/*!
 * @job_class{scheduled}
 */
bool PhysicalInterfaceConditionalBase::should_send(
   TrickHLA::Attribute *attr )
{
   bool send_attr = false;

   // If there is simulation data to compare to and if the attribute FOM name
   // has been specified, check the value of the current simulation variable
   // versus the previous value. Return true if there was a change.
   //
   // Check for Name change.
   if ( attr == name_attr ) {

      if ( interface.packing_data.name != NULL ) {
         if ( prev_data.name != NULL ) {
            if ( strcmp( interface.packing_data.name, prev_data.name ) ) {
               if ( trick_MM->delete_var( static_cast< void * >( prev_data.name ) ) ) {
                  message_publish( MSG_WARNING,
                                   "PhysicalInterfaceConditionalBase::should_send():%d WARNING failed to delete Trick Memory for 'prev_data.name'\n",
                                   __LINE__ );
               }
               // Update the previous value.
               prev_data.name = trick_MM->mm_strdup( interface.packing_data.name );
               // Mark to send.
               send_attr = true;
            }
         } else {
            // Update the previous value.
            prev_data.name = trick_MM->mm_strdup( interface.packing_data.name );
            // Mark to send.
            send_attr = true;
         }
      } else {
         ostringstream errmsg;
         errmsg << "PhysicalInterfaceConditionalBase::should_send():" << __LINE__
                << " ERROR: Unexpected NULL Name for PhysicalInterface!\n";
         // Print message and terminate.
         TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
      }

   } // Check for Parent Frame change.
   else if ( attr == parent_attr ) {

      if ( interface.packing_data.parent_name != NULL ) {
         if ( prev_data.parent_name != NULL ) {
            if ( strcmp( interface.packing_data.parent_name, prev_data.parent_name ) ) {
               if ( trick_MM->delete_var( static_cast< void * >( prev_data.parent_name ) ) ) {
                  message_publish( MSG_WARNING,
                                   "PhysicalInterfaceConditionalBase::should_send():%d WARNING failed to delete Trick Memory for 'prev_data.parent_name'\n",
                                   __LINE__ );
               }
               // Update the previous value.
               prev_data.parent_name = trick_MM->mm_strdup( interface.packing_data.parent_name );
               // Mark to send.
               send_attr = true;
            }
         } else {
            // Update the previous value.
            prev_data.parent_name = trick_MM->mm_strdup( interface.packing_data.parent_name );
            // Mark to send.
            send_attr = true;
         }
      } else {
         ostringstream errmsg;
         errmsg << "PhysicalInterfaceConditionalBase::should_send():" << __LINE__
                << " ERROR: Unexpected NULL Parent Frame for PhysicalInterface!\n";
         // Print message and terminate.
         TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
      }

   } else if ( attr == position_attr ) {
      // Check for change in position.

      if ( ( interface.packing_data.position[0] != prev_data.position[0] )
           || ( interface.packing_data.position[1] != prev_data.position[1] )
           || ( interface.packing_data.position[2] != prev_data.position[2] ) ) {

         // Update the previous value.
         for ( int iinc = 0; iinc < 3; ++iinc ) {
            prev_data.position[iinc] = interface.packing_data.position[iinc];
         }

         // Mark to send.
         send_attr = true;
      }

   } else if ( attr == attitude_attr ) {
      // Check for change in interface attitude.

      if ( interface.packing_data.attitude != prev_data.attitude ) {

         // Update the previous value.
         prev_data.attitude = interface.packing_data.attitude;

         // Mark to send.
         send_attr = true;
      }

   } else {
      ostringstream errmsg;
      errmsg << "PhysicalInterfaceConditionalBase::should_send("
             << attr->get_FOM_name() << "):" << __LINE__
             << "ERROR: Could not find the data for the specified FOM attribute!\n";
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   return send_attr;
}
