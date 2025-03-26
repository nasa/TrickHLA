/*!
@file SpaceFOM/RefFrameConditionalBase.cpp
@ingroup SpaceFOM
@brief This is a base class for implementing the TrickHLA Conditional class
for SpaceFOM RefFrame objects.

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
@trick_link_dependency{RefFrameConditionalBase.cpp}

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
#include "SpaceFOM/RefFrameConditionalBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
RefFrameConditionalBase::RefFrameConditionalBase(
   RefFrameBase &frame_ref )
   : TrickHLA::Conditional(),
     debug( false ),
     frame( frame_ref ),
     prev_data(),
     name_attr( NULL ),
     parent_name_attr( NULL ),
     state_attr( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
RefFrameConditionalBase::~RefFrameConditionalBase()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void RefFrameConditionalBase::initialize()
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
void RefFrameConditionalBase::initialize_callback(
   TrickHLA::Object *obj )
{
   // We must call the original function so that the callback is initialized.
   Conditional::initialize_callback( obj );

   // Get references to all the TrickHLA::Attribute for this object status.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   name_attr        = frame.name_attr;
   parent_name_attr = frame.parent_name_attr;
   state_attr       = frame.state_attr;

   // Mark this Conditional instance as initialized.
   this->initialized = true;

   return;
}

/*!
 * @job_class{scheduled}
 */
bool RefFrameConditionalBase::should_send(
   TrickHLA::Attribute *attr )
{
   bool send_attr = false;

   // If there is simulation data to compare to and if the attribute FOM name
   // has been specified, check the value of the current simulation variable
   // versus the previous value. Return true if there was a change.
   //
   // Check for Name change.
   if ( attr == name_attr ) {

      if ( frame.packing_data.name != NULL ) {
         if ( prev_data.name != NULL ) {
            if ( strcmp( frame.packing_data.name, prev_data.name ) ) {
               if ( trick_MM->delete_var( static_cast< void * >( prev_data.name ) ) ) {
                  message_publish( MSG_WARNING,
                                   "RefFrameConditionalBase::should_send():%d WARNING failed to delete Trick Memory for 'prev_data.name'\n",
                                   __LINE__ );
               }
               // Update the previous value.
               prev_data.name = trick_MM->mm_strdup( frame.packing_data.name );
               // Mark to send.
               send_attr = true;
            }
         } else {
            // Update the previous value.
            prev_data.name = trick_MM->mm_strdup( frame.packing_data.name );
            // Mark to send.
            send_attr = true;
         }
      } else {
         ostringstream errmsg;
         errmsg << "RefFrameConditionalBase::should_send():" << __LINE__
                << " ERROR: Unexpected NULL Name for RefFrame!\n";
         // Print message and terminate.
         TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
      }

   } else if ( attr == parent_name_attr ) {
      // Check for Parent Frame change.

      if ( frame.packing_data.parent_name != NULL ) {
         if ( prev_data.parent_name != NULL ) {
            if ( strcmp( frame.packing_data.parent_name, prev_data.parent_name ) ) {
               if ( trick_MM->delete_var( static_cast< void * >( prev_data.parent_name ) ) ) {
                  message_publish( MSG_WARNING,
                                   "RefFrameConditionalBase::should_send():%d WARNING failed to delete Trick Memory for 'prev_data.parent_name'\n",
                                   __LINE__ );
               }
               // Update the previous value.
               prev_data.parent_name = trick_MM->mm_strdup( frame.packing_data.parent_name );
               // Mark to send.
               send_attr = true;
            }
         } else {
            // Update the previous value.
            prev_data.parent_name = trick_MM->mm_strdup( frame.packing_data.parent_name );
            // Mark to send.
            send_attr = true;
         }
      } else {
         ostringstream errmsg;
         if ( frame.packing_data.name != NULL ) {
            errmsg << "RefFrameConditionalBase::should_send():" << __LINE__
                   << " ERROR: Unexpected NULL Parent Frame for RefFrame '" << frame.packing_data.name << "'!\n";
         } else {
            errmsg << "RefFrameConditionalBase::should_send():" << __LINE__
                   << " ERROR: Unexpected NULL Parent Frame for RefFrame!\n";
         }
         // Print message and terminate.
         TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
      }

   } else if ( attr == state_attr ) {
      // Check for change in state.

      if ( frame.packing_data.state != prev_data.state ) {

         // Update the previous value.
         prev_data.state = frame.packing_data.state;

         // Mark to send.
         send_attr = true;
      }

   } else {

      ostringstream errmsg;
      errmsg << "RefFrameConditionalBase::should_send("
             << attr->get_FOM_name() << "):" << __LINE__
             << "ERROR: Could not find the data for the specified FOM attribute!"
             << '\n';
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   return send_attr;
}
