/*!
@file TrickHLA/ObjectServices.cpp
@ingroup TrickHLA
@brief This class manages the HLA Object Services.

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
@trick_link_dependency{Interaction.cpp}
@trick_link_dependency{InteractionItem.cpp}
@trick_link_dependency{ObjectServices.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{Parameter.cpp}
@trick_link_dependency{ParameterItem.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{time/Int64BaseTime.cpp}
@trick_link_dependency{time/Int64Interval.cpp}
@trick_link_dependency{time/Int64Time.cpp}
@trick_link_dependency{time/TimeManagementServices.cpp}
@trick_link_dependency{utils/MutexLock.cpp}
@trick_link_dependency{utils/MutexProtection.cpp}
@trick_link_dependency{utils/SleepTimeout.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2006, --, DSES Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, April 2026, --, Refactored from old Manager class.}
@revs_end

*/

// System includes.
#include <climits>
#include <cstdint>
#include <cstring>
#include <map>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"
#include "trick/sim_mode.h"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionConfigurationBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/ObjectServices.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64BaseTime.hh"
#include "TrickHLA/time/Int64Interval.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/time/TimeManagementServices.hh"
#include "TrickHLA/utils/MutexProtection.hh"
#include "TrickHLA/utils/SleepTimeout.hh"
#include "TrickHLA/utils/StringUtilities.hh"
#include "TrickHLA/utils/Utilities.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Exception.h"
#include "RTI/Handle.h"
#include "RTI/RTIambassador.h"
#include "RTI/Typedefs.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
ObjectServices::ObjectServices( Federate &fed )
   : obj_count( 0 ),
     objects( NULL ),
     obj_discovery_mutex(),
     object_map(),
     obj_name_index_map(),
     federate( &fed )
{
   return;
}

/*!
 * @details Frees the Trick allocated memory.
 * @job_class{shutdown}
 */
ObjectServices::~ObjectServices()
{
   object_map.clear();
   obj_name_index_map.clear();

   // Make sure we destroy the mutex.
   obj_discovery_mutex.destroy();
}

void ObjectServices::initialize_HLA_cycle_time()
{
   // Set the core job cycle time now that we know what it is so that the
   // attribute cyclic ratios can now be calculated for any multi-rate
   // attributes.
   // TODO: Use child thread cycle rate for core cycle time?
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].set_core_job_cycle_time( federate->time_management_service.get_HLA_cycle_time() );
   }
}

/*! @brief Verify the user specified object and interaction arrays and counts. */
void ObjectServices::verify_object_arrays()
{
   // Check for the error condition of a valid object count but a null
   // objects array.
   if ( ( obj_count > 0 ) && ( objects == NULL ) ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::verify_object_arrays():" << __LINE__
             << " ERROR: Unexpected NULL 'objects' array for a non zero"
             << " obj_count:" << obj_count << ". Please check your input or"
             << " modified-data files to make sure the 'ObjectServices::objects'"
             << " array is correctly configured.\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // If we have a non-NULL objects array but the object-count is invalid
   // then let the user know.
   if ( ( obj_count <= 0 ) && ( objects != NULL ) ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::verify_object_arrays():" << __LINE__
             << " ERROR: Unexpected " << ( ( obj_count == 0 ) ? "zero" : "negative" )
             << " obj_count:" << obj_count << " for a non-NULL 'objects' array."
             << " Please check your input or modified-data files to make sure"
             << " the 'ObjectServices::objects' array is correctly configured.\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   if ( obj_count >= INT_MAX ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::verify_object_arrays():" << __LINE__
             << " ERROR: Unexpected obj_count:" << obj_count << " >= " << INT_MAX
             << ". Please check your input or modified-data files to make sure"
             << " the 'ObjectServices::objects' array is correctly configured.\n";
      DebugHandler::terminate( errmsg.str() );
   }

   // Reset the TrickHLA Object count if negative.
   if ( obj_count < 0 ) {
      obj_count = 0;
   }

   // Object instance names must be unique and can not be a duplicate.
   for ( int n = 0; n < obj_count; ++n ) {
      if ( !objects[n].get_name().empty() ) {

         for ( int k = n + 1; k < obj_count; ++k ) {
            if ( !objects[k].get_name().empty() ) {

               if ( objects[n].get_name() == objects[k].get_name() ) {
                  ostringstream errmsg;
                  errmsg << "ObjectServices::verify_object_arrays():" << __LINE__
                         << " ERROR: Object instance '" << objects[n].get_name()
                         << "' at array index " << n << " has the same name as"
                         << " object instance '" << objects[k].get_name()
                         << "' at array index " << k << ". Please check your"
                         << " input or modified-data files to make sure the"
                         << " object instance names are unique with no duplicates.\n";
                  DebugHandler::terminate( errmsg.str() );
                  return;
               }
            }
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::send_init_data()
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( federate->is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::send_init_data():%d Late joining \
federate so this call will be ignored.\n",
                          __LINE__ );
      }
      return;
   }

   // Go through the list of objects.
   for ( int n = 0; n < obj_count; ++n ) {
      // Make sure we have at least one piece of object init data we can send.
      if ( objects[n].any_locally_owned_published_init_attribute() ) {

         if ( federate->execution_control->wait_for_init_data() ) {

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
               message_publish( MSG_NORMAL, "ObjectServices::send_init_data():%d '%s'\n",
                                __LINE__, objects[n].get_name().c_str() );
            }

            // Send the object init data to the other federates.
            objects[n].send_init_data();

         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
               ostringstream msg;
               msg << "ObjectServices::send_init_data():" << __LINE__
                   << " '" << objects[n].name << "'"
                   << " WARNING: This call will be ignored because the"
                   << " Simulation Initialization Scheme (Type:'"
                   << federate->execution_control->get_type()
                   << "') does not support it.\n";
               message_publish( MSG_WARNING, msg.str().c_str() );
            }
         }
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            message_publish( MSG_NORMAL, "ObjectServices::send_init_data():%d Nothing to send for '%s'\n",
                             __LINE__, objects[n].get_name().c_str() );
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::send_init_data(
   string const &instance_name )
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( federate->is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::send_init_data():%d Late joining \
federate so the data will not be sent for '%s'.\n",
                          __LINE__, instance_name.c_str() );
      }
      return;
   }

   if ( instance_name.empty() ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::send_init_data():" << __LINE__
             << " ERROR: Empty Object Instance Name\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   wstring obj_instance_name;
   StringUtilities::to_wstring( obj_instance_name, instance_name );

   Object *obj = get_trickhla_object( obj_instance_name );

   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::send_init_data():" << __LINE__
             << " ERROR: The specified Object Instance"
             << " Name '" << instance_name << "' does not correspond to any"
             << " known object. Please check your S_define file or simulation"
             << " module to verify the settings.\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Make sure we have at least one piece of object init data we can send.
   if ( obj->any_locally_owned_published_init_attribute() ) {

      if ( federate->execution_control->wait_for_init_data() ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            message_publish( MSG_NORMAL, "ObjectServices::send_init_data():%d '%s'\n",
                             __LINE__, instance_name.c_str() );
         }

         // Send the object init data to the other federates.
         obj->send_init_data();

      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            ostringstream msg;
            msg << "ObjectServices::send_init_data():" << __LINE__
                << " '" << instance_name << "'"
                << " WARNING: This call will be ignored because the"
                << " Simulation Initialization Scheme (Type:'"
                << federate->execution_control->get_type()
                << "') does not support it.\n";
            message_publish( MSG_WARNING, msg.str().c_str() );
         }
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::send_init_data():%d Nothing to send for '%s'\n",
                          __LINE__, instance_name.c_str() );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::receive_init_data()
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( federate->is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Late joining \
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
         bool const obj_required = objects[n].is_required() && ( federate->execution_control->wait_for_init_data() );

         if ( obj_required ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
               message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Waiting for '%s', and marked as %s.\n",
                                __LINE__, objects[n].get_name().c_str(),
                                ( objects[n].is_required() ? "REQUIRED" : "not required" ) );
            }

            SleepTimeout print_timer;
            SleepTimeout sleep_timer;

            // Wait for the data to arrive.
            while ( !objects[n].is_changed() ) {

               // Check for shutdown.
               federate->check_for_shutdown_with_termination();

               sleep_timer.sleep();

               if ( !objects[n].is_changed() ) {

                  // To be more efficient, we get the time once and share it.
                  int64_t const wallclock_time = sleep_timer.time();

                  if ( sleep_timer.timeout( wallclock_time ) ) {
                     sleep_timer.reset();
                     if ( !federate->is_execution_member() ) {
                        ostringstream errmsg;
                        errmsg << "ObjectServices::receive_init_data():" << __LINE__
                               << " ERROR: Unexpectedly the Federate is no longer an execution member."
                               << " This means we are either not connected to the"
                               << " RTI or we are no longer joined to the federation"
                               << " execution because someone forced our resignation at"
                               << " the Central RTI Component (CRC) level!\n";
                        DebugHandler::terminate( errmsg.str() );
                        return;
                     }
                  }

                  if ( print_timer.timeout( wallclock_time ) ) {
                     print_timer.reset();
                     message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Waiting for '%s', and marked as %s.\n",
                                      __LINE__, objects[n].get_name().c_str(),
                                      ( objects[n].is_required() ? "REQUIRED" : "not required" ) );
                  }
               }
            }
         }

         // Check for changed data which means we received something.
         if ( objects[n].is_changed() ) {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
               message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Received '%s'\n",
                                __LINE__, objects[n].get_name().c_str() );
            }

            // Receive the data from the publishing federate.
            objects[n].receive_init_data();
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
               message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Received nothing for '%s', and marked as %s.\n",
                                __LINE__, objects[n].get_name().c_str(),
                                ( obj_required ? "REQUIRED" : "not required" ) );
            }
         }
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Nothing to receive for '%s'\n",
                             __LINE__, objects[n].get_name().c_str() );
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::receive_init_data(
   string const &instance_name )
{
   // Late joining federates do not get to participate in the multiphase
   // initialization process so just return.
   if ( federate->is_late_joining_federate() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Late joining federate so skipping data for '%s'\n",
                          __LINE__, instance_name.c_str() );
      }
      return;
   }

   if ( instance_name.empty() ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::receive_init_data():" << __LINE__
             << " ERROR: Empty Object Instance Name";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   wstring obj_instance_name;
   StringUtilities::to_wstring( obj_instance_name, instance_name );

   Object *obj = get_trickhla_object( obj_instance_name );

   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::receive_init_data():" << __LINE__
             << " ERROR: The specified Object Instance Name '" << instance_name
             << "' does not correspond to any known object. Please check your"
             << " S_define file or simulation module to verify the settings.\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Make sure we have at least one piece of data we can receive.
   if ( obj->any_remotely_owned_subscribed_init_attribute() ) {

      // Only wait for REQUIRED received init data and do not block waiting
      // to receive init data if we are using the simple init scheme.
      bool const obj_required = obj->is_required() && federate->execution_control->wait_for_init_data();

      if ( obj_required ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Waiting for '%s', and marked as %s.\n",
                             __LINE__, instance_name.c_str(),
                             ( obj->is_required() ? "REQUIRED" : "not required" ) );
         }

         SleepTimeout print_timer;
         SleepTimeout sleep_timer;

         // Wait for the data to arrive.
         while ( !obj->is_changed() ) {

            // Check for shutdown.
            federate->check_for_shutdown_with_termination();

            sleep_timer.sleep();

            if ( !obj->is_changed() ) {

               // To be more efficient, we get the time once and share it.
               int64_t const wallclock_time = sleep_timer.time();

               if ( sleep_timer.timeout( wallclock_time ) ) {
                  sleep_timer.reset();
                  if ( !federate->is_execution_member() ) {
                     ostringstream errmsg;
                     errmsg << "ObjectServices::receive_init_data():" << __LINE__
                            << " ERROR: Unexpectedly the Federate is no longer an execution member."
                            << " This means we are either not connected to the"
                            << " RTI or we are no longer joined to the federation"
                            << " execution because someone forced our resignation at"
                            << " the Central RTI Component (CRC) level!\n";
                     DebugHandler::terminate( errmsg.str() );
                     return;
                  }
               }

               if ( print_timer.timeout( wallclock_time ) ) {
                  print_timer.reset();
                  message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Waiting for '%s', and marked as %s.\n",
                                   __LINE__, instance_name.c_str(),
                                   ( obj->is_required() ? "REQUIRED" : "not required" ) );
               }
            }
         }
      }

      // Check for changed data which means we received something.
      if ( obj->is_changed() ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            message_publish( MSG_NORMAL,
                             "ObjectServices::receive_init_data():%d Received '%s'\n",
                             __LINE__, instance_name.c_str() );
         }

         // Receive the data from the publishing federate.
         obj->receive_init_data();
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Received nothing for '%s', and marked as %s.\n",
                             __LINE__, instance_name.c_str(),
                             ( obj_required ? "REQUIRED" : "not required" ) );
         }
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::receive_init_data():%d Nothing to receive for '%s'\n",
                          __LINE__, instance_name.c_str() );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::request_data_update(
   wstring const &instance_name )
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      string name_str;
      StringUtilities::to_string( name_str, instance_name );
      message_publish( MSG_NORMAL, "ObjectServices::request_data_update():%d Object:'%s'\n",
                       __LINE__, name_str.c_str() );
   }

   bool found = false;

   // First check to see if asking for and ExecutionConfiguration update.
   if ( federate->is_execution_configuration_used() ) {
      wstring ws_exec_config_name;
      StringUtilities::to_wstring( ws_exec_config_name,
                                   federate->get_execution_configuration()->get_name() );
      if ( instance_name == ws_exec_config_name ) {
         found = true;
         federate->get_execution_configuration()->request_attribute_value_update();
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
void ObjectServices::request_data_update(
   string const &instance_name )
{
   wstring ws_obj_instance_name;
   StringUtilities::to_wstring( ws_obj_instance_name, instance_name );

   return request_data_update( ws_obj_instance_name );
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::object_instance_name_reservation_succeeded(
   wstring const &obj_instance_name )
{

   // If the object instance isn't recognized by ExecutionControl, then
   // handle it here.
   if ( !federate->execution_control->object_instance_name_reservation_succeeded( obj_instance_name ) ) {

      Object *trickhla_obj = get_trickhla_object( obj_instance_name );
      if ( trickhla_obj != NULL ) {
         trickhla_obj->set_name_registered();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            message_publish( MSG_NORMAL, "ObjectServices::object_instance_name_reservation_succeeded():%d \
RESERVED Object Instance Name '%s'\n",
                             __LINE__, trickhla_obj->get_name().c_str() );
         }
      }
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::object_instance_name_reservation_failed(
   wstring const &obj_instance_name )
{

   // Different ExecutionControl mechanisms will handle object instance name
   // failure differently. So, check with the ExecutionControl to perform
   // any specialized failure handling. If the method returns 'true' then
   // it's not a fatal error; otherwise, continue with error handling and
   // exit.
   if ( federate->execution_control->object_instance_name_reservation_failed( obj_instance_name ) ) {
      return;
   }

   string name_str;
   StringUtilities::to_string( name_str, obj_instance_name );

   // Anything beyond this point is fatal.
   message_publish( MSG_WARNING, "ObjectServices::object_instance_name_reservation_failed():%d \
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
            message_publish( MSG_WARNING, "ObjectServices::object_instance_name_reservation_failed():%d\
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
   errmsg << "ObjectServices::object_instance_name_reservation_failed():" << __LINE__
          << " Exiting...\n";
   DebugHandler::terminate( errmsg.str() );
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::add_object_to_map(
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
void ObjectServices::setup_object_ref_attributes()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::setup_object_ref_attributes():%d\n",
                       __LINE__ );
   }

   // Create the map of object instance names to object array indexes.
   obj_name_index_map.clear();
   for ( size_t index = 0; index < (size_t)obj_count; ++index ) {
      obj_name_index_map[objects[index].get_name()] = index;
   }

   // Make sure the object-map is empty/clear before we continue.
   object_map.clear();

   if ( federate->is_execution_configuration_used() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::setup_object_ref_attributes():%d Execution-Configuration\n",
                          __LINE__ );
      }
      setup_object_ref_attributes( 1, federate->get_execution_configuration() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::setup_object_ref_attributes():%d Objects: %d\n",
                       __LINE__, obj_count );
   }
   setup_object_ref_attributes( obj_count, objects );
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::setup_object_ref_attributes(
   int const data_obj_count,
   Object   *data_objects )
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::setup_object_ref_attributes():%d\n",
                       __LINE__ );
   }

   // Resolve all the Ref-Attributes for all the simulation initialization
   // objects and attributes.
   for ( int n = 0; n < data_obj_count; ++n ) {

      // Initialize the TrickHLA-Object before we use it.
      data_objects[n].initialize( this->federate );

      ostringstream msg;

      int const  attr_count = data_objects[n].get_attribute_count();
      Attribute *attrs      = data_objects[n].get_attributes();

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         msg << "ObjectServices::setup_object_ref_attributes()" << __LINE__ << "\n"
             << "--------------- Trick REF-Attributes ---------------\n"
             << " Object:'" << data_objects[n].get_name() << "'"
             << " FOM-Name:'" << data_objects[n].get_FOM_name() << "'"
             << " Create HLA Instance:"
             << ( data_objects[n].is_create_HLA_instance() ? "Yes" : "No" )
             << " Attribute count:" << attr_count << "\n";
      }

      // Process the attributes for this object.
      for ( int i = 0; i < attr_count; ++i ) {

         // Initialize the TrickHLA-Attribute before we use it.
         attrs[i].initialize( data_objects[n].get_FOM_name().c_str(), n, i );

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            msg << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attrs[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attrs[i].get_trick_name() << "'\n";
         }
      }

      if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, msg.str().c_str() );
      }
   }
}

void ObjectServices::setup_object_RTI_handles()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::setup_object_RTI_handles():%d\n",
                       __LINE__ );
   }

   // Set up the object RTI handles for the ExecutionControl mechanisms.
   federate->execution_control->setup_object_RTI_handles();

   // Set up the object RTI handles for the simulation data objects.
   setup_object_RTI_handles( obj_count, objects );
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::setup_object_RTI_handles(
   int const data_obj_count,
   Object   *data_objects )
{
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = federate->get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::setup_object_RTI_handles():%d\n",
                       __LINE__ );
   }

   string obj_FOM_name  = "";
   string attr_FOM_name = "";
   int    FOM_name_type = 0; // 0:N/A 1:Object 2:Attribute - What name are we dealing with.

   // Initialize the Object and Attribute RTI handles.
   try {
      wstring ws_FOM_name = L"";

      // Resolve all the handles/ID's for the objects and attributes.
      for ( int n = 0; n < data_obj_count; ++n ) {
         ostringstream msg;

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            msg << "ObjectServices::setup_object_RTI_handles():" << __LINE__ << "\n"
                << "----------------- RTI Handles (Objects & Attributes) ---------------"
                << "\n"
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

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            string handle_str;
            StringUtilities::to_string( handle_str, data_objects[n].get_class_handle() );
            msg << "  Result for"
                << " Object:'" << data_objects[n].get_name() << "'"
                << " FOM-Name:'" << data_objects[n].get_FOM_name() << "'"
                << " Class-ID:" << handle_str << "\n";
         }

         int const  attr_count = data_objects[n].get_attribute_count();
         Attribute *attrs      = data_objects[n].get_attributes();

         // Resolve the handles/ID's for the attributes.
         for ( int i = 0; i < attr_count; ++i ) {

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
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

            if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
               string id_str;
               StringUtilities::to_string( id_str, attrs[i].get_attribute_handle() );
               msg << "\t  Result for Attribute '"
                   << data_objects[n].get_FOM_name() << "'->'"
                   << attrs[i].get_FOM_name() << "'"
                   << " Attribute-ID:" << id_str << "\n";
            }
         }

         // Make sure we build the attribute map now that the AttributeHandles
         // have been set.
         data_objects[n].build_attribute_map();

         if ( DebugHandler::show( DEBUG_LEVEL_9_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
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
            errmsg << "ObjectServices::setup_object_RTI_handles():" << __LINE__
                   << " ERROR: Object FOM Name '" << obj_FOM_name << "' Not Found. Please check"
                   << " your input or modified-data files to make sure the"
                   << " Object FOM Name is correctly specified.\n";
            DebugHandler::terminate( errmsg.str() );
            break;
         }
         case 2: { // Attribute
            ostringstream errmsg;
            errmsg << "ObjectServices::setup_object_RTI_handles():" << __LINE__
                   << " ERROR: For Object FOM Name '" << obj_FOM_name << "', Attribute FOM Name '"
                   << attr_FOM_name << "' Not Found. Please check your input or"
                   << " modified-data files to make sure the Object Attribute"
                   << " FOM Name is correctly specified.\n";
            DebugHandler::terminate( errmsg.str() );
            break;
         }
         default: { // FOM name we are working with is unknown.
            ostringstream errmsg;
            errmsg << "ObjectServices::setup_object_RTI_handles():" << __LINE__
                   << " ERROR: Object or Attribute FOM Name Not Found. Please check your input or"
                   << " modified-data files to make sure the FOM Name is"
                   << " correctly specified.\n";
            DebugHandler::terminate( errmsg.str() );
            break;
         }
      }
   } catch ( FederateNotExecutionMember const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "ObjectServices::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Federate Not Execution Member\n";
      DebugHandler::terminate( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "ObjectServices::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Not Connected\n";
      DebugHandler::terminate( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "ObjectServices::setup_object_RTI_handles():" << __LINE__
             << " ERROR: RTIinternalError: '"
             << rti_err_msg << "'\n";
      DebugHandler::terminate( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "ObjectServices::setup_object_RTI_handles():" << __LINE__
             << " ERROR: Exception for '"
             << rti_err_msg << "'\n";
      DebugHandler::terminate( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::publish()
{
   if ( !federate->is_RTI_ready( "publish" ) ) {
      return;
   }

   // Publish attributes for all the Trick-HLA-Objects we know about.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].publish_object_attributes();
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::unpublish()
{
   if ( !federate->is_RTI_ready( "unpublish" ) ) {
      return;
   }

   // Unpublish from all attributes for all the objects.
   for ( int i = 0; i < obj_count; ++i ) {
      // Only unpublish an object class if we had published at least
      // one attribute.
      if ( objects[i].any_attribute_published() ) {
         bool do_unpublish = true;
         for ( int k = 0; ( k < i ) && do_unpublish; ++k ) {
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
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::subscribe()
{
   if ( !federate->is_RTI_ready( "subscribe" ) ) {
      return;
   }

   // Subscribe to attributes for all the Trick-HLA-Objects we know about.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].subscribe_to_object_attributes();
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::unsubscribe()
{
   if ( !federate->is_RTI_ready( "unsubscribe" ) ) {
      return;
   }

   // Unsubscribe from all attributes for all the objects.
   for ( int i = 0; i < obj_count; ++i ) {
      // Only unsubscribe from an object class if we had subscribed to at
      // least one attribute.
      if ( objects[i].any_attribute_subscribed() ) {
         bool do_unsubscribe = true;
         for ( int k = 0; ( k < i ) && do_unsubscribe; ++k ) {
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
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::reserve_object_names_with_RTI()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::reserve_object_names_with_RTI():%d\n",
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
void ObjectServices::wait_for_reservation_of_object_names()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::wait_for_reservation_of_object_names():%d\n",
                       __LINE__ );
   }

   // Simulation object names.
   if ( obj_count > 0 ) {
      // Wait for each RTI object instance name to be registered with the RTI,
      // but for only the names we requested registration for.
      for ( int n = 0; n < obj_count; ++n ) {
         objects[n].wait_for_object_name_reservation();
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::wait_for_reservation_of_object_names():%d All Object instance names reserved.\n",
                          __LINE__ );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::wait_for_reservation_of_object_names():%d No Object instance names to reserve.\n",
                          __LINE__ );
      }
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::register_objects_with_RTI()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::register_objects_with_RTI():%d\n",
                       __LINE__ );
   }

   // Have the ExecutionControl register objects it needs with the RTI.
   federate->execution_control->register_objects_with_RTI();

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
void ObjectServices::setup_preferred_order_with_RTI()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::setup_preferred_order_with_RTI():%d\n",
                       __LINE__ );
   }

   if ( federate->is_execution_configuration_used() ) {
      // Register the execution configuration object.
      federate->get_execution_configuration()->setup_preferred_order_with_RTI();
   }

   // Setup the preferred order for all the object attributes.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].setup_preferred_order_with_RTI();
   }
}

/*!
 * @details Calling this function will block until all the required object
 * instances in the Federation have been registered.
 * @job_class{initialization}
 */
void ObjectServices::wait_for_registration_of_required_objects()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::wait_for_registration_of_required_objects():%d\n",
                       __LINE__ );
   }

   int  current_registered_obj_cnt  = 0;
   int  total_obj_cnt               = obj_count;
   int  current_required_obj_cnt    = 0;
   int  total_required_obj_cnt      = 0;
   bool print_summary               = DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES );
   bool print_only_unregistered_obj = false;
   bool any_unregistered_required_obj;

   if ( federate->is_execution_configuration_used() ) {
      // Make sure to count the exec-config object.
      ++total_obj_cnt;

      // Determine if the Execution-Configuration object is required and it should be.
      if ( federate->get_execution_configuration()->is_required() ) {
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

   SleepTimeout print_timer;
   SleepTimeout sleep_timer;

   do {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      // Data objects.
      if ( current_required_obj_cnt < total_required_obj_cnt ) {
         int required_obj_cnt   = 0;
         int registered_obj_cnt = 0;

         // Concurrency critical code section for discovered objects being set
         // in FedAmb callback.
         {
            // When auto_unlock_mutex goes out of scope it automatically unlocks
            // the mutex even if there is an exception.
            MutexProtection const auto_unlock_mutex( &obj_discovery_mutex );

            if ( federate->is_execution_configuration_used() ) {
               // Determine if the Execution-Configuration object has been
               // registered and only if it is required.
               if ( federate->get_execution_configuration()->is_instance_handle_valid() ) {
                  ++registered_obj_cnt;
                  if ( federate->get_execution_configuration()->is_required() ) {
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
               print_summary = DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES );
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
         summary << "ObjectServices::wait_for_registration_of_required_objects():"
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
            MutexProtection const auto_unlock_mutex( &obj_discovery_mutex );

            int cnt = 1;
            if ( federate->is_execution_configuration_used() ) {
               if ( !print_only_unregistered_obj
                    || !federate->get_execution_configuration()->is_instance_handle_valid() ) {

                  // Execution-Configuration object
                  summary << "\n  " << cnt << ":Object instance '" << federate->get_execution_configuration()->get_name() << "' ";

                  if ( federate->get_execution_configuration()->is_instance_handle_valid() ) {
                     string id_str;
                     StringUtilities::to_string( id_str, federate->get_execution_configuration()->get_instance_handle() );
                     summary << "(ID:" << id_str << ") ";
                  }
                  summary << "for class '" << federate->get_execution_configuration()->get_FOM_name() << "' is "
                          << ( federate->get_execution_configuration()->is_required() ? "REQUIRED" : "not required" )
                          << " and is "
                          << ( federate->get_execution_configuration()->is_instance_handle_valid() ? "REGISTERED" : "Not Registered" );
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
         summary << "\n";

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
         any_unregistered_required_obj = ( current_required_obj_cnt < total_required_obj_cnt ); // cppcheck-suppress [knownConditionTrueFalse,redundantAssignment]

         if ( any_unregistered_required_obj ) { // cppcheck-suppress [knownConditionTrueFalse,unmatchedSuppression]

            // To be more efficient, we get the time once and share it.
            int64_t const wallclock_time = sleep_timer.time();

            // If we timeout check to see if we are still an execution member.
            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();

               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "ObjectServices::wait_for_registration_of_required_objects():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution"
                         << " member. This means we are either not connected to"
                         << " the RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation"
                         << " at the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate( errmsg.str() );
                  return;
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
      MutexProtection const auto_unlock_mutex( &obj_discovery_mutex );

      if ( federate->is_execution_configuration_used() ) {
         // Add the exec-config instance to the map if it is not already in it.
         if ( ( federate->get_execution_configuration()->is_instance_handle_valid() )
              && ( object_map.find( federate->get_execution_configuration()->get_instance_handle() ) == object_map.end() ) ) {
            object_map[federate->get_execution_configuration()->get_instance_handle()] = federate->get_execution_configuration();
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
void ObjectServices::set_all_object_instance_handles_by_name()
{
   // Clear the map since we are going to rebuild it from the function
   // calls below.
   object_map.clear();

   if ( federate->is_execution_configuration_used() ) {
      // Execution Configuration object.
      set_object_instance_handles_by_name( 1, federate->get_execution_configuration() );
   }

   // Simulation data objects.
   set_object_instance_handles_by_name( obj_count, objects );
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::set_object_instance_handles_by_name(
   int const data_obj_count,
   Object   *data_objects )
{
   if ( this->federate == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Unexpected NULL 'federate' pointer!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = federate->get_RTI_ambassador();
   if ( rti_amb == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Unexpected NULL RTIambassador!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   ostringstream summary;
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      summary << "ObjectServices::set_object_instance_handles_by_name():"
              << __LINE__;
   }

   try {
      wstring ws_instance_name = L"";

      // Resolve all the handles/ID's for the objects and attributes.
      for ( int n = 0; n < data_obj_count; ++n ) {

         // Create the wide-string version of the instance name.
         string const instance_name = data_objects[n].get_name();
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

            if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
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
               errmsg << "ObjectServices::set_object_instance_handles_by_name():" << __LINE__
                      << " ERROR: Object Instance Not Known for '"
                      << instance_name << "'\n";
               DebugHandler::terminate( errmsg.str() );
               return;
            } else {
               if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
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
      errmsg << "ObjectServices::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Federation Not Execution Member\n";
      DebugHandler::terminate( errmsg.str() );
   } catch ( NotConnected const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      ostringstream errmsg;
      errmsg << "ObjectServices::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Not Connected\n";
      DebugHandler::terminate( errmsg.str() );
   } catch ( RTIinternalError const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "ObjectServices::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: RTIinternalError: '" << rti_err_msg << "'\n";
      DebugHandler::terminate( errmsg.str() );
   } catch ( RTI1516_NAMESPACE::Exception const &e ) {
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      ostringstream errmsg;
      errmsg << "ObjectServices::set_object_instance_handles_by_name():" << __LINE__
             << " ERROR: Exception for '"
             << rti_err_msg << "'\n";
      DebugHandler::terminate( errmsg.str() );
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      summary << "\n";
      message_publish( MSG_NORMAL, summary.str().c_str() );
   }
}

/*!
 * @job_class{scheduled}
 */
void ObjectServices::provide_attribute_update(
   ObjectInstanceHandle const &theObject,
   AttributeHandleSet const   &theAttributes )
{
   // Determine which data object the user is requesting an update for.
   Object *trickhla_obj = get_trickhla_object( theObject );
   if ( trickhla_obj != NULL ) {
      trickhla_obj->provide_attribute_update( theAttributes );
   } else {
      federate->execution_control->provide_attribute_update( theObject, theAttributes );
   }
}

/*!
 * @job_class{scheduled}
 */
void ObjectServices::send_cyclic_and_requested_data()
{
   // Current time values.
   int64_t const sim_time_in_base_time = Int64BaseTime::to_base_time( exec_get_sim_time() );
   int64_t const granted_base_time     = federate->get_granted_time().get_base_time();
   int64_t const lookahead_base_time   = federate->time_management_service.is_zero_lookahead_time()
                                            ? 0LL
                                            : federate->get_lookahead().get_base_time();

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
   int64_t   dt      = federate->time_management_service.get_HLA_cycle_time_in_base_time();
   int64_t   prev_dt = dt;
   Int64Time update_time( granted_base_time + dt );

   // Make sure the update time is not less than the granted time + lookahead,
   // which happens if the delta-time step is less than the lookahead time.
   if ( dt < lookahead_base_time ) {
      update_time.set( granted_base_time + lookahead_base_time );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::send_cyclic_and_requested_data():%d HLA-time:%.12G seconds.\n",
                       __LINE__, update_time.get_time_in_seconds() );
   }

   // Send any ExecutionControl data requested.
   federate->execution_control->send_requested_data( update_time );

   // Send data to remote RTI federates for each of the objects.
   for ( int obj_index = 0; obj_index < this->obj_count; ++obj_index ) {

      // Only send data if we are on the data cycle time boundary for this object.
      if ( federate->time_management_service.on_receive_data_cycle_boundary_for_obj( obj_index, sim_time_in_base_time ) ) {

         // Get the cyclic data time for the object.
         dt = federate->time_management_service.get_data_cycle_base_time_for_obj( obj_index, federate->time_management_service.get_HLA_cycle_time_in_base_time() );

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
void ObjectServices::receive_cyclic_data()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::receive_cyclic_data():%d\n", __LINE__ );
   }

   int64_t const sim_time_in_base_time = Int64BaseTime::to_base_time( exec_get_sim_time() );

   // Receive and process any updates for ExecutionControl.
   federate->execution_control->receive_cyclic_data();

   // Receive data from remote RTI federates for each of the objects.
   for ( int n = 0; n < obj_count; ++n ) {

      // Only receive data if we are on the data cycle time boundary for this object.
      if ( federate->time_management_service.on_receive_data_cycle_boundary_for_obj( n, sim_time_in_base_time ) ) {
         objects[n].receive_cyclic_data();
      }
   }
}

/*!
 * @job_class{scheduled}
 */
Object *ObjectServices::get_trickhla_object(
   ObjectInstanceHandle const &instance_id )
{
   // We use a map with the key being the ObjectIntanceHandle for fast lookups.
   ObjectInstanceMap::const_iterator const iter = object_map.find( instance_id );
   return ( ( iter != object_map.end() ) ? iter->second : NULL );
}

/*!
 * @job_class{scheduled}
 */
Object *ObjectServices::get_trickhla_object(
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
   return ( federate->execution_control->get_trickhla_object( obj_instance_name ) );
}

/*!
 * @job_class{scheduled}
 */
Object *ObjectServices::get_trickhla_object(
   wstring const &obj_instance_name )
{
   string obj_instance_name_str;
   StringUtilities::to_string( obj_instance_name_str, obj_instance_name );

   return ( get_trickhla_object( obj_instance_name_str ) );
}

/*!
 * @job_class{scheduled}
 */
bool ObjectServices::discover_object_instance(
   ObjectInstanceHandle const &theObject,
   ObjectClassHandle const    &theObjectClass,
   wstring const              &theObjectInstanceName )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &obj_discovery_mutex );

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

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         string id_str;
         StringUtilities::to_string( id_str, theObject );
         message_publish( MSG_NORMAL, "ObjectServices::discover_object_instance():%d Data-Object '%s' Instance-ID:%s\n",
                          __LINE__, trickhla_obj->get_name().c_str(), id_str.c_str() );
      }

   } else if ( ( federate != NULL ) && federate->is_MOM_HLAfederate_class( theObjectClass ) ) {

      // Add this to the list of joined federates.
      federate->add_joined_federate( theObject, theObjectInstanceName );
      return_value = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         string id_str;
         StringUtilities::to_string( id_str, theObject );
         string name_str;
         StringUtilities::to_string( name_str, theObjectInstanceName );
         message_publish( MSG_NORMAL, "ObjectServices::discover_object_instance():%d Discovered MOM HLA-Federate Object-Instance-ID:%s Name:'%s'\n",
                          __LINE__, id_str.c_str(), name_str.c_str() );
      }

   } else if ( ( federate != NULL ) && federate->is_MOM_HLAfederation_class( theObjectClass ) ) {

      federate->add_MOM_HLAfederation_instance_handle( theObject );
      return_value = true;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         string id_str;
         StringUtilities::to_string( id_str, theObject );
         string name_str;
         StringUtilities::to_string( name_str, theObjectInstanceName );
         message_publish( MSG_NORMAL, "ObjectServices::discover_object_instance():%d MOM HLA-Federation '%s' Instance-ID:%s\n",
                          __LINE__, name_str.c_str(), id_str.c_str() );
      }
   }

   return return_value;
}

/*!
 * @job_class{scheduled}
 */
Object *ObjectServices::get_unregistered_object(
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
   return ( federate->execution_control->get_unregistered_object( theObjectClass, theObjectInstanceName ) );
}

/*!
 * @job_class{scheduled}
 */
Object *ObjectServices::get_unregistered_remote_object(
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
           && ( !objects[n].is_name_required() || objects[n].get_name().empty() ) ) {
         return ( &objects[n] );
      }
   }

   // Check for a match with the ExecutionConfiguration object associated with
   // ExecutionControl. Returns NULL if match not found.
   return ( federate->execution_control->get_unregistered_remote_object( theObjectClass ) );
}

/*!
 * @job_class{scheduled}
 */
void ObjectServices::process_ownership()
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

void ObjectServices::mark_object_as_deleted_from_federation(
   ObjectInstanceHandle const &instance_id )
{

   // First check if this is associated with ExecutionControl.
   // If so, then perform any ExecutionControl specific actions.
   // If not, then just remove the object instance.
   if ( !federate->execution_control->mark_object_as_deleted_from_federation( instance_id ) ) {

      Object *obj = get_trickhla_object( instance_id );
      if ( obj != NULL ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            string id_str;
            StringUtilities::to_string( id_str, instance_id );
            message_publish( MSG_NORMAL, "ObjectServices::mark_object_as_deleted_from_federation():%d Object '%s' Instance-ID:%s Valid-ID:%s\n",
                             __LINE__, obj->get_name().c_str(), id_str.c_str(),
                             ( instance_id.isValid() ? "Yes" : "No" ) );
         }
         obj->remove_object_instance();
      }
   }
}

/*!
 * @job_class{logging}
 */
void ObjectServices::process_deleted_objects()
{
   // Process ExecutionControl deletions.
   federate->execution_control->process_deleted_objects();

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
void ObjectServices::pull_ownership()
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
void ObjectServices::pull_ownership_at_init(
   string const &obj_instance_name )
{
   pull_ownership_at_init( obj_instance_name, "" );
}

/*!
 * @brief Blocking function call to pull ownership of the named object
 * instance at initialization.
 * @param obj_instance_name Object instance name to pull ownership
 * of for all attributes
 * @param attribute_list Comma separated list of attributes.
 */
void ObjectServices::pull_ownership_at_init(
   string const &obj_instance_name,
   string const &attribute_list )
{
   if ( obj_instance_name.empty() ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::pull_ownership_at_init():" << __LINE__
             << " ERROR: Unexpected empty obj_instance_name specified!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   Object *obj = get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::pull_ownership_at_init():" << __LINE__
             << " ERROR: Failed to find object with instance name: '"
             << obj_instance_name << "'!\n";
      DebugHandler::terminate( errmsg.str() );
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
void ObjectServices::handle_pulled_ownership_at_init(
   string const &obj_instance_name )
{
   if ( obj_instance_name.empty() ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::handle_pulled_ownership_at_init():" << __LINE__
             << " ERROR: Unexpected empty obj_instance_name specified!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   Object *obj = get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::handle_pulled_ownership_at_init():" << __LINE__
             << " ERROR: Failed to find object with instance name: '"
             << obj_instance_name << "'!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   obj->handle_pulled_ownership_at_init();
}

/*!
 * @job_class{scheduled}
 */
void ObjectServices::pull_ownership_upon_rejoin()
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
void ObjectServices::push_ownership()
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
void ObjectServices::push_ownership_at_init(
   string const &obj_instance_name )
{
   push_ownership_at_init( obj_instance_name, "" );
}

/*!
 * @brief Blocking function call to push ownership of the named object
 * instance at initialization.
 * @param obj_instance_name Object instance name to push ownership
 * of for all attributes.
 * @param attribute_list Comma separated list of attribute FOM names.
 */
void ObjectServices::push_ownership_at_init(
   string const &obj_instance_name,
   string const &attribute_list )
{
   if ( obj_instance_name.empty() ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::push_ownership_at_init():" << __LINE__
             << " ERROR: Unexpected empty obj_instance_name specified!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   Object *obj = get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::push_ownership_at_init():" << __LINE__
             << " ERROR: Failed to find object with instance name: '"
             << obj_instance_name << "'!\n";
      DebugHandler::terminate( errmsg.str() );
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
void ObjectServices::handle_pushed_ownership_at_init(
   string const &obj_instance_name )
{
   if ( obj_instance_name.empty() ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::handle_pushed_ownership_at_init():" << __LINE__
             << " ERROR: Unexpected empty obj_instance_name specified!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   Object *obj = get_trickhla_object( obj_instance_name );
   if ( obj == NULL ) {
      ostringstream errmsg;
      errmsg << "ObjectServices::handle_pushed_ownership_at_init():" << __LINE__
             << " ERROR: Failed to find object with instance name: '"
             << obj_instance_name << "'!\n";
      DebugHandler::terminate( errmsg.str() );
      return;
   }

   obj->handle_pushed_ownership_at_init();
}

/*!
 * @job_class{scheduled}
 */
void ObjectServices::grant_pull_request()
{
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].grant_pull_request();
   }
}

/*!
 * @job_class{scheduled}
 */
void ObjectServices::release_ownership()
{
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].release_ownership();
   }
}

/*!
 * @job_class{initialization}
 */
void ObjectServices::convert_data_before_checkpoint()
{
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].convert_data_before_checkpoint();
   }
}

void ObjectServices::restore_data_after_checkpoint()
{
   // Restore the data structures of this class from the Trick checkpoint.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].restore_data_after_checkpoint();
   }
}

void ObjectServices::free_converted_data_for_checkpoint()
{
   // Clear/release the memory used for the checkpoint data structures.
   for ( int n = 0; n < obj_count; ++n ) {
      objects[n].free_converted_data_for_checkpoint();
   }
}

/*!
 * @details Calling this function will block until object instances have been
 * discovered.
 * @job_class{initialization}
 */
void ObjectServices::wait_for_discovery_of_objects()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
      message_publish( MSG_NORMAL, "ObjectServices::wait_for_discovery_of_object_instance():%d\n",
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

         if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
            message_publish( MSG_NORMAL, "ObjectServices::wait_for_discovery_of_object_instance():%d Waiting for object discovery callbacks to arrive.\n",
                             __LINE__ );
         }

         int64_t      wallclock_time; // cppcheck-suppress [variableScope,unmatchedSuppression]
         SleepTimeout print_timer;
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
                  errmsg << "ObjectServices::wait_for_discovery_of_object_instance():" << __LINE__
                         << " ERROR: Unexpectedly the Federate is no longer an execution member."
                         << " This means we are either not connected to the"
                         << " RTI or we are no longer joined to the federation"
                         << " execution because someone forced our resignation at"
                         << " the Central RTI Component (CRC) level!\n";
                  DebugHandler::terminate( errmsg.str() );
                  return;
               }
            }

            if ( print_timer.timeout( wallclock_time ) ) {
               print_timer.reset();
               message_publish( MSG_NORMAL, "ObjectServices::wait_for_discovery_of_object_instance():%d Waiting...\n",
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
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_OBJ_SERVICES ) ) {
         message_publish( MSG_NORMAL, "ObjectServices::wait_for_discovery_of_object_instance():%d - No Objects to discover.\n",
                          __LINE__ );
      }
   }
}
