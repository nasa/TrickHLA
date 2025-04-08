/*!
@file TrickHLA/Manager.cpp
@ingroup TrickHLA
@brief This class manages the interface between a Trick simulation and HLA.

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
@trick_link_dependency{Attribute.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionConfigurationBase.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Int64Interval.cpp}
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{Interaction.cpp}
@trick_link_dependency{InteractionItem.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{Parameter.cpp}
@trick_link_dependency{ParameterItem.cpp}
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2006, --, DSES Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdint>
#include <float.h>
#include <iomanip>
#include <map>
#include <string>

// Trick include files.
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/InteractionItem.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/ParameterItem.hh"
#include "TrickHLA/SleepTimeout.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
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

/*!
 * @job_class{initialization}
 */
Manager::Manager()
   : obj_count( 0 ),
     objects( NULL ),
     inter_count( 0 ),
     interactions( NULL ),
     restore_federation( 0 ),
     restore_file_name( NULL ),
     initiated_a_federation_save( false ),
     interactions_queue(),
     check_interactions_count( 0 ),
     check_interactions( NULL ),
     rejoining_federate( false ),
     restore_determined( false ),
     restore_federate( false ),
     mgr_initialized( false ),
     obj_discovery_mutex(),
     object_map(),
     obj_name_index_map(),
     federate_has_been_restored( false ),
     federate( NULL ),
     execution_control( NULL )
{
   return;
}

/*!
 * @details Frees the Trick allocated memory.
 * @job_class{shutdown}
 */
Manager::~Manager()
{
   object_map.clear();
   obj_name_index_map.clear();
   free_checkpoint_interactions();

   // Make sure we destroy the mutex.
   obj_discovery_mutex.destroy();
}

/*!
 * \par<b>Assumptions and Limitations:</b>
 * - The TrickHLA::ExecutionControlBase class is actually an abstract class.
 * Therefore, the actual object instance being passed in is an instantiable
 * polymorphic child of the TrickHLA::ExecutionControlBase class.
 *
 * @job_class{default_data}
 */
void Manager::setup(
   Federate             &federate,
   ExecutionControlBase &execution_control )
{
   // Set the TrickHLA::Federate instace reference.
   this->federate = &federate;

   // Set the TrickHLA::ExecutionControlBase instance reference.
   this->execution_control = &execution_control;
}

/*!
 * @job_class{initialization}
 */
void Manager::initialize()
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Just return if the TrickHLA Manager is already initialized.
   if ( this->mgr_initialized ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::initialize():%d Already initialized.\n",
                          __LINE__ );
      }
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::initialize():%d\n", __LINE__ );
   }

   // Check to make sure we have a reference to the TrickHLA::Federate.
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Check to make sure we have a reference to the TrickHLA::ExecutionControlBase.
   if ( this->execution_control == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " ERROR: Unexpected NULL 'execution_control' pointer!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // The manager is now initialized.
   this->mgr_initialized = true;

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Manager::restart_initialization()
{
   // Just return if the TrickHLA Manager is not initialized.
   if ( !mgr_initialized ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::restart_initialization():%d Manager Not initialized, returning.\n",
                          __LINE__ );
      }
      return;
   }

   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::restart_initialization():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::restart_initialization():%d\n", __LINE__ );
   }

   // Make sure the Federate is initialized after the restart.
   federate->restart_initialization();

   // To allow manager initialization to complete we need to reset the init flag.
   this->mgr_initialized = false;

   // Verify the user specified object and interaction arrays and counts.
   verify_object_and_interaction_arrays();

   // Setup the Execution Control and Execution Configuration objects now that
   // we know if we are the "Master" federate or not.
   if ( execution_control == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::restart_initialization():" << __LINE__
             << " ERROR: Unexpected NULL 'execution_control' pointer!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // The set_master() function set's additional parameter so call it again to
   // force the a complete master state.
   bool master_flag = execution_control->is_master(); // cppcheck-suppress [nullPointerRedundantCheck,unmatchedSuppression]
   execution_control->set_master( master_flag );      // cppcheck-suppress [nullPointerRedundantCheck,unmatchedSuppression]

   // Setup all the Trick Ref-Attributes for the user specified objects,
   // attributes, interactions and parameters.
   setup_all_ref_attributes();

   // Setup all the RTI handles for the objects, attributes and interaction
   // parameters.
   setup_all_RTI_handles();

   // Set the object instance handles based on its name.
   set_all_object_instance_handles_by_name();

   // Make sure we reinitialize the MOM interface handles.
   federate->initialize_MOM_handles();

   // Perform the next few steps if we are the Master federate.
   if ( execution_control->is_master() ) {

      // Make sure all the federate instance handles are reset based on
      // the federate name so that the wait for required federates will work
      // after a checkpoint reload.
      federate->set_all_federate_MOM_instance_handles_by_name();

      // Make sure all required federates have joined the federation.
      federate->wait_for_required_federates_to_join();
   }

   // Restore ownership_transfer data for all objects.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].decode_checkpoint();
   }

   // Restore checkpointed interactions.
   decode_checkpoint_interactions();

   // The manager is now initialized.
   this->mgr_initialized = true;
}

void Manager::initialize_HLA_cycle_time()
{
   // Set the core job cycle time now that we know what it is so that the
   // attribute cyclic ratios can now be calculated for any multi-rate
   // attributes.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].set_core_job_cycle_time( federate->get_HLA_cycle_time() );
   }
}

/*! @brief Verify the user specified object and interaction arrays and counts. */
void Manager::verify_object_and_interaction_arrays()
{
   // Check for the error condition of a valid object count but a null
   // objects array.
   if ( ( obj_count > 0 ) && ( objects == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::verify_object_and_interaction_arrays():" << __LINE__
             << " ERROR: Unexpected NULL 'objects' array for a non zero"
             << " obj_count:" << obj_count << ". Please check your input or"
             << " modified-data files to make sure the 'Manager::objects'"
             << " array is correctly configured.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have a non-NULL objects array but the object-count is invalid
   // then let the user know.
   if ( ( obj_count <= 0 ) && ( objects != NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::verify_object_and_interaction_arrays():" << __LINE__
             << " ERROR: Unexpected " << ( ( obj_count == 0 ) ? "zero" : "negative" )
             << " obj_count:" << obj_count << " for a non-NULL 'objects' array."
             << " Please check your input or modified-data files to make sure"
             << " the 'Manager::objects' array is correctly configured.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Object count if negative.
   if ( obj_count < 0 ) {
      obj_count = 0;
   }

   // Object instance names must be unique and can not be a duplicate.
   for ( int n = 0; n < obj_count; ++n ) {

      if ( ( objects[n].name != NULL ) && ( *objects[n].name != '\0' ) ) {
         string obj_name1 = objects[n].name;

         for ( int k = n + 1; k < obj_count; ++k ) {
            if ( ( objects[k].name != NULL ) && ( *objects[k].name != '\0' ) ) {
               string obj_name2 = objects[k].name;

               if ( obj_name1 == obj_name2 ) {
                  ostringstream errmsg;
                  errmsg << "Manager::verify_object_and_interaction_arrays():" << __LINE__
                         << " ERROR: Object instance '" << obj_name1
                         << "' at array index " << n << " has the same name as"
                         << " object instance '" << obj_name2
                         << "' at array index " << k << ". Please check your"
                         << " input or modified-data files to make sure the"
                         << " object instance names are unique with no duplicates.\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }
         }
      }
   }

   // Check for the error condition of a valid interaction count but a null
   // interactions array.
   if ( ( inter_count > 0 ) && ( interactions == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::verify_object_and_interaction_arrays():" << __LINE__
             << " ERROR: Unexpected NULL 'interactions' array for a non zero"
             << " inter_count:" << inter_count << ". Please check your input or"
             << " modified-data files to make sure the 'Manager::interactions'"
             << " array is correctly configured.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have a non-NULL interactions array but the interactions-count is
   // invalid then let the user know.
   if ( ( inter_count <= 0 ) && ( interactions != NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::verify_object_and_interaction_arrays():" << __LINE__
             << " ERROR: Unexpected " << ( ( inter_count == 0 ) ? "zero" : "negative" )
             << " inter_count:" << inter_count << " for a non-NULL 'interactions'"
             << " array. Please check your input or modified-data files to make"
             << " sure the 'Manager::interactions' array is correctly configured.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Interaction count if negative.
   if ( inter_count < 0 ) {
      inter_count = 0;
   }

   // Interactions must be unique and can not be a duplicate for a given
   // FOM-name. Only one interaction per FOM-name.
   for ( int i = 0; i < inter_count; ++i ) {

      if ( ( interactions[i].FOM_name != NULL ) && ( *interactions[i].FOM_name != '\0' ) ) {
         string inter_fom_name1 = interactions[i].FOM_name;

         for ( int k = i + 1; k < inter_count; ++k ) {
            if ( ( interactions[k].FOM_name != NULL ) && ( *interactions[k].FOM_name != '\0' ) ) {
               string inter_fom_name2 = interactions[k].FOM_name;

               if ( inter_fom_name1 == inter_fom_name2 ) {
                  ostringstream errmsg;
                  errmsg << "Manager::verify_object_and_interaction_arrays():" << __LINE__
                         << " ERROR: Interaction '" << inter_fom_name1
                         << "' at array index " << i << " has the same FOM name"
                         << " as interaction '" << inter_fom_name2
                         << "' at array index " << k << ". Please check your"
                         << " input or modified-data files to make sure the"
                         << " interaction FOM names are unique with no duplicates.\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }
         }
      }
   }

   // Get a comma separated list of the execution control interaction FOM names.
   VectorOfStrings exec_fom_names_vector;
   StringUtilities::tokenize( execution_control->get_interaction_FOM_names(),
                              exec_fom_names_vector,
                              "," );

   // Make sure there is not already a user defined Interaction that uses
   // the same interaction FOM name as the execution control interaction.
   for ( int i = 0; i < exec_fom_names_vector.size(); ++i ) {

      // Make sure Execution Control interactins names are not duplicates.
      for ( int n = i + 1; n < exec_fom_names_vector.size(); ++n ) {
         if ( exec_fom_names_vector[n] == exec_fom_names_vector[i] ) {
            ostringstream errmsg;
            errmsg << "Manager::verify_object_and_interaction_arrays():" << __LINE__
                   << " ERROR: Execution Control has duplicate Interactions for '"
                   << exec_fom_names_vector[i]
                   << "'. Please check your Execution Control implementation to"
                   << " make sure only one interaction implementation exists per"
                   << " HLA interaction class FOM name.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      }

      // Check Execution Control interaction names against user defined interactions.
      for ( int k = 0; k < inter_count; ++k ) {
         if ( ( interactions[k].FOM_name != NULL ) && ( *interactions[k].FOM_name != '\0' ) ) {
            string inter_fom_name = interactions[k].FOM_name;

            if ( exec_fom_names_vector[i] == inter_fom_name ) {
               ostringstream errmsg;
               errmsg << "Manager::verify_object_and_interaction_arrays():" << __LINE__
                      << " ERROR: Execution Control Interaction '"
                      << exec_fom_names_vector[i]
                      << "' has the same FOM name as user specified interaction '"
                      << inter_fom_name << "' at array index " << k
                      << ". Please check your input or modified-data files to"
                      << " make sure the interaction FOM names are unique with"
                      << " no duplicates.\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::send_init_data()
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::send_init_data():%d Late joining \
federate so this call will be ignored.\n",
                          __LINE__ );
      }
      return;
   }

   // Go through the list of objects.
   for ( int n = 0; n < obj_count; ++n ) {
      // Make sure we have at least one piece of object init data we can send.
      if ( objects[n].any_locally_owned_published_init_attribute() ) {

         if ( execution_control->wait_for_init_data() ) {

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               message_publish( MSG_NORMAL, "Manager::send_init_data():%d '%s'\n",
                                __LINE__, objects[n].get_name() );
            }

            // Send the object init data to the other federates.
            objects[n].send_init_data();

         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               ostringstream msg;
               msg << "Manager::send_init_data():" << __LINE__
                   << " '" << objects[n].name << "'"
                   << " This call will be ignored because the Simulation"
                   << " Initialization Scheme (Type:'"
                   << execution_control->get_type()
                   << "') does not support it.\n";
               message_publish( MSG_NORMAL, msg.str().c_str() );
            }
         }
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, "Manager::send_init_data():%d Nothing to send for '%s'\n",
                             __LINE__, objects[n].get_name() );
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::send_init_data(
   char const *instance_name )
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::send_init_data():%d Late joining \
federate so the data will not be sent for '%s'.\n",
                          __LINE__, instance_name );
      }
      return;
   }

   if ( instance_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::send_init_data():" << __LINE__
             << " ERROR: Null Object Instance Name\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   wstring obj_instance_name;
   StringUtilities::to_wstring( obj_instance_name, instance_name );

   Object *obj = get_trickhla_object( obj_instance_name );

   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::send_init_data():" << __LINE__
             << " ERROR: The specified Object Instance"
             << " Name '" << instance_name << "' does not correspond to any"
             << " known object. Please check your S_define file or simulation"
             << " module to verify the settings.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Make sure we have at least one piece of object init data we can send.
   if ( obj->any_locally_owned_published_init_attribute() ) {

      if ( execution_control->wait_for_init_data() ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, "Manager::send_init_data():%d '%s'\n",
                             __LINE__, instance_name );
         }

         // Send the object init data to the other federates.
         obj->send_init_data();

      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            ostringstream msg;
            msg << "Manager::send_init_data():" << __LINE__
                << " '" << instance_name << "'"
                << " This call will be ignored because the Simulation"
                << " Initialization Scheme (Type:'"
                << execution_control->get_type()
                << "') does not support it.\n";
            message_publish( MSG_NORMAL, msg.str().c_str() );
         }
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::send_init_data():%d Nothing to send for '%s'\n",
                          __LINE__, instance_name );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::receive_init_data()
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Late joining \
federate so this call will be ignored.\n",
                          __LINE__ );
      }
      return;
   }

   // Go through the list of objects.
   for ( int n = 0; n < obj_count; ++n ) {

      // Make sure we have at least one piece of data we can receive.
      if ( objects[n].any_remotely_owned_subscribed_init_attribute() ) {

         // Only wait for REQUIRED received init data and do not block waiting
         // to receive init data if we are using the simple init scheme.
         bool obj_required = objects[n].is_required() && ( execution_control->wait_for_init_data() );

         if ( obj_required ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.\n",
                                __LINE__, objects[n].get_name(),
                                ( objects[n].is_required() ? "REQUIRED" : "not required" ) );
            }

            int64_t      wallclock_time;
            SleepTimeout print_timer( federate->wait_status_time );
            SleepTimeout sleep_timer;

            // Wait for the data to arrive.
            while ( !objects[n].is_changed() ) {

               // Check for shutdown.
               federate->check_for_shutdown_with_termination();

               sleep_timer.sleep();

               if ( !objects[n].is_changed() ) {

                  // To be more efficient, we get the time once and share it.
                  wallclock_time = sleep_timer.time();

                  if ( sleep_timer.timeout( wallclock_time ) ) {
                     sleep_timer.reset();
                     if ( !federate->is_execution_member() ) {
                        ostringstream errmsg;
                        errmsg << "Manager::receive_init_data():" << __LINE__
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
                     message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.\n",
                                      __LINE__, objects[n].get_name(),
                                      ( objects[n].is_required() ? "REQUIRED" : "not required" ) );
                  }
               }
            }
         }

         // Check for changed data which means we received something.
         if ( objects[n].is_changed() ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Received '%s'\n",
                                __LINE__, objects[n].get_name() );
            }

            // Receive the data from the publishing federate.
            objects[n].receive_init_data();
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Received nothing for '%s', and marked as %s.\n",
                                __LINE__, objects[n].get_name(),
                                ( obj_required ? "REQUIRED" : "not required" ) );
            }
         }
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Nothing to receive for '%s'\n",
                             __LINE__, objects[n].get_name() );
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::receive_init_data(
   char const *instance_name )
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Late joining federate so skipping data for '%s'\n",
                          __LINE__, instance_name );
      }
      return;
   }

   if ( instance_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::receive_init_data():" << __LINE__
             << " ERROR: Null Object Instance Name";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   wstring obj_instance_name;
   StringUtilities::to_wstring( obj_instance_name, instance_name );

   Object *obj = get_trickhla_object( obj_instance_name );

   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::receive_init_data():" << __LINE__
             << " ERROR: The specified Object Instance Name '" << instance_name
             << "' does not correspond to any known object. Please check your"
             << " S_define file or simulation module to verify the settings.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Make sure we have at least one piece of data we can receive.
   if ( obj->any_remotely_owned_subscribed_init_attribute() ) {

      // Only wait for REQUIRED received init data and do not block waiting
      // to receive init data if we are using the simple init scheme.
      bool obj_required = obj->is_required() && execution_control->wait_for_init_data();

      if ( obj_required ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.\n",
                             __LINE__, instance_name,
                             ( obj->is_required() ? "REQUIRED" : "not required" ) );
         }

         int64_t      wallclock_time;
         SleepTimeout print_timer( federate->wait_status_time );
         SleepTimeout sleep_timer;

         // Wait for the data to arrive.
         while ( !obj->is_changed() ) {

            // Check for shutdown.
            federate->check_for_shutdown_with_termination();

            sleep_timer.sleep();

            if ( !obj->is_changed() ) {

               // To be more efficient, we get the time once and share it.
               wallclock_time = sleep_timer.time();

               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "Manager::receive_init_data():" << __LINE__
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
                  message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.\n",
                                   __LINE__, instance_name,
                                   ( obj->is_required() ? "REQUIRED" : "not required" ) );
               }
            }
         }
      }

      // Check for changed data which means we received something.
      if ( obj->is_changed() ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL,
                             "Manager::receive_init_data():%d Received '%s'\n",
                             __LINE__, instance_name );
         }

         // Receive the data from the publishing federate.
         obj->receive_init_data();
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Received nothing for '%s', and marked as %s.\n",
                             __LINE__, instance_name,
                             ( obj_required ? "REQUIRED" : "not required" ) );
         }
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::receive_init_data():%d Nothing to receive for '%s'\n",
                          __LINE__, instance_name );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::clear_init_sync_points()
{
   // Clear the multiphase initialization synchronization points associated
   // with ExecutionControl initialization.
   execution_control->clear_multiphase_init_sync_points();
}

/*!
 * @job_class{initialization}
 */
void Manager::wait_for_init_sync_point(
   char const *sync_point_label )
{
   if ( !execution_control->is_wait_for_init_sync_point_supported() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         ostringstream errmsg;
         errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
                << " This call will be ignored because the Simulation"
                << " Initialization Scheme (Type:'"
                << execution_control->get_type()
                << "') does not support it.\n";
         message_publish( MSG_NORMAL, errmsg.str().c_str() );
      }
      return;
   }

   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         ostringstream errmsg;
         errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
                << " Late joining federate so this call will be ignored.\n";
         message_publish( MSG_NORMAL, errmsg.str().c_str() );
      }
      return;
   }

   if ( sync_point_label == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
             << " ERROR: Null Sync-Point Label specified!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }
   if ( sync_point_label[0] == '\0' ) {
      ostringstream errmsg;
      errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
             << " ERROR: No Sync-Point label specified!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
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
         errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
                << " ERROR: Unexpected error waiting for sync-point '"
                << sync_point_label << "'!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   } else {
      ostringstream errmsg;
      errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
             << " ERROR: This federate has not been configured to use the"
             << " synchronization-point label '" << sync_point_label
             << "' as a multiphase initialization sync-point. Please check"
             << " your input.py file to ensure your federate adds the"
             << " multiphase initialization sync-point:\n"
             << "federate.add_multiphase_init_sync_point( '"
             << sync_point_label << "' )\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::request_data_update(
   wstring const &instance_name )
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      string name_str;
      StringUtilities::to_string( name_str, instance_name );
      message_publish( MSG_NORMAL, "Manager::request_data_update():%d Object:'%s'\n",
                       __LINE__, name_str.c_str() );
   }

   bool found = false;

   // First check to see if asking for and ExecutionConfiguration update.
   if ( is_execution_configuration_used() ) {
      wstring ws_exec_config_name;
      StringUtilities::to_wstring( ws_exec_config_name,
                                   get_execution_configuration()->get_name() );
      if ( instance_name == ws_exec_config_name ) {
         found = true;
         get_execution_configuration()->request_attribute_value_update();
      }
   }

   // If not ExecutionConfiguration, then check for other objects.
   if ( !found ) {
      Object *obj = get_trickhla_object( instance_name );
      if ( obj != NULL ) {
         obj->request_attribute_value_update();
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::request_data_update(
   char const *instance_name )
{
   if ( ( instance_name == NULL ) || ( instance_name[0] == '\0' ) ) {
      return;
   }

   wstring ws_obj_instance_name;
   StringUtilities::to_wstring( ws_obj_instance_name, instance_name );

   return request_data_update( ws_obj_instance_name );
}

/*!
 * @job_class{initialization}
 */
void Manager::object_instance_name_reservation_succeeded(
   wstring const &obj_instance_name )
{

   // If the object instance isn't recognized by ExecutionControl, then
   // handle it here.
   if ( !execution_control->object_instance_name_reservation_succeeded( obj_instance_name ) ) {

      Object *trickhla_obj = get_trickhla_object( obj_instance_name );
      if ( trickhla_obj != NULL ) {
         trickhla_obj->set_name_registered();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, "Manager::object_instance_name_reservation_succeeded():%d \
RESERVED Object Instance Name '%s'\n",
                             __LINE__, trickhla_obj->get_name() );
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::object_instance_name_reservation_failed(
   wstring const &obj_instance_name )
{

   // Different ExecutionControl mechanisms will handle object instance name
   // failure differently. So, check with the ExecutionControl to perform
   // any specialized failure handling. If the method returns 'true' then
   // it's not a fatal error; otherwise, continue with error handling and
   // exit.
   if ( execution_control->object_instance_name_reservation_failed( obj_instance_name ) ) {
      return;
   }

   string name_str;
   StringUtilities::to_string( name_str, obj_instance_name );

   // Anything beyond this point is fatal.
   message_publish( MSG_WARNING, "Manager::object_instance_name_reservation_failed():%d \
Name:'%s' Please check your input or modified data files to make sure the \
object instance name is unique, no duplicates, within the Federation. For \
example, try using fed_name.object_FOM_name for the object instance name. \
Also, an object should be owned by only one Federate so one common mistake is \
to have the 'create_HLA_instance' flag for the same object being set to true \
for more than one Federate.\n",
                    __LINE__, name_str.c_str() );

   wstring obj_name;
   for ( int n = 0; n < obj_count; ++n ) {
      StringUtilities::to_wstring( obj_name, objects[n].get_name() );
      if ( obj_name == obj_instance_name ) {
         if ( objects[n].is_create_HLA_instance() ) {
            message_publish( MSG_WARNING, "Manager::object_instance_name_reservation_failed():%d\
\n   ** You specified that this Federate can \
rejoin the Federation but the original instance attributes could not be located \
in order to re-acquire ownership. They were either deleted, or are orphans in the \
Federation with no possibility of regaining ownership. **\n   ** In order for \
the rejoin to succeed, you must resign this Federate with the directive to divest \
ownership of its instance attributes. This is accomplished by setting the \
'THLA.federate.can_rejoin_federation' flag to true in the input.py file which \
resigned this Federate. **\n   ** Note: In order for the Federation rejoin to \
be successful, make sure that there is at least one other Federate set up to \
publish at least one of the attributes (by setting the 'publish' flag to true in \
another Federate). This is necessary for the successful transfer of ownership \
which keeps the instance attribute's object from becoming a Federation orphan. **\n",
                             __LINE__ );
         }
      }
   }

   // Bad things have happened if the name reservation failed since it should
   // be unique to our object, so quit the simulation. However, since we are
   // running in a child thread created by the RTI, we need to tell the Trick
   // Executive to exit the simulation.
   exec_set_exec_command( ExitCmd );

   // Bail from the execution just in case the above command fails
   ostringstream errmsg;
   errmsg << "Manager::object_instance_name_reservation_failed():" << __LINE__
          << " Exiting...\n";
   DebugHandler::terminate_with_message( errmsg.str() );
}

/*!
 * @job_class{initialization}
 */
void Manager::add_object_to_map(
   Object *object )
{
   // Add the registered ExecutionConfiguration object instance to the map
   // only if it is not already in it.
   if ( ( object->is_instance_handle_valid() )
        && ( object_map.find( object->get_instance_handle() ) == object_map.end() ) ) {
      object_map[object->get_instance_handle()] = object;
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_all_ref_attributes()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::setup_all_ref_attributes():%d\n",
                       __LINE__ );
   }

   // Create the map of object instance names to object array indexes.
   obj_name_index_map.clear();
   for ( int n = 0; n < obj_count; ++n ) {
      obj_name_index_map[objects[n].get_name_string()] = n;
   }

   // Make sure the object-map is empty/clear before we continue.
   object_map.clear();

   if ( is_execution_configuration_used() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::setup_all_ref_attributes():%d Execution-Configuration \n",
                          __LINE__ );
      }
      setup_object_ref_attributes( 1, get_execution_configuration() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::setup_all_ref_attributes():%d Objects: %d \n",
                       __LINE__, obj_count );
   }
   setup_object_ref_attributes( obj_count, objects );

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::setup_all_ref_attributes():%d Interactions \n",
                       __LINE__ );
   }
   setup_interaction_ref_attributes();
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_object_ref_attributes(
   int const data_obj_count,
   Object   *data_objects )
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      message_publish( MSG_NORMAL, "Manager::setup_object_ref_attributes():%d Already initialized.\n",
                       __LINE__ );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::setup_object_ref_attributes():%d \n",
                       __LINE__ );
   }

   // Resolve all the Ref-Attributes for all the simulation initialization
   // objects and attributes.
   for ( int n = 0; n < data_obj_count; ++n ) {

      // Initialize the TrickHLA-Object before we use it.
      data_objects[n].initialize( this );

      ostringstream msg;

      int const  attr_count = data_objects[n].get_attribute_count();
      Attribute *attrs      = data_objects[n].get_attributes();

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         msg << "Manager::setup_object_ref_attributes()" << __LINE__ << '\n'
             << "--------------- Trick REF-Attributes ---------------\n"
             << " Object:'" << data_objects[n].get_name() << "'"
             << " FOM-Name:'" << data_objects[n].get_FOM_name() << "'"
             << " Create HLA Instance:"
             << ( data_objects[n].is_create_HLA_instance() ? "Yes" : "No" )
             << " Attribute count:" << attr_count << '\n';
      }

      // Process the attributes for this object.
      for ( int i = 0; i < attr_count; ++i ) {

         // Initialize the TrickHLA-Attribute before we use it.
         attrs[i].initialize( data_objects[n].get_FOM_name(), n, i );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attrs[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attrs[i].get_trick_name() << "'" << '\n';
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_interaction_ref_attributes()
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      message_publish( MSG_NORMAL, "Manager::setup_interaction_ref_attributes():%d Already initialized.\n",
                       __LINE__ );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::setup_interaction_ref_attributes():%d\n",
                       __LINE__ );
   }

   // Interactions.
   for ( int n = 0; n < inter_count; ++n ) {
      ostringstream msg;

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         msg << "Manager::setup_interaction_ref_attributes():" << __LINE__ << '\n'
             << "--------------- Trick REF-Attributes ---------------\n"
             << " FOM-Interaction:'" << interactions[n].get_FOM_name() << "'\n";
      }

      // Initialize the TrickHLA Interaction before we use it.
      interactions[n].initialize( this );

      int const  param_count = interactions[n].get_parameter_count();
      Parameter *params      = interactions[n].get_parameters();

      // Process the attributes for this object.
      for ( int i = 0; i < param_count; ++i ) {

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "   " << ( i + 1 ) << "/" << param_count
                << " FOM-Parameter:'" << params[i].get_FOM_name() << "'"
                << " Trick-Name:'" << params[i].get_trick_name() << "'\n";
         }

         // Initialize the TrickHLA Parameter.
         params[i].initialize( interactions[n].get_FOM_name(), n, i );
      }

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }

   // Tell the ExecutionControl object to setup the appropriate Trick Ref
   // ATTRIBUTES associated with the execution control mechanism.
   execution_control->setup_interaction_ref_attributes();
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_all_RTI_handles()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::setup_all_RTI_handles():%d\n",
                       __LINE__ );
   }

   // Set up the object RTI handles for the ExecutionControl mechanisms.
   execution_control->setup_object_RTI_handles();

   // Set up the object RTI handles for the simulation data objects.
   setup_object_RTI_handles( obj_count, objects );

   // Set up the object RTI handles for the ExecutionControl mechanisms.
   execution_control->setup_interaction_RTI_handles();

   // Simulation Interactions.
   setup_interaction_RTI_handles( inter_count, interactions );
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_object_RTI_handles(
   int const data_obj_count,
   Object   *data_objects )
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      message_publish( MSG_NORMAL, "Manager::setup_object_RTI_handles():%d Already initialized.\n",
                       __LINE__ );
      return;
   }

   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::setup_object_RTI_handles():%d\n",
                       __LINE__ );
   }

   char const *obj_FOM_name  = "";
   char const *attr_FOM_name = "";
   int         FOM_name_type = 0; // 0:N/A 1:Object 2:Attribute - What name are we dealing with.

   // Initialize the Object and Attribute RTI handles.
   try {
      wstring ws_FOM_name = L"";

      // Resolve all the handles/ID's for the objects and attributes.
      for ( int n = 0; n < data_obj_count; ++n ) {
         ostringstream msg;

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "Manager::setup_object_RTI_handles()" << __LINE__ << '\n'
                << "----------------- RTI Handles (Objects & Attributes) ---------------"
                << '\n'
                << "Getting RTI Object-Class-Handle for"
                << " Object:'" << data_objects[n].get_name() << "'"
                << " FOM-Name:'" << data_objects[n].get_FOM_name() << "'\n";
         }

         // Create the wide-string object FOM name.
         FOM_name_type = 1; // Object
         obj_FOM_name  = data_objects[n].get_FOM_name();
         StringUtilities::to_wstring( ws_FOM_name, obj_FOM_name );

         // Get the class handle for the given object FOM name.
         data_objects[n].set_class_handle( rti_amb->getObjectClassHandle( ws_FOM_name ) );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            string handle_str;
            StringUtilities::to_string( handle_str, data_objects[n].get_class_handle() );
            msg << "  Result for"
                << " Object:'" << data_objects[n].get_name() << "'"
                << " FOM-Name:'" << data_objects[n].get_FOM_name() << "'"
                << " Class-ID:" << handle_str << '\n';
         }

         int const  attr_count = data_objects[n].get_attribute_count();
         Attribute *attrs      = data_objects[n].get_attributes();

         // Resolve the handles/ID's for the attributes.
         for ( int i = 0; i < attr_count; ++i ) {

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               msg << "\tGetting RTI Attribute-Handle for '"
                   << data_objects[n].get_FOM_name() << "'->'"
                   << attrs[i].get_FOM_name() << "'\n";
            }

            // Create the wide-string Attribute FOM name.
            FOM_name_type = 2; // Attribute
            attr_FOM_name = attrs[i].get_FOM_name();
            StringUtilities::to_wstring( ws_FOM_name, attr_FOM_name );

            // Get the Attribute-Handle from the RTI.
            attrs[i].set_attribute_handle(
               rti_amb->getAttributeHandle( data_objects[n].get_class_handle(), ws_FOM_name ) );

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               string id_str;
               StringUtilities::to_string( id_str, attrs[i].get_attribute_handle() );
               msg << "\t  Result for Attribute '"
                   << data_objects[n].get_FOM_name() << "'->'"
                   << attrs[i].get_FOM_name() << "'"
                   << " Attribute-ID:" << id_str << '\n';
            }
         }

         // Make sure we build the attribute map now that the AttributeHandles
         // have been set.
         data_objects[n].build_attribute_map();

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, msg.str().c_str() );
         }
      }
   } catch ( NameNotFound const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      switch ( FOM_name_type ) {
         case 1: { // Object
            ostringstream errmsg;
            errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
                   << " ERROR: Object FOM Name '" << obj_FOM_name << "' Not Found. Please check"
                   << " your input or modified-data files to make sure the"
                   << " Object FOM Name is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         case 2: { // Attribute
            ostringstream errmsg;
            errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
                   << " ERROR: For Object FOM Name '" << obj_FOM_name << "', Attribute FOM Name '"
                   << attr_FOM_name << "' Not Found. Please check your input or"
                   << " modified-data files to make sure the Object Attribute"
                   << " FOM Name is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         default: { // FOM name we are working with is unknown.
            ostringstream errmsg;
            errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
                   << " ERROR: Object or Attribute FOM Name Not Found. Please check your input or"
                   << " modified-data files to make sure the FOM Name is"
                   << " correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
      }
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Federate Not Execution Member\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Not Connected\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " ERROR: RTIinternalError: '"
             << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " ERROR: RTI1516_EXCEPTION for '"
             << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_interaction_RTI_handles(
   int const    interactions_counter,
   Interaction *in_interactions )
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      message_publish( MSG_NORMAL, "Manager::setup_interaction_RTI_handles():%d Already initialized.\n",
                       __LINE__ );
      return;
   }

   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::setup_interaction_RTI_handles():%d\n",
                       __LINE__ );
   }

   char const *inter_FOM_name = "";
   char const *param_FOM_name = "";
   int         FOM_name_type  = 0; // 0:NA 1:Interaction 2:Parameter  What name we are dealing with.

   // Initialize the Interaction and Parameter RTI handles.
   try {
      wstring ws_FOM_name = L"";

      // Process all the Interactions.
      for ( int n = 0; n < interactions_counter; ++n ) {
         ostringstream msg;

         // The Interaction FOM name.
         FOM_name_type  = 1; // Interaction
         inter_FOM_name = in_interactions[n].get_FOM_name();
         StringUtilities::to_wstring( ws_FOM_name, inter_FOM_name );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "Manager::setup_interaction_RTI_handles()" << __LINE__ << '\n'
                << "----------------- RTI Handles (Interactions & Parameters) ---------------\n"
                << "Getting RTI Interaction-Class-Handle for"
                << " FOM-Name:'" << inter_FOM_name << "'\n";
         }

         // Get the Interaction class handle.
         in_interactions[n].set_class_handle( rti_amb->getInteractionClassHandle( ws_FOM_name ) );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            string handle_str;
            StringUtilities::to_string( handle_str, in_interactions[n].get_class_handle() );
            msg << "  Result for Interaction"
                << " FOM-Name:'" << inter_FOM_name << "'"
                << " Interaction-ID:" << handle_str << '\n';
         }

         // The parameters.
         int const  param_count = in_interactions[n].get_parameter_count();
         Parameter *params      = in_interactions[n].get_parameters();

         // Process the parameters for the interaction.
         for ( int i = 0; i < param_count; ++i ) {

            // The Parameter FOM name.
            FOM_name_type  = 2; // Parameter
            param_FOM_name = params[i].get_FOM_name();
            StringUtilities::to_wstring( ws_FOM_name, param_FOM_name );

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               msg << "\tGetting RTI Parameter-Handle for '"
                   << inter_FOM_name << "'->'" << param_FOM_name << "'\n";
            }

            // Get the Parameter Handle.
            params[i].set_parameter_handle(
               rti_amb->getParameterHandle(
                  in_interactions[n].get_class_handle(),
                  ws_FOM_name ) );

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               string handle_str;
               StringUtilities::to_string( handle_str, params[i].get_parameter_handle() );
               msg << "\t  Result for Parameter '"
                   << inter_FOM_name << "'->'" << param_FOM_name << "'"
                   << " Parameter-ID:" << handle_str << '\n';
            }
         }

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, msg.str().c_str() );
         }
      }
   } catch ( NameNotFound const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      switch ( FOM_name_type ) {
         case 1: { // Interaction
            ostringstream errmsg;
            errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
                   << " ERROR: Interaction FOM Name '" << inter_FOM_name << "' Not Found. Please"
                   << " check your input or modified-data files to make sure the"
                   << " Interaction FOM Name is correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         case 2: { // Parameter
            ostringstream errmsg;
            errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
                   << " ERROR: For Interaction FOM Name '" << inter_FOM_name
                   << "', Parameter FOM Name '" << param_FOM_name
                   << "' Not Found. Please check your input or modified-data files"
                   << " to make sure the Interaction Parameter FOM Name is"
                   << " correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         default: { // FOM name we are working with is unknown.
            ostringstream errmsg;
            errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
                   << " ERROR: Interaction or Parameter FOM Name Not Found. Please check your input"
                   << " or modified-data files to make sure the FOM Name is"
                   << " correctly specified.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
      }
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: FederateNotExecutionMember!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: NotConnected!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: RTIinternalError: '" << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: Exception for '" << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{initialization}
 */
void Manager::publish()
{
   if ( !is_RTI_ready( "publish" ) ) {
      return;
   }

   // Publish attributes for all the Trick-HLA-Objects we know about.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].publish_object_attributes();
   }

   // Publish the interactions.
   for ( int n = 0; n < inter_count; ++n ) {
      interactions[n].publish_interaction();
   }

   // Publish Execution Control objects and interactions.
   execution_control->publish();
}

/*!
 * @job_class{initialization}
 */
void Manager::unpublish()
{
   int  i, k;
   bool do_unpublish;

   if ( !is_RTI_ready( "unpublish" ) ) {
      return;
   }

   // Unpublish from all attributes for all the objects.
   for ( i = 0; i < obj_count; ++i ) {
      // Only unpublish an object class if we had published at least
      // one attribute.
      if ( objects[i].any_attribute_published() ) {
         do_unpublish = true;
         for ( k = 0; ( k < i ) && do_unpublish; ++k ) {
            // Unpublish an object Class only once, so see if we have already
            // unpublished from the same object class that was published.
            if ( objects[k].any_attribute_published()
                 && ( objects[i].get_class_handle() == objects[k].get_class_handle() ) ) {
               do_unpublish = false;
            }
         }
         if ( do_unpublish ) {
            objects[i].unpublish_all_object_attributes();
         }
      }
   }

   // Unpublish all the interactions.
   for ( i = 0; i < inter_count; ++i ) {
      // Only unpublish an interaction that we publish.
      if ( interactions[i].is_publish() ) {
         do_unpublish = true;
         for ( k = 0; ( k < i ) && do_unpublish; ++k ) {
            // Unpublish an interaction Class only once, so see if we have
            // already unpublished the same interaction class that was published.
            if ( interactions[k].is_publish()
                 && ( interactions[i].get_class_handle() == interactions[k].get_class_handle() ) ) {
               do_unpublish = false;
            }
         }
         if ( do_unpublish ) {
            interactions[i].unpublish_interaction();
         }
      }
   }

   // Unpublish Execution Control objects and interactions.
   execution_control->unpublish();
}

/*!
 * @job_class{initialization}
 */
void Manager::subscribe()
{
   if ( !is_RTI_ready( "subscribe" ) ) {
      return;
   }

   // Subscribe to attributes for all the Trick-HLA-Objects we know about.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].subscribe_to_object_attributes();
   }

   // Subscribe to the interactions.
   for ( int n = 0; n < inter_count; ++n ) {
      interactions[n].subscribe_to_interaction();
   }

   // Subscribe to anything needed for the execution control mechanisms.
   execution_control->subscribe();
}

/*!
 * @job_class{initialization}
 */
void Manager::unsubscribe()
{
   int  i, k;
   bool do_unsubscribe;

   if ( !is_RTI_ready( "unsubscribe" ) ) {
      return;
   }

   // Unsubscribe from all attributes for all the objects.
   for ( i = 0; i < obj_count; ++i ) {
      // Only unsubscribe from an object class if we had subscribed to at
      // least one attribute.
      if ( objects[i].any_attribute_subscribed() ) {
         do_unsubscribe = true;
         for ( k = 0; ( k < i ) && do_unsubscribe; ++k ) {
            // Unsubscribe from an object Class only once, so see if
            // we have already unsubscribed from the same object class
            // that was subscribed to.
            if ( objects[k].any_attribute_subscribed()
                 && ( objects[i].get_class_handle() == objects[k].get_class_handle() ) ) {
               do_unsubscribe = false;
            }
         }
         if ( do_unsubscribe ) {
            objects[i].unsubscribe_all_object_attributes();
         }
      }
   }

   // Unsubscribe from all the interactions.
   for ( i = 0; i < inter_count; ++i ) {
      // Only unsubscribe from interactions that are subscribed to.
      if ( interactions[i].is_subscribe() ) {
         do_unsubscribe = true;
         for ( k = 0; ( k < i ) && do_unsubscribe; ++k ) {
            // Unsubscribe from an interaction Class only once, so see if
            // we have already unsubscribed from the same interaction class
            // that was subscribed to.
            if ( interactions[k].is_subscribe()
                 && ( interactions[i].get_class_handle() == interactions[k].get_class_handle() ) ) {
               do_unsubscribe = false;
            }
         }
         if ( do_unsubscribe ) {
            interactions[i].unsubscribe_from_interaction();
         }
      }
   }

   // Unsubscribe to anything needed for the execution control mechanisms.
   execution_control->unsubscribe();
}

/*!
 * @job_class{initialization}
 */
void Manager::publish_and_subscribe()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::publish_and_subscribe():%d\n",
                       __LINE__ );
   }
   subscribe();
   publish();
}

/*!
 * @job_class{initialization}
 */
void Manager::reserve_object_names_with_RTI()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::reserve_object_names_with_RTI():%d\n",
                       __LINE__ );
   }

   // For the locally owned objects, reserve the object instance name with
   // the RTI.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].reserve_object_name_with_RTI();
   }
}

/*!
 * @details Calling this function will block until all the object instances
 * names for the locally owned objects have been reserved.
 * @job_class{initialization}
 */
void Manager::wait_for_reservation_of_object_names()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::wait_for_reservation_of_object_names():%d\n",
                       __LINE__ );
   }

   // Simulation object names.
   if ( obj_count > 0 ) {
      // Wait for each RTI object instance name to be registered with the RTI,
      // but for only the names we requested registration for.
      for ( int n = 0; n < obj_count; ++n ) {
         objects[n].wait_for_object_name_reservation();
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::wait_for_reservation_of_object_names():%d All Object instance names reserved.\n",
                          __LINE__ );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::wait_for_reservation_of_object_names():%d No Object instance names to reserve.\n",
                          __LINE__ );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::register_objects_with_RTI()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::register_objects_with_RTI():%d\n",
                       __LINE__ );
   }

   // Have the ExecutionControl register objects it needs with the RTI.
   execution_control->register_objects_with_RTI();

   // For the locally owned objects register it with the RTI to get its
   // RTI object instance ID.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].register_object_with_RTI();

      // Add the registered object instance to the map and only if it is
      // not already in it.
      if ( ( objects[n].is_instance_handle_valid() )
           && ( object_map.find( objects[n].get_instance_handle() ) == object_map.end() ) ) {
         object_map[objects[n].get_instance_handle()] = &objects[n];
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_preferred_order_with_RTI()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::setup_preferred_order_with_RTI():%d\n",
                       __LINE__ );
   }

   if ( is_execution_configuration_used() ) {
      // Register the execution configuration object.
      get_execution_configuration()->setup_preferred_order_with_RTI();
   }

   // Setup the preferred order for all the object attributes.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].setup_preferred_order_with_RTI();
   }

   // Setup the preferred order for all the interactions.
   for ( int i = 0; i < inter_count; ++i ) {
      interactions[i].setup_preferred_order_with_RTI();
   }
}

/*!
 * @details Calling this function will block until all the required object
 * instances in the Federation have been registered.
 * @job_class{initialization}
 */
void Manager::wait_for_registration_of_required_objects()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::wait_for_registration_of_required_objects():%d\n",
                       __LINE__ );
   }

   int  required_obj_cnt;
   int  registered_obj_cnt;
   int  current_registered_obj_cnt  = 0;
   int  total_obj_cnt               = obj_count;
   int  current_required_obj_cnt    = 0;
   int  total_required_obj_cnt      = 0;
   bool print_summary               = DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER );
   bool print_only_unregistered_obj = false;
   bool any_unregistered_required_obj;

   if ( is_execution_configuration_used() ) {
      // Make sure to count the exec-config object.
      ++total_obj_cnt;

      // Determine if the Execution-Configuration object is required and it should be.
      if ( get_execution_configuration()->is_required() ) {
         ++total_required_obj_cnt;
      }
   }

   // Loop through all of the objects to count the # of required objects; do not
   // assume that all of them are required!
   for ( int n = 0; n < obj_count; ++n ) {
      if ( objects[n].is_required() ) {
         ++total_required_obj_cnt;
      }
   }

   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   do {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // Data objects.
      if ( current_required_obj_cnt < total_required_obj_cnt ) {
         required_obj_cnt   = 0;
         registered_obj_cnt = 0;

         // Concurrency critical code section for discovered objects being set
         // in FedAmb callback.
         {
            // When auto_unlock_mutex goes out of scope it automatically unlocks
            // the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &obj_discovery_mutex );

            if ( is_execution_configuration_used() ) {
               // Determine if the Execution-Configuration object has been
               // registered and only if it is required.
               if ( get_execution_configuration()->is_instance_handle_valid() ) {
                  ++registered_obj_cnt;
                  if ( get_execution_configuration()->is_required() ) {
                     ++required_obj_cnt;
                  }
               }
            }

            // Determine how many data objects have been registered and only if
            // they are required.
            for ( int n = 0; n < obj_count; ++n ) {
               if ( objects[n].is_instance_handle_valid() ) {
                  ++registered_obj_cnt;
                  if ( objects[n].is_required() ) {
                     ++required_obj_cnt;
                  }
               }
            }
         }

         // If we have a new registration count then update the object
         // registration count and set the flag to show a new summary.
         if ( registered_obj_cnt > current_registered_obj_cnt ) {
            current_registered_obj_cnt = registered_obj_cnt;
            if ( !print_summary ) {
               print_summary = DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER );
            }
         }

         // Update the current count of the Required registered objects.
         if ( required_obj_cnt > current_required_obj_cnt ) {
            current_required_obj_cnt = required_obj_cnt;
         }
      }

      // Print a summary of what objects are registered and which ones are not.
      if ( print_summary || print_only_unregistered_obj ) {

         // If we need to print a summary because we have more registered
         // objects then clear the flag to only print unregistered objects
         // so that we get the full list.
         if ( print_summary ) {
            print_only_unregistered_obj = false;
         }

         // Build the summary as an output string stream.
         ostringstream summary;
         summary << "Manager::wait_for_registration_of_required_objects():"
                 << __LINE__ << "\nREQUIRED-OBJECTS:" << total_required_obj_cnt
                 << "  Total-Objects:" << total_obj_cnt;

         if ( print_only_unregistered_obj ) {
            summary << "\nSHOWING ONLY UNREGISTERED OBJECTS:";
         }

         // Concurrency critical code section for discovered objects being set
         // in FedAmb callback.
         {
            // When auto_unlock_mutex goes out of scope it automatically unlocks
            // the mutex even if there is an exception.
            MutexProtection auto_unlock_mutex( &obj_discovery_mutex );

            int cnt = 1;
            if ( is_execution_configuration_used() ) {
               if ( !print_only_unregistered_obj
                    || !get_execution_configuration()->is_instance_handle_valid() ) {

                  // Execution-Configuration object
                  summary << "\n  " << cnt << ":Object instance '" << get_execution_configuration()->get_name() << "' ";

                  if ( get_execution_configuration()->is_instance_handle_valid() ) {
                     string id_str;
                     StringUtilities::to_string( id_str, get_execution_configuration()->get_instance_handle() );
                     summary << "(ID:" << id_str << ") ";
                  }
                  summary << "for class '" << get_execution_configuration()->get_FOM_name() << "' is "
                          << ( get_execution_configuration()->is_required() ? "REQUIRED" : "not required" )
                          << " and is "
                          << ( get_execution_configuration()->is_instance_handle_valid() ? "REGISTERED" : "Not Registered" );
               }
               ++cnt; // Count the execution configuration.
            }

            for ( int n = 0; n < obj_count; ++n ) {
               if ( !print_only_unregistered_obj
                    || !objects[n].is_instance_handle_valid() ) {

                  // Adjust index based on sim-config or exec-config objects existing.
                  summary << "\n  " << ( n + cnt ) << ":Object instance '"
                          << objects[n].get_name() << "' ";

                  if ( objects[n].is_instance_handle_valid() ) {
                     string id_str;
                     StringUtilities::to_string( id_str, objects[n].get_instance_handle() );
                     summary << "(ID:" << id_str << ") ";
                  }
                  summary << "for class '" << objects[n].get_FOM_name() << "' is "
                          << ( objects[n].is_required() ? "REQUIRED" : "not required" )
                          << " and is "
                          << ( objects[n].is_instance_handle_valid() ? "REGISTERED" : "Not Registered" );
               }
            }
         }
         summary << '\n';

         // Display the summary.
         message_publish( MSG_NORMAL, summary.str().c_str() );

         // Reset the flags for printing a summary.
         print_summary               = false;
         print_only_unregistered_obj = false;
      }

      // Determine if we have any unregistered objects.
      any_unregistered_required_obj = ( current_required_obj_cnt < total_required_obj_cnt );

      // Wait a little while to allow the objects to be registered.
      if ( any_unregistered_required_obj ) {
         sleep_timer.sleep();

         // Check again to see if we have any unregistered objects.
         any_unregistered_required_obj = ( current_required_obj_cnt < total_required_obj_cnt ); // cppcheck-suppress [knownConditionTrueFalse]

         if ( any_unregistered_required_obj ) { // cppcheck-suppress [knownConditionTrueFalse,unmatchedSuppression]

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            // If we timeout check to see if we are still an execution member.
            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();

               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Manager::wait_for_registration_of_required_objects():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to"
                         << " the RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation"
                         << " at the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            // Determine if we should print a summary of unregistered objects.
            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();

               // If we timeout, then force print a summary of only the
               // unregistered objects just in case the user has a bad
               // configuration and they are in deadlock here. Print only
               // unregistered objects to keep the list short and to the point.
               print_only_unregistered_obj = true;
            }
         }
      }
   } while ( any_unregistered_required_obj );

   // Concurrency critical code section for discovered objects being set
   // in FedAmb callback.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &obj_discovery_mutex );

      if ( is_execution_configuration_used() ) {
         // Add the exec-config instance to the map if it is not already in it.
         if ( ( get_execution_configuration()->is_instance_handle_valid() )
              && ( object_map.find( get_execution_configuration()->get_instance_handle() ) == object_map.end() ) ) {
            object_map[get_execution_configuration()->get_instance_handle()] = get_execution_configuration();
         }
      }

      // Add all valid, registered object instances to the map and only if they are
      // not already in it.
      for ( int n = 0; n < obj_count; ++n ) {
         if ( ( objects[n].is_instance_handle_valid() )
              && ( object_map.find( objects[n].get_instance_handle() ) == object_map.end() ) ) {
            object_map[objects[n].get_instance_handle()] = &objects[n];
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::set_all_object_instance_handles_by_name()
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      message_publish( MSG_NORMAL, "Manager::set_all_object_instance_handles_by_name():%d Already initialized.\n",
                       __LINE__ );
      return;
   }

   // Clear the map since we are going to rebuild it from the function
   // calls below.
   object_map.clear();

   if ( is_execution_configuration_used() ) {
      // Execution Configuration object.
      set_object_instance_handles_by_name( 1, get_execution_configuration() );
   }

   // Simulation data objects.
   set_object_instance_handles_by_name( obj_count, objects );
}

/*!
 * @job_class{initialization}
 */
void Manager::set_object_instance_handles_by_name(
   int const data_obj_count,
   Object   *data_objects )
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      message_publish( MSG_NORMAL, "Manager::set_object_instance_handles_by_name():%d Already initialized.\n",
                       __LINE__ );
      return;
   }

   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   ostringstream summary;
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      summary << "Manager::set_object_instance_handles_by_name():"
              << __LINE__;
   }

   try {
      wstring ws_instance_name = L"";

      // Resolve all the handles/ID's for the objects and attributes.
      for ( int n = 0; n < data_obj_count; ++n ) {

         // Create the wide-string version of the instance name.
         char const *instance_name = data_objects[n].get_name();
         StringUtilities::to_wstring( ws_instance_name, instance_name );

         try {
            // Set the instance handle based on the instance name. We do this
            // even for objects that are not required because they may have
            // been used at some point during the federation execution.
            data_objects[n].set_instance_handle( rti_amb->getObjectInstanceHandle( ws_instance_name ) );

            // Now that we have an instance handle, add it to the object-map if
            // it is not already in it.
            if ( ( data_objects[n].is_instance_handle_valid() )
                 && ( object_map.find( data_objects[n].get_instance_handle() ) == object_map.end() ) ) {
               object_map[data_objects[n].get_instance_handle()] = &data_objects[n];
            }

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               string id_str;
               StringUtilities::to_string( id_str, data_objects[n].get_instance_handle() );
               summary << "\n    Object:'" << data_objects[n].get_name()
                       << "'  ID:" << id_str
                       << "  ID-Valid:" << ( data_objects[n].is_instance_handle_valid() ? "Yes" : "No" )
                       << "  Obj-Required:" << ( data_objects[n].is_required() ? "Yes" : "No" );
            }
         } catch ( ObjectInstanceNotKnown const &e ) {
            // If this object is not required, just ignore the object instance
            // not known exception, otherwise handle the exception.
            if ( data_objects[n].is_required() ) {
               // Macro to restore the saved FPU Control Word register value.
               TRICKHLA_RESTORE_FPU_CONTROL_WORD;
               TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

               ostringstream errmsg;
               errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
                      << " ERROR: Object Instance Not Known for '"
                      << ( instance_name != NULL ? instance_name : "" ) << "'\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            } else {
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
                  summary << "\n    Object:'" << data_objects[n].get_name()
                          << "'  ID:Inatance-Not-Known  ID-Valid:No  Obj-Required:No";
               }
            }
         }
      }
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Federation Not Execution Member\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Not Connected\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: RTIinternalError: '" << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: RTI1516_EXCEPTION for '"
             << rti_err_msg << "'\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      summary << '\n';
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::provide_attribute_update(
   ObjectInstanceHandle const &theObject,
   AttributeHandleSet const   &theAttributes )
{
   // Determine which data object the user is requesting an update for.
   Object *trickhla_obj = get_trickhla_object( theObject );
   if ( trickhla_obj != NULL ) {
      trickhla_obj->provide_attribute_update( theAttributes );
   } else {
      execution_control->provide_attribute_update( theObject, theAttributes );
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::send_cyclic_and_requested_data()
{
   // Current time values.
   int64_t const sim_time_in_base_time = Int64BaseTime::to_base_time( exec_get_sim_time() );
   int64_t const granted_base_time     = get_granted_base_time();
   int64_t const lookahead_base_time   = federate->is_zero_lookahead_time()
                                            ? 0LL
                                            : federate->get_lookahead_in_base_time();

   // The update_time should be the current granted time plus the data cycle
   // delta time for this job if HLA Time Management is enabled otherwise it
   // is the simulation time plus the cycle delta time for this job. Also, the
   // dt value would then be the job cycle delta time for this job for this
   // function. 11/28/2006 DDexter
   //
   // When Tsim+dt == Tgrant+Lookahead
   // Tgrant          Tgrant + Lookahead
   // +---------------+---------------
   // Tsim            Tsim + dt
   //
   // When Tsim+dt > Tgrant+Lookahead
   // Tgrant          Tmin = Tgrant + Lookahead
   // +---------------+--------+------
   // Tsim                     Tsim + dt
   //
   // Even when using HLA Time Management the simulation time (Tsim) will
   // not match the Granted time (Tgrant) for some circumstances, which is
   // the case for a late joining federate. The data cycle time (dt) is how
   // often we send and receive data, which may or may not match the lookahead.
   // This is why we prefer to use an updated time of Tupdate = Tgrant + dt.
   int64_t   dt      = federate->get_HLA_cycle_time_in_base_time();
   int64_t   prev_dt = dt;
   Int64Time update_time( granted_base_time + dt );

   // Make sure the update time is not less than the granted time + lookahead,
   // which happens if the delta-time step is less than the lookahead time.
   if ( dt < lookahead_base_time ) {
      update_time.set( granted_base_time + lookahead_base_time );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::send_cyclic_and_requested_data():%d HLA-time:%.12G seconds.\n",
                       __LINE__, update_time.get_time_in_seconds() );
   }

   // Send any ExecutionControl data requested.
   execution_control->send_requested_data( update_time );

   // Send data to remote RTI federates for each of the objects.
   for ( int obj_index = 0; obj_index < this->obj_count; ++obj_index ) {

      // Only send data if we are on the data cycle time boundary for this object.
      if ( federate->on_data_cycle_boundary_for_obj( obj_index, sim_time_in_base_time ) ) {

         // Get the cyclic data time for the object.
         dt = federate->get_data_cycle_base_time_for_obj( obj_index, federate->get_HLA_cycle_time_in_base_time() );

         // Reuse the update_time if the data cycle time (dt) is the same.
         if ( dt != prev_dt ) {
            prev_dt = dt;
            update_time.set( granted_base_time + dt );

            // Make sure the update time is not less than the granted time + lookahead,
            // which happens if the delta-time step is less than the lookahead time.
            if ( dt < lookahead_base_time ) {
               update_time.set( granted_base_time + lookahead_base_time );
            }
         }

         // Send the data for the object using the cycle time for this object.
         objects[obj_index].send_cyclic_and_requested_data( update_time );
      }
   }
}

/*!
 * @details If the object is owned remotely, this function copies its internal
 * data into simulation object and marks the object as "unchanged". This data
 * was deposited by the reflect callback and marked as "changed". By marking
 * it as unchanged, we avoid copying the same data over and over. If the object
 * is locally owned, we shouldn't be receiving any remote data anyway and if
 * we were to -- bogusly -- copy it to the internal byte buffer, we'd
 * continually reset our local simulation.
 * @job_class{scheduled}
 */
void Manager::receive_cyclic_data()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::receive_cyclic_data():%d\n", __LINE__ );
   }

   int64_t const sim_time_in_base_time = Int64BaseTime::to_base_time( exec_get_sim_time() );

   // Receive and process any updates for ExecutionControl.
   execution_control->receive_cyclic_data();

   // Receive data from remote RTI federates for each of the objects.
   for ( int n = 0; n < obj_count; ++n ) {

      // Only receive data if we are on the data cycle time boundary for this object.
      if ( federate->on_data_cycle_boundary_for_obj( n, sim_time_in_base_time ) ) {
         objects[n].receive_cyclic_data();
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::process_interactions()
{
   // Process any ExecutionControl mode transitions.
   execution_control->process_mode_interaction();

   // Just return if the interaction queue is empty.
   if ( interactions_queue.empty() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::process_interactions():%d\n", __LINE__ );
   }

   // Process all the interactions in the queue.
   while ( !interactions_queue.empty() ) {

      // Get a reference to the first item on the queue.
      InteractionItem *interaction_item =
         static_cast< InteractionItem * >( interactions_queue.front() );

      switch ( interaction_item->interaction_type ) {
         case INTERACTION_TYPE_USER_DEFINED: {
            // Process the interaction if we subscribed to it and the interaction
            // index is valid.
            if ( ( interaction_item->index >= 0 )
                 && ( interaction_item->index < inter_count )
                 && interactions[interaction_item->index].is_subscribe() ) {

               interactions[interaction_item->index].extract_data( interaction_item );

               interactions[interaction_item->index].process_interaction();
            }
            break;
         }

         default: {
            ostringstream errmsg;
            errmsg << "Manager::process_interactions():" << __LINE__
                   << " FATAL ERROR: encountered an invalid interaction type: "
                   << interaction_item->interaction_type
                   << ". Verify that you are specifying the correct interaction "
                   << "type defined in 'ManagerTypeOfInteractionEnum' enum "
                   << "found in 'Manager.hh' and re-run.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
      }

      // Now that we processed the interaction-item remove it from the queue,
      // which will result in the item being deleted and no longer valid.
      interactions_queue.pop();
   }

   free_checkpoint_interactions();
}

/*!
 * @job_class{scheduled}
 */
void Manager::receive_interaction(
   InteractionClassHandle const  &theInteraction,
   ParameterHandleValueMap const &theParameterValues,
   RTI1516_USERDATA const        &theUserSuppliedTag,
   LogicalTime const             &theTime,
   bool const                     received_as_TSO )
{
   // Let the ExectionControl receive and process the interaction
   // immediately if it uses it. Otherwise handle as a user interaction.
   if ( !execution_control->receive_interaction( theInteraction,
                                                 theParameterValues,
                                                 theUserSuppliedTag,
                                                 theTime,
                                                 received_as_TSO ) ) {

      // Find the user Interaction we received data for.
      for ( int i = 0; i < inter_count; ++i ) {

         // Process the interaction if we subscribed to it and we have the same class handle.
         if ( interactions[i].is_subscribe()
              && ( interactions[i].get_class_handle() == theInteraction ) ) {

            InteractionItem *item;
            if ( received_as_TSO ) {
               item = new InteractionItem( i,
                                           INTERACTION_TYPE_USER_DEFINED,
                                           interactions[i].get_parameter_count(),
                                           interactions[i].get_parameters(),
                                           theParameterValues,
                                           theUserSuppliedTag,
                                           theTime );
            } else {
               item = new InteractionItem( i,
                                           INTERACTION_TYPE_USER_DEFINED,
                                           interactions[i].get_parameter_count(),
                                           interactions[i].get_parameters(),
                                           theParameterValues,
                                           theUserSuppliedTag );
            }

            // Add the interaction item to the queue.
            interactions_queue.push( item );

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               string handle;
               StringUtilities::to_string( handle, theInteraction );

               if ( received_as_TSO ) {
                  Int64Time _time;
                  _time.set( theTime );
                  message_publish( MSG_NORMAL, "Manager::receive_interaction():%d ID:%s, HLA-time:%G\n",
                                   __LINE__, handle.c_str(), _time.get_time_in_seconds() );
               } else {
                  message_publish( MSG_NORMAL, "Manager::receive_interaction():%d ID:%s\n",
                                   __LINE__, handle.c_str() );
               }
            }

            // Return now that we put the interaction-item into the queue
            // for processing later in the S_define main thread when the
            // manager.process_interactions() job is called to ensure data
            // coherency. Only one interaction handler per HLA interaction
            // class is supported.
            return;
         }
      }
   }
}

/*!
 * @job_class{scheduled}
 */
Object *Manager::get_trickhla_object(
   ObjectInstanceHandle const &instance_id )
{
   // We use a map with the key being the ObjectIntanceHandle for fast lookups.
   ObjectInstanceMap::const_iterator iter = object_map.find( instance_id );
   return ( ( iter != object_map.end() ) ? iter->second : NULL );
}

/*!
 * @job_class{scheduled}
 */
Object *Manager::get_trickhla_object(
   string const &obj_instance_name )
{
   // Search the data objects first.
   TrickHLAObjInstanceNameIndexMap::const_iterator iter;
   iter = obj_name_index_map.find( obj_instance_name );
   if ( iter != obj_name_index_map.end() ) {
      return ( &objects[iter->second] );
   }

   // Check for a match with the ExecutionConfiguration object associated with
   // ExecutionControl. Returns NULL if match not found.
   return ( execution_control->get_trickhla_object( obj_instance_name ) );
}

/*!
 * @job_class{scheduled}
 */
Object *Manager::get_trickhla_object(
   wstring const &obj_instance_name )
{
   string obj_instance_name_str;
   StringUtilities::to_string( obj_instance_name_str, obj_instance_name );

   return ( get_trickhla_object( obj_instance_name_str ) );
}

/*!
 * @job_class{scheduled}
 */
bool Manager::discover_object_instance(
   ObjectInstanceHandle const &theObject,
   ObjectClassHandle const    &theObjectClass,
   wstring const              &theObjectInstanceName )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &obj_discovery_mutex );

   bool return_value = false;

   // Get the unregistered TrickHLA Object for the given class handle and
   // object instance name.
   Object *trickhla_obj = get_unregistered_object( theObjectClass, theObjectInstanceName );

   // If we did not find the object by class handle and instance name then
   // get the first unregistered object that is remotely owned for the given
   // object class type.
   if ( trickhla_obj == NULL ) {

      // Get the first unregistered remotely owned object that has the
      // given object class type and only if the object instance name is
      // not required.
      trickhla_obj = get_unregistered_remote_object( theObjectClass );
   }

   // Determine if the discovered instance was for a data object.
   if ( trickhla_obj != NULL ) {
      // Set the Instance ID for the discovered object.
      trickhla_obj->set_instance_handle_and_name( theObject, theObjectInstanceName );

      // Put this discovered instance in the map of object instance handles.
      if ( object_map.find( trickhla_obj->get_instance_handle() ) == object_map.end() ) {
         object_map[theObject] = trickhla_obj;
      }

      return_value = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         string id_str;
         StringUtilities::to_string( id_str, theObject );
         message_publish( MSG_NORMAL, "Manager::discover_object_instance():%d Data-Object '%s' Instance-ID:%s\n",
                          __LINE__, trickhla_obj->get_name(), id_str.c_str() );
      }
   } else if ( ( federate != NULL ) && federate->is_MOM_HLAfederate_class( theObjectClass ) ) {

      federate->add_federate_instance_id( theObject );
      return_value = true;

      // Save into my federate's discovered federate storage area
      federate->add_MOM_HLAfederate_instance_id( theObject, theObjectInstanceName );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, theObject );
         StringUtilities::to_string( name_str, theObjectInstanceName );
         message_publish( MSG_NORMAL, "Manager::discover_object_instance():%d Discovered MOM HLA-Federate Object-Instance-ID:%s Name:'%s'\n",
                          __LINE__, id_str.c_str(), name_str.c_str() );
      }
   } else if ( ( federate != NULL ) && federate->is_MOM_HLAfederation_class( theObjectClass ) ) {

      federate->add_MOM_HLAfederation_instance_id( theObject );
      return_value = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, theObject );
         StringUtilities::to_string( name_str, theObjectInstanceName );
         message_publish( MSG_NORMAL, "Manager::discover_object_instance():%d MOM HLA-Federation '%s' Instance-ID:%s\n",
                          __LINE__, name_str.c_str(), id_str.c_str() );
      }
   }

   return return_value;
}

/*!
 * @job_class{scheduled}
 */
Object *Manager::get_unregistered_object(
   ObjectClassHandle const &theObjectClass,
   wstring const           &theObjectInstanceName )
{
   wstring ws_obj_name;

   // Search the simulation data objects first.
   for ( int n = 0; n < obj_count; ++n ) {

      // Find the object that is not registered (i.e. the instance ID == 0),
      // has the same class handle as the one specified, and has the same name
      // as the object instance name that is specified.
      if ( ( objects[n].get_class_handle() == theObjectClass )
           && ( !objects[n].is_instance_handle_valid() ) ) {

         StringUtilities::to_wstring( ws_obj_name, objects[n].get_name() );

         // Determine if the name matches the object instance name.
         if ( ws_obj_name == theObjectInstanceName ) {
            return ( &objects[n] );
         }
      }
   }

   // Check for a match with the ExecutionConfiguration object associated with
   // ExecutionControl. Returns NULL if match not found.
   return ( execution_control->get_unregistered_object( theObjectClass, theObjectInstanceName ) );
}

/*!
 * @job_class{scheduled}
 */
Object *Manager::get_unregistered_remote_object(
   ObjectClassHandle const &theObjectClass )
{
   // Search the simulation data objects first.
   for ( int n = 0; n < obj_count; ++n ) {

      // Return the first TrickHLA object that we did not create an HLA
      // instance for, has the same class handle as the one specified, is not
      // registered (i.e. the instance ID == 0), and does not have an Object
      // Instance Name associated with it, and a name is not required or the
      // user did not specify one.
      if ( ( !objects[n].is_create_HLA_instance() )
           && ( objects[n].get_class_handle() == theObjectClass )
           && ( !objects[n].is_instance_handle_valid() )
           && ( !objects[n].is_name_required()
                || ( objects[n].get_name() == NULL )
                || ( *( objects[n].get_name() ) == '\0' ) ) ) {
         return ( &objects[n] );
      }
   }

   // Check for a match with the ExecutionConfiguration object associated with
   // ExecutionControl. Returns NULL if match not found.
   return ( execution_control->get_unregistered_remote_object( theObjectClass ) );
}

/*!
 * @job_class{scheduled}
 */
void Manager::process_ownership()
{
   // Push ownership to the other federates if the push ownership
   // flag has been enabled.
   push_ownership();

   // Release ownership if we have a request to divest.
   release_ownership();

   // Pull ownership from the other federates if the pull ownership
   // flag has been enabled.
   pull_ownership();

   // Grant any request to pull the ownership.
   grant_pull_request();
}

void Manager::mark_object_as_deleted_from_federation(
   ObjectInstanceHandle const &instance_id )
{

   // First check if this is associated with ExecutionControl.
   // If so, then perform any ExecutionControl specific actions.
   // If not, then just remove the object instance.
   if ( !execution_control->mark_object_as_deleted_from_federation( instance_id ) ) {

      Object *obj = get_trickhla_object( instance_id );
      if ( obj != NULL ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            string id_str;
            StringUtilities::to_string( id_str, instance_id );
            message_publish( MSG_NORMAL, "Manager::mark_object_as_deleted_from_federation():%d Object '%s' Instance-ID:%s Valid-ID:%s \n",
                             __LINE__, obj->get_name(), id_str.c_str(),
                             ( instance_id.isValid() ? "Yes" : "No" ) );
         }
         obj->remove_object_instance();
      }
   }
}

/*!
 * @job_class{logging}
 */
void Manager::process_deleted_objects()
{
   // Process ExecutionControl deletions.
   execution_control->process_deleted_objects();

   // Search the simulation data objects, looking for deleted objects.
   for ( int n = 0; n < obj_count; ++n ) {
      if ( objects[n].process_object_deleted_from_RTI ) {
         objects[n].process_deleted_object();
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::pull_ownership()
{
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].pull_ownership();
   }
}

/*!
 * @brief Blocking function call to pull ownership of the named object
 * instance at initialization.
 * @param obj_instance_name Object instance name to pull ownership
 *  of for all attributes.
 */
void Manager::pull_ownership_at_init(
   char const *obj_instance_name )

{
   pull_ownership_at_init( obj_instance_name, NULL );
}

/*!
 * @brief Blocking function call to pull ownership of the named object
 * instance at initialization.
 * @param obj_instance_name Object instance name to pull ownership
 * of for all attributes
 * @param attribute_list Comma separated list of attributes.
 */
void Manager::pull_ownership_at_init(
   char const *obj_instance_name,
   char const *attribute_list )
{
   if ( obj_instance_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::pull_ownership_at_init():" << __LINE__
             << " ERROR: Unexpected NULL obj_instance_name specified!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   Object *obj = get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::pull_ownership_at_init():" << __LINE__
             << " ERROR: Failed to find object with instance name: '"
             << obj_instance_name << "'!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   obj->pull_ownership_at_init( attribute_list );
}

/*!
 * @brief Blocking function call to wait to handle the remote request to Pull
 * ownership object attributes to this federate.
 * @param obj_instance_name Object instance name to handle the remote
 *  pulled ownership attributes from.
 */
void Manager::handle_pulled_ownership_at_init(
   char const *obj_instance_name )
{
   if ( obj_instance_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::handle_pulled_ownership_at_init():" << __LINE__
             << " ERROR: Unexpected NULL obj_instance_name specified!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   Object *obj = get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::handle_pulled_ownership_at_init():" << __LINE__
             << " ERROR: Failed to find object with instance name: '"
             << obj_instance_name << "'!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   obj->handle_pulled_ownership_at_init();
}

/*!
 * @job_class{scheduled}
 */
void Manager::pull_ownership_upon_rejoin()
{
   for ( int n = 0; n < obj_count; ++n ) {
      if ( objects[n].is_create_HLA_instance() ) {
         objects[n].pull_ownership_upon_rejoin();
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::push_ownership()
{
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].push_ownership();
   }
}

/*!
 * @brief Blocking function call to push ownership of all the locally owned
 * object attributes.
 * @param obj_instance_name Object instance name to push ownership
 * of for all attributes.
 */
void Manager::push_ownership_at_init(
   char const *obj_instance_name )
{
   push_ownership_at_init( obj_instance_name, NULL );
}

/*!
 * @brief Blocking function call to push ownership of the named object
 * instance at initialization.
 * @param obj_instance_name Object instance name to push ownership
 * of for all attributes.
 * @param attribute_list Comma separated list of attribute FOM names.
 */
void Manager::push_ownership_at_init(
   char const *obj_instance_name,
   char const *attribute_list )
{
   if ( obj_instance_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::push_ownership_at_init():" << __LINE__
             << " ERROR: Unexpected NULL obj_instance_name specified!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   Object *obj = get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::push_ownership_at_init():" << __LINE__
             << " ERROR: Failed to find object with instance name: '"
             << obj_instance_name << "'!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   obj->push_ownership_at_init( attribute_list );
}

/*!
 * @brief Blocking function call to wait to handle the remote request to
 * Push ownership object attributes to this federate.
 * @param obj_instance_name Object instance name to handle the remote
 * pushed ownership attributes from.
 */
void Manager::handle_pushed_ownership_at_init(
   char const *obj_instance_name )
{
   if ( obj_instance_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::handle_pushed_ownership_at_init():" << __LINE__
             << " ERROR: Unexpected NULL obj_instance_name specified!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   Object *obj = get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::handle_pushed_ownership_at_init():" << __LINE__
             << " ERROR: Failed to find object with instance name: '"
             << obj_instance_name << "'!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   obj->handle_pushed_ownership_at_init();
}

/*!
 * @job_class{scheduled}
 */
void Manager::grant_pull_request()
{
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].grant_pull_request();
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::release_ownership()
{
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].release_ownership();
   }
}

/*!
 * @details If the federate does not exist, -1.0 seconds is assigned to the
 * returned object.
 */
Int64Interval Manager::get_lookahead() const
{
   return federate->get_lookahead();
}

/*!
 * @details If the federate does not exist, Int64BaseTime::get_max_logical_time_in_seconds()
 * is assigned to the returned object.
 */
Int64Time Manager::get_granted_time() const
{
   return federate->get_granted_time();
}

/*!
 * @details Returns the granted time in base time.
 */
int64_t const Manager::get_granted_base_time() const
{
   return federate->get_granted_base_time();
}

bool Manager::is_RTI_ready(
   char const *method_name )
{
   if ( this->federate == NULL ) {
      message_publish( MSG_WARNING, "Manager::%s:%d Unexpected NULL 'federate' pointer!\n",
                       method_name, __LINE__ );
      return false;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   bool rti_valid = true;
   if ( get_RTI_ambassador() == NULL ) {
      message_publish( MSG_WARNING, "Manager::%s:%d Unexpected NULL RTIambassador!\n",
                       method_name, __LINE__ );
      rti_valid = false;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return rti_valid;
}

/*!
 * @details Trigger federation save, at current time or user-specified time.\n
 * NOTE: These routines do not coordinate a federation save via interactions
 * so make these internal routines so that the user does not accidentally call
 * them and mess things up.
 */
void Manager::initiate_federation_save(
   char const *file_name )
{
   federate->set_checkpoint_file_name( file_name );
   federate->initiate_save_announce();

   this->initiated_a_federation_save = true;
}

void Manager::start_federation_save(
   char const *file_name )
{
   start_federation_save_at_scenario_time( -DBL_MAX, file_name );
}

void Manager::start_federation_save_at_sim_time(
   double      freeze_sim_time,
   char const *file_name )
{
   start_federation_save_at_scenario_time(
      execution_control->convert_sim_time_to_scenario_time( freeze_sim_time ),
      file_name );
}

void Manager::start_federation_save_at_scenario_time(
   double      freeze_scenario_time,
   char const *file_name )
{
   // Call the ExecutionControl method.
   execution_control->start_federation_save_at_scenario_time( freeze_scenario_time, file_name );
}

/*!
 * @job_class{initialization}
 */
void Manager::encode_checkpoint()
{
   // Call the ExecutionControl method.
   execution_control->encode_checkpoint();

   for ( int n = 0; n < obj_count; ++n ) {
      // Any object with a valid instance handle must be marked as required
      // to ensure the restore process will wait for this object instance
      // to exist.
      if ( objects[n].is_instance_handle_valid() ) {
         objects[n].mark_required();
      }
      // Setup the ownership handler checkpoint data structures.
      objects[n].encode_checkpoint();
   }

   encode_checkpoint_interactions();
}

void Manager::decode_checkpoint()
{
   // Restore the state of this class from the Trick checkpoint.

   // Call the ExecutionControl method.
   execution_control->decode_checkpoint();

   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].decode_checkpoint();
   }

   decode_checkpoint_interactions();
}

void Manager::free_checkpoint()
{
   // Clear/release the memory used for the checkpoint data structures.

   // Call the ExecutionControl method.
   execution_control->free_checkpoint();

   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].free_checkpoint();
   }

   free_checkpoint_interactions();
}

void Manager::encode_checkpoint_interactions()
{
   // Clear the checkpoint for the interactions so that we don't leak memory.
   free_checkpoint_interactions();

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &interactions_queue.mutex );

   if ( !interactions_queue.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::encode_checkpoint_interactions():%d interactions_queue.size()=%d\n",
                          __LINE__, interactions_queue.size() );
      }

      // Get the count to use for the check.
      check_interactions_count = interactions_queue.size();

      // Allocate the interaction items base don the count.
      check_interactions = reinterpret_cast< InteractionItem * >(
         alloc_type( check_interactions_count, "TrickHLA::InteractionItem" ) );
      if ( check_interactions == NULL ) {
         ostringstream errmsg;
         errmsg << "Manager::encode_checkpoint_interactions():" << __LINE__
                << " ERROR: Failed to allocate enough memory for check_interactions"
                << " linear array of " << check_interactions_count << " elements.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
         return;
      }

      // interactions_queue.dump_head_pointers("interactions_queue.dump");

      for ( int i = 0; i < interactions_queue.size(); ++i ) {

         InteractionItem *item = static_cast< InteractionItem * >( interactions_queue.front() );

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, "Manager::encode_checkpoint_interactions():%d \
Checkpointing into check_interactions[%d] from interaction index %d.\n",
                             __LINE__, i, item->index );
         }
         check_interactions[i].index            = item->index;
         check_interactions[i].interaction_type = item->interaction_type;
         item->checkpoint_queue();
         check_interactions[i].parm_items_count       = item->parm_items_count;
         check_interactions[i].parm_items             = item->parm_items;
         check_interactions[i].user_supplied_tag_size = item->user_supplied_tag_size;
         if ( item->user_supplied_tag_size == 0 ) {
            check_interactions[i].user_supplied_tag = NULL;
         } else {
            check_interactions[i].user_supplied_tag =
               static_cast< unsigned char * >( trick_MM->declare_var( "unsigned char", item->user_supplied_tag_size ) );

            memcpy( check_interactions[i].user_supplied_tag, item->user_supplied_tag, item->user_supplied_tag_size );
         }
         check_interactions[i].order_is_TSO = item->order_is_TSO;
         check_interactions[i].time         = item->time;

         // Now that we extracted the data from the parameter-item, point to the
         // next element in the queue, without popping!
         interactions_queue.next( item );
      }

      interactions_queue.rewind();
   }
   // auto_unlock_mutex unlocks the mutex lock here
}

void Manager::decode_checkpoint_interactions()
{
   if ( check_interactions_count > 0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::decode_checkpoint_interactions():%d check_interactions_count=%d\n",
                          __LINE__, check_interactions_count );
      }

      for ( int i = 0; i < check_interactions_count; ++i ) {

         InteractionItem *item = new InteractionItem();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, "Manager::decode_checkpoint_interactions():%d \
restoring check_interactions[%d] into interaction index %d, parm_count=%d\n",
                             __LINE__, i, check_interactions[i].index,
                             check_interactions[i].parm_items_count );
         }
         item->index            = check_interactions[i].index;
         item->interaction_type = check_interactions[i].interaction_type;
         item->parm_items_count = check_interactions[i].parm_items_count;
         item->parm_items       = check_interactions[i].parm_items;
         item->restore_queue();
         item->user_supplied_tag_size = check_interactions[i].user_supplied_tag_size;
         if ( check_interactions[i].user_supplied_tag_size == 0 ) {
            item->user_supplied_tag = NULL;
         } else {
            item->user_supplied_tag = reinterpret_cast< unsigned char * >(
               trick_MM->mm_strdup( reinterpret_cast< char const * >( check_interactions[i].user_supplied_tag ) ) );
         }
         item->order_is_TSO = check_interactions[i].order_is_TSO;
         item->time         = check_interactions[i].time;

         interactions_queue.push( item );
      }
   }
}

void Manager::free_checkpoint_interactions()
{
   if ( check_interactions_count > 0 ) {
      for ( int i = 0; i < check_interactions_count; ++i ) {
         check_interactions[i].clear_parm_items();
      }
      if ( trick_MM->delete_var( static_cast< void * >( check_interactions ) ) ) {
         message_publish( MSG_WARNING, "Manager::free_checkpoint_interactions():%d WARNING failed to delete Trick Memory for 'check_interactions'\n",
                          __LINE__ );
      }
      check_interactions       = NULL;
      check_interactions_count = 0;
   }
}

void Manager::print_checkpoint_interactions()
{
   if ( check_interactions_count > 0 ) {
      ostringstream msg;
      msg << "Manager::print_checkpoint_interactions():" << __LINE__
          << "check_interactions contains these "
          << check_interactions_count << " elements:\n";
      for ( int i = 0; i < check_interactions_count; ++i ) {
         msg << "check_interactions[" << i << "].index                  = "
             << check_interactions[i].index << '\n'
             << "check_interactions[" << i << "].interaction_type       = '"
             << check_interactions[i].interaction_type << "'\n"
             << "check_interactions[" << i << "].parm_items_count       = "
             << check_interactions[i].parm_items_count << '\n';
         for ( int k = 0; k < check_interactions[i].parm_items_count; ++k ) {
            msg << "check_interactions[" << i << "].parm_items[" << k << "].index    = "
                << check_interactions[i].parm_items[k].index << '\n'
                << "check_interactions[" << i << "].parm_items[" << k << "].size     = "
                << check_interactions[i].parm_items[k].size << '\n';
         }
         msg << "check_interactions[" << i << "].user_supplied_tag_size = "
             << check_interactions[i].user_supplied_tag_size << '\n'
             << "check_interactions[" << i << "].order_is_TSO           = "
             << check_interactions[i].order_is_TSO << '\n'
             << "check_interactions[" << i << "].time                   = "
             << check_interactions[i].time.get_base_time() << '\n';
      }
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*!
 * @details Calling this function will block until object instances have been
 * discovered.
 * @job_class{initialization}
 */
void Manager::wait_for_discovery_of_objects()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      message_publish( MSG_NORMAL, "Manager::wait_for_discovery_of_object_instance():%d\n",
                       __LINE__ );
   }

   // Do we have Simulation object(s) to interrogate?
   if ( obj_count > 0 ) {

      // See if any object discoveries have occurred.
      int  required_count                   = 0;
      int  discovery_count                  = 0;
      bool create_HLA_instance_object_found = false;
      for ( int n = 0; n < obj_count; ++n ) {
         if ( objects[n].is_required() ) {
            ++required_count;
         }
         if ( objects[n].is_instance_handle_valid() ) {
            ++discovery_count;
            if ( objects[n].is_create_HLA_instance() ) {
               create_HLA_instance_object_found = true;
            }
         }
      }

      // If all of the required objects were discovered, exit immediately.
      if ( discovery_count == required_count ) {
         return;
      }

      // Figure out how many objects have been discovered so far.
      if ( ( !create_HLA_instance_object_found && // still missing some objects other than
             ( discovery_count < ( required_count - 1 ) ) )
           ||                                           // the one for the rejoining federate, or
           ( create_HLA_instance_object_found &&        // missing some other object(s) but
             ( discovery_count < required_count ) ) ) { // found the rejoining federate

         if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            message_publish( MSG_NORMAL, "Manager::wait_for_discovery_of_object_instance():%d Waiting for object discovery callbacks to arrive.\n",
                             __LINE__ );
         }

         int64_t      wallclock_time; // cppcheck-suppress [variableScope,unmatchedSuppression]
         SleepTimeout print_timer( federate->wait_status_time );
         SleepTimeout sleep_timer;

         // Block until some or all objects arrive.
         do {

            // Check for shutdown.
            federate->check_for_shutdown_with_termination();

            // Sleep for a little while to allow the RTI to trigger the object
            // discovery callbacks.
            sleep_timer.sleep();

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Manager::wait_for_discovery_of_object_instance():" << __LINE__
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
               message_publish( MSG_NORMAL, "Manager::wait_for_discovery_of_object_instance():%d Waiting...\n",
                                __LINE__ );
            }

            // Check if any objects were discovered while we were sleeping.
            discovery_count                  = 0;
            create_HLA_instance_object_found = false;
            for ( int n = 0; n < obj_count; ++n ) {
               if ( objects[n].is_required() && objects[n].is_instance_handle_valid() ) {
                  ++discovery_count;
                  if ( objects[n].is_create_HLA_instance() ) {
                     create_HLA_instance_object_found = true;
                  }
               }
            }

         } while ( ( !create_HLA_instance_object_found && // still missing some objects other than
                     ( discovery_count < ( required_count - 1 ) ) )
                   ||                                    // the one for the rejoining federate, or
                   ( create_HLA_instance_object_found && // missing some other object(s) but
                     ( discovery_count < required_count ) ) ); // found the rejoining federate
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         message_publish( MSG_NORMAL, "Manager::wait_for_discovery_of_object_instance():%d - No Objects to discover.\n",
                          __LINE__ );
      }
   }
}

/*!
 * @details If they have, true is returned if the 'create HLA instance' object
 * was discovered. If no discoveries took place or if the required
 * 'create HLA instance' object was not discovered, false is returned.
 * @job_class{initialization}
 */
bool Manager::is_this_a_rejoining_federate()
{
   for ( int n = 0; n < obj_count; ++n ) {
      // Was the required 'create_HLA_instance' object found?
      if ( objects[n].is_required()
           && objects[n].is_create_HLA_instance()
           && objects[n].is_instance_handle_valid() ) {

         // Set a flag to indicate that this federate is rejoining the federation
         rejoining_federate = true;
         return true;
      }
   }

   // Set a flag to indicate that this federate is not rejoining the federation
   rejoining_federate = false;
   return false;
}

RTIambassador *Manager::get_RTI_ambassador()
{
   return ( ( federate != NULL ) ? federate->get_RTI_ambassador() : NULL );
}

bool Manager::is_shutdown_called() const
{
   return ( ( this->federate != NULL ) ? federate->is_shutdown_called() : false );
}
