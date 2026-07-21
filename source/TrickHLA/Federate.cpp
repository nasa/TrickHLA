/*!
@file TrickHLA/Federate.cpp
@ingroup TrickHLA
@brief This class provides basic services for an HLA federate.

@copyright Copyright 2026 United States Government as represented by the
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
@trick_link_dependency{InteractionServices.cpp}
@trick_link_dependency{ObjectServices.cpp}
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
@rev_entry{Edwin Z. Crues, Titan Systems Corp., DIS, Jan 2004, --, Initial investigation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support & test.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, May 2026, --, Adjustments for SaveRestore support.}
@revs_end

*/

// System include files.
#include <algorithm>
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
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/sim_mode.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/InteractionServices.hh"
#include "TrickHLA/KnownFederate.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/ObjectServices.hh"
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
#include "RTI/RTIambassador.h"
#include "RTI/RTIambassadorFactory.h"
#include "RTI/Typedefs.h"
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/EncodingExceptions.h"
#include "RTI/encoding/HLAopaqueData.h"
#include "RTI/encoding/HLAvariableArray.h"

#if defined( IEEE_1516_2025 )
#   include "RTI/RtiConfiguration.h"
#else
#   pragma GCC diagnostic pop
#endif // IEEE_1516_2025

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

//--------------------------------------------------------------------
// Joined federate update process state enumeration support functions.
//--------------------------------------------------------------------
std::string TrickHLA::to_string( THLAFederateUpdateProcessEnum update_state )
{
   switch ( update_state ) {
      case THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_NONE:
         return ( "FEDERATE_UPDATE_NONE" );
         break;
      case THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_ACTIVATE:
         return ( "FEDERATE_UPDATE_ACTIVATE" );
         break;
      case THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_INITIATED:
         return ( "FEDERATE_UPDATE_INITIATED" );
         break;
      case THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_RECEIVED:
         return ( "FEDERATE_UPDATE_RECEIVED" );
         break;
      case THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_IN_PROGRESS:
         return ( "FEDERATE_UPDATE_IN_PROGRESS" );
         break;
      case THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_COMPLETE:
         return ( "FEDERATE_UPDATE_COMPLETE" );
         break;
      case THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_FAILED:
         return ( "FEDERATE_UPDATE_FAILED" );
         break;
      default:
         return ( "FEDERATE_UPDATE_UNKNOWN" );
   }
   return ( "FEDERATE_UPDATE_UNKNOWN" );
}

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
     debug_level( TrickHLA::DEBUG_LEVEL_NO_TRACE ),
     code_section( TrickHLA::DEBUG_SOURCE_ALL_MODULES ),
     can_rejoin_federation( false ),
     federation_created_by_federate( false ),
     federation_exists( false ),
     federation_joined( false ),
     all_federates_joined( false ),
     connected( false ),
     shutdown_called( false ),
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
     joined_federate_mutex(),
     joined_federates_map(),
     federate_update_state( THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_NONE ),
     federate_ambassador( *this ),
     time_management_service( *this ),
     object_service( *this ),
     save_restore_service( *this ),
     interaction_service( *this ),
     execution_control( NULL ),
     execution_config( NULL ),
     hla_logical_time( 0 ),
     hlt_seconds( 0.0 ),
     elapsed_time( 0.0 ),
     scenario_time( 0.0 )
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

   // Clear the list of known federates.
   known_federates.clear();

   // Clear the list of joined federates.
   joined_federates_map.clear();

   // Clear the MOM HLAfederation instance name map.
   MOM_HLAfederation_instance_name_map.clear();

   // Make sure we destroy the mutex.
   time_management_service.time_adv_state_mutex.destroy();
   joined_federate_mutex.destroy();

   return;
}

/*!
 * @job_class{initialization}
 */
void Federate::print_version()
{
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::print_version():" << __LINE__ << endl
          << "     TrickHLA-version:'" << Utilities::get_version() << "'" << endl
          << "TrickHLA-release-date:'" << Utilities::get_release_date() << "'" << endl
          << "             RTI-name:'" << Utilities::get_rti_name() << "'" << endl
          << "          RTI-version:'" << Utilities::get_rti_version() << "'" << endl;
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
#endif // FPU_CW_PROTECTION

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

   // Set the association in the Federate Ambassador.
   federate_ambassador.execution_control = this->execution_control;

   // Set the Federate execution configuration.
   this->execution_config = &federate_execution_config;

   // Register the ExecutionControl instance with the TrickHLA::SaveRestoreServices instance.
   this->save_restore_service.execution_control = &federate_execution_control;

   // Set up the TrickHLA::ExecutionControl instance.
   this->execution_control->setup( *this );
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
   time_management_service.refresh_HLA_time_constants();
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
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Make sure the federate name has been specified.
   if ( name.empty() ) {
      ostringstream errmsg;
      errmsg << "Federate::initialize():" << __LINE__
             << " ERROR: Unexpected NULL federate name." << endl;
      DebugHandler::terminate( errmsg.str() );
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

   object_service.verify_object_arrays();
   interaction_service.verify_interaction_arrays();

   /*
    * Set the HLA Save directory for HLA Save and Restore.
    * This may like an odd way to set that directory.  However, if the
    * directory is set in the input file, it will only test for the
    * existence of the directory.  If it is not set in the input file, it
    * will pull the defaults from the Trick simulation execution.
    */
   save_restore_service.set_HLA_save_directory( save_restore_service.get_HLA_save_directory() );

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

   time_management_service.restart_initialization();

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Verify the federate name.
   if ( name.empty() ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: NULL or zero length Federate Name." << endl;
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
   }

   // Verify the Federation execution name.
   if ( federation_name.empty() ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: Invalid Federate Execution Name."
             << " Please check your input.py or modified-data files to make sure"
             << " the 'federation_name' is correctly specified." << endl;
      DebugHandler::terminate( errmsg.str() );
   }

   // Check if there are known Federate in the Federation.
   if ( enable_known_feds ) {

      // Only need to do anything if there are known federates.
      if ( known_federates.empty() ) {
         // If we are enabling known federates, then there probably should be some.
         ostringstream errmsg;
         errmsg << "Federate::restart_initialization():" << __LINE__
                << " ERROR: No Known Federates Specified for the Federation." << endl;
         DebugHandler::terminate( errmsg.str() );
      }

      if ( known_federates.size() >= INT_MAX ) {
         ostringstream errmsg;
         errmsg << "Federate::restart_initialization():" << __LINE__
                << " ERROR: Known Federates count (" << known_federates.size()
                << ") is >= " << INT_MAX << "!" << endl;
         DebugHandler::terminate( errmsg.str() );
      }

      // Validate the name of each Federate known to be in the Federation.
      for ( size_t i = 0; i < known_federates.size(); ++i ) {

         // A zero length Federate name is not allowed.
         if ( known_federates[i].name.empty() ) {
            ostringstream errmsg;
            errmsg << "Federate::restart_initialization():" << __LINE__
                   << " ERROR: Invalid name of known Federate at array index: "
                   << i << endl;
            DebugHandler::terminate( errmsg.str() );
         }
      }
   }

   // Setup the Execution Control and Execution Configuration objects now that
   // we know if we are the "Master" federate or not.
   if ( this->execution_control == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::restart_initialization():" << __LINE__
             << " ERROR: Unexpected NULL 'execution_control' pointer!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Verify the user specified object and interaction arrays and counts.
   object_service.verify_object_arrays();
   interaction_service.verify_interaction_arrays();

   // The set_master() function set's additional parameter so call it again to
   // force the a complete master state.
   bool const master_flag = execution_control->is_master();
   execution_control->set_master( master_flag );

   // Setup all the Trick Ref-Attributes for the user specified objects,
   // attributes, interactions and parameters.
   object_service.setup_object_ref_attributes();
   interaction_service.setup_interaction_ref_attributes();

   // Only continue the restart initialization if the Federate is an
   // execution member and connected.
   if ( is_execution_member() ) {

      // Setup all the RTI handles for the objects, attributes and interaction
      // parameters.
      object_service.setup_object_RTI_handles();
      interaction_service.setup_interaction_RTI_handles();

      // Set the object instance handles based on its name.
      object_service.set_all_object_instance_handles_by_name();

      // Make sure we reinitialize the MOM interface handles.
      initialize_MOM_handles();

      // Perform the next few steps if we are the Master federate.
      if ( execution_control->is_master() ) {

         // Make sure all the federate instance handles are reset based on
         // the federate name so that the wait for required federates will work
         // after a checkpoint reload.
         set_all_federate_MOM_instance_handles_by_name();

         // Make sure all required federates have joined the federation.
         wait_for_required_federates_to_join();
      }

      // TODO: Should this even be called here because the checkpoint restore
      // should have already been called before we got here.
      // Restore ownership_transfer data for all objects.
      for ( int n = 0; n < object_service.obj_count; ++n ) {
         object_service.objects[n].restore_data_after_checkpoint();
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
   if ( !time_management_service.verify_time_constraints() ) {
      ostringstream errmsg;
      errmsg << "Federate::pre_multiphase_initialization():" << __LINE__
             << " ERROR: Time Constraints verification failed!" << endl;
      DebugHandler::terminate( errmsg.str() );
   }

   // Perform the Execution Control specific pre-multi-phase initialization.
   execution_control->pre_multi_phase_init_processes();

   // Debug printout.
   if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::pre_multiphase_initialization():%d\n     Completed pre-multiphase initialization...\n",
                       __LINE__ );
   }
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
   bool const trick_sigfpe_is_set = ( exec_get_trap_sigfpe() > 0 );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( trick_sigfpe_is_set ) {
      exec_set_trap_sigfpe( true );
   }
}

bool Federate::is_RTI_ready(
   string const &method_name )
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   bool rti_valid = true;
   if ( get_RTI_ambassador() == NULL ) {
      message_publish( MSG_WARNING, "Federate::%s:%d Unexpected NULL RTIambassador!\n",
                       method_name.c_str(), __LINE__ );
      rti_valid = false;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return rti_valid;
}

void Federate::add_joined_federate(
   ObjectInstanceHandle const &instance_hndl,
   wstring const              &instance_name )
{
   // Only add the federate instance if not already present.
   if ( !is_joined_federate_by_object_handle( instance_hndl ) ) {

      // Add a new entry into the joined federates list.
      // NOTE: This entry isn't completely filled out yet.
      KnownFederate known_federate;
      known_federate.object_instance_handle = instance_hndl;
      known_federate.MOM_instance_name      = instance_name;
      joined_federates_map[instance_hndl]   = known_federate;

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, instance_hndl );
         string name_str;
         StringUtilities::to_string( name_str, instance_name );

         ostringstream summary;
         summary << "Federate::add_joined_federate():" << __LINE__
                 << " Object '" << name_str << "', with Instance Handle:"
                 << handle_str << endl;
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }
   }

   return;
}

void Federate::remove_joined_federate(
   ObjectInstanceHandle const &instance_hndl )
{
   // If this isn't a joined federate, then just return.
   if ( !is_joined_federate_by_object_handle( instance_hndl ) ) {
      return;
   }
   // Concurrency critical code section because joined-federate state is changed
   // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
   // function.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

      // Find the federate in the joined federate map and remove it.
      KnownFederateMap::iterator iter;
      iter = joined_federates_map.find( instance_hndl );
      if ( iter != joined_federates_map.end() ) {

         joined_federates_map.erase( iter );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            string handle_str;
            StringUtilities::to_string( handle_str, instance_hndl );

            ostringstream summary;
            summary << "Federate::remove_joined_federate():" << __LINE__
                    << " Object Instance:" << handle_str << endl;
            message_publish( MSG_NORMAL, summary.str().c_str() );
         }
      }
   }

   return;
}

/*! @brief Decode the specified encoded Federate Handle.
 *  @return Federate Handle.
 *  @param enc_handle encoded Federate Handle */
FederateHandle Federate::decode_federate_handle(
   VariableLengthData const &enc_handle )
{
   // Handles defined by the MOM interface have a an encoding of
   // HLAvariableArray, which is different than the Handles returned
   // by the RTI-ambassador with the encoding of VariableLengthData.
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
      exit( 1 );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::decode_federate_handle():" << __LINE__
             << " ERROR: When decoding 'FederateHandle': EXCEPTION: FederateNotExecutionMember" << endl;
      DebugHandler::terminate( errmsg.str() );
      exit( 1 );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      ostringstream errmsg;
      errmsg << "Federate::decode_federate_handle():" << __LINE__
             << " ERROR: When decoding 'FederateHandle': EXCEPTION: NotConnected" << endl;
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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

wstring Federate::get_federate_MOM_name( KnownFederate const &federate )
{
   wstring federate_MOM_name;

   // Sanity check to make sure we have an RTI ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::get_federate_MOM_name():" << __LINE__
             << " Unexpected NULL RTIambassador." << endl;
      DebugHandler::terminate( errmsg.str() );
      return ( federate_MOM_name );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   try {

      federate_MOM_name = rti_amb->getObjectInstanceName( federate.object_instance_handle );

   } catch ( ObjectInstanceNotKnown const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Federate::get_federate_MOM_name():%d rti_amb->getObjectInstanceName() ERROR: ObjectInstanceNotKnown\n",
                       __LINE__ );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Federate::get_federate_MOM_name():%d rti_amb->getObjectInstanceName() ERROR: FederateNotExecutionMember\n",
                       __LINE__ );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      message_publish( MSG_WARNING, "Federate::get_federate_MOM_name():%d rti_amb->getObjectInstanceName() ERROR: NotConnected\n",
                       __LINE__ );
      set_connection_lost();
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Federate::get_federate_MOM_name():%d rti_amb->getObjectInstanceName() ERROR: RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      string id_str;
      StringUtilities::to_string( id_str, federate.MOM_instance_name );
      string fed_name_str;
      StringUtilities::to_string( fed_name_str, federate.name );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::get_federate_MOM_name():" << __LINE__
             << " ERROR: Exception getting MOM instance name for '"
             << fed_name_str << "' ID:" << id_str
             << " '" << rti_err_msg << "'." << endl;
      DebugHandler::terminate( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return ( federate_MOM_name );
}

void Federate::set_MOM_HLAfederate_instance_attributes(
   ObjectInstanceHandle const    &handle,
   AttributeHandleValueMap const &values )
{

   // Concurrency critical code section because joined-federate state used by
   // the blocking Federate::wait_for_required_federates_to_join() function.
   //
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

   // Add the federate handle if we don't know about it already.
   if ( !is_joined_federate_by_object_handle( handle ) ) {
      add_joined_federate( handle );
   }

   // Get the associate joined federate reference.
   KnownFederate &joined_federate = joined_federates_map[handle];

   //
   // Let's get the federate name information.
   //

   // Find the Federate name for the given MOM federate Name attribute handle.
   AttributeHandleValueMap::const_iterator attr_iter = values.find( MOM_HLAfederateName_handle );

   // Determine if we have a federate name attribute.
   if ( attr_iter != values.end() ) {

      // Federate name is encoded into variable length data.
      VariableLengthData const &value = dynamic_cast< VariableLengthData const & >( attr_iter->second );

      // Decode the federate name that is encoded as a Unicode string.
      HLAunicodeString fed_name_unicode;
      fed_name_unicode.decode( value );
      joined_federate.name = wstring( fed_name_unicode );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, handle );
         string name_str;
         StringUtilities::to_string( name_str, joined_federate.name );
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d Federate-OID:%s Name:'%s' size:%d\n",
                          __LINE__, handle_str.c_str(), name_str.c_str(),
                          (int)joined_federate.name.size() );
      }
   }

   //
   // Let's determine if this is a required federate.
   //
   for ( KnownFederate const &known_fed : known_federates ) {
      if ( joined_federate.name == known_fed.name ) {
         joined_federate.required = known_fed.required;
      }
   }

   //
   // Let's get the federate type information.
   //

   // Find the Federate type for the given MOM federate Type attribute handle.
   attr_iter = values.find( MOM_HLAfederateType_handle );

   // Determine if we have a federate type attribute.
   if ( attr_iter != values.end() ) {

      // Federate type is encoded into variable length data.
      VariableLengthData const &value = attr_iter->second;

      // Decode the federate type that is encoded as a Unicode string.
      HLAunicodeString fed_type_unicode;
      fed_type_unicode.decode( value );
      joined_federate.type = wstring( fed_type_unicode );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, handle );
         string type_str;
         StringUtilities::to_string( type_str, joined_federate.type );
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d Federate-OID:%s Type'%s' size:%d\n",
                          __LINE__, handle_str.c_str(), type_str.c_str(),
                          (int)joined_federate.type.size() );
      }
   }

   //
   // Let's get the MOM federate handle.
   //

   // Find the FederateHandle attribute for the given MOM federate handle.
   attr_iter = values.find( MOM_HLAfederate_handle );

   // Determine if we have a federate handle attribute.
   if ( attr_iter == values.end() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, handle );
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d FederateHandle Not found for Federate-OID:%s\n",
                          __LINE__, handle_str.c_str() );
      }

   } else { // We have a federate handle so decode it.

      joined_federate.federate_handle = decode_federate_handle( attr_iter->second );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, handle );
         string fed_handle;
         StringUtilities::to_string( fed_handle, joined_federate.federate_handle );
         message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederate_instance_attributes():%d Federate-OID:%s Federate-ID:%s\n",
                          __LINE__, handle_str.c_str(), fed_handle.c_str() );
      }
   }

   return;
}

void Federate::set_all_federate_MOM_instance_handles_by_name()
{
   // Make sure the discovered federate instances list is cleared.
   // Concurrency critical code section because joined-federate state is changed
   // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
   // function.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

      // Clear the list of joined federates.
      joined_federates_map.clear();
   }

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::set_all_federate_MOM_instance_handles_by_name():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador." << endl;
      DebugHandler::terminate( errmsg.str() );
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

      for ( auto const &known_fed : known_federates ) {

         if ( !known_fed.MOM_instance_name.empty() ) {

            // Copy the MOM instance name for the exception messages below.
            fed_mom_instance_name_ws = known_fed.MOM_instance_name;

            // Get the instance handle based on the instance name.
            ObjectInstanceHandle const fed_mom_obj_instance_hdl =
               rti_amb->getObjectInstanceHandle( known_fed.MOM_instance_name );

            // Add the federate instance handle.
            add_joined_federate( fed_mom_obj_instance_hdl );

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
               string id_str;
               StringUtilities::to_string( id_str, fed_mom_obj_instance_hdl );
               string mom_str;
               StringUtilities::to_string( mom_str, known_fed.MOM_instance_name );
               string name_str;
               StringUtilities::to_string( name_str, known_fed.name );
               string type_str;
               StringUtilities::to_string( type_str, known_fed.type );
               summary << endl
                       << "    Federate:'" << name_str
                       << "' Type:'" << type_str
                       << "' MOM-Name: '" << mom_str
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
   KnownFederateMap::iterator fed_iter;

   // Iterate through the collection of joined federates.
   for ( fed_iter = joined_federates_map.begin();
         fed_iter != joined_federates_map.end();
         ++fed_iter ) {

      // Cast to the KnownFederate type.
      KnownFederate *joined_federate = static_cast< KnownFederate * >( &( fed_iter->second ) );

      // If we don't already have the MOM instance name, then get it.
      if ( joined_federate->MOM_instance_name.empty() ) {
         joined_federate->MOM_instance_name = get_federate_MOM_name( *joined_federate );
      }
   }

   return;
}

bool Federate::is_required_federate(
   wstring const &federate_name )
{
   for ( size_t i = 0; i < known_federates.size(); ++i ) {
      if ( known_federates[i].required ) {
         if ( federate_name == known_federates[i].name ) {
            return true;
         }
      }
   }
   return false;
}

bool Federate::is_joined_federate_by_federate_handle(
   FederateHandle const &handle )
{
   // Loop thru all joined_federate_map entries.
   KnownFederateMap::const_iterator map_iter;
   for ( map_iter = joined_federates_map.begin();
         map_iter != joined_federates_map.end();
         ++map_iter ) {

      // Get the associate joined federate reference.
      KnownFederate const &joined_federate = static_cast< KnownFederate const & >( map_iter->second );

      // Compare the federate handles.
      if ( handle == joined_federate.federate_handle ) {
         return true;
      }
   }
   return false;
}

bool Federate::is_joined_federate_by_object_handle(
   ObjectInstanceHandle const &handle )
{
   return ( joined_federates_map.find( handle ) != joined_federates_map.end() );
}

bool Federate::is_joined_federate_by_MOM_name(
   wstring const &MOM_name )
{
   // Loop thru all joined_federate_map entries.
   KnownFederateMap::const_iterator map_iter;
   for ( map_iter = joined_federates_map.begin();
         map_iter != joined_federates_map.end();
         ++map_iter ) {

      // Get the associate joined federate reference.
      KnownFederate const &joined_federate = static_cast< KnownFederate const & >( map_iter->second );

      // Compare the MOM instance names.
      if ( MOM_name == joined_federate.MOM_instance_name ) {
         return true;
      }
   }
   return false;
}

bool Federate::is_joined_federate_by_name(
   string const &federate_name )
{
   wstring fed_name_ws;
   StringUtilities::to_wstring( fed_name_ws, federate_name );
   return is_joined_federate_by_name( fed_name_ws );
}

bool Federate::is_joined_federate_by_name(
   wstring const &federate_name )
{
   // Loop thru all joined_federate_map entries.
   KnownFederateMap::iterator map_iter;
   for ( map_iter = joined_federates_map.begin();
         map_iter != joined_federates_map.end();
         ++map_iter ) {

      // Get the associate joined federate reference.
      KnownFederate const &joined_federate = static_cast< KnownFederate const & >( map_iter->second );

      // Compare the names.
      if ( federate_name == joined_federate.name ) {
         return true;
      }
   }
   return false;
}

/*!
 *  @job_class{scheduled}
 *  @detail This function will check the list of joined federates against
 *  the federates in the federates in Federation list received from the MOM
 *  Federation federatesInFederation interface.  It returns true
 *  if every joined federates has a match against an entry in the
 *  federatesInFederation list.  Otherwise, it will return false.
 */
bool Federate::check_joined_federates_match()
{
   bool success = true;

   // The first trivial check is if the number of joined federates is larger
   // than the federatesInFederation list.  This indicates that there is a
   // mismatch between the MOM reported federatesInFederation and the number
   // of federates we have already discovered.
   if ( joined_federates_map.size() > federate_handles.size() ) {
      return ( false );
   }

   // Concurrency critical code section because joined-federate state is changed
   // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
   // function.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

      // Iterate through the joined federates map checking for a match.
      for ( auto const &map_entry : joined_federates_map ) {

         // Get the known federate reference.
         KnownFederate const &joined_federate = map_entry.second;

         // First trivial check for valid federate handle.  If it's not valid,
         // this this joined federate is not complete.  That's a fail.
         if ( !joined_federate.federate_handle.isValid() ) {
            success = false;
            break;
         }

#if defined( TRICKHLA_USE_STL_ALGORITHM )
         // If no match was found for at least one federate in the federates
         // in Federation list, then mark this as a fail.
         if ( std::all_of( federate_handles.begin(), federate_handles.end(),
                           [&joined_federate]( auto fed_handle ) -> bool {
                              return ( fed_handle != joined_federate.federate_handle );
                           } ) ) {
            success = false;
         }
#else
         // Iterate through the federates in Federation list.
         bool found = false;
         for ( FederateHandle federate_handle : federate_handles ) { // NOLINT(misc-const-correctness)

            // Check for matching federate handle.
            if ( federate_handle == joined_federate.federate_handle ) { // cppcheck-suppress [useStlAlgorithm]
               found = true;
               break;
            }
         }

         // If no match was found for at least one federate in the federates
         // in Federation list, then mark this as a fail.
         if ( !found ) {
            success = false;
         }
#endif // TRICKHLA_USE_STL_ALGORITHM

         // Break out of the loop if the we find any fail.
         if ( !success ) {
            break;
         }

      } // End joined federates loop.
   }

   return ( success );
}

/*!
 *  @job_class{scheduled}
 *  @detail This function will check the list of federate in the Federation
 *  received from the MOM Federation federatesInFederation interface and
 *  compare it to the list of discovered joined federates.  It returns true
 *  if all the federates in the federatesInFederation list match exactly with
 *  discovered and completely filled out joined federates.  Otherwise, it
 *  will return false.
 */
bool Federate::verify_joined_federates()
{
   bool success = true;

   // Concurrency critical code section because joined-federate state is changed
   // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
   // function.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

      // Initial check is that the number of federates is the same.
      if ( federate_handles.size() != joined_federates_map.size() ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            ostringstream errmsg;
            errmsg << "Federate::verify_joined_federates():" << __LINE__
                   << ": There are " << joined_federates_map.size()
                   << " joined federates but expected "
                   << federate_handles.size() << "." << endl;
            message_publish( MSG_WARNING, errmsg.str().c_str() );
            std::wcout << list_joined_federates() << endl;
         }
         return ( false );
      }

      // Iterate through the federates in Federation list.
      for ( FederateHandle federate_handle : federate_handles ) { // NOLINT(misc-const-correctness)

         bool found = false;

         // Iterate through the joined federates map checking for a match.
         for ( auto const &map_entry : joined_federates_map ) {

            // Get the known federate reference.
            KnownFederate const &joined_federate = map_entry.second;

            // Only check against completely determined joined federates.
            if ( joined_federate.is_complete() ) {
               // Check for matching federate handle.
               if ( federate_handle == joined_federate.federate_handle ) {
                  found = true;
                  break;
               }
            } else {
               // An incomplete joined federate is an automatic fail.
               success = false;
               break;
            }

         } // End joined federates loop.

         // If no match was found then mark this as a fail.
         if ( !found ) {
            success = false;
         }

         // Break out of the loop if the we find any fail.
         if ( !success ) {
            break;
         }

      } // End federates in Federation loop.

   } // End mutex scope.

   // Print out an error message if check failed.
   if ( !success ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream errmsg;
         errmsg << "Federate::verify_joined_federates():" << __LINE__
                << ": Could not match joined federates with federates in Federation:" << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
         std::wcout << list_joined_federates() << endl;
      }
   }

   return ( success );
}

/*!
 *  @job_class{freeze}
 *  @detail This routine is designed to be called cyclicly as a freeze class
 *  job.
 */
void Federate::update_joined_federates()
{

   // Just return if the joined federate update process is not active.
   if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_NONE ) {
      return;
   }

   // Just return if the joined federate update process has failed.  If the
   // process failed, it needs to be reset before proceeding.
   if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_FAILED ) {
      return;
   }

   // Ask the MOM for the federate names if the the joined federate update
   // process is active.  Subscribe to Federate names using MOM interface and
   // request an update.
   if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_ACTIVATE ) {

      // Ask the MOM to get the federation information we need.
      ask_MOM_for_federation_info();

      // Mark the update process and having been initiated.
      federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_INITIATED;

      return;
   }

   // If the federates in Federation list hasn't been updated yet, then just return.
   // We're waiting for a callback to update the list of federatesInFederation.
   if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_INITIATED ) {
      return;
   }

   // The federatesInFederation list has been received.  Now we will check for
   // compatibility between the federates in the federatesInFederation list
   // against the joined federates.  Depending on the match status, we are
   // either finished or need to wait for more updates.
   if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_RECEIVED ) {

      // Ask the MOM to unsubscribe to the federation info.
      unsubscribe_from_MOM_federation_info();

      // Check if the federates in Federation all match with joined federates.
      // If so, we can mark the update process as complete.
      if ( verify_joined_federates() ) {

         // Mark that all federate have joined.
         all_federates_joined = true;

         // Joined federates are up to date, so the process is complete.
         federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_COMPLETE;

         return;
      }

      // If we have more joined federates than the federatesInFederation list
      // then something is wrong.  This indicates that there is a mismatch
      // between the MOM reported federatesInFederation and the number of
      // federates we have already discovered.
      if ( joined_federates_map.size() > federate_handles.size() ) {
         ostringstream errmsg;
         errmsg << "Federate::update_joined_federates():" << __LINE__
                << " ERROR: There are " << joined_federates_map.size()
                << " but only " << federate_handles.size()
                << " federate in the federatesInFederation list!"
                << endl;
         message_publish( MSG_ERROR, errmsg.str().c_str() );

         // Mark the update process as failed.
         federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_FAILED;
      }

      // Check if we may have discovered additional federates since the
      // last update.  Then we need to reactivate the MOM federate interface
      // and wait for all the federates to update.
      if ( check_joined_federates_match() ) {

         // Joined federates are NOT up to date, so wait for up dates.
         federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_IN_PROGRESS;

         // Ask the MOM to get the federate information.
         ask_MOM_for_federate_info();

         return;
      }

      // Something went wrong.
      ostringstream errmsg;
      errmsg << "Federate::update_joined_federates():" << __LINE__
             << " ERROR: The federatesInFederation list is not consistent with the joined federates list!"
             << endl;
      message_publish( MSG_ERROR, errmsg.str().c_str() );

      // Mark the update process as failed.
      federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_FAILED;

      return;
   }

   // The federates in the Federation list has been completed and we are now
   // checking if all the federates have been discovered and added into the
   // joined federates map.
   if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_IN_PROGRESS ) {

      bool all_found = true;

      // If we have more handles than joined federate, we know we haven't
      // found them all yet.
      if ( federate_handles.size() > joined_federates_map.size() ) {

         all_found = false;

      } // If we have more joined federates than federate handles,
        // something went wrong.
      else if ( federate_handles.size() < joined_federates_map.size() ) {

         ostringstream errmsg;
         errmsg << "Federate::update_joined_federates():" << __LINE__
                << " ERROR: Found " << federate_handles.size()
                << " in the federatesInFederation list but there are "
                << joined_federates_map.size()
                << " in the joined federates map!" << endl;
         message_publish( MSG_ERROR, errmsg.str().c_str() );

         // Mark the update process as failed.
         federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_FAILED;

         all_found = false;
      }

      // Iterate through the federates in Federation set to see if they have a
      // counterpart in the joined federates map.
      for ( FederateHandle fed_handle : federate_handles ) { // NOLINT(misc-const-correctness)
         if ( !is_joined_federate_by_federate_handle( fed_handle ) ) {
            all_found = false;
         }
      }

      // Mark the joined federate update process as complete if all the
      // federates in the Federation list have been discovered.
      if ( all_found ) {

         federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_COMPLETE;

         // Mark that all federate have joined.
         all_federates_joined = true;
      }

      return;
   }

   // The joined federate update process is marked as complete.
   if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_COMPLETE ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream errmsg;
         errmsg << "Federate::update_joined_federates():" << __LINE__
                << ": Federate Name, Type, Required:" << endl;
         message_publish( MSG_NORMAL, errmsg.str().c_str() );
         std::wcout << list_joined_federates() << endl;
      }

      // Unsubscribe from all attributes for the MOM HLAfederate class.
      unsubscribe_all_HLAfederate_class_attributes_from_MOM();

      // The MOM federate discovery process does not fill in the MOM object
      // instance names.  We need to get them from the RTI and complete the
      // information in the joined federates list.
      determine_federate_MOM_object_instance_names();
   }

   // Deactivate the joined federate update process.
   federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_NONE;

   return;
}

/*!
 *  @job_class{freeze}
 *  @detail This job will block until all the federates in the federation are
 *  discovered and updated in the joined_federates_map.
 */
void Federate::wait_for_joined_federates_update()
{
   THLAFederateUpdateProcessEnum prev_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_NONE;

   //
   // Use the update_joined_federates cyclic job.
   //

   // First activate the update process.
   federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_ACTIVATE;

   // Now enter a while loop waiting on completion or error.
   while ( federate_update_state != THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_NONE
           && federate_update_state != THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_FAILED ) {

      // Only print out debug information when the state changes.
      if ( prev_state != federate_update_state ) {
         if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            ostringstream errmsg;
            errmsg << "Federate::wait_for_joined_federates_update(): State "
                   << to_string( federate_update_state ) << endl;
            message_publish( MSG_NORMAL, errmsg.str().c_str() );
            prev_state = federate_update_state;
         }
      }

      // Call the update function.
      update_joined_federates();
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::wait_for_joined_federates_update(): Joined federates: " << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
      std::wcout << list_joined_federates() << endl;
   }

   return;
}

/*!
 *  @job_class{initialization}
 */
wstring Federate::list_joined_federates()
{
   wstringstream federates_summary;

   // Iterate through the joined federates map.
   KnownFederateMap::iterator map_iter;
   for ( map_iter = joined_federates_map.begin();
         map_iter != joined_federates_map.end(); ++map_iter ) {

      // Get the associate joined federate reference.
      KnownFederate const &joined_federate = static_cast< KnownFederate const & >( map_iter->second );

      // No end of line at the beginning.
      if ( map_iter != joined_federates_map.begin() ) {
         federates_summary << endl;
      }

      // List out the federate information.
      federates_summary << joined_federate.name;
      federates_summary << ", " << joined_federate.type;
      federates_summary << ", " << ( joined_federate.required ? "True" : "False" );
   }

   // Return the joined federate list as a wide string.
   return ( federates_summary.str() );
}

/*!
 *  @job_class{initialization}
 */
string Federate::wait_for_required_federates_to_join()
{
   string                    status_string;
   FederateObjectInstanceSet matched_federates;

   // If the known Federates list is disabled then just return.
   if ( !enable_known_feds ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_NORMAL, "Federate::wait_for_required_federates_to_join():%d Check for required Federates DISABLED.\n",
                          __LINE__ );
      }
      return status_string;
   }

#if defined( TRICKHLA_USE_STL_ALGORITHM )
   // Determine how many required federates we have.
   size_t num_required_feds = 0;
   std::for_each( known_federates.begin(), known_federates.end(),
                  [&num_required_feds]( auto const &known_fed ) {
                     if ( known_fed.required ) {
                        ++num_required_feds;
                     }
                  } );
#else
   // Determine how many required federates we have.
   size_t num_required_feds = 0;
   for ( auto const &known_fed : known_federates ) {
      if ( known_fed.required ) {
         ++num_required_feds; // cppcheck-suppress [useStlAlgorithm]
      }
   }
#endif // TRICKHLA_USE_STL_ALGORITHM

   // If we don't have any required Federates then return.
   if ( num_required_feds == 0 ) {
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
                           << "WAITING FOR " << num_required_feds
                           << " REQUIRED FEDERATES:";

      // Display the initial summary of the required federates we are waiting for.
      int cnt = 0;
      for ( size_t i = 0; i < known_federates.size(); ++i ) {
         // Create a summary of the required federates by name.
         if ( known_federates[i].required ) {
            ++cnt;
            std::string name_str;
            StringUtilities::to_string( name_str, known_federates[i].name );
            required_fed_summary << endl
                                 << "    " << cnt
                                 << ": Waiting for required federate '"
                                 << name_str << "'";
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
   ask_MOM_for_federate_info();

   bool joined_federate_change       = false;
   bool found_an_unrequired_federate = false;
   bool print_summary                = false;

   set< string > unrequired_federates_list; // list of unique unrequired federate names

   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   this->all_federates_joined = false;

   // Wait for all the required federates to join.
   while ( !this->all_federates_joined ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      // Sleep a little while we wait for more federates to join.
      sleep_timer.sleep();

      // Concurrency critical code section because joined-federate state is changed
      // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
      // function.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

         // Check for the possibility that a matched federate may have resigned.
         for ( RTI1516_NAMESPACE::ObjectInstanceHandle const &oih : matched_federates ) {
            KnownFederateMap::const_iterator map_iter;
            map_iter = joined_federates_map.find( oih );
            if ( map_iter == joined_federates_map.end() ) {
               // The matched federate is no longer joined.  Remove it.
               matched_federates.erase( oih );
               joined_federate_change = true;
            }
         }

         // Check for a newly joined federate and add is to the match list.
         for ( auto const &map_entry : joined_federates_map ) {
            KnownFederate const &federate = map_entry.second;
            // Only check against completely determined joined federates.
            if ( federate.is_complete() ) {
               FederateObjectInstanceSet::iterator set_iter;
               set_iter = matched_federates.find( federate.object_instance_handle );
               if ( set_iter == matched_federates.end() ) {
                  // Add the joined federate to the matched list.
                  matched_federates.insert( federate.object_instance_handle );
                  joined_federate_change = true;
               }
            }
         }

         // Only check if the matched joined federate list has changed.
         if ( joined_federate_change ) {

            // Reset the joined federate change state.
            joined_federate_change = false;

            // Reset the required federates counts.
            size_t required_fed_cnt = 0;

            // Loop thru all joined_federates_map entries.
            // Count the number of joined Required federates.
            for ( auto const &map_entry : joined_federates_map ) {

               // Get the associate joined federate reference.
               KnownFederate const &joined_federate = map_entry.second;

               // Only check against a completely determined joined federate.
               // Federate discovery is separate from the MOM interface that
               // is used to populate the information associated with a
               // discovered federate.  This means that a federate may be in
               // the joined federate list without having all it's federate
               // information complete.  This protects against that.
               if ( joined_federate.is_complete() ) {

                  if ( is_required_federate( joined_federate.name ) ) {

                     // Increment the required federate count.
                     ++required_fed_cnt;

                  } else {

                     found_an_unrequired_federate = true;
                     string fedname;
                     StringUtilities::to_string( fedname, joined_federate.name );
                     if ( save_restore_service.restore_state == THLARestoreProcessEnum::RESTORE_ACTIVATE ) {
                        if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
                           message_publish( MSG_NORMAL, "Federate::wait_for_required_federates_to_join():%d Found an UNREQUIRED federate %s!\n",
                                            __LINE__, fedname.c_str() );
                        }
                        unrequired_federates_list.insert( fedname );
                     }
                  }
               }
            }

            // Determine if all the Required federates have joined.
            if ( required_fed_cnt >= num_required_feds ) {
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
                    << "WAITING FOR " << num_required_feds
                    << " REQUIRED FEDERATES:";

            // Summarize the required federates first.
            int cnt = 0;
            for ( KnownFederate &known_fed : known_federates ) { // NOLINT(misc-const-correctness)
               ++cnt;
               std::string know_fed_str;
               StringUtilities::to_string( know_fed_str, known_fed.name );
               if ( known_fed.required ) {
                  if ( is_joined_federate_by_name( known_fed.name ) ) {
                     summary << endl
                             << "    " << cnt
                             << ": Found joined required federate '"
                             << know_fed_str << "'";
                  } else {
                     summary << endl
                             << "    " << cnt
                             << ": Waiting for required federate '"
                             << know_fed_str << "'";
                  }
               }
            }

            // Summarize all the remaining non-required joined federates.
            KnownFederateMap::const_iterator map_iter;
            for ( map_iter = joined_federates_map.begin();
                  map_iter != joined_federates_map.end();
                  ++map_iter ) {

               // Get the associate joined federate reference.
               KnownFederate const &joined_federate = static_cast< KnownFederate const & >( map_iter->second );

               if ( joined_federate.is_complete() && !joined_federate.required ) {
                  ++cnt;

                  // We need a string version of the wide-string federate name.
                  string fedname;
                  StringUtilities::to_string( fedname, joined_federate.name );

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

      // If not all the required federates have joined, then reset the timers
      // for the next loop.
      if ( !this->all_federates_joined ) {

         // To be more efficient, we get the time once and share it.
         int64_t const wallclock_time = sleep_timer.time();

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
               DebugHandler::terminate( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            print_summary = true;
         }
      }

   } // End while ( !this->all_federates_joined )

   // Once a list of joined federates has been built, and we are to restore the
   // checkpoint if there are any non-required federates. If any are found,
   // terminate the simulation with a verbose message stating which federates
   // were joined as non-required, as well as the required federates, so the user
   // knows what happened and know how to properly restart the federation. We
   // do this to inform the user that they did something wrong and gracefully
   // terminate the execution instead of the federation failing to restore
   // and the user is left to scratch their heads why the federation failed
   // to restore!
   if ( save_restore_service.restore_state == THLARestoreProcessEnum::RESTORE_ACTIVATE
        && found_an_unrequired_federate ) {
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
      std::string                   names;
      for ( cii = unrequired_federates_list.begin();
            cii != unrequired_federates_list.end(); ++cii ) {
         names += *cii + ", ";
      }
      names.resize( names.length() - 2 ); // remove trailing comma and space
      errmsg << names << endl
             << "\tThe required federates are: ";
      names = "";
      for ( size_t i = 0; i < known_federates.size(); ++i ) {
         if ( known_federates[i].required ) {
            std::string known_fed_str;
            StringUtilities::to_string( known_fed_str, known_federates[i].name );
            names += known_fed_str;
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

   // The MOM federate discovery process does not fill in the MOM object
   // instance names.  We need to get them from the RTI and complete the
   // information in the joined federates list.
   determine_federate_MOM_object_instance_names();

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::wait_for_required_federates_to_join():%d FOUND ALL REQUIRED FEDERATES!!!\n",
                       __LINE__ );
   }

   return status_string;
}

/*!
 *  @job_class{initialization}
 *  @detail NOTE: This function will block.
 */
void Federate::update_and_print_joined_federates()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL, "Federate::update_and_print_joined_federates():%d started.\n",
                       __LINE__ );
   }

   // Check the state of the joined federates update process.
   // If we are not in an inactive state, something is wrong.
   if ( federate_update_state != THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_NONE ) {
      ostringstream errmsg;
      errmsg << "Federate::update_and_print_joined_federates():" << __LINE__
             << ": ERROR: Unexpected Federates update state."
             << "  We expected FEDERATE_UPDATE_NONE but the state was "
             << to_string( federate_update_state )
             << "!" << endl;
      DebugHandler::terminate( errmsg.str() );
   }

   // Timers used to manage wait and print cycles.
   int64_t      wallclock_time; // cppcheck-suppress [variableScope]
   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   // Activate the joined federate update process.
   federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_ACTIVATE;

   // Iterate through the joined federate update process until the joined
   // federates have been updated.
   // NOTE: This loop will block until all the joined federates are updated.
   while ( ( federate_update_state != THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_NONE )
           && ( federate_update_state != THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_FAILED ) ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      // Sleep a little while to wait for the information to update.
      sleep_timer.sleep();

      // Run the update joined federates process.
      update_joined_federates();

      // To be more efficient, we get the time once and share it.
      wallclock_time = sleep_timer.time();

      // Make sure we're still an execution member.
      if ( sleep_timer.timeout( wallclock_time ) ) {
         sleep_timer.reset();
         if ( !is_execution_member() ) {
            ostringstream errmsg;
            errmsg << "Federate::update_and_print_joined_federates():" << __LINE__
                   << " ERROR: Unexpectedly the Federate is no longer an execution member."
                   << " This means we are either not connected to the"
                   << " RTI or we are no longer joined to the federation"
                   << " execution because someone forced our resignation at"
                   << " the Central RTI Component (CRC) level!" << endl;
            DebugHandler::terminate( errmsg.str() );
         }
      }

      // Check for scheduled print.
      if ( print_timer.timeout( wallclock_time ) ) {

         print_timer.reset();

         // Let's print out some useful status information.
         if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_ACTIVATE ) {
            message_publish( MSG_NORMAL,
                             "Federate::update_and_print_joined_federates():%d: Active.\n",
                             __LINE__ );
         } else if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_INITIATED ) {
            message_publish( MSG_NORMAL,
                             "Federate::update_and_print_joined_federates():%d: \
Waiting for the federatesInFederation update.\n",
                             __LINE__ );
         } else if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_RECEIVED ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
               message_publish( MSG_NORMAL, "Federate::update_and_print_joined_federates():%d: \
MOM just informed us that there are %d federates currently joined to the federation.\n",
                                __LINE__, federate_handles.size() );
            }
         } else if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_IN_PROGRESS ) {
            message_publish( MSG_NORMAL,
                             "Federate::update_and_print_joined_federates():%d: \
Waiting for the identified federates to join.\n",
                             __LINE__ );
         } else if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_COMPLETE ) {
            message_publish( MSG_NORMAL,
                             "Federate::update_and_print_joined_federates():%d: \
Successfully updated the joined federates.\n",
                             __LINE__ );
         } else if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_FAILED ) {
            message_publish( MSG_ERROR,
                             "Federate::update_and_print_joined_federates():%d: \
ERROR: Something went wrong while updating the joined federates.\n",
                             __LINE__ );
         }
      }

   } // End of while( federate_update_state ) . . .

   // Print out a list of the joined Federates.
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {

      // Build the federate summary as an output string stream.
      ostringstream summary;
      unsigned int  cnt = 0;

      summary << "Federate::update_and_print_joined_federates():"
              << __LINE__ << endl
              << "There are " << joined_federates_map.size() << " federates:";

      // Iterate through the joined federates map.
      KnownFederateMap::iterator map_iter;
      for ( map_iter = joined_federates_map.begin();
            map_iter != joined_federates_map.end(); ++map_iter ) {

         // Get the associate joined federate reference.
         KnownFederate const &joined_federate = static_cast< KnownFederate & >( map_iter->second );

         ++cnt;
         std::string name_str;
         StringUtilities::to_string( name_str, joined_federate.name );
         summary << endl
                 << "    " << cnt
                 << ": Found running federate '"
                 << name_str << "'";
      }
      summary << endl;

      // Display the federate summary.
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_SAVE_RESTORE ) ) {
      message_publish( MSG_NORMAL,
                       "Federate::update_and_print_joined_federates():%d Done.\n",
                       __LINE__ );
   }

   return;
}

/*!
 *  @job_class{initialization}
 */
void Federate::get_joined_federate_handle_set( RTI1516_NAMESPACE::FederateHandleSet &handle_set )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

   // Clear the set of federate handles for the joined federates.
   handle_set.clear();

   // Iterate through the joined federates map to construct a new set of
   // joined federates.  This is a convenience function for some HLA calls.
   // NULL string entries.
   KnownFederateMap::iterator map_iter;
   for ( map_iter = joined_federates_map.begin();
         map_iter != joined_federates_map.end(); ++map_iter ) {

      // Get the associate joined federate reference.
      KnownFederate const &joined_federate = static_cast< KnownFederate & >( map_iter->second );

      // Grab the federate handle from the joined federate entry.
      handle_set.insert( joined_federate.federate_handle );
   }

   return;
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
      DebugHandler::terminate( "Federate::initialize_MOM_handles() ERROR Detected!" );
   }
}

/*! @brief Publishes Object & Interaction classes and their member data. */
void Federate::publish()
{
   if ( !is_RTI_ready( "publish" ) ) {
      return;
   }

   object_service.publish();
   interaction_service.publish();

   // Publish Execution Control objects and interactions.
   execution_control->publish();
}

/*! @brief Unpublish the Object & Interaction classes. */
void Federate::unpublish()
{
   if ( !is_RTI_ready( "unpublish" ) ) {
      return;
   }

   object_service.unpublish();
   interaction_service.unpublish();

   // Unpublish Execution Control objects and interactions.
   execution_control->unpublish();
}

/*! @brief Subscribe to Object and Interaction classes and their member data. */
void Federate::subscribe()
{
   if ( !is_RTI_ready( "subscribe" ) ) {
      return;
   }

   object_service.subscribe();
   interaction_service.subscribe();

   // Subscribe to anything needed for the execution control mechanisms.
   execution_control->subscribe();
}

/*! @brief Unubscribe from the Object and Interaction classes. */
void Federate::unsubscribe()
{
   if ( !is_RTI_ready( "unsubscribe" ) ) {
      return;
   }

   object_service.unsubscribe();
   interaction_service.unsubscribe();

   // Unsubscribe to anything needed for the execution control mechanisms.
   execution_control->unsubscribe();
}

/*! @brief Publish and Subscribe to Object and Interaction classes and their
 * member data. */
void Federate::publish_and_subscribe()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::publish_and_subscribe():%d\n",
                       __LINE__ );
   }
   subscribe();
   publish();
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
      DebugHandler::terminate( "Federate::subscribe_attributes() ERROR Detected!" );
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
      DebugHandler::terminate( "Federate::unsubscribe_attributes() ERROR Detected!" );
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
      DebugHandler::terminate( "Federate::request_attribute_update() ERROR Detected!" );
   }
}

void Federate::ask_MOM_for_federate_info()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::ask_MOM_for_federate_names():%d\n",
                       __LINE__ );
   }

   // Make sure the MOM handles get initialized before we try to use them.
   if ( !MOM_HLAfederateName_handle.isValid()
        || !MOM_HLAfederateType_handle.isValid()
        || !MOM_HLAfederate_handle.isValid() ) {
      initialize_MOM_handles();
   }

   AttributeHandleSet fedMomAttributes;
   fedMomAttributes.insert( MOM_HLAfederateType_handle );
   fedMomAttributes.insert( MOM_HLAfederateName_handle );
   fedMomAttributes.insert( MOM_HLAfederate_handle );
   subscribe_attributes( MOM_HLAfederate_class_handle, fedMomAttributes );

   AttributeHandleSet requestedAttributes;
   requestedAttributes.insert( MOM_HLAfederateType_handle );
   requestedAttributes.insert( MOM_HLAfederateName_handle );
   requestedAttributes.insert( MOM_HLAfederate_handle );
   request_attribute_update( MOM_HLAfederate_class_handle, requestedAttributes );

   fedMomAttributes.clear();
   requestedAttributes.clear();

   return;
}

void Federate::ask_MOM_for_federation_info()
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::ask_MOM_for_federation_info():%d\n",
                       __LINE__ );
   }

   // Concurrency critical code section because joined-federate state is changed
   // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
   // function.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &federate_update_mutex );

      // Clear the list of federates.
      federate_handles.clear();

      // Activate the federates in Federation update process.
      federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_ACTIVATE;
   }

   // Make sure the MOM handles get initialized before we try to use them.
   if ( !MOM_HLAfederatesInFederation_handle.isValid()
        || !MOM_HLAautoProvide_handle.isValid() ) {
      initialize_MOM_handles();
   }

   // Subscribe to the attributes we need.
   AttributeHandleSet fedMomAttributes;
   fedMomAttributes.insert( MOM_HLAfederatesInFederation_handle );
   fedMomAttributes.insert( MOM_HLAautoProvide_handle );
   subscribe_attributes( MOM_HLAfederation_class_handle, fedMomAttributes );

   AttributeHandleSet requestedAttributes;
   requestedAttributes.insert( MOM_HLAfederatesInFederation_handle );
   requestedAttributes.insert( MOM_HLAautoProvide_handle );
   request_attribute_update( MOM_HLAfederation_class_handle, requestedAttributes );

   fedMomAttributes.clear();
   requestedAttributes.clear();

   return;
}

void Federate::unsubscribe_from_MOM_federation_info()
{
   AttributeHandleSet attributes;

   // Build the attribute list.
   attributes.insert( MOM_HLAfederatesInFederation_handle );
   attributes.insert( MOM_HLAautoProvide_handle );

   // Unsubscribe from there particular attributes.
   unsubscribe_attributes( MOM_HLAfederation_class_handle, attributes );

   // Clear the attribute list before returning.
   attributes.clear();

   return;
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
      DebugHandler::terminate( "Federate::send_interaction() ERROR Detected!" );
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
 * @job_class{initialization}
 */
void Federate::wait_for_init_sync_point(
   string const &sync_point_label )
{
   if ( !execution_control->is_wait_for_init_sync_point_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream errmsg;
         errmsg << "Federate::wait_for_init_sync_point():" << __LINE__
                << " WARNING: This call will be ignored because the"
                << " Simulation Initialization Scheme (Type:'"
                << execution_control->get_type()
                << "') does not support it." << endl;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
      return;
   }

   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         ostringstream errmsg;
         errmsg << "Federate::wait_for_init_sync_point():" << __LINE__
                << " Late joining federate so this call will be ignored." << endl;
         message_publish( MSG_NORMAL, errmsg.str().c_str() );
      }
      return;
   }

   if ( sync_point_label.empty() ) {
      ostringstream errmsg;
      errmsg << "Federate::wait_for_init_sync_point():" << __LINE__
             << " ERROR: Empty Sync-Point Label specified!" << endl;
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   wstring ws_sync_point_label;
   StringUtilities::to_wstring( ws_sync_point_label, sync_point_label );

   // Determine if the multiphase init sync-point label is valid.
   if ( execution_control->contains_multiphase_init_sync_point( ws_sync_point_label ) ) {

      // Achieve the specified multiphase init sync-point and wait for
      // the federation to be synchronized on it.
      if ( !execution_control->achieve_sync_point_and_wait_for_synchronization( ws_sync_point_label ) ) {
         ostringstream errmsg;
         errmsg << "Federate::wait_for_init_sync_point():" << __LINE__
                << " ERROR: Unexpected error waiting for sync-point '"
                << sync_point_label << "'!" << endl;
         DebugHandler::terminate( errmsg.str() );
         return;
      }
   } else {
      ostringstream errmsg;
      errmsg << "Federate::wait_for_init_sync_point():" << __LINE__
             << " ERROR: This federate has not been configured to use the"
             << " synchronization-point label '" << sync_point_label
             << "' as a multiphase initialization sync-point. Please check"
             << " your input.py file to ensure your federate adds the"
             << " multiphase initialization sync-point:\n"
             << "federate.add_multiphase_init_sync_point( '"
             << sync_point_label << "' )" << endl;
      DebugHandler::terminate( errmsg.str() );
      return;
   }
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
 *  @job_class{freeze_init}
 */
void Federate::freeze_init()
{
   // Dispatch to the ExecutionControl method.
   execution_control->freeze_init();
}

/*!
 *  @job_class{freeze}
 */
void Federate::freeze_check_mode()
{
   // Check to see if we should shutdown.
   check_for_shutdown_with_termination();

   // Check to see if the ExecutionControl should exit freeze.
   if ( execution_control->check_freeze_exit() ) {
      return;
   }

   SIM_MODE const exec_mode = exec_get_mode();
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

   return;
}

/*!
 *  @job_class{freeze}
 */
void Federate::freeze_save()
{

   // Call the ExecutionControl restore process executive.
   // This function tracks the progression of the Federate HLA Save process
   // through the HLA Save sequences.
   execution_control->save_process();

   return;
}

/*!
 *  @job_class{freeze}
 */
void Federate::freeze_restore()
{

   // Call the ExecutionControl restore process executive.
   // This function tracks the progression of the Federate HLA Restore process
   // through the HLA Restore sequence.
   execution_control->restore_process();

   return;
}

/*!
 *  @job_class{unfreeze}
 */
void Federate::freeze_exit()
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::exit_freeze():%d announce_freeze:%s, freeze_federation:%s\n",
                       __LINE__, ( execution_control->is_freeze_announced() ? "Yes" : "No" ),
                       ( execution_control->is_freeze_pending() ? "Yes" : "No" ) );
   }

   // Dispatch to the ExecutionControl method.
   execution_control->exit_freeze();

   execution_control->set_freeze_pending( false );

   return;
}

/*!
 *  @job_class{scheduled}
 */
void Federate::goto_run()
{
   exec_run();
}

/*!
 *  @job_class{scheduled}
 */
void Federate::save( string const &label )
{
   wstring label_wstr;

   // Convert the string label to a wstring label.
   StringUtilities::to_wstring( label_wstr, label );

   // Call the wstring version.
   this->save( label_wstr );

   return;
}

/*!
 *  @job_class{scheduled}
 */
void Federate::save( wstring const &label )
{

   // Sanity checks.
   if ( execution_control == NULL ) {
      ostringstream msg;
      string        label_str;
      StringUtilities::to_string( label_str, label );
      msg << "Federate::save():" << __LINE__
          << " ERROR: No ExecutionControl for Saving \'"
          << label_str << "\'!";
      message_publish( MSG_ERROR, "%s\n", msg.str().c_str() );
      return;
   }

   // Call the execution control Save method.
   execution_control->save( label );

   return;
}

/*!
 *  @job_class{scheduled}
 */
void Federate::save_at_SET(
   wstring const &label,
   double         sim_time )
{
   string label_str;
   StringUtilities::to_string( label_str, label );

   // FIXME: Need implementation!
   ostringstream errmsg;
   errmsg << "Federate::save_at_SET():" << __LINE__
          << " ERROR: Not yet implemented!" << endl
          << " Label:'" << label_str << "'" << endl
          << " sim_time:" << sim_time << endl;
   DebugHandler::terminate( errmsg.str() );
   return;
}

/*!
 *  @job_class{scheduled}
 */
void Federate::save_at_SST(
   wstring const &label,
   double         sst )
{
   string label_str;
   StringUtilities::to_string( label_str, label );

   // FIXME: Need implementation!
   ostringstream errmsg;
   errmsg << "Federate::save_at_SST():" << __LINE__
          << " ERROR: Not yet implemented!" << endl
          << " Label:'" << label_str << "'" << endl
          << " scenario_time:" << sst << endl;
   DebugHandler::terminate( errmsg.str() );
   return;
}

/*!
 *  @job_class{scheduled}
 */
void Federate::save_at_HLT(
   wstring const                        &label,
   RTI1516_NAMESPACE::LogicalTime const &time )
{
   string label_str;
   StringUtilities::to_string( label_str, label );
   string time_str;
   StringUtilities::to_string( time_str, time.toString() );

   // FIXME: Need implementation!
   ostringstream errmsg;
   errmsg << "Federate::save_at_HLT():" << __LINE__
          << " ERROR: Not yet implemented!" << endl
          << " Label:'" << label_str << "'" << endl
          << " time:" << time_str << endl;
   DebugHandler::terminate( errmsg.str() );
   return;
}

/*!
 *  @job_class{scheduled}
 */
void Federate::restore( string const &label )
{
   wstring label_wstr;

   // Convert the string label to a wstring label.
   StringUtilities::to_wstring( label_wstr, label );

   // Call the wstring version.
   this->restore( label_wstr );

   return;
}

/*!
 *  @job_class{scheduled}
 */
void Federate::restore( wstring const &label )
{

   // Sanity checks.
   if ( execution_control == NULL ) {
      ostringstream msg;
      string        label_str;
      StringUtilities::to_string( label_str, label );
      msg << "Federate::save():" << __LINE__
          << " ERROR: No ExecutionControl for Saving \'"
          << label_str << "\'!";
      message_publish( MSG_ERROR, "%s\n", msg.str().c_str() );
      return;
   }

   // Call the execution control Save method.
   execution_control->restore( label );

   return;
}

//-------------------------------------------------------------------------
// CheckpointConversionBase Interface.
//-------------------------------------------------------------------------

/*!
 *  @job_class{freeze}
 */
/*! @brief Convert data to a form Trick can checkpoint. */
void Federate::convert_data_before_checkpoint()
{

   if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::convert_data_before_checkpoint():"
          << __LINE__ << ": Converting the federate data for checkpointing." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Delegate to the Execution Control specific implementation.
   if ( execution_control != NULL ) {
      execution_control->restore_data_after_checkpoint();
   }

   // TODO: Convert other Federate data into data types Trick can checkpoint.

   return;
}

/*!
 *  @job_class{freeze}
 */
/*! @brief Restore data structures after loading a Trick checkpoint. */
void Federate::restore_data_after_checkpoint()
{
   if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::restore_data_after_checkpoint():"
          << __LINE__ << ": Restoring the federate data after loading a checkpoint." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Delegate to the Execution Control specific implementation.
   if ( execution_control != NULL ) {
      execution_control->restore_data_after_checkpoint();
   }

   // TODO: Restore other checkpoint data into Federate data.

   return;
}

/*!
 *  @job_class{freeze}
 */
/*! @brief Clear/release the memory used for the conversion data for the checkpoint. */
void Federate::free_converted_data_for_checkpoint()
{
   if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::free_converted_data_for_checkpoint():"
          << __LINE__ << ": Freeing federate data allocated for checkpointing." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Delegate to the Execution Control specific implementation.
   if ( execution_control != NULL ) {
      execution_control->free_converted_data_for_checkpoint();
   }

   // TODO: Free other Federate checkpoint converted data.

   return;
}

/*!
 *  @job_class{checkpoint}
 */
void Federate::checkpoint_before()
{

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::checkpoint_before():"
          << __LINE__ << ": Preparing for a checkpoint." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Delegate to the Execution Control specific implementation.
   if ( execution_control != NULL ) {
      execution_control->checkpoint_before();
   }
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSim initialization schemes.
 *  @job_class{preload_checkpoint}
 */
void Federate::checkpoint_preload()
{
   // TrickHLA only supports a checkpoint load as part of an HLA Restore process.
   if ( save_restore_service.restore_state != THLARestoreProcessEnum::RESTORE_INITIATED ) {
      ostringstream msg;
      msg << "Federate::checkpoint_preload():" << __LINE__
          << ": Checkpoint loading only supported as part of an HLA Restore process!" << endl;
      message_publish( MSG_WARNING, msg.str().c_str() );

      string restore_label_str;
      StringUtilities::to_string( restore_label_str, save_restore_service.restore_label );
      ostringstream errmsg;
      errmsg << "Federate::checkpoint_preload():" << __LINE__
             << ": ERROR: Unexpected Restore state for label: " << restore_label_str << endl
             << "   Expected state: RESTORE_INITIATED" << endl
             << "   Current state : " << TrickHLA::to_string( save_restore_service.restore_state ) << endl;
      message_publish( MSG_WARNING, errmsg.str().c_str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::checkpoint_preload():"
          << __LINE__ << ": Preparing to load and checkpoint file as part of an HLA Restore process." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Delegate to the Execution Control specific implementation.
   if ( execution_control != NULL ) {
      execution_control->checkpoint_preload();
   }

   return;
}

/*!
 *  @job_class{checkpoint}
 */
void Federate::checkpoint_after()
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::checkpoint_after():"
          << __LINE__ << ": Cleaning up after a checkpoint." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Delegate to the Execution Control specific implementation.
   if ( execution_control != NULL ) {
      execution_control->checkpoint_after();
   }

   return;
}

/*!
 *  \par<b>Assumptions and Limitations:</b>
 *  - Currently only used with DIS and IMSim initialization schemes.
 *  @job_class{restart}
 */
void Federate::checkpoint_restart()
{
   // TrickHLA only supports a checkpoint load as part of an HLA Restore process.
   if ( save_restore_service.restore_state != THLARestoreProcessEnum::RESTORE_CHECKPOINT ) {
      ostringstream msg;
      msg << "Federate::checkpoint_restart():"
          << __LINE__ << ": Checkpoint restart only supported as part of an HLA Restore process!" << endl;
      message_publish( MSG_WARNING, msg.str().c_str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      ostringstream msg;
      msg << "Federate::checkpoint_restart():"
          << __LINE__ << ": Restarting after loading a checkpoint." << endl;
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Delegate to the Execution Control specific implementation.
   if ( execution_control != NULL ) {
      execution_control->checkpoint_restart();
   }

   return;
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );

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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      ostringstream errmsg;
      errmsg << "Federate::create_federation():" << __LINE__
             << " EXCEPTION: NotConnected" << endl;
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederateNameAlreadyInUse const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: FederateNameAlreadyInUse! Federate name:\""
             << get_federate_name() << "\"" << endl;

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );

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

      DebugHandler::terminate( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::FederateAlreadyExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " ERROR: The Federate '" << get_federate_name()
             << "' is already a member of the '"
             << get_federation_name() << "' Federation." << endl;

      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
      set_connection_lost();
   } catch ( RTI1516_NAMESPACE::CallNotAllowedFromWithinCallback const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::join_federation():" << __LINE__
             << " EXCEPTION: CallNotAllowedFromWithinCallback" << endl;

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( "Federate::enable_async_delivery() ERROR: NULL pointer to RTIambassador!" );
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

      DebugHandler::terminate( errmsg.str() );
   } catch ( RestoreInProgress const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: RestoreInProgress" << endl;

      DebugHandler::terminate( errmsg.str() );
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: FederateNotExecutionMember" << endl;

      DebugHandler::terminate( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      ostringstream errmsg;
      errmsg << "Federate::enable_async_delivery():" << __LINE__
             << " EXCEPTION: NotConnected" << endl;
      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
   TrickHLA::Object *obj = object_service.get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::send_zero_lookahead_and_requested_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'" << endl;
      DebugHandler::terminate( errmsg.str() );
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

   obj->send_zero_lookahead_and_requested_data( time_management_service.granted_time );
}

/*!
 *  @brief Blocking function call to wait to receive the zero lookahead data
 *  for the specified object instance.
 *  @param obj_instance_name Object instance name to wait for data.
 */
void Federate::wait_to_receive_zero_lookahead_data(
   string const &obj_instance_name )
{
   TrickHLA::Object *obj = object_service.get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::wait_to_receive_zero_lookahead_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'" << endl;
      DebugHandler::terminate( errmsg.str() );
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
      time_management_service.wait_for_zero_lookahead_TARA_TAG();

      int64_t      wallclock_time; // cppcheck-suppress [variableScope]
      SleepTimeout print_timer;
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
               DebugHandler::terminate( errmsg.str() );
            }
         }

         if ( print_timer.timeout( wallclock_time ) ) {
            print_timer.reset();
            message_publish( MSG_NORMAL, "Federate::wait_to_receive_zero_lookahead_data():%d Waiting...\n",
                             __LINE__ );
         }

         // The TARA will cause zero-lookahead data to be reflected before the TAG.
         time_management_service.wait_for_zero_lookahead_TARA_TAG();
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
   TrickHLA::Object *obj = object_service.get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::send_blocking_io_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'" << endl;
      DebugHandler::terminate( errmsg.str() );
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
   TrickHLA::Object *obj = object_service.get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Federate::wait_to_receive_blocking_io_data():" << __LINE__
             << " ERROR: Could not find the object instance for the name specified:'"
             << obj_instance_name << "'" << endl;
      DebugHandler::terminate( errmsg.str() );
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
      SleepTimeout print_timer;
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
               DebugHandler::terminate( errmsg.str() );
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
   DebugHandler::terminate( errmsg.str() );
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
   for ( int i = 0; i < object_service.obj_count; ++i ) {
      ostringstream msg1;
      msg1 << "Federate::shutdown():" << __LINE__
           << " Object[" << i << "]:'" << object_service.objects[i].get_name() << "'"
           << " send_count:" << object_service.objects[i].send_count
           << " receive_count:" << object_service.objects[i].receive_count << endl;
      message_publish( MSG_INFO, msg1.str().c_str() );
   }
#endif // TRICKHLA_CHECK_SEND_AND_RECEIVE_COUNTS

#ifdef TRICKHLA_CYCLIC_READ_TIME_STATS
   for ( int i = 0; i < object_service.obj_count; ++i ) {
      ostringstream msg2;
      msg2 << "Federate::shutdown():" << __LINE__
           << " Object[" << i << "]:'" << object_service.objects[i].get_name() << "' "
           << object_service.objects[i].elapsed_time_stats.to_string() << endl;
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
   time_management_service.shutdown_time_management();

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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // TODO: Do we really want to terminate here! DDexter 9/27/2010
   ostringstream errmsg;
   errmsg << "Federate::resign_so_we_can_rejoin():" << __LINE__
          << " ERROR: Federate '" << get_federate_name()
          << "' resigned from Federation '" << get_federation_name() << "'" << endl;
   DebugHandler::terminate( errmsg.str() );
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

      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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
      DebugHandler::terminate( errmsg.str() );
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

   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   while ( this->auto_provide_setting < 0 ) {

      // Check for shutdown.
      check_for_shutdown_with_termination();

      // Sleep a little while to wait for the information to update.
      sleep_timer.sleep();

      if ( this->auto_provide_setting < 0 ) {

         // To be more efficient, we get the time once and share it.
         int64_t const wallclock_time = sleep_timer.time();

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
               DebugHandler::terminate( errmsg.str() );
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
      string const auto_provide_status = get_auto_provide_status_string( auto_provide_setting );
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
      string const auto_provide_status = get_auto_provide_status_string( auto_provide_setting );
      message_publish( MSG_NORMAL, "Federate::enable_MOM_auto_provide_setting():%d Auto-Provide:%s\n",
                       __LINE__, auto_provide_status.c_str() );
   }

   publish_interaction_class( MOM_HLAsetSwitches_class_handle );

   ParameterHandleValueMap param_values_map;
   try {
      // HLAautoProvide attribute is an HLAswitch, which is an HLAinteger32BE.
      HLAinteger32BE const auto_provide_encoder( auto_provide_setting );

      param_values_map[MOM_HLAautoProvide_param_handle] = auto_provide_encoder.encode();

   } catch ( RTI1516_NAMESPACE::EncoderException &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Federate::enable_MOM_auto_provide_setting():" << __LINE__
             << " ERROR: Encoder exception '" << rti_err_msg << "'"
             << " trying to encode auto-provide switch setting (HLAautoProvide)"
             << " for value " << auto_provide_setting << "!" << endl;
      DebugHandler::terminate( errmsg.str() );
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
         string const auto_provide_status = get_auto_provide_status_string( orig_auto_provide_setting );
         message_publish( MSG_NORMAL, "Federate::restore_orig_MOM_auto_provide_setting():%d Auto-Provide:%s value:%d\n",
                          __LINE__, auto_provide_status.c_str(),
                          orig_auto_provide_setting );
      }
      enable_MOM_auto_provide_setting( orig_auto_provide_setting > 0 );
   }
}

void Federate::add_MOM_HLAfederation_instance_handle(
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

void Federate::remove_MOM_HLAfederation_instance_handle(
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

bool Federate::is_MOM_HLAfederation_instance_handle(
   ObjectInstanceHandle const &instance_hndl )
{
   return ( MOM_HLAfederation_instance_name_map.find( instance_hndl ) != MOM_HLAfederation_instance_name_map.end() );
}

void Federate::set_MOM_HLAfederation_instance_attributes(
   ObjectInstanceHandle const    &instance_hndl,
   AttributeHandleValueMap const &values )
{
   // Determine if this is a MOM HLAfederation instance.
   if ( !is_MOM_HLAfederation_instance_handle( instance_hndl ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         message_publish( MSG_WARNING, "Federate::set_federation_instance_attributes():%d WARNING: Unknown object class, expected 'HLAmanager.HLAfederation'.\n",
                          __LINE__ );
      }
      return;
   }

   // Look for the Federation attributes we are interested in.
   AttributeHandleValueMap::const_iterator attr_iter;
   for ( attr_iter = values.begin(); attr_iter != values.end(); ++attr_iter ) {

      AttributeHandle const    &handle = attr_iter->first;
      VariableLengthData const &data   = attr_iter->second;

      if ( handle == MOM_HLAautoProvide_handle ) {

         try {
            // HLAautoProvide attribute is an HLAswitch, which is an HLAinteger32BE.
            // Decode directly into the auto_provide_setting variable.
            HLAinteger32BE auto_provide_encoder( &auto_provide_setting );

            auto_provide_encoder.decode( data );

         } catch ( RTI1516_NAMESPACE::EncoderException &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            ostringstream errmsg;
            errmsg << "Federate::set_MOM_HLAfederation_instance_attributes():" << __LINE__
                   << " ERROR: Encoder exception '" << rti_err_msg << "'"
                   << " trying to decode auto-provide switch setting"
                   << " (HLAautoProvide)!" << endl;
            DebugHandler::terminate( errmsg.str() );
         }
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            string const auto_provide_status = get_auto_provide_status_string( auto_provide_setting );
            message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederation_instance_attributes():%d Auto-Provide:%s value:%d\n",
                             __LINE__, auto_provide_status.c_str(),
                             auto_provide_setting );
         }

      } else if ( handle == MOM_HLAfederatesInFederation_handle ) {

         // HLAfederatesInFederation is a data type of HLAhandleList,
         // which is an HLAvariableArray encoding of element type HLAhandle.
         // HLAhandle is an HLAvariableArray encoding of element type HLAbyte.
         try {

            HLAopaqueData const      fed_handle_proto;
            HLAvariableArray         feds_list( fed_handle_proto );
            VariableLengthData const encoded_fed_handle;

            // Clear the federates in federation list.
            federate_handles.clear();

            // Decode the federatesInFederation attribute.
            feds_list.decode( data );

            // Iterate through the decoded federate handle list to extract the handles.
            for ( unsigned int iinc = 0; iinc < feds_list.size(); iinc++ ) {

               // Place the encoded federate handle data into a VariableLengthData.
               HLAopaqueData const     &opaqueData = dynamic_cast< HLAopaqueData const & >( feds_list.get( iinc ) );
               VariableLengthData const encoded_fed_data( opaqueData.get(), opaqueData.dataLength() );

               // Decode the federate handle and insert it into the federates
               // in Federation handle set.
               federate_handles.insert( decode_federate_handle( encoded_fed_data ) );
            }

            // Check if a joined federate update process is active.
            if ( federate_update_state == THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_INITIATED ) {
               federate_update_state = THLAFederateUpdateProcessEnum::FEDERATE_UPDATE_RECEIVED;
            }

         } catch ( RTI1516_NAMESPACE::EncoderException &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            ostringstream errmsg;
            errmsg << "Federate::set_federation_instance_attributes():" << __LINE__
                   << " ERROR: Encoder exception '" << rti_err_msg << "'"
                   << " trying to decode HLAfederatesInFederation variable array!"
                   << endl;
            DebugHandler::terminate( errmsg.str() );
         }

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
            message_publish( MSG_NORMAL, "Federate::set_MOM_HLAfederation_instance_attributes():%d Found a FederationID list with %d elements.\n",
                             __LINE__, federate_handles.size() );
         }
      }
   }

   return;
}

// FIXME: Should this code be deprecated.  It only appears to be use in the IMSim code.
void Federate::restore_federate_handles_from_MOM()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
      message_publish( MSG_NORMAL, "Federate::restore_federate_handles_from_MOM:%d\n",
                       __LINE__ );
   }

   // Check to insure that we are in the correct Restore state.
   if ( save_restore_service.restore_state != THLARestoreProcessEnum::RESTORE_CHECKPOINT ) {
      message_publish( MSG_WARNING,
                       "Federate::restore_federate_handles_from_MOM:%d : Invalid Restore state: \'%s\'!\n",
                       __LINE__, TrickHLA::to_string( save_restore_service.restore_state ).c_str() );
      return;
   }

   // Concurrency critical code section because joined-federate state is changed
   // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
   // function.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

      // Clear the list of joined federates.
      joined_federates_map.clear();
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

   bool const   all_found = false;
   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   // Wait for all the federate handles to be retrieved.
   do {
      // Concurrency critical code section because joined-federate state is changed
      // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
      // function.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

         // FIXME: Is this sufficient?  Do we need to check for the needed federates?
         // We should probably be using the update_joined_federates here.
         // Determine if all the federate handles have been found.
         // all_found = ( joined_federates_map.size() >= save_restore_service.running_feds_count );
      }

      if ( !all_found ) {

         // Check for shutdown.
         check_for_shutdown_with_termination();

         sleep_timer.sleep();

         // To be more efficient, we get the time once and share it.
         int64_t const wallclock_time = sleep_timer.time();

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
               DebugHandler::terminate( errmsg.str() );
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

   fedMomAttributes.clear();
   requestedAttributes.clear();

   return;
}

void Federate::rebuild_federate_handles(
   ObjectInstanceHandle const    &instance_hndl,
   AttributeHandleValueMap const &values )
{
   KnownFederateMap::iterator              fed_iter;
   AttributeHandleValueMap::const_iterator attr_iter;

   // Find the joined federate associated with this object instance handle.
   fed_iter = joined_federates_map.find( instance_hndl );
   if ( fed_iter == joined_federates_map.end() ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_hndl );
      message_publish( MSG_ERROR, "Federate::rebuild_federate_handles():%d Federate OID:%s\n",
                       __LINE__, id_str.c_str() );
      return;
   }

   // Get the reference to the joined federate.
   KnownFederate &joined_federate = static_cast< KnownFederate & >( fed_iter->second );

   // Loop through all federate handles
   for ( attr_iter = values.begin(); attr_iter != values.end(); ++attr_iter ) {

      VariableLengthData const &encoded_federate_handle = static_cast< VariableLengthData const & >( attr_iter->second );

      FederateHandle const fed_handle = decode_federate_handle( encoded_federate_handle );

      // Concurrency critical code section because joined-federate state is changed
      // by FedAmb callback to the Federate::set_MOM_HLAfederate_instance_attributes()
      // function.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection const auto_unlock_mutex( &joined_federate_mutex );

         // Add this FederateHandle to the set of joined federates.
         joined_federate.federate_handle = fed_handle;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FEDERATE ) ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_hndl );
         string fed_id;
         StringUtilities::to_string( fed_id, fed_handle );
         message_publish( MSG_NORMAL, "Federate::rebuild_federate_handles():%d Federate OID:%s Federate-ID:%s\n",
                          __LINE__, id_str.c_str(), fed_id.c_str() );
      }
   }

   return;
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
   wstring const required_fed_name;
   for ( size_t i = 0; i < this->known_federates.size(); ++i ) {
      if ( known_federates[i].required ) {
         if ( fed_name == known_federates[i].name ) { // found an exact match
            return true;
         } else {
            // look for instance attributes of a required object. to do this,
            // check if the "required federate name" is found inside the supplied
            // federate name.
            size_t const found = fed_name.find( required_fed_name );
            if ( found != wstring::npos ) {
               // found the "required federate name" inside the supplied federate name
               return true;
            }
         }
      }
   }
   return false;
}
