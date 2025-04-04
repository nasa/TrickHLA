/*!
@file IMSim/ExecutionConfiguration.cpp
@ingroup IMSim
@brief Implementation of the TrickHLA IMSim Simulation Configuration Object.

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
@trick_link_dependency{../TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../TrickHLA/Int64BaseTime.cpp}
@trick_link_dependency{../TrickHLA/Object.cpp}
@trick_link_dependency{../TrickHLA/Packing.cpp}
@trick_link_dependency{../TrickHLA/Federate.cpp}
@trick_link_dependency{../TrickHLA/Manager.cpp}
@trick_link_dependency{../TrickHLA/SleepTimeout.cpp}
@trick_link_dependency{../TrickHLA/ExecutionConfigurationBase.cpp}
@trick_link_dependency{ExecutionConfiguration.cpp}
@trick_link_dependency{ExecutionControl.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, IMSim, June 2007, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, IMSim support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, January 2024, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdint>
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
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

// IMSim include files.
#include "IMSim/ExecutionConfiguration.hh"
#include "IMSim/ExecutionControl.hh"

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
using namespace IMSim;

#ifdef __cplusplus
extern "C" {
#endif
// Include the Trick generated ATTRIBUTES for the ExecutionConfiguration class.
// This is used to set up Trick based simulation variable mapping into the
// TrickHLA::Object associated with this class.
extern ATTRIBUTES attrIMSim__ExecutionConfiguration[];
#ifdef __cplusplus
}
#endif

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration::ExecutionConfiguration()
   : TrickHLA::ExecutionConfigurationBase(),
     owner( NULL ),
     scenario( NULL ),
     mode( NULL ),
     run_duration( -1 ),
     number_of_federates( 0 ),
     required_federates( NULL ),
     start_year( 2024 ),
     start_seconds( 1517932 ),
     DUT1( 0.0 ),
     deltaAT( 37 )
{
   // Set default empty strings.
   this->owner = trick_MM->mm_strdup( const_cast< char * >( "" ) );

   this->scenario = trick_MM->mm_strdup( const_cast< char * >( "" ) );

   this->mode = trick_MM->mm_strdup( const_cast< char * >( "" ) );

   this->required_federates = trick_MM->mm_strdup( const_cast< char * >( "" ) );

   // Note that the default start time is 18 January 2024, 13:38;52 UTC.

   // This is both a TrickHLA::Object and Packing.
   // So, it can safely reference itself.
   this->packing = this;
}

/*!
 * @job_class{initialization}
 */
ExecutionConfiguration::ExecutionConfiguration(
   char const *s_define_name )
   : TrickHLA::ExecutionConfigurationBase( s_define_name ),
     owner( NULL ),
     scenario( NULL ),
     mode( NULL ),
     run_duration( -1 ),
     number_of_federates( 0 ),
     required_federates( NULL ),
     start_year( 2024 ),
     start_seconds( 1517932 ),
     DUT1( 0.0 ),
     deltaAT( 37 )
{
   // Set default empty strings.
   this->owner = trick_MM->mm_strdup( const_cast< char * >( "" ) );

   this->scenario = trick_MM->mm_strdup( const_cast< char * >( "" ) );

   this->mode = trick_MM->mm_strdup( const_cast< char * >( "" ) );

   this->required_federates = trick_MM->mm_strdup( const_cast< char * >( "" ) );

   // Note that the default start time is 18 January 2024, 13:38;52 UTC.

   // This is both a TrickHLA::Object and Packing.
   // So, it can safely reference itself.
   this->packing = this;
}

/*!
 * @details Even though this is a pure virtual destructor, we provide a
 * default implementation that can be called from an inheriting class.
 * @job_class{shutdown}
 */
ExecutionConfiguration::~ExecutionConfiguration() // RETURN: -- None.
{
   // Free the allocated strings.
   if ( this->owner != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->owner ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionConfiguration::~ExecutionConfiguration():%d WARNING failed to delete Trick Memory for 'this->owner'\n",
                          __LINE__ );
      }
      this->owner = NULL;
   }
   if ( this->scenario != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->scenario ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionConfiguration::~ExecutionConfiguration():%d WARNING failed to delete Trick Memory for 'this->scenario'\n",
                          __LINE__ );
      }
      this->scenario = NULL;
   }
   if ( this->mode != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->mode ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionConfiguration::~ExecutionConfiguration():%d WARNING failed to delete Trick Memory for 'this->mode'\n",
                          __LINE__ );
      }
      this->mode = NULL;
   }
   if ( this->required_federates != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->required_federates ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionConfiguration::~ExecutionConfiguration():%d WARNING failed to delete Trick Memory for 'this->required_federates'\n",
                          __LINE__ );
      }
      this->owner = required_federates;
   }
   this->execution_control = NULL;
}

/*!
 * @details These can be overridden in the input.py file.
 * @job_class{default_data}
 */
void ExecutionConfiguration::configure_attributes(
   char const *sim_config_name )
{
   // Check to make sure we have a reference to the TrickHLA::FedAmb.
   if ( sim_config_name == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionConfiguration::configure_attributes():" << __LINE__
             << " ERROR: Unexpected NULL S_define_name.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Set the S_define instance name.
   if ( this->S_define_name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( const_cast< char * >( this->S_define_name ) ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionConfiguration::configure_attributes():%d WARNING failed to delete Trick Memory for 'this->S_define_name'\n",
                          __LINE__ );
      }
      this->S_define_name = trick_MM->mm_strdup( sim_config_name );
   }

   // Now call the default configure_attributes function.
   configure_attributes();

   return;
}

/*!
 * @details These can be overridden in the input.py file.
 * @job_class{default_data}
 */
void ExecutionConfiguration::configure_attributes()
{
   // Check to make sure we have an S_define name for this simulation configuration instance.
   if ( S_define_name == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionConfiguration::configure_attributes():" << __LINE__
             << " ERROR: Unexpected NULL S_define_name.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   string simconfig_name_str = string( S_define_name );
   string trick_name_str;

   //---------------------------------------------------------
   // Set up the execution configuration HLA object mappings.
   //---------------------------------------------------------
   // Set the FOM name of the SimulationConfiguration object.
   this->FOM_name = trick_MM->mm_strdup( "SimulationConfiguration" );
   this->name     = trick_MM->mm_strdup( "sim_config_v2" );
   // this->create_HLA_instance = is_master;
   this->packing = this;
   // Allocate the attributes for the SimulationConfiguration HLA object.
   this->attr_count = 10;
   this->attributes = static_cast< Attribute * >( TMM_declare_var_1d( "TrickHLA::Attribute", this->attr_count ) );

   //
   // Specify the SimulationConfiguration attributes.
   //
   // Setup the "root_frame_name" attribute.
   this->attributes[0].FOM_name     = trick_MM->mm_strdup( "owner" );
   trick_name_str                   = simconfig_name_str + string( ".owner" );
   this->attributes[0].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[0].rti_encoding = ENCODING_UNICODE_STRING;

   this->attributes[1].FOM_name     = trick_MM->mm_strdup( "scenario" );
   trick_name_str                   = simconfig_name_str + string( ".scenario" );
   this->attributes[1].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[1].rti_encoding = ENCODING_UNICODE_STRING;

   this->attributes[2].FOM_name     = trick_MM->mm_strdup( "mode" );
   trick_name_str                   = simconfig_name_str + string( ".mode" );
   this->attributes[2].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[2].rti_encoding = ENCODING_UNICODE_STRING;

   this->attributes[3].FOM_name     = trick_MM->mm_strdup( "run_duration" );
   trick_name_str                   = simconfig_name_str + string( ".run_duration" );
   this->attributes[3].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[3].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[4].FOM_name     = trick_MM->mm_strdup( "number_of_federates" );
   trick_name_str                   = simconfig_name_str + string( ".number_of_federates" );
   this->attributes[4].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[4].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[5].FOM_name     = trick_MM->mm_strdup( "required_federates" );
   trick_name_str                   = simconfig_name_str + string( ".required_federates" );
   this->attributes[5].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[5].rti_encoding = ENCODING_UNICODE_STRING;

   this->attributes[6].FOM_name     = trick_MM->mm_strdup( "start_year" );
   trick_name_str                   = simconfig_name_str + string( ".start_year" );
   this->attributes[6].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[6].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[7].FOM_name     = trick_MM->mm_strdup( "start_seconds" );
   trick_name_str                   = simconfig_name_str + string( ".start_seconds" );
   this->attributes[7].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[7].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[8].FOM_name     = trick_MM->mm_strdup( "DUT1" );
   trick_name_str                   = simconfig_name_str + string( ".DUT1" );
   this->attributes[8].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[8].rti_encoding = ENCODING_LITTLE_ENDIAN;

   this->attributes[9].FOM_name     = trick_MM->mm_strdup( "deltaAT" );
   trick_name_str                   = simconfig_name_str + string( ".deltaAT" );
   this->attributes[9].trick_name   = trick_MM->mm_strdup( trick_name_str.c_str() );
   this->attributes[9].rti_encoding = ENCODING_LITTLE_ENDIAN;

   return;
}

/*!
 * @job_class{initialization}
 */
void ExecutionConfiguration::configure()
{
   // Check to make sure we have a reference to the TrickHLA::FedAmb.
   if ( manager == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionConfiguration::configure():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA::Manager.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // The Simulation Configuration object must have a name.
   if ( name == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionConfiguration::configure():" << __LINE__
             << " ERROR: Simulation configuration must have a name!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Lag compensation is not supported for the Execution Configuration object.
   set_lag_compensation_type( LAG_COMPENSATION_NONE );

   // Ownership transfer will not be used for the Execution Configuration object.
   ownership = NULL;

   // Make sure the ExecutionConfiguration attributes go out in
   // Receive-Order so that a late joining federate can get them.
   reset_preferred_order();

   return;
}

/*!
 * @job_class{scheduled}
 */
void ExecutionConfiguration::set_imsim_control( IMSim::ExecutionControl *exec_control )
{
   execution_control = static_cast< TrickHLA::ExecutionControlBase * >( exec_control );
   return;
}

/*!
 * @job_class{scheduled}
 */
IMSim::ExecutionControl *ExecutionConfiguration::get_imsim_control()
{
   IMSim::ExecutionControl *imsim_exec_cntrl;

   imsim_exec_cntrl = dynamic_cast< IMSim::ExecutionControl * >( execution_control );
   if ( imsim_exec_cntrl == NULL ) {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionConfiguration::get_imsim_control():" << __LINE__
             << " ERROR: Dynamic cast error from base class reference to IMSim reference!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return imsim_exec_cntrl;
}

/*!
@details This function is called before the data is sent to the RTI.
*/
void ExecutionConfiguration::pack()
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "=============================================================\n"
          << "IMSim::ExecutionConfiguration::pack():" << __LINE__ << '\n'
          << "\t Current Scenario Time:   " << setprecision( 18 ) << execution_control->scenario_timeline->get_time() << '\n'
          << "\t Current Simulation Time: " << the_exec->get_sim_time() << '\n'
          << "\t Current HLA grant time:  " << get_federate()->get_granted_time().get_time_in_seconds() << '\n'
          << "\t Current HLA request time:" << get_federate()->get_requested_time().get_time_in_seconds() << '\n'
          << ".............................................................\n";
      print_simconfig( msg );
      msg << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*!
@details This function is called after data is received from the RTI.
*/
void ExecutionConfiguration::unpack()
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      ostringstream msg;
      msg << "=============================================================\n"
          << "IMSim::ExecutionConfiguration::unpack():" << __LINE__ << '\n'
          << "\t Current Scenario Time:   " << setprecision( 18 ) << execution_control->scenario_timeline->get_time() << '\n'
          << "\t Current Simulation Time: " << the_exec->get_sim_time() << '\n'
          << "\t Current HLA grant time:  " << get_federate()->get_granted_time().get_time_in_seconds() << '\n'
          << "\t Current HLA request time:" << get_federate()->get_requested_time().get_time_in_seconds() << '\n'
          << ".............................................................\n";
      print_simconfig( msg );
      msg << "=============================================================\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Mark that we have a Simulation Configuration update with pending changes.
   this->pending_update = true;
}

void ExecutionConfiguration::set_owner(
   char const *owner_name )
{
   // Free the Trick memory if it's already allocated.
   if ( this->owner != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->owner ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionConfiguration::set_owner():%d WARNING failed to delete Trick Memory for 'this->owner'\n",
                          __LINE__ );
      }
      this->owner = NULL;
   }

   // Allocate and duplicate the new root reference frame name.
   this->owner = trick_MM->mm_strdup( const_cast< char * >( owner_name ) );
}

void ExecutionConfiguration::set_scenario(
   char const *scenario_id )
{
   // Free the Trick memory if it's already allocated.
   if ( this->scenario != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->scenario ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionConfiguration::set_scenario():%d WARNING failed to delete Trick Memory for 'this->scenario'\n",
                          __LINE__ );
      }
      this->scenario = NULL;
   }

   // Allocate and duplicate the new root reference frame name.
   this->scenario = trick_MM->mm_strdup( const_cast< char * >( scenario_id ) );
}

void ExecutionConfiguration::set_mode(
   char const *mode_id )
{
   // Free the Trick memory if it's already allocated.
   if ( this->mode != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->mode ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionConfiguration::set_mode():%d WARNING failed to delete Trick Memory for 'this->mode'\n",
                          __LINE__ );
      }
      this->mode = NULL;
   }

   // Allocate and duplicate the new root reference frame name.
   this->mode = trick_MM->mm_strdup( const_cast< char * >( mode_id ) );
}

void ExecutionConfiguration::set_required_federates(
   char const *federates )
{
   // Free the Trick memory if it's already allocated.
   if ( this->required_federates != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( this->required_federates ) ) ) {
         message_publish( MSG_WARNING, "IMSim::ExecutionConfiguration::set_required_federates():%d WARNING failed to delete Trick Memory for 'this->required_federates'\n",
                          __LINE__ );
      }
      this->required_federates = NULL;
   }

   // Allocate and duplicate the new root reference frame name.
   this->required_federates = trick_MM->mm_strdup( const_cast< char * >( federates ) );
}

/*!
 * @details WARNING: This function is BROKEN!
 */
void ExecutionConfiguration::setup_ref_attributes(
   TrickHLA::Packing *packing_obj )
{
   ostringstream errormsg;
   errormsg << "IMSim::ExecutionConfiguration::setup_ref_attributes():" << __LINE__
            << " ERROR: This routine does NOT work and should not be called!\n";
   DebugHandler::terminate_with_message( errormsg.str() );
   return;
}

void ExecutionConfiguration::print_execution_configuration() const
{
   ostringstream msg;
   msg << "=============================================================\n"
       << "IMSim::ExecutionConfiguration::print_execution_configuration():" << __LINE__ << '\n';
   print_simconfig( msg );
   msg << "=============================================================\n";
   message_publish( MSG_NORMAL, msg.str().c_str() );
}

void ExecutionConfiguration::print_simconfig( std::ostream &stream ) const
{
   stream << "\t Object-Name:         '" << get_name() << "'\n"
          << "\t owner:               '" << owner << '\n'
          << "\t scenario:            " << scenario << '\n'
          << "\t mode:                " << mode << '\n'
          << "\t run duration:        " << run_duration << '\n'
          << "\t number of federates: " << number_of_federates << '\n'
          << "\t required federates:  " << required_federates << '\n'
          << "\t start year:          '" << start_year << '\n'
          << "\t start seconds:       " << start_seconds << " (s)\n"
          << "\t DUT1:                " << DUT1 << " (s)\n"
          << "\t delta AT:            " << deltaAT << " (s)\n";
   return;
}

bool ExecutionConfiguration::wait_for_update() // RETURN: -- None.
{
   Federate *federate = get_federate();

   // We can only receive the exec-configuration if we are not the master.
   if ( execution_control->is_master() ) {
      return false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
      message_publish( MSG_NORMAL, "IMSim::ExecutionConfiguration::wait_for_update():%d Waiting...\n",
                       __LINE__ );
   }

   // Make sure we have at least one piece of exec-config data we can receive.
   if ( any_remotely_owned_subscribed_init_attribute() ) {

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Wait for the data to arrive.
      while ( !is_changed() ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !is_changed() ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "IMSim::ExecutionConfiguration::wait_for_update():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "IMSim::ExecutionConfiguration::wait_for_update():%d Waiting...\n",
                                __LINE__ );
            }
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_EXECUTION_CONFIG ) ) {
         message_publish( MSG_NORMAL, "IMSim::ExecutionConfiguration::wait_for_update():%d Received data.\n",
                          __LINE__ );
      }

      // Receive the exec-config data from the master federate.
      receive_init_data();

   } else {
      ostringstream errmsg;
      errmsg << "IMSim::ExecutionConfiguration::wait_for_update():" << __LINE__
             << " ERROR: Execution-Configuration"
             << " is not configured to receive at least one object attribute."
             << " Make sure at least one 'exec_config' attribute has"
             << " 'subscribe = true' set. Please check your input or modified-data"
             << " files to make sure the 'subscribe' value is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return true;
}
