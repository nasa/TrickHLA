/*!
@file TrickHLA/Federate.cpp
@ingroup TrickHLA
@brief This class is the abstract base class for representing timelines.

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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{FedAmb.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{TrickThreadCoordinator.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, Titan Systems Corp., DIS, Titan Systems Corp., --, Initial investigation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support & test.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <arpa/inet.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib> // for atof
#include <float.h>
#include <fstream> // for ifstream
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <time.h>

// Trick include files.
#include "trick/CheckPointRestart.hh"
#include "trick/CheckPointRestart_c_intf.hh"
#include "trick/Clock.hh"
#include "trick/DataRecordDispatcher.hh" //DANNY2.7 need the_drd to init data recording groups when restoring at init time (IMSIM)
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/command_line_protos.h"
#include "trick/exec_proto.h"
#include "trick/input_processor_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/release.h"

// TrickHLA include files.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/FedAmb.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/TrickThreadCoordinator.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

// Access the Trick global objects for CheckPoint restart and the Clock.
extern Trick::CheckPointRestart *the_cpr;

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include <RTI/encoding/BasicDataElements.h>
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details NOTE: In most cases, we would allocate and set default names in
 * the constructor. However, since we want this class to be Input Processor
 * friendly, we cannot do that here since the Input Processor may not have
 * been initialized yet. So, we have to set the name information to NULL and
 * then allocate and set the defaults in the initialization job if not
 * already set in the input stream.
 *
 * @job_class{initialization}
 */
Federate::Federate()
   : name( NULL ),
     type( NULL ),
     federation_name( NULL ),
     local_settings( NULL ),
     FOM_modules( NULL ),
     MIM_module( NULL ),
     lookahead_time( 0.0 ),
     time_regulating( true ),
     time_constrained( true ),
     time_management( true ),
     enable_known_feds( true ),
     known_feds_count( 0 ),
     known_feds( NULL ),
     debug_level( DEBUG_LEVEL_NO_TRACE ),
     code_section( DEBUG_SOURCE_ALL_MODULES ),
     wait_status_time( 30.0 ),
     can_rejoin_federation( false ),
     freeze_delay_frames( 2 ),
     unfreeze_after_save( false ),
     federation_created_by_federate( false ),
     federation_exists( false ),
     federation_joined( false ),
     all_federates_joined( false ),
     lookahead( 0.0 ),
     HLA_cycle_time( 0.0 ),
     HLA_cycle_time_in_base_time( 0LL ),
     shutdown_called( false ),
     HLA_save_directory( "" ),
     initiate_save_flag( false ),
     restore_process( No_Restore ),
     prev_restore_process( No_Restore ),
     initiate_restore_flag( false ),
     restore_in_progress( false ),
     restore_failed( false ),
     restore_is_imminent( false ),
     save_label( "" ),
     announce_save( false ),
     save_label_generated( false ),
     save_request_complete( false ),
     save_completed( false ),
     stale_data_counter( 0 ),
     restore_label( "" ),
     announce_restore( false ),
     restore_label_generated( false ),
     restore_begun( false ),
     restore_request_complete( false ),
     restore_completed( false ),
     federation_restore_failed_callback_complete( false ),
     federate_has_been_restarted( false ),
     publish_data( true ),
     running_feds_count( 0 ),
     running_feds( NULL ),
     running_feds_count_at_time_of_restore( 0 ),
     checkpoint_file_name( "" ),
     checkpoint_rt_itimer( Off ),
     execution_has_begun( false ),
     time_adv_state( TIME_ADVANCE_RESET ),
     time_adv_state_mutex(),
     granted_time( 0.0 ),
     requested_time( 0.0 ),
     HLA_time( 0.0 ),
     start_to_save( false ),
     start_to_restore( false ),
     restart_flag( false ),
     restart_cfg_flag( false ),
     time_regulating_state( false ),
     time_constrained_state( false ),
     got_startup_sync_point( false ),
     make_copy_of_run_directory( false ),
     MOM_HLAfederation_class_handle(),
     MOM_HLAfederatesInFederation_handle(),
     MOM_HLAautoProvide_handle(),
     MOM_HLAfederation_instance_name_map(),
     auto_provide_setting( -1 ),
     orig_auto_provide_setting( -1 ),
     MOM_HLAfederate_class_handle(),
     MOM_HLAfederateType_handle(),
     MOM_HLAfederateName_handle(),
     MOM_HLAfederate_handle(),
     MOM_HLAfederate_instance_name_map(),
     joined_federate_mutex(),
     joined_federate_name_map(),
     joined_federate_handles(),
     joined_federate_names(),
     thread_coordinator(),
     RTI_ambassador( NULL ),
     federate_ambassador( NULL ),
     manager( NULL ),
     execution_control( NULL )
{
   TRICKHLA_INIT_FPU_CONTROL_WORD;

   // As a sanity check validate the FPU code word.
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @details Free up the Trick allocated memory associated with the attributes
 * of this class.
 * @job_class{shutdown}
 */
Federate::~Federate()
{
   // Free the memory used for the federate name.
   if ( name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( name ) ) ) {
         message_publish( MSG_WARNING, "Federate::~Federate():%d WARNING failed to delete Trick Memory for 'name'\n",
                          __LINE__ );
      }
      name = NULL;
   }

   // Free the memory used for the federate type.
   if ( type != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( type ) ) ) {
         message_publish( MSG_WARNING, "Federate::~Federate():%d WARNING failed to delete Trick Memory for 'type'\n",
                          __LINE__ );
      }
      type = NULL;
   }

   // Free the memory used for local-settings
   if ( local_settings != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( local_settings ) ) ) {
         message_publish( MSG_WARNING, "Federate::~Federate():%d WARNING failed to delete Trick Memory for 'local_settings'\n",
                          __LINE__ );
      }
      local_settings = NULL;
   }

   // Free the memory used for the Federation Execution name.
   if ( federation_name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( federation_name ) ) ) {
         message_publish( MSG_WARNING, "Federate::~Federate():%d WARNING failed to delete Trick Memory for 'federation_name'\n",
                          __LINE__ );
      }
      federation_name = NULL;
   }

   // Free the memory used by the FOM module filenames.
   if ( FOM_modules != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( FOM_modules ) ) ) {
         message_publish( MSG_WARNING, "Federate::~Federate():%d WARNING failed to delete Trick Memory for 'FOM_modules'\n",
                          __LINE__ );
      }
      FOM_modules = NULL;
   }
   if ( MIM_module != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( MIM_module ) ) ) {
         message_publish( MSG_WARNING, "Federate::~Federate():%d WARNING failed to delete Trick Memory for 'MIM_module'\n",
                          __LINE__ );
      }
      MIM_module = NULL;
   }

   // Free the memory used by the array of known Federates for the Federation.
   if ( known_feds != NULL ) {
      for ( int i = 0; i < known_feds_count; ++i ) {
         if ( known_feds[i].MOM_instance_name != NULL ) {
            if ( trick_MM->delete_var( static_cast< void * >( known_feds[i].MOM_instance_name ) ) ) {
               message_publish( MSG_WARNING, "Federate::~Federate():%d WARNING failed to delete Trick Memory for 'known_feds[i].MOM_instance_name'\n",
                                __LINE__ );
            }
            known_feds[i].MOM_instance_name = NULL;
         }
         if ( known_feds[i].name != NULL ) {
            if ( trick_MM->delete_var( static_cast< void * >( known_feds[i].name ) ) ) {
               message_publish( MSG_WARNING, "Federate::~Federate():%d WARNING failed to delete Trick Memory for 'known_feds[i].name'\n",
                                __LINE__ );
            }
            known_feds[i].name = NULL;
         }
      }
      if ( trick_MM->delete_var( static_cast< void * >( known_feds ) ) ) {
         message_publish( MSG_WARNING, "Federate::~Federate():%d WARNING failed to delete Trick Memory for 'known_feds'\n",
                          __LINE__ );
      }
      known_feds       = NULL;
      known_feds_count = 0;
   }

   // Clear the joined federate name map.
   joined_federate_name_map.clear();

   // Clear the set of federate handles for the joined federates.
   joined_federate_handles.clear();

   // Clear the list of joined federate names.
   joined_federate_names.clear();

   // Free the memory used by the array of running Federates for the Federation.
   clear_running_feds();

   // Clear the MOM HLAfederation instance name map.
   MOM_HLAfederation_instance_name_map.clear();

   // Clear the list of discovered object federate names.
   MOM_HLAfederate_instance_name_map.clear();

   // Set the references to the ambassadors.
   federate_ambassador = NULL;

   // Make sure we destroy the mutex.
   time_adv_state_mutex.destroy();
   joined_federate_mutex.destroy();
}

/*!
 * @job_class{initialization}
 */
void Federate::print_version() const
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string rti_name;
      StringUtilities::to_string( rti_name, RTI1516_NAMESPACE::rtiName() );
      string rti_version;
      StringUtilities::to_string( rti_version, RTI1516_NAMESPACE::rtiVersion() );

      ostringstream msg;
      msg << "Federate::print_version()::" << __LINE__ << '\n'
          << "     TrickHLA-version:'" << Utilities::get_version() << "'\n"
          << "TrickHLA-release-date:'" << Utilities::get_release_date() << "'\n"
          << "             RTI-name:'" << rti_name << "'\n"
          << "          RTI-version:'" << rti_version << "'\n";
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*!
 * @details Check that the FPU Control Word matches the value at simulation
 *  startup. If not it will reset it back to the startup value. It will use
 *  the FPU Control Word value set by the Python Input Processor.
 */
void Federate::fix_FPU_control_word()
{
#if defined( FPU_CW_PROTECTION ) && ( defined( __i386__ ) || defined( __x86_64__ ) )
   // Get the current FPU control word value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Reset the FPU control word value at program startup to use the current
   // FPU control word value that has been set by the input processor when
   // Python changed it to use IEEE-754 double precision floating point numbers
   // with a 53-bit Mantissa.
   if ( _fpu_cw != __fpu_control ) {
      // Reset the original FPU Control Word to the current value set by Python.
      __fpu_control = _fpu_cw;
   }
#endif

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - The TrickHLA::FedAmb class is actually an abstract class. Therefore,
 * the actual object instance being passed in is an instantiable polymorphic
 * child of the RTI1516_NAMESPACE::FederateAmbassador class.
 *
 * - The TrickHLA::ExecutionControlBase class is actually an abstract class.
 * Therefore, the actual object instance being passed in is an instantiable
 * polymorphic child of the TrickHLA::ExecutionControlBase class.
 *
 * @job_class{default_data}
 */
void Federate::setup(
   FedAmb               &federate_amb,
   Manager              &federate_manager,
   ExecutionControlBase &federate_execution_control )
{
   // Set the Federate ambassador.
   this->federate_ambassador = &federate_amb;

   // Set the Federate manager.
   this->manager = &federate_manager;

   // Set the Federate execution control.
   this->execution_control = &federate_execution_control;

   // Setup the TrickHLA::FedAmb instance.
   federate_ambassador->setup( *this, *( this->manager ) );

   // Setup the TrickHLA::Manager instance.
   manager->setup( *this, *( this->execution_control ) );

   // Set up the TrickHLA::ExecutionControl instance.
   execution_control->setup( *this, *( this->manager ) );

   // Set up the TrickHLA::TrickThreadCoordinator instance.
   thread_coordinator.setup( *this, *( this->manager ) );
}

/*! @brief Initialization the debug settings, show the version and apply
 * the FPU control word fix. */
void Federate::initialize_debug()
{
   // Check and fix the FPU Control Word as a job that runs just after
   // the Input Processor runs.
   fix_FPU_control_word();

   // Verify the debug level is correct just in case the user specifies it in
   // the input.py file as an integer instead of using the ENUM values...
   if ( ( this->debug_level < DEBUG_LEVEL_NO_TRACE ) || ( this->debug_level > DEBUG_LEVEL_FULL_TRACE ) ) {
      message_publish( MSG_WARNING, "Federate::initialize():%d You specified an \
invalid debug level '%d' in the input.py file using an integer value instead of \
an ENUM. Please double check the value you specified in the input.py file against \
the documented ENUM values.\n",
                       __LINE__, (int)this->debug_level );
      if ( this->debug_level < DEBUG_LEVEL_NO_TRACE ) {
         this->debug_level = DEBUG_LEVEL_NO_TRACE;
         message_publish( MSG_WARNING, "Federate::initialize():%d No TrickHLA debug messages will be emitted.\n",
                          __LINE__ );
      } else {
         this->debug_level = DEBUG_LEVEL_FULL_TRACE;
         message_publish( MSG_WARNING, "Federate::initialize():%d All TrickHLA debug messages will be emitted.\n",
                          __LINE__ );
      }
   }

   // Set the debug level and code section in the global DebugHandler.
   DebugHandler::set( this->debug_level, this->code_section );

   // Print the current TrickHLA version string.
   print_version();

   // Refresh the HLA time constants since the base time units may have changed
   // from a setting in the input file.
   refresh_HLA_time_constants();
}

/*!
 * @brief Initialize the thread memory associated with the Trick child threads.
 */
void Federate::initialize_thread_state(
   double const main_thread_data_cycle_time )
{
   this->HLA_cycle_time              = main_thread_data_cycle_time;
   this->HLA_cycle_time_in_base_time = Int64BaseTime::to_base_time( this->HLA_cycle_time );

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::initialize_thread_state():%d Trick main thread (id:0, data_cycle:%.3f).\n",
                       __LINE__, this->HLA_cycle_time );
   }

   // Make sure the Trick thread coordinator is initialized. This will
   // also associate the Trick main thread. TrickHLA will maintain data
   // coherency for the HLA object instances specified in the input file
   // over the data cycle time specified.
   thread_coordinator.initialize( this->HLA_cycle_time );

   // Initialize the manager with the verified HLA cycle time.
   manager->initialize_HLA_cycle_time();
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - The TrickHLA::FedAmb class is actually an abstract class. Therefore,
 * the actual object instance being passed in is an instantiable polymorphic
 * child of the RTI1516_NAMESPACE::FederateAmbassador class.
 * @job_class{initialization}
 */
void Federate::initialize()
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Make sure the federate name has been specified.
   if ( ( name == NULL ) || ( *name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Federate::initialize():" << __LINE__
             << " ERROR: Unexpected NULL federate name.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // If a federate type is not specified make it the same as the federate name.
   if ( ( type == NULL ) || ( *type == '\0' ) ) {
      type = trick_MM->mm_strdup( name );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::initialize():%d Federate:\"%s\" Type:\"%s\"\n",
                       __LINE__, name, type );
   }

   // Determine if the Trick time Tic resolution can support the HLA base time.
   if ( exec_get_time_tic_value() < Int64BaseTime::get_base_time_multiplier() ) {
      ostringstream errmsg;
      errmsg << "Federate::initialize():" << __LINE__
             << " ERROR: The Trick time tic value (" << exec_get_time_tic_value()
             << ") cannot support the HLA base time resolution ("
             << Int64BaseTime::get_units()
             << ") corresponding to THLA.federate.set_HLA_base_time_unit("
             << Int64BaseTime::get_base_units()
             << "). Please update the Trick time tic value in your input.py file"
             << " (i.e. by calling 'trick.exec_set_time_tic_value()').\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check to make sure we have a reference to the TrickHLA::FedAmb.
   if ( federate_ambassador == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::initialize():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA::FedAmb.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Initialize the TrickHLA::FedAmb object instance.
   federate_ambassador->initialize();

   // Check to make sure we have a reference to the TrickHLA::Manager.
   if ( manager == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::initialize():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA::Manager.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Verify the user specified object and interaction arrays and counts.
   manager->verify_object_and_interaction_arrays();

   // Check to make sure we have a reference to the TrickHLA::ExecutionControlBase.
   if ( execution_control == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::initialize():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA::ExecutionControlBase.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Initialize the TrickHLA::ExecutionControl object instance.
   execution_control->initialize();

   // Finish doing the initialization.
   restart_initialization();

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{initialization}
 */
void Federate::restart_initialization()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::restart_initialization():%d \n",
                       __LINE__ );
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Update the lookahead time in our HLA time line.
   set_lookahead( lookahead_time );

   if ( federate_ambassador == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: NULL pointer to FederateAmbassador!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Verify the federate name.
   if ( ( name == NULL ) || ( *name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: NULL or zero length Federate Name.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // The lookahead time can not be negative.
   if ( lookahead_time < 0.0 ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: Invalid HLA lookahead time!"
             << " Lookahead time (" << lookahead_time << " seconds)"
             << " must be greater than or equal to zero and not negative. Make"
             << " sure 'lookahead_time' in your input.py or modified-data file is"
             << " not a negative number.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify the FOM-modules value.
   if ( ( FOM_modules == NULL ) || ( *FOM_modules == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: Invalid FOM-modules."
             << " Please check your input.py or modified-data files to make sure"
             << " 'FOM_modules' is correctly specified, where 'FOM_modules' is"
             << " a comma separated list of FOM-module filenames.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify the Federation execution name.
   if ( ( federation_name == NULL ) || ( *federation_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: Invalid Federate Execution Name."
             << " Please check your input.py or modified-data files to make sure"
             << " the 'federation_name' is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check if there are known Federate in the Federation.
   if ( enable_known_feds ) {

      // Only need to do anything if there are known federates.
      if ( ( known_feds_count <= 0 ) || ( known_feds == NULL ) ) {

         // Make sure the count reflects the state of the array.
         known_feds_count = 0;

         // If we are enabling known federates, then there probably should be some.
         ostringstream errmsg;
         errmsg << "Federate::restart_initialization():" << __LINE__
                << " ERROR: No Known Federates Specified for the Federation.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // Validate the name of each Federate known to be in the Federation.
      for ( int i = 0; i < known_feds_count; ++i ) {

         // A NULL or zero length Federate name is not allowed.
         if ( ( known_feds[i].name == NULL ) || ( *known_feds[i].name == '\0' ) ) {
            ostringstream errmsg;
            errmsg << "Federate::restart_initialization():" << __LINE__
                   << " ERROR: Invalid name of known Federate at array index: "
                   << i << '\n';
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      }
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @details This performs all the startup steps prior to any multi-phase
 * initialization process defined by the user. The multi-phase initialization
 * will be performed as initialization jobs between P_INIT and P_LAST
 * phased initialization jobs.
 *
 * @job_class{initialization}
 */
void Federate::pre_multiphase_initialization()
{
   // The P1 ("initialization") federate.initialize_thread_state( data_cycle_time );
   // job should be called before this one, but verify the HLA cycle time
   // again to catch the case where a user did not pick up the changes to
   // the THLABase.sm file.
   if ( !verify_time_constraints() ) {
      ostringstream errmsg;
      errmsg << "Federate::pre_multiphase_initialization():" << __LINE__
             << " ERROR: Time Constraints verification failed!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Perform the Execution Control specific pre-multi-phase initialization.
   execution_control->pre_multi_phase_init_processes();

   // Debug printout.
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::pre_multiphase_initialization():%d\n     Completed pre-multiphase initialization...\n",
                       __LINE__ );
   }

   // Check to make sure we have a reference to the TrickHLA::Manager.
   if ( manager == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::initialize():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA::Manager.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Initialize the TrickHLA::Manager object instance.
   manager->initialize();
}

/*!
 * @details This performs all the startup steps after any multi-phase
 * initialization process defined by the user.
 *
 * @job_class{initialization}
 */
void Federate::post_multiphase_initialization()
{
   // Perform the Execution Control specific post-multi-phase initialization.
   execution_control->post_multi_phase_init_processes();

   // Debug printout.
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::post_multiphase_initialization():%d\n     Simulation has started and is now running...\n",
                       __LINE__ );
   }

   // Mark the federate as having begun execution.
   set_federate_has_begun_execution();
}

/*!
 * @job_class{initialization}
 */
void Federate::create_RTI_ambassador_and_connect()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Just return if we have already created the RTI ambassador.
   if ( RTI_ambassador.get() != NULL ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      return;
   }

   // To work around an issue caused by the Java VM throwing a Signal Floating
   // Point Exception from the garbage collector. We disable the SIGFPE set by
   // Trick, create the RTI-Ambassador, and then enable the SIGFPE again. This
   // will allow the JVM to start up its threads without the SIGFPE set. See
   // Pitch RTI bug case #9704.
   // TODO: Is this still necessary?
   bool trick_sigfpe_is_set = ( exec_get_trap_sigfpe() > 0 );
   if ( trick_sigfpe_is_set ) {
      exec_set_trap_sigfpe( false );
   }

   // For HLA-Evolved, the user can set a vendor specific local settings for
   // the connect() API.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      if ( ( local_settings == NULL ) || ( *local_settings == '\0' ) ) {
         ostringstream msg;
         msg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " WARNING: Local settings designator 'THLA.federate.local_settings'"
             << " for the RTI-Ambassador connection was not specified in the"
             << " input.py file, using HLA-Evolved vendor defaults.\n";
         message_publish( MSG_NORMAL, msg.str().c_str() );
      } else {
         ostringstream msg;
         msg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " Local settings designator for RTI-Ambassador connection:\n'"
             << local_settings << "'\n";
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }

   try {
      // Create the RTI ambassador factory.
      RTIambassadorFactory *rtiAmbassadorFactory = new RTIambassadorFactory();

      // Create the RTI ambassador.
      this->RTI_ambassador = rtiAmbassadorFactory->createRTIambassador();

      if ( ( local_settings == NULL ) || ( *local_settings == '\0' ) ) {
         // Use default vendor local settings.
         RTI_ambassador->connect( *federate_ambassador,
                                  RTI1516_NAMESPACE::HLA_IMMEDIATE );
      } else {
         wstring local_settings_ws;
         StringUtilities::to_wstring( local_settings_ws, local_settings );

         RTI_ambassador->connect( *federate_ambassador,
                                  RTI1516_NAMESPACE::HLA_IMMEDIATE,
                                  local_settings_ws );
      }

      // Reset the Federate shutdown-called flag now that we are connected.
      this->shutdown_called = false;

      // Make sure we delete the factory now that we are done with it.
      delete rtiAmbassadorFactory;

   } catch ( ConnectionFailed const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " ERROR: For Federate: '" << name
             << "' of Federation: '" << federation_name
             << "' with local_settings: '" << ( ( local_settings != NULL ) ? local_settings : "" )
             << "' with EXCEPTION: ConnectionFailed: '" << rti_err_msg << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( InvalidLocalSettingsDesignator const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " ERROR: For Federate: '" << name
             << "' of Federation: '" << federation_name
             << "' with local_settings: '" << ( ( local_settings != NULL ) ? local_settings : "" )
             << "' with EXCEPTION: InvalidLocalSettingsDesignator: '"
             << rti_err_msg << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( UnsupportedCallbackModel const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " For Federate: '" << name
             << "' of Federation: '" << federation_name
             << "' with local_settings: '" << ( ( local_settings != NULL ) ? local_settings : "" )
             << "' with EXCEPTION: UnsupportedCallbackModel: '"
             << rti_err_msg << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( AlreadyConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_RTI_ambassador_and_connect()"
             << " ERROR: For Federate: '" << name
             << "' of Federation: '" << federation_name
             << "' with local_settings: '" << ( ( local_settings != NULL ) ? local_settings : "" )
             << "' with EXCEPTION: AlreadyConnected: '"
             << rti_err_msg << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( CallNotAllowedFromWithinCallback const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " ERROR: For Federate: '" << name
             << "' of Federation: '" << federation_name
             << "' with local_settings: '" << ( ( local_settings != NULL ) ? local_settings : "" )
             << "' with EXCEPTION: CallNotAllowedFromWithinCallback: '"
             << rti_err_msg << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " ERROR: For Federate: '" << name
             << "' of Federation: '" << federation_name
             << "' with local_settings: '" << ( ( local_settings != NULL ) ? local_settings : "" )
             << "' with RTIinternalError: '" << rti_err_msg
             << "'. One possible"
             << " cause could be that the Central RTI Component is not running,"
             << " or is not running on the computer you think it is on. Please"
             << " check your CRC host and port settings and make sure the RTI"
             << " is running.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( trick_sigfpe_is_set ) {
      exec_set_trap_sigfpe( true );
   }
}

void Federate::add_federate_instance_id(
   ObjectInstanceHandle const &instance_hndl )
{
   joined_federate_name_map[instance_hndl] = L"";
}

void Federate::remove_federate_instance_id(
   ObjectInstanceHandle const &instance_hndl )
{
   TrickHLAObjInstanceNameMap::iterator iter;
   iter = joined_federate_name_map.find( instance_hndl );
   if ( iter != joined_federate_name_map.end() ) {
      joined_federate_name_map.erase( iter );

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, instance_hndl );

         ostringstream summary;
         summary << "Federate::remove_federate_instance_id():" << __LINE__
                 << " Object Instance:" << handle_str << '\n';
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }
   }
}

bool Federate::is_federate_instance_id(
   ObjectInstanceHandle const &id )
{
   return ( joined_federate_name_map.find( id ) != joined_federate_name_map.end() );
}

void Federate::set_MOM_HLAfederate_instance_attributes(
   ObjectInstanceHandle const    &id,
   AttributeHandleValueMap const &values )
{
   // Concurrency critical code section because joined-federate state used by
   // the blocking Federate::wait_for_required_federates_to_join() function.
   //
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &joined_federate_mutex );

   // Add the federate ID (i.e. federate handle) if we don't know about it already.
   if ( !is_federate_instance_id( id ) ) {
      add_federate_instance_id( id );
   }

   wstring federate_name_ws( L"" );

   // Find the Federate name for the given MOM federate Name attribute handle.
   AttributeHandleValueMap::const_iterator attr_iter = values.find( MOM_HLAfederateName_handle );

   // Determine if we have a federate name attribute.
   if ( attr_iter != values.end() ) {

      // Federate name is encoded into variable length data.
      VariableLengthData const &value = attr_iter->second;

      // Decode the federate name that is encoded as a Unicode string.
      HLAunicodeString fed_name_unicode;
      fed_name_unicode.decode( value );
      federate_name_ws = wstring( fed_name_unicode );

      // Map the federate name to the federate ID.
      joined_federate_name_map[id] = federate_name_ws;

      // Make sure that the federate name does not exist before adding.
      bool found = false;
      for ( int i = 0; !found && ( i < (int)joined_federate_names.size() ); ++i ) {
         if ( joined_federate_names[i] == federate_name_ws ) {
            found = true;
         }
      }
      if ( !found ) {
         // Record the federate name.
         joined_federate_names.push_back( federate_name_ws );
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string id_str;
         StringUtilities::to_string( id_str, id );
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d Federate OID:%s name:'%s' size:%d \n",
                          __LINE__, id_str.c_str(), federate_name_ws.c_str(),
                          (int)federate_name_ws.size() );
      }
   }

   // Find the FederateHandle attribute for the given MOM federate handle.
   attr_iter = values.find( MOM_HLAfederate_handle );

   // Determine if we have a federate handle attribute.
   if ( attr_iter != values.end() ) {

      // Do a sanity check on the overall encoded data size.
      if ( attr_iter->second.size() != 8 ) {
         ostringstream errmsg;
         errmsg << "Federate::set_MOM_HLAfederate_instance_attributes():"
                << __LINE__ << " ERROR: Unexpected number of bytes in the"
                << " Encoded FederateHandle because the byte count is "
                << attr_iter->second.size()
                << " but we expected 8!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         exit( 1 );
      }

      // The HLAfederateHandle has the HLAhandle datatype which has the
      // HLAvariableArray encoding with an HLAbyte element type.
      //  0 0 0 4 0 0 0 2
      //  ---+--- | | | |
      //     |    ---+---
      // #elem=4  fedID = 2
      //
      // First 4 bytes (first 32-bit integer) is the number of elements.
      // Decode size from Big Endian encoded integer.
      unsigned char const *data = static_cast< unsigned char const * >( attr_iter->second.data() );

      int size = Utilities::is_transmission_byteswap( ENCODING_BIG_ENDIAN )
                    ? Utilities::byteswap_int( *reinterpret_cast< int const * >( data ) )
                    : *reinterpret_cast< int const * >( data );
      if ( size != 4 ) {
         ostringstream errmsg;
         errmsg << "Federate::set_MOM_HLAfederate_instance_attributes():"
                << __LINE__ << " ERROR: FederateHandle size is "
                << size << " but expected it to be 4!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         exit( 1 );
      }

      // Point to the start of the federate handle ID in the encoded data.
      data += 4;

      VariableLengthData encoded_fed_handle;
      encoded_fed_handle.setData( data, size );

      FederateHandle fed_handle;

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      try {
         fed_handle = RTI_ambassador->decodeFederateHandle( encoded_fed_handle );
      } catch ( CouldNotDecode const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Federate::set_MOM_HLAfederate_instance_attributes():" << __LINE__
                << " ERROR: When decoding 'FederateHandle': EXCEPTION: CouldNotDecode\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         exit( 1 );
      } catch ( FederateNotExecutionMember const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Federate::set_MOM_HLAfederate_instance_attributes():" << __LINE__
                << " ERROR: When decoding 'FederateHandle': EXCEPTION: FederateNotExecutionMember\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         exit( 1 );
      } catch ( NotConnected const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Federate::set_MOM_HLAfederate_instance_attributes():" << __LINE__
                << " ERROR: When decoding 'FederateHandle': EXCEPTION: NotConnected\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         exit( 1 );
      } catch ( RTIinternalError const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         ostringstream errmsg;
         errmsg << "Federate::set_MOM_HLAfederate_instance_attributes():" << __LINE__
                << " ERROR: When decoding 'FederateHandle': EXCEPTION: "
                << "RTIinternalError: %s" << rti_err_msg << '\n';
         DebugHandler::terminate_with_message( errmsg.str() );
         exit( 1 );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      // Add this FederateHandle to the set of joined federates.
      joined_federate_handles.insert( fed_handle );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string id_str, fed_id;
         StringUtilities::to_string( id_str, id );
         StringUtilities::to_string( fed_id, fed_handle );
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d Federate-OID:%s num_bytes:%d Federate-ID:%s \n",
                          __LINE__, id_str.c_str(), size, fed_id.c_str() );
      }

      // If this federate is running, add the new entry into running_feds.
      if ( is_federate_executing() ) {
         bool found = false;
         for ( int loop = 0; loop < running_feds_count; ++loop ) {
            char const *tName = StringUtilities::ip_strdup_wstring( federate_name_ws );
            if ( !strcmp( running_feds[loop].name, tName ) ) {
               found = true;
               break;
            }
         }
         // Update the running_feds if the federate name was not found.
         if ( !found ) {
            if ( joined_federate_name_map.size() == 1 ) {
               add_a_single_entry_into_running_feds();

               // Clear the entry after it is absorbed into running_feds.
               joined_federate_name_map.clear();
            } else {
               // Loop thru all joined_federate_name_map entries removing stray
               // NULL string entries.
               TrickHLAObjInstanceNameMap::iterator map_iter;
               for ( map_iter = joined_federate_name_map.begin();
                     map_iter != joined_federate_name_map.end(); ++map_iter ) {
                  if ( map_iter->second.length() == 0 ) {
                     joined_federate_name_map.erase( map_iter );

                     // Re-process all entries if any are deleted since the
                     // delete modified the iterator position.
                     map_iter = joined_federate_name_map.begin();
                  }
               }

               // After the purge, if there is only one value, process the
               // single element.
               if ( joined_federate_name_map.size() == 1 ) {
                  add_a_single_entry_into_running_feds();

                  // Clear the entry after it is absorbed into running_feds.
                  joined_federate_name_map.clear();
               } else {
                  // Process multiple joined_federate_name_map entries.
                  clear_running_feds();
                  ++running_feds_count;
                  update_running_feds();

                  // Clear the entries after they are absorbed into running_feds.
                  joined_federate_name_map.clear();
               }
            }
         }
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string id_str;
         StringUtilities::to_string( id_str, id );
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d FederateHandle Not found for Federate OID:%s \n",
                          __LINE__, id_str.c_str() );
      }
   }
}

void Federate::set_all_federate_MOM_instance_handles_by_name()
{
   // Make sure the discovered federate instances list is cleared.
   joined_federate_name_map.clear();

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   wstring fed_mom_instance_name_ws = L"";

   ostringstream summary;
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      summary << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Resolve all the federate instance handles given the federate names.
   try {
      for ( int i = 0; i < known_feds_count; ++i ) {
         if ( known_feds[i].MOM_instance_name != NULL ) {

            // Create the wide-string version of the MOM instance name.
            StringUtilities::to_wstring( fed_mom_instance_name_ws,
                                         known_feds[i].MOM_instance_name );

            // Get the instance handle based on the instance name.
            ObjectInstanceHandle fed_mom_obj_instance_hdl =
               rti_amb->getObjectInstanceHandle( fed_mom_instance_name_ws );

            // Add the federate instance handle.
            add_federate_instance_id( fed_mom_obj_instance_hdl );

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
               string id_str;
               StringUtilities::to_string( id_str, fed_mom_obj_instance_hdl );
               summary << "\n    Federate:'" << known_feds[i].name
                       << "' MOM-Object-ID:" << id_str;
            }
         }
      }
   } catch ( ObjectInstanceNotKnown const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << '\n';
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }

      string fed_mom_instance_name;
      StringUtilities::to_string( fed_mom_instance_name, fed_mom_instance_name_ws );
      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: Object Instance Not Known for '"
             << fed_mom_instance_name << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << '\n';
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }

      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: Federation Not Execution Member\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << '\n';
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }

      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: NotConnected\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << '\n';
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " RTIinternalError: '" << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << '\n';
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: RTI1516_EXCEPTION for '" << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      summary << '\n';
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }
}

/*!
 *  @job_class{initialization}
 */
void Federate::determine_federate_MOM_object_instance_names()
{
   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::determine_federate_MOM_object_instance_names():" << __LINE__
             << " Unexpected NULL RTIambassador.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   wstring                                    fed_name_ws = L"";
   ObjectInstanceHandle                       fed_mom_instance_hdl;
   TrickHLAObjInstanceNameMap::const_iterator fed_iter;

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      for ( fed_iter = joined_federate_name_map.begin();
            fed_iter != joined_federate_name_map.end(); ++fed_iter ) {

         for ( int i = 0; i < known_feds_count; ++i ) {
            StringUtilities::to_wstring( fed_name_ws, known_feds[i].name );

            if ( fed_iter->second.compare( fed_name_ws ) == 0 ) {
               fed_mom_instance_hdl = fed_iter->first;

               // Get the instance name based on the MOM object instance handle
               // and make sure it is in the Trick memory space.
               known_feds[i].MOM_instance_name =
                  StringUtilities::ip_strdup_wstring(
                     rti_amb->getObjectInstanceName( fed_mom_instance_hdl ) );
            }
         }
      }
   } catch ( ObjectInstanceNotKnown const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: ObjectInstanceNotKnown\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: NotConnected\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      string id_str;
      StringUtilities::to_string( id_str, fed_mom_instance_hdl );
      string fed_name_str;
      StringUtilities::to_string( fed_name_str, fed_name_ws );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Object::register_object_with_RTI():" << __LINE__
             << " ERROR: Exception getting MOM instance name for '"
             << fed_name_str << "' ID:" << id_str
             << " '" << rti_err_msg << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

bool Federate::is_required_federate(
   wstring const &federate_name )
{
   for ( int i = 0; i < known_feds_count; ++i ) {
      if ( known_feds[i].required ) {
         wstring required_fed_name;
         StringUtilities::to_wstring( required_fed_name, known_feds[i].name );
         if ( federate_name == required_fed_name ) {
            return true;
         }
      }
   }
   return false;
}

bool Federate::is_joined_federate(
   char const *federate_name )
{
   wstring fed_name_ws;
   StringUtilities::to_wstring( fed_name_ws, federate_name );
   return is_joined_federate( fed_name_ws );
}

bool Federate::is_joined_federate(
   wstring const &federate_name )
{
   for ( int i = 0; i < (int)joined_federate_names.size(); ++i ) {
      if ( federate_name == joined_federate_names[i] ) {
         return true;
      }
   }
   return false;
}

/*!
 *  @job_class{initialization}
 */
string Federate::wait_for_required_federates_to_join()
{
   string status_string;

   // If the known Federates list is disabled then just return.
   if ( !enable_known_feds ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::wait_for_required_federates_to_join():%d Check for required Federates DISABLED.\n",
                          __LINE__ );
      }
      return status_string;
   }

   // Determine how many required federates we have.
   int required_feds_count = 0;
   for ( int i = 0; i < known_feds_count; ++i ) {
      if ( known_feds[i].required ) {
         ++required_feds_count;
      }
   }

   // If we don't have any required Federates then return.
   if ( required_feds_count == 0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::wait_for_required_federates_to_join():%d NO REQUIRED FEDERATES!!!\n",
                          __LINE__ );
      }
      return status_string;
   }

   // Create a summary of the required federates.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream required_fed_summary;
      required_fed_summary << "Federate::wait_for_required_federates_to_join():"
                           << __LINE__ << "\nWAITING FOR " << required_feds_count
                           << " REQUIRED FEDERATES:";

      // Display the initial summary of the required federates we are waiting for.
      int cnt = 0;
      for ( int i = 0; i < known_feds_count; ++i ) {
         // Create a summary of the required federates by name.
         if ( known_feds[i].required ) {
            ++cnt;
            required_fed_summary << "\n    " << cnt
                                 << ": Waiting for required federate '"
                                 << known_feds[i].name << "'";
         }
      }

      required_fed_summary << '\n';

      // Display a summary of the required federate by name.
      message_publish( MSG_NORMAL, required_fed_summary.str().c_str() );

      // Display a message that we are requesting the federate names.
      message_publish( MSG_NORMAL, "Federate::wait_for_required_federates_to_join():%d Requesting list of joined federates from CRC.\n",
                       __LINE__ );
   }

   // Subscribe to Federate names using MOM interface and request an update.
   ask_MOM_for_federate_names();

   int i, req_fed_cnt;
   int joined_fed_cnt = 0;

   bool          print_summary                = false;
   bool          found_an_unrequired_federate = false;
   set< string > unrequired_federates_list; // list of unique unrequired federate names

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   this->all_federates_joined = false;

   // Wait for all the required federates to join.
   while ( !this->all_federates_joined ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      // Sleep a little while to wait for more federates to join.
      sleep_timer.sleep();

      // Concurrency critical code section because joined-federate state is changed
      // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
      // function.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &joined_federate_mutex );

         // Determine what federates have joined only if the joined federate
         // count has changed.
         if ( joined_fed_cnt != (int)joined_federate_names.size() ) {
            joined_fed_cnt = joined_federate_names.size();

            // Count the number of joined Required federates.
            req_fed_cnt = 0;
            for ( i = 0; i < (int)joined_federate_names.size(); ++i ) {
               if ( is_required_federate( joined_federate_names[i] ) ) {
                  ++req_fed_cnt;
               } else {
                  found_an_unrequired_federate = true;
                  string fedname;
                  StringUtilities::to_string( fedname, joined_federate_names[i] );
                  if ( restore_is_imminent ) {
                     if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
                        message_publish( MSG_NORMAL, "Federate::wait_for_required_federates_to_join():%d Found an UNREQUIRED federate %s!\n",
                                         __LINE__, fedname.c_str() );
                     }
                     unrequired_federates_list.insert( fedname );
                  }
               }
            }

            // Determine if all the Required federates have joined.
            if ( req_fed_cnt >= required_feds_count ) {
               this->all_federates_joined = true;
            }

            // Determine if we should print a summary.
            print_summary = DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE );
         }

         // Print out a list of the Joined Federates.
         if ( print_summary ) {
            print_summary = false;

            // Build the federate summary as an output string stream.
            ostringstream summary;
            summary << "Federate::wait_for_required_federates_to_join():"
                    << __LINE__ << "\nWAITING FOR " << required_feds_count
                    << " REQUIRED FEDERATES:";

            // Summarize the required federates first.
            int cnt = 0;
            for ( i = 0; i < known_feds_count; ++i ) {
               ++cnt;
               if ( known_feds[i].required ) {
                  if ( is_joined_federate( known_feds[i].name ) ) {
                     summary << "\n    " << cnt
                             << ": Found joined required federate '"
                             << known_feds[i].name << "'";
                  } else {
                     summary << "\n    " << cnt
                             << ": Waiting for required federate '"
                             << known_feds[i].name << "'";
                  }
               }
            }

            // Summarize all the remaining non-required joined federates.
            for ( i = 0; i < (int)joined_federate_names.size(); ++i ) {
               if ( !is_required_federate( joined_federate_names[i] ) ) {
                  ++cnt;

                  // We need a string version of the wide-string federate name.
                  string fedname;
                  StringUtilities::to_string( fedname, joined_federate_names[i] );

                  summary << "\n    " << cnt << ": Found joined federate '"
                          << fedname << "'";
               }
            }
            summary << '\n';

            // Display the federate summary.
            message_publish( MSG_NORMAL, summary.str().c_str() );
         }
      } // Mutex protection goes out of scope here

      if ( !this->all_federates_joined ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_for_required_federates_to_join():" << __LINE__
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
            print_summary = true;
         }
      }
   }

   // Once a list of joined federates has been built, and we are to restore the
   // checkpoint if there are any non-required federates. If any are found,
   // terminate the simulation with a verbose message stating which federates
   // were joined as non-required, as well as the required federates, so the user
   // knows what happened and know how to properly restart the federation. We
   // do this to inform the user that they did something wrong and gracefully
   // terminate the execution instead of the federation failing to restore
   // and the user is left to scratch their heads why the federation failed
   // to restore!
   if ( restore_is_imminent && found_an_unrequired_federate ) {
      ostringstream errmsg;
      errmsg << "FATAL ERROR: You indicated a restore of a checkpoint set but "
             << "at least one federate which was NOT executing at the time of "
             << "the checkpoint is currently joined in the federation. This "
             << "violates IEEE Std 1516.2000, section 4.18 (Request Federation "
             << "Restore), precondition d), \"The correct number of joined "
             << "federates of the correct types that were joined to the "
             << "federation execution when the save was accomplished are "
             << "currently joined to the federation execution.\"\n\tThe "
             << "extraneous ";
      if ( unrequired_federates_list.size() == 1 ) {
         errmsg << "federate is: ";
      } else {
         errmsg << "federates are: ";
      }
      set< string >::const_iterator cii;
      string                        names;
      for ( cii = unrequired_federates_list.begin();
            cii != unrequired_federates_list.end(); ++cii ) {
         names += *cii + ", ";
      }
      names.resize( names.length() - 2 ); // remove trailing comma and space
      errmsg << names << "\n\tThe required federates are: ";
      names = "";
      for ( i = 0; i < known_feds_count; ++i ) {
         if ( known_feds[i].required ) {
            names += known_feds[i].name;
            names += ", ";
         }
      }
      names.resize( names.length() - 2 ); // remove trailing comma and space
      errmsg << names << "\nTERMINATING EXECUTION!";

      status_string = errmsg.str();
      return status_string;
   }

   // Unsubscribe from all attributes for the MOM HLAfederate class.
   unsubscribe_all_HLAfederate_class_attributes_from_MOM();

   // Get the federate object instance names so that we can recover the
   // instance handles for the MOM object associated with each federate.
   determine_federate_MOM_object_instance_names();

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_required_federates_to_join():%d FOUND ALL REQUIRED FEDERATES!!!\n",
                       __LINE__ );
   }

   return status_string;
}

/*!
 *  @job_class{initialization}
 */
void Federate::initialize_MOM_handles()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::initialize_MOM_handles():%d\n",
                       __LINE__ );
   }

   bool error_flag = false;

   // Get the MOM Federation Class handle.
   try {
      this->MOM_HLAfederation_class_handle = RTI_ambassador->getObjectClassHandle( L"HLAmanager.HLAfederation" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getObjectClassHandle('HLAmanager.HLAfederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getObjectClassHandle('HLAmanager.HLAfederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getObjectClassHandle('HLAmanager.HLAfederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getObjectClassHandle('HLAmanager.HLAfederation')\n",
                       __LINE__ );
   }

   // Get the MOM Federates In Federation Attribute handle.
   try {
      this->MOM_HLAfederatesInFederation_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederation_class_handle,
                                                                                      L"HLAfederatesInFederation" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
   }

   // Get the MOM Auto Provide Attribute handle.
   try {
      this->MOM_HLAautoProvide_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederation_class_handle,
                                                                            L"HLAautoProvide" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   }

   // Get the MOM Federate Class handle.
   try {
      this->MOM_HLAfederate_class_handle = RTI_ambassador->getObjectClassHandle( L"HLAobjectRoot.HLAmanager.HLAfederate" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getObjectClassHandle('HLAobjectRoot.HLAmanager.HLAfederate')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getObjectClassHandle('HLAobjectRoot.HLAmanager.HLAfederate')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getObjectClassHandle('HLAobjectRoot.HLAmanager.HLAfederate')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getObjectClassHandle('HLAobjectRoot.HLAmanager.HLAfederate')\n",
                       __LINE__ );
   }

   // Get the MOM Federate Name Attribute handle.
   try {
      this->MOM_HLAfederateName_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederate_class_handle, L"HLAfederateName" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
   }

   // Get the MOM Federate Type Attribute handle.
   try {
      this->MOM_HLAfederateType_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederate_class_handle, L"HLAfederateType" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
   }

   // Get the MOM Federate Attribute handle.
   try {
      this->MOM_HLAfederate_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederate_class_handle, L"HLAfederateHandle" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
   }

   // Interaction: HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches
   //   Parameter: HLAautoProvide of type HLAswitches which is a HLAinteger32BE
   try {
      this->MOM_HLAsetSwitches_class_handle = RTI_ambassador->getInteractionClassHandle( L"HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getInteractionClassHandle('HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getInteractionClassHandle('HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getInteractionClassHandle('HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getInteractionClassHandle('HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches')\n",
                       __LINE__ );
   }

   try {
      this->MOM_HLAautoProvide_param_handle = RTI_ambassador->getParameterHandle( MOM_HLAsetSwitches_class_handle, L"HLAautoProvide" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getParameterHandle(MOM_HLAsetSwitches_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidInteractionClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
InvalidInteractionClassHandle ERROR for RTI_amb->getParameterHandle(MOM_HLAsetSwitches_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getParameterHandle(MOM_HLAsetSwitches_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getParameterHandle(MOM_HLAsetSwitches_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getParameterHandle(MOM_HLAsetSwitches_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( error_flag ) {
      DebugHandler::terminate_with_message( "Federate::initialize_MOM_handles() ERROR Detected!" );
   }
}

void Federate::subscribe_attributes(
   ObjectClassHandle const  &class_handle,
   AttributeHandleSet const &attribute_list )
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream summary;
      summary << "Federate::subscribe_attributes():" << __LINE__ << '\n';

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, class_handle );
         summary << "  Class-Handle:" << handle_str << " with "
                 << attribute_list.size() << " Attributes\n";

         AttributeHandleSet::const_iterator attr_iter;
         for ( attr_iter = attribute_list.begin();
               attr_iter != attribute_list.end();
               ++attr_iter ) {

            StringUtilities::to_string( handle_str, *attr_iter );
            summary << "   + Attribute-Handle:" << handle_str << '\n';
         }
      }
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   bool error_flag = false;

   try {
      RTI_ambassador->subscribeObjectClassAttributes( class_handle, attribute_list, true );
   } catch ( RTI1516_NAMESPACE::ObjectClassNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::subscribe_attributes():%d ObjectClassNotDefined: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::AttributeNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::subscribe_attributes():%d AttributeNotDefined: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::subscribe_attributes():%d FederateNotExecutionMember: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::subscribe_attributes():%d SaveInProgress: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::subscribe_attributes():%d RestoreInProgress: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidUpdateRateDesignator const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::subscribe_attributes():%d InvalidUpdateRateDesignator: MOM Object Attributed Subscribe FAILED!!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::subscribe_attributes():%d NotConnected: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::subscribe_attributes():%d RTIinternalError: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( error_flag ) {
      DebugHandler::terminate_with_message( "Federate::subscribe_attributes() ERROR Detected!" );
   }
}

void Federate::unsubscribe_attributes(
   ObjectClassHandle const  &class_handle,
   AttributeHandleSet const &attribute_list )
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream summary;
      summary << "Federate::unsubscribe_attributes():" << __LINE__ << '\n';

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, class_handle );
         summary << "  Class-Handle:" << handle_str << " with "
                 << attribute_list.size() << " Attributes\n";

         AttributeHandleSet::const_iterator attr_iter;
         for ( attr_iter = attribute_list.begin();
               attr_iter != attribute_list.end();
               ++attr_iter ) {
            StringUtilities::to_string( handle_str, *attr_iter );
            summary << "   + Attribute-Handle:" << handle_str << '\n';
         }
      }
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   bool error_flag = false;

   try {
      RTI_ambassador->unsubscribeObjectClassAttributes( class_handle, attribute_list );
   } catch ( RTI1516_NAMESPACE::ObjectClassNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::unsubscribe_attributes():%d ObjectClassNotDefined: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::AttributeNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::unsubscribe_attributes():%d AttributeNotDefined: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::unsubscribe_attributes():%d FederateNotExecutionMember: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::unsubscribe_attributes():%d SaveInProgress: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::unsubscribe_attributes():%d RestoreInProgress: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::unsubscribe_attributes():%d NotConnected: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::unsubscribe_attributes():%d RTIinternalError: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( error_flag ) {
      DebugHandler::terminate_with_message( "Federate::unsubscribe_attributes() ERROR Detected!" );
   }
}

void Federate::request_attribute_update(
   ObjectClassHandle const  &class_handle,
   AttributeHandleSet const &attribute_list )
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream summary;
      summary << "Federate::request_attribute_update():" << __LINE__ << '\n';

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, class_handle );
         summary << "  Class-Handle:" << handle_str << " with "
                 << attribute_list.size() << " Attributes\n";

         AttributeHandleSet::const_iterator attr_iter;
         for ( attr_iter = attribute_list.begin();
               attr_iter != attribute_list.end();
               ++attr_iter ) {
            StringUtilities::to_string( handle_str, *attr_iter );
            summary << "   + Attribute-Handle:" << handle_str << '\n';
         }
      }
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   bool error_flag = false;

   try {
      // Request initial values.
      RTI_ambassador->requestAttributeValueUpdate( class_handle,
                                                   attribute_list,
                                                   RTI1516_USERDATA( 0, 0 ) );
   } catch ( ObjectClassNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::request_attribute_update():%d ObjectClassNotDefined: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( AttributeNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::request_attribute_update():%d AttributeNotDefined: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::request_attribute_update():%d FederateNotExecutionMember: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::request_attribute_update():%d SaveInProgress: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::request_attribute_update():%d RestoreInProgress: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::request_attribute_update():%d NotConnected: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::request_attribute_update():%d RTIinternalError: MOM Object Attributed update request FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( error_flag ) {
      DebugHandler::terminate_with_message( "Federate::request_attribute_update() ERROR Detected!" );
   }
}

void Federate::ask_MOM_for_federate_names()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::ask_MOM_for_federate_names():%d\n",
                       __LINE__ );
   }

   // Concurrency critical code section because joined-federate state is changed
   // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
   // function.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &joined_federate_mutex );

      // NOTE: Do not clear the joined_federate_name_map because it will cause
      // reflections to fail because lookup will not find the discovered instance.

      // Clear the set of federate handles for the joined federates.
      joined_federate_handles.clear();

      // Clear the list of joined federate names.
      joined_federate_names.clear();
   }

   // Make sure the MOM handles get initialized before we try to use them.
   if ( !MOM_HLAfederateName_handle.isValid() ) {
      initialize_MOM_handles();
   }

   AttributeHandleSet fedMomAttributes;
   fedMomAttributes.insert( MOM_HLAfederateName_handle );
   fedMomAttributes.insert( MOM_HLAfederate_handle );
   subscribe_attributes( MOM_HLAfederate_class_handle, fedMomAttributes );

   AttributeHandleSet requestedAttributes;
   requestedAttributes.insert( MOM_HLAfederateName_handle );
   requestedAttributes.insert( MOM_HLAfederate_handle );
   request_attribute_update( MOM_HLAfederate_class_handle, requestedAttributes );

   fedMomAttributes.clear();
   requestedAttributes.clear();
}

void Federate::unsubscribe_all_HLAfederate_class_attributes_from_MOM()
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream summary;
      summary << "Federate::unsubscribe_all_HLAfederate_class_attributes_from_MOM():"
              << __LINE__ << '\n';

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, MOM_HLAfederate_class_handle );
         summary << "  Class-Handle:" << handle_str << '\n';
      }
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      // We are done with the MOM interface to unsubscribe from all the
      // class attributes.
      RTI_ambassador->unsubscribeObjectClass( MOM_HLAfederate_class_handle );
   } catch ( ObjectClassNotDefined const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederate_class_attributes_from_MOM():%d ObjectClassNotDefined: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederate_class_attributes_from_MOM():%d FederateNotExecutionMember: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederate_class_attributes_from_MOM():%d SaveInProgress: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederate_class_attributes_from_MOM():%d RestoreInProgress: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederate_class_attributes_from_MOM():%d NotConnected: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederate_class_attributes_from_MOM():%d RTIinternalError: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::unsubscribe_all_HLAfederation_class_attributes_from_MOM()
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::unsubscribe_all_HLAfederation_class_attributes_from_MOM():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      // We are done with the MOM interface so unsubscribe from the class we used.
      RTI_ambassador->unsubscribeObjectClass( MOM_HLAfederation_class_handle );
   } catch ( ObjectClassNotDefined const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederation_class_attributes_from_MOM():%d ObjectClassNotDefined: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederation_class_attributes_from_MOM():%d FederateNotExecutionMember: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederation_class_attributes_from_MOM():%d SaveInProgress: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederation_class_attributes_from_MOM():%d RestoreInProgress: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederation_class_attributes_from_MOM():%d NotConnected: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederation_class_attributes_from_MOM():%d RTIinternalError: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::publish_interaction_class(
   RTI1516_NAMESPACE::InteractionClassHandle const &class_handle )
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::publish_interaction_class():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      RTI_ambassador->publishInteractionClass( class_handle );
   } catch ( InteractionClassNotDefined const &e ) {
      message_publish( MSG_WARNING, "Federate::publish_interaction_class():%d InteractionClassNotDefined: Publish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::publish_interaction_class():%d FederateNotExecutionMember: Publish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::publish_interaction_class():%d SaveInProgress: Publish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::publish_interaction_class():%d RestoreInProgress: Publish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::publish_interaction_class():%d NotConnected: Publish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::publish_interaction_class():%d RTIinternalError: Publish interaction class FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::unpublish_interaction_class(
   RTI1516_NAMESPACE::InteractionClassHandle const &class_handle )
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::unpublish_interaction_class():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      RTI_ambassador->unpublishInteractionClass( class_handle );
   } catch ( InteractionClassNotDefined const &e ) {
      message_publish( MSG_WARNING, "Federate::unpublish_interaction_class():%d InteractionClassNotDefined: Unpublish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::unpublish_interaction_class():%d FederateNotExecutionMember: Unpublish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::unpublish_interaction_class():%d SaveInProgress: Unpublish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::unpublish_interaction_class():%d RestoreInProgress: Unpublish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::unpublish_interaction_class():%d NotConnected: Unpublish interaction class FAILED!\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::unpublish_interaction_class():%d RTIinternalError: Unpublish interaction class FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::send_interaction(
   RTI1516_NAMESPACE::InteractionClassHandle const  &class_handle,
   RTI1516_NAMESPACE::ParameterHandleValueMap const &parameter_list )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   bool error_flag = false;
   try {
      RTI_ambassador->sendInteraction( class_handle, parameter_list, RTI1516_USERDATA( 0, 0 ) );
   } catch ( InteractionClassNotPublished const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::send_interaction():%d InteractionClassNotPublished: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( InteractionParameterNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::send_interaction():%d InteractionParameterNotDefined: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( InteractionClassNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::send_interaction():%d InteractionClassNotDefined: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::send_interaction():%d SaveInProgress: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::send_interaction():%d RestoreInProgress: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::send_interaction():%d FederateNotExecutionMember: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::send_interaction():%d NotConnected: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_WARNING, "Federate::send_interaction():%d RTIinternalError: Send interaction FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( error_flag ) {
      DebugHandler::terminate_with_message( "Federate::send_interaction() ERROR Detected!" );
   }
}

void Federate::announce_sync_point(
   wstring const          &label,
   RTI1516_USERDATA const &user_supplied_tag )
{
   // Delegate to the Execution Control to handle the FedAmb callback. It will
   // check for any synchronization points that require special handling.
   execution_control->sync_point_announced( label, user_supplied_tag );
}

void Federate::sync_point_registration_succeeded(
   wstring const &label )
{
   // Delegate to the Execution Control to handle the FedAmb callback.
   execution_control->sync_point_registration_succeeded( label );
}

void Federate::sync_point_registration_failed(
   wstring const                    &label,
   SynchronizationPointFailureReason reason )
{
   // Delegate to the Execution Control to handle the FedAmb callback.
   execution_control->sync_point_registration_failed( label, reason );
}

void Federate::federation_synchronized(
   wstring const &label )
{
   // Delegate to the Execution Control to handle the FedAmb callback.
   execution_control->sync_point_federation_synchronized( label );
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Currently only used with SRFOM initialization schemes.
 *  @job_class{freeze_init}
 */
void Federate::freeze_init()
{
   // Dispatch to the ExecutionControl method.
   execution_control->freeze_init();
}

/*!
 *  @job_class{end_of_frame}
 */
void Federate::enter_freeze()
{
   // Initiate a federation freeze when a Trick freeze is commanded. (If we're
   // here at time 0, set_exec_freeze_command was called in input.py file.)
   // Otherwise get out now.
   if ( execution_control->get_sim_time() > 0.0 ) {
      if ( exec_get_exec_command() != FreezeCmd ) {
         return; // Trick freeze has not been commanded.
      }
      if ( execution_control->is_freeze_pending() ) {
         return; // freeze already commanded and we will freeze at top of next frame
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::enter_freeze():%d\n", __LINE__ );
   }

   // Dispatch to the ExecutionControl method.
   execution_control->enter_freeze();
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Currently only used with DIS and IMSIM initialization schemes.
 *  @job_class{unfreeze}
 */
void Federate::exit_freeze()
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::exit_freeze():%d announce_freeze:%s, freeze_federation:%s\n",
                       __LINE__, ( execution_control->is_freeze_announced() ? "Yes" : "No" ),
                       ( execution_control->is_freeze_pending() ? "Yes" : "No" ) );
   }

   // Dispatch to the ExecutionControl method.
   execution_control->exit_freeze();

   execution_control->set_freeze_pending( false );
}

/*!
 *  @job_class{freeze}
 */
void Federate::check_freeze()
{
   // Check to see if we should shutdown.
   check_for_shutdown_with_termination();

   // Check to see if the ExecutionControl should exit freeze.
   if ( execution_control->check_freeze_exit() ) {
      return;
   }

   SIM_MODE exec_mode = exec_get_mode();
   if ( exec_mode == Initialization ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::check_freeze():%d Pass first Time.\n",
                          __LINE__ );
      }
      return;
   }
   // We should only check for freeze if we are in Freeze mode. If we are not
   // in Freeze mode then return to avoid running the code below more than once.
   if ( exec_mode != Freeze ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::check_freeze():%d not in Freeze mode so returning.\n",
                          __LINE__ );
      }
      return;
   }
}

/*!
 *  Unfreeze the simulation.
 */
void Federate::un_freeze()
{
   exec_run();
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Currently only used with DIS and IMSIM initialization schemes.
 */
bool Federate::is_HLA_save_and_restore_supported()
{
   // Dispatch to the ExecutionControl mechanism.
   return ( execution_control->is_save_and_restore_supported() );
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSIM initialization schemes.
 *  @job_class{freeze}
 */
void Federate::perform_checkpoint()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_HLA_save_and_restore_supported() ) {
      return;
   }

   // Dispatch to the ExecutionControl method.
   bool force_checkpoint = execution_control->perform_save();

   if ( this->start_to_save || force_checkpoint ) {
      // If I announced the save, sim control panel was clicked and invokes the checkpoint
      if ( !announce_save ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::perform_checkpoint():%d Federate Save Started \n",
                             __LINE__ );
         }
         // Create the filename from the Federation name and the "save-name".
         // Replace all directory characters with an underscore.
         string save_name_str;
         StringUtilities::to_string( save_name_str, this->save_name );
         string str_save_label = string( get_federation_name() ) + "_" + save_name_str;
         for ( int i = 0; i < (int)str_save_label.length(); ++i ) {
            if ( str_save_label[i] == '/' ) {
               str_save_label[i] = '_';
            }
         }

         // calls setup_checkpoint first
         checkpoint( str_save_label.c_str() );
      }
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::perform_checkpoint():%d Checkpoint Dump Completed.\n",
                          __LINE__ );
      }

      post_checkpoint();
   }
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with IMSim initialization scheme.
 *  @job_class{checkpoint}
 */
void Federate::setup_checkpoint()
{
   string str_save_label( this->save_label );

   // Don't do federate save during Init or Exit (this allows "regular" init and shutdown checkpoints)
   if ( ( exec_get_mode() == Initialization ) || ( exec_get_mode() == ExitMode ) ) {
      return;
   }

   // Determine if I am the federate that clicked Dump Chkpnt on sim control panel
   // or I am the federate that called start_federation_save
   this->announce_save = !this->start_to_save;

   // Check to see if the save has been initiated in the ExecutionControl process?
   // If not then just return.
   if ( !execution_control->is_save_initiated() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::setup_checkpoint():%d Federate Save Pre-checkpoint \n",
                       __LINE__ );
   }

   // If I announced the save, must initiate federation save
   if ( this->announce_save ) {
      if ( save_name.length() ) {
         // When user calls start_federation_save, save_name is already set
      } else {
         // When user clicks Dump Chkpnt, we need to set the save_name here
         string trick_filename;
         string slash( "/" );
         int    found;
         string save_name_str;

         // get checkpoint file name specified in control panel
         trick_filename = checkpoint_get_output_file();

         // Trick filename contains dir/filename,
         // need to prepend federation name to filename entered in sim control panel popup
         found = trick_filename.rfind( slash );
         if ( found != (int)string::npos ) {
            save_name_str              = trick_filename.substr( found + 1 );
            string federation_name_str = string( get_federation_name() );
            if ( save_name_str.compare( 0, federation_name_str.length(), federation_name_str ) != 0 ) {
               // dir/federation_filename
               trick_filename.replace( found, slash.length(), slash + federation_name_str + "_" );
            } else {
               // If it already has federation name prepended, output_file name
               // is good to go but remove it from save_name_str so our
               // str_save_label setting below is correct
               save_name_str = trick_filename.substr( found + 1 + federation_name_str.length() + 1 ); // filename
            }
         } else {
            save_name_str = trick_filename;
         }

         // TODO: Clean this up later.
         // Set the checkpoint restart files name.
         the_cpr->output_file = trick_filename;

         // federation_filename
         str_save_label = string( get_federation_name() ) + "_" + save_name_str;

         // Set the federate save_name to filename (without the federation name)
         // - this gets announced to other feds
         wstring save_name_ws;
         StringUtilities::to_wstring( save_name_ws, save_name_str );

         set_save_name( save_name_ws );
      } // end set save_name

      // Don't request a save if another federate has already requested one
      if ( this->initiate_save_flag ) {
         // initiate_save_flag becomes false if another save is occurring
         request_federation_save_status();
         wait_for_save_status_to_complete();

         request_federation_save();

         int64_t      wallclock_time;
         SleepTimeout print_timer( this->wait_status_time );
         SleepTimeout sleep_timer;

         // need to wait for federation to initiate save
         while ( !start_to_save ) {

            // Check for shutdown.
            check_for_shutdown_with_termination();

            sleep_timer.sleep();

            if ( !this->start_to_save ) {

               // To be more efficient, we get the time once and share it.
               wallclock_time = sleep_timer.time();

               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();
                  if ( !is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "Federate::setup_checkpoint():" << __LINE__
                            << " ERROR: Unexpectedly the Federate is no longer an execution"
                            << " member. This means we are either not connected to the"
                            << " RTI or we are no longer joined to the federation"
                            << " execution because someone forced our resignation at"
                            << " the Central RTI Component (CRC) level!\n";
                     DebugHandler::terminate_with_message( errmsg.str() );
                  }
               }

               if ( print_timer.timeout( wallclock_time ) ) {
                  print_timer.reset();
                  message_publish( MSG_NORMAL, "Federate::setup_checkpoint():%d Federate Save Pre-checkpoint, wiating...\n",
                                   __LINE__ );
               }
            }
         }
         this->initiate_save_flag = false;
      } else {
         message_publish( MSG_NORMAL, "Federate::setup_checkpoint():%d Federation Save is already in progress! \n",
                          __LINE__ );
         return;
      }
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;
   try {
      RTI_ambassador->federateSaveBegun();
   } catch ( SaveNotInitiated const &e ) {
      message_publish( MSG_WARNING, "Federate::setup_checkpoint():%d EXCEPTION: SaveNotInitiated\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::setup_checkpoint():%d EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::setup_checkpoint():%d EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::setup_checkpoint():%d EXCEPTION: NotConnected\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_checkpoint():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // This is a shortcut so that we can enforce that only these federates exist
   // when we restore
   write_running_feds_file( str_save_label );

   // Tell the manager to setup the checkpoint data structures.
   manager->encode_checkpoint();

   // Save any synchronization points.
   convert_sync_pts();
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSIM initialization schemes.
 *  @job_class{post_checkpoint}
 */
void Federate::post_checkpoint()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_HLA_save_and_restore_supported() ) {
      return;
   }

   if ( this->start_to_save ) {

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;
      try {
         RTI_ambassador->federateSaveComplete();
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::post_checkpoint():%d Federate Save Completed.\n",
                             __LINE__ );
         }
         start_to_save = false;
      } catch ( FederateHasNotBegunSave const &e ) {
         message_publish( MSG_WARNING, "Federate::post_checkpoint():%d EXCEPTION: FederateHasNotBegunSave\n",
                          __LINE__ );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Federate::post_checkpoint():%d EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::post_checkpoint():%d EXCEPTION: RestoreInProgress\n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Federate::post_checkpoint():%d EXCEPTION: NotConnected\n",
                          __LINE__ );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Federate::post_checkpoint():%d EXCEPTION: RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::post_checkpoint():%d Federate Save Already Completed.\n",
                          __LINE__ );
      }
   }
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSIM initialization schemes.
 *  @job_class{freeze}
 */
void Federate::perform_restore()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_HLA_save_and_restore_supported() ) {
      return;
   }

   if ( this->start_to_restore ) {
      // if I announced the restore, sim control panel was clicked and invokes the load
      if ( !this->announce_restore ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::perform_restore():%d Federate Restore Started.\n",
                             __LINE__ );
         }

         // Create the filename from the Federation name and the "restore-name".
         // Replace all directory characters with an underscore.
         string restore_name_str;
         StringUtilities::to_string( restore_name_str, restore_name );
         string str_restore_label = string( get_federation_name() ) + "_" + restore_name_str;
         for ( int i = 0; i < (int)str_restore_label.length(); ++i ) {
            if ( str_restore_label[i] == '/' ) {
               str_restore_label[i] = '_';
            }
         }
         message_publish( MSG_NORMAL, "Federate::perform_restore():%d LOADING %s\n",
                          __LINE__, str_restore_label.c_str() );

         // make sure we have a save directory specified
         check_HLA_save_directory();

         // This will run pre-load-checkpoint jobs, clear memory, read checkpoint file, and run restart jobs
         load_checkpoint( ( this->HLA_save_directory + "/" + str_restore_label ).c_str() );

         load_checkpoint_job();

         // exec_freeze();
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::perform_restore():%d Checkpoint Load Completed.\n",
                          __LINE__ );
      }

      post_restore();
   }
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSIM initialization schemes.
 *  @job_class{preload_checkpoint}
 */
void Federate::setup_restore()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_HLA_save_and_restore_supported() ) {
      return;
   }

   // if restoring at startup, do nothing here (that is handled in restore_checkpoint)
   if ( !is_federate_executing() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::setup_restore():%d Federate Restore Pre-load.\n",
                       __LINE__ );
   }
   // Determine if I am the federate that clicked Load Chkpnt on sim control panel
   this->announce_restore = !this->start_to_restore;
   execution_control->set_freeze_announced( this->announce_restore );

   // if I announced the restore, must initiate federation restore
   if ( this->announce_restore ) {
      string trick_filename;
      string slash_fedname( "/" + string( get_federation_name() ) + "_" );
      int    found;

      // Otherwise set restore_name_str using trick's file name
      trick_filename = checkpoint_get_load_file();

      // Trick memory manager load_checkpoint_file_name already contains correct dir/federation_filename
      // (chosen in sim control panel popup) we need just the filename minus the federation name to initiate restore
      found = trick_filename.rfind( slash_fedname );
      string restore_name_str;
      if ( found != (int)string::npos ) {
         restore_name_str = trick_filename.substr( found + slash_fedname.length() ); // filename
      } else {
         restore_name_str = trick_filename;
      }
      // federation_filename
      string str_restore_label = string( get_federation_name() ) + "_" + restore_name_str;

      // make sure we have a save directory specified
      check_HLA_save_directory();

      // make sure only the required federates are in the federation before we do the restore
      read_running_feds_file( str_restore_label );

      string return_string;
      return_string = wait_for_required_federates_to_join(); // sets running_feds_count
      if ( !return_string.empty() ) {
         return_string += '\n';
         ostringstream errmsg;
         errmsg << "Federate::setup_restore():" << __LINE__ << '\n'
                << "ERROR: " << return_string;
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      // set the federate restore_name to filename (without the federation name)- this gets announced to other feds
      initiate_restore_announce( restore_name_str );

      int64_t      wallclock_time;
      SleepTimeout print_timer( this->wait_status_time );
      SleepTimeout sleep_timer;

      // need to wait for federation to initiate restore
      while ( !this->start_to_restore ) {

         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !this->start_to_restore ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Federate::setup_restore():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "Federate::setup_restore():%d Federate Restore Pre-load, waiting...\n",
                                __LINE__ );
            }
         }
      }
   }

   this->restore_process = Restore_In_Progress;
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSIM initialization schemes.
 */
void Federate::post_restore()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_HLA_save_and_restore_supported() ) {
      return;
   }

   if ( this->start_to_restore ) {
      restore_process = Restore_Complete;

      // Make a copy of restore_process because it is used in the
      // inform_RTI_of_restore_completion() function.
      // (backward compatibility with previous restore process)
      this->prev_restore_process = this->restore_process;

      copy_running_feds_into_known_feds();

      // wait for RTI to inform us that the federation restore has
      // begun before informing the RTI that we are done.
      wait_for_federation_restore_begun();

      // signal RTI that this federate has already been loaded
      inform_RTI_of_restore_completion();

      // wait until we get a callback to inform us that the federation restore is complete
      string tStr = wait_for_federation_restore_to_complete();
      if ( tStr.length() ) {
         wait_for_federation_restore_failed_callback_to_complete();
         ostringstream errmsg;
         errmsg << "TrickFederate::post_restore():" << __LINE__
                << " ERROR: " << tStr << '\n';
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::post_restore():%d Federation Restore Completed.\n",
                          __LINE__ );
         message_publish( MSG_NORMAL, "Federate::post_restore():%d Rebuilding HLA Handles.\n",
                          __LINE__ );
      }

      // get us restarted again...
      // reset RTI data to the state it was in when checkpointed
      manager->reset_mgr_initialized();
      manager->setup_all_ref_attributes();
      manager->setup_all_RTI_handles();
      manager->set_all_object_instance_handles_by_name();

      if ( this->announce_restore ) {
         set_all_federate_MOM_instance_handles_by_name();
         restore_federate_handles_from_MOM();
      }

      // Restore interactions and sync points
      manager->decode_checkpoint_interactions();
      reinstate_logged_sync_pts();

      // Restore ownership transfer data for all objects
      Object *objects   = manager->get_objects();
      int     obj_count = manager->get_object_count();
      for ( int i = 0; i < obj_count; ++i ) {
         objects[i].decode_checkpoint();
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;
      try {
         HLAinteger64Time time;
         RTI_ambassador->queryLogicalTime( time );
         set_granted_time( time );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Federate::post_restore():%d queryLogicalTime EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::post_restore():%d queryLogicalTime EXCEPTION: SaveInProgress\n",
                          __LINE__ );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::post_restore():%d queryLogicalTime EXCEPTION: RestoreInProgress\n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Federate::post_restore():%d queryLogicalTime EXCEPTION: NotConnected\n",
                          __LINE__ );
      } catch ( RTIinternalError const &e ) {
         message_publish( MSG_WARNING, "Federate::post_restore():%d queryLogicalTime EXCEPTION: RTIinternalError\n",
                          __LINE__ );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
         this->requested_time = this->granted_time;
      }

      federation_restored();

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::post_restore():%d Federate Restart Completed.\n",
                          __LINE__ );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::post_restore():%d Federate Restore Already Completed.\n",
                          __LINE__ );
      }
   }
}

/*! @brief Set the time advance as granted. */
void Federate::set_time_advance_granted(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   Int64Time int64_time( time );

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

   // Ignore any granted time less than the requested time otherwise it will
   // break our concept of HLA time since we are using scheduled jobs for
   // processing HLA data sends, receives, etc and expected the next granted
   // time to match our requested time. Dan Dexter, 2/12/2007
   if ( int64_time >= this->requested_time ) {

      granted_time.set( int64_time );

      // Record the granted time in the HLA_time variable, so we can plot it
      // in Trick data products.
      this->HLA_time = granted_time.get_time_in_seconds();

      this->time_adv_state = TIME_ADVANCE_GRANTED;

      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::set_time_advance_granted():%d Granted-time:%f, Requested-time:%f.\n",
                          __LINE__, this->HLA_time, requested_time.get_time_in_seconds() );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::set_time_advance_granted():%d WARNING: Federate \"%s\" \
IGNORING GRANTED TIME %.12G because it is less then requested time %.12G.\n",
                          __LINE__, get_federate_name(), int64_time.get_time_in_seconds(),
                          requested_time.get_time_in_seconds() );
      }
   }
}

void Federate::set_granted_time(
   double const time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

   granted_time.set( time );

   // Record the granted time in the HLA_time variable, so we can plot it
   // in Trick data products.
   this->HLA_time = granted_time.get_time_in_seconds();
}

void Federate::set_granted_time(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

   granted_time.set( time );

   // Record the granted time in the HLA_time variable, so we can plot it
   // in Trick data products.
   this->HLA_time = granted_time.get_time_in_seconds();
}

void Federate::set_requested_time(
   double const time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
   requested_time.set( time );
}

void Federate::set_requested_time(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
   requested_time.set( time );
}

HLABaseTimeEnum Federate::get_HLA_base_time_units() const
{
   return Int64BaseTime::get_base_units();
}

void Federate::set_HLA_base_time_units(
   HLABaseTimeEnum const base_time_units )
{
   // Set the HLA Logical time base units in the global Int64BaseTime.
   Int64BaseTime::set( base_time_units );

   // Refresh the HLA time constants based on the updated base time.
   refresh_HLA_time_constants();
}

void Federate::refresh_HLA_time_constants()
{
   // Refresh the lookahead time given a possible new HLA base time units.
   refresh_lookahead();

   // Refresh the LCTS given a possible new HLA base time units.
   execution_control->refresh_least_common_time_step();

   // Refresh the HLA cycle time in base time.
   this->HLA_cycle_time_in_base_time = Int64BaseTime::to_base_time( this->HLA_cycle_time );
}

void Federate::scale_trick_tics_to_base_time_units()
{
   long long time_res  = Int64BaseTime::get_base_time_multiplier();
   long long tic_value = exec_get_time_tic_value();

   // Scale up the Trick time Tic value to support the HLA base time units.
   // Trick Time Tics is limited to a value of 2^31.
   while ( ( tic_value < time_res ) && ( tic_value < std::numeric_limits< int >::max() ) ) {
      tic_value *= 10;
   }

   if ( tic_value <= std::numeric_limits< int >::max() ) {
      // Update the Trick Time Tic value only if we are increasing the resolution.
      if ( tic_value > exec_get_time_tic_value() ) {
         exec_set_time_tic_value( tic_value );

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::scale_trick_tics_to_base_time_units():%d New Trick time tics:%d.\n",
                             __LINE__, tic_value );
         }
      }
   } else {
      ostringstream errmsg;
      errmsg << "Federate::scale_trick_tics_to_base_time_units():" << __LINE__
             << " ERROR: Trick cannot represent the required time Tic value "
             << setprecision( 18 ) << time_res
             << " in order to support the HLA base units of '"
             << Int64BaseTime::get_units()
             << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

void Federate::set_lookahead(
   double const value )
{
   // Determine if the lookahead time needs a resolution that exceeds the
   // configured HLA base time.
   if ( Int64BaseTime::exceeds_base_time_resolution( value ) ) {
      ostringstream errmsg;
      errmsg << "Federate::set_lookahead():" << __LINE__
             << " ERROR: The lookahead time specified (" << setprecision( 18 ) << value
             << " seconds) requires more resolution than whole "
             << Int64BaseTime::get_units()
             << ". The HLA Logical Time is a 64-bit integer"
             << " representing " << Int64BaseTime::get_units()
             << " and cannot represent a lookahead time of "
             << setprecision( 18 ) << ( value * Int64BaseTime::get_base_time_multiplier() )
             << " " << Int64BaseTime::get_units() << ". You can adjust the"
             << " base HLA Logical Time resolution by setting"
             << " 'THLA.federate.HLA_time_base_units = trick."
             << Int64BaseTime::get_units_string( Int64BaseTime::best_base_time_resolution( value ) )
             << "' or 'federate.set_HLA_base_time_units( "
             << Int64BaseTime::get_units_string( Int64BaseTime::best_base_time_resolution( value ) )
             << " )' in your input.py file. The current HLA base time resolution is "
             << Int64BaseTime::get_units_string( Int64BaseTime::get_base_units() )
             << ". You also need to update both the Federation Execution"
             << " Specific Federation Agreement (FESFA) and Federate Compliance"
             << " Declaration (FCD) documents for your Federation to document"
             << " the change in timing class resolution.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Determine if the Trick time Tic can represent the lookahead time.
   if ( Int64BaseTime::exceeds_base_time_resolution( value, exec_get_time_tic_value() ) ) {
      ostringstream errmsg;
      errmsg << "Federate::set_lookahead():" << __LINE__
             << " ERROR: The Trick time tic value (" << exec_get_time_tic_value()
             << ") does not have enough resolution to represent the HLA lookahead time ("
             << setprecision( 18 ) << value
             << " seconds). Please update the Trick time tic value in your"
             << " input.py file (i.e. by calling 'trick.exec_set_time_tic_value()').\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
   lookahead.set( value );
   this->lookahead_time = value;
}

/*! @brief Update the HLA lookahead base time. */
void Federate::refresh_lookahead()
{
   // Recalculate the lookahead HLA time in base time units.
   set_lookahead( this->lookahead_time );
}

void Federate::time_advance_request_to_GALT()
{
   // Simply return if we are the master federate that created the federation,
   // or if time management is not enabled.
   if ( !this->time_management || ( execution_control->is_master() && !execution_control->is_late_joiner() ) ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      HLAinteger64Time time;
      if ( RTI_ambassador->queryGALT( time ) ) {
         int64_t L = lookahead.get_base_time();
         if ( L > 0 ) {
            int64_t GALT = time.getTime();

            // Make sure the time is an integer multiple of the lookahead time.
            time.setTime( ( ( GALT / L ) + 1 ) * L );
         }
         set_requested_time( time );
      }
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: NotConnected\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT():%d Query-GALT EXCEPTION: RTIinternalError\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::time_advance_request_to_GALT():%d Requested-Time:%lf\n",
                       __LINE__, requested_time.get_time_in_seconds() );
   }

   // Perform the time-advance request to go to the requested time.
   perform_time_advance_request();
}

void Federate::time_advance_request_to_GALT_LCTS_multiple()
{
   // Simply return if we are the master federate that created the federation,
   // or if time management is not enabled.
   if ( !this->time_management || ( execution_control->is_master() && !execution_control->is_late_joiner() ) ) {
      return;
   }

   // Setup the Least-Common-Time-Step time value.
   int64_t LCTS = execution_control->get_least_common_time_step();
   if ( LCTS <= 0 ) {
      LCTS = lookahead.get_base_time();
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      HLAinteger64Time time;
      if ( RTI_ambassador->queryGALT( time ) ) {
         if ( LCTS > 0 ) {
            int64_t GALT = time.getTime();

            // Make sure the time is an integer multiple of the LCTS time.
            time.setTime( ( ( GALT / LCTS ) + 1 ) * LCTS );
         }
         set_requested_time( time );
      }
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: SaveInProgress\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: RestoreInProgress\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: NotConnected\n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::time_advance_request_to_GALT_LCTS_multiple():%d Query-GALT EXCEPTION: RTIinternalError\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::time_advance_request_to_GALT_LCTS_multiple():%d Requested-Time:%lf\n",
                       __LINE__, requested_time.get_time_in_seconds() );
   }

   // Perform the time-advance request to go to the requested time.
   perform_time_advance_request();
}

/*!
 * @job_class{initialization}
 */
void Federate::create_federation()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( RTI_ambassador.get() == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " ERROR: NULL pointer to RTIambassador!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::create_federation():%d Attempting to create Federation '%s'\n",
                       __LINE__, get_federation_name() );
   }

   // Create the wide-string version of the federation name.
   wstring federation_name_ws;
   StringUtilities::to_wstring( federation_name_ws, federation_name );

   try {
      this->federation_created_by_federate = false;
      this->federation_exists              = false;

      wstring          MIM_module_ws = L"";
      VectorOfWstrings FOM_modules_vector;

      // Add the user specified FOM-modules to the vector by parsing the comma
      // separated list of modules.
      if ( FOM_modules != NULL ) {
         StringUtilities::tokenize( FOM_modules, FOM_modules_vector, "," );
      }

      // Determine if the user specified a MIM-module, which determines how
      // we create the federation execution.
      if ( MIM_module != NULL ) {
         StringUtilities::to_wstring( MIM_module_ws, MIM_module );
      }

      if ( MIM_module_ws.empty() ) {
         // Create the Federation execution.
         RTI_ambassador->createFederationExecution( federation_name_ws,
                                                    FOM_modules_vector,
                                                    L"HLAinteger64Time" );
      } else {
         // Create the Federation execution with a user specified MIM.
         RTI_ambassador->createFederationExecutionWithMIM( federation_name_ws,
                                                           FOM_modules_vector,
                                                           MIM_module_ws,
                                                           L"HLAinteger64Time" );
      }

      this->federation_created_by_federate = true;
      this->federation_exists              = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::create_federation():%d Created Federation '%s'\n",
                          __LINE__, get_federation_name() );
      }
   } catch ( RTI1516_NAMESPACE::FederationExecutionAlreadyExists const &e ) {
      // Just ignore the exception if the federation execution already exits
      // because of how the multiphase initialization is designed this is not
      // an error since everyone tries to create the federation as the first
      // thing they do.
      this->federation_exists = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::create_federation():%d Federation already exists for '%s'\n",
                          __LINE__, get_federation_name() );
      }
   } catch ( RTI1516_NAMESPACE::CouldNotOpenFDD const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " ERROR: Could not open FOM-modules: '"
             << FOM_modules << "'";
      if ( MIM_module != NULL ) {
         errmsg << " or MIM-module: '" << MIM_module << "'";
      }
      errmsg << ", RTI Exception: " << rti_err_msg << '\n';
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::ErrorReadingFDD const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " ERROR: Problem reading FOM-modules: '"
             << FOM_modules << "'";
      if ( MIM_module != NULL ) {
         errmsg << " or MIM-module: '" << MIM_module << "'";
      }
      errmsg << ", RTI Exception: " << rti_err_msg << '\n';
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::CouldNotCreateLogicalTimeFactory const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " ERROR: Could not create logical time factory 'HLAinteger64Time"
             << "', RTI Exception: " << rti_err_msg << "\n  Make sure that you "
             << "are using a IEEE_1516_2010-compliant RTI version which "
             << "supplies the 'HLAinteger64Time' class.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " EXCEPTION: NotConnected\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " RTI Internal Error: " << rti_err_msg << '\n';
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      // This is an error so show out an informative message and terminate.
      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " ERROR: Unrecoverable error in federation '" << get_federation_name()
             << "' creation, RTI Exception: " << rti_err_msg << '\n';
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{initialization}
 */
void Federate::join_federation(
   char const *const federate_name,
   char const *const federate_type )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( RTI_ambassador.get() == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " ERROR: NULL pointer to RTIambassador!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   if ( federate_ambassador == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " ERROR: NULL pointer to FederateAmbassador!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   if ( this->federation_joined ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream errmsg;
         errmsg << "Federate::join_federation():" << __LINE__
                << " Federation '" << get_federation_name()
                << "': ALREADY JOINED FEDERATION EXECUTION\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Make sure the federate name has been specified.
   if ( ( federate_name == NULL ) || ( *federate_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " ERROR: Unexpected NULL federate name.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Create the wide-string version of the federation and federate name & type.
   wstring federation_name_ws;
   StringUtilities::to_wstring( federation_name_ws, federation_name );
   wstring fed_name_ws;
   StringUtilities::to_wstring( fed_name_ws, federate_name );
   wstring fed_type_ws;
   if ( ( federate_type == NULL ) || ( *federate_type == '\0' ) ) {
      // Just set the federate type to the name if it was not specified.
      StringUtilities::to_wstring( fed_type_ws, federate_name );
   } else {
      StringUtilities::to_wstring( fed_type_ws, federate_type );
   }

   // Join the named federation execution as the named federate type.
   // Federate types (2nd argument to joinFederationExecution) does not have
   // to be unique in a federation execution; however, the save/restore
   // services use this information but we are not doing save/restore here
   // so we won't worry about it here (best to make the names
   // unique if you do save/restore unless you understand how save/restore
   // will use the information.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::join_federation():%d Attempting to Join Federation '%s'\n",
                       __LINE__, get_federation_name() );
   }
   try {
      this->federation_joined = false;

      VectorOfWstrings fomModulesVector;

      // Add the user specified FOM-modules to the vector by parsing the comma
      // separated list of modules.
      if ( this->FOM_modules != NULL ) {
         StringUtilities::tokenize( FOM_modules, fomModulesVector, "," );
      }

      federate_id = RTI_ambassador->joinFederationExecution( fed_name_ws,
                                                             fed_type_ws,
                                                             federation_name_ws,
                                                             fomModulesVector );

      this->federation_joined = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string id_str;
         StringUtilities::to_string( id_str, federate_id );

         message_publish( MSG_NORMAL, "Federate::join_federation():%d Joined Federation '%s', Federate-Handle:%s\n",
                          __LINE__, get_federation_name(), id_str.c_str() );
      }
   } catch ( RTI1516_NAMESPACE::CouldNotCreateLogicalTimeFactory const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: CouldNotCreateLogicalTimeFactory\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederateNameAlreadyInUse const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: FederateNameAlreadyInUse! Federate name:\""
             << get_federate_name() << "\"\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::InconsistentFDD const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: InconsistentFDD! FOM-modules:\""
             << FOM_modules << "\"\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::ErrorReadingFDD const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: ErrorReadingFDD! FOM-modules:\""
             << FOM_modules << "\"\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::CouldNotOpenFDD const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: CouldNotOpenFDD! FOM-modules:\""
             << FOM_modules << "\"\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederateAlreadyExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " ERROR: The Federate '" << get_federate_name()
             << "' is already a member of the '"
             << get_federation_name() << "' Federation.\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederationExecutionDoesNotExist const &e ) {
      // The federation we created must have been destroyed by another
      // federate before we could join, so try again.
      this->federation_created_by_federate = false;
      this->federation_exists              = false;
      message_publish( MSG_WARNING, "Federate::join_federation():%d EXCEPTION: %s Federation Execution does not exist.\n",
                       __LINE__, get_federation_name() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::join_federation():%d EXCEPTION: SaveInProgress\n", __LINE__ );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::join_federation():%d EXCEPTION: RestoreInProgress\n", __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: NotConnected\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::CallNotAllowedFromWithinCallback const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: CallNotAllowedFromWithinCallback\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " ERROR: Federate '" << get_federate_name() << "' for Federation '"
             << get_federation_name() << "' encountered RTI Internal Error: "
             << rti_err_msg << '\n';

      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{initialization}
 */
void Federate::create_and_join_federation()
{
   if ( this->federation_joined ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream errmsg;
         errmsg << "Federate::create_and_join_federation():" << __LINE__
                << " Federation \"" << get_federation_name()
                << "\": ALREADY JOINED FEDERATION EXECUTION\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Here we loop around the create and join federation calls until until we
   // are successful or hit the maximum number of attempts.
   int const max_retries = 100;

   for ( int k = 1; ( !this->federation_joined ) && ( k <= max_retries ); ++k ) {

      if ( !this->federation_exists ) {
         create_federation();
      }

      join_federation( get_federate_name(), get_federate_type() );

      if ( !this->federation_joined ) {
         message_publish( MSG_WARNING, "Federate::create_and_join_federation():%d Failed to join federation \"%s\" on attempt %d of %d!\n",
                          __LINE__, get_federation_name(), k, max_retries );
         Utilities::micro_sleep( 100000 );
      }
   }

   if ( !this->federation_joined ) {
      ostringstream errmsg;
      errmsg << "Federate::create_and_join_federation():" << __LINE__
             << " ERROR: Federate '" << get_federate_name() << "' FAILED TO JOIN the '"
             << get_federation_name() << "' Federation.\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*!
 * @job_class{initialization}
 */
void Federate::enable_async_delivery()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( RTI_ambassador.get() == NULL ) {
      DebugHandler::terminate_with_message( "Federate::enable_async_delivery() ERROR: NULL pointer to RTIambassador!" );
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::enable_async_delivery():%d Enabling Asynchronous Delivery \n",
                          __LINE__ );
      }

      // Turn on asynchronous delivery of receive ordered messages. This will
      // allow us to receive messages that are not TimeStamp Ordered outside of
      // a time advancement.
      RTI_ambassador->enableAsynchronousDelivery();
   } catch ( AsynchronousDeliveryAlreadyEnabled const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Federate::enable_async_delivery():%d EXCEPTION: AsynchronousDeliveryAlreadyEnabled\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: SaveInProgress\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: RestoreInProgress\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: FederateNotExecutionMember\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: NotConnected\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: RTIinternalError: '" << rti_err_msg << "'\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::enable_async_delivery():%d \"%s\": Unexpected RTI exception!\nRTI Exception: RTIinternalError: '%s'\n\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{shutdown}
 */
bool Federate::check_for_shutdown()
{
   return ( execution_control->check_for_shutdown() );
}

/*!
 * @details NOTE: If a shutdown has been announced, this routine calls the
 * Trick exec_teminate() function. So, for shutdown, it should never return.
 * @job_class{shutdown}
 */
bool Federate::check_for_shutdown_with_termination()
{
   return ( execution_control->check_for_shutdown_with_termination() );
}

/*!
 * @job_class{initialization}.
 */
void Federate::setup_time_management()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::setup_time_management():%d time_management:%s time_regulating:%s time_constrained:%s \n",
                       __LINE__,
                       ( this->time_management ? "Yes" : "No" ),
                       ( this->time_regulating ? "Yes" : "No" ),
                       ( this->time_constrained ? "Yes" : "No" ) );
   }

   // Determine if HLA time management is enabled.
   if ( this->time_management ) {

      // Setup time constrained if the user wants to be constrained and our
      // current HLA time constrained state indicates we are not constrained.
      if ( this->time_constrained ) {
         if ( !this->time_constrained_state ) {
            setup_time_constrained();
         }
      } else {
         if ( this->time_constrained_state ) {
            // Disable time constrained if our current HLA state indicates we
            // are already constrained.
            shutdown_time_constrained();
         }
      }

      // Setup time regulation if the user wanted to be regulated and our
      // current HLA time regulating state indicates we are not regulated.
      if ( this->time_regulating ) {
         if ( !this->time_regulating_state ) {
            setup_time_regulation();
         }
      } else {
         if ( this->time_regulating_state ) {
            // Disable time regulation if our current HLA state indicates we
            // are already regulating.
            shutdown_time_regulating();
         }
      }
   } else {
      // HLA Time Management is disabled.

      // Disable time constrained and time regulation.
      if ( this->time_constrained_state ) {
         shutdown_time_constrained();
      }
      if ( this->time_regulating_state ) {
         shutdown_time_regulating();
      }
   }
}

void Federate::set_time_constrained_enabled(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      // Set the control flags after the debug show above to avoid a race condition
      // with the main Trick thread printing to the console when these flags are set.
      set_requested_time( time );
      set_time_advance_granted( time );
      set_time_constrained_state( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "Federate::set_time_constrained_enabled():%d Federate \
\"%s\" Time granted to: %.12G \n",
                       __LINE__, get_federate_name(),
                       get_granted_time().get_time_in_seconds() );
   }
}

/*!
 * @job_class{initialization}.
 */
void Federate::setup_time_constrained()
{
   // Just return if HLA time management is not enabled, the user does
   // not want time constrained enabled, or if we are already constrained.
   if ( !this->time_management || !this->time_constrained || this->time_constrained_state ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( RTI_ambassador.get() == NULL ) {
      DebugHandler::terminate_with_message( "Federate::setup_time_constrained() ERROR: NULL pointer to RTIambassador!" );
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::setup_time_constrained()%d \"%s\": ENABLING TIME CONSTRAINED \n",
                          __LINE__, get_federation_name() );
      }

      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

         this->time_adv_state         = TIME_ADVANCE_RESET;
         this->time_constrained_state = false;
      }

      // Turn on constrained status so that regulating federates will control
      // our advancement in time.
      //
      // If we are constrained and sending federates specify the Class
      // attributes and Communication interaction with timestamp in the
      // simulation fed file we will receive TimeStamp Ordered messages.
      RTI_ambassador->enableTimeConstrained();

      int64_t      wallclock_time;
      SleepTimeout print_timer( this->wait_status_time );
      SleepTimeout sleep_timer;

      // This spin lock waits for the time constrained flag to be set from the RTI.
      while ( !this->time_constrained_state ) {

         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !this->time_constrained_state ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Federate::setup_time_constrained():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "Federate::setup_time_constrained()%d \"%s\": ENABLING TIME CONSTRAINED, waiting...\n",
                                __LINE__, get_federation_name() );
            }
         }
      }
   } catch ( RTI1516_NAMESPACE::TimeConstrainedAlreadyEnabled const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->time_constrained_state = true;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_constrained():%d \"%s\": Time Constrained Already Enabled : '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_constrained():%d \"%s\": ERROR: InTimeAdvancingState : '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RequestForTimeConstrainedPending const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_constrained():%d \"%s\": ERROR: RequestForTimeConstrainedPending : '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_constrained():%d \"%s\": ERROR: FederateNotExecutionMember : '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "TrickHLAFderate::setup_time_constrained():%d \"%s\": ERROR: SaveInProgress : '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_constrained():%d \"%s\": ERROR: RestoreInProgress : '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_constrained():%d \"%s\": ERROR: NotConnected : '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_constrained():%d \"%s\": ERROR: RTIinternalError : '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_constrained():%d \"%s\": Unexpected RTI exception!\nRTI Exception: RTIinternalError: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*! @brief Enable time regulating.
 *  @param time the granted HLA Logical time */
void Federate::set_time_regulation_enabled(
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      // Set the control flags after the show above to avoid a race condition with
      // the main Trick thread printing to the console when these flags are set.
      set_requested_time( time );
      set_time_advance_granted( time );
      set_time_regulation_state( true );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "Federate::set_time_regulation_enabled():%d Federate \
\"%s\" Time granted to: %.12G \n",
                       __LINE__, get_federate_name(),
                       get_granted_time().get_time_in_seconds() );
   }
}

/*!
 * @job_class{initialization}.
 */
void Federate::setup_time_regulation()
{
   // Just return if HLA time management is not enabled, the user does
   // not want time regulation enabled, or if we are already regulating.
   if ( !this->time_management || !this->time_regulating || this->time_regulating_state ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( RTI_ambassador.get() == NULL ) {
      DebugHandler::terminate_with_message( "Federate::setup_time_regulation() ERROR: NULL pointer to RTIambassador!" );
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::setup_time_regulation():%d \"%s\": ENABLING TIME REGULATION WITH LOOKAHEAD = %G seconds.\n",
                          __LINE__, get_federation_name(), lookahead.get_time_in_seconds() );
      }

      // RTI_amb->enableTimeRegulation() is an implicit
      // RTI_amb->timeAdvanceRequest() so clear the flags since we will get a
      // FedAmb::timeRegulationEnabled() callback which will set the
      // time-adv state and time_regulating_state flags to true/granted.

      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

         this->time_adv_state        = TIME_ADVANCE_RESET;
         this->time_regulating_state = false;
      }

      // Turn on regulating status so that constrained federates will be
      // controlled by our time.
      //
      // If we are regulating and our object attributes and interaction
      // parameters are specified with timestamp in the FOM we will send
      // TimeStamp Ordered messages.
      RTI_ambassador->enableTimeRegulation( lookahead.get() );

      int64_t      wallclock_time;
      SleepTimeout print_timer( this->wait_status_time );
      SleepTimeout sleep_timer;

      // This spin lock waits for the time regulation flag to be set from the RTI.
      while ( !this->time_regulating_state ) {

         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( !this->time_regulating_state ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Federate::setup_time_regulation():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "Federate::setup_time_regulation():%d \"%s\": ENABLING TIME REGULATION WITH LOOKAHEAD = %G seconds, waiting...\n",
                                __LINE__, get_federation_name(), lookahead.get_time_in_seconds() );
            }
         }
      }

   } catch ( RTI1516_NAMESPACE::TimeRegulationAlreadyEnabled const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->time_regulating_state = true;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": Time Regulation Already Enabled: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::InvalidLookahead const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": ERROR: InvalidLookahead: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::InTimeAdvancingState const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": ERROR: InTimeAdvancingState: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RequestForTimeRegulationPending const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": ERROR: RequestForTimeRegulationPending: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": ERROR: FederateNotExecutionMember: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": ERROR: SaveInProgress: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": ERROR: RestoreInProgress: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": ERROR: NotConnected : '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": ERROR: RTIinternalError: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::setup_time_regulation():%d \"%s\": Unexpected RTI exception!\nRTI Exception: RTIinternalError: '%s'\n",
                       __LINE__, get_federation_name(), rti_err_msg.c_str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void Federate::time_advance_request()
{
   // Skip requesting time-advancement if we are not time-regulating and
   // not time-constrained (i.e. not using time management).
   if ( !this->time_management ) {
      return;
   }

   // Do not ask for a time advance on an initialization pass.
   if ( exec_get_mode() == Initialization ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::time_advance_request():%d exec_init_pass() == true so returning.\n",
                          __LINE__ );
      }
      return;
   }

   // -- start of checkpoint additions --
   this->save_completed = false; // reset ONLY at the bottom of the frame...
   // -- end of checkpoint additions --

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      // Build the requested HLA logical time for the next time step.
      if ( is_zero_lookahead_time() ) {
         // Use the TAR job cycle time for the time-step.
         this->requested_time += this->HLA_cycle_time_in_base_time;
      } else {
         // Use the lookahead time for the time-step.
         // Requested time = granted time + lookahead
         this->requested_time += this->lookahead;
      }
   }

   // Perform the time-advance request to go to the requested time.
   perform_time_advance_request();
}

/*!
 * @job_class{scheduled}
 */
void Federate::perform_time_advance_request()
{
   // -- start of checkpoint additions --
   this->save_completed = false; // reset ONLY at the bottom of the frame...
   // -- end of checkpoint additions --

   // Skip requesting time-advancement if we are not time-regulating and
   // not time-constrained (i.e. not using time management).
   if ( !this->time_management ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      if ( is_zero_lookahead_time() ) {
         message_publish( MSG_NORMAL, "Federate::perform_time_advance_request():%d Time Advance Request Available (TARA) to %.12G seconds.\n",
                          __LINE__, requested_time.get_time_in_seconds() );
      } else {
         message_publish( MSG_NORMAL, "Federate::perform_time_advance_request():%d Time Advance Request (TAR) to %.12G seconds.\n",
                          __LINE__, requested_time.get_time_in_seconds() );
      }
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      if ( this->time_adv_state == TIME_ADVANCE_REQUESTED ) {
         message_publish( MSG_NORMAL, "Federate::perform_time_advance_request():%d WARNING: Already in time requested state!\n",
                          __LINE__ );
      }

      // Clear the TAR flag before we make our request.
      this->time_adv_state = TIME_ADVANCE_RESET;

      try {
         if ( is_zero_lookahead_time() ) {
            // Request that time be advanced to the new time, but still allow
            // TSO data for Treq = Tgrant
            RTI_ambassador->timeAdvanceRequestAvailable( requested_time.get() );
         } else {
            // Request that time be advanced to the new time.
            RTI_ambassador->timeAdvanceRequest( requested_time.get() );
         }

         // Indicate we issued a TAR since we successfully made the request
         // without an exception.
         this->time_adv_state = TIME_ADVANCE_REQUESTED;

      } catch ( InvalidLogicalTime const &e ) {
         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d EXCEPTION: InvalidLogicalTime\n",
                          __LINE__ );
      } catch ( LogicalTimeAlreadyPassed const &e ) {
         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d EXCEPTION: LogicalTimeAlreadyPassed\n",
                          __LINE__ );
      } catch ( InTimeAdvancingState const &e ) {
         // A time advance request is still being processed by the RTI so show
         // a message and treat this as a successful time advance request.
         //
         // Indicate we are in the time advance requested state.
         this->time_adv_state = TIME_ADVANCE_REQUESTED;

         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d WARNING: Ignoring InTimeAdvancingState HLA Exception.\n",
                          __LINE__ );
      } catch ( RequestForTimeRegulationPending const &e ) {
         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d EXCEPTION: RequestForTimeRegulationPending\n",
                          __LINE__ );
      } catch ( RequestForTimeConstrainedPending const &e ) {
         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d EXCEPTION: RequestForTimeConstrainedPending\n",
                          __LINE__ );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d EXCEPTION: FederateNotExecutionMember\n",
                          __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d EXCEPTION: SaveInProgress\n",
                          __LINE__ );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d EXCEPTION: RestoreInProgress\n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d EXCEPTION: NotConnected\n",
                          __LINE__ );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Federate::perform_time_advance_request():%d \"%s\": Unexpected RTI exception!\n RTI Exception: RTIinternalError: '%s'\n",
                          __LINE__, get_federation_name(), rti_err_msg.c_str() );
      }
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void Federate::wait_for_zero_lookahead_TARA_TAG()
{
   // Skip requesting time-advancement if we are not time-regulating and
   // not time-constrained (i.e. not using time management).
   if ( !this->time_management ) {
      return;
   }

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      if ( this->time_adv_state == TIME_ADVANCE_REQUESTED ) {
         message_publish( MSG_NORMAL, "Federate::wait_for_zero_lookahead_TARA_TAG():%d WARNING: Already in time requested state!\n",
                          __LINE__ );
      } else {

         // Clear the TAR flag before we make our request.
         this->time_adv_state = TIME_ADVANCE_RESET;

         // Macro to save the FPU Control Word register value.
         TRICKHLA_SAVE_FPU_CONTROL_WORD;

         // Time Advance Request Available (TARA)
         try {
            // Request that time be advanced to the new time, but still allow
            // TSO data for Treq = Tgrant
            RTI_ambassador->timeAdvanceRequestAvailable( requested_time.get() );

            // Indicate we issued a TAR since we successfully made the request
            // without an exception.
            this->time_adv_state = TIME_ADVANCE_REQUESTED;

         } catch ( InvalidLogicalTime const &e ) {
            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: InvalidLogicalTime\n",
                             __LINE__ );
         } catch ( LogicalTimeAlreadyPassed const &e ) {
            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: LogicalTimeAlreadyPassed\n",
                             __LINE__ );
         } catch ( InTimeAdvancingState const &e ) {
            // A time advance request is still being processed by the RTI so show
            // a message and treat this as a successful time advance request.
            //
            // Indicate we are in the time advance requested state.
            this->time_adv_state = TIME_ADVANCE_REQUESTED;

            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d WARNING: Ignoring InTimeAdvancingState HLA Exception.\n",
                             __LINE__ );
         } catch ( RequestForTimeRegulationPending const &e ) {
            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: RequestForTimeRegulationPending\n",
                             __LINE__ );
         } catch ( RequestForTimeConstrainedPending const &e ) {
            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: RequestForTimeConstrainedPending\n",
                             __LINE__ );
         } catch ( FederateNotExecutionMember const &e ) {
            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: FederateNotExecutionMember\n",
                             __LINE__ );
         } catch ( SaveInProgress const &e ) {
            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: SaveInProgress\n",
                             __LINE__ );
         } catch ( RestoreInProgress const &e ) {
            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: RestoreInProgress\n",
                             __LINE__ );
         } catch ( NotConnected const &e ) {
            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d EXCEPTION: NotConnected\n",
                             __LINE__ );
         } catch ( RTIinternalError const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Federate::wait_for_zero_lookahead_TARA_TAG():%d \"%s\": Unexpected RTI exception!\n RTI Exception: RTIinternalError: '%s'\n",
                             __LINE__, get_federation_name(), rti_err_msg.c_str() );
         }

         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         // We had an error if we are not in the time advance requested state.
         if ( this->time_adv_state != TIME_ADVANCE_REQUESTED ) {
            if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
               message_publish( MSG_NORMAL, "Federate::wait_for_zero_lookahead_TARA_TAG():%d WARNING: No Time Advance Request Available call made!\n",
                                __LINE__ );
            }
            return;
         }
      }
   }

   unsigned short state;
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
      state = this->time_adv_state;
   }

   // Wait for Time Advance Grant (TAG)
   if ( state != TIME_ADVANCE_GRANTED ) {

      int64_t      wallclock_time;
      SleepTimeout print_timer( this->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // This spin lock waits for the time advance grant from the RTI.
      do {
         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            // When auto_unlock_mutex goes out of scope it automatically unlocks
            // the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
            state = this->time_adv_state;
         }

         if ( state != TIME_ADVANCE_GRANTED ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Federate::wait_for_zero_lookahead_TARA_TAG():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "Federate::wait_for_zero_lookahead_TARA_TAG():%d Waiting...\n",
                                __LINE__ );
            }
         }
      } while ( state != TIME_ADVANCE_GRANTED );
   }
}

/*!
 * @brief Associate a Trick child thread with TrickHLA.
 */
void Federate::associate_to_trick_child_thread(
   unsigned int const thread_id,
   double const       data_cycle )
{
   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::associate_to_trick_child_thread():%d Trick child thread (id:%d, data_cycle:%.3f).\n",
                       __LINE__, thread_id, data_cycle );
   }

   // Delegate to the Trick child thread coordinator.
   thread_coordinator.associate_to_trick_child_thread( thread_id, data_cycle );
}

/*!
 * @brief Disable the comma separated list of Trick child thread IDs associated to TrickHLA.
 */
void Federate::disable_trick_child_thread_associations(
   char const *thread_ids )
{
   // Delegate to the Trick child thread coordinator.
   thread_coordinator.disable_trick_thread_associations( thread_ids );
}

/*!
 * @brief Verify the thread IDs associated to the objects.
 */
void Federate::verify_trick_child_thread_associations()
{
   // Delegate to the Trick thread coordinator.
   thread_coordinator.verify_trick_thread_associations();
}

/*
 * @brief Verify the time constraints (i.e. Lookahead, LCTS, RT and dt).
 */
bool const Federate::verify_time_constraints()
{
   return thread_coordinator.verify_time_constraints();
}

/*!
 * @brief Announce all the HLA data was sent.
 */
void Federate::announce_data_available()
{
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::announce_data_available():%d Thread:%d\n",
                       __LINE__, exec_get_process_id() );
   }

   // Delegate to the Trick child thread coordinator.
   thread_coordinator.announce_data_available();
}

/*!
 * @brief Announce all the HLA data was sent.
 */
void Federate::announce_data_sent()
{
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::announce_data_sent():%d Thread:%d Granted HLA-time:%.12G seconds.\n",
                       __LINE__, exec_get_process_id(), granted_time.get_time_in_seconds() );
   }

   // Delegate to the Trick child thread coordinator.
   thread_coordinator.announce_data_sent();
}

/*!
 * @brief Wait for the HLA data to be sent if a Trick child thread or if the
 * calling thread is the Trick main thread then wait for all associated Trick
 * child threads to have called this function.
 */
void Federate::wait_to_send_data()
{
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_to_send_data():%d Thread:%d\n",
                       __LINE__, exec_get_process_id() );
   }

   // Delegate to the Trick child thread coordinator.
   thread_coordinator.wait_to_send_data();
}

/*! @brief Wait to receive data when the Trick main thread is ready. */
void Federate::wait_to_receive_data()
{
   if ( DebugHandler::show( DEBUG_LEVEL_6_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_to_receive_data():%d Thread:%d\n",
                       __LINE__, exec_get_process_id() );
   }

   // Delegate to the Trick child thread coordinator.
   thread_coordinator.wait_to_receive_data();
}

/*! @brief Get the data cycle time for the configured object index or return
 * the default data cycle time otherwise. */
int64_t const Federate::get_data_cycle_base_time_for_obj(
   unsigned int const obj_index,
   int64_t const      default_data_cycle_base_time ) const
{
   // Delegate to the Trick child thread coordinator.
   return thread_coordinator.get_data_cycle_base_time_for_obj( obj_index, default_data_cycle_base_time );
}

/*! @brief Is the object for the given index on a data cycle boundary. */
bool const Federate::on_data_cycle_boundary_for_obj(
   unsigned int const obj_index,
   int64_t const      sim_time_in_base_time ) const
{
   // Delegate to the Trick child thread coordinator.
   return thread_coordinator.on_receive_data_cycle_boundary_for_obj( obj_index, sim_time_in_base_time );
}

/*!
 * @brief Send zero lookahead or requested data for the specified object instance.
 * @param obj_instance_name Object instance name to send data for.
 */
void Federate::send_zero_lookahead_and_requested_data(
   string const &obj_instance_name )
{
   TrickHLA::Object *obj = manager->get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::send_zero_lookahead_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // We can only send zero-lookahead attribute updates for the attributes we
   // own and are configured to publish.
   if ( !obj->any_locally_owned_published_zero_lookahead_or_requested_attribute() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::send_zero_lookahead_data():%d Object:'%s'\n",
                       __LINE__, obj_instance_name.c_str() );
   }

   obj->send_zero_lookahead_and_requested_data( this->granted_time );
}

/*!
 *  @brief Blocking function call to wait to receive the zero lookahead data
 *  for the specified object instance.
 *  @param obj_instance_name Object instance name to wait for data.
 */
void Federate::wait_to_receive_zero_lookahead_data(
   string const &obj_instance_name )
{
   TrickHLA::Object *obj = manager->get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::wait_to_receive_zero_lookahead_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // We can only receive data if we subscribe to at least one attribute that
   // is remotely owned, otherwise just return.
   if ( !obj->any_remotely_owned_subscribed_zero_lookahead_attribute() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_to_receive_zero_lookahead_data():%d Object:'%s'\n",
                       __LINE__, obj_instance_name.c_str() );
   }

   // See if we already have data without the overhead of calling TARA/TAG. This
   // is most likely the case if multiple data sends happen at the same time and
   // subsequent calls to wait_to_receive_zero_lookahead_data() will have data
   // for other objects.
   if ( !obj->is_changed() && obj->any_remotely_owned_subscribed_zero_lookahead_attribute() ) {

      // The TARA will cause zero-lookahead data to be reflected before the TAG.
      wait_for_zero_lookahead_TARA_TAG();

      int64_t      wallclock_time; // cppcheck-suppress [variableScope]
      SleepTimeout print_timer( this->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Block waiting for the named object instance data by repeatedly doing a
      // TARA and wait for TAG with a zero lookahead.
      while ( !obj->is_changed() && obj->any_remotely_owned_subscribed_zero_lookahead_attribute() ) {

         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_to_receive_zero_lookahead_data():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution"
                      << " member. This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "Federate::wait_to_receive_zero_lookahead_data():%d Waiting...\n",
                             __LINE__ );
         }

         // The TARA will cause zero-lookahead data to be reflected before the TAG.
         wait_for_zero_lookahead_TARA_TAG();
      }
   }

   obj->receive_zero_lookahead_data();
}

/*!
 * @brief Send blocking I/O or requested data for the specified object instance.
 * @param obj_instance_name Object instance name to send data for. */
void Federate::send_blocking_io_data(
   string const &obj_instance_name )
{
   TrickHLA::Object *obj = manager->get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::send_blocking_io_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // We can only send blocking I/O attribute updates for the attributes we
   // own and are configured to publish.
   if ( !obj->any_locally_owned_published_blocking_io_attribute() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::send_blocking_io_data():%d Object:'%s'\n",
                       __LINE__, obj_instance_name.c_str() );
   }

   obj->send_blocking_io_data();
}

/*!
 * @brief Blocking function call to wait to receive the blocking I/O data
 * for the specified object instance.
 * @param obj_instance_name Object instance name to wait for data.
 */
void Federate::wait_to_receive_blocking_io_data(
   string const &obj_instance_name )
{
   TrickHLA::Object *obj = manager->get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::wait_to_receive_blocking_io_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // We can only receive data if we subscribe to at least one attribute that
   // is remotely owned, otherwise just return.
   if ( !obj->any_remotely_owned_subscribed_blocking_io_attribute() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_to_receive_blocking_io_data():%d Object:'%s'\n",
                       __LINE__, obj_instance_name.c_str() );
   }

   // See if we already have data. This is most likely the case if multiple data
   // sends happen at the same time and subsequent calls to
   // wait_to_receive_blocking_io_data() will have data for other objects.
   if ( !obj->is_changed() && obj->any_remotely_owned_subscribed_blocking_io_attribute() ) {

      int64_t      wallclock_time; // cppcheck-suppress [variableScope]
      SleepTimeout print_timer( this->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // Block waiting for the named object instance data.
      while ( !obj->is_changed() && obj->any_remotely_owned_subscribed_blocking_io_attribute() ) {

         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_to_receive_blocking_io_data():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution"
                      << " member. This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "Federate::wait_to_receive_blocking_io_data():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }

   obj->receive_blocking_io_data();
}

/*!
 *  @job_class{scheduled}
 */
void Federate::wait_for_time_advance_grant()
{
   // Skip requesting time-advancement if time management is not enabled.
   if ( !this->time_management ) {
      return;
   }

   // Do not ask for a time advance on an initialization pass.
   if ( exec_get_mode() == Initialization ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::wait_for_time_advance_grant():%d In Initialization mode so returning.\n",
                          __LINE__ );
      }
      return;
   }

   unsigned short state;
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
      state = this->time_adv_state;
   }

   if ( state == TIME_ADVANCE_RESET ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::wait_for_time_advance_grant():%d WARNING: No Time Advance Requested!\n",
                          __LINE__ );
      }
      return;
   }

   if ( state != TIME_ADVANCE_GRANTED ) {

      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::wait_for_time_advance_grant():%d Waiting for Time Advance Grant (TAG) to %.12G seconds.\n",
                          __LINE__, requested_time.get_time_in_seconds() );
      }

      int64_t      wallclock_time;
      SleepTimeout print_timer( this->wait_status_time );
      SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

      // This spin lock waits for the time advance grant from the RTI.
      do {
         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            // When auto_unlock_mutex goes out of scope it automatically unlocks
            // the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &time_adv_state_mutex );
            state = this->time_adv_state;
         }

         if ( state != TIME_ADVANCE_GRANTED ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Federate::wait_for_time_advance_grant():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "Federate::wait_for_time_advance_grant():%d Waiting...\n",
                                __LINE__ );
            }
         }
      } while ( state != TIME_ADVANCE_GRANTED );
   }

   // Add the line number for a higher trace level.
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_time_advance_grant():%d Time Advance Grant (TAG) to %.12G seconds.\n",
                       __LINE__, granted_time.get_time_in_seconds() );
   }
}

/*!
 *  @job_class{scheduled}
 */
bool Federate::is_execution_member()
{
   if ( RTI_ambassador.get() != NULL ) {
      bool is_exec_member = true;
      try {
         RTI_ambassador->getOrderName( RTI1516_NAMESPACE::TIMESTAMP );
      } catch ( RTI1516_NAMESPACE::InvalidOrderType const &e ) {
         // Do nothing
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         is_exec_member = false;
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         is_exec_member = false;
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         // Do nothing
      }
      return is_exec_member;
   }
   return false;
}

/*!
 *  @details Shutdown the federate by shutting down the time management,
 *  resigning from the federation, and then attempt to destroy the federation.
 *  @job_class{shutdown}
 */
void Federate::shutdown()
{
   // Guard against doing a shutdown more than once.
   if ( !is_shutdown_called() ) {
      this->shutdown_called = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::shutdown():%d \n", __LINE__ );
      }

#ifdef THLA_CHECK_SEND_AND_RECEIVE_COUNTS
      for ( int i = 0; i < manager->obj_count; ++i ) {
         ostringstream msg;
         msg << "Federate::shutdown():" << __LINE__
             << " Object[" << i << "]:'" << manager->objects[i].get_name() << "'"
             << " send_count:" << manager->objects[i].send_count
             << " receive_count:" << manager->objects[i].receive_count
             << endl;
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
#endif

#ifdef THLA_CYCLIC_READ_TIME_STATS
      for ( int i = 0; i < manager->obj_count; ++i ) {
         ostringstream msg;
         msg << "Federate::shutdown():" << __LINE__
             << " Object[" << i << "]:'" << manager->objects[i].get_name() << "' "
             << manager->objects[i].elapsed_time_stats.to_string()
             << endl;
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
#endif

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Check for Execution Control shutdown. If this is NULL, then we are
      // probably shutting down prior to initialization.
      if ( this->execution_control != NULL ) {
         // Call Execution Control shutdown method.
         execution_control->shutdown();
      }

      // Disable Time Constrained and Time Regulation for this federate.
      shutdown_time_management();

      // Resign from the federation.
      // If the federate can rejoin, resign in a way so we can rejoin later...
      if ( this->can_rejoin_federation ) {
         resign_so_we_can_rejoin();
      } else {
         resign();
      }

      // Attempt to destroy the federation.
      destroy();

      // Remove the ExecutionConfiguration object.
      if ( this->execution_control != NULL ) {
         execution_control->remove_execution_configuration();
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;

#if defined( FPU_CW_PROTECTION ) && ( defined( __i386__ ) || defined( __x86_64__ ) )
      // As the last thing we do, check to see if we did a good job of
      // protecting against FPU control-word precision-control changes by
      // comparing the current precision-control value to the one at program
      // startup (__fpu_control is automatically set for us, and the _fpu_cw
      // variable comes from the TRICKHLA_SAVE_FPU_CONTROL_WORD macro). Print
      // a warning message if they are different. Only support the Intel CPU's.
      // NOTE: Don't use the TRICKHLA_VALIDATE_FPU_CONTROL_WORD because it can
      // be disabled in the TrickHLA compile-config header file.
      if ( ( _fpu_cw & _FPU_PC_MASK ) != ( __fpu_control & _FPU_PC_MASK ) ) {
         message_publish( MSG_WARNING, "%s:%d WARNING: We have detected that the current \
Floating-Point Unit (FPU) Control-Word Precision-Control value (%#x: %s) does \
not match the Precision-Control value at program startup (%#x: %s). The change \
in FPU Control-Word Precision-Control could cause the numerical values in your \
simulation to be slightly different in the 7th or 8th decimal place. Please \
contact the TrickHLA team for support.\n",
                          __FILE__, __LINE__,
                          ( _fpu_cw & _FPU_PC_MASK ), _FPU_PC_PRINT( _fpu_cw ),
                          ( __fpu_control & _FPU_PC_MASK ), _FPU_PC_PRINT( __fpu_control ) );
      }
#endif
   }
}

/*!
 *  @details Shutdown this federate's time management by shutting down time
 *  constraint management and time regulating management.
 *  @job_class{shutdown}
 */
void Federate::shutdown_time_management()
{
   shutdown_time_constrained();
   shutdown_time_regulating();
}

/*!
 *  @job_class{shutdown}
 */
void Federate::shutdown_time_constrained()
{
   if ( !this->time_constrained_state ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::shutdown_time_constrained():%d HLA Time Constrained Already Disabled.\n",
                          __LINE__ );
      }
   } else {
      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Make sure we've been able to get the RTI ambassador.
      if ( RTI_ambassador.get() == NULL ) {
         return;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::shutdown_time_constrained():%d Disabling HLA Time Constrained.\n",
                          __LINE__ );
      }

      try {
         RTI_ambassador->disableTimeConstrained();
         this->time_constrained_state = false;
      } catch ( RTI1516_NAMESPACE::TimeConstrainedIsNotEnabled const &e ) {
         this->time_constrained_state = false;
         message_publish( MSG_WARNING, "Federate::shutdown_time_constrained():%d \"%s\": TimeConstrainedIsNotEnabled EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         this->time_constrained_state = false;
         message_publish( MSG_WARNING, "Federate::shutdown_time_constrained():%d \"%s\": FederateNotExecutionMember EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::shutdown_time_constrained():%d \"%s\": SaveInProgress EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::shutdown_time_constrained():%d \"%s\": RestoreInProgress EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         this->time_constrained_state = false;
         message_publish( MSG_WARNING, "Federate::shutdown_time_constrained():%d \"%s\": NotConnected EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Federate::shutdown_time_constrained():%d \"%s\": RTIinternalError EXCEPTION: '%s'\n",
                          __LINE__, get_federation_name(), rti_err_msg.c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         message_publish( MSG_WARNING, "Federate::shutdown_time_constrained():%d \"%s\": Unexpected RTI EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

/*!
 *  @job_class{shutdown}
 */
void Federate::shutdown_time_regulating()
{
   if ( !this->time_regulating_state ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::shutdown_time_regulating():%d HLA Time Regulation Already Disabled.\n",
                          __LINE__ );
      }
   } else {
      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Make sure we've been able to get the RTI ambassador.
      if ( RTI_ambassador.get() == NULL ) {
         return;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::shutdown_time_regulating():%d Disabling HLA Time Regulation.\n",
                          __LINE__ );
      }

      try {
         RTI_ambassador->disableTimeRegulation();
         this->time_regulating_state = false;
      } catch ( RTI1516_NAMESPACE::TimeConstrainedIsNotEnabled const &e ) {
         this->time_regulating_state = false;
         message_publish( MSG_WARNING, "Federate::shutdown_time_regulating():%d \"%s\": TimeConstrainedIsNotEnabled EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         this->time_regulating_state = false;
         message_publish( MSG_WARNING, "Federate::shutdown_time_regulating():%d \"%s\": FederateNotExecutionMember EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::shutdown_time_regulating():%d \"%s\": SaveInProgress EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::shutdown_time_regulating():%d \"%s\": RestoreInProgress EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         this->time_constrained_state = false;
         message_publish( MSG_WARNING, "Federate::shutdown_time_regulating():%d \"%s\": NotConnected EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Federate::shutdown_time_regulating():%d \"%s\": RTIinternalError EXCEPTION: '%s'\n",
                          __LINE__, get_federation_name(), rti_err_msg.c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         message_publish( MSG_WARNING, "Federate::shutdown_time_regulating():%d \"%s\": Unexpected RTI EXCEPTION!\n",
                          __LINE__, get_federation_name() );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

/*!
 *  @job_class{shutdown}
 */
void Federate::resign()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Make sure we've been able to set the RTI ambassador.
   if ( RTI_ambassador.get() == NULL ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      return;
   }
   // Resign from the federation execution to remove this federate from
   // participation. The flag provided will instruct the RTI to call
   // deleteObjectInstance for all objects this federate has the
   // privilegeToDelete for (which by default is all objects that this
   // federate registered) and to release ownership of any attributes that
   // this federate owns but does not own the privilegeToDelete for.
   try {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::resign():%d Attempting to resign from Federation '%s'\n",
                          __LINE__, get_federation_name() );
      }

      if ( is_execution_member() ) {
         RTI_ambassador->resignFederationExecution( RTI1516_NAMESPACE::CANCEL_THEN_DELETE_THEN_DIVEST );

         this->federation_joined = false;

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::resign():%d Resigned from Federation '%s'\n",
                             __LINE__, get_federation_name() );
         }
      } else {
         message_publish( MSG_NORMAL, "Federate::resign():%d Not execution member of Federation '%s'\n",
                          __LINE__, get_federation_name() );
      }
   } catch ( InvalidResignAction const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "InvalidResignAction\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( OwnershipAcquisitionPending const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "OwnershipAcquisitionPending\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( FederateOwnsAttributes const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "FederateOwnsAttributes";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->federation_joined = false;

      ostringstream errmsg;
      errmsg << "Federate::resign():" << __LINE__
             << " Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "FederateNotExecutionMember\n";

      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->federation_joined = false;

      ostringstream errmsg;
      errmsg << "Federate::resign():" << __LINE__
             << " Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "NotConnected\n";

      // Just display an error message and don't terminate if we are not connected.
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( CallNotAllowedFromWithinCallback const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "CallNotAllowedFromWithinCallback\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::resign():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because of the RTIinternalError: "
             << rti_err_msg << '\n';

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::resign():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because of the RTI Exception: "
             << rti_err_msg << '\n';

      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 *  @details Resign from the federation but divest ownership of my attributes
 *  and do not delete the federate from the federation when resigning.
 *  @job_class{logging}
 */
void Federate::resign_so_we_can_rejoin()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Make sure we've been able to set the RTI ambassador.
   if ( RTI_ambassador.get() == NULL ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      return;
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::resign_so_we_can_rejoin():%d \
Federation \"%s\": RESIGNING FROM FEDERATION (with the ability to rejoin federation)\n",
                          __LINE__, get_federation_name() );
      }

      RTI_ambassador->resignFederationExecution( RTI1516_NAMESPACE::UNCONDITIONALLY_DIVEST_ATTRIBUTES );

      this->federation_joined = false;

   } catch ( InvalidResignAction const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "InvalidResignAction\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( OwnershipAcquisitionPending const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "OwnershipAcquisitionPending\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( FederateOwnsAttributes const &e ) {
      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation received an EXCEPTION: "
             << "FederateOwnsAttributes\n";

      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "FederateNotExecutionMember\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "NotConnected\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( CallNotAllowedFromWithinCallback const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "CallNotAllowedFromWithinCallback\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because of the RTIinternalError: "
             << rti_err_msg << '\n';

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because of the RTI Exception: "
             << rti_err_msg << '\n';

      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // TODO: Do we really want to terminate here! DDexter 9/27/2010
   ostringstream errmsg;
   errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
          << " ERROR: Federate '" << get_federate_name()
          << "' resigned from Federation '" << get_federation_name() << "'\n";
   DebugHandler::terminate_with_message( errmsg.str() );
}

/*!
 *  @job_class{shutdown}
 */
void Federate::destroy()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Destroy the federation execution in case we are the last federate. This
   // will not do anything bad if there other federates joined. The RTI will
   // throw us an exception telling us that other federates are joined and we
   // can just ignore that.
   if ( RTI_ambassador.get() == NULL ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      return;
   }

   // Create the wide-string version of the federation name.
   wstring federation_name_ws;
   StringUtilities::to_wstring( federation_name_ws, federation_name );

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy():%d Attempting to Destroy Federation '%s'\n",
                          __LINE__, get_federation_name() );
      }

      RTI_ambassador->destroyFederationExecution( federation_name_ws );

      this->federation_exists = false;
      this->federation_joined = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy():%d Destroyed Federation '%s'\n",
                          __LINE__, get_federation_name() );
      }
   } catch ( RTI1516_NAMESPACE::FederatesCurrentlyJoined const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->federation_joined = false;

      // Put this warning message at a higher trace level since every
      // federate that is not the last one in the federation will see this
      // message when they try to destroy the federation. This is expected.
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy():%d Federation '%s' destroy failed because this is not the last federate, which is expected.\n",
                          __LINE__, get_federation_name() );
      }
   } catch ( RTI1516_NAMESPACE::FederationExecutionDoesNotExist const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->federation_exists = false;
      this->federation_joined = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "Federate::destroy():%d Federation '%s' Already Destroyed.\n",
                          __LINE__, get_federation_name() );
      }
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->federation_exists = false;
      this->federation_joined = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "Federate::destroy():%d Federation '%s' destroy failed because we are NOT CONNECTED to the federation.\n",
                          __LINE__, get_federation_name() );
      }
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::destroy():" << __LINE__
             << " ERROR: Federation '" << get_federation_name()
             << "': Unexpected RTI exception when destroying federation!\n"
             << "RTI Exception: RTIinternalError: '"
             << rti_err_msg << "'\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy():%d Attempting to disconnect from RTI \n",
                          __LINE__ );
      }

      RTI_ambassador->disconnect();

      this->federation_exists = false;
      this->federation_joined = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy():%d Disconnected from RTI \n",
                          __LINE__ );
      }
   } catch ( RTI1516_NAMESPACE::FederateIsExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "Federate::destroy():%d Cannot disconnect from RTI because this federate is still joined.\n",
                          __LINE__ );
      }
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::destroy():" << __LINE__
             << " ERROR: Unexpected RTI exception when disconnecting from RTI!\n"
             << "RTI Exception: RTIinternalError: '"
             << rti_err_msg << "'\n";

      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 *  @job_class{initialization}
 */
void Federate::destroy_orphaned_federation()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Print an error message if the RTI ambassador is NULL.
   if ( RTI_ambassador.get() == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::destroy_orphaned_federation():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Create the wide-string version of the federation name.
   wstring federation_name_ws;
   StringUtilities::to_wstring( federation_name_ws, federation_name );

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::destroy_orphaned_federation():%d Attempting to Destroy Orphaned Federation '%s'.\n",
                       __LINE__, get_federation_name() );
   }

   try {
      RTI_ambassador->destroyFederationExecution( federation_name_ws );

      // If we don't get an exception then we successfully destroyed
      // an orphaned federation.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy_orphaned_federation():%d Successfully Destroyed Orphaned Federation '%s'.\n",
                          __LINE__, get_federation_name() );
      }
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Ignore any exception since we are just removing an orphaned federation.
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 *  @job_class{initialization}
 */
void Federate::set_federation_name(
   string const &exec_name )
{
   // Check for a NULL current federatin name or a self assigned name.
   if ( ( this->federation_name == NULL ) || ( this->federation_name != exec_name ) ) {

      // Check for an empty (i.e. zero length) name.
      if ( !exec_name.empty() ) {

         // Reallocate and set the federation execution name.
         if ( this->federation_name != NULL ) {
            if ( trick_MM->delete_var( static_cast< void * >( this->federation_name ) ) ) {
               message_publish( MSG_WARNING, "Federate::set_federation_name():%d WARNING failed to delete Trick Memory for 'federation_name'\n",
                                __LINE__ );
            }
            this->federation_name = NULL;
         }

         // Set the federation execution name.
         this->federation_name = trick_MM->mm_strdup( const_cast< char * >( exec_name.c_str() ) );
      } else {

         // Set to a default value if not already set in the input stream.
         if ( this->federation_name == NULL ) {
            this->federation_name = trick_MM->mm_strdup( const_cast< char * >( "TrickHLA Federation" ) );
         }
      }
   }
}

void Federate::ask_MOM_for_auto_provide_setting()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::ask_MOM_for_auto_provide_setting():%d\n",
                       __LINE__ );
   }

   // Make sure the MOM handles get initialized before we try to use them.
   if ( !MOM_HLAautoProvide_handle.isValid() ) {
      initialize_MOM_handles();
   }

   // Reset the value to an unknown state so that we will know when we get the
   // actual value from the MOM.
   this->auto_provide_setting = -1;

   // Use the MOM to get the list of registered federates.
   AttributeHandleSet fedMomAttributes;
   fedMomAttributes.insert( MOM_HLAautoProvide_handle );
   subscribe_attributes( MOM_HLAfederation_class_handle, fedMomAttributes );

   AttributeHandleSet requestedAttributes;
   requestedAttributes.insert( MOM_HLAautoProvide_handle );
   request_attribute_update( MOM_HLAfederation_class_handle, requestedAttributes );

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   while ( this->auto_provide_setting < 0 ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      // Sleep a little while to wait for the information to update.
      sleep_timer.sleep();

      if ( this->auto_provide_setting < 0 ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::ask_MOM_for_auto_provide_setting():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::ask_MOM_for_auto_provide_setting():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }

   // Only unsubscribe from the attributes we subscribed to in this function.
   unsubscribe_attributes( MOM_HLAfederation_class_handle, fedMomAttributes );

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::ask_MOM_for_auto_provide_setting():%d Auto-Provide:%s value:%d\n",
                       __LINE__, ( ( auto_provide_setting != 0 ) ? "Yes" : "No" ),
                       auto_provide_setting );
   }

   fedMomAttributes.clear();
   requestedAttributes.clear();
}

void Federate::enable_MOM_auto_provide_setting(
   bool enable )
{
   // Keep the auto-provide setting in sync with our enable request and set the
   // Big Endian value the RTI expects for the auto-provide setting.
   int requested_auto_provide;
   if ( enable ) {
      this->auto_provide_setting = 1;
      // 1 as 32-bit Big Endian as required for the HLAautoProvide parameter.
      requested_auto_provide = Utilities::is_transmission_byteswap( ENCODING_BIG_ENDIAN )
                                  ? Utilities::byteswap_int( 1 )
                                  : 1;
   } else {
      this->auto_provide_setting = 0;
      requested_auto_provide     = 0;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::enable_MOM_auto_provide_setting():%d Auto-Provide:%s\n",
                       __LINE__, ( enable ? "Yes" : "No" ) );
   }

   publish_interaction_class( MOM_HLAsetSwitches_class_handle );

   ParameterHandleValueMap param_values_map;
   param_values_map[MOM_HLAautoProvide_param_handle] =
      VariableLengthData( &requested_auto_provide,
                          sizeof( requested_auto_provide ) );

   send_interaction( MOM_HLAsetSwitches_class_handle, param_values_map );

   unpublish_interaction_class( MOM_HLAsetSwitches_class_handle );
}

void Federate::backup_auto_provide_setting_from_MOM_then_disable()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::backup_auto_provide_setting_from_MOM_then_disable():%d\n",
                       __LINE__ );
   }

   ask_MOM_for_auto_provide_setting();

   // Backup the original auto-provide setting.
   this->orig_auto_provide_setting = this->auto_provide_setting;

   // Disable Auto-Provide if it is enabled.
   if ( this->auto_provide_setting != 0 ) {
      enable_MOM_auto_provide_setting( false );
   }
}

void Federate::restore_orig_MOM_auto_provide_setting()
{
   // Only update the auto-provide setting if the original setting does not
   // match the current setting.
   if ( auto_provide_setting != orig_auto_provide_setting ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::restore_orig_MOM_auto_provide_setting():%d Auto-Provide:%s\n",
                          __LINE__, ( ( orig_auto_provide_setting != 0 ) ? "Yes" : "No" ) );
      }
      enable_MOM_auto_provide_setting( orig_auto_provide_setting != 0 );
   }
}

//**************************************************************************
//**************************************************************************
//*************** START OF CHECKPOINT / RESTORE CODE ***********************
//**************************************************************************
//**************************************************************************

void Federate::load_and_print_running_federate_names()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::load_and_print_running_federate_names():%d started.\n",
                       __LINE__ );
   }

   // Make sure the MOM handles get initialized before we try to use them.
   if ( !MOM_HLAfederation_class_handle.isValid() ) {
      initialize_MOM_handles();
   }

   AttributeHandleSet fedMomAttributes;
   fedMomAttributes.insert( MOM_HLAfederatesInFederation_handle );
   subscribe_attributes( MOM_HLAfederation_class_handle, fedMomAttributes );

   AttributeHandleSet requestedAttributes;
   requestedAttributes.insert( MOM_HLAfederatesInFederation_handle );
   request_attribute_update( MOM_HLAfederation_class_handle, requestedAttributes );

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   while ( this->running_feds_count <= 0 ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      // Sleep a little while to wait for the information to update.
      sleep_timer.sleep();

      if ( this->running_feds_count <= 0 ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::load_and_print_running_federate_names():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::load_and_print_running_federate_names():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }

   // Only unsubscribe from the attributes we subscribed to in this function.
   unsubscribe_attributes( MOM_HLAfederation_class_handle, fedMomAttributes );

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::load_and_print_running_federate_names():%d \
MOM just informed us that there are %d federates currently running in the federation.\n",
                       __LINE__, running_feds_count );
   }

   ask_MOM_for_federate_names();

   int joinedFedCount = 0;

   // Wait for all the required federates to join.
   this->all_federates_joined = false;

   print_timer.reset();
   sleep_timer.reset();

   while ( !this->all_federates_joined ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      // Sleep a little while to wait for more federates to join.
      sleep_timer.sleep();

      // Determine what federates have joined only if the joined federate
      // count has changed.
      if ( joinedFedCount != (int)joined_federate_names.size() ) {
         joinedFedCount = joined_federate_names.size();

         if ( joinedFedCount >= running_feds_count ) {
            this->all_federates_joined = true;
         }
      }
      if ( !this->all_federates_joined ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::load_and_print_running_federate_names():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::load_and_print_running_federate_names():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }

   // Execute a blocking loop until the RTI responds with information for all
   // running federates
   print_timer.reset();
   sleep_timer.reset();
   while ( joined_federate_names.size() < (unsigned int)running_feds_count ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      sleep_timer.sleep();

      if ( joined_federate_names.size() < (unsigned int)running_feds_count ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::load_and_print_running_federate_names():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::load_and_print_running_federate_names():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }

   // Now, copy the new information into my data stores and restore the saved
   // information back to what is was before this routine ran (so we can get a
   // valid checkpoint).
   clear_running_feds();
   update_running_feds();

   // Print out a list of the Running Federates.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      // Build the federate summary as an output string stream.
      ostringstream summary;
      unsigned int  cnt = 0;

      summary << "Federate::load_and_print_running_federate_names():"
              << __LINE__ << "\n'running_feds' data structure contains these "
              << running_feds_count << " federates:";

      // Summarize the required federates first.
      for ( int i = 0; i < running_feds_count; ++i ) {
         ++cnt;
         summary << "\n    " << cnt
                 << ": Found running federate '"
                 << running_feds[i].name << "'";
      }
      summary << '\n';

      // Display the federate summary.
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }

   // Clear the entry since it was absorbed into running_feds...
   joined_federate_name_map.clear();

   fedMomAttributes.clear();
   requestedAttributes.clear();

   // Do not un-subscribe to this MOM data; we DO want updates as federates
   // join / resign the federation!

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::load_and_print_running_federate_names():%d Done.\n",
                       __LINE__ );
   }
}

void Federate::clear_running_feds()
{
   if ( this->running_feds != NULL ) {
      for ( int i = 0; i < running_feds_count; ++i ) {
         if ( running_feds[i].MOM_instance_name != NULL ) {
            if ( trick_MM->delete_var( static_cast< void * >( running_feds[i].MOM_instance_name ) ) ) {
               message_publish( MSG_WARNING, "Federate::clear_running_feds():%d WARNING failed to delete Trick Memory for 'running_feds[i].MOM_instance_name'\n",
                                __LINE__ );
            }
            running_feds[i].MOM_instance_name = NULL;
         }
         if ( running_feds[i].name != NULL ) {
            if ( trick_MM->delete_var( static_cast< void * >( running_feds[i].name ) ) ) {
               message_publish( MSG_WARNING, "Federate::clear_running_feds():%d WARNING failed to delete Trick Memory for 'running_feds[i].name'\n",
                                __LINE__ );
            }
            running_feds[i].name = NULL;
         }
      }
      if ( trick_MM->delete_var( static_cast< void * >( this->running_feds ) ) ) {
         message_publish( MSG_WARNING, "Federate::clear_running_feds():%d WARNING failed to delete Trick Memory for 'this->running_feds'\n",
                          __LINE__ );
      }
      this->running_feds = NULL;
   }
}

void Federate::update_running_feds()
{
   // Make a copy of the updated known feds before restoring the saved copy...
   running_feds = reinterpret_cast< KnownFederate * >(
      alloc_type( running_feds_count, "TrickHLA::KnownFederate" ) );

   if ( running_feds == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::update_running_feds():" << __LINE__
             << " ERROR: Could not allocate memory for running_feds!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( (int)joined_federate_name_map.size() != running_feds_count ) {
      // Show the contents of 'joined_federate_name_map'
      TrickHLAObjInstanceNameMap::const_iterator map_iter;
      for ( map_iter = joined_federate_name_map.begin();
            map_iter != joined_federate_name_map.end();
            ++map_iter ) {
         string fed_name_str;
         StringUtilities::to_string( fed_name_str, MOM_HLAfederate_instance_name_map[map_iter->first] );
         string obj_name_str;
         StringUtilities::to_string( obj_name_str, map_iter->second );
         message_publish( MSG_NORMAL, "Federate::update_running_feds():%d joined_federate_name_map[%s]=%s \n",
                          __LINE__, fed_name_str.c_str(), obj_name_str.c_str() );
      }

      for ( int i = 0; i < running_feds_count; ++i ) {
         message_publish( MSG_NORMAL, "Federate::update_running_feds():%d running_feds[%d]=%s \n",
                          __LINE__, i, running_feds[i].name );
      }

      // Terminate the execution since the counters are out of sync...
      ostringstream errmsg;
      errmsg << "Federate::update_running_feds():" << __LINE__
             << " FATAL_ERROR: joined_federate_name_map contains "
             << joined_federate_name_map.size()
             << " entries but running_feds_count = " << running_feds_count
             << "!!!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Loop through joined_federate_name_map to build the running_feds list
   unsigned int index = 0;

   TrickHLAObjInstanceNameMap::const_iterator map_iter;
   for ( map_iter = joined_federate_name_map.begin();
         map_iter != joined_federate_name_map.end(); ++map_iter ) {

      running_feds[index].name = StringUtilities::ip_strdup_wstring( map_iter->second.c_str() );

      running_feds[index].MOM_instance_name = StringUtilities::ip_strdup_wstring(
         MOM_HLAfederate_instance_name_map[map_iter->first].c_str() );

      // If the federate was running at the time of the checkpoint, it must be
      // a 'required' federate in the restore, regardless if it is was required
      // when the federation originally started up.
      running_feds[index].required = true;

      ++index;
   }
}

void Federate::add_a_single_entry_into_running_feds()
{
   // Allocate a new structure to absorb the original values plus the new one.
   KnownFederate *temp_feds;
   temp_feds = reinterpret_cast< KnownFederate * >(
      alloc_type( running_feds_count + 1, "TrickHLA::KnownFederate" ) );

   if ( temp_feds == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::add_a_single_entry_into_running_feds():" << __LINE__
             << " ERROR: Could not allocate memory for temp_feds when attempting to add"
             << " an entry into running_feds!\n";
      DebugHandler::terminate_with_message( errmsg.str() );

   } else {

      // copy current running_feds entries into temporary structure...
      for ( int i = 0; i < running_feds_count; ++i ) {
         temp_feds[i].MOM_instance_name = trick_MM->mm_strdup( running_feds[i].MOM_instance_name );
         temp_feds[i].name              = trick_MM->mm_strdup( running_feds[i].name );
         temp_feds[i].required          = running_feds[i].required;
      }

      TrickHLAObjInstanceNameMap::const_iterator map_iter;
      map_iter                                        = joined_federate_name_map.begin();
      temp_feds[running_feds_count].MOM_instance_name = StringUtilities::ip_strdup_wstring( MOM_HLAfederate_instance_name_map[map_iter->first].c_str() );
      temp_feds[running_feds_count].name              = StringUtilities::ip_strdup_wstring( map_iter->second.c_str() );
      temp_feds[running_feds_count].required          = true;

      // delete running_feds data structure.
      clear_running_feds();

      // assign temp_feds into running_feds
      this->running_feds = temp_feds;

      ++running_feds_count; // make the new running_feds_count size permanent
   }

#if 0
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::add_a_single_entry_into_running_feds():%d Exiting routine, here is what running_feds contains:\n",
               __LINE__, '\n');
      for ( int t = 0; t < running_feds_count; t++) {
         message_publish( MSG_NORMAL, "Federate::add_a_single_entry_into_running_feds():%d running_feds[%d].MOM_instance_name='%s'\n",
                  __LINE__, t, running_feds[t].MOM_instance_name, '\n');
         message_publish( MSG_NORMAL, "Federate::add_a_single_entry_into_running_feds():%d running_feds[%d].name='%s'\n",
                  __LINE__, t, running_feds[t].name, '\n');
         message_publish( MSG_NORMAL, "Federate::add_a_single_entry_into_running_feds():%d running_feds[%d].required=%d \n",
                  __LINE__, t, running_feds[t].required, '\n');
      }
   }
#endif
}

void Federate::add_MOM_HLAfederate_instance_id(
   ObjectInstanceHandle const &instance_hndl,
   wstring const              &instance_name )
{
   this->MOM_HLAfederate_instance_name_map[instance_hndl] = instance_name;

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string handle_str;
      StringUtilities::to_string( handle_str, instance_hndl );
      string name_str;
      StringUtilities::to_string( name_str, instance_name );

      ostringstream summary;
      summary << "Federate::add_MOM_HLAfederate_instance_id():" << __LINE__
              << " Object '" << name_str << "', with Instance Handle:"
              << handle_str << '\n';
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }
}

void Federate::remove_MOM_HLAfederate_instance_id(
   ObjectInstanceHandle const &instance_hndl )
{
   remove_federate_instance_id( instance_hndl );
   remove_MOM_HLAfederation_instance_id( instance_hndl );

   char const *tMOMName  = NULL;
   char const *tFedName  = NULL;
   bool        foundName = false;

   TrickHLAObjInstanceNameMap::iterator iter = MOM_HLAfederate_instance_name_map.find( instance_hndl );
   if ( iter != MOM_HLAfederate_instance_name_map.end() ) {
      tMOMName  = StringUtilities::ip_strdup_wstring( iter->second.c_str() );
      foundName = true;
      MOM_HLAfederate_instance_name_map.erase( iter );

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, instance_hndl );

         ostringstream summary;
         summary << "Federate::remove_MOM_HLAfederate_instance_id():" << __LINE__
                 << " Object '" << tMOMName << "', with Instance Handle:"
                 << handle_str << '\n';
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }
   }

   // If the federate_id was not found, there is nothing else to do so exit the routine...
   if ( !foundName ) {
      return;
   }

   // Search for the federate information from running_feds...
   foundName = false;
   for ( int i = 0; i < running_feds_count; ++i ) {
      if ( !strcmp( running_feds[i].MOM_instance_name, tMOMName ) ) {
         foundName = true;
         tFedName  = trick_MM->mm_strdup( running_feds[i].name );
      }
   }

   // if the name was not found, there is nothing else to do so exit the routine...
   if ( !foundName ) {
      return;
   }

   // otherwise, the name was found. it needs to be deleted from the list of running_feds.
   // since the memory is Trick-controlled and not random access, the only way to delete
   // it is to copy the whole element list omitting the requested name...
   KnownFederate *tmp_feds;

   // allocate temporary list...
   tmp_feds = reinterpret_cast< KnownFederate * >(
      alloc_type( this->running_feds_count - 1, "TrickHLA::KnownFederate" ) );
   if ( tmp_feds == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::remove_MOM_HLAfederate_instance_id():" << __LINE__
             << " ERROR: Could not allocate memory for tmp_feds!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // now, copy everything minus the requested name from the original list...
   int tmp_feds_cnt = 0;
   for ( int i = 0; i < this->running_feds_count; ++i ) {
      // if the name is not the one we are looking for...
      if ( strcmp( running_feds[i].name, tFedName ) ) {
         if ( running_feds[i].MOM_instance_name != NULL ) {
            tmp_feds[tmp_feds_cnt].MOM_instance_name = trick_MM->mm_strdup( running_feds[i].MOM_instance_name );
         }
         tmp_feds[tmp_feds_cnt].name     = trick_MM->mm_strdup( running_feds[i].name );
         tmp_feds[tmp_feds_cnt].required = running_feds[i].required;
         ++tmp_feds_cnt;
      }
   }

   // now, clear out the original memory...
   clear_running_feds();

   // assign the new element count into running_feds_count.
   this->running_feds_count = tmp_feds_cnt;

   // assign pointer from the temporary list to the permanent list...
   this->running_feds = tmp_feds;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_hndl );
      message_publish( MSG_INFO, "Federate::remove_MOM_HLAfederate_instance_id():%d \
Removed Federate '%s' Instance-ID:%s Valid-ID:%s \n",
                       __LINE__, tFedName, id_str.c_str(),
                       ( instance_hndl.isValid() ? "Yes" : "No" ) );
   }
}

void Federate::add_MOM_HLAfederation_instance_id(
   ObjectInstanceHandle const &instance_hndl )
{
   string id_str;
   StringUtilities::to_string( id_str, instance_hndl );
   wstring id_ws;
   StringUtilities::to_wstring( id_ws, id_str );
   MOM_HLAfederation_instance_name_map[instance_hndl] = id_ws;

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream summary;
      summary << "Federate::add_MOM_HLAfederation_instance_id():" << __LINE__
              << " Object Instance:" << id_str << '\n';
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }
}

void Federate::remove_MOM_HLAfederation_instance_id(
   ObjectInstanceHandle const &instance_hndl )
{
   TrickHLAObjInstanceNameMap::iterator iter;
   iter = MOM_HLAfederation_instance_name_map.find( instance_hndl );

   if ( iter != MOM_HLAfederation_instance_name_map.end() ) {
      MOM_HLAfederation_instance_name_map.erase( iter );

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, instance_hndl );

         ostringstream summary;
         summary << "Federate::remove_MOM_HLAfederation_instance_id():" << __LINE__
                 << " Object Instance:" << handle_str << '\n';
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }
   }
}

void Federate::write_running_feds_file(
   string const &file_name )
{
   string   full_path;
   ofstream file;

   full_path = this->HLA_save_directory + "/" + file_name + ".running_feds";
   file.open( full_path.c_str(), ios::out );
   if ( file.is_open() ) {
      file << this->running_feds_count << '\n';

      // echo the contents of running_feds into file...
      for ( int i = 0; i < this->running_feds_count; ++i ) {
         file << trick_MM->mm_strdup( running_feds[i].MOM_instance_name ) << '\n';
         file << trick_MM->mm_strdup( running_feds[i].name ) << '\n';
         file << running_feds[i].required << '\n';
      }

      file.close(); // close the file.

   } else {
      ostringstream errmsg;
      errmsg << "Federate::write_running_feds_file():" << __LINE__
             << " ERROR: Failed to open file '" << full_path << "' for writing!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*!
 *  @job_class{freeze}
 */
void Federate::request_federation_save()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_HLA_save_and_restore_supported() ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string name_str;
         StringUtilities::to_string( name_str, this->save_name );
         message_publish( MSG_NORMAL, "Federate::request_federation_save():%d save_name:%s \n",
                          __LINE__, name_str.c_str() );
      }
      RTI_ambassador->requestFederationSave( this->save_name );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_save():%d EXCEPTION: FederateNotExecutionMember \n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_save():%d EXCEPTION: SaveInProgress \n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_save():%d EXCEPTION: RestoreInProgress \n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_save():%d EXCEPTION: NotConnected \n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "Federate::request_federation_save():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::restore_checkpoint(
   string const &file_name )
{
   string trick_filename = file_name;
   // DANNY2.7 prepend federation name to the filename (if it's not already prepended)
   string federation_name_str = string( get_federation_name() );
   if ( trick_filename.compare( 0, federation_name_str.length(), federation_name_str ) != 0 ) {
      trick_filename = federation_name_str + "_" + file_name;
   }
   message_publish( MSG_NORMAL, "Federate::restore_checkpoint() Restoring checkpoint file %s\n",
                    trick_filename.c_str() );

   // DANNY2.7 must init all data recording groups since we are restarting at init
   // time before Trick would normally do this. Prior to Trick 10.8, the only way
   // to do this is by calling each recording group init() routine in the S_define

   // This will run pre-load-checkpoint jobs, clear memory, read checkpoint
   // file, and run restart jobs.
   load_checkpoint( ( this->HLA_save_directory + "/" + trick_filename ).c_str() );

   load_checkpoint_job();

   // TODO: Load the checkpoint base time units into the Int64BaseTime class
   // so that all the HLA time representations use the correct base time.
   //
   // Refresh the HLA time constants given the HLA base time from the checkpoint.
   refresh_HLA_time_constants();

   // If exec_set_freeze_command(true) is in master fed's input.py file when
   // check-pointed, then restore starts up in freeze.
   // DANNY2.7 Clear non-master fed's freeze command so it does not cause
   // unnecessary freeze interaction to be sent.
   if ( !execution_control->is_master() ) {
      exec_set_freeze_command( false );
   }

   message_publish( MSG_NORMAL, "Federate::restore_checkpoint():%d Checkpoint file load complete.\n",
                    __LINE__ );

   // indicate that the restore was completed successfully
   this->restore_process = Restore_Complete;

   // make a copy of the 'restore_process' ENUM just in case it gets overwritten.
   this->prev_restore_process = this->restore_process;
}

void Federate::inform_RTI_of_restore_completion()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   if ( this->prev_restore_process == Restore_Complete ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::inform_RTI_of_restore_completion():%d Restore Complete.\n",
                          __LINE__ );
      }

      try {
         RTI_ambassador->federateRestoreComplete();
      } catch ( RestoreNotRequested const &e ) {
         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: RestoreNotRequested \n",
                          __LINE__ );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: FederateNotExecutionMember \n",
                          __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: SaveInProgress \n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: NotConnected \n",
                          __LINE__ );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore complete -- EXCEPTION: RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      }

   } else if ( this->prev_restore_process == Restore_Failed ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::inform_RTI_of_restore_completion():%d Restore Failed!\n",
                          __LINE__ );
      }

      try {
         RTI_ambassador->federateRestoreNotComplete();
      } catch ( RestoreNotRequested const &e ) {
         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: RestoreNotRequested \n",
                          __LINE__ );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: FederateNotExecutionMember \n",
                          __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: SaveInProgress \n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: NotConnected \n",
                          __LINE__ );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         message_publish( MSG_WARNING, "Federate::inform_RTI_of_restore_completion():%d -- restore NOT complete -- EXCEPTION: RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      }
   } else {
      message_publish( MSG_NORMAL, "Federate::inform_RTI_of_restore_completion():%d ERROR: \
Unexpected restore process %d, which is not 'Restore_Complete' or 'Restore_Request_Failed'.\n",
                       __LINE__, restore_process );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::read_running_feds_file(
   string const &file_name )
{
   string   full_path;
   ifstream file;

   // DANNY2.7 Prepend federation name to the filename (if it's not already prepended)
   string federation_name_str = string( get_federation_name() );
   if ( file_name.compare( 0, federation_name_str.length(), federation_name_str ) == 0 ) {
      // Already prepended
      full_path = this->HLA_save_directory + "/" + file_name + ".running_feds";
   } else {
      // Prepend it here
      full_path = this->HLA_save_directory + "/" + federation_name_str + "_" + file_name + ".running_feds";
   }

   file.open( full_path.c_str(), ios::in );
   if ( file.is_open() ) {

      // Clear out the known_feds from memory...
      for ( int i = 0; i < known_feds_count; ++i ) {
         if ( known_feds[i].MOM_instance_name != NULL ) {
            if ( trick_MM->delete_var( static_cast< void * >( known_feds[i].MOM_instance_name ) ) ) {
               message_publish( MSG_WARNING, "Federate::read_running_feds_file():%d WARNING failed to delete Trick Memory for 'known_feds[i].MOM_instance_name'\n",
                                __LINE__ );
            }
            known_feds[i].MOM_instance_name = NULL;
         }
         if ( known_feds[i].name != NULL ) {
            if ( trick_MM->delete_var( static_cast< void * >( known_feds[i].name ) ) ) {
               message_publish( MSG_WARNING, "Federate::read_running_feds_file():%d WARNING failed to delete Trick Memory for 'known_feds[i].name'\n",
                                __LINE__ );
            }
            known_feds[i].name = NULL;
         }
      }
      if ( trick_MM->delete_var( static_cast< void * >( this->known_feds ) ) ) {
         message_publish( MSG_WARNING, "Federate::read_running_feds_file():%d WARNING failed to delete Trick Memory for 'this->known_feds'\n",
                          __LINE__ );
      }
      this->known_feds = NULL;

      file >> this->known_feds_count;

      // Re-allocate it...
      this->known_feds = reinterpret_cast< KnownFederate * >(
         alloc_type( this->known_feds_count, "TrickHLA::KnownFederate" ) );
      if ( this->known_feds == NULL ) {
         ostringstream errmsg;
         errmsg << "Federate::read_running_feds_file():" << __LINE__
                << " ERROR: Could not allocate memory for known_feds!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      string current_line;
      for ( int i = 0; i < this->known_feds_count; ++i ) {
         file >> current_line;
         known_feds[i].MOM_instance_name = trick_MM->mm_strdup( const_cast< char * >( current_line.c_str() ) );

         file >> current_line;
         known_feds[i].name = trick_MM->mm_strdup( const_cast< char * >( current_line.c_str() ) );

         file >> current_line;
         known_feds[i].required = ( atoi( current_line.c_str() ) != 0 );
      }

      file.close(); // Close the file before exiting
   } else {
      ostringstream errmsg;
      errmsg << "Federate::read_running_feds_file()" << __LINE__
             << " ERROR: Failed to open file '" << full_path << "'!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

void Federate::copy_running_feds_into_known_feds()
{
   // Clear out the known_feds from memory...
   for ( int i = 0; i < this->known_feds_count; ++i ) {
      if ( known_feds[i].MOM_instance_name != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( known_feds[i].MOM_instance_name ) ) ) {
            message_publish( MSG_WARNING, "Federate::copy_running_feds_into_known_feds():%d WARNING failed to delete Trick Memory for 'known_feds[i].MOM_instance_name_name'\n",
                             __LINE__ );
         }
         known_feds[i].MOM_instance_name = NULL;
      }
      if ( known_feds[i].name != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( known_feds[i].name ) ) ) {
            message_publish( MSG_WARNING, "Federate::copy_running_feds_into_known_feds():%d WARNING failed to delete Trick Memory for 'known_feds[i].name'\n",
                             __LINE__ );
         }
         known_feds[i].name = NULL;
      }
   }
   if ( trick_MM->delete_var( static_cast< void * >( this->known_feds ) ) ) {
      message_publish( MSG_WARNING, "Federate::copy_running_feds_into_known_feds():%d WARNING failed to delete Trick Memory for 'this->known_feds'\n",
                       __LINE__ );
   }

   // Re-allocate it...
   this->known_feds = reinterpret_cast< KnownFederate * >(
      alloc_type( running_feds_count, "TrickHLA::KnownFederate" ) );
   if ( this->known_feds == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::copy_running_feds_into_known_feds():" << __LINE__
             << " ERROR: Could not allocate memory for known_feds!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Copy everything from running_feds into known_feds...
   this->known_feds_count = 0;
   for ( int i = 0; i < this->running_feds_count; ++i ) {
      known_feds[this->known_feds_count].MOM_instance_name = trick_MM->mm_strdup( running_feds[i].MOM_instance_name );
      known_feds[this->known_feds_count].name              = trick_MM->mm_strdup( running_feds[i].name );
      known_feds[this->known_feds_count].required          = running_feds[i].required;
      this->known_feds_count++;
   }
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - Currently only used Used with IMSIM initialization scheme; only for restore at simulation startup.
 *  @job_class{environment}
 */
void Federate::restart_checkpoint()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::restart_checkpoint():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      HLAinteger64Time fedTime;
      RTI_ambassador->queryLogicalTime( fedTime );
      set_granted_time( fedTime );
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::restart_checkpoint():%d queryLogicalTime EXCEPTION: FederateNotExecutionMember \n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::restart_checkpoint():%d queryLogicalTime EXCEPTION: SaveInProgress \n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::restart_checkpoint():%d queryLogicalTime EXCEPTION: RestoreInProgress \n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::restart_checkpoint():%d queryLogicalTime EXCEPTION: NotConnected \n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::restart_checkpoint():%d queryLogicalTime EXCEPTION: RTIinternalError \n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &time_adv_state_mutex );

      this->requested_time  = this->granted_time;
      this->restore_process = No_Restore;
   }

   reinstate_logged_sync_pts();

   federation_restored();
}

/*!
 *  @job_class{freeze}
 */
void Federate::federation_saved()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::federation_saved():%d\n", __LINE__ );
   }
   this->announce_save         = false;
   this->save_label_generated  = false;
   this->save_request_complete = false;
   this->save_label            = "";
   this->save_name             = L"";
   this->checkpoint_file_name  = "";

   if ( this->unfreeze_after_save ) {
      // This keeps from generating the RUNFED_v2 sync point since it's not needed
      execution_control->set_freeze_announced( false );

      // Exit freeze mode.
      un_freeze();
   }
}

/*!
 *  @job_class{freeze}
 */
void Federate::federation_restored()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::federation_restored():%d\n",
                       __LINE__ );
   }
   complete_restore();
   this->start_to_restore     = false;
   this->announce_restore     = false;
   this->save_label_generated = false;
   this->restore_begun        = false;
   this->restore_is_imminent  = false;
   this->restore_label        = "";
   this->restore_process      = No_Restore;
}

void Federate::wait_for_federation_restore_begun()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_begun():%d Waiting...\n",
                       __LINE__ );
   }

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !this->restore_begun ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->restore_begun ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_for_federation_restore_begun():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_begun():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_begun():%d Done.\n",
                       __LINE__ );
   }
}

void Federate::wait_until_federation_is_ready_to_restore()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_until_federation_is_ready_to_restore():%d Waiting...\n",
                       __LINE__ );
   }

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !this->start_to_restore ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->start_to_restore ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_until_federation_is_ready_to_restore():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::wait_until_federation_is_ready_to_restore():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_until_federation_is_ready_to_restore():%d Done.\n",
                       __LINE__ );
   }
}

string Federate::wait_for_federation_restore_to_complete()
{
   string return_string;

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   if ( this->restore_failed ) {
      return_string = "Federate::wait_for_federation_restore_to_complete() "
                      "Restore of federate failed\nTERMINATING SIMULATION!";
      return return_string;
   }

   if ( this->federation_restore_failed_callback_complete ) {
      return_string = "Federate::wait_for_federation_restore_to_complete() "
                      "Federation restore failed\nTERMINATING SIMULATION!";
      return return_string;
   }

   if ( this->restore_process == Restore_Failed ) {
      // before we enter the blocking loop, the RTI informed us that it accepted
      // the failure of the the federate restore. build and return a message.
      return_string = "Federate::wait_for_federation_restore_to_complete() "
                      "Federation restore FAILED! Look at the message from the "
                      "Federate::print_restore_failure_reason() routine "
                      "for a reason why the federation restore failed.\n"
                      "TERMINATING SIMULATION!";
      return return_string;
   }

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   // nobody reported any problems, wait until the restore is completed.
   while ( !this->restore_completed ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      if ( this->running_feds_count_at_time_of_restore > this->running_feds_count ) {
         // someone has resigned since the federation restore has been initiated.
         // build a message detailing what happened and exit the routine.
         return_string = "Federate::wait_for_federation_restore_to_complete() "
                         "While waiting for restore of the federation "
                         "a federate resigned before the federation restore "
                         "completed!\nTERMINATING SIMULATION!";
         return return_string;
      } else {
         sleep_timer.sleep(); // sleep until RTI responds...

         if ( !this->restore_completed ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Federate::wait_for_federation_restore_to_complete():" << __LINE__
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
               message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_to_complete():%d Waiting...\n",
                                __LINE__ );
            }
         }
      }
   }

   if ( this->restore_process == Restore_Failed ) {
      // after this federate restore blocking loop has finished, check if the RTI
      // accepted the failure of the federate restore. build and return a message.
      return_string = "Federate::wait_for_federation_restore_to_complete() "
                      "Federation restore FAILED! Look at the message from the "
                      "Federate::print_restore_failure_reason() routine "
                      "for a reason why the federation restore failed.\n"
                      "TERMINATING SIMULATION!";
      return return_string;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_to_complete():%d Done.\n",
                       __LINE__ );
   }
   return return_string;
}

void Federate::wait_for_restore_request_callback()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_restore_request_callback():%d Waiting...\n",
                       __LINE__ );
   }

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !has_restore_process_restore_request_failed()
           && !has_restore_process_restore_request_succeeded() ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !has_restore_process_restore_request_failed()
           && !has_restore_process_restore_request_succeeded() ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_for_restore_request_callback():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::wait_for_restore_request_callback():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_restore_request_callback():%d Done.\n",
                       __LINE__ );
   }
}

void Federate::wait_for_restore_status_to_complete()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_restore_status_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !this->restore_request_complete ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->restore_request_complete ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_for_restore_status_to_complete():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::wait_for_restore_status_to_complete():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_restore_status_to_complete():%d Done.\n",
                       __LINE__ );
   }
}

void Federate::wait_for_save_status_to_complete()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_save_status_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !this->save_request_complete ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->save_request_complete ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_for_save_status_to_complete():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::wait_for_save_status_to_complete():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_save_status_to_complete():%d Done.\n",
                       __LINE__ );
   }
}

void Federate::wait_for_federation_restore_failed_callback_to_complete()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_failed_callback_to_complete():%d Waiting...\n",
                       __LINE__ );
   }

   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !this->federation_restore_failed_callback_complete ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      // if the federate has already been restored, do not wait for a signal
      // from the RTI that the federation restore failed, you'll never get it!
      if ( this->restore_completed ) {
         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_failed_callback_to_complete():%d Restore Complete, Done.\n",
                             __LINE__ );
         }
         return;
      }
      sleep_timer.sleep(); // sleep until RTI responds...

      if ( !this->federation_restore_failed_callback_complete ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_for_federation_restore_failed_callback_to_complete():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_failed_callback_to_complete():%d Waiting...\n",
                             __LINE__ );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_federation_restore_failed_callback_to_complete():%d Done.\n",
                       __LINE__ );
   }
}

void Federate::request_federation_save_status()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::request_federation_save_status():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      RTI_ambassador->queryFederationSaveStatus();
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_save_status():%d EXCEPTION: FederateNotExecutionMember \n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_save_status():%d EXCEPTION: RestoreInProgress \n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_save_status():%d EXCEPTION: NotConnected \n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "Federate::request_federation_save_status():%d EXCEPTION: RTIinternalError: '%s' \n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::request_federation_restore_status()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::request_federation_restore_status():%d\n",
                       __LINE__ );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {
      RTI_ambassador->queryFederationRestoreStatus();
   } catch ( FederateNotExecutionMember const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_restore_status():%d EXCEPTION: FederateNotExecutionMember \n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_restore_status():%d EXCEPTION: SaveInProgress \n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_restore_status():%d EXCEPTION: RestoreInProgress \n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      message_publish( MSG_WARNING, "Federate::request_federation_restore_status():%d EXCEPTION: NotConnected \n",
                       __LINE__ );
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      message_publish( MSG_WARNING, "Federate::request_federation_restore_status():%d EXCEPTION: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 *  @job_class{freeze}
 */
void Federate::requested_federation_restore_status(
   bool status )
{
   if ( !status ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::requested_federation_restore_status():%d\n",
                          __LINE__ );
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      federate_ambassador->set_federation_restore_status_response_to_echo();
      try {
         RTI_ambassador->queryFederationRestoreStatus();
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Federate::requested_federation_restore_status():%d EXCEPTION: FederateNotExecutionMember \n",
                          __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::requested_federation_restore_status():%d EXCEPTION: SaveInProgress \n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Federate::requested_federation_restore_status():%d EXCEPTION: NotConnected \n",
                          __LINE__ );
      } catch ( RTIinternalError const &e ) {
         message_publish( MSG_WARNING, "Federate::requested_federation_restore_status():%d EXCEPTION: RTIinternalError \n",
                          __LINE__ );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

void Federate::print_requested_federation_restore_status(
   FederateRestoreStatusVector const &status_vector )
{
   FederateRestoreStatusVector::const_iterator vector_iter;

   // dump the contents...
   ostringstream msg;
   // load the first element from 'theFederateStatusVector'.
   vector_iter = status_vector.begin();

   // Determine if were successful.
   while ( vector_iter != status_vector.end() ) {

      // dump the contents, for now...
      string id_name;
      StringUtilities::to_string( id_name, vector_iter->preRestoreHandle );
      msg << "Federate::print_requested_federation_restore_status() " << __LINE__
          << "pre-restore fed_id=" << id_name;
      StringUtilities::to_string( id_name, vector_iter->postRestoreHandle );
      msg << ", post-restore fed_id =" << id_name
          << ", status matrix: \n   NO_RESTORE_IN_PROGRESS="
          << ( vector_iter->status == NO_RESTORE_IN_PROGRESS )
          << "\n   FEDERATE_RESTORE_REQUEST_PENDING="
          << ( vector_iter->status == FEDERATE_RESTORE_REQUEST_PENDING )
          << "\n   FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN="
          << ( vector_iter->status == FEDERATE_WAITING_FOR_RESTORE_TO_BEGIN )
          << "\n   FEDERATE_PREPARED_TO_RESTORE="
          << ( vector_iter->status == FEDERATE_PREPARED_TO_RESTORE )
          << "\n   FEDERATE_RESTORING="
          << ( vector_iter->status == FEDERATE_RESTORING )
          << "\n   FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE="
          << ( vector_iter->status == FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE )
          << '\n';
      // Load the next element from 'theFederateStatusVector'.
      ++vector_iter;
   }
   message_publish( MSG_NORMAL, msg.str().c_str() );
}

void Federate::process_requested_federation_restore_status(
   FederateRestoreStatusVector const &status_vector )
{
   FederateRestoreStatusVector::const_iterator vector_iter;
   FederateRestoreStatusVector::const_iterator vector_end;
   vector_iter = status_vector.begin();
   vector_end  = status_vector.end();

   // DANNY2.7 if any of our federates have a restore in progress, we will NOT initiate restore
   this->initiate_restore_flag = true;

   // while there are elements in Federate Restore Status Vector...
   while ( vector_iter != vector_end ) {
      if ( vector_iter->status != NO_RESTORE_IN_PROGRESS ) {
         this->initiate_restore_flag = false;
         break;
      }
      ++vector_iter;
   }

   // only initiate if all federates do not have restore in progress
   if ( this->initiate_restore_flag ) {
      this->restore_process = Initiate_Restore;
   }

   // indicate that the request has completed...
   restore_request_complete = true;
}

void Federate::process_requested_federation_save_status(
   FederateHandleSaveStatusPairVector const &status_vector )
{
   FederateHandleSaveStatusPairVector::const_iterator vector_iter;
   FederateHandleSaveStatusPairVector::const_iterator vector_end;
   vector_iter = status_vector.begin();
   vector_end  = status_vector.end();

   // DANNY2.7 if any of our federates have a save in progress, we will NOT initiate save
   initiate_save_flag = true;

   // while there are elements in Federate Save Status Vector...
   while ( initiate_save_flag && ( vector_iter != vector_end ) ) {
      if ( vector_iter->second != RTI1516_NAMESPACE::NO_SAVE_IN_PROGRESS ) {
         initiate_save_flag = false;
      }
      ++vector_iter;
   }

   // indicate that the request has completed...
   save_request_complete = true;
}

void Federate::print_restore_failure_reason(
   RestoreFailureReason reason )
{
   // dump the contents...
   ostringstream msg;

   if ( reason == RTI_UNABLE_TO_RESTORE ) {
      msg << "Federate::print_restore_failure_reason():" << __LINE__
          << " failure reason=\"RTI_UNABLE_TO_RESTORE\"\n";
   }
   if ( reason == FEDERATE_REPORTED_FAILURE_DURING_RESTORE ) {
      msg << "Federate::print_restore_failure_reason():" << __LINE__
          << " failure reason=\"FEDERATE_REPORTED_FAILURE_DURING_RESTORE\"\n";
   }
   if ( reason == FEDERATE_RESIGNED_DURING_RESTORE ) {
      msg << "Federate::print_restore_failure_reason():" << __LINE__
          << " failure reason=\"FEDERATE_RESIGNED_DURING_RESTORE\"\n";
   }
   if ( reason == RTI_DETECTED_FAILURE_DURING_RESTORE ) {
      msg << "Federate::print_restore_failure_reason():" << __LINE__
          << " failure reason=\"RTI_DETECTED_FAILURE_DURING_RESTORE\"\n";
   }
   message_publish( MSG_NORMAL, msg.str().c_str() );

   this->federation_restore_failed_callback_complete = true;
}

void Federate::print_save_failure_reason(
   SaveFailureReason reason )
{
   // dump the contents...
   ostringstream msg;

   if ( reason == RTI_UNABLE_TO_SAVE ) {
      msg << "Federate::print_save_failure_reason():" << __LINE__
          << " failure reason=\"RTI_UNABLE_TO_SAVE\"\n";
   }
   if ( reason == FEDERATE_REPORTED_FAILURE_DURING_SAVE ) {
      msg << "Federate::print_save_failure_reason():" << __LINE__
          << " failure reason=\"FEDERATE_REPORTED_FAILURE_DURING_SAVE\"\n";
   }
   if ( reason == FEDERATE_RESIGNED_DURING_SAVE ) {
      msg << "Federate::print_save_failure_reason():" << __LINE__
          << " failure reason=\"FEDERATE_RESIGNED_DURING_SAVE\"\n";
   }
   if ( reason == RTI_DETECTED_FAILURE_DURING_SAVE ) {
      msg << "Federate::print_save_failure_reason():" << __LINE__
          << " failure reason=\"=RTI_DETECTED_FAILURE_DURING_SAVE\"\n";
   }
   if ( reason == SAVE_TIME_CANNOT_BE_HONORED ) {
      msg << "Federate::print_save_failure_reason():" << __LINE__
          << " failure reason=\"SAVE_TIME_CANNOT_BE_HONORED\"\n";
   }
   message_publish( MSG_NORMAL, msg.str().c_str() );
}

/*!
 *  @job_class{environment}
 */
void Federate::set_checkpoint_file_name(
   string const &name ) // IN: -- checkpoint file name
{
   this->checkpoint_file_name = name;
   StringUtilities::to_wstring( this->save_name, name );
}

/*!
 *  @job_class{environment}
 */
void Federate::initiate_save_announce()
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_HLA_save_and_restore_supported() ) {
      return;
   }

   if ( this->save_label_generated ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "Federate::initiate_save_announce():%d save_label already generated for federate '%s'\n",
                          __LINE__, name );
      }
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::initiate_save_announce():%d Checkpoint filename:'%s'\n",
                       __LINE__, checkpoint_file_name.c_str() );
   }

   // Save the checkpoint_file_name into 'save_label'
   this->save_label = this->checkpoint_file_name;

   this->save_label_generated = true;
}

void Federate::initiate_restore_announce(
   string const &restore_name_label )
{
   // Just return if HLA save and restore is not supported by the simulation
   // initialization scheme selected by the user.
   if ( !is_HLA_save_and_restore_supported() ) {
      return;
   }

   this->restore_label = restore_name_label;

   // Wide String restore label
   wstring ws_restore_label;
   StringUtilities::to_wstring( ws_restore_label, this->restore_label );

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // figure out if anybody else requested a RESTORE before initiating the RESTORE!
   // change context to process for the status request...
   this->restore_request_complete = false;
   federate_ambassador->set_federation_restore_status_response_to_process();
   request_federation_restore_status();
   wait_for_restore_status_to_complete();

   if ( this->restore_process == Initiate_Restore ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string name_str;
         StringUtilities::to_string( name_str, ws_restore_label );
         message_publish( MSG_NORMAL, "Federate::initiate_restore_announce():%d \
restore_process == Initiate_Restore, Telling RTI to request federation \
restore with label '%s'.\n",
                          __LINE__, name_str.c_str() );
      }
      try {
         RTI_ambassador->requestFederationRestore( ws_restore_label );
         this->restore_process = Restore_In_Progress;

         // Save the # of running_feds at the time federation restore is initiated.
         // this way, when the count decreases, we know someone has resigned!
         this->running_feds_count_at_time_of_restore = this->running_feds_count;
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Federate::initiate_restore_announce():%d EXCEPTION: FederateNotExecutionMember \n",
                          __LINE__ );
         this->restore_process = No_Restore;
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::initiate_restore_announce():%d EXCEPTION: SaveInProgress \n",
                          __LINE__ );
         this->restore_process = No_Restore;
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Federate::initiate_restore_announce():%d EXCEPTION: RestoreInProgress \n",
                          __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Federate::initiate_restore_announce():%d EXCEPTION: NotConnected \n",
                          __LINE__ );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         message_publish( MSG_WARNING, "Federate::initiate_restore_announce():%d EXCEPTION: RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
         this->restore_process = No_Restore;
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "Federate::initiate_restore_announce():%d \
After communicating with RTI, restore_process != Initiate_Restore, \
Something went WRONG! \n",
                          __LINE__ );
      }
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::complete_restore()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::complete_restore():%d\n",
                       __LINE__ );
   }

   if ( this->restore_process != Restore_In_Progress ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::complete_restore():%d Restore Process != Restore_In_Progress.\n",
                          __LINE__ );
      }
      return;
   }

   if ( !this->start_to_restore ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::complete_restore():%d Start to restore flag is false so set restore_completed = true.\n",
                          __LINE__ );
      }
      restore_completed = true;
   }
}

bool Federate::is_federate_executing() const
{
   // Check if the manager has set a flag that the federate initialization has
   // completed and the federate is now executing.
   return this->execution_has_begun;
}

bool Federate::is_MOM_HLAfederation_instance_id(
   ObjectInstanceHandle const &instance_hndl )
{
   return ( MOM_HLAfederation_instance_name_map.find( instance_hndl ) != MOM_HLAfederation_instance_name_map.end() );
}

void Federate::set_MOM_HLAfederation_instance_attributes(
   ObjectInstanceHandle const    &instance_hndl,
   AttributeHandleValueMap const &values )
{
   // Determine if this is a MOM HLAfederation instance.
   if ( !is_MOM_HLAfederation_instance_id( instance_hndl ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::set_federation_instance_attributes():%d WARNING: Unknown object class, expected 'HLAmanager.HLAfederation'.\n",
                          __LINE__ );
      }
      return;
   }

   AttributeHandleValueMap::const_iterator attr_iter;
   for ( attr_iter = values.begin(); attr_iter != values.end(); ++attr_iter ) {

      if ( attr_iter->first == MOM_HLAautoProvide_handle ) {
         // HLAautoProvide attribute is an HLAswitch, which is an HLAinteger32BE.
         int const *data = static_cast< int const * >( attr_iter->second.data() );

         int auto_provide_state = Utilities::is_transmission_byteswap( ENCODING_BIG_ENDIAN )
                                     ? Utilities::byteswap_int( data[0] )
                                     : data[0];

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::set_federation_instance_attributes():%d Auto-Provide:%s value:%d\n",
                             __LINE__, ( ( auto_provide_state != 0 ) ? "Yes" : "No" ),
                             auto_provide_state );
         }

         this->auto_provide_setting = auto_provide_state;

      } else if ( attr_iter->first == MOM_HLAfederatesInFederation_handle ) {

         // Extract the size of the data and the data bytes.
         int const *data = static_cast< int const * >( attr_iter->second.data() );

         // The HLAfederatesInFederation has the HLAhandle datatype which has
         // the HLAvariableArray encoding with an HLAbyte element type. The
         // entry is the number of elements, followed by that number of
         // HLAvariableArrays.
         //  0 0 0 2 0 0 0 4 0 0 0 3 0 0 0 4 0 0 0 2
         //  ---+--- | | | | ---+--- | | | | ---+---
         //     |    ---+---    |    ---+---    |
         //   count   size   id #1    size   id #2
         //
         // The first 4 bytes (first 32-bit integer) is the number
         // of elements. WE ARE INTERESTED ONLY IN THIS VALUE!
         //
         // Determine if we need to byteswap or not since the FederateHandle
         // is in Big Endian. First 4 bytes (first 32-bit integer) is the number
         // of elements.
         int num_elements = Utilities::is_transmission_byteswap( ENCODING_BIG_ENDIAN )
                               ? Utilities::byteswap_int( data[0] )
                               : data[0];

         // save the count into running_feds_count
         this->running_feds_count = num_elements;

         // Since this list of federate id's is current, there is no reason to
         // thrash the RTI and chase down each federate id into a name. The
         // wait_for_required_federates_to_join() method already queries the
         // names from the RTI for all required federates. We will eventually
         // utilize the same MOM interface to rebuild this list...

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::set_federation_instance_attributes():%d Found a FederationID list with %d elements.\n",
                             __LINE__, num_elements );
         }
      }
   }
}

/*!
 *  @job_class{checkpoint}
 */
void Federate::convert_sync_pts()
{
   // Dispatch to the ExecutionControl specific process.
   execution_control->convert_loggable_sync_pts();
}

void Federate::reinstate_logged_sync_pts()
{
   // Dispatch to the ExecutionControl specific process.
   execution_control->reinstate_logged_sync_pts();
}

void Federate::check_HLA_save_directory()
{
   // If the save directory is not specified, set it to the current RUN directory
   if ( HLA_save_directory.empty() ) {

      string run_dir = command_line_args_get_output_dir();
      string def_dir = command_line_args_get_default_dir();

      // build a absolute path to the RUN directory by combining default_dir
      // and run_dir from the EXECUTIVE.
      this->HLA_save_directory = def_dir + "/" + run_dir;
   }
}

void Federate::restore_federate_handles_from_MOM()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::restore_federate_handles_from_MOM:%d \n",
                       __LINE__ );
   }

   // Make sure that we are in federate handle rebuild mode...
   federate_ambassador->set_federation_restored_rebuild_federate_handle_set();

   // Concurrency critical code section because joined-federate state is changed
   // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
   // function.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &joined_federate_mutex );

      // Note: Since we are doing reset we can safely clear the joined federate
      // name map. If we were not resetting, clearing the map will cause reflections
      // to fail since the instance lookup will fail.
      joined_federate_name_map.clear();

      // Clear the set of federate handles for the joined federates.
      joined_federate_handles.clear();

      // Clear the list of joined federate names.
      joined_federate_names.clear();
   }

   // Make sure we initialize the MOM handles we will use below. This should
   // also handle the case if the handles change after a checkpoint restore or
   // if this federate is now a master federate after the restore.
   initialize_MOM_handles();

   AttributeHandleSet fedMomAttributes;
   fedMomAttributes.insert( MOM_HLAfederate_handle );
   subscribe_attributes( MOM_HLAfederate_class_handle, fedMomAttributes );

   AttributeHandleSet requestedAttributes;
   requestedAttributes.insert( MOM_HLAfederate_handle );
   request_attribute_update( MOM_HLAfederate_class_handle, requestedAttributes );

   bool         all_found = false;
   int64_t      wallclock_time;
   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   // Wait for all the federate handles to be retrieved.
   do {
      // Concurrency critical code section because joined-federate state is changed
      // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
      // function.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &joined_federate_mutex );

         // Determine if all the federate handles have been found.
         all_found = ( (int)joined_federate_handles.size() >= running_feds_count );
      }

      if ( !all_found ) {

         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::restore_federate_handles_from_MOM():" << __LINE__
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
            message_publish( MSG_NORMAL, "Federate::restore_federate_handles_from_MOM:%d Waiting...\n",
                             __LINE__ );
         }
      }
   } while ( !all_found );

   // Only unsubscribe from the attributes we subscribed to in this function.
   unsubscribe_attributes( MOM_HLAfederate_class_handle, fedMomAttributes );

   // Make sure that we are no longer in federate handle rebuild mode...
   federate_ambassador->reset_federation_restored_rebuild_federate_handle_set();

   fedMomAttributes.clear();
   requestedAttributes.clear();
}

void Federate::rebuild_federate_handles(
   ObjectInstanceHandle const    &instance_hndl,
   AttributeHandleValueMap const &values )
{
   AttributeHandleValueMap::const_iterator attr_iter;

   // Loop through all federate handles
   for ( attr_iter = values.begin(); attr_iter != values.end(); ++attr_iter ) {

      // Do a sanity check on the overall encoded data size.
      if ( attr_iter->second.size() != 8 ) {
         ostringstream errmsg;
         errmsg << "Federate::rebuild_federate_handles():"
                << __LINE__ << " ERROR: Unexpected number of bytes in the"
                << " Encoded FederateHandle because the byte count is "
                << attr_iter->second.size()
                << " but we expected 8!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // The HLAfederateHandle has the HLAhandle datatype which is has the
      // HLAvariableArray encoding with an HLAbyte element type.
      //  0 0 0 4 0 0 0 2
      //  ---+--- | | | |
      //     |    ---+---
      // #elem=4  fedID = 2
      //
      // First 4 bytes (first 32-bit integer) is the number of elements.
      // Decode size from Big Endian encoded integer.
      unsigned char const *dataPtr = reinterpret_cast< unsigned char const * >( attr_iter->second.data() );

      int size = Utilities::is_transmission_byteswap( ENCODING_BIG_ENDIAN )
                    ? Utilities::byteswap_int( *reinterpret_cast< int const * >( dataPtr ) )
                    : *reinterpret_cast< int const * >( dataPtr );

      if ( size != 4 ) {
         ostringstream errmsg;
         errmsg << "Federate::rebuild_federate_handles():"
                << __LINE__ << " ERROR: FederateHandle size is "
                << size << " but expected it to be 4!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // Point to the start of the federate handle ID in the encoded data.
      dataPtr += 4;

      VariableLengthData t;
      t.setData( dataPtr, size );

      FederateHandle tHandle;

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      try {
         tHandle = RTI_ambassador->decodeFederateHandle( t );
      } catch ( CouldNotDecode const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Federate::rebuild_federate_handles():" << __LINE__
                << " EXCEPTION: CouldNotDecode\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( FederateNotExecutionMember const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Federate::rebuild_federate_handles():" << __LINE__
                << " EXCEPTION: FederateNotExecutionMember\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( NotConnected const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         ostringstream errmsg;
         errmsg << "Federate::rebuild_federate_handles():" << __LINE__
                << " EXCEPTION: NotConnected\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      } catch ( RTIinternalError const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );

         ostringstream errmsg;
         errmsg << "Federate::rebuild_federate_handles():" << __LINE__
                << " EXCEPTION: RTIinternalError: %s" << rti_err_msg << '\n';
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      // Concurrency critical code section because joined-federate state is changed
      // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
      // function.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &joined_federate_mutex );

         // Add this FederateHandle to the set of joined federates.
         joined_federate_handles.insert( tHandle );
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string id_str, fed_id;
         StringUtilities::to_string( id_str, instance_hndl );
         StringUtilities::to_string( fed_id, tHandle );
         message_publish( MSG_NORMAL, "Federate::rebuild_federate_handles():%d Federate OID:%s num_bytes:%d Federate-ID:%s\n",
                          __LINE__, id_str.c_str(), size, fed_id.c_str() );
      }
   }
}

/*!
 * @details Returns true if the supplied name is a required startup federate
 * or an instance object of a required startup federate.
 * \par<b>Assumptions and Limitations:</b>
 * - Assumes that the instance attributes' object name is in the format
 * 'object_name.FOM_name'. Otherwise, this logic fails.
 */
bool Federate::is_a_required_startup_federate(
   wstring const &fed_name )
{
   wstring required_fed_name;
   for ( int i = 0; i < this->known_feds_count; ++i ) {
      if ( known_feds[i].required ) {
         StringUtilities::to_wstring( required_fed_name, known_feds[i].name );
         if ( fed_name == required_fed_name ) { // found an exact match
            return true;
         } else {
            // look for instance attributes of a required object. to do this,
            // check if the "required federate name" is found inside the supplied
            // federate name.
            int found = fed_name.find( required_fed_name );
            if ( found != (int)wstring::npos ) {
               // found the "required federate name" inside the supplied federate name
               return true;
            }
         }
      }
   }
   return false;
}
