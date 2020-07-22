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
@trick_link_dependency{Types.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2006, --, DSES Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <float.h>

// Trick include files.
#include "trick/Executive.hh"
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/Constants.hh"
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/InteractionItem.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/ParameterItem.hh"
#include "TrickHLA/Utilities.hh"
#include <TrickHLA/SleepTimeout.hh>

// HLA include files.
#include RTI1516_HEADER

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
     debug_handler(),
     restore_federation( 0 ),
     restore_file_name( NULL ),
     initiated_a_federation_save( false ),
     interactions_queue(),
     check_interactions_count( 0 ),
     check_interactions( NULL ),
     job_cycle_time( 0.0 ),
     rejoining_federate( false ),
     restore_determined( false ),
     restore_federate( false ),
     mgr_initialized( false ),
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
   Federate &            federate,
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
      send_hs( stderr, "Manager::initialize():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   // Check to make sure we have a reference to the TrickHLA::Manager.
   if ( federate == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " Unexpected NULL TrickHLA::Federate." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
      return;
   }

   // Check to make sure we have a reference to the TrickHLA::ExecutionControlBase.
   if ( execution_control == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::initialize():" << __LINE__
             << " Unexpected NULL TrickHLA::ExecutionControlBase." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
      return;
   }

   // Reset the TrickHLA Object count value if needed.
   if ( ( obj_count < 0 ) || ( objects == NULL ) ) {
      obj_count = 0;
   }

   // Reset the TrickHLA Interaction count value if needed.
   if ( ( inter_count < 0 ) || ( interactions == NULL ) ) {
      inter_count = 0;
   }

   // The manager is now initialized.
   this->mgr_initialized = true;

   // Verify the debug level is correct just in case the user specifies it in
   // the input file as an integer instead of using the ENUM values...
   if ( ( debug_handler.get_debug_level_as_int() < DEBUG_LEVEL_NO_TRACE ) || ( debug_handler.get_debug_level_as_int() > DEBUG_LEVEL_FULL_TRACE ) ) {
      send_hs( stderr, "Manager::initialize():%d You specified an \
invalid debug level '%d' in the input file using an integer value instead of \
an ENUM. Please double check the value you specified in the input file against \
the documented ENUM values.%c",
               __LINE__, debug_handler.get_debug_level_as_int(),
               THLA_NEWLINE );
      if ( debug_handler.get_debug_level_as_int() < DEBUG_LEVEL_NO_TRACE ) {
         send_hs( stderr, "Manager::initialize():%d No TrickHLA debug messages will be emitted.%c",
                  __LINE__, THLA_NEWLINE );
      } else {
         send_hs( stderr, "Manager::initialize():%d All TrickHLA debug messages will be emitted.%c",
                  __LINE__, THLA_NEWLINE );
      }
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

void Manager::restart_initialization()
{
   // Just return if the TrickHLA Manager is not initialized.
   if ( !mgr_initialized ) {
      send_hs( stderr, "Manager::restart_initialization():%d Manager Not initialized, returning...%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( federate == NULL ) {
      send_hs( stderr, "Manager::restart_initialization():%d Unexpected NULL Federate.%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Manager::restart_initialization() Unexpected NULL Federate." );
   }

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::restart_initialization():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Make sure the Federate is initialized after the restart.
   federate->restart_initialization();

   // To allow manager initialization to complete we need to reset the init flag.
   this->mgr_initialized = false;

   // Reset the TrickHLA Object count value if needed.
   if ( ( obj_count < 0 ) || ( objects == NULL ) ) {
      obj_count = 0;
   }

   // Reset the TrickHLA Interaction count value if needed.
   if ( ( inter_count < 0 ) || ( interactions == NULL ) ) {
      inter_count = 0;
   }

   // Setup the Execution Control and Execution Configuration objects now that
   // we know if we are the "Master" federate or not.
   if ( execution_control == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::restart_initialization():" << __LINE__
             << " NULL ExecutionControl reference." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } else {
      // FIXME: I think that this is circular.
      execution_control->set_master( execution_control->is_master() );
   }

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
      (void)federate->wait_for_required_federates_to_join();
   }

   // Restore ownership_transfer data for all objects...
   for ( int i = 0; i < obj_count; i++ ) {
      objects[i].restore_ownership_transfer_checkpointed_data();
   }

   // Restore checkpointed interactions...
   restore_interactions();

   // The manager is now initialized.
   this->mgr_initialized = true;

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::send_init_data()
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::send_init_data():%d Late joining \
federate so this call will be ignored.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   // Go through the list of objects.
   for ( int i = 0; i < obj_count; i++ ) {
      // Make sure we have at least one piece of object init data we can send.
      if ( objects[i].any_locally_owned_published_init_attribute() ) {

         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::send_init_data():%d '%s'%c",
                     __LINE__, objects[i].get_name(), THLA_NEWLINE );
         }

         // Send the object init data to the other federates.
         objects[i].send_init_data();
      } else {
         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::send_init_data():%d Nothing to send for '%s'%c",
                     __LINE__, objects[i].get_name(), THLA_NEWLINE );
         }
      }
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::send_init_data(
   const char *instance_name )
{

   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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
             << " Null Object Instance Name" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
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
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } else {

      // Make sure we have at least one piece of object init data we can send.
      if ( obj->any_locally_owned_published_init_attribute() ) {

         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::send_init_data():%d '%s'%c",
                     __LINE__, instance_name, THLA_NEWLINE );
         }

         // Send the object init data to the other federates.
         obj->send_init_data();
      } else {
         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::send_init_data():%d Nothing to send for '%s'%c",
                     __LINE__, instance_name, THLA_NEWLINE );
         }
      }
   }
   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::receive_init_data()
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::receive_init_data():%d Late joining \
federate so this call will be ignored.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   // Go through the list of objects.
   for ( int i = 0; i < obj_count; i++ ) {

      // Make sure we have at least one piece of data we can receive.
      if ( objects[i].any_remotely_owned_subscribed_init_attribute() ) {

         // Only wait for REQUIRED received init data and do not block waiting
         // to receive init data if we are using the simple init scheme.
         bool obj_required = objects[i].is_required() && ( execution_control->wait_on_init_data() );

         if ( obj_required ) {
            if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.%c",
                        __LINE__, objects[i].get_name(),
                        ( objects[i].is_required() ? "REQUIRED" : "not required" ), THLA_NEWLINE );
            }

            SleepTimeout sleep_timer( 10.0, 1000 );

            // Wait for the data to arrive.
            while ( !objects[i].is_changed() ) {

               // Check for shutdown.
               federate->check_for_shutdown_with_termination();

               (void)sleep_timer.sleep();

               if ( !objects[i].is_changed() && sleep_timer.timeout() ) {
                  sleep_timer.reset();
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "Manager::receive_init_data():" << __LINE__
                            << " Unexpectedly the Federate is no longer an execution member."
                            << " This means we are either not connected to the"
                            << " RTI or we are no longer joined to the federation"
                            << " execution because someone forced our resignation at"
                            << " the Central RTI Component (CRC) level!"
                            << THLA_ENDL;
                     send_hs( stderr, (char *)errmsg.str().c_str() );
                     exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
                  }
               }
            }
         }

         // Check for changed data which means we received something.
         if ( objects[i].is_changed() ) {
            if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Received '%s'%c",
                        __LINE__, objects[i].get_name(), THLA_NEWLINE );
            }

            // Receive the data from the publishing federate.
            objects[i].receive_init_data();
         } else {
            if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Received nothing for '%s', and marked as %s.%c",
                        __LINE__, objects[i].get_name(),
                        ( obj_required ? "REQUIRED" : "not required" ), THLA_NEWLINE );
            }
         }
      } else {
         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::receive_init_data():%d Nothing to receive for '%s'%c",
                     __LINE__, objects[i].get_name(), THLA_NEWLINE );
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::receive_init_data(
   const char *instance_name )
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::receive_init_data():%d Late joining federate so skipping data for '%s'%c",
                  __LINE__, instance_name, THLA_NEWLINE );
      }
      return;
   }

   if ( instance_name == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::receive_init_data():" << __LINE__
             << " Null Object Instance Name";
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   wstring obj_instance_name;
   StringUtilities::to_wstring( obj_instance_name, instance_name );

   Object *obj = get_trickhla_object( obj_instance_name );

   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::receive_init_data():" << __LINE__
             << " ERROR: The specified"
             << " Object Instance Name '" << instance_name
             << "' does not correspond to any known object. Please check your"
             << " S_define file or simulation module to verify the settings."
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } else {

      // Make sure we have at least one piece of data we can receive.
      if ( obj->any_remotely_owned_subscribed_init_attribute() ) {

         // Only wait for REQUIRED received init data and do not block waiting
         // to receive init data if we are using the simple init scheme.
         bool obj_required = obj->is_required() && ( execution_control->wait_on_init_data() );

         if ( obj_required ) {
            if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Waiting for '%s', and marked as %s.%c",
                        __LINE__, instance_name,
                        ( obj->is_required() ? "REQUIRED" : "not required" ), THLA_NEWLINE );
            }

            SleepTimeout sleep_timer( 10.0, 1000 );

            // Wait for the data to arrive.
            while ( !obj->is_changed() ) {

               // Check for shutdown.
               federate->check_for_shutdown_with_termination();

               (void)sleep_timer.sleep();

               if ( !obj->is_changed() && sleep_timer.timeout() ) {
                  sleep_timer.reset();
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "Manager::receive_init_data():" << __LINE__
                            << " Unexpectedly the Federate is no longer an execution member."
                            << " This means we are either not connected to the"
                            << " RTI or we are no longer joined to the federation"
                            << " execution because someone forced our resignation at"
                            << " the Central RTI Component (CRC) level!"
                            << THLA_ENDL;
                     send_hs( stderr, (char *)errmsg.str().c_str() );
                     exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
                  }
               }
            }
         }

         // Check for changed data which means we received something.
         if ( obj->is_changed() ) {
            if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout,
                        "Manager::receive_init_data():%d Received '%s'%c",
                        __LINE__, instance_name, THLA_NEWLINE );
            }

            // Receive the data from the publishing federate.
            obj->receive_init_data();
         } else {
            if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               send_hs( stdout, "Manager::receive_init_data():%d Received nothing for '%s', and marked as %s.%c",
                        __LINE__, instance_name,
                        ( obj_required ? "REQUIRED" : "not required" ), THLA_NEWLINE );
            }
         }
      } else {
         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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
   execution_control->clear_multiphase_init_sync_points();

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::wait_for_init_sync_point(
   const char *sync_point_label )
{
   //if ( ( sim_initialization_scheme == INIT_SCHEME_SIMPLE ) || ( sim_initialization_scheme == INIT_SCHEME_DIS_COMPATIBLE ) ) {
   if ( !get_execution_control()->wait_on_init_sync_point() ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::wait_for_init_sync_point():%d This call \
will be ignored because sim_initialization_scheme == (INIT_SCHEME_SIMPLE or \
INIT_SCHEME_DIS_COMPATIBLE).%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( is_late_joining_federate() ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::wait_for_init_sync_point():%d Late \
joining federate so this call will be ignored.%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   if ( sync_point_label == NULL ) {
      ostringstream errmsg;
      errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
             << " Null Sync-Point Label" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   wstring ws_syc_point_label;
   StringUtilities::to_wstring( ws_syc_point_label, sync_point_label );

   // Determine if the sync-point label is valid.
   if ( execution_control->contains( ws_syc_point_label ) ) {

      // Achieve the specified sync-point and wait for the federation
      // to be synchronized on it.
      execution_control->achieve_and_wait_for_synchronization( *( this->get_RTI_ambassador() ),
                                                               this->federate,
                                                               ws_syc_point_label );

   } else {
      ostringstream errmsg;
      errmsg << "Manager::wait_for_init_sync_point():" << __LINE__
             << " The specified Initialization Synchronization-Point label '"
             << sync_point_label
             << "' is not known. Please check your input or modified data files"
             << " to make sure the 'THLA.federate.multiphase_init_sync_points'"
             << " correctly specifies all the synchronization-point labels that"
             << " will be used for multi-phase initialization." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::request_data_update(
   wstring const &instance_name )
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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
   const char *instance_name )
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

         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::object_instance_name_reservation_succeeded():%d \
RESERVED Object Instance Name '%s'%c",
                     __LINE__, trickhla_obj->get_name(),
                     THLA_NEWLINE );
         }
      }
   }

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::object_instance_name_reservation_failed(
   wstring const &obj_instance_name )
{

   // Different ExecutionControl mechanisms will handle object instance name
   // failure differently.  So, check with the ExecutionControl to perform
   // any specialized failure handling.  If the method returns 'true' then
   // it's not a fatal error; otherwise, continue with error handling and
   // exit.
   if ( execution_control->object_instance_name_reservation_failed( obj_instance_name ) ) {
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
   for ( int n = 0; n < obj_count; n++ ) {
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
'THLA.federate.can_rejoin_federation' flag to true in the input file which \
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
   // be unique to our object, so quit the simulation.  However, since we are
   // running in a child thread created by the RTI, we need to tell the Trick
   // Executive to exit the simulation.
   exec_set_exec_command( ExitCmd );
   // Bail from the execution just in case the above command fails
   ostringstream errmsg;
   errmsg << "Manager::object_instance_name_reservation_failed:" << __LINE__
          << " Exiting..." << THLA_ENDL;
   send_hs( stderr, (char *)errmsg.str().c_str() );
   exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   exit( 1 );

   return;
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
        && ( this->object_map.find( object->get_instance_handle() )
             == this->object_map.end() ) ) {
      this->object_map[object->get_instance_handle()] = object;
   }
   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_all_ref_attributes()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_all_ref_attributes():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Make sure the object-map is empty/clear before we continue.
   object_map.clear();

   if ( is_execution_configuration_used() ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::setup_all_ref_attributes():%d Execution-Configuration %c",
                  __LINE__, THLA_NEWLINE );
      }
      setup_object_ref_attributes( 1, get_execution_configuration() );
   }

   if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_all_ref_attributes():%d Objects %c",
               __LINE__, THLA_NEWLINE );
   }
   setup_object_ref_attributes( obj_count, objects );

   if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_all_ref_attributes():%d Interactions %c",
               __LINE__, THLA_NEWLINE );
   }
   setup_interaction_ref_attributes();
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_object_ref_attributes(
   const int data_obj_count,
   Object *  data_objects )
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      send_hs( stderr, "Manager::setup_object_ref_attributes():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_object_ref_attributes():%d %c",
               __LINE__, THLA_NEWLINE );
   }

   // Resolve all the Ref-Attributes for all the simulation initialization
   // objects and attributes.
   for ( int n = 0; n < data_obj_count; n++ ) {

      // Initialize the TrickHLA-Object before we use it.
      data_objects[n].initialize( this );

      ostringstream msg;

      if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         msg << "Manager::setup_object_ref_attributes()" << __LINE__ << endl
             << "--------------- Trick REF-Attributes ---------------" << endl
             << " Object:'" << data_objects[n].get_name() << "'"
             << " FOM-Name:'" << data_objects[n].get_FOM_name() << "'"
             << " Create HLA Instance:"
             << ( data_objects[n].is_create_HLA_instance() ? "Yes" : "No" )
             << endl;
      }

      int        attr_count = data_objects[n].get_attribute_count();
      Attribute *attrs      = data_objects[n].get_attributes();

      // Process the attributes for this object.
      for ( int i = 0; i < attr_count; i++ ) {

         // Initialize the TrickHLA-Attribute before we use it.
         attrs[i].set_debug_level( debug_handler );
         attrs[i].initialize( data_objects[n].get_FOM_name(), n, i );

         if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attrs[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attrs[i].get_trick_name() << "'"
                << endl;
         }
      }

      if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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
      send_hs( stderr, "Manager::setup_interaction_ref_attributes():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_interaction_ref_attributes():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Interactions.
   for ( int n = 0; n < inter_count; n++ ) {
      ostringstream msg;

      if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         msg << "Manager::setup_interaction_ref_attributes():" << __LINE__ << endl
             << "--------------- Trick REF-Attributes ---------------" << endl
             << " FOM-Interaction:'" << interactions[n].get_FOM_name() << "'"
             << endl;
      }

      // Initialize the TrickHLA Interaction before we use it.
      interactions[n].initialize( this );

      int        param_count = interactions[n].get_parameter_count();
      Parameter *params      = interactions[n].get_parameters();

      // Process the attributes for this object.
      for ( int i = 0; i < param_count; i++ ) {

         if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "   " << ( i + 1 ) << "/" << param_count
                << " FOM-Parameter:'" << params[i].get_FOM_name() << "'"
                << " Trick-Name:'" << params[i].get_trick_name() << "'"
                << endl;
         }

         // Initialize the TrickHLA Parameter.
         params[i].set_debug_level( debug_handler );
         params[i].initialize( interactions[n].get_FOM_name(), n, i );
      }

      if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, (char *)msg.str().c_str() );
      }
   }

   // Tell the ExecutionControl object to setup the appropriate Trick Ref
   // ATTRIBUTES associated with the execution control mechanism.
   execution_control->setup_interaction_ref_attributes();

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_all_RTI_handles()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_all_RTI_handles():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Set up the object RTI handles for the ExecutionControl mechanisms.
   execution_control->setup_object_RTI_handles();

   // Set up the object RTI handles for the simulation data objects.
   setup_object_RTI_handles( obj_count, objects );

   // Set up the object RTI handles for the ExecutionControl mechanisms.
   execution_control->setup_interaction_RTI_handles();

   // Simulation Interactions.
   setup_interaction_RTI_handles( inter_count, interactions );

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_object_RTI_handles(
   const int data_obj_count,
   Object *  data_objects )
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      send_hs( stderr, "Manager::setup_object_RTI_handles():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( federate == NULL ) {
      send_hs( stderr, "Manager::setup_object_RTI_handles():%d Unexpected NULL Federate.%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Manager::setup_object_RTI_handles() Unexpected NULL Federate." );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      send_hs( stderr, "Manager::setup_object_RTI_handles():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Manager::setup_object_RTI_handles() Unexpected NULL RTIambassador." );
      return;
   }

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_object_RTI_handles():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   const char *obj_FOM_name  = "";
   const char *attr_FOM_name = "";
   wstring     ws_FOM_name   = L"";
   int         FOM_name_type = 0; //0:N/A 1:Object 2:Attribute - What name are we dealing with.

   // Initialize the Object and Attribute RTI handles.
   try {

      // Resolve all the handles/ID's for the objects and attributes.
      for ( int n = 0; n < data_obj_count; n++ ) {
         ostringstream msg;

         if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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

         if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            string handle_str;
            StringUtilities::to_string( handle_str, data_objects[n].get_class_handle() );
            msg << "  Result for"
                << " Object:'" << data_objects[n].get_name() << "'"
                << " FOM-Name:'" << data_objects[n].get_FOM_name() << "'"
                << " Class-ID:" << handle_str << endl;
         }

         int        attr_count = data_objects[n].get_attribute_count();
         Attribute *attrs      = data_objects[n].get_attributes();

         // Resolve the handles/ID's for the attributes.
         for ( int i = 0; i < attr_count; i++ ) {

            if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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

            if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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

         if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, (char *)msg.str().c_str() );
         }
      }
   } catch ( NameNotFound &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      switch ( FOM_name_type ) {
         case 1: { // Object
            ostringstream errmsg;
            errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
                   << " Object FOM Name '" << obj_FOM_name << "' Not Found. Please check"
                   << " your input or modified-data files to make sure the"
                   << " Object FOM Name is correctly specified." << THLA_ENDL;
            send_hs( stderr, (char *)errmsg.str().c_str() );
            exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            break;
         }
         case 2: { // Attribute
            ostringstream errmsg;
            errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
                   << " For Object FOM Name '" << obj_FOM_name << "', Attribute FOM Name '"
                   << attr_FOM_name << "' Not Found. Please check your input or"
                   << " modified-data files to make sure the Object Attribute"
                   << " FOM Name is correctly specified." << THLA_ENDL;
            send_hs( stderr, (char *)errmsg.str().c_str() );
            exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            break;
         }
         default: { // FOM name we are working with is unknown.
            ostringstream errmsg;
            errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
                   << " Object or Attribute FOM Name Not Found. Please check your input or"
                   << " modified-data files to make sure the FOM Name is"
                   << " correctly specified." << THLA_ENDL;
            send_hs( stderr, (char *)errmsg.str().c_str() );
            exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            break;
         }
      }
   } catch ( FederateNotExecutionMember &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " Federate Not Execution Member" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } catch ( NotConnected &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " Not Connected" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } catch ( RTIinternalError &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " RTIinternalError: '"
             << rti_err_msg << "'" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_object_RTI_handles():" << __LINE__
             << " RTI1516_EXCEPTION for '"
             << rti_err_msg << "'" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_interaction_RTI_handles(
   const int    interactions_counter,
   Interaction *in_interactions )
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      send_hs( stderr, "Manager::setup_interaction_RTI_handles():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( federate == NULL ) {
      send_hs( stderr, "Manager::setup_interaction_RTI_handles():%d Unexpected NULL Federate.%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Manager::setup_interaction_RTI_handles() Unexpected NULL Federate." );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      send_hs( stderr, "Manager::setup_interaction_RTI_handles():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Manager::setup_interaction_RTI_handles() Unexpected NULL RTIambassador." );
      return;
   }

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_interaction_RTI_handles():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   const char *inter_FOM_name = "";
   const char *param_FOM_name = "";
   wstring     ws_FOM_name    = L"";
   int         FOM_name_type  = 0; //0:NA 1:Interaction 2:Parameter  What name we are dealing with.

   // Initialize the Interaction and Parameter RTI handles.
   try {

      // Process all the Interactions.
      for ( int n = 0; n < interactions_counter; n++ ) {
         ostringstream msg;

         // The Interaction FOM name.
         FOM_name_type  = 1; // Interaction
         inter_FOM_name = in_interactions[n].get_FOM_name();
         StringUtilities::to_wstring( ws_FOM_name, inter_FOM_name );

         if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            msg << "Manager::setup_interaction_RTI_handles()" << __LINE__ << endl
                << "----------------- RTI Handles (Interactions & Parameters) ---------------" << endl
                << "Getting RTI Interaction-Class-Handle for"
                << " FOM-Name:'" << inter_FOM_name << "'"
                << endl;
         }

         // Get the Interaction class handle.
         in_interactions[n].set_class_handle( rti_amb->getInteractionClassHandle( ws_FOM_name ) );

         if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            string handle_str;
            StringUtilities::to_string( handle_str,
                                        in_interactions[n].get_class_handle() );
            msg << "  Result for Interaction"
                << " FOM-Name:'" << inter_FOM_name << "'"
                << " Interaction-ID:" << handle_str << endl;
         }

         // The parameters.
         int        param_count = in_interactions[n].get_parameter_count();
         Parameter *params      = in_interactions[n].get_parameters();

         // Process the parameters for the interaction.
         for ( int i = 0; i < param_count; i++ ) {

            // The Parameter FOM name.
            FOM_name_type  = 2; // Parameter
            param_FOM_name = params[i].get_FOM_name();
            StringUtilities::to_wstring( ws_FOM_name, param_FOM_name );

            if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               msg << "\tGetting RTI Parameter-Handle for '"
                   << inter_FOM_name << "'->'" << param_FOM_name << "'" << endl;
            }

            // Get the Parameter Handle.
            params[i].set_parameter_handle(
               rti_amb->getParameterHandle(
                  in_interactions[n].get_class_handle(),
                  ws_FOM_name ) );

            if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               string handle_str;
               StringUtilities::to_string( handle_str,
                                           params[i].get_parameter_handle() );
               msg << "\t  Result for Parameter '"
                   << inter_FOM_name << "'->'" << param_FOM_name << "'"
                   << " Parameter-ID:" << handle_str << endl;
            }
         }

         if ( debug_handler.should_print( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, (char *)msg.str().c_str() );
         }
      }
   } catch ( NameNotFound &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      switch ( FOM_name_type ) {
         case 1: { // Interaction
            ostringstream errmsg;
            errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
                   << " Interaction FOM Name '" << inter_FOM_name << "' Not Found. Please"
                   << " check your input or modified-data files to make sure the"
                   << " Interaction FOM Name is correctly specified." << THLA_ENDL;
            send_hs( stderr, (char *)errmsg.str().c_str() );
            exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            break;
         }
         case 2: { // Parameter
            ostringstream errmsg;
            errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
                   << " For Interaction FOM Name '" << inter_FOM_name
                   << "', Parameter FOM Name '" << param_FOM_name
                   << "' Not Found. Please check your input or modified-data files"
                   << " to make sure the Interaction Parameter FOM Name is"
                   << " correctly specified." << THLA_ENDL;
            send_hs( stderr, (char *)errmsg.str().c_str() );
            exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            break;
         }
         default: { // FOM name we are working with is unknown.
            ostringstream errmsg;
            errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
                   << " Interaction or Parameter FOM Name Not Found. Please check your input"
                   << " or modified-data files to make sure the FOM Name is"
                   << " correctly specified." << THLA_ENDL;
            send_hs( stderr, (char *)errmsg.str().c_str() );
            exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            break;
         }
      }
   } catch ( FederateNotExecutionMember &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      send_hs( stderr, "Manager::setup_interaction_RTI_handles():%d FederateNotExecutionMember%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Manager::setup_interaction_RTI_handles() FederateNotExecutionMember" );
   } catch ( NotConnected &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      send_hs( stderr, "Manager::setup_interaction_RTI_handles():%d NotConnected%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Manager::setup_interaction_RTI_handles() NotConnected" );
   } catch ( RTIinternalError &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " RTIinternalError: '" << rti_err_msg << "'" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::setup_interaction_RTI_handles():" << __LINE__
             << " exception for '" << rti_err_msg << "'" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
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
   int n;

   if ( !is_RTI_ready( "publish" ) ) {
      return;
   }

   // Publish attributes for all the Trick-HLA-Objects we know about.
   for ( n = 0; n < obj_count; n++ ) {
      objects[n].publish_object_attributes();
   }

   // Publish the interactions.
   for ( n = 0; n < inter_count; n++ ) {
      interactions[n].publish_interaction();
   }

   // Unpublish and Execution Control objects and interactions.
   execution_control->publish();

   return;
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
   for ( i = 0; i < obj_count; i++ ) {
      // Only unpublish an object class if we had published at least
      // one attribute.
      if ( objects[i].any_attribute_published() ) {
         do_unpublish = true;
         for ( k = 0; ( k < i ) && do_unpublish; k++ ) {
            // Unpublish an object Class only once, so see if we have already
            // unpublished from the same object class that was published.
            if ( objects[k].any_attribute_published() && ( objects[i].get_class_handle() == objects[k].get_class_handle() ) ) {
               do_unpublish = false;
            }
         }
         if ( do_unpublish ) {
            objects[i].unpublish_all_object_attributes();
         }
      }
   }

   // Unpublish all the interactions.
   for ( i = 0; i < inter_count; i++ ) {
      // Only unpublish an interaction that we publish.
      if ( interactions[i].is_publish() ) {
         do_unpublish = true;
         for ( k = 0; ( k < i ) && do_unpublish; k++ ) {
            // Unpublish an interaction Class only once, so see if we have
            // already unpublished the same interaction class that was published.
            if ( interactions[k].is_publish() && ( interactions[i].get_class_handle() == interactions[k].get_class_handle() ) ) {
               do_unpublish = false;
            }
         }
         if ( do_unpublish ) {
            interactions[i].unpublish_interaction();
         }
      }
   }

   // Unpublish and Execution Control objects and interactions.
   execution_control->unpublish();

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::subscribe()
{
   int n;

   if ( !is_RTI_ready( "subscribe" ) ) {
      return;
   }

   // Subscribe to attributes for all the Trick-HLA-Objects we know about.
   for ( n = 0; n < obj_count; n++ ) {
      objects[n].subscribe_to_object_attributes();
   }

   // Subscribe to the interactions.
   for ( n = 0; n < inter_count; n++ ) {
      interactions[n].subscribe_to_interaction();
   }

   // Subscribe to anything needed for the execution control mechanisms.
   execution_control->subscribe();

   return;
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
   for ( i = 0; i < obj_count; i++ ) {
      // Only unsubscribe from an object class if we had subscribed to at
      // least one attribute.
      if ( objects[i].any_attribute_subscribed() ) {
         do_unsubscribe = true;
         for ( k = 0; ( k < i ) && do_unsubscribe; k++ ) {
            // Unsubscribe from an object Class only once, so see if
            // we have already unsubscribed from the same object class
            // that was subscribed to.
            if ( objects[k].any_attribute_subscribed() && ( objects[i].get_class_handle() == objects[k].get_class_handle() ) ) {
               do_unsubscribe = false;
            }
         }
         if ( do_unsubscribe ) {
            objects[i].unsubscribe_all_object_attributes();
         }
      }
   }

   // Unsubscribe from all the interactions.
   for ( i = 0; i < inter_count; i++ ) {
      // Only unsubscribe from interactions that are subscribed to.
      if ( interactions[i].is_subscribe() ) {
         do_unsubscribe = true;
         for ( k = 0; ( k < i ) && do_unsubscribe; k++ ) {
            // Unsubscribe from an interaction Class only once, so see if
            // we have already unsubscribed from the same interaction class
            // that was subscribed to.
            if ( interactions[k].is_subscribe() && ( interactions[i].get_class_handle() == interactions[k].get_class_handle() ) ) {
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

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::publish_and_subscribe()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::reserve_object_names_with_RTI():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // For the locally owned objects, reserve the object instance name with
   // the RTI.
   for ( int n = 0; n < obj_count; n++ ) {
      objects[n].reserve_object_name_with_RTI();
   }
}

/*!
 * @details Calling this function will block until all the object instances
 * names for the locally owned objects have been reserved.
 * @job_class{initialization}
 */
void Manager::wait_on_reservation_of_object_names()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::wait_on_reservation_of_object_names():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Simulation object names.
   if ( obj_count > 0 ) {
      // Wait for each RTI object instance name to be registered with the RTI,
      // but for only the names we requested registration for.
      for ( int n = 0; n < obj_count; n++ ) {
         objects[n].wait_on_object_name_reservation();
      }

      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::wait_on_reservation_of_object_names():%d All Object instance names reserved.%c",
                  __LINE__, THLA_NEWLINE );
      }
   } else {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::wait_on_reservation_of_object_names():%d No Object instance names to reserve.%c",
                  __LINE__, THLA_NEWLINE );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::register_objects_with_RTI()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::register_objects_with_RTI():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Have the ExecutionControl register objects it needs with the RTI.
   execution_control->register_objects_with_RTI();

   // For the locally owned objects register it with the RTI to get its
   // RTI object instance ID.
   for ( int n = 0; n < obj_count; n++ ) {
      objects[n].register_object_with_RTI();

      // Add the registered object instance to the map and only if it is
      // not already in it.
      if ( ( objects[n].is_instance_handle_valid() ) && ( object_map.find( objects[n].get_instance_handle() ) == object_map.end() ) ) {
         object_map[objects[n].get_instance_handle()] = &objects[n];
      }
   }
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_preferred_order_with_RTI()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::setup_preferred_order_with_RTI():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   if ( is_execution_configuration_used() ) {
      // Register the execution configuration object.
      get_execution_configuration()->setup_preferred_order_with_RTI();
   }

   // Setup the preferred order for all the object attributes.
   for ( int n = 0; n < obj_count; n++ ) {
      objects[n].setup_preferred_order_with_RTI();
   }

   // Setup the preferred order for all the interactions.
   for ( int i = 0; i < inter_count; i++ ) {
      interactions[i].setup_preferred_order_with_RTI();
   }
}

/*!
 * @details Calling this function will block until all the required object
 * instances in the Federation have been registered.
 * @job_class{initialization}
 */
void Manager::wait_on_registration_of_required_objects()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::wait_on_registration_of_required_objects():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   int  n;
   int  required_obj_cnt;
   int  registered_obj_cnt;
   int  current_registered_obj_cnt = 0;
   int  total_obj_cnt              = obj_count;
   int  current_required_obj_cnt   = 0;
   int  total_required_obj_cnt     = 0;
   bool print_summary              = debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER );
   bool any_unregistered_required_obj;

   if ( is_execution_configuration_used() ) {
      // Make sure to count the exec-config object.
      total_obj_cnt++;

      // Determine if the Execution-Configuration object is required and it should be.
      if ( get_execution_configuration()->is_required() ) {
         total_required_obj_cnt++;
      }
   }

   // Loop through all of the objects to count the # of required objects; do not
   // assume that all of them are required!
   for ( int i = 0; i < obj_count; i++ ) {
      if ( objects[i].is_required() ) {
         total_required_obj_cnt++;
      }
   }

   SleepTimeout sleep_timer( 10.0, 1000 );

   do {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // Data objects.
      if ( current_required_obj_cnt < total_required_obj_cnt ) {
         required_obj_cnt   = 0;
         registered_obj_cnt = 0;

         if ( is_execution_configuration_used() ) {
            // Determine if the Execution-Configuration object has been
            // registered and only if it is required.
            if ( get_execution_configuration()->is_instance_handle_valid() ) {
               registered_obj_cnt++;
               if ( get_execution_configuration()->is_required() ) {
                  required_obj_cnt++;
               }
            }
         }

         // Determine how many data objects have been registered and only if
         // they are required.
         for ( n = 0; n < obj_count; n++ ) {
            if ( objects[n].is_instance_handle_valid() ) {
               registered_obj_cnt++;
               if ( objects[n].is_required() ) {
                  required_obj_cnt++;
               }
            }
         }

         // If we have a new registration count then update the object
         // registration count and set the flag to print a new summary.
         if ( registered_obj_cnt > current_registered_obj_cnt ) {
            current_registered_obj_cnt = registered_obj_cnt;
            print_summary              = debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER );
         }

         // Update the current count of the Required registered objects.
         if ( required_obj_cnt > current_required_obj_cnt ) {
            current_required_obj_cnt = required_obj_cnt;
         }
      }

      // Print a summary of what objects are registered and which ones are not.
      if ( print_summary ) {
         print_summary = false;
         int cnt       = 1;

         // Build the summary as an output string stream.
         ostringstream summary;
         summary << "Manager::wait_on_registration_of_required_objects():"
                 << __LINE__ << "\nREQUIRED-OBJECTS:" << total_required_obj_cnt
                 << "   Total-Objects:" << total_obj_cnt;

         if ( is_execution_configuration_used() ) {
            // Execution-Configuration object
            summary << "\n  " << cnt << ":Object instance '" << get_execution_configuration()->get_name() << "' ";
            cnt++;
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

         for ( n = 0; n < obj_count; n++ ) {
            // Adjust index based on sim-config or exec-config objects existing.
            summary << "\n  " << ( n + cnt ) << ":Object instance '"
                    << objects[n].get_name() << "' ";

            if ( objects[n].is_instance_handle_valid() ) {
               string id_str;
               StringUtilities::to_string(
                  id_str, objects[n].get_instance_handle() );
               summary << "(ID:" << id_str << ") ";
            }
            summary << "for class '" << objects[n].get_FOM_name() << "' is "
                    << ( objects[n].is_required() ? "REQUIRED" : "not required" )
                    << " and is "
                    << ( objects[n].is_instance_handle_valid() ? "REGISTERED" : "Not Registered" );
         }
         summary << THLA_ENDL;

         // Display the summary.
         send_hs( stdout, (char *)summary.str().c_str() );
      }

      // Determine if we have any unregistered objects.
      any_unregistered_required_obj = ( current_required_obj_cnt < total_required_obj_cnt );

      // Wait a little while to allow the objects to be registered.
      if ( any_unregistered_required_obj ) {
         (void)sleep_timer.sleep();

         // Check again to see if we have any unregistered objects.
         any_unregistered_required_obj = ( current_required_obj_cnt < total_required_obj_cnt );

         if ( any_unregistered_required_obj && sleep_timer.timeout() ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Manager::wait_on_registration_of_required_objects():" << __LINE__
                      << " Unexpectedly the Federate is no longer an execution member."
                      << " This means we are either not connected to the"
                      << " RTI or we are no longer joined to the federation"
                      << " execution because someone forced our resignation at"
                      << " the Central RTI Component (CRC) level!"
                      << THLA_ENDL;
               send_hs( stderr, (char *)errmsg.str().c_str() );
               exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            }
         }
      }
   } while ( any_unregistered_required_obj );

   if ( is_execution_configuration_used() ) {
      // Add the exec-config instance to the map if it is not already in it.
      if ( ( get_execution_configuration()->is_instance_handle_valid() ) && ( object_map.find( get_execution_configuration()->get_instance_handle() ) == object_map.end() ) ) {
         object_map[get_execution_configuration()->get_instance_handle()] = get_execution_configuration();
      }
   }

   // Add all valid, registered object instances to the map and only if they are
   // not already in it.
   for ( n = 0; n < obj_count; n++ ) {
      if ( ( objects[n].is_instance_handle_valid() ) && ( object_map.find( objects[n].get_instance_handle() ) == object_map.end() ) ) {
         object_map[objects[n].get_instance_handle()] = &objects[n];
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
      send_hs( stderr, "Manager::set_all_object_instance_handles_by_name():%d Already initialized.%c",
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
   const int data_obj_count,
   Object *  data_objects )
{
   // Just return if we are already initialized.
   if ( this->mgr_initialized ) {
      send_hs( stderr, "Manager::set_object_instance_handles_by_name():%d Already initialized.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( federate == NULL ) {
      send_hs( stderr, "Manager::set_object_instance_handles_by_name():%d Unexpected NULL Federate.%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Manager::set_object_instance_handles_by_name() Unexpected NULL Federate." );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      send_hs( stderr, "Manager::set_object_instance_handles_by_name():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Manager::set_object_instance_handles_by_name() Unexpected NULL RTIambassador." );
      return;
   }

   wstring ws_instance_name = L"";

   ostringstream summary;
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      summary << "Manager::set_object_instance_handles_by_name():"
              << __LINE__;
   }

   try {
      // Resolve all the handles/ID's for the objects and attributes.
      for ( int n = 0; n < data_obj_count; n++ ) {

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
            if ( ( data_objects[n].is_instance_handle_valid() ) && ( object_map.find( data_objects[n].get_instance_handle() ) == object_map.end() ) ) {
               object_map[data_objects[n].get_instance_handle()] = &data_objects[n];
            }

            if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
               string id_str;
               StringUtilities::to_string( id_str, data_objects[n].get_instance_handle() );
               summary << "\n    Object:'" << data_objects[n].get_name()
                       << "'  ID:" << id_str
                       << "  ID-Valid:" << ( data_objects[n].is_instance_handle_valid() ? "Yes" : "No" )
                       << "  Obj-Required:" << ( data_objects[n].is_required() ? "Yes" : "No" );
            }
         } catch ( ObjectInstanceNotKnown &e ) {
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
               send_hs( stderr, (char *)errmsg.str().c_str() );
               exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
            } else {
               if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
                  summary << "\n    Object:'" << data_objects[n].get_name()
                          << "'  ID:Inatance-Not-Known  ID-Valid:No  Obj-Required:No";
               }
            }
         }
      }
   } catch ( FederateNotExecutionMember &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Federation Not Execution Member" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } catch ( NotConnected &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Not Connected" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } catch ( RTIinternalError &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " RTIinternalError: '" << rti_err_msg << "'" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "Manager::set_object_instance_handles_by_name():" << __LINE__
             << " RTI1516_EXCEPTION for '"
             << rti_err_msg << "'" << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      summary << THLA_ENDL;
      send_hs( stdout, (char *)summary.str().c_str() );
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::provide_attribute_update(
   ObjectInstanceHandle const &theObject,
   AttributeHandleSet const &  theAttributes )
{
   // Determine which data object the user is requesting an update for.
   Object *trickhla_obj = get_trickhla_object( theObject );
   if ( trickhla_obj != NULL ) {
      trickhla_obj->provide_attribute_update( theAttributes );
   } else {
      execution_control->provide_attribute_update( theObject, theAttributes );
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void Manager::determine_job_cycle_time()
{
   if ( this->job_cycle_time > 0.0 ) {
      return;
   }

   if ( debug_handler.should_print( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::determine_job_cycle_time():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // Get the lookahead time.
   double lookahead_time = ( federate != NULL ) ? federate->get_lookahead_time() : 0.0;

   this->job_cycle_time = exec_get_job_cycle( NULL );

   // Verify the job cycle time against the HLA lookahead time.
   if ( ( this->job_cycle_time <= 0.0 ) || ( this->job_cycle_time < lookahead_time ) ) {
      ostringstream errmsg;
      errmsg << "Manager::determine_job_cycle_time():" << __LINE__
             << " ERROR: The cycle time for this job is less than the HLA"
             << " lookahead time! The HLA Lookahead time (" << lookahead_time
             << " seconds) must be less than or equal to the job cycle time ("
             << this->job_cycle_time << " seconds). Make sure 'lookahead_time' in"
             << " your input or modified-data file is less than or equal to the"
             << " 'THLA_DATA_CYCLE_TIME' time specified in the S_define file for"
             << " the send_cyclic_and_requested_data() and"
             << " receive_cyclic_data() jobs." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // Set the core job cycle time now that we know what it is so that the
   // attribute cyclic ratios can now be calculated for any multi-rate
   // attributes.
   for ( int n = 0; n < this->obj_count; ++n ) {
      objects[n].set_core_job_cycle_time( this->job_cycle_time );
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::send_cyclic_and_requested_data()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::send_cyclic_and_requested_data():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   double current_sim_time = exec_get_sim_time();

   // Determine the cycle time for this job if it is not yet known.
   if ( this->job_cycle_time <= 0.0 ) {
      determine_job_cycle_time();
   }

   // Send any ExecutionControl data requested.
   execution_control->send_requested_data( current_sim_time, this->job_cycle_time );

   // Send data to remote RTI federates for each of the objects.
   for ( int n = 0; n < this->obj_count; ++n ) {
      objects[n].send_cyclic_and_requested_data( current_sim_time, this->job_cycle_time );
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
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::receive_cyclic_data():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   double current_sim_time = exec_get_sim_time();

   // Receive and process and updates for ExecutionControl.
   execution_control->receive_cyclic_data( current_sim_time );

   // Determine the cycle time for this job if it is not yet known.
   if ( this->job_cycle_time <= 0.0 ) {
      determine_job_cycle_time();
   }

   // Receive data from remote RTI federates for each of the objects.
   for ( int n = 0; n < obj_count; n++ ) {
      objects[n].receive_cyclic_data( current_sim_time, this->job_cycle_time );
   }

   return;
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

   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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
            if ( interaction_item->index >= 0 && interaction_item->index < inter_count && interactions[interaction_item->index].is_subscribe() ) {

               interactions[interaction_item->index].extract_data( interaction_item );

               interactions[interaction_item->index].process_interaction();
            }
            break;
         }

         default: {
            ostringstream msg;
            msg << "Manager::process_interactions():" << __LINE__
                << "FATAL ERROR: encountered an invalid interaction type: "
                << interaction_item->interaction_type
                << ". Verify that you are specifying the correct interaction "
                << "type defined in 'ManagerTypeOfInteractionEnum' enum "
                << "found in 'Manager.hh' and re-run." << THLA_ENDL;
            send_hs( stderr, (char *)msg.str().c_str() );
            exec_terminate( __FILE__, (char *)msg.str().c_str() );
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
   InteractionClassHandle const & theInteraction,
   ParameterHandleValueMap const &theParameterValues,
   RTI1516_USERDATA const &       theUserSuppliedTag,
   LogicalTime const &            theTime,
   bool                           received_as_TSO )
{
   // Find the Interaction we have data for.
   for ( int i = 0; i < inter_count; i++ ) {

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

         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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
   execution_control->receive_interaction( theInteraction,
                                           theParameterValues,
                                           theUserSuppliedTag,
                                           theTime,
                                           received_as_TSO );

   return;
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
   for ( int n = 0; n < obj_count; n++ ) {
      StringUtilities::to_wstring( ws_obj_name, objects[n].get_name() );
      if ( ws_obj_name == obj_instance_name ) {
         return ( &objects[n] );
      }
   }

   // Check for a match with the ExecutionConfiguration object associated with
   // ExecutionControl.  Returns NULL if match not found.
   return ( execution_control->get_trickhla_object( obj_instance_name ) );
}

/*!
 * @job_class{scheduled}
 */
bool Manager::discover_object_instance(
   ObjectInstanceHandle theObject,
   ObjectClassHandle    theObjectClass,
   wstring const &      theObjectInstanceName )
{
   bool return_value = false;

   // Get the unregistered TrickHLA Object for the given class handle and
   // object instance name.
   Object *trickhla_obj = get_unregistered_object( theObjectClass, theObjectInstanceName );

   // If we did not find the object by class handle and instance name then
   // get the first unregistered object that is remotely owned for the given
   // object class type.
   if ( trickhla_obj == NULL ) {

      // Get the first unregistered remotely owned object that has the
      // given object class type.
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

      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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

      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, theObject );
         StringUtilities::to_string( name_str, theObjectInstanceName );
         send_hs( stdout, "Manager::discover_object_instance():%d Discovered MOM HLA-Federate Object-Instance-ID:%s Name:'%s'%c",
                  __LINE__, id_str.c_str(), name_str.c_str(), THLA_NEWLINE );
      }
   } else if ( ( federate != NULL ) && federate->is_MOM_HLAfederation_class( theObjectClass ) ) {

      federate->add_MOM_HLAfederation_instance_id( theObject );
      return_value = true;

      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
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
   wstring const &          theObjectInstanceName )
{
   wstring ws_obj_name;

   // Search the simulation data objects first.
   for ( int n = 0; n < obj_count; n++ ) {

      // Find the object that is not registered (i.e. the instance ID == 0),
      // has the same class handle as the one specified, and has the same name
      // as the object instance name that is specified.
      if ( ( objects[n].get_class_handle() == theObjectClass ) && ( !objects[n].is_instance_handle_valid() ) ) {

         StringUtilities::to_wstring( ws_obj_name, objects[n].get_name() );

         // Determine if the name matches the object instance name.
         if ( ws_obj_name == theObjectInstanceName ) {
            return ( &objects[n] );
         }
      }
   }

   // Check for a match with the ExecutionConfiguration object associated with
   // ExecutionControl.  Returns NULL if match not found.
   return ( execution_control->get_unregistered_object( theObjectClass, theObjectInstanceName ) );
}

/*!
 * @job_class{scheduled}
 */
Object *Manager::get_unregistered_remote_object(
   ObjectClassHandle const &theObjectClass )
{
   // Search the simulation data objects first.
   for ( int n = 0; n < obj_count; n++ ) {

      // Return the first TrickHLA object that we did not create an HLA
      // instance for, has the same class handle as the one specified, is not
      // registered (i.e. the instance ID == 0), and does not have an Object
      // Instance Name associated with it, and a name is not required or the
      // user did not specify one.
      if ( ( !objects[n].is_create_HLA_instance() ) && ( objects[n].get_class_handle() == theObjectClass ) && ( !objects[n].is_instance_handle_valid() ) && ( !objects[n].is_name_required() || ( objects[n].get_name() == NULL ) || ( *( objects[n].get_name() ) == '\0' ) ) ) {
         return ( &objects[n] );
      }
   }

   // Check for a match with the ExecutionConfiguration object associated with
   // ExecutionControl.  Returns NULL if match not found.
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
         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            string id_str;
            StringUtilities::to_string( id_str, instance_id );
            send_hs( stderr, "Manager::mark_object_as_deleted_from_federation():%d Object '%s' Instance-ID:%s Valid-ID:%s %c",
                     __LINE__, obj->get_name(), id_str.c_str(),
                     ( instance_id.isValid() ? "Yes" : "No" ), THLA_NEWLINE );
         }
         obj->remove_object_instance();
      }
   }

   return;
}

/*!
 * @job_class{logging}
 */
void Manager::process_deleted_objects()
{

   // Process ExecutionControl deletions.
   execution_control->process_deleted_objects();

   // Search the simulation data objects, looking for deleted objects.
   for ( int n = 0; n < obj_count; n++ ) {
      if ( objects[n].is_object_deleted_from_RTI() ) {
         objects[n].process_deleted_object();
      }
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void Manager::pull_ownership()
{
   for ( int n = 0; n < obj_count; n++ ) {
      objects[n].pull_ownership();
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::push_ownership()
{
   for ( int n = 0; n < obj_count; n++ ) {
      objects[n].push_ownership();
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::grant_pull_request()
{
   for ( int n = 0; n < obj_count; n++ ) {
      objects[n].grant_pull_request();
   }
}

/*!
 * @job_class{scheduled}
 */
void Manager::release_ownership()
{
   for ( int n = 0; n < obj_count; n++ ) {
      objects[n].release_ownership();
   }
}

double Manager::get_granted_time() const
{
   return ( federate != NULL ) ? federate->get_granted_time() : 0.0;
}

/*!
 * @details If the federate does not exist, -1.0 seconds is assigned to the
 * returned object.
 */
Int64Interval Manager::get_fed_lookahead() const
{
   Int64Interval di;
   if ( federate != NULL ) {
      di = federate->get_lookahead();
   } else {
      di = Int64Interval( -1.0 );
   }
   return di;
}

/*!
 * @details If the federate does not exist, MAX_LOGICAL_TIME_SECONDS is
 * assigned to the returned object.
 */
Int64Time Manager::get_granted_fed_time() const
{
   Int64Time dt;
   if ( federate != NULL ) {
      dt = federate->get_granted_fed_time();
   } else {
      dt = Int64Time( MAX_LOGICAL_TIME_SECONDS );
   }
   return dt;
}

bool Manager::is_RTI_ready(
   const char *method_name )
{
   if ( federate == NULL ) {
      send_hs( stderr, "Manager::%s:%d Unexpected NULL Federate.%c",
               method_name, __LINE__, THLA_NEWLINE );
      return false;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   bool rti_valid = true;
   if ( get_RTI_ambassador() == NULL ) {
      send_hs( stderr, "Manager::%s:%d Unexpected NULL RTIambassador.%c",
               method_name, __LINE__, THLA_NEWLINE );
      rti_valid = false;
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return rti_valid;
}

/*!
 * @details Trigger federation save, at current time or user-specified time...\n
 * NOTE: These routines do not coordinate a federation save via interactions
 * so make these internal routines so that the user does not accidentally call
 * them and mess things up.
 */
void Manager::initiate_federation_save(
   const char *file_name )
{
   federate->set_checkpoint_file_name( file_name );
   federate->initiate_save_announce();

   this->initiated_a_federation_save = true;
}

void Manager::start_federation_save(
   const char *file_name )
{
   start_federation_save_at_scenario_time( -DBL_MAX, file_name );
}

void Manager::start_federation_save_at_sim_time(
   double      freeze_sim_time,
   const char *file_name )
{
   start_federation_save_at_scenario_time(
      execution_control->convert_sim_time_to_scenario_time( freeze_sim_time ),
      file_name );

   return;
}

void Manager::start_federation_save_at_scenario_time(
   double      freeze_scenario_time,
   const char *file_name )
{

   // Call the ExecutionControl method.
   execution_control->start_federation_save_at_scenario_time( freeze_scenario_time, file_name );

   return;
}

/*!
 * @job_class{initialization}
 */
void Manager::setup_checkpoint()
{

   // Call the ExecutionControl method.
   execution_control->setup_checkpoint();

   for ( int i = 0; i < obj_count; i++ ) {
      // Any object with a valid instance handle must be marked as required
      // to ensure the restore process will wait for this object instance
      // to exist.
      if ( objects[i].is_instance_handle_valid() ) {
         objects[i].mark_required();
      }
      // Setup the ownership handler checkpoint data structures.
      objects[i].setup_ownership_transfer_checkpointed_data();
   }

   setup_checkpoint_interactions();
}

void Manager::setup_checkpoint_interactions()
{
   // Clear the checkpoint for the interactions so that we don't lead memory.
   clear_interactions();

   if ( !interactions_queue.empty() ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::setup_checkpoint_interactions():%d interactions_queue.size()=%d%c",
                  __LINE__, interactions_queue.size(), THLA_NEWLINE );
      }

      check_interactions_count = interactions_queue.size();
      check_interactions       = reinterpret_cast< InteractionItem * >(
         alloc_type( check_interactions_count, "TrickHLA::InteractionItem" ) );
      if ( check_interactions == static_cast< InteractionItem * >( NULL ) ) {
         ostringstream msg;
         msg << "Manager::setup_checkpoint_interactions():" << __LINE__
             << " Failed to allocate enough memory for a check_interactions"
             << " linear array of " << check_interactions_count << " elements"
             << THLA_ENDL;
         send_hs( stderr, (char *)msg.str().c_str() );
         exec_terminate( __FILE__, (char *)msg.str().c_str() );
      }

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &interactions_queue.mutex );

      //interactions_queue.dump_head_pointers("interactions_queue.dump");

      for ( int i = 0; i < interactions_queue.size(); i++ ) {

         InteractionItem *item =
            static_cast< InteractionItem * >( interactions_queue.front() );

         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::setup_checkpoint_interactions():%d \
checkpointing into check_interactions[%d] from interaction index %d...%c",
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

      // auto_unlock_mutex unlocks the mutex lock here
   }
}

void Manager::clear_interactions()
{
   if ( check_interactions_count > 0 ) {
      for ( int i = 0; i < check_interactions_count; i++ ) {
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
      for ( int i = 0; i < check_interactions_count; i++ ) {
         msg << "check_interactions[" << i << "].index                  = "
             << check_interactions[i].index << endl
             << "check_interactions[" << i << "].interaction_type       = '"
             << check_interactions[i].interaction_type << "'" << endl
             << "check_interactions[" << i << "].parm_items_count       = "
             << check_interactions[i].parm_items_count
             << endl;
         for ( int j = 0; j < check_interactions[i].parm_items_count; j++ ) {
            msg << "check_interactions[" << i << "].parm_items[" << j << "].index    = "
                << check_interactions[i].parm_items[j].index << endl
                << "check_interactions[" << i << "].parm_items[" << j << "].size     = "
                << check_interactions[i].parm_items[j].size
                << endl;
         }
         msg << "check_interactions[" << i << "].user_supplied_tag_size = "
             << check_interactions[i].user_supplied_tag_size << endl
             << "check_interactions[" << i << "].order_is_TSO           = "
             << check_interactions[i].order_is_TSO << endl
             << "check_interactions[" << i << "].time                   = "
             << check_interactions[i].time.get_time_in_micros()
             << endl;
      }
      send_hs( stdout, (char *)msg.str().c_str() );
   }
}

void Manager::restore_interactions()
{
   if ( check_interactions_count > 0 ) {
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::restore_interactions():%d check_interactions_count=%d%c",
                  __LINE__, check_interactions_count, THLA_NEWLINE );
      }

      for ( int i = 0; i < check_interactions_count; i++ ) {

         InteractionItem *item = new InteractionItem();

         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::restore_interactions():%d \
restoring check_interactions[%d] into interaction index %d...parm_count=%d%c",
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
   for ( int n = 0; n < obj_count; n++ ) {
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
void Manager::wait_on_discovery_of_objects()
{
   if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
      send_hs( stdout, "Manager::wait_on_discovery_of_object_instance():%d%c",
               __LINE__, THLA_NEWLINE );
   }

   // do we have Simulation object(s) to interrogate?
   if ( obj_count > 0 ) {

      // see if any object discoveries have occurred...
      int  required_count                   = 0;
      int  discovery_count                  = 0;
      bool create_HLA_instance_object_found = false;
      for ( int n = 0; n < obj_count; n++ ) {
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

      // if all of the required objects were discovered, exit immediately.
      if ( discovery_count == required_count ) {
         return;
      }

      // figure out how many objects have been discovered so far...
      if ( ( !create_HLA_instance_object_found && // still missing some objects other than
             ( discovery_count < ( required_count - 1 ) ) )
           ||                                           // the one for the rejoining federate, or
           ( create_HLA_instance_object_found &&        // missing some other object(s) but
             ( discovery_count < required_count ) ) ) { // found the rejoining federate

         if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
            send_hs( stdout, "Manager::wait_on_discovery_of_object_instance():%d \
- blocking loop until object discovery callbacks arrive.%c",
                     __LINE__, THLA_NEWLINE );
         }

         SleepTimeout sleep_timer( 10.0, 1000 );

         // block until some / all arrive.
         do {

            // Check for shutdown.
            federate->check_for_shutdown_with_termination();

            // Sleep for a little while to allow the RTI to trigger the object
            // discovery callbacks.
            (void)sleep_timer.sleep();

            if ( sleep_timer.timeout() ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Manager::wait_on_discovery_of_object_instance():" << __LINE__
                         << " Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!"
                         << THLA_ENDL;
                  send_hs( stderr, (char *)errmsg.str().c_str() );
                  exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
               }
            }

            // Check if any objects were discovered while we were napping.
            discovery_count                  = 0;
            create_HLA_instance_object_found = false;
            for ( int n = 0; n < obj_count; n++ ) {
               if ( objects[n].is_required() && objects[n].is_instance_handle_valid() ) {
                  discovery_count++;
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
      if ( debug_handler.should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_MANAGER ) ) {
         send_hs( stdout, "Manager::wait_on_discovery_of_object_instance():%d - No Objects to discover.%c",
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
   for ( int n = 0; n < obj_count; n++ ) {
      // Was the required 'create_HLA_instance' object found?
      if ( objects[n].is_required() && objects[n].is_create_HLA_instance() && objects[n].is_instance_handle_valid() ) {

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
