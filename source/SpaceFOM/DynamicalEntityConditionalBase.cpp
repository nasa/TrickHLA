/*!
@file SpaceFOM/DynamicalEntityConditionalBase.cpp
@ingroup SpaceFOM
@brief This is a base class for implementing the TrickHLA Conditional class
for SpaceFOM DynamicalEntity objects.

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
@trick_link_dependency{DynamicalEntityConditionalBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, December 2023, --, Initial version.}
@revs_end

*/

// System include files.
#include <iostream>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Object.hh"

// Model include files.
#include "SpaceFOM/DynamicalEntityConditionalBase.hh"

using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityConditionalBase::DynamicalEntityConditionalBase(
   DynamicalEntityBase &entity_ref )
   : SpaceFOM::PhysicalEntityConditionalBase( entity_ref ),
     de_entity( entity_ref ),
     prev_de_data(),
     force_attr( NULL ),
     torque_attr( NULL ),
     mass_attr( NULL ),
     mass_rate_attr( NULL ),
     inertia_attr( NULL ),
     inertia_rate_attr( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
DynamicalEntityConditionalBase::~DynamicalEntityConditionalBase()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void DynamicalEntityConditionalBase::initialize()
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
void DynamicalEntityConditionalBase::initialize_callback(
   TrickHLA::Object *obj )
{

   // Get references to all the TrickHLA::Attribute for this object status.
   // We do this here so that we only do the attribute lookup once instead of
   // looking it up every time the unpack function is called.
   force_attr        = de_entity.force_attr;
   torque_attr       = de_entity.torque_attr;
   mass_attr         = de_entity.mass_attr;
   mass_rate_attr    = de_entity.mass_rate_attr;
   inertia_attr      = de_entity.inertia_attr;
   inertia_rate_attr = de_entity.inertia_rate_attr;

   // We must call the original function so that the callback is initialized.
   PhysicalEntityConditionalBase::initialize_callback( obj );

   return;
}

/*!
 * @job_class{scheduled}
 */
bool DynamicalEntityConditionalBase::should_send(
   TrickHLA::Attribute *attr )
{
   bool send_attr = false;

   // If there is simulation data to compare to and if the attribute FOM name
   // has been specified, check the value of the current simulation variable
   // versus the previous value. Return true if there was a change.
   //
   // Check for change in force.
   if ( attr == force_attr ) {

      if ( ( de_entity.de_packing_data.force[0] != prev_de_data.force[0] )
           || ( de_entity.de_packing_data.force[1] != prev_de_data.force[1] )
           || ( de_entity.de_packing_data.force[2] != prev_de_data.force[2] ) ) {

         // Update the previous value.
         for ( int iinc = 0; iinc < 3; ++iinc ) {
            prev_de_data.force[iinc] = de_entity.de_packing_data.force[iinc];
         }

         // Mark to send.
         send_attr = true;
      }

   } // Check for change in torque.
   else if ( attr == torque_attr ) {

      if ( ( de_entity.de_packing_data.torque[0] != prev_de_data.torque[0] )
           || ( de_entity.de_packing_data.torque[1] != prev_de_data.torque[1] )
           || ( de_entity.de_packing_data.torque[2] != prev_de_data.torque[2] ) ) {

         // Update the previous value.
         for ( int iinc = 0; iinc < 3; ++iinc ) {
            prev_de_data.torque[iinc] = de_entity.de_packing_data.torque[iinc];
         }

         // Mark to send.
         send_attr = true;
      }

   } // Check for change in mass.
   else if ( attr == mass_attr ) {

      if ( de_entity.de_packing_data.mass != prev_de_data.mass ) {

         // Update the previous value.
         prev_de_data.mass = de_entity.de_packing_data.mass;

         // Mark to send.
         send_attr = true;
      }

   } // Check for change in mass rate.
   else if ( attr == mass_rate_attr ) {

      if ( de_entity.de_packing_data.mass_rate != prev_de_data.mass_rate ) {

         // Update the previous value.
         prev_de_data.mass_rate = de_entity.de_packing_data.mass_rate;

         // Mark to send.
         send_attr = true;
      }

   } // Check for change in inertia matrix.
   else if ( attr == inertia_attr ) {

      if ( ( de_entity.de_packing_data.inertia[0][0] != prev_de_data.inertia[0][0] )
           || ( de_entity.de_packing_data.inertia[0][1] != prev_de_data.inertia[0][1] )
           || ( de_entity.de_packing_data.inertia[0][2] != prev_de_data.inertia[0][2] )
           || ( de_entity.de_packing_data.inertia[1][0] != prev_de_data.inertia[1][0] )
           || ( de_entity.de_packing_data.inertia[1][1] != prev_de_data.inertia[1][1] )
           || ( de_entity.de_packing_data.inertia[1][2] != prev_de_data.inertia[1][2] )
           || ( de_entity.de_packing_data.inertia[2][0] != prev_de_data.inertia[2][0] )
           || ( de_entity.de_packing_data.inertia[2][1] != prev_de_data.inertia[2][1] )
           || ( de_entity.de_packing_data.inertia[2][2] != prev_de_data.inertia[2][2] ) ) {

         // Update the previous value.
         for ( int iinc = 0; iinc < 3; ++iinc ) {
            for ( int jinc = 0; jinc < 3; ++jinc ) {
               prev_de_data.inertia[iinc][jinc] = de_entity.de_packing_data.inertia[iinc][jinc];
            }
         }

         // Mark to send.
         send_attr = true;
      }

   } // Check for change in inertia rate matrix.
   else if ( attr == inertia_rate_attr ) {

      if ( ( de_entity.de_packing_data.inertia_rate[0][0] != prev_de_data.inertia_rate[0][0] )
           || ( de_entity.de_packing_data.inertia_rate[0][1] != prev_de_data.inertia_rate[0][1] )
           || ( de_entity.de_packing_data.inertia_rate[0][2] != prev_de_data.inertia_rate[0][2] )
           || ( de_entity.de_packing_data.inertia_rate[1][0] != prev_de_data.inertia_rate[1][0] )
           || ( de_entity.de_packing_data.inertia_rate[1][1] != prev_de_data.inertia_rate[1][1] )
           || ( de_entity.de_packing_data.inertia_rate[1][2] != prev_de_data.inertia_rate[1][2] )
           || ( de_entity.de_packing_data.inertia_rate[2][0] != prev_de_data.inertia_rate[2][0] )
           || ( de_entity.de_packing_data.inertia_rate[2][1] != prev_de_data.inertia_rate[2][1] )
           || ( de_entity.de_packing_data.inertia_rate[2][2] != prev_de_data.inertia_rate[2][2] ) ) {

         // Update the previous value.
         for ( int iinc = 0; iinc < 3; ++iinc ) {
            for ( int jinc = 0; jinc < 3; ++jinc ) {
               prev_de_data.inertia_rate[iinc][jinc] = de_entity.de_packing_data.inertia_rate[iinc][jinc];
            }
         }

         // Mark to send.
         send_attr = true;
      }

   } else {

      // We did not find the attribute in the DynamicalEntity, so now check the
      // PhysicalEntity.  This call will terminate with an error message if
      // the attribute still isn't found.
      if ( PhysicalEntityConditionalBase::should_send( attr ) ) {

         // Mark to send.
         send_attr = true;
      }
   }

   return send_attr;
}
