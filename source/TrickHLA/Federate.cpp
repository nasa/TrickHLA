/*!
@file TrickHLA/Federate.cpp
@ingroup TrickHLA
@brief This class provides basic services for an HLA federate.

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
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{SaveRestoreServices.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{time/TimeManagementServices.cpp}
@trick_link_dependency{time/TrickThreadCoordinator.cpp}
@trick_link_dependency{utils/MutexLock.cpp}
@trick_link_dependency{utils/MutexProtection.cpp}
@trick_link_dependency{utils/SleepTimeout.cpp}
@trick_link_dependency{utils/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, Titan Systems Corp., DIS, Titan Systems Corp., --, Initial investigation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support & test.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#if defined( IEEE_1516_2025 )
#   include <memory>
#endif // IEEE_1516_2025

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/sim_mode.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/KnownFederate.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/SaveRestoreServices.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/TimeManagementServices.hh"
#include "TrickHLA/utils/MutexProtection.hh"
#include "TrickHLA/utils/SleepTimeout.hh"
#include "TrickHLA/utils/StringUtilities.hh"
#include "TrickHLA/utils/Utilities.hh"

#if defined( IEEE_1516_2025 )
#   include "TrickHLA/FedAmbHLA4.hh"
#else
#   include "TrickHLA/FedAmbHLA3.hh"
#endif // IEEE_1516_2025

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Enums.h"
#include "RTI/Exception.h"
#include "RTI/Handle.h"
#include "RTI/RTI1516.h"
#include "RTI/RTIambassador.h"
#include "RTI/RTIambassadorFactory.h"
#include "RTI/Typedefs.h"
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/EncodingExceptions.h"
#include "RTI/encoding/HLAvariableArray.h"

#if defined( IEEE_1516_2025 )
#   include "RTI/RtiConfiguration.h"
#else
#   pragma GCC diagnostic pop
#endif // IEEE_1516_2025

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
   : name(),
     type(),
     federation_name(),
     rti_address(),
     local_settings(),
     FOM_modules(),
     MIM_module(),
     join_constraint( TrickHLA::FEDERATE_JOIN_EARLY_OR_LATE ),
     enable_known_feds( true ),
     known_feds_count( 0 ),
     known_feds( NULL ),
     debug_level( TrickHLA::DEBUG_LEVEL_NO_TRACE ),
     code_section( TrickHLA::DEBUG_SOURCE_ALL_MODULES ),
     wait_status_time( 30.0 ),
     can_rejoin_federation( false ),
     freeze_delay_frames( 2 ),
     unfreeze_after_save( false ),
     federation_created_by_federate( false ),
     federation_exists( false ),
     federation_joined( false ),
     all_federates_joined( false ),
     connected( false ),
     shutdown_called( false ),
     got_startup_sync_point( false ),
     make_copy_of_run_directory( false ),
     publish_data( true ),
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
     federate_ambassador( *this ),
     time_management_srvc( *this ),
     manager( *this ),
     save_restore_srvc( *this ),
     execution_control( NULL ),
     execution_config( NULL )
#if defined( IEEE_1516_2010 )
     ,
     RTI_ambassador( NULL )
#endif
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
   // Free the memory used by the array of known Federates for the Federation.
   clear_known_feds();
   known_feds_count = 0;

   // Clear the joined federate name map.
   joined_federate_name_map.clear();

   // Clear the set of federate handles for the joined federates.
   joined_federate_handles.clear();

   // Clear the list of joined federate names.
   joined_federate_names.clear();

   // Clear the MOM HLAfederation instance name map.
   MOM_HLAfederation_instance_name_map.clear();

   // Clear the list of discovered object federate names.
   MOM_HLAfederate_instance_name_map.clear();

   // Make sure we destroy the mutex.
   time_management_srvc.time_adv_state_mutex.destroy();
   joined_federate_mutex.destroy();
}

/*!
 * @job_class{initialization}
 */
void Federate::print_version()
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string rti_name;
      StringUtilities::to_string( rti_name, RTI1516_NAMESPACE::rtiName() );
      string rti_version;
      StringUtilities::to_string( rti_version, RTI1516_NAMESPACE::rtiVersion() );

      ostringstream msg;
      msg << "Federate::print_version():" << __LINE__ << endl
          << "     TrickHLA-version:'" << Utilities::get_version() << "'" << endl
          << "TrickHLA-release-date:'" << Utilities::get_release_date() << "'" << endl
          << "             RTI-name:'" << rti_name << "'" << endl
          << "          RTI-version:'" << rti_version << "'" << endl;
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
   ExecutionControlBase       &federate_execution_control,
   ExecutionConfigurationBase &federate_execution_config )
{
   // Set the Federate execution control.
   this->execution_control = &federate_execution_control;

   // Set the Federate execution configuration.
   this->execution_config = &federate_execution_config;

   // Register the ExecutionControl instance with the TrickHLA::Manager instance.
   this->manager.execution_control = &federate_execution_control;

   // Register the ExecutionControl instance with the TrickHLA::SaveRestoreServices instance.
   this->save_restore_srvc.execution_control = &federate_execution_control;

   // Set up the TrickHLA::ExecutionControl instance.
   this->execution_control->setup( *this );

   return;
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
      message_publish( MSG_WARNING, "Federate::initialize_debug():%d You specified an \
invalid debug level '%d' in the input.py file using an integer value instead of \
an ENUM. Please double check the value you specified in the input.py file against \
the documented ENUM values.\n",
                       __LINE__, (int)this->debug_level );
      if ( this->debug_level < DEBUG_LEVEL_NO_TRACE ) {
         this->debug_level = DEBUG_LEVEL_NO_TRACE;
         message_publish( MSG_WARNING, "Federate::initialize_debug():%d No TrickHLA debug messages will be emitted.\n",
                          __LINE__ );
      } else {
         this->debug_level = DEBUG_LEVEL_FULL_TRACE;
         message_publish( MSG_WARNING, "Federate::initialize_debug():%d All TrickHLA debug messages will be emitted.\n",
                          __LINE__ );
      }
   }

   // Set the debug level and code section in the global DebugHandler.
   DebugHandler::set( this->debug_level, this->code_section );

   // Print the current TrickHLA version string.
   print_version();

   // Refresh the HLA time constants since the base time units may have changed
   // from a setting in the input file.
   time_management_srvc.refresh_HLA_time_constants();
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

   // Check to make sure we have a reference to the TrickHLA::ExecutionControlBase.
   if ( execution_control == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::initialize():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA::ExecutionControlBase." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Make sure the federate name has been specified.
   if ( name.empty() ) {
      ostringstream errmsg;
      errmsg << "Federate::initialize():" << __LINE__
             << " ERROR: Unexpected NULL federate name." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // If a federate type is not specified make it the same as the federate name.
   if ( type.empty() ) {
      this->type = name;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::initialize():%d Federate:\"%s\" Type:\"%s\"\n",
                       __LINE__, name.c_str(), type.c_str() );
   }

   federate_ambassador.initialize();

   manager.verify_object_and_interaction_arrays();

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
      message_publish( MSG_NORMAL, "Federate::restart_initialization():%d\n",
                       __LINE__ );
   }

   time_management_srvc.restart_initialization();

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Verify the federate name.
   if ( name.empty() ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: NULL or zero length Federate Name." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Verify the FOM-modules value.
   if ( FOM_modules.empty() ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: Invalid FOM-modules."
             << " Please check your input.py or modified-data files to make sure"
             << " 'FOM_modules' is correctly specified, where 'FOM_modules' is"
             << " a comma separated list of FOM-module filenames." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Verify the Federation execution name.
   if ( federation_name.empty() ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: Invalid Federate Execution Name."
             << " Please check your input.py or modified-data files to make sure"
             << " the 'federation_name' is correctly specified." << endl;
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
                << " ERROR: No Known Federates Specified for the Federation." << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      if ( known_feds_count >= INT_MAX ) {
         ostringstream errmsg;
         errmsg << "Federate::restart_initialization():" << __LINE__
                << " ERROR: Known Federates count (" << known_feds_count
                << ") is >= " << INT_MAX << "!" << endl;
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // Validate the name of each Federate known to be in the Federation.
      for ( int i = 0; i < known_feds_count; ++i ) {

         // A zero length Federate name is not allowed.
         if ( known_feds[i].name.empty() ) {
            ostringstream errmsg;
            errmsg << "Federate::restart_initialization():" << __LINE__
                   << " ERROR: Invalid name of known Federate at array index: "
                   << i << endl;
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
   if ( !time_management_srvc.verify_time_constraints() ) {
      ostringstream errmsg;
      errmsg << "Federate::pre_multiphase_initialization():" << __LINE__
             << " ERROR: Time Constraints verification failed!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Perform the Execution Control specific pre-multi-phase initialization.
   execution_control->pre_multi_phase_init_processes();

   // Debug printout.
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::pre_multiphase_initialization():%d\n     Completed pre-multiphase initialization...\n",
                       __LINE__ );
   }

   // Initialize the TrickHLA::Manager object instance.
   manager.initialize();
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
   save_restore_srvc.set_federate_has_begun_execution();
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

#if defined( IEEE_1516_2025 )
   RtiConfiguration rti_config = RtiConfiguration::createConfiguration();
   if ( !rti_address.empty() ) {
      wstring rti_address_ws;
      StringUtilities::to_wstring( rti_address_ws, rti_address );
      rti_config = rti_config.withRtiAddress( rti_address_ws );
   }
   if ( !local_settings.empty() ) {
      wstring local_settings_ws;
      StringUtilities::to_wstring( local_settings_ws, local_settings );
      rti_config = rti_config.withAdditionalSettings( local_settings_ws );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
          << StringUtilities::to_string( rti_config ) << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
#else
   // For HLA-Evolved, the user can set a vendor specific local settings for
   // the connect() API.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      if ( local_settings.empty() ) {
         ostringstream msg;
         msg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " WARNING: Local settings designator 'THLA.federate.local_settings'"
             << " for the RTI was not specified in the input.py file. Using"
             << " vendor defaults." << endl;
         message_publish( MSG_WARNING, msg.str().c_str() );
      } else {
         ostringstream msg;
         msg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " Local settings designator for RTI connection:"
             << "'" << local_settings << "'" << endl;
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }

      if ( !rti_address.empty() ) {
         ostringstream msg;
         msg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << " WARNING: The 'THLA.federate.rti_address' setting is ignored"
             << " when using IEEE 1516-2010." << endl;
         message_publish( MSG_WARNING, msg.str().c_str() );
      }
   }
#endif // IEEE_1516_2025

   // Create the RTI ambassador factory, RTI-ambassador, and then connect.
   try {
#if defined( IEEE_1516_2025 )
      auto rti_amb_factory = std::make_unique< RTIambassadorFactory >();
      this->RTI_ambassador = rti_amb_factory->createRTIambassador();

      ConfigurationResult config_result;
      config_result   = RTI_ambassador->connect( federate_ambassador,
                                                 RTI1516_NAMESPACE::HLA_IMMEDIATE,
                                                 rti_config );
      this->connected = true;

      if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream msg;
         msg << "Federate::create_RTI_ambassador_and_connect():" << __LINE__
             << StringUtilities::to_string( config_result ) << endl;
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
#else
      RTIambassadorFactory *rti_amb_factory = new RTIambassadorFactory();
      this->RTI_ambassador                  = rti_amb_factory->createRTIambassador();

      if ( local_settings.empty() ) {
         // Use default vendor local settings.
         RTI_ambassador->connect( federate_ambassador,
                                  RTI1516_NAMESPACE::HLA_IMMEDIATE );
      } else {
         wstring local_settings_ws;
         StringUtilities::to_wstring( local_settings_ws, local_settings );

         RTI_ambassador->connect( federate_ambassador,
                                  RTI1516_NAMESPACE::HLA_IMMEDIATE,
                                  local_settings_ws );
      }
      this->connected = true;

      // Make sure we delete the factory now that we are done with it.
      delete rti_amb_factory;

#endif // IEEE_1516_2025

      // Reset the Federate shutdown-called flag now that we are connected.
      this->shutdown_called = false;

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
             << "' with local_settings: '" << local_settings
             << "' with EXCEPTION: ConnectionFailed: '" << rti_err_msg << "'." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
#if defined( IEEE_1516_2010 )
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
             << "' with local_settings: '" << local_settings
             << "' with EXCEPTION: InvalidLocalSettingsDesignator: '"
             << rti_err_msg << "'." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
#endif // IEEE_1516_2010
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
             << "' with local_settings: '" << local_settings
             << "' with EXCEPTION: UnsupportedCallbackModel: '"
             << rti_err_msg << "'." << endl;
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
             << "' with local_settings: '" << local_settings
             << "' with EXCEPTION: AlreadyConnected: '"
             << rti_err_msg << "'." << endl;
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
             << "' with local_settings: '" << local_settings
             << "' with EXCEPTION: CallNotAllowedFromWithinCallback: '"
             << rti_err_msg << "'." << endl;
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
             << "' with local_settings: '" << local_settings
             << "' with RTIinternalError: '" << rti_err_msg
             << "'. One possible"
             << " cause could be that the Central RTI Component is not running,"
             << " or is not running on the computer you think it is on. Please"
             << " check your CRC host and port settings and make sure the RTI"
             << " is running." << endl;
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
                 << " Object Instance:" << handle_str << endl;
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }
   }
}

bool Federate::is_federate_instance_id(
   ObjectInstanceHandle const &id )
{
   return ( joined_federate_name_map.find( id ) != joined_federate_name_map.end() );
}

/*! @brief Decode the specified encoded Federate Handle.
 *  @return Federate Handle.
 *  @param enc_handle encoded Federate Handle */
FederateHandle Federate::decode_federate_handle(
   VariableLengthData const &enc_handle )
{
   // Handles defined by the MOM interface have a an encoding of
   // HLAvariableArray, which is different than the Handles returned
   // by the  RTI-ambassador with the encoding of VariableLengthData.
   //
   // From IEEE 1516.1-2025:
   // Table 15 — MOM array data type table, page 327
   // Name: HLAfederateHandle
   // Element Type: HLAbyte
   // Cardinality: Dynamic
   // Encoding: HLAvariableArray
   //
   // Table 26 — Noncomplex C++ encoding helpers, page 380
   // HLA data representation: HLAfederateHandle
   // Encoding helper class: HLAfederateHandle
   // C++ type/macro: VariableLengthData
   //
   // Table 28 — Handle C++ encoding helpers, page 382
   // C++ handle representation FederateHandle
   // Encoding helper class: HLAfederateHandle
   // HLA data representation: HLAfederateHandle

   // Need an encoded handle that is of the VariableLengthData form.
   VariableLengthData encoded_fed_handle;

   if ( enc_handle.size() > 4 ) {
      // MOM defined Handle so convert to RTI-ambassador encoded handle.

      // The HLAfederateHandle has the HLAhandle data type which has the
      // HLAvariableArray encoding with an HLAbyte element type.
      //  0 0 0 4 0 0 0 2
      //  ---+--- | | | |
      //     |    ---+---
      // #elem=4  fedID = 2
      //
      // First 4 bytes (first 32-bit integer) is the number of elements
      // in the HLAvariableArray.
      unsigned char const *data = static_cast< unsigned char const * >( enc_handle.data() );

      // Point to the start of the federate handle ID in the encoded data.
      encoded_fed_handle.setData( data + 4, enc_handle.size() - 4 );

   } else if ( enc_handle.size() == 4 ) {
      // RTI-ambassador defined Handle so use as is.
      encoded_fed_handle = enc_handle;
   } else {
      ostringstream errmsg;
      errmsg << "Federate::decode_federate_handle():"
             << __LINE__ << " ERROR: Unexpected number of bytes in the"
             << " Encoded FederateHandle because the byte count is "
             << enc_handle.size() << ", but expected 4 or more bytes!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      exit( 1 );
   }

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
      errmsg << "Federate::decode_federate_handle():" << __LINE__
             << " ERROR: When decoding 'FederateHandle': EXCEPTION: CouldNotDecode" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      exit( 1 );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::decode_federate_handle():" << __LINE__
             << " ERROR: When decoding 'FederateHandle': EXCEPTION: FederateNotExecutionMember" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      exit( 1 );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      ostringstream errmsg;
      errmsg << "Federate::decode_federate_handle():" << __LINE__
             << " ERROR: When decoding 'FederateHandle': EXCEPTION: NotConnected" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::decode_federate_handle():" << __LINE__
             << " ERROR: When decoding 'FederateHandle': EXCEPTION: "
             << "RTIinternalError: %s" << rti_err_msg << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      exit( 1 );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_5_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string fed_id;
      StringUtilities::to_string( fed_id, fed_handle );
      message_publish( MSG_NORMAL, "Federate::decode_federate_handle():%d Federate-Handle:%s\n",
                       __LINE__, fed_id.c_str() );
   }

   return fed_handle;
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
      for ( size_t i = 0; !found && ( i < joined_federate_names.size() ); ++i ) {
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
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d Federate-OID:%s Name:'%s' size:%d\n",
                          __LINE__, id_str.c_str(), federate_name_ws.c_str(),
                          (int)federate_name_ws.size() );
      }
   }

   // Find the FederateHandle attribute for the given MOM federate handle.
   attr_iter = values.find( MOM_HLAfederate_handle );

   // Determine if we have a federate handle attribute.
   if ( attr_iter == values.end() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string id_str;
         StringUtilities::to_string( id_str, id );
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d FederateHandle Not found for Federate-OID:%s\n",
                          __LINE__, id_str.c_str() );
      }
   } else {

      FederateHandle fed_handle = decode_federate_handle( attr_iter->second );

      // Add this FederateHandle to the set of joined federates.
      joined_federate_handles.insert( fed_handle );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string id_str, fed_id;
         StringUtilities::to_string( id_str, id );
         StringUtilities::to_string( fed_id, fed_handle );
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d Federate-OID:%s Federate-ID:%s\n",
                          __LINE__, id_str.c_str(), fed_id.c_str() );
      }

      // If this federate is running, add the new entry into running_feds.
      if ( this->is_federate_executing() ) {
         bool found = false;
         for ( size_t loop = 0; loop < save_restore_srvc.running_feds_count; ++loop ) {
            string tName;
            StringUtilities::to_string( tName, federate_name_ws );

            if ( save_restore_srvc.running_feds[loop].name == tName ) {
               found = true;
               break;
            }
         }
         // Update the running_feds if the federate name was not found.
         if ( !found ) {
            if ( joined_federate_name_map.size() == 1 ) {
               save_restore_srvc.add_a_single_entry_into_running_feds();

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
                  save_restore_srvc.add_a_single_entry_into_running_feds();

                  // Clear the entry after it is absorbed into running_feds.
                  joined_federate_name_map.clear();
               } else {
                  // Process multiple joined_federate_name_map entries.
                  save_restore_srvc.clear_running_feds();
                  ++save_restore_srvc.running_feds_count;
                  save_restore_srvc.update_running_feds();

                  // Clear the entries after they are absorbed into running_feds.
                  joined_federate_name_map.clear();
               }
            }
         }
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
             << " ERROR: Unexpected NULL RTIambassador." << endl;
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
         if ( !known_feds[i].MOM_instance_name.empty() ) {

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
               summary << endl
                       << "    Federate:'" << known_feds[i].name
                       << "' MOM-Object-ID:" << id_str;
            }
         }
      }
   } catch ( ObjectInstanceNotKnown const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << endl;
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }

      string fed_mom_instance_name;
      StringUtilities::to_string( fed_mom_instance_name, fed_mom_instance_name_ws );
      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: Object Instance Not Known for '"
             << fed_mom_instance_name << "'" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << endl;
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }

      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: Federation Not Execution Member" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << endl;
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }
      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: NotConnected" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << endl;
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " RTIinternalError: '" << rti_err_msg << "'" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         summary << endl;
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: Exception for '" << rti_err_msg << "'" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      summary << endl;
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
             << " Unexpected NULL RTIambassador." << endl;
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
                  StringUtilities::mm_strdup_wstring(
                     rti_amb->getObjectInstanceName( fed_mom_instance_hdl ) );
            }
         }
      }
   } catch ( ObjectInstanceNotKnown const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Federate::determine_federate_MOM_object_instance_names():%d rti_amb->getObjectInstanceName() ERROR: ObjectInstanceNotKnown\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Federate::determine_federate_MOM_object_instance_names():%d rti_amb->getObjectInstanceName() ERROR: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Federate::determine_federate_MOM_object_instance_names():%d rti_amb->getObjectInstanceName() ERROR: NotConnected\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::determine_federate_MOM_object_instance_names():%d rti_amb->getObjectInstanceName() ERROR: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
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
      errmsg << "Federate::determine_federate_MOM_object_instance_names():" << __LINE__
             << " ERROR: Exception getting MOM instance name for '"
             << fed_name_str << "' ID:" << id_str
             << " '" << rti_err_msg << "'." << endl;
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
   string const &federate_name )
{
   wstring fed_name_ws;
   StringUtilities::to_wstring( fed_name_ws, federate_name );
   return is_joined_federate( fed_name_ws );
}

bool Federate::is_joined_federate(
   wstring const &federate_name )
{
   for ( size_t i = 0; i < joined_federate_names.size(); ++i ) {
      if ( federate_name == joined_federate_names[i] ) { // cppcheck-suppress [useStlAlgorithm]
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
                           << __LINE__ << endl
                           << "WAITING FOR " << required_feds_count
                           << " REQUIRED FEDERATES:";

      // Display the initial summary of the required federates we are waiting for.
      int cnt = 0;
      for ( int i = 0; i < known_feds_count; ++i ) {
         // Create a summary of the required federates by name.
         if ( known_feds[i].required ) {
            ++cnt;
            required_fed_summary << endl
                                 << "    " << cnt
                                 << ": Waiting for required federate '"
                                 << known_feds[i].name << "'";
         }
      }

      required_fed_summary << endl;

      // Display a summary of the required federate by name.
      message_publish( MSG_NORMAL, required_fed_summary.str().c_str() );

      // Display a message that we are requesting the federate names.
      message_publish( MSG_NORMAL, "Federate::wait_for_required_federates_to_join():%d Requesting list of joined federates from CRC.\n",
                       __LINE__ );
   }

   // Subscribe to Federate names using MOM interface and request an update.
   ask_MOM_for_federate_names();

   size_t joined_fed_cnt = 0;

   bool print_summary                = false;
   bool found_an_unrequired_federate = false;

   set< string > unrequired_federates_list; // list of unique unrequired federate names

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
         if ( joined_fed_cnt != joined_federate_names.size() ) {
            joined_fed_cnt = joined_federate_names.size();

            // Count the number of joined Required federates.
            int req_fed_cnt = 0;
            for ( size_t i = 0; i < joined_federate_names.size(); ++i ) {
               if ( is_required_federate( joined_federate_names[i] ) ) {
                  ++req_fed_cnt;
               } else {
                  found_an_unrequired_federate = true;
                  string fedname;
                  StringUtilities::to_string( fedname, joined_federate_names[i] );
                  if ( save_restore_srvc.restore_is_imminent ) {
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
                    << __LINE__ << endl
                    << "WAITING FOR " << required_feds_count
                    << " REQUIRED FEDERATES:";

            // Summarize the required federates first.
            int cnt = 0;
            for ( int i = 0; i < known_feds_count; ++i ) {
               ++cnt;
               if ( known_feds[i].required ) {
                  if ( is_joined_federate( known_feds[i].name ) ) {
                     summary << endl
                             << "    " << cnt
                             << ": Found joined required federate '"
                             << known_feds[i].name << "'";
                  } else {
                     summary << endl
                             << "    " << cnt
                             << ": Waiting for required federate '"
                             << known_feds[i].name << "'";
                  }
               }
            }

            // Summarize all the remaining non-required joined federates.
            for ( size_t i = 0; i < joined_federate_names.size(); ++i ) {
               if ( !is_required_federate( joined_federate_names[i] ) ) {
                  ++cnt;

                  // We need a string version of the wide-string federate name.
                  string fedname;
                  StringUtilities::to_string( fedname, joined_federate_names[i] );

                  summary << endl
                          << "    " << cnt << ": Found joined federate '"
                          << fedname << "'";
               }
            }
            summary << endl;

            // Display the federate summary.
            message_publish( MSG_NORMAL, summary.str().c_str() );
         }
      } // Mutex protection goes out of scope here

      if ( !this->all_federates_joined ) {

         // To be more efficient, we get the time once and share it.
         int64_t wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::wait_for_required_federates_to_join():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
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
   if ( save_restore_srvc.restore_is_imminent && found_an_unrequired_federate ) {
      ostringstream errmsg;
      errmsg << "FATAL ERROR: You indicated a restore of a checkpoint set but "
             << "at least one federate which was NOT executing at the time of "
             << "the checkpoint is currently joined in the federation. This "
             << "violates IEEE Std 1516.2000, section 4.18 (Request Federation "
             << "Restore), precondition d), \"The correct number of joined "
             << "federates of the correct types that were joined to the "
             << "federation execution when the save was accomplished are "
             << "currently joined to the federation execution.\"" << endl
             << "\tThe extraneous ";
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
      errmsg << names << endl
             << "\tThe required federates are: ";
      names = "";
      for ( int i = 0; i < known_feds_count; ++i ) {
         if ( known_feds[i].required ) {
            names += known_feds[i].name;
            names += ", ";
         }
      }
      names.resize( names.length() - 2 ); // remove trailing comma and space
      errmsg << names << endl
             << "TERMINATING EXECUTION!";

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
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getObjectClassHandle('HLAmanager.HLAfederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getObjectClassHandle('HLAmanager.HLAfederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getObjectClassHandle('HLAmanager.HLAfederation')\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getObjectClassHandle('HLAmanager.HLAfederation')\n",
                       __LINE__ );
   }

   // Get the MOM Federates In Federation Attribute handle.
   try {
      this->MOM_HLAfederatesInFederation_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederation_class_handle,
                                                                                      L"HLAfederatesInFederation" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAfederatesInFederation')\n",
                       __LINE__ );
   }

   // Get the MOM Auto Provide Attribute handle.
   try {
      this->MOM_HLAautoProvide_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederation_class_handle,
                                                                            L"HLAautoProvide" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle( MOM_federation_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   }

   // Get the MOM Federate Class handle.
   try {
      this->MOM_HLAfederate_class_handle = RTI_ambassador->getObjectClassHandle( L"HLAobjectRoot.HLAmanager.HLAfederate" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getObjectClassHandle('HLAobjectRoot.HLAmanager.HLAfederate')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getObjectClassHandle('HLAobjectRoot.HLAmanager.HLAfederate')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getObjectClassHandle('HLAobjectRoot.HLAmanager.HLAfederate')\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getObjectClassHandle('HLAobjectRoot.HLAmanager.HLAfederate')\n",
                       __LINE__ );
   }

   // Get the MOM Federate Name Attribute handle.
   try {
      this->MOM_HLAfederateName_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederate_class_handle, L"HLAfederateName" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateName')\n",
                       __LINE__ );
   }

   // Get the MOM Federate Type Attribute handle.
   try {
      this->MOM_HLAfederateType_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederate_class_handle, L"HLAfederateType" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateType')\n",
                       __LINE__ );
   }

   // Get the MOM Federate Attribute handle.
   try {
      this->MOM_HLAfederate_handle = RTI_ambassador->getAttributeHandle( MOM_HLAfederate_class_handle, L"HLAfederateHandle" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidObjectClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
InvalidObjectClassHandle ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getAttributrHandle(MOM_federate_class_handle, 'HLAfederateHandle')\n",
                       __LINE__ );
   }

   // Interaction: HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches
   //   Parameter: HLAautoProvide of type HLAswitches which is a HLAinteger32BE
   try {
      this->MOM_HLAsetSwitches_class_handle = RTI_ambassador->getInteractionClassHandle( L"HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getInteractionClassHandle('HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getInteractionClassHandle('HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getInteractionClassHandle('HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches')\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
RTIinternalError for RTI_amb->getInteractionClassHandle('HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches')\n",
                       __LINE__ );
   }

   try {
      this->MOM_HLAautoProvide_param_handle = RTI_ambassador->getParameterHandle( MOM_HLAsetSwitches_class_handle, L"HLAautoProvide" );
   } catch ( RTI1516_NAMESPACE::NameNotFound const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NameNotFound ERROR for RTI_amb->getParameterHandle(MOM_HLAsetSwitches_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidInteractionClassHandle const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
InvalidInteractionClassHandle ERROR for RTI_amb->getParameterHandle(MOM_HLAsetSwitches_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
FederateNotExecutionMember ERROR for RTI_amb->getParameterHandle(MOM_HLAsetSwitches_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
NotConnected ERROR for RTI_amb->getParameterHandle(MOM_HLAsetSwitches_class_handle, 'HLAautoProvide')\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::initialize_MOM_handles():%d \
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
      summary << "Federate::subscribe_attributes():" << __LINE__ << endl;

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, class_handle );
         summary << "  Class-Handle:" << handle_str << " with "
                 << attribute_list.size() << " Attributes" << endl;

         AttributeHandleSet::const_iterator attr_iter;
         for ( attr_iter = attribute_list.begin();
               attr_iter != attribute_list.end();
               ++attr_iter ) {

            StringUtilities::to_string( handle_str, *attr_iter );
            summary << "   + Attribute-Handle:" << handle_str << endl;
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
      message_publish( MSG_ERROR, "Federate::subscribe_attributes():%d ObjectClassNotDefined: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::AttributeNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::subscribe_attributes():%d AttributeNotDefined: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::subscribe_attributes():%d FederateNotExecutionMember: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::subscribe_attributes():%d SaveInProgress: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::subscribe_attributes():%d RestoreInProgress: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::InvalidUpdateRateDesignator const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::subscribe_attributes():%d InvalidUpdateRateDesignator: MOM Object Attributed Subscribe FAILED!!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::subscribe_attributes():%d NotConnected: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::subscribe_attributes():%d RTIinternalError: MOM Object Attributed Subscribe FAILED!\n",
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
      summary << "Federate::unsubscribe_attributes():" << __LINE__ << endl;

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, class_handle );
         summary << "  Class-Handle:" << handle_str << " with "
                 << attribute_list.size() << " Attributes" << endl;

         AttributeHandleSet::const_iterator attr_iter;
         for ( attr_iter = attribute_list.begin();
               attr_iter != attribute_list.end();
               ++attr_iter ) {
            StringUtilities::to_string( handle_str, *attr_iter );
            summary << "   + Attribute-Handle:" << handle_str << endl;
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
      message_publish( MSG_ERROR, "Federate::unsubscribe_attributes():%d ObjectClassNotDefined: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::AttributeNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::unsubscribe_attributes():%d AttributeNotDefined: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::unsubscribe_attributes():%d FederateNotExecutionMember: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::unsubscribe_attributes():%d SaveInProgress: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::unsubscribe_attributes():%d RestoreInProgress: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::unsubscribe_attributes():%d NotConnected: MOM Object Attributed Subscribe FAILED!\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::unsubscribe_attributes():%d RTIinternalError: MOM Object Attributed Subscribe FAILED!\n",
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
      summary << "Federate::request_attribute_update():" << __LINE__ << endl;

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, class_handle );
         summary << "  Class-Handle:" << handle_str << " with "
                 << attribute_list.size() << " Attributes" << endl;

         AttributeHandleSet::const_iterator attr_iter;
         for ( attr_iter = attribute_list.begin();
               attr_iter != attribute_list.end();
               ++attr_iter ) {
            StringUtilities::to_string( handle_str, *attr_iter );
            summary << "   + Attribute-Handle:" << handle_str << endl;
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
                                                   TrickHLA::EMPTY_USER_SUPPLIED_TAG );
   } catch ( ObjectClassNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::request_attribute_update():%d ObjectClassNotDefined: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( AttributeNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::request_attribute_update():%d AttributeNotDefined: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::request_attribute_update():%d FederateNotExecutionMember: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::request_attribute_update():%d SaveInProgress: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::request_attribute_update():%d RestoreInProgress: Attribute update request FAILED!\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::request_attribute_update():%d NotConnected: Attribute update request FAILED!\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::request_attribute_update():%d RTIinternalError: MOM Object Attributed update request FAILED!\n",
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
              << __LINE__ << endl;

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, MOM_HLAfederate_class_handle );
         summary << "  Class-Handle:" << handle_str << endl;
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
      set_connection_lost();
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
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::unsubscribe_all_HLAfederation_class_attributes_from_MOM():%d RTIinternalError: Unsubscribe object class FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::publish_interaction_class( // cppcheck-suppress [functionStatic, unmatchedSuppression]
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
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::publish_interaction_class():%d RTIinternalError: Publish interaction class FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::unpublish_interaction_class( // cppcheck-suppress [functionStatic, unmatchedSuppression]
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
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      message_publish( MSG_WARNING, "Federate::unpublish_interaction_class():%d RTIinternalError: Unpublish interaction class FAILED!\n",
                       __LINE__ );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Federate::send_interaction( // cppcheck-suppress [functionStatic, unmatchedSuppression]
   RTI1516_NAMESPACE::InteractionClassHandle const  &class_handle,
   RTI1516_NAMESPACE::ParameterHandleValueMap const &parameter_list )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   bool error_flag = false;
   try {
      RTI_ambassador->sendInteraction( class_handle, parameter_list, TrickHLA::EMPTY_USER_SUPPLIED_TAG );
   } catch ( InteractionClassNotPublished const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::send_interaction():%d InteractionClassNotPublished: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( InteractionParameterNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::send_interaction():%d InteractionParameterNotDefined: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( InteractionClassNotDefined const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::send_interaction():%d InteractionClassNotDefined: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( SaveInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::send_interaction():%d SaveInProgress: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( RestoreInProgress const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::send_interaction():%d RestoreInProgress: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::send_interaction():%d FederateNotExecutionMember: Send interaction FAILED!\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::send_interaction():%d NotConnected: Send interaction FAILED!\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      error_flag = true;
      message_publish( MSG_ERROR, "Federate::send_interaction():%d RTIinternalError: Send interaction FAILED!\n",
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
   wstring const            &label,
   VariableLengthData const &user_supplied_tag )
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
 * - Currently only used with DIS and IMSim initialization schemes.
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
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with IMSim initialization scheme.
 *  @job_class{checkpoint}
 */
void Federate::setup_checkpoint()
{
   // Delegate to the Execution Control specific implementation.
   execution_control->setup_checkpoint();
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSim initialization schemes.
 *  @job_class{freeze}
 */
void Federate::perform_checkpoint()
{
   // Delegate to the Execution Control specific implementation.
   execution_control->perform_checkpoint();
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSim initialization schemes.
 *  @job_class{preload_checkpoint}
 */
void Federate::setup_restore()
{
   // Delegate to the Execution Control specific implementation.
   execution_control->setup_restore();
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSim initialization schemes.
 *  @job_class{freeze}
 */
void Federate::perform_restore()
{
   // Delegate to the Execution Control specific implementation.
   execution_control->perform_restore();
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
             << " ERROR: NULL pointer to RTIambassador!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::create_federation():%d Attempting to create Federation '%s'\n",
                       __LINE__, get_federation_name().c_str() );
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
      if ( !FOM_modules.empty() ) {
         StringUtilities::tokenize( FOM_modules, FOM_modules_vector, "," );
      }

      // Determine if the user specified a MIM-module, which determines how
      // we create the federation execution.
      if ( !MIM_module.empty() ) {
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
                          __LINE__, get_federation_name().c_str() );
      }
   } catch ( RTI1516_NAMESPACE::FederationExecutionAlreadyExists const &e ) {
      // Just ignore the exception if the federation execution already exits
      // because of how the multiphase initialization is designed this is not
      // an error since everyone tries to create the federation as the first
      // thing they do.
      this->federation_exists = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::create_federation():%d Federation already exists for '%s'\n",
                          __LINE__, get_federation_name().c_str() );
      }
#if defined( IEEE_1516_2025 )
   } catch ( RTI1516_NAMESPACE::CouldNotOpenFOM const &e ) {
#else
   } catch ( RTI1516_NAMESPACE::CouldNotOpenFDD const &e ) {
#endif
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " ERROR: Could not open FOM-modules: '"
             << FOM_modules << "'";
      if ( !MIM_module.empty() ) {
         errmsg << " or MIM-module: '" << MIM_module << "'";
      }
      errmsg << ", RTI Exception: " << rti_err_msg << endl;
      DebugHandler::terminate_with_message( errmsg.str() );

#if defined( IEEE_1516_2025 )
   } catch ( RTI1516_NAMESPACE::ErrorReadingFOM const &e ) {
#else
   } catch ( RTI1516_NAMESPACE::ErrorReadingFDD const &e ) {
#endif
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " ERROR: Problem reading FOM-modules: '"
             << FOM_modules << "'";
      if ( !MIM_module.empty() ) {
         errmsg << " or MIM-module: '" << MIM_module << "'";
      }
      errmsg << ", RTI Exception: " << rti_err_msg << endl;
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
             << "', RTI Exception: " << rti_err_msg << endl
             << "  Make sure that you "
             << "are using a IEEE_1516_2010-compliant RTI version which "
             << "supplies the 'HLAinteger64Time' class." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " EXCEPTION: NotConnected" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " RTI Internal Error: " << rti_err_msg << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      // This is an error so show out an informative message and terminate.
      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " ERROR: Unrecoverable error in federation '" << get_federation_name()
             << "' creation, RTI Exception: " << rti_err_msg << endl;
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
   string const &federate_name,
   string const &federate_type )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Sanity check.
   if ( RTI_ambassador.get() == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " ERROR: NULL pointer to RTIambassador!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   if ( this->federation_joined ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream errmsg;
         errmsg << "Federate::join_federation():" << __LINE__
                << " Federation '" << get_federation_name()
                << "': ALREADY JOINED FEDERATION EXECUTION" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Make sure the federate name has been specified.
   if ( federate_name.empty() ) {
      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " ERROR: Unexpected empty federate name." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Create the wide-string version of the federation and federate name & type.
   wstring federation_name_ws;
   StringUtilities::to_wstring( federation_name_ws, federation_name );
   wstring fed_name_ws;
   StringUtilities::to_wstring( fed_name_ws, federate_name );
   wstring fed_type_ws;
   if ( federate_type.empty() ) {
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
                       __LINE__, get_federation_name().c_str() );
   }
   try {
      this->federation_joined = false;

      VectorOfWstrings fomModulesVector;

      // Add the user specified FOM-modules to the vector by parsing the comma
      // separated list of modules.
      if ( !FOM_modules.empty() ) {
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
                          __LINE__, get_federation_name().c_str(), id_str.c_str() );
      }
   } catch ( RTI1516_NAMESPACE::CouldNotCreateLogicalTimeFactory const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: CouldNotCreateLogicalTimeFactory" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederateNameAlreadyInUse const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: FederateNameAlreadyInUse! Federate name:\""
             << get_federate_name() << "\"" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
#if defined( IEEE_1516_2025 )
   } catch ( RTI1516_NAMESPACE::InconsistentFOM const &e ) {
#else
   } catch ( RTI1516_NAMESPACE::InconsistentFDD const &e ) {
#endif

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: Inconsistent FOM! FOM-modules:\""
             << FOM_modules << "\"" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
#if defined( IEEE_1516_2025 )
   } catch ( RTI1516_NAMESPACE::ErrorReadingFOM const &e ) {
#else
   } catch ( RTI1516_NAMESPACE::ErrorReadingFDD const &e ) {
#endif

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: Error Reading FOM! FOM-modules:\""
             << FOM_modules << "\"" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );

#if defined( IEEE_1516_2025 )
   } catch ( RTI1516_NAMESPACE::CouldNotOpenFOM const &e ) {
#else
   } catch ( RTI1516_NAMESPACE::CouldNotOpenFDD const &e ) {
#endif
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: Could Not Open FOM! FOM-modules:\""
             << FOM_modules << "\"" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederateAlreadyExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " ERROR: The Federate '" << get_federate_name()
             << "' is already a member of the '"
             << get_federation_name() << "' Federation." << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederationExecutionDoesNotExist const &e ) {
      // The federation we created must have been destroyed by another
      // federate before we could join, so try again.
      this->federation_created_by_federate = false;
      this->federation_exists              = false;
      message_publish( MSG_WARNING, "Federate::join_federation():%d EXCEPTION: %s Federation Execution does not exist.\n",
                       __LINE__, get_federation_name().c_str() );
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
             << " EXCEPTION: NotConnected" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::CallNotAllowedFromWithinCallback const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: CallNotAllowedFromWithinCallback" << endl;

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
             << rti_err_msg << endl;

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
                << "\": ALREADY JOINED FEDERATION EXECUTION" << endl;
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
                          __LINE__, get_federation_name().c_str(), k, max_retries );
         Utilities::micro_sleep( 100000 );
      }
   }

   if ( !this->federation_joined ) {
      ostringstream errmsg;
      errmsg << "Federate::create_and_join_federation():" << __LINE__
             << " ERROR: Federate '" << get_federate_name() << "' FAILED TO JOIN the '"
             << get_federation_name() << "' Federation." << endl;

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
         message_publish( MSG_NORMAL, "Federate::enable_async_delivery():%d Enabling Asynchronous Delivery\n",
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
             << " EXCEPTION: SaveInProgress" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: RestoreInProgress" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: FederateNotExecutionMember" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: NotConnected" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: RTIinternalError: '" << rti_err_msg << "'" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::enable_async_delivery():%d \"%s\": Unexpected RTI exception!\nRTI Exception: RTIinternalError: '%s'\n\n",
                       __LINE__, get_federation_name().c_str(), rti_err_msg.c_str() );
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
   if ( !connected ) {
      ostringstream errmsg;
      errmsg << "Federate::check_for_shutdown_with_termination():" << __LINE__
             << " ERROR: Lost the connection to the RTI. Terminating the simulation!"
             << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   return ( execution_control->check_for_shutdown_with_termination() );
}

/*!
 * @brief Send zero lookahead or requested data for the specified object instance.
 * @param obj_instance_name Object instance name to send data for.
 */
void Federate::send_zero_lookahead_and_requested_data(
   string const &obj_instance_name )
{
   TrickHLA::Object *obj = manager.get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::send_zero_lookahead_and_requested_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // We can only send zero-lookahead attribute updates for the attributes we
   // own and are configured to publish.
   if ( !obj->any_locally_owned_published_zero_lookahead_or_requested_attribute() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::send_zero_lookahead_and_requested_data():%d Object:'%s'\n",
                       __LINE__, obj_instance_name.c_str() );
   }

   obj->send_zero_lookahead_and_requested_data( time_management_srvc.granted_time );
}

/*!
 *  @brief Blocking function call to wait to receive the zero lookahead data
 *  for the specified object instance.
 *  @param obj_instance_name Object instance name to wait for data.
 */
void Federate::wait_to_receive_zero_lookahead_data(
   string const &obj_instance_name )
{
   TrickHLA::Object *obj = manager.get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::wait_to_receive_zero_lookahead_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'" << endl;
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
      time_management_srvc.wait_for_zero_lookahead_TARA_TAG();

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
                      << " the Central RTI Component (CRC) level!" << endl;
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "Federate::wait_to_receive_zero_lookahead_data():%d Waiting...\n",
                             __LINE__ );
         }

         // The TARA will cause zero-lookahead data to be reflected before the TAG.
         time_management_srvc.wait_for_zero_lookahead_TARA_TAG();
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
   TrickHLA::Object *obj = manager.get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::send_blocking_io_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'" << endl;
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
   TrickHLA::Object *obj = manager.get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::wait_to_receive_blocking_io_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'" << endl;
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
                      << " the Central RTI Component (CRC) level!" << endl;
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

/*! @brief Is the federate connected to the RTI.
 *  @param connected_state True if the federate is connected; False otherwise. */
void Federate::set_connection_lost()
{
   this->connected = false;

   ostringstream errmsg;
   errmsg << "Federate::set_connection_lost():" << __LINE__
          << " ERROR: Lost the connection to the RTI. Terminating the simulation!"
          << endl;
   DebugHandler::terminate_with_message( errmsg.str() );
}

/*!
 *  @job_class{scheduled}
 */
bool Federate::is_execution_member() // cppcheck-suppress [functionStatic, unmatchedSuppression]
{
   if ( connected && ( RTI_ambassador.get() != NULL ) ) {
      bool is_exec_member = true;
      try {
         RTI_ambassador->getOrderName( RTI1516_NAMESPACE::TIMESTAMP );
      } catch ( RTI1516_NAMESPACE::InvalidOrderType const &e ) {
         // Do nothing
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         is_exec_member = false;
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         is_exec_member = false;
         set_connection_lost();
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
   if ( is_shutdown_called() ) {
      return;
   }
   this->shutdown_called = true;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::shutdown():%d\n", __LINE__ );
   }

#if defined( TRICKHLA_COLLECT_TAG_STATS )
   double const  tag_wait_time     = (double)tag_wait_sum / exec_get_time_tic_value();
   double const  avg_tag_wait_time = ( tag_wait_count != 0 )
                                        ? ( tag_wait_time / tag_wait_count )
                                        : tag_wait_time;
   ostringstream tag_msg;
   tag_msg << "Federate::shutdown():" << __LINE__ << endl
           << "Total # waits for TAG:" << tag_wait_count << endl
           << "  Total TAG wait time:" << tag_wait_time << " seconds" << endl
           << "Average TAG wait time:" << avg_tag_wait_time << " seconds" << endl;
   message_publish( MSG_INFO, tag_msg.str().c_str() );
#endif // TRICKHLA_COLLECT_TAG_STATS

#ifdef TRICKHLA_CHECK_SEND_AND_RECEIVE_COUNTS
   for ( int i = 0; i < manager.obj_count; ++i ) {
      ostringstream msg1;
      msg1 << "Federate::shutdown():" << __LINE__
           << " Object[" << i << "]:'" << manager.objects[i].get_name() << "'"
           << " send_count:" << manager.objects[i].send_count
           << " receive_count:" << manager.objects[i].receive_count << endl;
      message_publish( MSG_INFO, msg1.str().c_str() );
   }
#endif // TRICKHLA_CHECK_SEND_AND_RECEIVE_COUNTS

#ifdef TRICKHLA_CYCLIC_READ_TIME_STATS
   for ( int i = 0; i < manager.obj_count; ++i ) {
      ostringstream msg2;
      msg2 << "Federate::shutdown():" << __LINE__
           << " Object[" << i << "]:'" << manager.objects[i].get_name() << "' "
           << manager.objects[i].elapsed_time_stats.to_string() << endl;
      message_publish( MSG_INFO, msg2.str().c_str() );
   }
#endif // TRICKHLA_CYCLIC_READ_TIME_STATS

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Check for Execution Control shutdown. If this is NULL, then we are
   // probably shutting down prior to initialization.
   if ( this->execution_control != NULL ) {
      // Call Execution Control shutdown method.
      execution_control->shutdown();
   }

   // Disable Time Constrained and Time Regulation for this federate.
   time_management_srvc.shutdown_time_management();

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
#endif // FPU_CW_PROTECTION
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
                          __LINE__, get_federation_name().c_str() );
      }

      if ( is_execution_member() ) {
         RTI_ambassador->resignFederationExecution( RTI1516_NAMESPACE::CANCEL_THEN_DELETE_THEN_DIVEST );

         this->federation_joined = false;

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::resign():%d Resigned from Federation '%s'\n",
                             __LINE__, get_federation_name().c_str() );
         }
      } else {
         message_publish( MSG_NORMAL, "Federate::resign():%d Not execution member of Federation '%s'\n",
                          __LINE__, get_federation_name().c_str() );
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
             << "InvalidResignAction" << endl;

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
             << "OwnershipAcquisitionPending" << endl;

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
             << "FederateNotExecutionMember" << endl;

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
             << "NotConnected" << endl;

      // Just display an error message and don't terminate if we are not connected.
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      set_connection_lost();
   } catch ( CallNotAllowedFromWithinCallback const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "CallNotAllowedFromWithinCallback" << endl;

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
             << rti_err_msg << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
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
             << rti_err_msg << endl;

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
                          __LINE__, get_federation_name().c_str() );
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
             << "InvalidResignAction" << endl;

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
             << "OwnershipAcquisitionPending" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( FederateOwnsAttributes const &e ) {
      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation received an EXCEPTION: "
             << "FederateOwnsAttributes" << endl;

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
             << "FederateNotExecutionMember" << endl;

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
             << "NotConnected" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
      set_connection_lost();
   } catch ( CallNotAllowedFromWithinCallback const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
             << " ERROR: Failed to resign Federate from the '"
             << get_federation_name()
             << "' Federation because it received an EXCEPTION: "
             << "CallNotAllowedFromWithinCallback" << endl;

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
             << rti_err_msg << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
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
             << rti_err_msg << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // TODO: Do we really want to terminate here! DDexter 9/27/2010
   ostringstream errmsg;
   errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
          << " ERROR: Federate '" << get_federate_name()
          << "' resigned from Federation '" << get_federation_name() << "'" << endl;
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
         message_publish( MSG_NORMAL, "Federate::destroy():%d Attempting to Destroy Federation '%s'.\n",
                          __LINE__, get_federation_name().c_str() );
      }

      RTI_ambassador->destroyFederationExecution( federation_name_ws );

      this->federation_exists = false;
      this->federation_joined = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy():%d Destroyed Federation '%s'.\n",
                          __LINE__, get_federation_name().c_str() );
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
                          __LINE__, get_federation_name().c_str() );
      }
   } catch ( RTI1516_NAMESPACE::FederationExecutionDoesNotExist const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->federation_exists = false;
      this->federation_joined = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "Federate::destroy():%d Federation '%s' Already Destroyed.\n",
                          __LINE__, get_federation_name().c_str() );
      }
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      this->federation_exists = false;
      this->federation_joined = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "Federate::destroy():%d Federation '%s' destroy failed because we are NOT CONNECTED to the federation.\n",
                          __LINE__, get_federation_name().c_str() );
      }
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
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
             << rti_err_msg << "'" << endl;

      DebugHandler::terminate_with_message( errmsg.str() );
   }

   try {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy():%d Attempting to disconnect from RTI.\n",
                          __LINE__ );
      }

      RTI_ambassador->disconnect();
      this->federation_exists = false;
      this->federation_joined = false;
      this->connected         = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy():%d Disconnected from RTI.\n",
                          __LINE__ );
      }
   } catch ( RTI1516_NAMESPACE::FederateIsExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "Federate::destroy():%d Cannot disconnect from RTI because this federate is still an execution member.\n",
                          __LINE__ );
      }
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::destroy():" << __LINE__
             << " ERROR: Unexpected RTI exception when disconnecting from RTI!\n"
             << "RTI Exception: RTIinternalError: '"
             << rti_err_msg << "'" << endl;
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
             << " ERROR: Unexpected NULL RTIambassador." << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Create the wide-string version of the federation name.
   wstring federation_name_ws;
   StringUtilities::to_wstring( federation_name_ws, federation_name );

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::destroy_orphaned_federation():%d Attempting to Destroy Orphaned Federation '%s'.\n",
                       __LINE__, get_federation_name().c_str() );
   }

   try {
      RTI_ambassador->destroyFederationExecution( federation_name_ws );

      // If we don't get an exception then we successfully destroyed
      // an orphaned federation.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::destroy_orphaned_federation():%d Successfully Destroyed Orphaned Federation '%s'.\n",
                          __LINE__, get_federation_name().c_str() );
      }
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
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
   // Check for a empty current federation name or a self assigned name.
   if ( federation_name.empty() || ( federation_name != exec_name ) ) {

      // Check for an empty (i.e. zero length) name.
      if ( !exec_name.empty() ) {
         // Set the federation execution name as a copy.
         this->federation_name = exec_name;
      } else {
         // Set to a default value if not already set in the input stream.
         if ( federation_name.empty() ) {
            this->federation_name = "TrickHLA Federation";
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

   SleepTimeout print_timer( this->wait_status_time );
   SleepTimeout sleep_timer;

   while ( this->auto_provide_setting < 0 ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      // Sleep a little while to wait for the information to update.
      sleep_timer.sleep();

      if ( this->auto_provide_setting < 0 ) {

         // To be more efficient, we get the time once and share it.
         int64_t wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::ask_MOM_for_auto_provide_setting():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
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
      string auto_provide_status = get_auto_provide_status_string( auto_provide_setting );
      message_publish( MSG_NORMAL, "Federate::ask_MOM_for_auto_provide_setting():%d Auto-Provide:%s value:%d\n",
                       __LINE__, auto_provide_status.c_str(), auto_provide_setting );
   }

   fedMomAttributes.clear();
   requestedAttributes.clear();
}

void Federate::enable_MOM_auto_provide_setting(
   bool enable )
{
   // Keep the auto-provide setting in sync with our enable request.
   this->auto_provide_setting = enable ? 1 : 0;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string auto_provide_status = get_auto_provide_status_string( auto_provide_setting );
      message_publish( MSG_NORMAL, "Federate::enable_MOM_auto_provide_setting():%d Auto-Provide:%s\n",
                       __LINE__, auto_provide_status.c_str() );
   }

   publish_interaction_class( MOM_HLAsetSwitches_class_handle );

   ParameterHandleValueMap param_values_map;
   try {
      // HLAautoProvide attribute is an HLAswitch, which is an HLAinteger32BE.
      HLAinteger32BE auto_provide_encoder( auto_provide_setting );

      param_values_map[MOM_HLAautoProvide_param_handle] = auto_provide_encoder.encode();

   } catch ( RTI1516_NAMESPACE::EncoderException &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::enable_MOM_auto_provide_setting():" << __LINE__
             << " ERROR: Encoder exception '" << rti_err_msg << "'"
             << " trying to encode auto-provide switch setting (HLAautoProvide)"
             << " for value " << auto_provide_setting << "!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

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
   this->orig_auto_provide_setting = auto_provide_setting;

   // Disable Auto-Provide only if it is enabled or an unknown state (i.e. -1).
   if ( auto_provide_setting != 0 ) {
      enable_MOM_auto_provide_setting( false );
   }
}

void Federate::restore_orig_MOM_auto_provide_setting()
{
   // Only update the auto-provide setting if the original setting does not
   // match the current setting.
   if ( auto_provide_setting != orig_auto_provide_setting ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string auto_provide_status = get_auto_provide_status_string( orig_auto_provide_setting );
         message_publish( MSG_NORMAL, "Federate::restore_orig_MOM_auto_provide_setting():%d Auto-Provide:%s value:%d\n",
                          __LINE__, auto_provide_status.c_str(),
                          orig_auto_provide_setting );
      }
      enable_MOM_auto_provide_setting( orig_auto_provide_setting > 0 );
   }
}

void Federate::clear_known_feds()
{
   if ( this->known_feds != NULL ) {
      // Clear out the known_feds from memory...
      for ( int i = 0; i < known_feds_count; ++i ) {
         known_feds[i].MOM_instance_name = "";
         known_feds[i].name              = "";
      }
      if ( trick_MM->delete_var( static_cast< void * >( this->known_feds ) ) ) {
         message_publish( MSG_WARNING, "Federate::clear_known_feds():%d WARNING failed to delete Trick Memory for 'this->known_feds'\n",
                          __LINE__ );
      }
      this->known_feds = NULL;
   }
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
              << handle_str << endl;
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }
}

void Federate::remove_MOM_HLAfederate_instance_id(
   ObjectInstanceHandle const &instance_hndl )
{
   remove_federate_instance_id( instance_hndl );
   remove_MOM_HLAfederation_instance_id( instance_hndl );

   string tMOMName  = "";
   string tFedName  = "";
   bool   foundName = false;

   TrickHLAObjInstanceNameMap::iterator iter = MOM_HLAfederate_instance_name_map.find( instance_hndl );
   if ( iter != MOM_HLAfederate_instance_name_map.end() ) {
      StringUtilities::to_string( tMOMName, iter->second );
      foundName = true;
      MOM_HLAfederate_instance_name_map.erase( iter );

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, instance_hndl );

         ostringstream summary;
         summary << "Federate::remove_MOM_HLAfederate_instance_id():" << __LINE__
                 << " Object '" << tMOMName << "', with Instance Handle:"
                 << handle_str << endl;
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }
   }

   // If the federate_id was not found, there is nothing else to do so exit the routine...
   if ( !foundName ) {
      return;
   }

   // Search for the federate information from running_feds...
   foundName = false;
   for ( size_t i = 0; i < save_restore_srvc.running_feds_count; ++i ) {
      if ( save_restore_srvc.running_feds[i].MOM_instance_name != tMOMName ) {
         foundName = true;
         tFedName  = save_restore_srvc.running_feds[i].name;
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
      alloc_type( (int)( save_restore_srvc.running_feds_count - 1 ), "TrickHLA::KnownFederate" ) );
   if ( tmp_feds == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::remove_MOM_HLAfederate_instance_id():" << __LINE__
             << " ERROR: Could not allocate memory for tmp_feds!" << endl;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // now, copy everything minus the requested name from the original list...
   int tmp_feds_cnt = 0;
   for ( size_t i = 0; i < save_restore_srvc.running_feds_count; ++i ) {
      // if the name is not the one we are looking for...
      if ( save_restore_srvc.running_feds[i].name != tFedName ) {
         if ( save_restore_srvc.running_feds[i].MOM_instance_name.empty() ) {
            tmp_feds[tmp_feds_cnt].MOM_instance_name = save_restore_srvc.running_feds[i].MOM_instance_name;
         }
         tmp_feds[tmp_feds_cnt].name     = save_restore_srvc.running_feds[i].name;
         tmp_feds[tmp_feds_cnt].required = save_restore_srvc.running_feds[i].required;
         ++tmp_feds_cnt;
      }
   }

   // now, clear out the original memory...
   save_restore_srvc.clear_running_feds();

   // assign the new element count into running_feds_count.
   save_restore_srvc.running_feds_count = tmp_feds_cnt;

   // assign pointer from the temporary list to the permanent list...
   save_restore_srvc.running_feds = tmp_feds;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_hndl );
      message_publish( MSG_INFO, "Federate::remove_MOM_HLAfederate_instance_id():%d \
Removed Federate '%s' Instance-ID:%s Valid-ID:%s\n",
                       __LINE__, tFedName.c_str(), id_str.c_str(),
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
              << " Object Instance:" << id_str << endl;
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
                 << " Object Instance:" << handle_str << endl;
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }
   }
}

bool Federate::is_federate_executing() const
{
   // Check if the manager has set a flag that the federate initialization has
   // completed and the federate is now executing.
   return execution_control->execution_has_begun;
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
         message_publish( MSG_WARNING, "Federate::set_federation_instance_attributes():%d WARNING: Unknown object class, expected 'HLAmanager.HLAfederation'.\n",
                          __LINE__ );
      }
      return;
   }

   AttributeHandleValueMap::const_iterator attr_iter;
   for ( attr_iter = values.begin(); attr_iter != values.end(); ++attr_iter ) {

      if ( attr_iter->first == MOM_HLAautoProvide_handle ) {
         try {
            // HLAautoProvide attribute is an HLAswitch, which is an HLAinteger32BE.
            // Decode directly into the auto_provide_setting variable.
            HLAinteger32BE auto_provide_encoder( &auto_provide_setting );

            auto_provide_encoder.decode( attr_iter->second );

         } catch ( RTI1516_NAMESPACE::EncoderException &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            ostringstream errmsg;
            errmsg << "Federate::set_federation_instance_attributes():" << __LINE__
                   << " ERROR: Encoder exception '" << rti_err_msg << "'"
                   << " trying to decode auto-provide switch setting"
                   << " (HLAautoProvide)!" << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            string auto_provide_status = get_auto_provide_status_string( auto_provide_setting );
            message_publish( MSG_NORMAL, "Federate::set_federation_instance_attributes():%d Auto-Provide:%s value:%d\n",
                             __LINE__, auto_provide_status.c_str(),
                             auto_provide_setting );
         }

      } else if ( attr_iter->first == MOM_HLAfederatesInFederation_handle ) {

         // HLAfederatesInFederation has a data type of HLAfederateReferenceList,
         // which is an HLAvariableArray of HLAfederateReferences.
         // HLAfederateReference is a HLAfederateHandle representation that is
         // an HLAvariableArray of HLAbyte elements.
         try {
            HLAbyte          byte_proto;
            HLAvariableArray fed_handle_proto( byte_proto );
            HLAvariableArray feds_list( fed_handle_proto );

            feds_list.decode( attr_iter->second );

            // Since this list of federate id's is current, there is no reason
            // to thrash the RTI and chase down each federate handle ID and
            // convert into a name. The wait_for_required_federates_to_join()
            // method already queries the names from the RTI for all required
            // federates. We will eventually utilize the same MOM interface to
            // rebuild this list.
            save_restore_srvc.running_feds_count = feds_list.size();

         } catch ( RTI1516_NAMESPACE::EncoderException &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            ostringstream errmsg;
            errmsg << "Federate::set_federation_instance_attributes():" << __LINE__
                   << " ERROR: Encoder exception '" << rti_err_msg << "'"
                   << " trying to decode HLAfederatesInFederation variable array!"
                   << endl;
            DebugHandler::terminate_with_message( errmsg.str() );
         }

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::set_federation_instance_attributes():%d Found a FederationID list with %d elements.\n",
                             __LINE__, save_restore_srvc.running_feds_count );
         }
      }
   }
}

void Federate::restore_federate_handles_from_MOM()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::restore_federate_handles_from_MOM:%d\n",
                       __LINE__ );
   }

   // Make sure that we are in federate handle rebuild mode...
   federate_ambassador.set_federation_restored_rebuild_federate_handle_set();

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
         all_found = ( joined_federate_handles.size() >= save_restore_srvc.running_feds_count );
      }

      if ( !all_found ) {

         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         // To be more efficient, we get the time once and share it.
         int64_t wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Federate::restore_federate_handles_from_MOM():" << __LINE__
                      << " ERROR: Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!" << endl;
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
   federate_ambassador.reset_federation_restored_rebuild_federate_handle_set();

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

      FederateHandle fed_handle = decode_federate_handle( attr_iter->second );

      // Concurrency critical code section because joined-federate state is changed
      // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
      // function.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &joined_federate_mutex );

         // Add this FederateHandle to the set of joined federates.
         joined_federate_handles.insert( fed_handle );
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string id_str, fed_id;
         StringUtilities::to_string( id_str, instance_hndl );
         StringUtilities::to_string( fed_id, fed_handle );
         message_publish( MSG_NORMAL, "Federate::rebuild_federate_handles():%d Federate OID:%s Federate-ID:%s\n",
                          __LINE__, id_str.c_str(), fed_id.c_str() );
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
            size_t found = fed_name.find( required_fed_name );
            if ( found != wstring::npos ) {
               // found the "required federate name" inside the supplied federate name
               return true;
            }
         }
      }
   }
   return false;
}
