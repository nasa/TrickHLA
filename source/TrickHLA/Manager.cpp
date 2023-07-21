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
#include <string>

// Trick include files.
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
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

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

#ifdef __cplusplus
extern "C" {
#endif
// C based model includes.

extern ATTRIBUTES attrTrickHLA__InteractionItem[];

extern ATTRIBUTES attrTrickHLA__Interaction[];

extern ATTRIBUTES attrSRFOM__MTRInteractionHandler[];

extern ATTRIBUTES attrTrickHLA__Parameter[];

#ifdef __cplusplus
}
#endif

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
     job_cycle_base_time( 0LL ),
     rejoining_federate( false ),
     restore_determined( false ),
     restore_federate( false ),
     mgr_initialized( false ),
     obj_discovery_mutex(),
     object_map(),
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
   clear_interactions();

   // Make sure we unlock the mutex.
   (void)obj_discovery_mutex.unlock();
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
         send_hs( stdout, "Manager::initialize():%d Already initialized.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::initialize():%d%c", __LINE__, THLA_NEWLINE );
   }

   // Check to make sure we have a reference to the TrickHLA::Manager.
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check to make sure we have a reference to the TrickHLA::ExecutionControlBase.
   if ( this->execution_control == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " ERROR: Unexpected NULL 'execution_control' pointer!"
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check for the error condition of a valid object count but a null
   // objects array.
   if ( ( obj_count > 0 ) && ( objects == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " ERROR: Unexpected NULL 'objects' array for a non zero"
             << " obj_count:" << obj_count << ". Please check your input or"
             << " modified-data files to make sure the 'Manager::objects'"
             << " array is correctly configured." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have a non-NULL objects array but the object-count is invalid
   // then let the user know.
   if ( ( obj_count <= 0 ) && ( objects != NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " ERROR: Unexpected " << ( ( obj_count == 0 ) ? "zero" : "negative" )
             << " obj_count:" << obj_count << " for a non-NULL 'objects' array."
             << " Please check your input or modified-data files to make sure"
             << " the 'Manager::objects' array is correctly configured."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Object count if negative.
   if ( obj_count < 0 ) {
      obj_count = 0;
   }

   // Check for the error condition of a valid interaction count but a null
   // interactions array.
   if ( ( inter_count > 0 ) && ( interactions == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " ERROR: Unexpected NULL 'interactions' array for a non zero"
             << " inter_count:" << inter_count << ". Please check your input or"
             << " modified-data files to make sure the 'Manager::interactions'"
             << " array is correctly configured." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have a non-NULL interactions array but the interactions-count is
   // invalid then let the user know.
   if ( ( inter_count <= 0 ) && ( interactions != NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " ERROR: Unexpected " << ( ( inter_count == 0 ) ? "zero" : "negative" )
             << " inter_count:" << inter_count << " for a non-NULL 'interactions'"
             << " array. Please check your input or modified-data files to make"
             << " sure the 'Manager::interactions' array is correctly configured."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Interaction count if negative.
   if ( inter_count < 0 ) {
      inter_count = 0;
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
         send_hs( stdout, "Manager::restart_initialization():%d Manager Not initialized, returning.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::restart_initialization():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::restart_initialization():%d%c", __LINE__, THLA_NEWLINE );
   }

   // Make sure the Federate is initialized after the restart.
   federate->restart_initialization();

   // To allow manager initialization to complete we need to reset the init flag.
   this->mgr_initialized = false;

   // Check for the error condition of a valid object count but a null
   // objects array.
   if ( ( obj_count > 0 ) && ( objects == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::restart_initialization():" << __LINE__
             << " ERROR: Unexpected NULL 'objects' array for a non zero"
             << " obj_count:" << obj_count << ". Please check your input or"
             << " modified-data files to make sure the 'Manager::objects'"
             << " array is correctly configured." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have a non-NULL objects array but the object-count is invalid
   // then let the user know.
   if ( ( obj_count <= 0 ) && ( objects != NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::restart_initialization():" << __LINE__
             << " ERROR: Unexpected " << ( ( obj_count == 0 ) ? "zero" : "negative" )
             << " obj_count:" << obj_count << " for a non-NULL 'objects' array."
             << " Please check your input or modified-data files to make sure"
             << " the 'Manager::objects' array is correctly configured."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Object count if negative.
   if ( obj_count < 0 ) {
      obj_count = 0;
   }

   // Check for the error condition of a valid interaction count but a null
   // interactions array.
   if ( ( inter_count > 0 ) && ( interactions == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::restart_initialization():" << __LINE__
             << " ERROR: Unexpected NULL 'interactions' array for a non zero"
             << " inter_count:" << inter_count << ". Please check your input or"
             << " modified-data files to make sure the 'Manager::interactions'"
             << " array is correctly configured." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have a non-NULL interactions array but the interactions-count is
   // invalid then let the user know.
   if ( ( inter_count <= 0 ) && ( interactions != NULL ) ) {
      ostringstream errmsg;
      errmsg << "Manager::restart_initialization():" << __LINE__
             << " ERROR: Unexpected " << ( ( inter_count == 0 ) ? "zero" : "negative" )
             << " inter_count:" << inter_count << " for a non-NULL 'interactions'"
             << " array. Please check your input or modified-data files to make"
             << " sure the 'Manager::interactions' array is correctly configured."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Interaction count if negative.
   if ( inter_count < 0 ) {
      inter_count = 0;
   }

   // Setup the Execution Control and Execution Configuration objects now that
   // we know if we are the "Master" federate or not.
   if ( execution_control == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::restart_initialization():" << __LINE__
             << " ERROR: Unexpected NULL 'execution_control' pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // The set_master() function set's additional parameter so call it again to
   // force the a complete master state.
   bool master_flag = this->execution_control->is_master(); // cppcheck-suppress [nullPointerRedundantCheck,unmatchedSuppression]
   this->execution_control->set_master( master_flag );      // cppcheck-suppress [nullPointerRedundantCheck,unmatchedSuppression]

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
   if ( this->execution_control->is_master() ) {

      // Make sure all the federate instance handles are reset based on
      // the federate name so that the wait for required federates will work
      // after a checkpoint reload.
      federate->set_all_federate_MOM_instance_handles_by_name();

      // Make sure all required federates have joined the federation.
      (void)federate->wait_for_required_federates_to_join();
   }

   // Restore ownership_transfer data for all objects.
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      objects[n].restore_ownership_transfer_checkpointed_data();
   }

   // Restore checkpointed interactions.
   restore_interactions();

   // The manager is now initialized.
   this->mgr_initialized = true;
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
         send_hs( stdout, "Manager::send_init_data():%d Late joining \
federate so this call will be ignored.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   // Go through the list of objects.
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      // Make sure we have at least one piece of object init data we can send.
      if ( objects[n].any_locally_owned_published_init_attribute() ) {

         if ( this->execution_control->wait_for_init_data() ) {

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::send_init_data():%d '%s'%c",
                        __LINE__, objects[n].get_name(), THLA_NEWLINE );
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
                   << this->execution_control->get_type()
                   << "') does not support it." << THLA_ENDL;
               send_hs( stdout, (char *)msg.str().c_str() );
            }
         }
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::send_init_data():%d Nothing to send for '%s'%c",
                     __LINE__, objects[n].get_name(), THLA_NEWLINE );
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
         send_hs( stdout, "Manager::send_init_data():%d Late joining \
federate so the data will not be sent for '%s'.%c",
                  __LINE__, instance_name,
                  THLA_NEWLINE );
      }
      return;
   }

   if ( instance_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::send_init_data():" << __LINE__
             << " ERROR: Null Object Instance Name" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
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
             << " module to verify the settings." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      // Make sure we have at least one piece of object init data we can send.
      if ( obj->any_locally_owned_published_init_attribute() ) {

         if ( this->execution_control->wait_for_init_data() ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::send_init_data():%d '%s'%c",
                        __LINE__, instance_name, THLA_NEWLINE );
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
                   << this->execution_control->get_type()
                   << "') does not support it." << THLA_ENDL;
               send_hs( stdout, (char *)msg.str().c_str() );
            }
         }
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::send_init_data():%d Nothing to send for '%s'%c",
                     __LINE__, instance_name, THLA_NEWLINE );
         }
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
         send_hs( stdout, "Manager::receive_init_data():%d Late joining \
federate so this call will be ignored.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   // Go through the list of objects.
   for ( unsigned int n = 0; n < obj_count; ++n ) {

      // Make sure we have at least one piece of data we can receive.
      if ( objects[n].any_remotely_owned_subscribed_init_attribute() ) {

         // Only wait for REQUIRED received init data and do not block waiting
         // to receive init data if we are using the simple init scheme.
         bool obj_required = objects[n].is_required() && ( this->execution_control->wait_for_init_data() );

         if ( obj_required ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.%c",
                        __LINE__, objects[n].get_name(),
                        ( objects[n].is_required() ? "REQUIRED" : "not required" ), THLA_NEWLINE );
            }

            int64_t      wallclock_time;
            SleepTimeout print_timer( federate->wait_status_time );
            SleepTimeout sleep_timer;

            // Wait for the data to arrive.
            while ( !objects[n].is_changed() ) {

               // Check for shutdown.
               federate->check_for_shutdown_with_termination();

               (void)sleep_timer.sleep();

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
                               << " the Central RTI Component (CRC) level!"
                               << THLA_ENDL;
                        DebugHandler::terminate_with_message( errmsg.str() );
                     }
                  }

                  if ( print_timer.timeout( wallclock_time ) ) {
                     print_timer.reset();
                     send_hs( stdout, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.%c",
                              __LINE__, objects[n].get_name(),
                              ( objects[n].is_required() ? "REQUIRED" : "not required" ), THLA_NEWLINE );
                  }
               }
            }
         }

         // Check for changed data which means we received something.
         if ( objects[n].is_changed() ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Received '%s'%c",
                        __LINE__, objects[n].get_name(), THLA_NEWLINE );
            }

            // Receive the data from the publishing federate.
            objects[n].receive_init_data();
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Received nothing for '%s', and marked as %s.%c",
                        __LINE__, objects[n].get_name(),
                        ( obj_required ? "REQUIRED" : "not required" ), THLA_NEWLINE );
            }
         }
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::receive_init_data():%d Nothing to receive for '%s'%c",
                     __LINE__, objects[n].get_name(), THLA_NEWLINE );
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
         send_hs( stdout, "Manager::receive_init_data():%d Late joining federate so skipping data for '%s'%c",
                  __LINE__, instance_name, THLA_NEWLINE );
      }
      return;
   }

   if ( instance_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::receive_init_data():" << __LINE__
             << " ERROR: Null Object Instance Name";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   wstring obj_instance_name;
   StringUtilities::to_wstring( obj_instance_name, instance_name );

   Object *obj = get_trickhla_object( obj_instance_name );

   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::receive_init_data():" << __LINE__
             << " ERROR: The specified Object Instance Name '" << instance_name
             << "' does not correspond to any known object. Please check your"
             << " S_define file or simulation module to verify the settings."
             << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } else {

      // Make sure we have at least one piece of data we can receive.
      if ( obj->any_remotely_owned_subscribed_init_attribute() ) {

         // Only wait for REQUIRED received init data and do not block waiting
         // to receive init data if we are using the simple init scheme.
         bool obj_required = obj->is_required() && this->execution_control->wait_for_init_data();

         if ( obj_required ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.%c",
                        __LINE__, instance_name,
                        ( obj->is_required() ? "REQUIRED" : "not required" ), THLA_NEWLINE );
            }

            int64_t      wallclock_time;
            SleepTimeout print_timer( federate->wait_status_time );
            SleepTimeout sleep_timer;

            // Wait for the data to arrive.
            while ( !obj->is_changed() ) {

               // Check for shutdown.
               federate->check_for_shutdown_with_termination();

               (void)sleep_timer.sleep();

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
                               << " the Central RTI Component (CRC) level!"
                               << THLA_ENDL;
                        DebugHandler::terminate_with_message( errmsg.str() );
                     }
                  }

                  if ( print_timer.timeout( wallclock_time ) ) {
                     print_timer.reset();
                     send_hs( stdout, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.%c",
                              __LINE__, instance_name,
                              ( obj->is_required() ? "REQUIRED" : "not required" ), THLA_NEWLINE );
                  }
               }
            }
         }

         // Check for changed data which means we received something.
         if ( obj->is_changed() ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout,
                        "Manager::receive_init_data():%d Received '%s'%c",
                        __LINE__, instance_name, THLA_NEWLINE );
            }

            // Receive the data from the publishing federate.
            obj->receive_init_data();
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Received nothing for '%s', and marked as %s.%c",
                        __LINE__, instance_name,
                        ( obj_required ? "REQUIRED" : "not required" ), THLA_NEWLINE );
            }
         }
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::receive_init_data():%d Nothing to receive for '%s'%c",
                     __LINE__, instance_name, THLA_NEWLINE );
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::clear_init_sync_points()
{
   // Clear the multiphase initalization synchronization points associated
   // with ExecutionControl initialization.
   this->execution_control->clear_multiphase_init_sync_points();
}

/*!
 * @job_class{initialization}
 */
void Manager::wait_for_init_sync_point(
   char const *sync_point_label )
{
   if ( !this->execution_control->wait_for_init_sync_point() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         ostringstream errmsg;
         errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
                << " This call will be ignored because the Simulation"
                << " Initialization Scheme (Type:'"
                << this->execution_control->get_type()
                << "') does not support it." << THLA_ENDL;
         send_hs( stdout, (char *)errmsg.str().c_str() );
      }
      return;
   }

   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::wait_for_init_sync_point():%d Late \
joining federate so this call will be ignored.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   if ( sync_point_label == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
             << " ERROR: Null Sync-Point Label" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   wstring ws_syc_point_label;
   StringUtilities::to_wstring( ws_syc_point_label, sync_point_label );

   // Determine if the sync-point label is valid.
   if ( this->execution_control->contains( ws_syc_point_label ) ) {

      // Achieve the specified sync-point and wait for the federation
      // to be synchronized on it.
      this->execution_control->achieve_and_wait_for_synchronization( *( this->get_RTI_ambassador() ),
                                                                     this->federate,
                                                                     ws_syc_point_label );

   } else {
      ostringstream errmsg;
      errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
             << " ERROR: The specified Initialization Synchronization-Point label '"
             << sync_point_label
             << "' is not known. Please check your input or modified data files"
             << " to make sure the 'THLA.federate.multiphase_init_sync_points'"
             << " correctly specifies all the synchronization-point labels that"
             << " will be used for multi-phase initialization." << THLA_ENDL;
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
      send_hs( stdout, "Manager::request_data_update():%d Object:'%ls'%c",
               __LINE__, instance_name.c_str(), THLA_NEWLINE );
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
   if ( !this->execution_control->object_instance_name_reservation_succeeded( obj_instance_name ) ) {

      Object *trickhla_obj = get_trickhla_object( obj_instance_name );
      if ( trickhla_obj != NULL ) {
         trickhla_obj->set_name_registered();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::object_instance_name_reservation_succeeded():%d \
RESERVED Object Instance Name '%s'%c",
                     __LINE__, trickhla_obj->get_name(),
                     THLA_NEWLINE );
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
   if ( this->execution_control->object_instance_name_reservation_failed( obj_instance_name ) ) {
      return;
   }

   // Anything beyond this point is fatal.
   send_hs( stderr, "Manager::object_instance_name_reservation_failed():%d \
Name:'%ls' Please check your input or modified data files to make sure the \
object instance name is unique, no duplicates, within the Federation. For \
example, try using fed_name.object_FOM_name for the object instance name. \
Also, an object should be owned by only one Federate so one common mistake is \
to have the 'create_HLA_instance' flag for the same object being set to true \
for more than one Federate.%c",
            __LINE__, obj_instance_name.c_str(), THLA_NEWLINE );

   wstring obj_name;
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      StringUtilities::to_wstring( obj_name, objects[n].get_name() );
      if ( obj_name == obj_instance_name ) {
         if ( objects[n].is_create_HLA_instance() ) {
            send_hs( stderr, "Manager::object_instance_name_reservation_failed():%d\
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
which keeps the instance attribute's object from becoming a Federation orphan. **%c",
                     __LINE__, THLA_NEWLINE );
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
          << " Exiting..." << THLA_ENDL;
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
        && ( this->object_map.find( object->get_instance_handle() ) == this->object_map.end() ) ) {
      this->object_map[object->get_instance_handle()] = object;
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_all_ref_attributes()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_all_ref_attributes():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Make sure the object-map is empty/clear before we continue.
   object_map.clear();

   if ( is_execution_configuration_used() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::setup_all_ref_attributes():%d Execution-Configuration %c",
                  __LINE__, THLA_NEWLINE );
      }
      setup_object_ref_attributes( 1, get_execution_configuration() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_all_ref_attributes():%d Objects: %d %c",
               __LINE__, obj_count, THLA_NEWLINE );
   }
   setup_object_ref_attributes( obj_count, objects );

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_all_ref_attributes():%d Interactions %c",
               __LINE__, THLA_NEWLINE );
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
      send_hs( stdout, "Manager::setup_object_ref_attributes():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_object_ref_attributes():%d %c",
               __LINE__, THLA_NEWLINE );
   }

   // Resolve all the Ref-Attributes for all the simulation initialization
   // objects and attributes.
   for ( unsigned int n = 0; n < data_obj_count; ++n ) {

      // Initialize the TrickHLA-Object before we use it.
      data_objects[n].initialize( this );

      ostringstream msg;

      int const  attr_count = data_objects[n].get_attribute_count();
      Attribute *attrs      = data_objects[n].get_attributes();

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         msg << "Manager::setup_object_ref_attributes()" << __LINE__ << endl
             << "--------------- Trick REF-Attributes ---------------" << endl
             << " Object:'" << data_objects[n].get_name() << "'"
             << " FOM-Name:'" << data_objects[n].get_FOM_name() << "'"
             << " Create HLA Instance:"
             << ( data_objects[n].is_create_HLA_instance() ? "Yes" : "No" )
             << " Attribute count:" << attr_count << endl;
      }

      // Process the attributes for this object.
      for ( unsigned int i = 0; i < attr_count; ++i ) {

         // Initialize the TrickHLA-Attribute before we use it.
         attrs[i].initialize( data_objects[n].get_FOM_name(), n, i );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attrs[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attrs[i].get_trick_name() << "'"
                << endl;
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, (char *)msg.str().c_str() );
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
      send_hs( stdout, "Manager::setup_interaction_ref_attributes():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_interaction_ref_attributes():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Interactions.
   for ( int n = 0; n < inter_count; ++n ) {
      ostringstream msg;

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         msg << "Manager::setup_interaction_ref_attributes():" << __LINE__ << endl
             << "--------------- Trick REF-Attributes ---------------" << endl
             << " FOM-Interaction:'" << interactions[n].get_FOM_name() << "'"
             << endl;
      }

      // Initialize the TrickHLA Interaction before we use it.
      interactions[n].initialize( this );

      int const  param_count = interactions[n].get_parameter_count();
      Parameter *params      = interactions[n].get_parameters();

      // Process the attributes for this object.
      for ( unsigned int i = 0; i < param_count; ++i ) {

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "   " << ( i + 1 ) << "/" << param_count
                << " FOM-Parameter:'" << params[i].get_FOM_name() << "'"
                << " Trick-Name:'" << params[i].get_trick_name() << "'"
                << endl;
         }

         // Initialize the TrickHLA Parameter.
         params[i].initialize( interactions[n].get_FOM_name(), n, i );
      }

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, (char *)msg.str().c_str() );
      }
   }

   // Tell the ExecutionControl object to setup the appropriate Trick Ref
   // ATTRIBUTES associated with the execution control mechanism.
   this->execution_control->setup_interaction_ref_attributes();
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_all_RTI_handles()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_all_RTI_handles():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Set up the object RTI handles for the ExecutionControl mechanisms.
   this->execution_control->setup_object_RTI_handles();

   // Set up the object RTI handles for the simulation data objects.
   setup_object_RTI_handles( obj_count, objects );

   // Set up the object RTI handles for the ExecutionControl mechanisms.
   this->execution_control->setup_interaction_RTI_handles();

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
      send_hs( stdout, "Manager::setup_object_RTI_handles():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_object_RTI_handles():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   char const *obj_FOM_name  = "";
   char const *attr_FOM_name = "";
   int         FOM_name_type = 0; // 0:N/A 1:Object 2:Attribute - What name are we dealing with.

   // Initialize the Object and Attribute RTI handles.
   try {
      wstring ws_FOM_name = L"";

      // Resolve all the handles/ID's for the objects and attributes.
      for ( unsigned int n = 0; n < data_obj_count; ++n ) {
         ostringstream msg;

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "Manager::setup_object_RTI_handles()" << __LINE__ << endl
                << "----------------- RTI Handles (Objects & Attributes) ---------------"
                << endl
                << "Getting RTI Object-Class-Handle for"
                << " Object:'" << data_objects[n].get_name() << "'"
                << " FOM-Name:'" << data_objects[n].get_FOM_name() << "'"
                << endl;
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
                << " Class-ID:" << handle_str << endl;
         }

         int const  attr_count = data_objects[n].get_attribute_count();
         Attribute *attrs      = data_objects[n].get_attributes();

         // Resolve the handles/ID's for the attributes.
         for ( unsigned int i = 0; i < attr_count; ++i ) {

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               msg << "\tGetting RTI Attribute-Handle for '"
                   << data_objects[n].get_FOM_name() << "'->'"
                   << attrs[i].get_FOM_name() << "'" << endl;
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
               StringUtilities::to_string( id_str,
                                           attrs[i].get_attribute_handle() );
               msg << "\t  Result for Attribute '"
                   << data_objects[n].get_FOM_name() << "'->'"
                   << attrs[i].get_FOM_name() << "'"
                   << " Attribute-ID:" << id_str << endl;
            }
         }

         // Make sure we build the attribute map now that the AttributeHandles
         // have been set.
         data_objects[n].build_attribute_map();

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, (char *)msg.str().c_str() );
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
                   << " Object FOM Name is correctly specified." << THLA_ENDL;
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         case 2: { // Attribute
            ostringstream errmsg;
            errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
                   << " ERROR: For Object FOM Name '" << obj_FOM_name << "', Attribute FOM Name '"
                   << attr_FOM_name << "' Not Found. Please check your input or"
                   << " modified-data files to make sure the Object Attribute"
                   << " FOM Name is correctly specified." << THLA_ENDL;
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         default: { // FOM name we are working with is unknown.
            ostringstream errmsg;
            errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
                   << " ERROR: Object or Attribute FOM Name Not Found. Please check your input or"
                   << " modified-data files to make sure the FOM Name is"
                   << " correctly specified." << THLA_ENDL;
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
             << " ERROR: Federate Not Execution Member" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Not Connected" << THLA_ENDL;
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
             << rti_err_msg << "'" << THLA_ENDL;
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
             << rti_err_msg << "'" << THLA_ENDL;
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
      send_hs( stdout, "Manager::setup_interaction_RTI_handles():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_interaction_RTI_handles():%d%c",
               __LINE__, THLA_NEWLINE );
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
            msg << "Manager::setup_interaction_RTI_handles()" << __LINE__ << endl
                << "----------------- RTI Handles (Interactions & Parameters) ---------------" << endl
                << "Getting RTI Interaction-Class-Handle for"
                << " FOM-Name:'" << inter_FOM_name << "'"
                << endl;
         }

         // Get the Interaction class handle.
         in_interactions[n].set_class_handle( rti_amb->getInteractionClassHandle( ws_FOM_name ) );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            string handle_str;
            StringUtilities::to_string( handle_str,
                                        in_interactions[n].get_class_handle() );
            msg << "  Result for Interaction"
                << " FOM-Name:'" << inter_FOM_name << "'"
                << " Interaction-ID:" << handle_str << endl;
         }

         // The parameters.
         int const  param_count = in_interactions[n].get_parameter_count();
         Parameter *params      = in_interactions[n].get_parameters();

         // Process the parameters for the interaction.
         for ( unsigned int i = 0; i < param_count; ++i ) {

            // The Parameter FOM name.
            FOM_name_type  = 2; // Parameter
            param_FOM_name = params[i].get_FOM_name();
            StringUtilities::to_wstring( ws_FOM_name, param_FOM_name );

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               msg << "\tGetting RTI Parameter-Handle for '"
                   << inter_FOM_name << "'->'" << param_FOM_name << "'" << endl;
            }

            // Get the Parameter Handle.
            params[i].set_parameter_handle(
               rti_amb->getParameterHandle(
                  in_interactions[n].get_class_handle(),
                  ws_FOM_name ) );

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               string handle_str;
               StringUtilities::to_string( handle_str,
                                           params[i].get_parameter_handle() );
               msg << "\t  Result for Parameter '"
                   << inter_FOM_name << "'->'" << param_FOM_name << "'"
                   << " Parameter-ID:" << handle_str << endl;
            }
         }

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, (char *)msg.str().c_str() );
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
                   << " Interaction FOM Name is correctly specified." << THLA_ENDL;
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
                   << " correctly specified." << THLA_ENDL;
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
         default: { // FOM name we are working with is unknown.
            ostringstream errmsg;
            errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
                   << " ERROR: Interaction or Parameter FOM Name Not Found. Please check your input"
                   << " or modified-data files to make sure the FOM Name is"
                   << " correctly specified." << THLA_ENDL;
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
             << " ERROR: FederateNotExecutionMember!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: NotConnected!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: RTIinternalError: '" << rti_err_msg << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " ERROR: Exception for '" << rti_err_msg << "'" << THLA_ENDL;
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
   unsigned int n;

   if ( !is_RTI_ready( "publish" ) ) {
      return;
   }

   // Publish attributes for all the Trick-HLA-Objects we know about.
   for ( n = 0; n < obj_count; ++n ) {
      objects[n].publish_object_attributes();
   }

   // Publish the interactions.
   for ( n = 0; n < inter_count; ++n ) {
      interactions[n].publish_interaction();
   }

   // Publish Execution Control objects and interactions.
   this->execution_control->publish();
}

/*!
 * @job_class{initialization}
 */
void Manager::unpublish()
{
   unsigned int i, k;
   bool         do_unpublish;

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
   this->execution_control->unpublish();
}

/*!
 * @job_class{initialization}
 */
void Manager::subscribe()
{
   unsigned int n;

   if ( !is_RTI_ready( "subscribe" ) ) {
      return;
   }

   // Subscribe to attributes for all the Trick-HLA-Objects we know about.
   for ( n = 0; n < obj_count; ++n ) {
      objects[n].subscribe_to_object_attributes();
   }

   // Subscribe to the interactions.
   for ( n = 0; n < inter_count; ++n ) {
      interactions[n].subscribe_to_interaction();
   }

   // Subscribe to anything needed for the execution control mechanisms.
   this->execution_control->subscribe();
}

/*!
 * @job_class{initialization}
 */
void Manager::unsubscribe()
{
   unsigned int i, k;
   bool         do_unsubscribe;

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
   this->execution_control->unsubscribe();
}

/*!
 * @job_class{initialization}
 */
void Manager::publish_and_subscribe()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::publish_and_subscribe():%d%c",
               __LINE__, THLA_NEWLINE );
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
      send_hs( stdout, "Manager::reserve_object_names_with_RTI():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // For the locally owned objects, reserve the object instance name with
   // the RTI.
   for ( unsigned int n = 0; n < obj_count; ++n ) {
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
      send_hs( stdout, "Manager::wait_for_reservation_of_object_names():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Simulation object names.
   if ( obj_count > 0 ) {
      // Wait for each RTI object instance name to be registered with the RTI,
      // but for only the names we requested registration for.
      for ( unsigned int n = 0; n < obj_count; ++n ) {
         objects[n].wait_for_object_name_reservation();
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::wait_for_reservation_of_object_names():%d All Object instance names reserved.%c",
                  __LINE__, THLA_NEWLINE );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::wait_for_reservation_of_object_names():%d No Object instance names to reserve.%c",
                  __LINE__, THLA_NEWLINE );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::register_objects_with_RTI()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::register_objects_with_RTI():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Have the ExecutionControl register objects it needs with the RTI.
   this->execution_control->register_objects_with_RTI();

   // For the locally owned objects register it with the RTI to get its
   // RTI object instance ID.
   for ( unsigned int n = 0; n < obj_count; ++n ) {
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
      send_hs( stdout, "Manager::setup_preferred_order_with_RTI():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   if ( is_execution_configuration_used() ) {
      // Register the execution configuration object.
      get_execution_configuration()->setup_preferred_order_with_RTI();
   }

   // Setup the preferred order for all the object attributes.
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      objects[n].setup_preferred_order_with_RTI();
   }

   // Setup the preferred order for all the interactions.
   for ( unsigned int i = 0; i < inter_count; ++i ) {
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
      send_hs( stdout, "Manager::wait_for_registration_of_required_objects():%d%c",
               __LINE__, THLA_NEWLINE );
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
   for ( unsigned int n = 0; n < obj_count; ++n ) {
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
            for ( unsigned int n = 0; n < obj_count; ++n ) {
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

            for ( unsigned int n = 0; n < obj_count; ++n ) {
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
         summary << THLA_ENDL;

         // Display the summary.
         send_hs( stdout, (char *)summary.str().c_str() );

         // Reset the flags for printing a summary.
         print_summary               = false;
         print_only_unregistered_obj = false;
      }

      // Determine if we have any unregistered objects.
      any_unregistered_required_obj = ( current_required_obj_cnt < total_required_obj_cnt );

      // Wait a little while to allow the objects to be registered.
      if ( any_unregistered_required_obj ) {
         (void)sleep_timer.sleep();

         // Check again to see if we have any unregistered objects.
         any_unregistered_required_obj = ( current_required_obj_cnt < total_required_obj_cnt );

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
                         << " at the Central RTI Component (CRC) level!"
                         << THLA_ENDL;
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
      for ( unsigned int n = 0; n < obj_count; ++n ) {
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
      send_hs( stdout, "Manager::set_all_object_instance_handles_by_name():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
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
      send_hs( stdout, "Manager::set_object_instance_handles_by_name():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador!" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   ostringstream summary;
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      summary << "Manager::set_object_instance_handles_by_name():"
              << __LINE__;
   }

   try {
      wstring ws_instance_name = L"";

      // Resolve all the handles/ID's for the objects and attributes.
      for ( unsigned int n = 0; n < data_obj_count; ++n ) {

         // Create the wide-string version of the instance name.
         char *instance_name = (char *)data_objects[n].get_name();
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
                      << ( instance_name != NULL ? instance_name : "" ) << "'" << THLA_ENDL;
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
             << " ERROR: Federation Not Execution Member" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Not Connected" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: RTIinternalError: '" << rti_err_msg << "'" << THLA_ENDL;
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
             << rti_err_msg << "'" << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      summary << THLA_ENDL;
      send_hs( stdout, (char *)summary.str().c_str() );
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
      this->execution_control->provide_attribute_update( theObject, theAttributes );
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::determine_job_cycle_time()
{
   if ( this->job_cycle_base_time > 0LL ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::determine_job_cycle_time():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Get the lookahead time.
   int64_t const lookahead_time_base_time = federate->get_lookahead_in_base_time();

   // Get the cycle time.
   double const cycle_time   = exec_get_job_cycle( NULL );
   this->job_cycle_base_time = Int64BaseTime::to_base_time( cycle_time );

   // Verify the job cycle time against the HLA lookahead time.
   if ( ( this->job_cycle_base_time <= 0LL ) || ( this->job_cycle_base_time < lookahead_time_base_time ) ) {
      ostringstream errmsg;
      errmsg << "Manager::determine_job_cycle_time():" << __LINE__
             << " ERROR: The cycle time for this job is less than the HLA"
             << " lookahead time! The HLA Lookahead time ("
             << Int64BaseTime::to_seconds( lookahead_time_base_time )
             << " seconds) must be less than or equal to the job cycle time ("
             << cycle_time << " seconds). Make sure 'lookahead_time' in"
             << " your input or modified-data file is less than or equal to the"
             << " 'THLA_DATA_CYCLE_TIME' time specified in the S_define file for"
             << " the send_cyclic_and_requested_data() and"
             << " receive_cyclic_data() jobs." << THLA_ENDL;
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Set the core job cycle time now that we know what it is so that the
   // attribute cyclic ratios can now be calculated for any multi-rate
   // attributes.
   for ( unsigned int n = 0; n < this->obj_count; ++n ) {
      objects[n].set_core_job_cycle_time( cycle_time );
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::send_cyclic_and_requested_data()
{
   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::send_cyclic_and_requested_data():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   int64_t const sim_time_in_base_time = Int64BaseTime::to_base_time( exec_get_sim_time() );
   int64_t const granted_base_time     = get_granted_base_time();
   bool const    zero_lookahead        = federate->is_zero_lookahead_time();

   // Initial time values.
   int64_t   dt      = zero_lookahead ? 0LL : federate->get_lookahead_in_base_time();
   int64_t   prev_dt = dt;
   Int64Time granted_plus_lookahead( granted_base_time + dt );
   Int64Time update_time( granted_plus_lookahead );

   // Determine the main thread cycle time for this job if it is not yet known.
   if ( this->job_cycle_base_time <= 0LL ) {
      determine_job_cycle_time();
   }

   // Only update the time if time management is enabled.
   if ( federate->is_time_management_enabled() ) {

      // Check for a zero lookahead time, which means the cycle_time (i.e. dt)
      // should be zero as well.
      dt = zero_lookahead ? 0LL : this->job_cycle_base_time;

      // Reuse the update_time if the data cycle time (dt) is the same.
      if ( dt != prev_dt ) {
         prev_dt = dt;

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
         update_time.set( granted_base_time + dt );

         // Make sure the update time is not less than the granted time + lookahead.
         if ( update_time < granted_plus_lookahead ) {
            update_time.set( granted_plus_lookahead );
         }
      }
   }

   // Send any ExecutionControl data requested.
   this->execution_control->send_requested_data( update_time );

   // Send data to remote RTI federates for each of the objects.
   for ( unsigned int obj_index = 0; obj_index < this->obj_count; ++obj_index ) {

      // Only send data if we are on the data cycle time boundary for this object.
      if ( this->federate->on_data_cycle_boundary_for_obj( obj_index, sim_time_in_base_time ) ) {

         // Only update the time if time management is enabled.
         if ( federate->is_time_management_enabled() ) {

            // Check for a zero lookahead time, which means the cycle_time
            // (i.e. dt) should be zero as well.
            dt = zero_lookahead ? 0LL
                                : this->federate->get_data_cycle_base_time_for_obj(
                                   obj_index, this->job_cycle_base_time );

            // Reuse the update_time if the data cycle time (dt) is the same.
            if ( dt != prev_dt ) {
               prev_dt = dt;

               update_time.set( granted_base_time + dt );

               // Make sure the update time is not less than the granted time + lookahead.
               if ( update_time < granted_plus_lookahead ) {
                  update_time.set( granted_plus_lookahead );
               }
            }
         }

         // Send the data for the object.
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
      send_hs( stdout, "Manager::receive_cyclic_data():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   int64_t const sim_time_in_base_time = Int64BaseTime::to_base_time( exec_get_sim_time() );

   // Receive and process any updates for ExecutionControl.
   this->execution_control->receive_cyclic_data();

   // Receive data from remote RTI federates for each of the objects.
   for ( unsigned int n = 0; n < obj_count; ++n ) {

      // Only receive data if we are on the data cycle time boundary for this object.
      if ( this->federate->on_data_cycle_boundary_for_obj( n, sim_time_in_base_time ) ) {
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
   this->execution_control->process_mode_interaction();

   // Just return if the interaction queue is empty.
   if ( interactions_queue.empty() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::process_interactions():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Process all the interactions in the queue.
   while ( !interactions_queue.empty() ) {

      // Get a reference to the first item on the queue.
      InteractionItem *interaction_item =
         static_cast< InteractionItem * >( interactions_queue.front() );

      switch ( interaction_item->interaction_type ) {
         case TRICKHLA_MANAGER_USER_DEFINED_INTERACTION: {
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
                   << "found in 'Manager.hh' and re-run." << THLA_ENDL;
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
      }

      // Now that we processed the interaction-item remove it from the queue,
      // which will result in the item being deleted and no longer valid.
      interactions_queue.pop();
   }

   clear_interactions();
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
   // Find the Interaction we have data for.
   for ( unsigned int i = 0; i < inter_count; ++i ) {

      // Process the interaction if we subscribed to it and we have the same class handle.
      if ( interactions[i].is_subscribe()
           && ( interactions[i].get_class_handle() == theInteraction ) ) {

         InteractionItem *item;
         if ( received_as_TSO ) {
            item = new InteractionItem( i,
                                        TRICKHLA_MANAGER_USER_DEFINED_INTERACTION,
                                        interactions[i].get_parameter_count(),
                                        interactions[i].get_parameters(),
                                        theParameterValues,
                                        theUserSuppliedTag,
                                        theTime );
         } else {
            item = new InteractionItem( i,
                                        TRICKHLA_MANAGER_USER_DEFINED_INTERACTION,
                                        interactions[i].get_parameter_count(),
                                        interactions[i].get_parameters(),
                                        theParameterValues,
                                        theUserSuppliedTag );
         }

         // Add the interaction item to the queue.
         interactions_queue.push( item );

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            if ( received_as_TSO ) {
               Int64Time _time;
               _time.set( theTime );

               string handle;
               StringUtilities::to_string( handle, theInteraction );
               send_hs( stdout, "Manager::receive_interaction():%d ID:%s, HLA-time:%G%c",
                        __LINE__, handle.c_str(), _time.get_time_in_seconds(),
                        THLA_NEWLINE );
            } else {
               string handle;
               StringUtilities::to_string( handle, theInteraction );
               send_hs( stdout, "Manager::receive_interaction():%d ID:%s%c",
                        __LINE__, handle.c_str(), THLA_NEWLINE );
            }
         }

         // Return now that we put the interaction-item into the queue.
         return;
      }
   }

   // Let ExectionControl receive any interactions.
   this->execution_control->receive_interaction( theInteraction,
                                                 theParameterValues,
                                                 theUserSuppliedTag,
                                                 theTime,
                                                 received_as_TSO );
}

/*!
 * @job_class{scheduled}
 */
Object *Manager::get_trickhla_object(
   RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_id )
{
   // We use a map with the key being the ObjectIntanceHandle for fast lookups.
   ObjectInstanceMap::const_iterator iter = object_map.find( instance_id );
   return ( ( iter != object_map.end() ) ? iter->second : static_cast< Object * >( NULL ) );
}

/*!
 * @job_class{scheduled}
 */
Object *Manager::get_trickhla_object(
   wstring const &obj_instance_name )
{
   wstring ws_obj_name;

   // Search the data objects first.
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      StringUtilities::to_wstring( ws_obj_name, objects[n].get_name() );
      if ( ws_obj_name == obj_instance_name ) {
         return ( &objects[n] );
      }
   }

   // Check for a match with the ExecutionConfiguration object associated with
   // ExecutionControl. Returns NULL if match not found.
   return ( this->execution_control->get_trickhla_object( obj_instance_name ) );
}

/*!
 * @job_class{scheduled}
 */
bool Manager::discover_object_instance(
   ObjectInstanceHandle theObject,
   ObjectClassHandle    theObjectClass,
   wstring const       &theObjectInstanceName )
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
         send_hs( stdout, "Manager::discover_object_instance():%d Data-Object '%s' Instance-ID:%s%c",
                  __LINE__, trickhla_obj->get_name(), id_str.c_str(), THLA_NEWLINE );
      }
   } else if ( ( federate != NULL ) && federate->is_MOM_HLAfederate_class( theObjectClass ) ) {

      federate->add_federate_instance_id( theObject );
      return_value = true;

      // save into my federate's discovered federate storage area
      federate->add_MOM_HLAfederate_instance_id( theObject, theObjectInstanceName );

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, theObject );
         StringUtilities::to_string( name_str, theObjectInstanceName );
         send_hs( stdout, "Manager::discover_object_instance():%d Discovered MOM HLA-Federate Object-Instance-ID:%s Name:'%s'%c",
                  __LINE__, id_str.c_str(), name_str.c_str(), THLA_NEWLINE );
      }
   } else if ( ( federate != NULL ) && federate->is_MOM_HLAfederation_class( theObjectClass ) ) {

      federate->add_MOM_HLAfederation_instance_id( theObject );
      return_value = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, theObject );
         StringUtilities::to_string( name_str, theObjectInstanceName );
         send_hs( stdout, "Manager::discover_object_instance():%d MOM HLA-Federation '%s' Instance-ID:%s%c",
                  __LINE__, name_str.c_str(), id_str.c_str(), THLA_NEWLINE );
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
   for ( unsigned int n = 0; n < obj_count; ++n ) {

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
   return ( this->execution_control->get_unregistered_object( theObjectClass, theObjectInstanceName ) );
}

/*!
 * @job_class{scheduled}
 */
Object *Manager::get_unregistered_remote_object(
   ObjectClassHandle const &theObjectClass )
{
   // Search the simulation data objects first.
   for ( unsigned int n = 0; n < obj_count; ++n ) {

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
   return ( this->execution_control->get_unregistered_remote_object( theObjectClass ) );
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
   if ( !this->execution_control->mark_object_as_deleted_from_federation( instance_id ) ) {

      Object *obj = get_trickhla_object( instance_id );
      if ( obj != NULL ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            string id_str;
            StringUtilities::to_string( id_str, instance_id );
            send_hs( stdout, "Manager::mark_object_as_deleted_from_federation():%d Object '%s' Instance-ID:%s Valid-ID:%s %c",
                     __LINE__, obj->get_name(), id_str.c_str(),
                     ( instance_id.isValid() ? "Yes" : "No" ), THLA_NEWLINE );
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
   this->execution_control->process_deleted_objects();

   // Search the simulation data objects, looking for deleted objects.
   for ( unsigned int n = 0; n < obj_count; ++n ) {
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
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      objects[n].pull_ownership();
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::push_ownership()
{
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      objects[n].push_ownership();
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::grant_pull_request()
{
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      objects[n].grant_pull_request();
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::release_ownership()
{
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      objects[n].release_ownership();
   }
}

/*!
 * @details If the federate does not exist, -1.0 seconds is assigned to the
 * returned object.
 */
Int64Interval Manager::get_lookahead() const
{
   return this->federate->get_lookahead();
}

/*!
 * @details If the federate does not exist, Int64BaseTime::get_max_logical_time_in_seconds()
 * is assigned to the returned object.
 */
Int64Time Manager::get_granted_time() const
{
   return this->federate->get_granted_time();
}

/*!
 * @details Returns the granted time in base time.
 */
int64_t const Manager::get_granted_base_time() const
{
   return this->federate->get_granted_base_time();
}

bool Manager::is_RTI_ready(
   char const *method_name )
{
   if ( this->federate == NULL ) {
      send_hs( stderr, "Manager::%s:%d Unexpected NULL 'federate' pointer!%c",
               method_name, __LINE__, THLA_NEWLINE );
      return false;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   bool rti_valid = true;
   if ( get_RTI_ambassador() == NULL ) {
      send_hs( stderr, "Manager::%s:%d Unexpected NULL RTIambassador!%c",
               method_name, __LINE__, THLA_NEWLINE );
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
      this->execution_control->convert_sim_time_to_scenario_time( freeze_sim_time ),
      file_name );
}

void Manager::start_federation_save_at_scenario_time(
   double      freeze_scenario_time,
   char const *file_name )
{
   // Call the ExecutionControl method.
   this->execution_control->start_federation_save_at_scenario_time( freeze_scenario_time, file_name );
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_checkpoint()
{
   // Call the ExecutionControl method.
   this->execution_control->setup_checkpoint();

   for ( unsigned int n = 0; n < obj_count; ++n ) {
      // Any object with a valid instance handle must be marked as required
      // to ensure the restore process will wait for this object instance
      // to exist.
      if ( objects[n].is_instance_handle_valid() ) {
         objects[n].mark_required();
      }
      // Setup the ownership handler checkpoint data structures.
      objects[n].setup_ownership_transfer_checkpointed_data();
   }

   setup_checkpoint_interactions();
}

void Manager::setup_checkpoint_interactions()
{
   // Clear the checkpoint for the interactions so that we don't leak memory.
   clear_interactions();

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &interactions_queue.mutex );

   if ( !interactions_queue.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::setup_checkpoint_interactions():%d interactions_queue.size()=%d%c",
                  __LINE__, interactions_queue.size(), THLA_NEWLINE );
      }

      check_interactions_count = interactions_queue.size();
      check_interactions       = reinterpret_cast< InteractionItem       *>(
         alloc_type( check_interactions_count, "TrickHLA::InteractionItem" ) );
      if ( check_interactions == static_cast< InteractionItem * >( NULL ) ) {
         ostringstream errmsg;
         errmsg << "Manager::setup_checkpoint_interactions():" << __LINE__
                << " ERROR: Failed to allocate enough memory for check_interactions"
                << " linear array of " << check_interactions_count << " elements."
                << THLA_ENDL;
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // interactions_queue.dump_head_pointers("interactions_queue.dump");

      for ( unsigned int i = 0; i < interactions_queue.size(); ++i ) {

         InteractionItem *item = static_cast< InteractionItem * >( interactions_queue.front() );

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::setup_checkpoint_interactions():%d \
Checkpointing into check_interactions[%d] from interaction index %d.%c",
                     __LINE__, i, item->index, THLA_NEWLINE );
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
               (unsigned char *)trick_MM->declare_var( "unsigned char", (int)item->user_supplied_tag_size );
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

void Manager::clear_interactions()
{
   if ( check_interactions_count > 0 ) {
      for ( unsigned int i = 0; i < check_interactions_count; ++i ) {
         check_interactions[i].clear_parm_items();
      }
      trick_MM->delete_var( check_interactions );
      check_interactions       = NULL;
      check_interactions_count = 0;
   }
}

void Manager::dump_interactions()
{
   if ( check_interactions_count > 0 ) {
      ostringstream msg;
      msg << "Manager::dump_interactions():" << __LINE__
          << "check_interactions contains these "
          << check_interactions_count << " elements:" << endl;
      for ( unsigned int i = 0; i < check_interactions_count; ++i ) {
         msg << "check_interactions[" << i << "].index                  = "
             << check_interactions[i].index << endl
             << "check_interactions[" << i << "].interaction_type       = '"
             << check_interactions[i].interaction_type << "'" << endl
             << "check_interactions[" << i << "].parm_items_count       = "
             << check_interactions[i].parm_items_count
             << endl;
         for ( unsigned int k = 0; k < check_interactions[i].parm_items_count; ++k ) {
            msg << "check_interactions[" << i << "].parm_items[" << k << "].index    = "
                << check_interactions[i].parm_items[k].index << endl
                << "check_interactions[" << i << "].parm_items[" << k << "].size     = "
                << check_interactions[i].parm_items[k].size
                << endl;
         }
         msg << "check_interactions[" << i << "].user_supplied_tag_size = "
             << check_interactions[i].user_supplied_tag_size << endl
             << "check_interactions[" << i << "].order_is_TSO           = "
             << check_interactions[i].order_is_TSO << endl
             << "check_interactions[" << i << "].time                   = "
             << check_interactions[i].time.get_base_time()
             << endl;
      }
      send_hs( stdout, (char *)msg.str().c_str() );
   }
}

void Manager::restore_interactions()
{
   if ( check_interactions_count > 0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::restore_interactions():%d check_interactions_count=%d%c",
                  __LINE__, check_interactions_count, THLA_NEWLINE );
      }

      for ( unsigned int i = 0; i < check_interactions_count; ++i ) {

         InteractionItem *item = new InteractionItem();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::restore_interactions():%d \
restoring check_interactions[%d] into interaction index %d, parm_count=%d%c",
                     __LINE__, i, check_interactions[i].index,
                     check_interactions[i].parm_items_count, THLA_NEWLINE );
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
            item->user_supplied_tag = (unsigned char *)trick_MM->mm_strdup( (char *)check_interactions[i].user_supplied_tag );
         }
         item->order_is_TSO = check_interactions[i].order_is_TSO;
         item->time         = check_interactions[i].time;

         interactions_queue.push( item );
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::pull_ownership_upon_rejoin()
{
   for ( unsigned int n = 0; n < obj_count; ++n ) {
      if ( objects[n].is_create_HLA_instance() ) {
         objects[n].pull_ownership_upon_rejoin();
      }
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
      send_hs( stdout, "Manager::wait_for_discovery_of_object_instance():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Do we have Simulation object(s) to interrogate?
   if ( obj_count > 0 ) {

      // See if any object discoveries have occurred.
      int  required_count                   = 0;
      int  discovery_count                  = 0;
      bool create_HLA_instance_object_found = false;
      for ( unsigned int n = 0; n < obj_count; ++n ) {
         if ( objects[n].is_required() ) {
            required_count++;
         }
         if ( objects[n].is_instance_handle_valid() ) {
            discovery_count++;
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
            send_hs( stdout, "Manager::wait_for_discovery_of_object_instance():%d Waiting for object discovery callbacks to arrive.%c",
                     __LINE__, THLA_NEWLINE );
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
            (void)sleep_timer.sleep();

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
                         << " the Central RTI Component (CRC) level!"
                         << THLA_ENDL;
                  DebugHandler::terminate_with_message( errmsg.str() );
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               send_hs( stdout, "Manager::wait_for_discovery_of_object_instance():%d Waiting...%c",
                        __LINE__, THLA_NEWLINE );
            }

            // Check if any objects were discovered while we were sleeping.
            discovery_count                  = 0;
            create_HLA_instance_object_found = false;
            for ( unsigned int n = 0; n < obj_count; ++n ) {
               if ( objects[n].is_required() && objects[n].is_instance_handle_valid() ) {
                  ++discovery_count;
                  if ( objects[n].is_create_HLA_instance() ) {
                     create_HLA_instance_object_found = true;
                  }
               }
            }

         } while ( ( !create_HLA_instance_object_found && // still missing some objects other than
                     ( discovery_count < ( required_count - 1 ) ) )
                   ||                                          // the one for the rejoining federate, or
                   ( create_HLA_instance_object_found &&       // missing some other object(s) but
                     ( discovery_count < required_count ) ) ); // found the rejoining federate
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::wait_for_discovery_of_object_instance():%d - No Objects to discover.%c",
                  __LINE__, THLA_NEWLINE );
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
   for ( unsigned int n = 0; n < obj_count; ++n ) {
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
   return ( ( federate != NULL ) ? federate->get_RTI_ambassador()
                                 : static_cast< RTI1516_NAMESPACE::RTIambassador * >( NULL ) );
}

bool Manager::is_shutdown_called() const
{
   return ( ( this->federate != NULL ) ? this->federate->is_shutdown_called() : false );
}
