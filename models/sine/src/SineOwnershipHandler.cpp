/*!
@ingroup Sine
@file models/sine/src/SineOwnershipHandler.cpp
@brief This class handles the HLA ownership transfer for the sine wave simulation.

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
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../../source/TrickHLA/OwnershipHandler.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{sine/src/SineOwnershipHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial implementation.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

// TrickHLA include files.
#include "TrickHLA/Object.hh"
#include "TrickHLA/OwnershipHandler.hh"
#include "TrickHLA/Types.hh"

// Model include files.
#include "../include/SineOwnershipHandler.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
SineOwnershipHandler::SineOwnershipHandler()
   : TrickHLA::OwnershipHandler()
{
   return;
} // Default constructor.

/*!
 * @job_class{shutdown}
 */
SineOwnershipHandler::~SineOwnershipHandler()
{
   return;
}

/*!
 * @details From the TrickHLA::OwnershipHandler class. We override this
 * function so that we can initialize ownership transfer of some attributes
 * at a specific time.
 *
 * @job_class{initialization}
 */
void SineOwnershipHandler::initialize_callback(
   TrickHLA::Object *obj )
{
   // Make sure we call the original function so that the callback is initialized.
   OwnershipHandler::initialize_callback( obj );

   // Get the vector of attribute FOM names.
   VectorOfStrings attr_FOM_names = get_attribute_FOM_names();

   int const TEST_CASE = 4;

   switch ( TEST_CASE ) {
      case 0:
         // Some examples on pulling attribute ownership.

         // Examples showing how to Pull all attributes.
         pull_ownership(); // As soon as possible for all attributes.
         pull_ownership( 3.0 );
         pull_ownership( 5.0 );

         // Examples showing how to Pull specific attributes.
         pull_ownership( "Time" ); // As soon as possible for this attribute.
         pull_ownership( "Value", 6.1 );

         for ( unsigned int i = 0; i < attr_FOM_names.size(); ++i ) {
            pull_ownership( attr_FOM_names[i].c_str(), 7.0 );
         }
         break;

      case 1:
         // Some examples on pushing attribute ownership.

         // Examples showing how to Push all attributes.
         push_ownership(); // As soon as possible for all attributes.
         push_ownership( 3.0 );
         push_ownership( 5.0 );

         // Examples showing how to Push specific attributes.
         push_ownership( "Time" ); // As soon as possible for this attribute.
         push_ownership( "Value", 6.1 );

         for ( unsigned int i = 0; i < attr_FOM_names.size(); ++i ) {
            push_ownership( attr_FOM_names[i].c_str(), 7.0 );
         }
         break;

      case 2:
         // Example showing a mix of pushing and pulling attribute ownership.

         push_ownership(); // As soon as possible for all attributes.
         pull_ownership( 3.0 );
         push_ownership( 5.0 );
         pull_ownership( 7.0 );
         push_ownership( "Value", 9.0 );
         break;

      case 3:
         // Example showing a simple push and pull of ownership.

         push_ownership( 1.0 );
         pull_ownership( 5.0 );
         break;

      case 4:
         // Example showing a simple push and pull of ownership
         // for a single attribute.

         // This example assumes the S_define is configured to do ownership
         // transfer of the Name attribute at initialization using a job like this:
         // P_MPI_1 ("initialization") thla_manager.push_ownership_at_init( "A-side-Federate.Sine", "Name" );

         pull_ownership( "Name", 5.0 );
         break;

      default:
         // No test selected.
         break;
   }
}
