/*!
@file models/simconfig/src/SimpleSimConfig.cpp
@ingroup TrickHLAModel
@brief This class contains a basic simulation configuration.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLAModel}

@tldh
@trick_link_dependency{../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../source/TrickHLA/Int64BaseTime.cpp}
@trick_link_dependency{../source/TrickHLA/Int64Interval.cpp}
@trick_link_dependency{../source/TrickHLA/Object.cpp}
@trick_link_dependency{../source/TrickHLA/Types.cpp}
@trick_link_dependency{simconfig/src/SimpleSimConfig.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2007, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h" // for send_hs

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Types.hh"

// Model include files.
#include "../include/SimpleSimConfig.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
SimpleSimConfig::SimpleSimConfig()
   : run_duration( 0.0 ),
     run_duration_base_time( 0L ),
     num_federates( 0 ),
     required_federates( NULL ),
     owner( NULL )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
SimpleSimConfig::~SimpleSimConfig()
{
   if ( required_federates != static_cast< char * >( NULL ) ) {
      if ( TMM_is_alloced( required_federates ) ) {
         TMM_delete_var_a( required_federates );
      }
      required_federates = static_cast< char * >( NULL );
   }

   if ( owner != static_cast< char * >( NULL ) ) {
      if ( TMM_is_alloced( owner ) ) {
         TMM_delete_var_a( owner );
      }
      owner = static_cast< char * >( NULL );
   }
}

/*!
 * @job_class{initialization}
 */
void SimpleSimConfig::initialize(
   int const                known_feds_count,
   TrickHLA::KnownFederate *known_feds )
{
   // Release the memory used by the required_federates c-string.
   if ( required_federates != static_cast< char * >( NULL ) ) {
      TMM_delete_var_a( required_federates );
      required_federates = static_cast< char * >( NULL );
   }

   ostringstream fed_list;
   int           req_fed_cnt = 0;

   // Build a comma separated list of required federate names.
   for ( int i = 0; i < known_feds_count; ++i ) {
      if ( known_feds[i].required ) {
         if ( req_fed_cnt > 0 ) {
            fed_list << ",";
         }
         fed_list << known_feds[i].name;
         req_fed_cnt++;
      }
   }

   // Set the number of required federates.
   this->num_federates = req_fed_cnt;

   // Make sure we use correct function so that it is Trick managed memory.
   this->required_federates = TMM_strdup( const_cast< char * >( fed_list.str().c_str() ) );
}

void SimpleSimConfig::pack()
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_PACKING ) ) {
      cout << "===================================================" << endl;
   }

   double terminate_time = exec_get_terminate_time();

   // Set the stop/termination time of the Trick simulation based on the
   // run_duration setting.
   if ( terminate_time >= 1.0e20 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_PACKING ) ) {
         cout << "SimpleSimConfig::pack():" << __LINE__
              << " Setting simulation termination time to "
              << run_duration << " seconds." << endl;
      }
      exec_set_terminate_time( this->run_duration );
   } else {
      // Set the run_duration based on the Trick simulation termination time
      // and the current granted HLA time.
      this->run_duration = terminate_time - this->object->get_granted_time().get_time_in_seconds();
      if ( run_duration < 0.0 ) {
         run_duration = 0.0;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_PACKING ) ) {
         cout << "SimpleSimConfig::pack(:" << __LINE__
              << " Setting simulation duration to "
              << run_duration << " seconds." << endl;
      }
   }

   // Encode the run duration into a 64 bit integer in the base time.
   this->run_duration_base_time = Int64BaseTime::to_base_time( this->run_duration );

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_PACKING ) ) {
      cout << "SimpleSimConfig::pack():" << __LINE__ << endl
           << "\t Object-Name:'" << this->object->get_name() << "'" << endl
           << "\t owner:'" << ( owner != NULL ? owner : "" ) << "'" << endl
           << "\t run_duration:" << run_duration << " seconds" << endl
           << "\t run_duration_base_time:" << run_duration_base_time << " "
           << Int64BaseTime::get_units() << endl
           << "\t num_federates:" << num_federates << endl
           << "\t required_federates:'" << ( required_federates != NULL ? required_federates : "" ) << "'" << endl
           << "===================================================" << endl;
   }
}

void SimpleSimConfig::unpack()
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_PACKING ) ) {
      cout << "===================================================" << endl;
   }

   // Decode the run duration from a 64 bit integer in the base time.
   this->run_duration = Int64BaseTime::to_seconds( this->run_duration_base_time );

   // Set the stop/termination time of the Trick simulation based on the
   // run_duration setting.
   if ( run_duration >= 0.0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_PACKING ) ) {
         cout << "SimpleSimConfig::unpack():" << __LINE__
              << " Setting simulation duration to "
              << run_duration << " seconds." << endl;
      }
      exec_set_terminate_time( this->run_duration );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_PACKING ) ) {
      cout << "SimpleSimConfig::unpack():" << __LINE__ << endl
           << "\t Object-Name:'" << this->object->get_name() << "'" << endl
           << "\t owner:'" << ( owner != NULL ? owner : "" ) << "'" << endl
           << "\t run_duration:" << run_duration << " seconds" << endl
           << "\t run_duration_base_time:" << run_duration_base_time << " "
           << Int64BaseTime::get_units() << endl
           << "\t num_federates:" << num_federates << endl
           << "\t required_federates:'" << ( required_federates != NULL ? required_federates : "" ) << "'" << endl
           << "===================================================" << endl;
   }
}
