/*!
@file TrickHLA/ExecutionConfiguration.cpp
@ingroup TrickHLA
@brief Implementation of the TrickHLA Execution Configuration Object (ExCO).

\par<b>Assumptions and Limitations:</b>
- One and only one ExecutionConfiguration object should exist in an federation
execution.

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
@trick_link_dependency{Attribute.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionConfiguration.cpp}
@trick_link_dependency{ExecutionConfigurationBase.cpp}
@trick_link_dependency{ExecutionControl.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{Packing.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2007, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, TrickHLA support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

// Trick include files.
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionConfiguration.hh"
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/ExecutionControl.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Packing.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

#ifdef __cplusplus
extern "C" {
#endif
// Include the Trick generated ATTRIBUTES for the ExecutionConfiguration class.
// This is used to set up Trick based simulation variable mapping into the
// TrickHLA::Object associated with this class.
extern ATTRIBUTES attrTrickHLA__ExecutionConfiguration[];
#ifdef __cplusplus
}
#endif

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration::ExecutionConfiguration()
   : TrickHLA::ExecutionConfigurationBase(),
     run_duration( 0.0 ),
     run_duration_base_time( 0L ),
     num_federates( 0 ),
     required_federates( NULL ),
     owner( NULL )
{
   return;
}

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration::ExecutionConfiguration(
   char const *s_define_name )
   : TrickHLA::ExecutionConfigurationBase( s_define_name ),
     run_duration( 0.0 ),
     run_duration_base_time( 0L ),
     num_federates( 0 ),
     required_federates( NULL ),
     owner( NULL )
{
   return;
}

/*!
 * @details Even though this is a pure virtual destructor, we provide a
 * default implementation that can be called from an inheriting class.
 * @job_class{shutdown}
 */
ExecutionConfiguration::~ExecutionConfiguration() // RETURN: -- None.
{
   if ( required_federates != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( required_federates ) ) ) {
         message_publish( MSG_WARNING, "ExecutionConfiguration::~ExecutionConfiguration():%d WARNING failed to delete Trick Memory for 'required_federates'\n",
                          __LINE__ );
      }
      required_federates = NULL;
   }

   if ( owner != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( owner ) ) ) {
         message_publish( MSG_WARNING, "ExecutionConfiguration::~ExecutionConfiguration():%d WARNING failed to delete Trick Memory for 'owner'\n",
                          __LINE__ );
      }
      owner = NULL;
   }
}

/*!
 * @details These can be overridden in the input.py file.
 * @job_class{default_data}
 */
void ExecutionConfiguration::configure_attributes()
{
   string exco_name_str;
   string trick_name_str;

   // Check to make sure we have an S_define name for this ExCO instance.
   if ( S_define_name == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionConfiguration::configure_attributes():" << __LINE__
             << " ERROR: Unexpected NULL S_define_name.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Convert the S_define_name to a string.
   exco_name_str = string( S_define_name );

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the ExCO object.
   this->FOM_name = trick_MM->mm_strdup( "SimulationConfiguration" );
   this->name     = trick_MM->mm_strdup( "SimConfig" );
   this->packing  = this;
   // Allocate the attributes for the ExCO HLA object.
   this->attr_count = 4;
   this->attributes = static_cast< Attribute * >( TMM_declare_var_1d( "TrickHLA::Attribute", this->attr_count ) );

   //
   // Specify the ExCO attributes.
   //
   this->attributes[0].FOM_name     = trick_MM->mm_strdup( "owner" );
   trick_name_str                   = exco_name_str + string( ".owner" );
   this->attributes[0].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[0].rti_encoding = ENCODING_UNICODE_STRING;

   this->attributes[1].FOM_name     = trick_MM->mm_strdup( "run_duration" );
   trick_name_str                   = exco_name_str + string( ".run_duration_base_time" );
   this->attributes[1].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[1].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[2].FOM_name     = trick_MM->mm_strdup( "number_of_federates" );
   trick_name_str                   = exco_name_str + string( ".num_federates" );
   this->attributes[2].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[2].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[3].FOM_name     = trick_MM->mm_strdup( "required_federates" );
   trick_name_str                   = exco_name_str + string( ".required_federates" );
   this->attributes[3].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[3].rti_encoding = ENCODING_UNICODE_STRING;
}

/*!
 * @job_class{initialization}
 * NOTE: The initialize( manager ) function must be called before this function.
 */
void ExecutionConfiguration::configure()
{
   // Check the manager.
   if ( this->manager == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionConfiguration::configure():" << __LINE__
             << " ERROR: Null TrickHLA::Manager passed in!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Release the memory used by the required_federates c-string.
   if ( required_federates != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( required_federates ) ) ) {
         message_publish( MSG_WARNING, "ExecutionConfiguration::configure():%d WARNING failed to delete Trick Memory for 'required_federates'\n",
                          __LINE__ );
      }
      required_federates = NULL;
   }

   ostringstream federate_list;
   int           required_federate_count = 0;

   Federate *federate = manager->get_federate();
   if ( federate == NULL ) {
      ostringstream errmsg;
      errmsg << "TrickHLA::ExecutionConfiguration::configure():" << __LINE__
             << " ERROR: Null TrickHLA-Federate pointer!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Build a comma separated list of required federate names.
   for ( int i = 0; i < federate->known_feds_count; ++i ) {
      if ( federate->known_feds[i].required ) {
         if ( required_federate_count > 0 ) {
            federate_list << ",";
         }
         federate_list << federate->known_feds[i].name;
         ++required_federate_count;
      }
   }

   // Set the number of required federates.
   this->num_federates = required_federate_count;

   // Make sure we use correct function so that it is Trick managed memory.
   this->required_federates = trick_MM->mm_strdup( const_cast< char * >( federate_list.str().c_str() ) );
}

/*!
@details This function is called before the data is sent to the RTI.
*/
void ExecutionConfiguration::pack()
{
   ostringstream msg;
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      msg << "===================================================\n";
   }

   double terminate_time = exec_get_terminate_time();

   // Set the stop/termination time of the Trick simulation based on the
   // run_duration setting.
   if ( terminate_time >= 1.0e20 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
         msg << "TrickHLA::ExecutionConfiguration::pack():" << __LINE__
             << " Setting simulation termination time to "
             << run_duration << " seconds.\n";
      }
      exec_set_terminate_time( this->run_duration );
   } else {
      // Set the run_duration based on the Trick simulation termination time
      // and the current granted HLA time.
      this->run_duration = terminate_time - object->get_granted_time().get_time_in_seconds();
      if ( run_duration < 0.0 ) {
         run_duration = 0.0;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
         msg << "TrickHLA::ExecutionConfiguration::pack():" << __LINE__
             << " Setting simulation duration to "
             << run_duration << " seconds.\n";
      }
   }

   // Encode the run duration into a 64 bit integer base time.
   this->run_duration_base_time = Int64BaseTime::to_base_time( this->run_duration );

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      msg << "TrickHLA::ExecutionConfiguration::pack():" << __LINE__ << '\n'
          << "\tObject-Name:'" << object->get_name() << "'\n"
          << "\towner:'" << ( owner != NULL ? owner : "" ) << "'\n"
          << "\trun_duration:" << run_duration << " seconds\n"
          << "\trun_duration_base_time:" << run_duration_base_time << " " << Int64BaseTime::get_units() << '\n'
          << "\tnum_federates:" << num_federates << '\n'
          << "\trequired_federates:'" << ( required_federates != NULL ? required_federates : "" ) << "'\n"
          << "===================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*!
@details This function is called after data is received from the RTI.
*/
void ExecutionConfiguration::unpack()
{
   ostringstream msg;
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      msg << "===================================================\n";
   }

   // Decode the run duration from a 64 bit integer in bae time to seconds.
   this->run_duration = Int64BaseTime::to_seconds( this->run_duration_base_time );

   // Set the stop/termination time of the Trick simulation based on the
   // run_duration setting.
   if ( run_duration >= 0.0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
         msg << "TrickHLA::ExecutionConfiguration::unpack():" << __LINE__
             << " Setting simulation duration to "
             << run_duration << " seconds.\n";
      }
      exec_set_terminate_time( this->run_duration );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      msg << "TrickHLA::ExecutionConfiguration::unpack():" << __LINE__ << '\n'
          << "\tObject-Name:'" << object->get_name() << "'\n"
          << "\towner:'" << ( owner != NULL ? owner : "" ) << "'\n"
          << "\trun_duration:" << run_duration << " seconds\n"
          << "\run_duration_base_time:" << run_duration_base_time << " " << Int64BaseTime::get_units() << '\n'
          << "\tnum_federates:" << num_federates << '\n'
          << "\trequired_federates:'" << ( required_federates != NULL ? required_federates : "" ) << "'\n"
          << "===================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Mark that we have an ExCO update with pending changes.
   this->pending_update = true;
}

/*!
 * @details WARNING: This function is BROKEN!
 */
void ExecutionConfiguration::setup_ref_attributes(
   Packing *packing_obj )
{
   return;
}

void ExecutionConfiguration::print_execution_configuration() const
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << '\n'
          << "=============================================================\n"
          << "TrickHLA::ExecutionConfiguration::print_exec_config():" << __LINE__ << '\n'
          << "\t Object-Name:           '" << get_name() << "'\n"
          << "\t run_duration:          " << setprecision( 18 ) << run_duration << " seconds\n"
          << "\t run_duration_base_time:" << setprecision( 18 ) << run_duration_base_time << " " << Int64BaseTime::get_units() << '\n'
          << "\t num_federates:         " << setprecision( 18 ) << num_federates << '\n'
          << "\t required_federates:    '" << required_federates << "'\n"
          << "\t owner:                 '" << owner << "'\n"
          << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}
