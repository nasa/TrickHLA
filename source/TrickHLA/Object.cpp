/*!
@file TrickHLA/Object.cpp
@ingroup TrickHLA
@brief This class represents an HLA object that is managed by Trick.

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
@trick_link_dependency{Conditional.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ElapsedTimeStats.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64BaseTime.cpp}
@trick_link_dependency{Int64Interval.cpp}
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{LagCompensation.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{ObjectDeleted.cpp}
@trick_link_dependency{OwnershipHandler.cpp}
@trick_link_dependency{Packing.cpp}
@trick_link_dependency{SleepTimeout.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2006, --, DSES Created Object}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <pthread.h>
#include <sstream>
#include <string>
#include <vector>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/release.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Conditional.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ElapsedTimeStats.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/Int64Interval.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/LagCompensation.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/ObjectDeleted.hh"
#include "TrickHLA/OwnershipHandler.hh"
#include "TrickHLA/Packing.hh"
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

//-----------------------------------------------------------------
// Enable/disable (verbose) debug printing
//-----------------------------------------------------------------
#define THLA_OBJ_DEBUG 0
#define THLA_OBJ_DEBUG_SEND 0
#define THLA_OBJ_DEBUG_RECEIVE 0
#define THLA_OBJ_OWNERSHIP_DEBUG 0

//-----------------------------------------------------------------
// Enable/disable debug printing when the object is valid but we do
// not have a data update for it...
//-----------------------------------------------------------------
#define THLA_OBJ_DEBUG_VALID_OBJECT_RECEIVE 0

/*!
 * @job_class{initialization}
 */
Object::Object()
   : data_changed( false ),
     name( NULL ),
     name_required( true ),
     FOM_name( NULL ),
     create_HLA_instance( false ),
     required( true ),
     blocking_cyclic_read( false ),
     thread_ids( NULL ),
     attr_count( 0 ),
     attributes( NULL ),
     lag_comp( NULL ),
     lag_comp_type( LAG_COMPENSATION_NONE ),
     packing( NULL ),
     ownership( NULL ),
     deleted( NULL ),
     conditional( NULL ),
     thread_ids_array_count( 0 ),
     thread_ids_array( NULL ),
     process_object_deleted_from_RTI( false ),
     object_deleted_from_RTI( false ),
     push_mutex(),
     ownership_mutex(),
     send_mutex(),
     receive_mutex(),
     clock(),
     name_registered( false ),
     changed( false ),
     attr_update_requested( false ),
     removed_instance( false ),
     first_blocking_cyclic_read( true ),
     any_attribute_FOM_specified_order( false ),
     any_attribute_timestamp_order( false ),
     pull_requested( false ),
     divest_requested( false ),
     ownership_acquired( false ),
     attribute_FOM_names(),
     manager( NULL ),
     rti_ambassador( NULL ),
     thla_reflected_attributes_queue(),
     thla_attribute_map(),
     send_count( 0LL ),
     receive_count( 0LL ),
     elapsed_time_stats()
{
   // Make sure we allocate the map.
   this->attribute_values_map = new AttributeHandleValueMap();
}

/*!
 * @details Frees memory allocated, and remove this object from the federation
 * execution.
 * @job_class{shutdown}
 */
Object::~Object()
{
   if ( !removed_instance ) {
      // Make sure we switch to unblocking cyclic reads so that we let any
      // blocking threads go.
      set_to_unblocking_cyclic_reads();

      // Remove this object from the federation execution.
      remove();

      if ( name != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( name ) ) ) {
            message_publish( MSG_WARNING, "Object::~Object():%d WARNING failed to delete Trick Memory for 'name'\n",
                             __LINE__ );
         }
         name = NULL;
      }

      if ( this->thread_ids_array != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( this->thread_ids_array ) ) ) {
            message_publish( MSG_WARNING, "Object::~Object():%d WARNING failed to delete Trick Memory for 'this->thread_ids_array'\n",
                             __LINE__ );
         }
         this->thread_ids_array       = NULL;
         this->thread_ids_array_count = 0;
      }

      // FIXME: There is a problem with deleting attribute_values_map?
      if ( attribute_values_map != NULL ) {
         if ( !attribute_values_map->empty() ) {
            attribute_values_map->clear();
         }
         delete attribute_values_map;
         attribute_values_map = NULL;
      }

      thla_attribute_map.clear();

      // Make sure we destroy the mutexs.
      push_mutex.destroy();
      ownership_mutex.destroy();
      send_mutex.destroy();
      receive_mutex.destroy();

      removed_instance = true;
   }
}

/*!
 * @job_class{initialization}
 */
void Object::initialize(
   Manager *trickhla_mgr )
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   if ( trickhla_mgr == NULL ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA-Manager!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   this->manager = trickhla_mgr;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      ostringstream msg;
      msg << "Object::initialize():" << __LINE__
          << " Name:'" << name << "' FOM_name:'" << FOM_name
          << "' create_HLA_instance:"
          << ( is_create_HLA_instance() ? "True" : "False" ) << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   // Make sure we have a valid object instance name if the user has indicated
   // that we are creating the HLA instance or if the instance name is required.
   if ( ( is_create_HLA_instance() || is_name_required() )
        && ( ( name == NULL ) || ( *name == '\0' ) ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: Missing Object Name. Please check your input or modified-data"
             << " files to make sure the object name is correctly specified."
             << " A valid object instance name is required if you are creating"
             << " an HLA instance of this object (i.e. 'create_HLA_instance'"
             << " field is set to true) or if the 'name_required' field is set"
             << " to true, which is the default.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   } else if ( name == NULL ) {
      // Make sure the name is at least not NULL.
      set_name( "" );
   }

   // Make sure we have a valid object FOM name.
   if ( ( FOM_name == NULL ) || ( *FOM_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: Object '" << name << "' is missing the Object FOM Name."
             << " Please check your input or modified-data files to make sure"
             << " the object FOM name is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Do a bounds check on the 'lag_comp_type' value.
   if ( ( lag_comp_type < LAG_COMPENSATION_FIRST_VALUE ) || ( lag_comp_type > LAG_COMPENSATION_LAST_VALUE ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "', the Lag-Compensation Type"
             << " setting 'lag_comp_type' has a value that is out of the valid"
             << " range of " << LAG_COMPENSATION_FIRST_VALUE << " to "
             << LAG_COMPENSATION_LAST_VALUE << ". Please check your input"
             << " or modified-data files to make sure the 'lag_comp_type' value"
             << " is correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Make sure we have a lag compensation object if lag-compensation is specified.
   if ( ( lag_comp_type != LAG_COMPENSATION_NONE ) && ( lag_comp == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "', Lag-Compensation 'lag_comp_type'"
             << " is specified, but 'lag_comp' is NULL! Please check your input"
             << " or modified-data files to make sure the Lag-Compensation type"
             << " and object are correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have an attribute count but no attributes then let the user know.
   if ( ( attr_count > 0 ) && ( attributes == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "', the 'attr_count' is "
             << attr_count << " but no 'attributes' are"
             << " specified. Please check your input or modified-data files to"
             << " make sure the attributes are correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If we have attributes but the attribute-count is invalid then let
   // the user know.
   if ( ( attr_count <= 0 ) && ( attributes != NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "', the 'attr_count' is "
             << attr_count << " but 'attributes' have been"
             << " specified. Please check your input or modified-data files to"
             << " make sure the attributes are correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If the user specified a packing object then make sure it extends the
   // Packing virtual class.
   if ( ( packing != NULL ) && ( dynamic_cast< Packing * >( packing ) == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "',"
             << " the 'packing' setting does not point to a class that"
             << " extends the Packing class. Please check your input"
             << " or modified-data files to make sure the attributes are"
             << " correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If the user specified ownership handler object then make sure it extends
   // the OwnershipHandler virtual class.
   if ( ( ownership != NULL ) && ( dynamic_cast< OwnershipHandler * >( ownership ) == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "',"
             << " the 'ownership' setting does not point to a class that"
             << " extends the OwnershipHandler class. Please check"
             << " your input or modified-data files to make sure the"
             << " attributes are correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If the user specified a resignation identification object then make sure
   // it extends the ObjectDeleted virtual class.
   if ( ( deleted != NULL ) && ( dynamic_cast< ObjectDeleted * >( deleted ) == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "', the 'deleted' setting does not"
             << " point to a class that extends the ObjectDeleted"
             << " class. Please check your input or modified-data files to make"
             << " sure the attributes are correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Reset the TrickHLA Attributes count if it is negative or if there
   // are no attributes.
   if ( ( this->attr_count < 0 ) || ( attributes == NULL ) ) {
      this->attr_count = 0;
   }

   // Check for the case where attributes are a mix of Zero Lookahead and Cyclic
   // since this can result in deadlock.
   bool any_cyclic_attr         = false;
   bool any_zero_lookahead_attr = false;
   bool any_blocking_io_attr    = false;
   for ( int i = 0; i < attr_count; ++i ) {
      if ( ( attributes[i].get_configuration() & CONFIG_CYCLIC ) == CONFIG_CYCLIC ) {
         any_cyclic_attr = true;
      }
      if ( ( attributes[i].get_configuration() & CONFIG_ZERO_LOOKAHEAD ) == CONFIG_ZERO_LOOKAHEAD ) {
         any_zero_lookahead_attr = true;
      }
      if ( ( attributes[i].get_configuration() & CONFIG_BLOCKING_IO ) == CONFIG_BLOCKING_IO ) {
         any_blocking_io_attr = true;
      }
      if ( any_cyclic_attr && any_zero_lookahead_attr ) {
         ostringstream errmsg;
         errmsg << "Object::initialize():" << __LINE__
                << " ERROR: For object '" << name << "', detected Attributes"
                << " with a mix of CONFIG_CYCLIC and CONFIG_ZERO_LOOKAHEAD for"
                << " the 'config' setting, which can lead to deadlock. Please"
                << " configure all the Attributes of this object to use one of"
                << " CONFIG_CYCLIC, CONFIG_ZERO_LOOKAHEAD or CONFIG_BLOCKING_IO"
                << " for the Attribute 'config' setting.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      if ( any_cyclic_attr && any_blocking_io_attr ) {
         ostringstream errmsg;
         errmsg << "Object::initialize():" << __LINE__
                << " ERROR: For object '" << name << "', detected Attributes"
                << " with a mix of CONFIG_CYCLIC and CONFIG_BLOCKING_IO for"
                << " the 'config' setting, which can lead to deadlock. Please"
                << " configure all the Attributes of this object to use one of"
                << " CONFIG_CYCLIC, CONFIG_ZERO_LOOKAHEAD or CONFIG_BLOCKING_IO"
                << " for the Attribute 'config' setting.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      if ( any_zero_lookahead_attr && any_blocking_io_attr ) {
         ostringstream errmsg;
         errmsg << "Object::initialize():" << __LINE__
                << " ERROR: For object '" << name << "', detected Attributes"
                << " with a mix of CONFIG_ZERO_LOOKAHEAD and CONFIG_BLOCKING_IO for"
                << " the 'config' setting, which can lead to deadlock. Please"
                << " configure all the Attributes of this object to use one of"
                << " CONFIG_CYCLIC, CONFIG_ZERO_LOOKAHEAD or CONFIG_BLOCKING_IO"
                << " for the Attribute 'config' setting.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }

   // If any attribute is configured for zero-lookahead then lag compensation
   // cannot be specified because zero-lookahead avoids any latency in the data
   // transfer (i.e. intra-frame).
   if ( any_zero_lookahead_attr && ( lag_comp != NULL ) && ( lag_comp_type != LAG_COMPENSATION_NONE ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "', detected Attributes"
             << " with a 'config' setting of CONFIG_ZERO_LOOKAHEAD but a"
             << " Lag-Compensation 'lag_comp' callback has also been specified"
             << " with a 'lag_comp_type' setting that is not LAG_COMPENSATION_NONE!"
             << " Please check your input or modified-data files to make sure"
             << " the Lag-Compensation type 'lag_comp_type' is set to"
             << " LAG_COMPENSATION_NONE to disable Lag-Compensation when using"
             << " zero-lookahead configured object attributes.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If any attribute is configured for blocking I/O then lag compensation
   // cannot be specified because blocking I/O avoids any latency in the data
   // transfer (i.e. intra-frame).
   if ( any_blocking_io_attr && ( lag_comp != NULL ) && ( lag_comp_type != LAG_COMPENSATION_NONE ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "', detected Attributes"
             << " with a 'config' setting of CONFIG_BLOCKING_IO but a"
             << " Lag-Compensation 'lag_comp' callback has also been specified"
             << " with a 'lag_comp_type' setting that is not LAG_COMPENSATION_NONE!"
             << " Please check your input or modified-data files to make sure"
             << " the Lag-Compensation type 'lag_comp_type' is set to"
             << " LAG_COMPENSATION_NONE to disable Lag-Compensation when using"
             << " blocking I/O configured object attributes.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // If any attribute is configured for zero-lookahead then the federate must
   // also be configured for a lookahead time of zero.
   if ( any_zero_lookahead_attr && !get_federate()->is_zero_lookahead_time() ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "', detected Attributes"
             << " with a 'config' setting of CONFIG_ZERO_LOOKAHEAD but the"
             << " federate has been configured with a non-zero lookahead time of "
             << get_lookahead().get_time_in_seconds() << " seconds ("
             << get_lookahead().get_base_time() << " "
             << Int64BaseTime::get_units() << "). The lookahead time must be"
             << " set to zero to support zero-lookahead data exchanges, which"
             << " is what this object is configured for.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // TODO: Get the preferred order by parsing the FOM.
   //
   // Determine if any attribute is the FOM specified order.
   this->any_attribute_FOM_specified_order = false;
   for ( int i = 0; !any_attribute_FOM_specified_order && i < attr_count; ++i ) {
      if ( attributes[i].get_preferred_order() == TRANSPORT_SPECIFIED_IN_FOM ) {
         this->any_attribute_FOM_specified_order = true;
      }
   }

   // TODO: Get the preferred order by parsing the FOM.
   //
   // Determine if any attribute is Timestamp Order.
   this->any_attribute_timestamp_order = false;
   for ( int i = 0; !any_attribute_timestamp_order && i < attr_count; ++i ) {
      if ( attributes[i].get_preferred_order() == TRANSPORT_TIMESTAMP_ORDER ) {
         this->any_attribute_timestamp_order = true;
      }
   }

   // Build the string array of valid attribute FOM names and also check for
   // duplicate attribute FOM names.
   for ( int i = 0; i < attr_count; ++i ) {
      // Validate the FOM-name to make sure we don't have a problem with the
      // list of names as well as get a difficult to debug runtime error for
      // the string constructor if we had a null FOM-name.
      if ( ( attributes[i].get_FOM_name() == NULL ) || ( *( attributes[i].get_FOM_name() ) == '\0' ) ) {
         ostringstream errmsg;
         errmsg << "Object::initialize():" << __LINE__
                << " ERROR: Object '" << name << "' has a missing Attribute"
                << " FOM Name at array index " << i << ". Please check your input"
                << " or modified-data files to make sure the object attribute"
                << " FOM name is correctly specified.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      string fom_name_str( attributes[i].get_FOM_name() );

      // Since Object updates are sent as a AttributeHandleValueMap there can be
      // no duplicate Attributes because the map only allows unique AttributeHandles.
      for ( int k = i + 1; k < attr_count; ++k ) {
         if ( ( attributes[k].get_FOM_name() != NULL ) && ( *( attributes[k].get_FOM_name() ) != '\0' ) ) {

            if ( fom_name_str == string( attributes[k].get_FOM_name() ) ) {
               ostringstream errmsg;
               errmsg << "Object::initialize():" << __LINE__
                      << " ERROR: Object '" << name << "' has Attributes at"
                      << " array indexes " << i << " and " << k
                      << " that have the same FOM Name '" << fom_name_str
                      << "'. Please check your input or modified-data files to"
                      << " make sure the object attributes do not use duplicate"
                      << " FOM names.\n";
               DebugHandler::terminate_with_message( errmsg.str() );
            }
         }
      }

      // Add the unique attribute FOM name.
      attribute_FOM_names.push_back( fom_name_str );
   }

   // If the user specified a lag_comp object then make sure it extends the
   // LagCompensation virtual class.
   if ( ( lag_comp != NULL ) && ( dynamic_cast< LagCompensation * >( lag_comp ) == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " ERROR: For object '" << name << "', the 'lag_comp' setting does not"
             << " point to a class that extends the LagCompensation"
             << " class. Please check your input or modified-data files to make"
             << " sure the attributes are correctly specified.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Initialize the Packing handler.
   if ( packing != NULL ) {
      packing->initialize_callback( this );
   }

   // Initialize the Lag-Compensation handler.
   if ( lag_comp != NULL ) {
      lag_comp->initialize_callback( this );
   }

   // Initialize the Conditional handler.
   if ( conditional != NULL ) {
      conditional->initialize_callback( this );
   }

   // Initialize the Ownership handler.
   if ( ownership != NULL ) {
      ownership->initialize_callback( this );
   }

   // Initialize the deleted object instance handler.
   if ( deleted != NULL ) {
      deleted->initialize_callback( this );
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

Federate *Object::get_federate()
{
   return ( ( this->manager != NULL ) ? manager->get_federate() : NULL );
}

RTIambassador *Object::get_RTI_ambassador()
{
   if ( rti_ambassador == NULL ) {
      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Get the Trick-Federate.
      Federate *federate = get_federate();

      // Get the RTI-Ambassador.
      rti_ambassador = ( federate != NULL ) ? federate->get_RTI_ambassador() : NULL;

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
   return ( rti_ambassador );
}

/*!
 * @details Called from the virtual destructor.
 * @job_class{shutdown}
 */
void Object::remove()
{
   // Only delete it if we locally own it.
   if ( !removed_instance && is_create_HLA_instance() && is_shutdown_called() ) {

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Get the RTI-Ambassador.
      RTIambassador *rti_amb = get_RTI_ambassador();

      // If this is a local instance, we need to delete it from the
      // federation execution.
      if ( rti_amb != NULL ) {

         // This call returns an event retraction handle but we don't
         // support event retraction so no need to store it.
         try {

            Federate *federate = get_federate();
            if ( federate != NULL ) {
               // Only delete an object instance that has a valid instance handle.
               if ( is_instance_handle_valid() && federate->is_execution_member() ) {

                  // Delete the object instance at a specific time if we are
                  // time-regulating.
                  if ( federate->in_time_regulating_state() ) {
                     Int64Time update_time( get_granted_time() + get_lookahead() );
                     rti_amb->deleteObjectInstance( instance_handle,
                                                    RTI1516_USERDATA( 0, 0 ),
                                                    update_time.get() );
                  } else {
                     rti_amb->deleteObjectInstance( instance_handle,
                                                    RTI1516_USERDATA( 0, 0 ) );
                  }
               }
            }
         } catch ( DeletePrivilegeNotHeld const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of DeletePrivilegeNotHeld Exception: '%s'\n",
                             __LINE__, get_name(), rti_err_msg.c_str() );
         } catch ( ObjectInstanceNotKnown const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of ObjectInstanceNotKnown Exception: '%s'\n",
                             __LINE__, get_name(), rti_err_msg.c_str() );
         } catch ( FederateNotExecutionMember const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of FederateNotExecutionMember Exception: '%s'\n",
                             __LINE__, get_name(), rti_err_msg.c_str() );
         } catch ( SaveInProgress const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of SaveInProgress Exception: '%s'\n",
                             __LINE__, get_name(), rti_err_msg.c_str() );
         } catch ( RestoreInProgress const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of RestoreInProgress Exception: '%s'\n",
                             __LINE__, get_name(), rti_err_msg.c_str() );
         } catch ( NotConnected const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of NotConnected Exception: '%s'\n",
                             __LINE__, get_name(), rti_err_msg.c_str() );
         } catch ( RTIinternalError const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of RTIinternalError Exception: '%s'\n",
                             __LINE__, get_name(), rti_err_msg.c_str() );
         } catch ( RTI1516_EXCEPTION const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of Exception: '%s'\n",
                             __LINE__, get_name(), rti_err_msg.c_str() );
         }

         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         removed_instance = true;
      }
   }
}

void Object::remove_object_instance()
{
   // Set a flag to capture the notification of object deletion.
   this->process_object_deleted_from_RTI = true;
   this->object_deleted_from_RTI         = true;

   // Since the object has been deleted from the federation, make sure it is
   // no longer required in the federation. if a checkpoint is cut after this
   // object is removed, we need to correctly identify this object as not
   // required when restoring the saved federation...
   this->required = false;

   // Handle the case where we are publishing data for attributes we got
   // ownership transferred to us, since we don't own the privilege to delete.
   mark_all_attributes_as_nonlocal();

   // If we have a thread blocked waiting for data, make sure we change to
   // unblocking cyclic reads now that the object has been deleted. This will
   // help prevent the main execution thread being deadlocked waiting for data
   // to arrive, which will never happen now that the object has been deleted.
   set_to_unblocking_cyclic_reads();

   // Override the instance handle so that it is no longer valid, making it
   // unregistered so that it can be discovered and assigned a new object
   // instance handle again.
   this->instance_handle = ObjectInstanceHandle();

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_NORMAL, "Object::remove_object_instance():%d Object '%s' Instance-ID:%s Valid-ID:%s \n",
                       __LINE__, get_name(), id_str.c_str(),
                       ( is_instance_handle_valid() ? "Yes" : "No" ) );
   }
}

void Object::process_deleted_object()
{
   if ( this->process_object_deleted_from_RTI ) {

      // Set the flag that callback may have been triggered, so that we
      // only process the deleted object once.
      this->process_object_deleted_from_RTI = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_NORMAL, "Object::process_deleted_object():%d Object '%s' Instance-ID:%s Valid-ID:%s \n",
                          __LINE__, get_name(), id_str.c_str(),
                          ( is_instance_handle_valid() ? "Yes" : "No" ) );
      }

      // If the callback class has been defined, call it...
      if ( ( deleted != NULL ) && ( dynamic_cast< ObjectDeleted * >( deleted ) != NULL ) ) {
         deleted->deleted();
      }
   }
}

/*!
 * @details Turns off the local flag for all attributes in this object. This
 * is needed so when an object pushed the ownership of its attributes and
 * disappears before pulling the ownership back or the object resigns from the
 * federation, the object which received the ownership does not have delete
 * privilege (that disappeared with the declaring object) so it must not
 * continue to publish the deleted object's data. This also applies to
 * attributes of an object that resigns from the federation.
 */
void Object::mark_all_attributes_as_nonlocal()
{
   ostringstream msg;
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );

      msg << "Object::mark_all_attributes_as_nonlocal():"
          << __LINE__ << '\n'
          << "  Object:'" << get_name() << "'"
          << " FOM-Name:'" << get_FOM_name() << "'"
          << " Instance-ID:" << id_str;
   }
   for ( int i = 0; i < attr_count; ++i ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         msg << '\n'
             << "   " << ( i + 1 ) << "/" << attr_count
             << " FOM-Attribute:'" << attributes[i].get_FOM_name() << "'"
             << " Trick-Name:'" << attributes[i].get_trick_name() << "'"
             << " locally_owned: "
             << ( attributes[i].is_locally_owned() ? "Yes" : "No" );
      }

      if ( attributes[i].is_locally_owned() ) {
         attributes[i].unmark_locally_owned();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            msg << '\n'
                << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attributes[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attributes[i].get_trick_name() << "'"
                << " locally_owned: "
                << ( attributes[i].is_locally_owned() ? "Yes" : "No" );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      msg << '\n';
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }
}

/*!
 * @job_class{initialization}
 */
void Object::publish_object_attributes()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   // Publish our associated CLASS & attributes
   if ( rti_amb != NULL ) {

      if ( ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) && any_attribute_published() ) {
         message_publish( MSG_NORMAL, "Object::publish_object_attributes():%d For object '%s'.\n",
                          __LINE__, get_name() );
      }

      try {
         AttributeHandleSet attrs;

         // Publish only the attributes we have the publish flag set for.
         for ( int i = 0; i < attr_count; ++i ) {
            if ( attributes[i].is_publish() ) {
               attrs.insert( attributes[i].get_attribute_handle() );
            }
         }

         if ( !attrs.empty() ) {
            rti_amb->publishObjectClassAttributes( this->class_handle, attrs );
         }
      } catch ( ObjectClassNotDefined const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::publish_object_attributes():%d ObjectClassNotDefined : '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( AttributeNotDefined const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::publish_object_attributes():%d ObjectClassNotDefined : '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( FederateNotExecutionMember const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::publish_object_attributes():%d FederateNotExecutionMember : '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( SaveInProgress const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::publish_object_attributes():%d SaveInProgress : '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RestoreInProgress const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::publish_object_attributes():%d RestoreInProgress : '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( NotConnected const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::publish_object_attributes():%d NotConnected : '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::publish_object_attributes():%d RTIinternalError : '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::publish_object_attributes():%d class attributes exception for '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      message_publish( MSG_WARNING, "Object::publish_object_attributes():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
   }
}

/*!
 * @job_class{initialization}
 */
void Object::unpublish_all_object_attributes()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   // Subscribe to CLASS & attributes
   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Object::unpublish_all_object_attributes():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

   if ( any_attribute_published() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::unpublish_all_object_attributes():%d For object '%s'.\n",
                          __LINE__, get_name() );
      }

      try {
         rti_amb->unpublishObjectClass( this->class_handle );
      } catch ( RTI1516_NAMESPACE::ObjectClassNotDefined const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unpublish_all_object_attributes():%d ObjectClassNotDefined '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::OwnershipAcquisitionPending const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unpublish_all_object_attributes():%d OwnershipAcquisitionPending '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unpublish_all_object_attributes():%d FederateNotExecutionMember '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unpublish_all_object_attributes():%d SaveInProgress '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unpublish_all_object_attributes():%d RestoreInProgress '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unpublish_all_object_attributes():%d NotConnected '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unpublish_all_object_attributes():%d RTIinternalError '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unpublish_all_object_attributes():%d exception '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

/*!
 * @job_class{initialization}
 */
void Object::subscribe_to_object_attributes()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   // Subscribe to CLASS & attributes
   if ( rti_amb != NULL ) {

      if ( ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) && any_attribute_subscribed() ) {
         message_publish( MSG_NORMAL, "Object::subscribe_to_object_attributes():%d For object '%s'.\n",
                          __LINE__, get_name() );
      }

      try {
         // To actually subscribe we need to build an AttributeHandleSet
         // that contains a list of attribute type ids (AttributeHandle).

         AttributeHandleSet attrs;

         // Subscribe only to the attributes we have the subscribe flag set for.
         for ( int i = 0; i < attr_count; ++i ) {
            if ( attributes[i].is_subscribe() ) {
               attrs.insert( attributes[i].get_attribute_handle() );
            }
         }

         if ( !attrs.empty() ) {
            rti_amb->subscribeObjectClassAttributes( this->class_handle,
                                                     attrs,
                                                     true );
         }
      } catch ( ObjectClassNotDefined const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d ObjectClassNotDefined : '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( AttributeNotDefined const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d AttributeNotDefined : '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( FederateNotExecutionMember const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d FederateNotExecutionMember : '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( SaveInProgress const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d SaveInProgress : '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RestoreInProgress const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d RestoreInProgress : '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( InvalidUpdateRateDesignator const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d InvalidUpdateRateDesignator : '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( NotConnected const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d NotConnected : '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d RTIinternalError : '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d exception '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      message_publish( MSG_WARNING, "Object::subscribe_to_object_attributes():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
   }
}

/*!
 * @job_class{initialization}
 */
void Object::unsubscribe_all_object_attributes()
{
   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   // Subscribe to CLASS & attributes
   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Object::unsubscribe_all_object_attributes():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

   if ( any_attribute_subscribed() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::unsubscribe_all_object_attributes():%d For object '%s'.\n",
                          __LINE__, get_name() );
      }

      try {
         rti_amb->unsubscribeObjectClass( this->class_handle );
      } catch ( RTI1516_NAMESPACE::ObjectClassNotDefined const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unsubscribe_all_object_attributes():%d ObjectClassNotDefined '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unsubscribe_all_object_attributes():%d FederateNotExecutionMember '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::SaveInProgress const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unsubscribe_all_object_attributes():%d SaveInProgress '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unsubscribe_all_object_attributes():%d RestoreInProgress '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::NotConnected const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unsubscribe_all_object_attributes():%d NotConnected '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_NAMESPACE::RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unsubscribe_all_object_attributes():%d RTIinternalError '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::unsubscribe_all_object_attributes():%d exception '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

/*!
 * @job_class{initialization}
 */
void Object::reserve_object_name_with_RTI()
{
   // Just return if the object instance name is not required.
   if ( !is_name_required() ) {
      return;
   }

   // If we don't create an instance of object we should not reserve the object
   // instance name with the RTI.
   if ( !is_create_HLA_instance() ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Object::reserve_object_name_with_RTI():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

   // Reserving an object instance name with the RTI allows the object to be
   // recognized by name when discovered by other federates in the federation
   // execution. Note: We are notified in a callback if the name reservation
   // was successful or failed.
   if ( is_instance_handle_valid() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::reserve_object_name_with_RTI():%d \
WARNING: Object instance already exists so we will not reserve the instance name '%s' for it!\n",
                          __LINE__, get_name() );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::reserve_object_name_with_RTI():%d \
Requesting reservation of Object instance name '%s'.\n",
                          __LINE__, get_name() );
      }

      // Create the wide-string version of the object instance name.
      wstring ws_obj_name;
      StringUtilities::to_wstring( ws_obj_name, get_name() );

      try {
         rti_amb->reserveObjectInstanceName( ws_obj_name );
      } catch ( IllegalName const &e ) {
         message_publish( MSG_WARNING, "Object::reserve_object_name_with_RTI():%d IllegalName \n", __LINE__ );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Object::reserve_object_name_with_RTI():%d FederateNotExecutionMember \n", __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::reserve_object_name_with_RTI():%d SaveInProgress \n", __LINE__ );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::reserve_object_name_with_RTI():%d RestoreInProgress \n", __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Object::reserve_object_name_with_RTI():%d NotConnected \n", __LINE__ );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::reserve_object_name_with_RTI():%d RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         ostringstream errmsg;
         errmsg << "Object::reserve_object_name_with_RTI():" << __LINE__
                << " ERROR: Exception reserving '" << get_name() << "': '"
                << rti_err_msg << "'.";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

/*!
 * @details Calling this function will block until the object instance name is
 * reserved, but only if the object is locally owned.
 * @job_class{initialization}
 */
void Object::wait_for_object_name_reservation()
{
   // Just return if the object instance name is not required.
   if ( !is_name_required() ) {
      return;
   }

   // If we don't create an HLA instance of the object we should not be waiting
   // for the reservation of the object instance name.
   if ( !is_create_HLA_instance() ) {
      return;
   }

   // If we already have an instance handle/ID then we can safely assume the
   // name has already been reserved, so just return.
   if ( is_instance_handle_valid() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      message_publish( MSG_NORMAL, "Object::wait_for_object_name_reservation():%d \
Waiting on reservation of Object Instance Name '%s'.\n",
                       __LINE__, get_name() );
   }

   Federate    *federate = get_federate();
   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !name_registered ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep();

      if ( !name_registered ) { // cppcheck-suppress [knownConditionTrueFalse,unmatchedSuppression]

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Object::wait_for_object_name_reservation():" << __LINE__
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
            message_publish( MSG_NORMAL, "Object::wait_for_object_name_reservation():%d \
Waiting on reservation of Object Instance Name '%s'.\n",
                             __LINE__, get_name() );
         }
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      message_publish( MSG_NORMAL, "Object::wait_for_object_name_reservation():%d \
Object instance name '%s' is reserved.\n",
                       __LINE__, get_name() );
   }
}

/*!
 * @job_class{initialization}
 */
void Object::register_object_with_RTI()
{
   // If we don't create an HLA instance of the object we should not register
   // it with the RTI.
   if ( !is_create_HLA_instance() ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

   // Register object with the RTI. Registering an object with the RTI allows
   // the object to be discovered by other federates in the federation execution.
   if ( is_instance_handle_valid() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_NORMAL, "Object::register_object_with_RTI():%d WARNING: \
Detected object already registered '%s' Instance-ID:%s\n",
                          __LINE__, get_name(), id_str.c_str() );
      }
   } else {
      try {
         if ( is_name_required() ) {
            wstring ws_obj_name;
            StringUtilities::to_wstring( ws_obj_name, get_name() );
            this->instance_handle = rti_amb->registerObjectInstance( this->class_handle, ws_obj_name );
         } else {
            this->instance_handle = rti_amb->registerObjectInstance( this->class_handle );
         }
      } catch ( ObjectInstanceNameInUse const &e ) {
         message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d ObjectInstanceNameInUse %s \n",
                          __LINE__, get_name() );
      } catch ( ObjectInstanceNameNotReserved const &e ) {
         message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d ObjectInstanceNameNotReserved %s \n",
                          __LINE__, get_name() );
      } catch ( ObjectClassNotDefined const &e ) {
         message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d ObjectClassNotDefined %s \n",
                          __LINE__, get_name() );
      } catch ( ObjectClassNotPublished const &e ) {
         message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d ObjectClassNotPublished %s \n",
                          __LINE__, get_name() );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d FederateNotExecutionMember %s \n",
                          __LINE__, get_name() );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d SaveInProgress %s \n",
                          __LINE__, get_name() );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d RestoreInProgress %s \n",
                          __LINE__, get_name() );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d NotConnected %s \n",
                          __LINE__, get_name() );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d % RTIinternalError: '%s'\n",
                          __LINE__, get_name(), rti_err_msg.c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         ostringstream errmsg;
         errmsg << "Object::register_object_with_RTI():" << __LINE__
                << " ERROR: Exception registering '" << get_name() << "': '"
                << rti_err_msg << "'.\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      // If the name was not required then get the name the RTI is using for
      // our object.
      if ( !is_name_required() ) {
         // Now get the instance name for this object, the name the RTI is using
         // for our object since we did not supply a name for the object when we
         // registered it.
         try {
            // Get the instance name based on the object instance handle. Use
            // the set_name() function to avoid a memory leak on the name.
            set_name( StringUtilities::ip_strdup_wstring(
               rti_amb->getObjectInstanceName( this->instance_handle ) ) );
         } catch ( ObjectInstanceNotKnown const &e ) {
            message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: ObjectInstanceNotKnown \n",
                             __LINE__ );
         } catch ( FederateNotExecutionMember const &e ) {
            message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: FederateNotExecutionMember \n",
                             __LINE__ );
         } catch ( NotConnected const &e ) {
            message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: NotConnected \n",
                             __LINE__ );
         } catch ( RTIinternalError const &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            message_publish( MSG_WARNING, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: RTIinternalError: '%s'\n",
                             __LINE__, rti_err_msg.c_str() );
         } catch ( RTI1516_EXCEPTION const &e ) {
            // Macro to restore the saved FPU Control Word register value.
            TRICKHLA_RESTORE_FPU_CONTROL_WORD;
            TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
            string id_str;
            StringUtilities::to_string( id_str, instance_handle );
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            ostringstream errmsg;
            errmsg << "Object::register_object_with_RTI():" << __LINE__
                   << " ERROR: Exception getting instance name for '" << get_name()
                   << "' ID:" << id_str << "  '" << rti_err_msg << "'.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_NORMAL, "Object::register_object_with_RTI():%d Registered '%s' Instance-ID:%s\n",
                          __LINE__, get_name(), id_str.c_str() );
      }
   }
}

/*!
 * @details Calling this function will block until the object instance is registered.
 * @job_class{initialization}
 */
void Object::wait_for_object_registration()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      message_publish( MSG_NORMAL, "Object::wait_for_object_registration():%d Waiting on registration of '%s' for object '%s'.\n",
                       __LINE__, FOM_name, get_name() );
   }

   Federate    *federate = get_federate();
   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;

   while ( !is_instance_handle_valid() ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep();

      if ( !is_instance_handle_valid() ) { // cppcheck-suppress [knownConditionTrueFalse,unmatchedSuppression]

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Object::wait_for_object_registration():" << __LINE__
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
            message_publish( MSG_NORMAL, "Object::wait_for_object_registration():%d Waiting on registration of '%s' for object '%s'.\n",
                             __LINE__, FOM_name, get_name() );
         }
      }
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      message_publish( MSG_NORMAL, "Object::wait_for_object_registration():%d Object \
instance '%s' for object '%s' is registered.\n",
                       __LINE__, FOM_name,
                       get_name() );
   }
}

/*!
 * @job_class{scheduled}
 */
void Object::setup_preferred_order_with_RTI()
{
   // The instance handle must be valid and there needs to be at one locally
   // owned attribute, otherwise just return.
   if ( !is_instance_handle_valid() || !any_locally_owned_attribute() ) {
      return;
   }

   ostringstream msg;
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      msg << "Object::setup_preferred_order_with_RTI():" << __LINE__ << '\n'
          << "--------- Setup Preferred-Order of Locally-Owned Attributes ---------\n"
          << " Object:'" << get_name() << "'"
          << " FOM-Name:'" << get_FOM_name() << "'"
          << " Create HLA Instance:" << ( is_create_HLA_instance() ? "Yes" : "No" )
          << '\n';
   }

   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();

   AttributeHandleSet TSO_attr_handle_set = AttributeHandleSet();
   AttributeHandleSet RO_attr_handle_set  = AttributeHandleSet();

   // Create the sets of Attribute handles for the Timestamp preferred order.
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_locally_owned()
           && ( attributes[i].get_preferred_order() == TRANSPORT_TIMESTAMP_ORDER ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            msg << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attributes[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attributes[i].get_trick_name() << "'"
                << " Preferred-Order:TIMESTAMP\n";
         }
         TSO_attr_handle_set.insert( attributes[i].get_attribute_handle() );
      }
   }

   // Create the sets of Attribute handles for the Receive preferred order.
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_locally_owned()
           && ( attributes[i].get_preferred_order() == TRANSPORT_RECEIVE_ORDER ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            msg << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attributes[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attributes[i].get_trick_name() << "'"
                << " Preferred-Order:RECEIVE\n";
         }
         RO_attr_handle_set.insert( attributes[i].get_attribute_handle() );
      }
   }

   // If we don't have any attributes with a preferred order we need to change
   // then just return.
   if ( TSO_attr_handle_set.empty() && RO_attr_handle_set.empty() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      message_publish( MSG_NORMAL, msg.str().c_str() );
   }

   try {
      if ( !TSO_attr_handle_set.empty() ) {
         rti_amb->changeAttributeOrderType( this->instance_handle,
                                            TSO_attr_handle_set,
                                            RTI1516_NAMESPACE::TIMESTAMP );
      }
      // Must free the memory
      TSO_attr_handle_set.clear();

      if ( !RO_attr_handle_set.empty() ) {
         rti_amb->changeAttributeOrderType( this->instance_handle,
                                            RO_attr_handle_set,
                                            RTI1516_NAMESPACE::RECEIVE );
      }
      // Must free the memory
      RO_attr_handle_set.clear();

   } catch ( ObjectInstanceNotKnown const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::setup_preferred_order_with_RTI():%d Object instance not known for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( AttributeNotOwned const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::setup_preferred_order_with_RTI():%d attribute not owned for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( AttributeNotDefined const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::setup_preferred_order_with_RTI():%d attribute not defined for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( FederateNotExecutionMember const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::setup_preferred_order_with_RTI():%d federation not execution member for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( SaveInProgress const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::setup_preferred_order_with_RTI():%d save in progress for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( RestoreInProgress const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::setup_preferred_order_with_RTI():%d restore in progress for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( NotConnected const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::setup_preferred_order_with_RTI():%d not connected error for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( RTIinternalError const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::setup_preferred_order_with_RTI():%d RTI internal error for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object.setup_preferred_order_with_RTI():%d Exception: '%s' and instance_id=%s\n",
                       __LINE__, rti_err_msg.c_str(), id_str.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void Object::request_attribute_value_update()
{
   // There needs to be at one remotely owned attribute that is subscribed to
   // before we can request an update.
   if ( !any_remotely_owned_subscribed_attribute() ) {
      return;
   }

   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();

   // Create the set of Attribute handles we need to request an update for.
   AttributeHandleSet attr_handle_set = AttributeHandleSet();
   for ( int i = 0; i < attr_count; ++i ) {
      // Only include attributes that are remotely owned that are subscribed to.
      if ( attributes[i].is_remotely_owned() && attributes[i].is_subscribe() ) {
         attr_handle_set.insert( attributes[i].get_attribute_handle() );
      }
   }

   try {
      rti_amb->requestAttributeValueUpdate( this->instance_handle,
                                            attr_handle_set,
                                            RTI1516_USERDATA( 0, 0 ) );
      // Must free the memory
      attr_handle_set.clear();

   } catch ( AttributeNotDefined const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::request_attribute_value_update():%d attribute not defined for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( ObjectInstanceNotKnown const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::request_attribute_value_update():%d object instance not known for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( FederateNotExecutionMember const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::request_attribute_value_update():%d federation not execution member for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( SaveInProgress const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::request_attribute_value_update():%d save in progress for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( RestoreInProgress const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::request_attribute_value_update():%d restore in progress for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( NotConnected const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::request_attribute_value_update():%d not connected error for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( RTIinternalError const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::request_attribute_value_update():%d RTI internal error for '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object.request_attribute_value_update():%d Exception: '%s' and instance_id=%s\n",
                       __LINE__, get_name(), id_str.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void Object::provide_attribute_update(
   AttributeHandleSet const &theAttributes )
{
   bool any_attr_requested = false;

   // Search the TrickHLA object attributes to see if any of them are
   // part of the set of the attribute value update request.
   for ( int i = 0; i < attr_count; ++i ) {

      // Determine if this object attribute needs to provide an update.
      if ( theAttributes.find( attributes[i].get_attribute_handle() ) != theAttributes.end() ) {

         // Mark the attribute as having a request for an update.
         attributes[i].set_update_requested( true );
         any_attr_requested = true;
      }
   }

   // Make sure we mark the request at the object level if we had at least
   // attribute we need to provide an update for.
   if ( any_attr_requested ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::provide_attribute_update():%d Object '%s'\n",
                          __LINE__, get_name() );
      }

      this->attr_update_requested = true;
   }
}

/*!
 * @job_class{scheduled}
 */
void Object::send_requested_data()
{
   if ( attr_update_requested ) {
      Int64Time granted_plus_lookahead( get_granted_time() + get_lookahead() );
      send_requested_data( granted_plus_lookahead );
   }
}

/*!
 * @job_class{scheduled}
 */
void Object::send_requested_data(
   Int64Time const &update_time )
{
   // If no attribute update has been requested then just return.
   if ( !attr_update_requested ) {
      return;
   }

   // Make sure we clear the attribute update request flag because we only
   // want to send data once per request.
   this->attr_update_requested = false;

   // We can only send attribute updates for the attributes we own and are
   // configured to publish.
   if ( !any_locally_owned_published_requested_attribute() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      message_publish( MSG_NORMAL, "Object::send_requested_data():%d Object '%s'\n",
                       __LINE__, get_name() );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &send_mutex );

   // Do lag compensation.
   if ( lag_comp != NULL ) {
      switch ( lag_comp_type ) {
         case LAG_COMPENSATION_SEND_SIDE: {
            lag_comp->send_lag_compensation();
            break;
         }
         case LAG_COMPENSATION_RECEIVE_SIDE: {
            // There are locally owned attributes that are published by this
            // federate that we need to send even tough Received-side lag
            // compensation has been configured. Maybe a result of attribute
            // ownership transfer.
            lag_comp->bypass_send_lag_compensation();
            break;
         }
         case LAG_COMPENSATION_NONE:
         default: {
            lag_comp->bypass_send_lag_compensation();
            break;
         }
      }
   }

   // If we have a data packing object then pack the data now.
   if ( packing != NULL ) {
      packing->pack();
   }

   // Buffer the requested attribute values for the object.
   pack_requested_attribute_buffers();

   try {
      // Create the map of "requested" attribute values we will be updating.
      create_requested_attribute_set();
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::send_requested_data():%d Can not create attribute value/pair set: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   Federate const *federate = get_federate();

   // The message will only be sent as TSO if our Federate is in the HLA Time
   // Regulating state and we have at least one attribute with a preferred
   // timestamp order. Assumes the FOM specified order is TSO.
   // See IEEE-1516.1-2010, Sections 6.6 and 8.1.1.
   bool const send_with_timestamp = federate->in_time_regulating_state()
                                    && ( this->any_attribute_timestamp_order
                                         || this->any_attribute_FOM_specified_order );

   try {
      // Send the Attributes to the federation.
      //
      // This call returns an event retraction handle but we don't support event
      // retraction so no need to store it.

      // Do not send any data if federate save or restore has begun (see
      // IEEE-1516.1-2010 sections 4.12, 4.20)
      if ( federate->should_publish_data() ) {

         RTIambassador *rti_amb = get_RTI_ambassador();

         if ( send_with_timestamp ) {
            if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::send_requested_data():%d \
Object '%s', Timestamp Order (TSO) Attribute update, HLA Logical Time:%f seconds.\n",
                                __LINE__, get_name(), update_time.get_time_in_seconds() );
            }
            // Send as Timestamp Order
            rti_amb->updateAttributeValues( this->instance_handle,
                                            *attribute_values_map,
                                            RTI1516_USERDATA( 0, 0 ),
                                            update_time.get() );
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::send_requested_data():%d Object '%s', Receive Order (RO) Attribute update.\n",
                                __LINE__, get_name() );
            }

            // Send as Receive Order
            rti_amb->updateAttributeValues( this->instance_handle,
                                            *attribute_values_map,
                                            RTI1516_USERDATA( 0, 0 ) );
         }
#ifdef THLA_CHECK_SEND_AND_RECEIVE_COUNTS
         ++send_count;
#endif
      }
   } catch ( InvalidLogicalTime const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::send_requested_data():%d invalid logical time \
exception for '%s' with error message '%s'.\n",
                       __LINE__, get_name(), rti_err_msg.c_str() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: InvalidLogicalTime\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << " ("
             << get_granted_time().get_base_time() << " " << Int64BaseTime::get_units()
             << ")\n"
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << " ("
             << get_lookahead().get_base_time() << " " << Int64BaseTime::get_units()
             << ")\n"
             << "  update_time=" << update_time.get_time_in_seconds() << " ("
             << update_time.get_base_time() << " " << Int64BaseTime::get_units()
             << ")\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( AttributeNotOwned const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING,
                       "Object::send_requested_data():%d detected remote ownership for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: AttributeNotOwned\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
             << "  update_time=" << update_time.get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( ObjectInstanceNotKnown const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING,
                       "Object::send_requested_data():%d object instance not known for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: ObjectInstanceNotKnown\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
             << "  update_time=" << update_time.get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( AttributeNotDefined const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_requested_data():%d attribute not defined for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: AttributeNotDefined\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
             << "  update_time=" << update_time.get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( FederateNotExecutionMember const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_requested_data():%d federation not execution member for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception:FederateNotExecutionMember\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
             << "  update_time=" << update_time.get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( SaveInProgress const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_requested_data():%d save in progress for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: SaveInProgress\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
             << "  update_time=" << update_time.get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( RestoreInProgress const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_requested_data():%d restore in progress for '%s'",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: RestoreInProgress\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
             << "  update_time=" << update_time.get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( NotConnected const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_requested_data():%d not connected error for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: NotConnected\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
             << "  update_time=" << update_time.get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( RTIinternalError const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_requested_data():%d RTI internal error for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: RTIinternalError\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
             << "  update_time=" << update_time.get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object.send_requested_data():%d Exception: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " RTI1516_EXCEPTION\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
             << "  update_time=" << update_time.get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void Object::send_cyclic_and_requested_data(
   Int64Time const &update_time )
{
   // Make sure we clear the attribute update request flag because we only
   // want to send data once per request.
   this->attr_update_requested = false;

   // We can only send cyclic attribute updates for the attributes we own, are
   // configured to publish and the cycle-time is ready for a send or was requested.
   if ( !any_locally_owned_published_cyclic_data_ready_or_requested_attribute() ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &send_mutex );

   // Do lag compensation.
   if ( lag_comp != NULL ) {
      switch ( lag_comp_type ) {
         case LAG_COMPENSATION_SEND_SIDE: {
            lag_comp->send_lag_compensation();
            break;
         }
         case LAG_COMPENSATION_RECEIVE_SIDE: {
            // There are locally owned attributes that are published by this
            // federate that we need to send even tough Received-side lag
            // compensation has been configured. Maybe a result of attribute
            // ownership transfer.
            lag_comp->bypass_send_lag_compensation();
            break;
         }
         case LAG_COMPENSATION_NONE:
         default: {
            lag_comp->bypass_send_lag_compensation();
            break;
         }
      }
   }

   // If we have a data packing object then pack the data now.
   if ( packing != NULL ) {
      packing->pack();
   }

   // Buffer the attribute values for the object.
   pack_cyclic_and_requested_attribute_buffers();

   try {
      // Create the map of "cyclic" and requested attribute values we will be updating.
      create_attribute_set( CONFIG_CYCLIC, true );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d For object '%s', cannot create attribute value/pair set: '%s'\n",
                       __LINE__, get_name(), rti_err_msg.c_str() );
   }

   // Make sure we don't send an empty attribute map to the other federates.
   if ( !attribute_values_map->empty() ) {

      Federate const *federate = get_federate();

      // The message will only be sent as TSO if our Federate is in the HLA Time
      // Regulating state and we have at least one attribute with a preferred
      // timestamp order. Assumes the FOM specified order is TSO.
      // See IEEE-1516.1-2010, Sections 6.6 and 8.1.1.
      bool const send_with_timestamp = federate->in_time_regulating_state()
                                       && ( this->any_attribute_timestamp_order
                                            || this->any_attribute_FOM_specified_order );

      try {
         // Do not send any data if federate save or restore has begun (see
         // IEEE-1516.1-2010 sections 4.12, 4.20)
         if ( federate->should_publish_data() ) {

            RTIambassador *rti_amb = get_RTI_ambassador();

            if ( send_with_timestamp ) {

               if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                  message_publish( MSG_NORMAL, "Object::send_cyclic_and_requested_data():%d \
Object '%s', Timestamp Order (TSO) Attribute update, HLA Logical Time:%f seconds.\n",
                                   __LINE__, get_name(), update_time.get_time_in_seconds() );
               }

               // Send as Timestamp Order
               rti_amb->updateAttributeValues( this->instance_handle,
                                               *attribute_values_map,
                                               RTI1516_USERDATA( 0, 0 ),
                                               update_time.get() );
            } else {
               if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                  message_publish( MSG_NORMAL, "Object::send_cyclic_and_requested_data():%d Object '%s', Receive Order (RO) Attribute update.\n",
                                   __LINE__, get_name() );
               }

               // Send as Receive Order (i.e. with no timestamp).
               rti_amb->updateAttributeValues( this->instance_handle,
                                               *attribute_values_map,
                                               RTI1516_USERDATA( 0, 0 ) );
            }
#ifdef THLA_CHECK_SEND_AND_RECEIVE_COUNTS
            ++send_count;
#endif
         }
      } catch ( InvalidLogicalTime const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d invalid logical time \
exception for '%s' with error message '%s'.\n",
                          __LINE__, get_name(), rti_err_msg.c_str() );

         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " Exception: InvalidLogicalTime\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << " ("
                << get_granted_time().get_base_time() << " " << Int64BaseTime::get_units()
                << ")\n"
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << " ("
                << get_lookahead().get_base_time() << " " << Int64BaseTime::get_units()
                << ")\n"
                << "  update_time=" << update_time.get_time_in_seconds() << " ("
                << update_time.get_base_time() << " " << Int64BaseTime::get_units()
                << ")\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( AttributeNotOwned const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d detected remote ownership for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " Exception: AttributeNotOwned\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( ObjectInstanceNotKnown const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d object instance not known for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " Exception: ObjectInstanceNotKnown\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( AttributeNotDefined const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d attribute not defined for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " Exception: AttributeNotDefined\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( FederateNotExecutionMember const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d federation not execution member for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " Exception: FederateNotExecutionMember\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( SaveInProgress const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d save in progress for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " Exception: SaveInProgress\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( RestoreInProgress const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d restore in progress for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " Exception: RestoreInProgress\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( NotConnected const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d not connected for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " Exception: NotConnected\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( RTIinternalError const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_cyclic_and_requested_data():%d RTI internal error for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " Exception: RTIinternalError\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object.send_cyclic_and_requested_data():%d Exception: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
         ostringstream errmsg;
         errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
                << " RTI1516_EXCEPTION\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void Object::send_zero_lookahead_and_requested_data(
   Int64Time const &update_time )
{
   // Make sure we clear the attribute update request flag because we only
   // want to send data once per request.
   this->attr_update_requested = false;

   // We can only send zero-lookahead attribute updates for the attributes we
   // own, are configured to publish.
   if ( !any_locally_owned_published_zero_lookahead_or_requested_attribute() ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &send_mutex );

   // Lag-compensation is not supported for zero-lookahead, but if specified
   // we call the bypass function.
   if ( lag_comp != NULL ) {
      switch ( lag_comp_type ) {
         case LAG_COMPENSATION_NONE: {
            lag_comp->bypass_send_lag_compensation();
            break;
         }
         case LAG_COMPENSATION_SEND_SIDE:
         case LAG_COMPENSATION_RECEIVE_SIDE:
         default: {
            ostringstream errmsg;
            errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                   << " ERROR: For object '" << this->name << "', detected a"
                   << " Lag-Compensation 'lag_comp' callback has also been"
                   << " specified with a 'lag_comp_type' setting that is not"
                   << " LAG_COMPENSATION_NONE! Please check your input or"
                   << " modified-data files to make sure the Lag-Compensation"
                   << " type 'lag_comp_type' is set to LAG_COMPENSATION_NONE"
                   << " to disable Lag-Compensation when sending zero-lookahead"
                   << " configured object attributes.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
      }
   }

   // If we have a data packing object then pack the data now.
   if ( packing != NULL ) {
      packing->pack();
   }

   // Buffer the attribute values for the object.
   pack_zero_lookahead_and_requested_attribute_buffers();

   try {
      // Create the map of "zero lookahead" and requested attribute values we
      // will be updating.
      create_attribute_set( CONFIG_ZERO_LOOKAHEAD, true );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d For object '%s', cannot create attribute value/pair set: '%s'\n",
                       __LINE__, get_name(), rti_err_msg.c_str() );
   }

   // Make sure we don't send an empty attribute map to the other federates.
   if ( attribute_values_map->empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " WARNING: For object '" << this->name << "', detected that there"
                << " are no attributes to send an update for, which can lead to"
                << " deadlock for another federate waiting to receive zero-lookahead"
                << " data. Please check your input or modified-data files to make"
                << " sure at least attribute is configured for zero-lookahead and"
                << " if you are using the TrickHLA::Conditional API make sure you"
                << " enable at least one attribute to be sent.\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   } else {

      Federate const *federate = get_federate();

      // The message will only be sent as TSO if our Federate is in the HLA Time
      // Regulating state and we have at least one attribute with a preferred
      // timestamp order. Assumes the FOM specified order is TSO.
      // See IEEE-1516.1-2010, Sections 6.6 and 8.1.1.
      bool const send_with_timestamp = federate->in_time_regulating_state()
                                       && ( this->any_attribute_timestamp_order
                                            || this->any_attribute_FOM_specified_order );

      try {
         // Do not send any data if federate save or restore has begun (see
         // IEEE-1516.1-2010 sections 4.12, 4.20)
         if ( federate->should_publish_data() ) {

            RTIambassador *rti_amb = get_RTI_ambassador();

            if ( send_with_timestamp ) {

               if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                  message_publish( MSG_NORMAL, "Object::send_zero_lookahead_and_requested_data():%d \
Object '%s', Timestamp Order (TSO) Attribute update, HLA Logical Time:%f seconds.\n",
                                   __LINE__, get_name(), update_time.get_time_in_seconds() );
               }

               // Send as Timestamp Order
               rti_amb->updateAttributeValues( this->instance_handle,
                                               *attribute_values_map,
                                               RTI1516_USERDATA( 0, 0 ),
                                               update_time.get() );
            } else {
               if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                  message_publish( MSG_NORMAL, "Object::send_zero_lookahead_and_requested_data():%d \
Object '%s', Receive Order (RO) Attribute update.\n",
                                   __LINE__, get_name() );
               }

               // Send as Receive Order (i.e. with no timestamp).
               rti_amb->updateAttributeValues( this->instance_handle,
                                               *attribute_values_map,
                                               RTI1516_USERDATA( 0, 0 ) );
            }
#ifdef THLA_CHECK_SEND_AND_RECEIVE_COUNTS
            ++send_count;
#endif
         }
      } catch ( InvalidLogicalTime const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d Exception: \
Invalid logical time exception for '%s' with error message '%s'.\n",
                          __LINE__, get_name(), rti_err_msg.c_str() );

         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " Exception: InvalidLogicalTime\n"
                << "  instance_id=" << id_str << '\n'
                << "  send-with-timestamp=" << ( send_with_timestamp ? "True" : "False" ) << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << " ("
                << get_granted_time().get_base_time() << " " << Int64BaseTime::get_units()
                << ")\n"
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << " ("
                << get_lookahead().get_base_time() << " " << Int64BaseTime::get_units()
                << ")\n"
                << "  update_time=" << update_time.get_time_in_seconds() << " ("
                << update_time.get_base_time() << " " << Int64BaseTime::get_units()
                << ")\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( AttributeNotOwned const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d detected remote ownership for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " Exception: AttributeNotOwned\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( ObjectInstanceNotKnown const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d object instance not known for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " Exception: ObjectInstanceNotKnown\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( AttributeNotDefined const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d attribute not defined for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " Exception: AttributeNotDefined\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( FederateNotExecutionMember const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d federation not execution member for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " Exception: FederateNotExecutionMember\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( SaveInProgress const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d save in progress for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " Exception: SaveInProgress\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( RestoreInProgress const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d restore in progress for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " Exception: RestoreInProgress\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( NotConnected const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d not connected for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " Exception: NotConnected\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( RTIinternalError const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_zero_lookahead_and_requested_data():%d RTI internal error for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " Exception: RTIinternalError\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object.send_zero_lookahead_and_requested_data():%d Exception: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
         ostringstream errmsg;
         errmsg << "Object::send_zero_lookahead_and_requested_data():" << __LINE__
                << " RTI1516_EXCEPTION\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n'
                << "  update_time=" << update_time.get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void Object::send_blocking_io_data()
{
   // Make sure we clear the attribute update request flag because we only
   // want to send data once per request.
   this->attr_update_requested = false;

   // We can only send blocking I/O attribute updates for the attributes we
   // own, and are configured to publish.
   if ( !any_locally_owned_published_blocking_io_attribute() ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &send_mutex );

   // Lag-compensation is not supported for blocking I/O (intraframe), but if
   // specified we call the bypass function.
   if ( lag_comp != NULL ) {
      switch ( lag_comp_type ) {
         case LAG_COMPENSATION_NONE: {
            lag_comp->bypass_send_lag_compensation();
            break;
         }
         case LAG_COMPENSATION_SEND_SIDE:
         case LAG_COMPENSATION_RECEIVE_SIDE:
         default: {
            ostringstream errmsg;
            errmsg << "Object::send_blocking_io_data():" << __LINE__
                   << " ERROR: For object '" << name << "', detected a"
                   << " Lag-Compensation 'lag_comp' callback has also been"
                   << " specified with a 'lag_comp_type' setting that is not"
                   << " LAG_COMPENSATION_NONE! Please check your input or"
                   << " modified-data files to make sure the Lag-Compensation"
                   << " type 'lag_comp_type' is set to LAG_COMPENSATION_NONE"
                   << " to disable Lag-Compensation when sending blocking I/O"
                   << " configured object attributes.\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            break;
         }
      }
   }

   // If we have a data packing object then pack the data now.
   if ( packing != NULL ) {
      packing->pack();
   }

   // Buffer the attribute values for the object.
   pack_blocking_io_attribute_buffers();

   try {
      // Create the map of "blocking I/O" attribute values we will be updating.
      create_attribute_set( CONFIG_BLOCKING_IO, false );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d For object '%s', cannot create attribute value/pair set: '%s'\n",
                       __LINE__, get_name(), rti_err_msg.c_str() );
   }

   // Make sure we don't send an empty attribute map to the other federates.
   if ( attribute_values_map->empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " WARNING: For object '" << this->name << "', detected that there"
                << " are no attributes to send an update for, which can lead to"
                << " deadlock for another federate waiting to receive blocking I/O"
                << " data. Please check your input or modified-data files to make"
                << " sure at least attribute is configured for blocking I/O and"
                << " if you are using the TrickHLA::Conditional API make sure you"
                << " enable at least one attribute to be sent.\n";
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   } else {

      Federate const *federate = get_federate();

      try {
         // Do not send any data if federate save or restore has begun (see
         // IEEE-1516.1-2010 sections 4.12, 4.20)
         if ( federate->should_publish_data() ) {

            RTIambassador *rti_amb = get_RTI_ambassador();

            if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::send_blocking_io_data():%d \
Object '%s', Receive Order (RO) Attribute update.\n",
                                __LINE__, get_name() );
            }

            // Send as Receive Order (i.e. with no timestamp).
            rti_amb->updateAttributeValues( this->instance_handle,
                                            *attribute_values_map,
                                            RTI1516_USERDATA( 0, 0 ) );

#ifdef THLA_CHECK_SEND_AND_RECEIVE_COUNTS
            ++send_count;
#endif
         }
      } catch ( InvalidLogicalTime const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d invalid logical time \
exception for '%s' with error message '%s'.\n",
                          __LINE__, get_name(), rti_err_msg.c_str() );

         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " Exception: InvalidLogicalTime\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << " ("
                << get_granted_time().get_base_time() << " " << Int64BaseTime::get_units()
                << ")\n"
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << " ("
                << get_lookahead().get_base_time() << " " << Int64BaseTime::get_units()
                << ")\n";
         ;
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( AttributeNotOwned const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d detected remote ownership for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " Exception: AttributeNotOwned\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( ObjectInstanceNotKnown const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d object instance not known for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " Exception: ObjectInstanceNotKnown\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( AttributeNotDefined const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d attribute not defined for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " Exception: AttributeNotDefined\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( FederateNotExecutionMember const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d federation not execution member for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " Exception: FederateNotExecutionMember\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( SaveInProgress const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d save in progress for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " Exception: SaveInProgress\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( RestoreInProgress const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d restore in progress for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " Exception: RestoreInProgress\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( NotConnected const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d not connected for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " Exception: NotConnected\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( RTIinternalError const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         message_publish( MSG_WARNING, "Object::send_blocking_io_data():%d RTI internal error for '%s'\n",
                          __LINE__, get_name() );
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " Exception: RTIinternalError\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object.send_blocking_io_data():%d Exception: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
         ostringstream errmsg;
         errmsg << "Object::send_blocking_io_data():" << __LINE__
                << " RTI1516_EXCEPTION\n"
                << "  instance_id=" << id_str << '\n'
                << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
                << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
         message_publish( MSG_WARNING, errmsg.str().c_str() );
      }
   }
   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @details If the object is owned remotely, this function copies its internal
 * data into simulation object and marks the object as "unchanged". This data
 * was deposited by the reflect callback and marked as "changed". By marking it
 * as unchanged, we avoid copying the same data over and over. If the object
 * is locally owned, we shouldn't be receiving any remote data anyway and if
 * we were to -- bogusly -- copy it to the internal byte buffer, we'd
 * continually reset our local simulation.
 * @job_class{scheduled}
 */
void Object::receive_cyclic_data()
{
   // There must be some remotely owned attribute that we subscribe to in
   // order for us to receive it.
   if ( !any_remotely_owned_subscribed_cyclic_attribute() ) {
      return;
   }

   // Block waiting for received data if the user has specified we must do so.
   if ( blocking_cyclic_read ) {

      // If we are doing blocking reads, we need to skip the first one since it
      // occurs at the top of the frame before any data has been sent resulting
      // in deadlock until the read 10 second timer expires.
      if ( first_blocking_cyclic_read ) {
         first_blocking_cyclic_read = false;

         // For the first read attempt, just return if no data has been received.
         if ( !is_changed() ) {
            return;
         }
      }

      // Block waiting for data if it has not arrived yet.
      if ( !is_changed() ) {

         SleepTimeout sleep_timer( THLA_LOW_LATENCY_SLEEP_WAIT_IN_MICROS );

         // On average using "usleep()" to wait for data is faster but at the
         // cost of latency spikes every once in a while. The CPU utilization
         // will be much less using usleep() as compared to the assembly "nop"
         // instruction. If you care about hard realtime performance
         // (i.e. maximum latency) then use the assembly "nop" method, otherwise
         // usleep() will give better average performance mainly because it will
         // free up the CPU allowing the HLA callback thread a better chance to
         // run.
         //
         // NOTE: Using usleep() for a spin-lock delay will cause a
         // 1 millisecond saw-tooth pattern in the latency between when we
         // received the data in the FedAmb callback and when we process the
         // data here. Dan Dexter 2/13/2008
         //
         // Wait for the data to change by using a spin lock that can timeout.
         while ( !is_changed()
                 && !sleep_timer.timeout()
                 && blocking_cyclic_read
                 && any_remotely_owned_subscribed_cyclic_attribute() ) {

            // Yield the processor.
            sleep_timer.sleep();
         }

         // Display a warning message if we timed out.
         if ( sleep_timer.timeout() ) {
            if ( is_changed() ) {
               message_publish( MSG_WARNING, "Object::receive_cyclic_data():%d Received data at a timeout boundary at simulation-time %f.\n",
                                __LINE__, exec_get_sim_time() );
            } else {
               message_publish( MSG_WARNING, "Object::receive_cyclic_data():%d Timed out waiting for data at simulation-time %f.\n",
                                __LINE__, exec_get_sim_time() );
            }
         }
      }
   }

   // Process the data now that it has been received (i.e. changed).
   if ( is_changed() ) {

#ifdef THLA_CYCLIC_READ_TIME_STATS
      elapsed_time_stats.measure();
#endif

      do {
#if THLA_OBJ_DEBUG_RECEIVE
         message_publish( MSG_NORMAL, "Object::receive_cyclic_data():%d for '%s' at HLA-logical-time=%G\n",
                          __LINE__, get_name(), manager->get_federate()->get_granted_time().get_time_in_seconds() );
#endif

         // Unpack the buffer and copy the values to the object attributes.
         unpack_cyclic_attribute_buffers();

         // Unpack the data for the object if we have a packing object.
         if ( packing != NULL ) {
            packing->unpack();
         }

         // Do lag compensation.
         if ( lag_comp != NULL ) {
            switch ( lag_comp_type ) {
               case LAG_COMPENSATION_RECEIVE_SIDE: {
                  lag_comp->receive_lag_compensation();
                  break;
               }
               case LAG_COMPENSATION_SEND_SIDE: {
                  // There are remotely owned attributes that are subscribed by
                  // this federate that we need to receive even tough Send-side
                  // lag compensation has been configured. Maybe a result of
                  // attribute ownership transfer.
                  lag_comp->bypass_receive_lag_compensation();
                  break;
               }
               case LAG_COMPENSATION_NONE:
               default: {
                  lag_comp->bypass_receive_lag_compensation();
                  break;
               }
            }
         }

         // Mark this data as unchanged now that we have processed it from the buffer.
         mark_unchanged();

         // Check for more object attribute data in the buffer/queue for this
         // object instance, which will show up as still being changed.
      } while ( is_changed() );
   }
#if THLA_OBJ_DEBUG_VALID_OBJECT_RECEIVE
   else if ( is_instance_handle_valid() && ( exec_get_sim_time() > 0.0 ) ) {
      message_publish( MSG_NORMAL, "Object::receive_cyclic_data():%d NO new data for valid object '%s' at HLA-logical-time=%G\n",
                       __LINE__, get_name(), manager->get_federate()->get_granted_time().get_time_in_seconds() );
   }
#endif
#if THLA_OBJ_DEBUG_RECEIVE
   else {
      message_publish( MSG_NORMAL, "Object::receive_cyclic_data():%d NO new data for '%s' at HLA-logical-time=%G\n",
                       __LINE__, get_name(), manager->get_federate()->get_granted_time().get_time_in_seconds() );
   }
#endif
}

/*!
 * @details If the object is owned remotely, this function copies its internal
 * data into simulation object and marks the object as "unchanged". This data
 * was deposited by the reflect callback and marked as "changed". By marking it
 * as unchanged, we avoid copying the same data over and over. If the object
 * is locally owned, we shouldn't be receiving any remote data anyway and if
 * we were to -- bogusly -- copy it to the internal byte buffer, we'd
 * continually reset our local simulation.
 * @job_class{scheduled}
 */
void Object::receive_zero_lookahead_data()
{
   // There must be some remotely owned attribute that we subscribe to in
   // order for us to receive it.
   if ( !any_remotely_owned_subscribed_zero_lookahead_attribute() ) {
      return;
   }

   // Process only one data item now that it has been received (i.e. changed).
   if ( is_changed() ) {

#if THLA_OBJ_DEBUG_RECEIVE
      message_publish( MSG_NORMAL, "Object::receive_zero_lookahead_data():%d for '%s' at HLA-logical-time=%G\n",
                       __LINE__, get_name(), manager->get_federate()->get_granted_time().get_time_in_seconds() );
#endif

      // Unpack the buffer and copy the values to the object attributes.
      unpack_zero_lookahead_attribute_buffers();

      // Unpack the data for the object if we have a packing object.
      if ( packing != NULL ) {
         packing->unpack();
      }

      // Lag-compensation is not supported for zero-lookahead, but if
      // specified we call the bypass function.
      if ( lag_comp != NULL ) {
         switch ( lag_comp_type ) {
            case LAG_COMPENSATION_NONE: {
               lag_comp->bypass_receive_lag_compensation();
               break;
            }
            case LAG_COMPENSATION_SEND_SIDE:
            case LAG_COMPENSATION_RECEIVE_SIDE:
            default: {
               ostringstream errmsg;
               errmsg << "Object::receive_zero_lookahead_data():" << __LINE__
                      << " ERROR: For object '" << name << "', detected a"
                      << " Lag-Compensation 'lag_comp' callback has also been"
                      << " specified with a 'lag_comp_type' setting that is not"
                      << " LAG_COMPENSATION_NONE! Please check your input or"
                      << " modified-data files to make sure the Lag-Compensation"
                      << " type 'lag_comp_type' is set to LAG_COMPENSATION_NONE"
                      << " to disable Lag-Compensation when sending zero-lookahead"
                      << " configured object attributes.\n";
               DebugHandler::terminate_with_message( errmsg.str() );
               break;
            }
         }
      }

      // Mark this data as unchanged now that we have processed it from the buffer.
      mark_unchanged();
   }
#if THLA_OBJ_DEBUG_VALID_OBJECT_RECEIVE
   else if ( is_instance_handle_valid() && ( exec_get_sim_time() > 0.0 ) ) {
      message_publish( MSG_NORMAL, "Object::receive_zero_lookahead_data():%d NO new data for valid object '%s' at HLA-logical-time=%G\n",
                       __LINE__, get_name(), manager->get_federate()->get_granted_time().get_time_in_seconds() );
   }
#endif
#if THLA_OBJ_DEBUG_RECEIVE
   else {
      message_publish( MSG_NORMAL, "Object::receive_zero_lookahead_data():%d NO new data for '%s' at HLA-logical-time=%G\n",
                       __LINE__, get_name(), manager->get_federate()->get_granted_time().get_time_in_seconds() );
   }
#endif
}

/*!
 * @details If the object is owned remotely, this function copies its internal
 * data into simulation object and marks the object as "unchanged". This data
 * was deposited by the reflect callback and marked as "changed". By marking it
 * as unchanged, we avoid copying the same data over and over. If the object
 * is locally owned, we shouldn't be receiving any remote data anyway and if
 * we were to -- bogusly -- copy it to the internal byte buffer, we'd
 * continually reset our local simulation.
 * @job_class{scheduled}
 */
void Object::receive_blocking_io_data()
{
   // There must be some remotely owned attribute that we subscribe to in
   // order for us to receive it.
   if ( !any_remotely_owned_subscribed_blocking_io_attribute() ) {
      return;
   }

   // Process the data now that it has been received (i.e. changed).
   if ( is_changed() ) {

#if THLA_OBJ_DEBUG_RECEIVE
      message_publish( MSG_NORMAL, "Object::receive_blocking_io_data():%d for '%s' at HLA-logical-time=%G\n",
                       __LINE__, get_name(), manager->get_federate()->get_granted_time().get_time_in_seconds() );
#endif

      // Unpack the buffer and copy the values to the object attributes.
      unpack_blocking_io_attribute_buffers();

      // Unpack the data for the object if we have a packing object.
      if ( packing != NULL ) {
         packing->unpack();
      }

      // Lag-compensation is not supported for blocking I/O, but if
      // specified we call the bypass function.
      if ( lag_comp != NULL ) {
         switch ( lag_comp_type ) {
            case LAG_COMPENSATION_NONE: {
               lag_comp->bypass_receive_lag_compensation();
               break;
            }
            case LAG_COMPENSATION_SEND_SIDE:
            case LAG_COMPENSATION_RECEIVE_SIDE:
            default: {
               ostringstream errmsg;
               errmsg << "Object::receive_blocking_io_data():" << __LINE__
                      << " ERROR: For object '" << name << "', detected a"
                      << " Lag-Compensation 'lag_comp' callback has also been"
                      << " specified with a 'lag_comp_type' setting that is not"
                      << " LAG_COMPENSATION_NONE! Please check your input or"
                      << " modified-data files to make sure the Lag-Compensation"
                      << " type 'lag_comp_type' is set to LAG_COMPENSATION_NONE"
                      << " to disable Lag-Compensation when receiving blocking I/O"
                      << " configured object attributes.\n";
               DebugHandler::terminate_with_message( errmsg.str() );
               break;
            }
         }
      }

      // Mark this data as unchanged now that we have processed it from the buffer.
      mark_unchanged();
   }
#if THLA_OBJ_DEBUG_VALID_OBJECT_RECEIVE
   else if ( is_instance_handle_valid() && ( exec_get_sim_time() > 0.0 ) ) {
      message_publish( MSG_NORMAL, "Object::receive_blocking_io_data():%d NO new data for valid object '%s' at HLA-logical-time=%G\n",
                       __LINE__, get_name(), manager->get_federate()->get_granted_time().get_time_in_seconds() );
   }
#endif
#if THLA_OBJ_DEBUG_RECEIVE
   else {
      message_publish( MSG_NORMAL, "Object::receive_blocking_io_data():%d NO new data for '%s' at HLA-logical-time=%G\n",
                       __LINE__, get_name(), manager->get_federate()->get_granted_time().get_time_in_seconds() );
   }
#endif
}

/*!
 * @job_class{scheduled}
 */
void Object::send_init_data()
{
   // We can only send init attribute updates for the attributes we own and are
   // configured to publish.
   if ( !any_locally_owned_published_init_attribute() ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   Federate const *federate = get_federate();
   RTIambassador  *rti_amb  = get_RTI_ambassador();

   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &send_mutex );

   // Lag-compensation is not supported for init-data, but if
   // specified we call the bypass function.
   if ( lag_comp != NULL ) {
      lag_comp->bypass_send_lag_compensation();
   }

   // If we have a data packing object then pack the data now.
   if ( packing != NULL ) {
      packing->pack();
   }

   // Buffer the attribute values for the object.
   pack_init_attribute_buffers();

   try {
      // Create the map of "initialize" attribute values we will be updating.
      create_attribute_set( CONFIG_INITIALIZE );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::send_init_data():%d For object '%s', can not create attribute value/pair set: '%s'\n",
                       __LINE__, get_name(), rti_err_msg.c_str() );
   }

   try {
      // Do not send any data if federate save / restore has begun (see
      // IEEE-1516.1-2010 sections 4.12, 4.20)
      if ( federate->should_publish_data() ) {

         if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            message_publish( MSG_NORMAL, "Object::send_init_data():%d For object '%s', updating attribute values as Receive Order.\n",
                             __LINE__, get_name() );
         }

         // Send the Attributes to the federation. This call returns an
         // event retraction handle but we don't support event retraction
         // so no need to store it.
         rti_amb->updateAttributeValues( this->instance_handle,
                                         *attribute_values_map,
                                         RTI1516_USERDATA( 0, 0 ) );
#ifdef THLA_CHECK_SEND_AND_RECEIVE_COUNTS
         ++send_count;
#endif
      }
   } catch ( InvalidLogicalTime const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::send_init_data():%d invalid logical time exception for '%s' with error message '%s'.\n",
                       __LINE__, get_name(), rti_err_msg.c_str() );

      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: InvalidLogicalTime\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << " ("
             << get_granted_time().get_base_time() << " " << Int64BaseTime::get_units()
             << ")\n"
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << " ("
             << get_lookahead().get_base_time() << " " << Int64BaseTime::get_units()
             << ")\n";
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( AttributeNotOwned const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_init_data():%d detected remote ownership for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: AttributeNotOwned\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( ObjectInstanceNotKnown const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_init_data():%d object instance not known for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: ObjectInstanceNotKnown\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( AttributeNotDefined const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_init_data():%d attribute not defined for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: AttributeNotDefined\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( FederateNotExecutionMember const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_init_data():%d federation not execution member for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: FederateNotExecutionMember\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( SaveInProgress const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_init_data():%d save in progress for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: SaveInProgress\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( RestoreInProgress const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_init_data():%d restore in progress for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: RestoreInProgress\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( NotConnected const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_init_data():%d not connected error for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: NotConnected\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( RTIinternalError const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      message_publish( MSG_WARNING, "Object::send_init_data():%d RTI internal error for '%s'\n",
                       __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: RTIinternalError\n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object.send_init_data():%d Exception: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: \n"
             << "  instance_id=" << id_str << '\n'
             << "  granted=" << get_granted_time().get_time_in_seconds() << '\n'
             << "  lookahead=" << get_lookahead().get_time_in_seconds() << '\n';
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @details If the object is owned remotely, this function copies its internal
 * data into simulation object and marks the object as "unchanged". This data
 * was deposited by the reflect callback and marked as "changed". By marking
 * it as unchanged, we avoid copying the same data over and over. If the
 * object is locally owned, we shouldn't be receiving any remote data anyway
 * and if we were to -- bogusly -- copy it to the internal byte buffer, we'd
 * continually reset our local simulation.
 * @job_class{scheduled}
 */
void Object::receive_init_data()
{
   // There must be some remotely owned attribute that we subscribe to in
   // order for us to receive it.
   if ( !any_remotely_owned_subscribed_init_attribute() ) {
      return;
   }

   if ( is_changed() ) {

#if THLA_OBJ_DEBUG_RECEIVE
      message_publish( MSG_NORMAL, "Object::receive_init_data():%d for '%s'\n",
                       __LINE__, get_name() );
#endif

      // Unpack the buffer and copy the values to the object attributes.
      unpack_init_attribute_buffers();

      // Unpack the data for the object if we have a packing object.
      if ( packing != NULL ) {
         packing->unpack();
      }

      // Lag-compensation is not supported for init-data, but if
      // specified we call the bypass function.
      if ( lag_comp != NULL ) {
         lag_comp->bypass_receive_lag_compensation();
      }

      // Mark the data as unchanged now that we have unpacked the buffer.
      mark_unchanged();
   }
#if THLA_OBJ_DEBUG_RECEIVE
   else {
      message_publish( MSG_NORMAL, "Object::receive_init_data():%d NO initialization data for '%s'\n",
                       __LINE__, get_name() );
   }
#endif
}

/*!
 * @job_class{scheduled}
 */
void Object::create_requested_attribute_set()
{
   // Make sure we clear the map before we populate it.
   if ( !attribute_values_map->empty() ) {
      attribute_values_map->clear();
   }

   for ( int i = 0; i < attr_count; ++i ) {

      // Only include attributes that have been requested, we own, and we publish.
      if ( attributes[i].is_update_requested()
           && attributes[i].is_locally_owned()
           && attributes[i].is_publish() ) {

         // If there was a requested update for this attribute make sure
         // we clear the request flag.
         attributes[i].set_update_requested( false );

         if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            message_publish( MSG_NORMAL, "Object::create_requested_attribute_set():%d Adding '%s' to attribute map.\n",
                             __LINE__, attributes[i].get_FOM_name() );
         }
         // Create the Attribute-Value from the buffered data.
         ( *attribute_values_map )[attributes[i].get_attribute_handle()] = attributes[i].get_attribute_value();
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void Object::create_attribute_set(
   DataUpdateEnum const required_config,
   bool const           include_requested )
{
   // Make sure we clear the map before we populate it.
   if ( !attribute_values_map->empty() ) {
      attribute_values_map->clear();
   }

   // If the cyclic bit is set in the required-configuration then we need
   // to check to make sure the sub-rate is ready to send flag is set for
   // each attribute.
   if ( ( required_config & CONFIG_CYCLIC ) == CONFIG_CYCLIC ) {
      for ( int i = 0; i < attr_count; ++i ) {

         // Only include attributes that have the required configuration,
         // we own, we publish, and the sub-rate says we are ready to
         // send or the attribute has been requested.
         if ( attributes[i].is_locally_owned()
              && attributes[i].is_publish()
              && ( ( include_requested && attributes[i].is_update_requested() )
                   || ( attributes[i].is_data_cycle_ready()
                        && ( ( attributes[i].get_configuration() & required_config ) == required_config ) ) ) ) {

            // If there is no sub-classed TrickHLA-Conditional object for this
            // attribute or if the sub-classed Conditional object indicates that
            // it should be sent, then add this attribute into the attribute
            // map. NOTE: Override the Conditional if the attribute has been
            // requested by another Federate to make sure it is sent.
            if ( ( conditional == NULL )
                 || conditional->should_send( &attributes[i] )
                 || ( include_requested && attributes[i].is_update_requested() ) ) {

               // If there was a requested update for this attribute make sure
               // we clear the request flag now since we are handling it here.
               attributes[i].set_update_requested( false );

               if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                  message_publish( MSG_NORMAL, "Object::create_attribute_set():%d For cyclic object '%s', adding '%s' to attribute map.\n",
                                   __LINE__, get_name(), attributes[i].get_FOM_name() );
               }
               // Create the Attribute-Value from the buffered data.
               ( *attribute_values_map )[attributes[i].get_attribute_handle()] =
                  attributes[i].get_attribute_value();
            }
         }
      }
   } else if ( ( required_config & CONFIG_ZERO_LOOKAHEAD ) == CONFIG_ZERO_LOOKAHEAD ) {
      for ( int i = 0; i < attr_count; ++i ) {

         // Only include attributes that have the required configuration,
         // we own, we publish, or the attribute has been requested.
         if ( attributes[i].is_locally_owned()
              && attributes[i].is_publish()
              && ( ( include_requested && attributes[i].is_update_requested() )
                   || ( ( attributes[i].get_configuration() & required_config ) == required_config ) ) ) {

            // If there is no sub-classed TrickHLA-Conditional object for this
            // attribute or if the sub-classed Conditional object indicates that
            // it should be sent, then add this attribute into the attribute
            // map. NOTE: Override the Conditional if the attribute has been
            // requested by another Federate to make sure it is sent.
            if ( ( conditional == NULL )
                 || conditional->should_send( &attributes[i] )
                 || ( include_requested && attributes[i].is_update_requested() ) ) {

               // If there was a requested update for this attribute make sure
               // we clear the request flag now since we are handling it here.
               attributes[i].set_update_requested( false );

               if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                  message_publish( MSG_NORMAL, "Object::create_attribute_set():%d For object '%s', adding '%s' to attribute map.\n",
                                   __LINE__, get_name(), attributes[i].get_FOM_name() );
               }
               // Create the Attribute-Value from the buffered data.
               ( *attribute_values_map )[attributes[i].get_attribute_handle()] =
                  attributes[i].get_attribute_value();
            }
         }
      }
   } else {
      for ( int i = 0; i < attr_count; ++i ) {

         // Only include attributes that have the required configuration,
         // we own, and we publish.
         if ( attributes[i].is_locally_owned()
              && attributes[i].is_publish()
              && ( ( attributes[i].get_configuration() & required_config ) == required_config ) ) {

            // If there was a requested update for this attribute make
            // sure we clear the request flag now since we are handling
            // it here.
            attributes[i].set_update_requested( false );

            // Create the Attribute-Value from the buffered data.
            ( *attribute_values_map )[attributes[i].get_attribute_handle()] =
               attributes[i].get_attribute_value();
         }
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void Object::enqueue_data(
   AttributeHandleValueMap const &theAttributes )
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &receive_mutex );

   thla_reflected_attributes_queue.push( theAttributes );
}

/*!
 * @details This routine is called by the federate ambassador when new
 * attribute values come in for this object.
 * @job_class{scheduled}
 */
bool Object::extract_data(
   AttributeHandleValueMap &theAttributes )
{
   // We need to iterate through the AttributeHandleValuePairSet
   // to extract each AttributeHandleValuePair. Based on the type
   // specified ( the value returned by getHandle() ) we need to
   // extract the data from the buffer that is returned by
   // getValue().

   if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_ATTRIBUTE ) ) {
      message_publish( MSG_NORMAL, "Object::extract_data():%d '%s' FOM-name:'%s'.\n",
                       __LINE__, get_name(), get_FOM_name() );
   }

   bool any_attr_received = false;

   AttributeHandleValueMap::iterator iter;

   for ( iter = theAttributes.begin(); iter != theAttributes.end(); ++iter ) {

      // Get the TrickHLA Attribute for the given AttributeHandle from the
      // attribute-handle-value map.
      // NOTE: The value of iter->first is of type AttributeHandle.
      Attribute *attr = get_attribute( iter->first );

      // Determine if this object has this attribute.
      if ( attr != NULL ) {

         // Place the RTI AttributeValue into the TrickHLA Attribute.
         if ( attr->extract_data( &( iter->second ) ) ) {
            any_attr_received = true;
         }
      } else if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         string id_str;
         StringUtilities::to_string( id_str, iter->first );
         message_publish( MSG_WARNING, "Object::extract_data():%d WARNING: For \
Object '%s' with FOM name '%s', data was received for Attribute-ID:%s, which \
has not been configured for this object instance in the input.py file. Ignoring \
this attribute.\n",
                          __LINE__, name, FOM_name, id_str.c_str() );
      }
   }

   // Set the change flag once all the attributes have been processed.
   if ( any_attr_received ) {
      // Mark the data as changed since at least one attribute was received.
      mark_changed();

      // Flag for user use to indicate the data changed.
      this->data_changed = true;
   }
   return any_attr_received;
}

/*!
 * @job_class{scheduled}
 */
void Object::release_ownership()
{
   // If divestiture request hasn't come from RTI ambassador, skip.
   if ( !this->divest_requested ) {
      return;
   }

   // Make sure we have an Instance ID for the object, if we don't then return.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::release_ownership():%d Object-Instance-Handle not set for '%s'.\n",
                       __LINE__, get_name() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Object::release_ownership():%d Unexpected NULL RTIambassador.\n",
                       __LINE__ );
      return;
   }

#if THLA_OBJ_OWNERSHIP_DEBUG
   message_publish( MSG_NORMAL, "Object::release_ownership():%d Attributes of Object '%s'.\n",
                    __LINE__, get_name() );
#endif

   AttributeHandleSet attrs;

   // Now do one last check of the divist_requested flag with thread safety and
   // use braces to create scope for the mutex-protection to auto unlock the mutex.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &ownership_mutex );

      if ( ownership != NULL ) {
         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            message_publish( MSG_NORMAL, "Object::release_ownership():%d Telling ownership handler to clear checkpoint.\n",
                             __LINE__ );
         }
         ownership->free_checkpoint();
      }

      if ( !this->divest_requested ) {
#if THLA_OBJ_OWNERSHIP_DEBUG
         message_publish( MSG_NORMAL, "Object::release_ownership():%d NOTE: Another thread beat us to release Attributes of Object '%s'.\n",
                          __LINE__, get_name() );
#endif

         // Another thread beat us to release the Attributes and is processing the
         // divest request so just return.
         return;
      }

      // Create the list of attributes we can divest ownership of.
      for ( int i = 0; i < attr_count; ++i ) {
         // Only release ownership of the attributes we locally own.
         if ( attributes[i].is_divest_requested() && attributes[i].is_locally_owned() ) {
            attrs.insert( attributes[i].get_attribute_handle() );
         }

         // Clear the flag now that the attribute has been handled.
         attributes[i].set_divest_requested( false );
      }

      // Clear the flag now that we are servicing the divest request.
      this->divest_requested = false;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      message_publish( MSG_NORMAL, "Object::release_ownership():%d Attributes of Object '%s'.\n",
                       __LINE__, get_name() );
   }

   try {
      // IEEE 1516.1-2010 section 7.6
      rti_amb->confirmDivestiture( this->instance_handle,
                                   attrs,
                                   RTI1516_USERDATA( 0, 0 ) );

      AttributeHandleSet::iterator divest_iter;

      MutexProtection auto_unlock_mutex( &ownership_mutex );

      // For the attributes we just divested, mark them as being remotely owned.
      for ( divest_iter = attrs.begin(); divest_iter != attrs.end(); ++divest_iter ) {

         Attribute *trick_hla_attr = get_attribute( *divest_iter );

         if ( trick_hla_attr != NULL ) {

            // The attribute is now divested which means it is now remotely owned.
            trick_hla_attr->mark_remotely_owned();

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::release_ownership():%d\
\n   DIVESTED Ownership of attribute '%s'->'%s' of object '%s'.\n",
                                __LINE__,
                                get_FOM_name(), trick_hla_attr->get_FOM_name(),
                                get_name() );
            }
         }
      }
   } catch ( ObjectInstanceNotKnown const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated ObjectInstanceNotKnown: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( AttributeNotDefined const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated AttributeNotDefined: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( AttributeNotOwned const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated AttributeNotOwned: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( AttributeDivestitureWasNotRequested const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated AttributeDivestitureWasNotRequested: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( NoAcquisitionPending const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated NoAcquisitionPending: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( FederateNotExecutionMember const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated FederateNotExecutionMember: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( SaveInProgress const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated SaveInProgress: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( RestoreInProgress const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated RestoreInProgress: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( NotConnected const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated NotConnected: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( RTIinternalError const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated RTIinternalError: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   } catch ( RTI1516_EXCEPTION const &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      message_publish( MSG_WARNING, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated Exception: '%s'\n",
                       __LINE__, rti_err_msg.c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void Object::pull_ownership()
{
   // Just return if we don't have any pull requests or an ownership object.
   if ( ( ownership == NULL ) || ownership->pull_requests.empty() ) {
      return;
   }

   // Make sure we have an Instance ID for the object, otherwise just return.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::pull_ownership():%d Object-Instance-Handle not set for '%s'.\n",
                       __LINE__, get_name() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();

   // We need an RTI ambassador to be able to continue.
   if ( rti_amb == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::pull_ownership():%d Unexpected Null RTIambassador!\n",
                          __LINE__ );
      }
      return;
   }

   double    current_time = 0.0;
   Federate *federate     = get_federate();

   // Use the HLA Federated Logical Time if time management is enabled.
   if ( federate->is_time_management_enabled() ) {

      try {
         HLAinteger64Time HLAtime;
         rti_amb->queryLogicalTime( HLAtime );
         Int64Time fedTime = Int64Time( (int64_t)HLAtime.getTime() );

         // Get the current HLA logical time.
         current_time = fedTime.get_time_in_seconds();
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Object::pull_ownership():%d EXCEPTION: FederateNotExecutionMember \n", __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::pull_ownership():%d EXCEPTION: SaveInProgress \n", __LINE__ );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::pull_ownership():%d EXCEPTION: RestoreInProgress \n", __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Object::pull_ownership():%d EXCEPTION: NotConnected \n", __LINE__ );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::pull_ownership():%d EXCEPTION: RTIinternalError: '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      // Use the simulation time for the current time.
      current_time = federate->get_execution_control()->get_sim_time();
   }

   THLAAttributeMap                     *attr_map;
   THLAAttributeMap::const_iterator      attr_map_iter;
   AttributeOwnershipMap::const_iterator pull_ownership_iter;

   // The Set of attribute handle to pull ownership of.
   AttributeHandleSet attr_hdl_set;

   // Lock the ownership mutex since we are processing the ownership pull list and
   // use braces to create scope for the mutex-protection to auto unlock the mutex.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &ownership_mutex );

      // Process the pull requests.
      pull_ownership_iter = ownership->pull_requests.begin();
      while ( pull_ownership_iter != ownership->pull_requests.end() ) {

         // Make the iterator value easier to work with and understand what it is.
         double pull_time = pull_ownership_iter->first;

         if ( current_time >= pull_time ) {

            // Make the iterator value easier to understand what it is.
            attr_map = pull_ownership_iter->second;

            // Process the map of attributes we desire to pull ownership for.
            for ( attr_map_iter = attr_map->begin();
                  attr_map_iter != attr_map->end(); ++attr_map_iter ) {

               Attribute const *attr = attr_map_iter->second;

               // Determine if attribute ownership can be pulled.
               if ( attr->is_remotely_owned() && attr->is_publish() ) {

                  // We will try and pull ownership of this attribute.
                  attr_hdl_set.insert( attr->get_attribute_handle() );

                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     message_publish( MSG_NORMAL, "Object::pull_ownership():%d\
\n   Attribute '%s'->'%s' of object '%s'.\n",
                                      __LINE__, get_FOM_name(),
                                      attr->get_FOM_name(), get_name() );
                  }
               } else {
                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     message_publish( MSG_NORMAL, "Object::pull_ownership():%d Can not \
pull Attribute '%s'->'%s' of object '%s' for time %G because it is either already \
owned or is not configured to be published.\n",
                                      __LINE__, get_FOM_name(),
                                      attr->get_FOM_name(), get_name(), pull_time );
                  }
               }
            }

            // Erase the Attribute Map for the given pull-time from the pull
            // requests now that we have processed it.
            ownership->pull_requests.erase( pull_time );

            // Point to the start of the iterator since we just erased an entry.
            pull_ownership_iter = ownership->pull_requests.begin();

            // We are done with the Attribute Map for the given pull-time so clear
            // it and delete it.
            attr_map->clear();
            delete attr_map;
         } else {
            // Point to the next item in the pull ownership iterator.
            ++pull_ownership_iter;
         }
      }
   }

   // Make the request only if we have attributes to pull ownership of.
   if ( attr_hdl_set.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::pull_ownership():%d No active requests, \
%d scheduled request(s) pending for object '%s'.\n",
                          __LINE__,
                          (int)ownership->pull_requests.size(), get_name() );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::pull_ownership():%d Pulling ownership \
for Attributes of object '%s'.\n",
                          __LINE__, get_name() );
      }

      try {
         // IEEE 1516.1-2010 section 7.8
         rti_amb->attributeOwnershipAcquisition(
            this->instance_handle,
            attr_hdl_set,
            RTI1516_USERDATA( get_name(), strlen( get_name() ) + 1 ) );

      } catch ( RTI1516_EXCEPTION const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::pull_ownership():%d Unable to pull attributes of \
object '%s' because of error: '%s'\n",
                          __LINE__, get_name(), rti_err_msg.c_str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
}

/*!
 * @brief Pull ownership of the named object instance at initialization.
 * @param attribute_list Comma separated list of attributes FOM names.
 * */
void Object::pull_ownership_at_init(
   char const *attribute_list )
{
   // Make sure we have an Instance ID for the object.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::pull_ownership_at_init():%d Object-Instance-Handle not set for '%s'.\n",
                       __LINE__, get_name() );
      return;
   }

   // Determine the list of attribute names to pull ownership.
   VectorOfStrings attr_name_vector;
   if ( attribute_list == NULL ) {
      // Add all the attributes.
      for ( int i = 0; i < attr_count; ++i ) {
         attr_name_vector.push_back( attributes[i].get_FOM_name() );
      }
   } else {
      // Parse the user supplied list of attribute FOM names to pull ownership.
      StringUtilities::tokenize( attribute_list, attr_name_vector, "," );
   }

   if ( attr_name_vector.empty() ) {
      ostringstream errmsg;
      errmsg << "Object::pull_ownership_at_init():" << __LINE__
             << " ERROR: No attributes found to push ownership for object '"
             << get_name_string() << "'!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // The Set of attribute handle to pull ownership of.
   AttributeHandleSet attr_hdl_set;

   // Lock the ownership mutex since we are processing the ownership pull
   // list and use braces to create scope for the mutex-protection to auto
   // unlock the mutex.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &ownership_mutex );

      for ( int i = 0; i < (int)attr_name_vector.size(); ++i ) {
         Attribute const *attr = get_attribute( attr_name_vector[i] );

         if ( attr == NULL ) {
            ostringstream errmsg;
            errmsg << "Object::pull_ownership_at_init():" << __LINE__
                   << " ERROR: For object '" << get_name_string()
                   << "', no TrickHLA-Attribute found for attribute FOM name '"
                   << attr_name_vector[i] << "'!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return;
         }

         // Determine if attribute ownership can be pulled.
         if ( !attr->is_locally_owned() ) {

            // We are will try and push ownership of this attribute.
            attr_hdl_set.insert( attr->get_attribute_handle() );

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::pull_ownership_at_init():%d\n   Attribute '%s'->'%s' of object '%s'.\n",
                                __LINE__, get_FOM_name(),
                                attr->get_FOM_name(), get_name() );
            }
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::pull_ownership_at_init():%d Can not \
push Attribute '%s'->'%s' of object '%s' because it is already remotely owned.\n",
                                __LINE__, get_FOM_name(), attr->get_FOM_name(), get_name() );
            }
         }
      }
   }

   // Determine if we have any attributes to pull ownership of.
   if ( attr_hdl_set.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::pull_ownership_at_init():%d No locally owned attributes to push ownership for object '%s'.\n",
                          __LINE__, get_name() );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::pull_ownership_at_init():%d Pushing ownership for Attributes of object '%s'.\n",
                          __LINE__, get_name() );
      }

      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      RTIambassador *rti_amb = get_RTI_ambassador();

      // We need an RTI ambassador to be able to continue.
      if ( rti_amb == NULL ) {
         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            message_publish( MSG_NORMAL, "Object::pull_ownership_at_init():%d Unexpected Null RTIambassador!\n",
                             __LINE__ );
         }
         return;
      }

      try {
         // IEEE 1516.1-2010 section 7.8
         rti_amb->attributeOwnershipAcquisition(
            this->instance_handle,
            attr_hdl_set,
            RTI1516_USERDATA( get_name(), strlen( get_name() ) + 1 ) );

      } catch ( RTI1516_EXCEPTION const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::pull_ownership_at_init():%d Unable to pull attributes of \
object '%s' because of error: '%s'\n",
                          __LINE__, get_name(), rti_err_msg.c_str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

      Federate    *federate = get_federate();
      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer;
      bool         acquired;

      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks
         // the mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &ownership_mutex );
         acquired = this->ownership_acquired;
      }

      // The spin lock waits for Ownership Acquisition.
      while ( !acquired ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            MutexProtection auto_unlock_mutex( &ownership_mutex );
            acquired = this->ownership_acquired;
         }

         if ( !acquired ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Object::pull_ownership_at_init():" << __LINE__
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
               message_publish( MSG_NORMAL, "Object::pull_ownership_at_init()%d \"%s\": Waiting for Ownership Acquisition Notification callback...\n",
                                __LINE__, federate->get_federation_name() );
            }
         }
      }

      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks
         // the mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &ownership_mutex );
         this->ownership_acquired = false;
      }
   }
}

/*!
 * @brief Wait to handle the remote request to Pull ownership object
 *  attributes to this federate. */
void Object::handle_pulled_ownership_at_init()
{
   // Make sure we have an Instance ID for the object.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::handle_pulled_ownership_at_init():%d Object-Instance-Handle not set for '%s'.\n",
                       __LINE__, get_name() );
      return;
   }

   Federate    *federate = get_federate();
   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;
   bool         requested;

   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks
      // the mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &ownership_mutex );
      requested = this->pull_requested;
   }

   // The spin lock waits for Ownership Acquisition.
   while ( !requested ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep();

      {
         MutexProtection auto_unlock_mutex( &ownership_mutex );
         requested = this->pull_requested;
      }

      if ( !requested ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Object::handle_pulled_ownership_at_init():" << __LINE__
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
            message_publish( MSG_NORMAL, "Object::handle_pulled_ownership_at_init()%d \"%s\": Waiting for Pull Requested notification...\n",
                             __LINE__, federate->get_federation_name() );
         }
      }
   }

   grant_pull_request();
}

/*!
 * @job_class{scheduled}
 */
void Object::grant_pull_request()
{
   if ( !this->pull_requested ) {
      return;
   }

   // Make sure we have an Instance ID for the object, otherwise just return.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::grant_pull_request():%d Object-Instance-Handle not set for '%s'\n",
                       __LINE__, get_name() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   if ( rti_amb == NULL ) {
      message_publish( MSG_WARNING, "Object::grant_pull_request():%d Unexpected NULL RTIambassador!\n",
                       __LINE__ );
      return;
   }

   AttributeHandleSet attrs_to_divest;

   {
      MutexProtection auto_unlock_mutex( &ownership_mutex );

      // Determine which attributes to grant the pull request for.
      for ( int i = 0; i < attr_count; ++i ) {
         // Another federate is trying to pull ownership so grant the pull
         // request for only the attributes that have a pull request enabled
         // and that we locally own.
         if ( attributes[i].is_pull_requested() && attributes[i].is_locally_owned() ) {
            attrs_to_divest.insert( attributes[i].get_attribute_handle() );
         }

         // Clear the pull requested flag for the attribute.
         attributes[i].set_pull_requested( false );
      }
   }

   if ( attrs_to_divest.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::grant_pull_request():%d No requested attributes to divest ownership of for object '%s'.\n",
                          __LINE__, get_name() );
      }
   } else {

      // List of attributes to divest.
      AttributeHandleSet *divested_attrs = new AttributeHandleSet();

      try {
         // IEEE 1516.1-2010 section 7.12
         rti_amb->attributeOwnershipDivestitureIfWanted( this->instance_handle,
                                                         attrs_to_divest,
                                                         *divested_attrs );

         // Divest ownership only if we have attributes we need to do this for.
         if ( divested_attrs->empty() ) {
            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::grant_pull_request():%d \
No attributes Divested since no federate wanted them for object '%s'.\n",
                                __LINE__, get_name() );
            }
         } else {
            MutexProtection auto_unlock_mutex( &ownership_mutex );

            // Process the list of attributes that were divisted by the RTI
            // and set the state of the ownership.
            AttributeHandleSet::iterator divested_iter;
            for ( divested_iter = divested_attrs->begin();
                  divested_iter != divested_attrs->end(); ++divested_iter ) {

               // Get the attribute associated with the Attribute Handle.
               Attribute *trick_hla_attr = get_attribute( *divested_iter );

               // Mark the attribute as remotely owned since it was divested.
               if ( trick_hla_attr != NULL ) {

                  trick_hla_attr->mark_remotely_owned();

                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     message_publish( MSG_NORMAL, "Object::grant_pull_request():%d\
\n   DIVESTED Ownership for attribute '%s'->'%s' of object '%s'.\n",
                                      __LINE__,
                                      get_FOM_name(), trick_hla_attr->get_FOM_name(),
                                      get_name() );
                  }
               }
            }
         }
      } catch ( RTI1516_EXCEPTION const &e ) {
         message_publish( MSG_WARNING, "Object::grant_pull_request():%d Unable to grant \
pull request for Trick-HLA-Object '%s'\n",
                          __LINE__, get_name() );
      }

      // Make sure we delete the attribute divest list.
      divested_attrs->clear();
      delete divested_attrs;

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }

   {
      MutexProtection auto_unlock_mutex( &ownership_mutex );

      // Clear the flag now that the pull request has been serviced.
      this->pull_requested = false;
   }
}

/*!
 * @brief The function that runs in the push P-thread that handles the push
 * request grant.
 * @details This function is local to this file and is NOT part of the class.
 * @return Void pointer and is always NULL.
 * @param arg Arguments list.
 * @job_class{scheduled}
 */
void *grant_push_pthread_function(
   void *arg )
{
   Object *pushThreadTHLAObj = static_cast< Object * >( arg );
   pushThreadTHLAObj->grant_push_request();
   pthread_exit( NULL );
   return ( NULL );
}

/*!
 * @job_class{scheduled}
 */
void Object::grant_push_request_pthread()
{
   pthread_t push;

   int ret = pthread_create( &push, NULL, grant_push_pthread_function, this );

   if ( ret ) {
      message_publish( MSG_NORMAL, "Object::grant_push_request_pthread():%d Failed to create a thread!\n",
                       __LINE__ );
      exit( 0 );
   }
}

/*!
 * @job_class{scheduled}
 */
void Object::grant_push_request()
{
   // Make sure we have an Instance ID for the object, otherwise just return.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::grant_push_request():%d Object-Instance-Handle not set for '%s'.\n",
                       __LINE__, get_name() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   if ( rti_amb != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::grant_push_request():%d Granting push request for '%s'.\n",
                          __LINE__, get_name() );
      }

      AttributeHandleSet attrs;

      // To make the state of the attribute push_requested thread safe we
      // lock the mutex now.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &push_mutex );

         // Another federate is trying to push the attribute ownership to us so
         // determine which attributes we will take ownership of.
         for ( int i = 0; i < attr_count; ++i ) {
            // Add the attribute to the list of attributes we will accept ownership
            // of provided the attribute is marked as one the other federate is
            // trying to push to us and the attribute is not already owned by us.
            if ( attributes[i].is_push_requested() && attributes[i].is_remotely_owned() ) {
               attrs.insert( attributes[i].get_attribute_handle() );
            }

            // Clear the push request flag since the attribute will be processed.
            attributes[i].set_push_requested( false );
         }
         // Release the lock on the mutex when auto_unlock_mutex goes out of scope.
      }

      // Acquire the attribute ownership if there is at least one in the set.
      if ( !attrs.empty() ) {
         try {

            // IEEE 1516.1-2010 section 7.8
            rti_amb->attributeOwnershipAcquisition(
               this->instance_handle,
               attrs,
               RTI1516_USERDATA( get_name(), strlen( get_name() ) + 1 ) );
         } catch ( FederateOwnsAttributes const &e ) {
            //            set_locally_owned();

            message_publish( MSG_NORMAL, "Object::grant_push_request():%d Already owns \
the attribute for object '%s'.\n",
                             __LINE__, get_name() );
         } catch ( RTI1516_EXCEPTION const &e ) {
            message_publish( MSG_WARNING, "Object::grant_push_request():%d Unable to grant \
push request for object '%s'.\n",
                             __LINE__, get_name() );
         }

      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            message_publish( MSG_NORMAL, "Object::grant_push_request():%d No attributes \
available to acquire ownership for object '%s'.\n",
                             __LINE__, get_name() );
         }
      }
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @brief The function that runs in the ownership divestiture P-thread that
 * handles ownership transfer.
 * @details This function is local to this file and is NOT part of the class.
 * @return Void pointer and is always NULL.
 * @param arg Arguments list.
 * @job_class{scheduled}
 */
void *ownership_divestiture_pthread_function(
   void *arg )
{
   if ( arg != NULL ) {
      DivestThreadArgs *divest_thread_args = reinterpret_cast< DivestThreadArgs * >( arg );

#if THLA_OBJ_OWNERSHIP_DEBUG
      message_publish( MSG_NORMAL, "====== Object::ownership_divestiture_pthread_function():%d \
calling negotiated_attribute_ownership_divestiture()\n",
                       __LINE__ );
#endif

      // Divest ownership of the specified set of attribute handles.
      divest_thread_args->trick_hla_obj->negotiated_attribute_ownership_divestiture(
         divest_thread_args->handle_set );

#if THLA_OBJ_OWNERSHIP_DEBUG
      message_publish( MSG_NORMAL, "====== Object::ownership_divestiture_pthread_function():%d \
returned from calling negotiated_attribute_ownership_divestiture()\n",
                       __LINE__ );
#endif

      delete divest_thread_args;
   }
   pthread_exit( NULL );
   return ( NULL );
}

/*!
 * @job_class{scheduled}
 */
void Object::negotiated_attribute_ownership_divestiture(
   AttributeHandleSet *attr_hdl_set )
{
   if ( attr_hdl_set == NULL ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();

   // We need an RTI ambassador to be able to continue.
   if ( rti_amb != NULL ) {
      try {
         // IEEE 1516.1-2010 section 7.3
         rti_amb->negotiatedAttributeOwnershipDivestiture(
            this->instance_handle,
            *attr_hdl_set,
            RTI1516_USERDATA( get_name(), strlen( get_name() ) + 1 ) );
      } catch ( ObjectInstanceNotKnown const &e ) {
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d ObjectInstanceNotKnown \n", __LINE__ );
      } catch ( AttributeNotDefined const &e ) {
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d AttributeNotDefined \n", __LINE__ );
      } catch ( AttributeNotOwned const &e ) {
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d AttributeNotOwned \n", __LINE__ );

         // TODO: Need to handle this exception, however every effort has been
         // made to make sure this does not happen.

         //            set_remotely_owned();
         // TODO: Determine if we need to set all attributes as remotely owned
         // if we get this exception? DDexter 6/26/2006
      } catch ( AttributeAlreadyBeingDivested const &e ) {
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d AttributeAlreadyBeingDivested \n", __LINE__ );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d FederateNotExecutionMember \n", __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d SaveInProgress \n", __LINE__ );
      } catch ( RestoreInProgress const &e ) {
         // Be patient. Evidently it's in progress already (perhaps due to
         // a previous invocation of this function).
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d RestoreInProgress \n", __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d NotConnected \n", __LINE__ );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d RTIinternalError: '%s'\n",
                          __LINE__, rti_err_msg.c_str() );
      } catch ( RTI1516_EXCEPTION const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::negotiated_attribute_ownership_divestiture():%d Unable to push for '%s': '%s'\n",
                          __LINE__, get_name(), rti_err_msg.c_str() );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }

   // Make sure we clean up the memory used by the attribute handle set.
   attr_hdl_set->clear();
   delete attr_hdl_set;
}

/*!
 * @job_class{scheduled}
 */
void Object::push_ownership()
{
   // Just return if we don't have any push requests or an ownership object.
   if ( ( ownership == NULL ) || ownership->push_requests.empty() ) {
      return;
   }

   // Make sure we have an Instance ID for the object.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::push_ownership():%d Object-Instance-Handle not set for '%s'.\n",
                       __LINE__, get_name() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();

   // We need an RTI ambassador to be able to continue.
   if ( rti_amb == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::push_ownership():%d Unexpected Null RTIambassador!\n",
                          __LINE__ );
      }
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      return;
   }

   // Get the current time.
   double current_time = 0.0;

   Federate *federate = get_federate();

   // Use the HLA Logical Time if time management is enabled.
   if ( federate->is_time_management_enabled() ) {

      try {
         HLAinteger64Time HLAtime;
         rti_amb->queryLogicalTime( HLAtime );
         Int64Time fedTime = Int64Time( (int64_t)HLAtime.getTime() );

         // Get the current HLA time value.
         current_time = fedTime.get_time_in_seconds();
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Object::push_ownership():%d EXCEPTION: FederateNotExecutionMember \n", __LINE__ );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::push_ownership():%d EXCEPTION: SaveInProgress \n", __LINE__ );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::push_ownership():%d EXCEPTION: RestoreInProgress \n", __LINE__ );
      } catch ( NotConnected const &e ) {
         message_publish( MSG_WARNING, "Object::push_ownership():%d EXCEPTION: NotConnected \n", __LINE__ );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::push_ownership():%d EXCEPTION: RTIinternalError: '%s'.\n",
                          __LINE__, rti_err_msg.c_str() );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      // Use the simulation time for the current time.
      current_time = federate->get_execution_control()->get_sim_time();
   }

   THLAAttributeMap                     *attr_map;
   THLAAttributeMap::const_iterator      attr_map_iter;
   AttributeOwnershipMap::const_iterator push_ownership_iter;

   // The Set of attribute handle to push ownership of.
   // NOTE: The ownership divest thread will delete this object when it is
   // done using it.
   AttributeHandleSet *attr_hdl_set = new AttributeHandleSet();

   // Lock the ownership mutex since we are processing the ownership push list and
   // use braces to create scope for the mutex-protection to auto unlock the mutex.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &ownership_mutex );

      // Process the push requests.
      push_ownership_iter = ownership->push_requests.begin();
      while ( push_ownership_iter != ownership->push_requests.end() ) {

         // Make the iterator value easier to work with and understand what it is.
         double push_time = push_ownership_iter->first;

         if ( current_time >= push_time ) {

            // Make the iterator value easier to understand what it is.
            attr_map = push_ownership_iter->second;

            // Process the map of attributes we desire to push ownership for.
            for ( attr_map_iter = attr_map->begin();
                  attr_map_iter != attr_map->end(); ++attr_map_iter ) {

               Attribute const *attr = attr_map_iter->second;

               // Determine if attribute ownership can be pushed.
               if ( attr->is_locally_owned() ) {

                  // We are will try and push ownership of this attribute.
                  attr_hdl_set->insert( attr->get_attribute_handle() );

                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     message_publish( MSG_NORMAL, "Object::push_ownership():%d\
\n   Attribute '%s'->'%s' of object '%s'.\n",
                                      __LINE__, get_FOM_name(),
                                      attr->get_FOM_name(), get_name() );
                  }
               } else {
                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     message_publish( MSG_NORMAL, "Object::push_ownership():%d Can not \
push Attribute '%s'->'%s' of object '%s' for time %G because it is either already \
owned or is not configured to be published.\n",
                                      __LINE__, get_FOM_name(),
                                      attr->get_FOM_name(), get_name(), push_time );
                  }
               }
            }

            // Erase the Attribute Map for the given push-time from the push
            // requests now that we have processed it.
            ownership->push_requests.erase( push_time );

            // Point to the start of the iterator since we just deleted an item.
            push_ownership_iter = ownership->push_requests.begin();

            // We are done with the Attribute Map for the given push-time so clear
            // it and delete it.
            attr_map->clear();
            delete attr_map;
         } else {
            // Point to the next item in the push iterator.
            ++push_ownership_iter;
         }
      }
   }

   // Determine if we have any attributes to push ownership of.
   if ( attr_hdl_set->empty() ) {
      // Make sure we delete the attribute handle set so that we don't have
      // a memory leak.
      delete attr_hdl_set;

      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::push_ownership():%d No active requests, \
%d scheduled request(s) pending for object '%s'.\n",
                          __LINE__,
                          (int)ownership->push_requests.size(), get_name() );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::push_ownership():%d Pushing ownership \
for Attributes of object '%s'.\n",
                          __LINE__, get_name() );
      }

      // Create and populate the structure the thread will use for processing
      // the ownership divestiture. NOTE: The divest ownership thread will
      // delete the args when it is done with it.
      DivestThreadArgs *divest_thread_args = new DivestThreadArgs();
      divest_thread_args->trick_hla_obj    = this;

      // The attr_hdl_set will be deleted by the ownership divestiture thread
      // function when it is done. This avoids a memory leak.
      divest_thread_args->handle_set = attr_hdl_set;

      pthread_t divest;
      int       ret = pthread_create( &divest,
                                      NULL,
                                      ownership_divestiture_pthread_function,
                                      divest_thread_args );
      if ( ret ) {
         ostringstream errmsg;
         errmsg << "Object::push_ownership():" << __LINE__
                << " ERROR: Failed to create ownership divestiture pthread!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }
   }
}

/*!
 * @brief Push ownership of the named object instance at initialization.
 * @param obj_instance_name Object instance name to push ownership
 * of for all attributes.
 * @param attribute_list Comma separated list of attribute FOM names.
 */
void Object::push_ownership_at_init(
   char const *attribute_list )
{
   // Make sure we have an Instance ID for the object.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::push_ownership_at_init():%d Object-Instance-Handle not set for '%s'.\n",
                       __LINE__, get_name() );
      return;
   }

   // Determine the list of attribute names to push ownership.
   VectorOfStrings attr_name_vector;
   if ( attribute_list == NULL ) {
      // Add all the attributes.
      for ( int i = 0; i < attr_count; ++i ) {
         attr_name_vector.push_back( attributes[i].get_FOM_name() );
      }
   } else {
      // Parse the user supplied list of attribute FOM names to push ownership.
      StringUtilities::tokenize( attribute_list, attr_name_vector, "," );
   }

   if ( attr_name_vector.empty() ) {
      ostringstream errmsg;
      errmsg << "Object::push_ownership_at_init():" << __LINE__
             << " ERROR: No attributes found to push ownership for object '"
             << get_name_string() << "'!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // The Set of attribute handle to push ownership of.
   // NOTE: The ownership divest thread will delete this object when it is
   // done using it.
   AttributeHandleSet *attr_hdl_set = new AttributeHandleSet();

   // Lock the ownership mutex since we are processing the ownership push list and
   // use braces to create scope for the mutex-protection to auto unlock the mutex.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &ownership_mutex );

      for ( int i = 0; i < (int)attr_name_vector.size(); ++i ) {
         Attribute const *attr = get_attribute( attr_name_vector[i] );

         if ( attr == NULL ) {
            delete attr_hdl_set;

            ostringstream errmsg;
            errmsg << "Object::push_ownership_at_init():" << __LINE__
                   << " ERROR: For object '" << get_name_string()
                   << "', no TrickHLA-Attribute found for attribute FOM name '"
                   << attr_name_vector[i] << "'!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
            return;
         }

         // Determine if attribute ownership can be pushed.
         if ( attr->is_locally_owned() ) {

            // We are will try and push ownership of this attribute.
            attr_hdl_set->insert( attr->get_attribute_handle() );

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::push_ownership_at_init():%d\n   Attribute '%s'->'%s' of object '%s'.\n",
                                __LINE__, get_FOM_name(),
                                attr->get_FOM_name(), get_name() );
            }
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::push_ownership_at_init():%d Can not \
push Attribute '%s'->'%s' of object '%s' because it is already remotely owned.\n",
                                __LINE__, get_FOM_name(), attr->get_FOM_name(), get_name() );
            }
         }
      }
   }

   // Determine if we have any attributes to push ownership of.
   if ( attr_hdl_set->empty() ) {
      // Make sure we delete the attribute handle set so that we don't have
      // a memory leak.
      delete attr_hdl_set;

      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::push_ownership_at_init():%d No locally owned attributes to push ownership for object '%s'.\n",
                          __LINE__, get_name() );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::push_ownership_at_init():%d Pushing ownership for Attributes of object '%s'.\n",
                          __LINE__, get_name() );
      }

      // Create and populate the structure the thread will use for processing
      // the ownership divestiture. NOTE: The divest ownership thread will
      // delete the args when it is done with it.
      DivestThreadArgs *divest_thread_args = new DivestThreadArgs();
      divest_thread_args->trick_hla_obj    = this;

      // The attr_hdl_set will be deleted by the ownership divestiture thread
      // function when it is done. This avoids a memory leak.
      divest_thread_args->handle_set = attr_hdl_set;

      pthread_t divest;

      int ret = pthread_create( &divest,
                                NULL,
                                ownership_divestiture_pthread_function,
                                divest_thread_args );
      if ( ret ) {
         ostringstream errmsg;
         errmsg << "Object::push_ownership_at_init():" << __LINE__
                << " ERROR: Failed to create ownership divestiture pthread!\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      Federate *federate = get_federate();

      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer;

      bool divest_req_flag;
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &ownership_mutex );
         divest_req_flag = this->divest_requested;
      }

      // The spin lock waits for divest requested flag to be set ending
      // attribute ownership.
      while ( !divest_req_flag ) {

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         {
            MutexProtection auto_unlock_mutex( &ownership_mutex );
            divest_req_flag = this->divest_requested;
         }

         if ( !divest_req_flag ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Object::push_ownership_at_init():" << __LINE__
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
               message_publish( MSG_NORMAL, "Object::push_ownership_at_init()%d \"%s\": Waiting for Divestiture Confirmation callback...\n",
                                __LINE__, federate->get_federation_name() );
            }
         }
      }

      // Release attribute ownership once we have received the divest callback.
      release_ownership();
   }
}

/*!
 * @brief Wait to handle the remote request to Push ownership object
 *  attributes to this federate.
 *  */
void Object::handle_pushed_ownership_at_init()
{
   // Make sure we have an Instance ID for the object.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::handle_pushed_ownership_at_init():%d Object-Instance-Handle not set for '%s'.\n",
                       __LINE__, get_name() );
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      message_publish( MSG_NORMAL, "Object::handle_pushed_ownership_at_init():%d Object: '%s'.\n",
                       __LINE__, get_name() );
   }

   Federate *federate = get_federate();

   int64_t      wallclock_time;
   SleepTimeout print_timer( federate->wait_status_time );
   SleepTimeout sleep_timer;
   bool         acquired;

   {
      MutexProtection auto_unlock_mutex( &ownership_mutex );
      acquired = this->ownership_acquired;
   }

   // Wait for Ownership Acquisition flag to be set.
   while ( !acquired ) {

      // Check for shutdown.
      federate->check_for_shutdown_with_termination();

      sleep_timer.sleep();

      {
         MutexProtection auto_unlock_mutex( &ownership_mutex );
         acquired = this->ownership_acquired;
      }

      if ( !acquired ) {

         // To be more efficient, we get the time once and share it.
         wallclock_time = sleep_timer.time();

         if ( sleep_timer.timeout( wallclock_time ) ) {
            sleep_timer.reset();
            if ( !federate->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Object::handle_pushed_ownership_at_init():" << __LINE__
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
            message_publish( MSG_NORMAL, "Object::handle_pushed_ownership_at_init()%d \"%s\": Waiting for Ownership Acquisition Notification callback...\n",
                             __LINE__, federate->get_federation_name() );
         }
      }
   }

   {
      MutexProtection auto_unlock_mutex( &ownership_mutex );
      this->ownership_acquired = false;
   }
}

void Object::encode_checkpoint()
{
   if ( ownership != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::encode_checkpoint():%d Object: %s.\n",
                          __LINE__, get_name() );
      }
      ownership->encode_checkpoint();
   }
}

void Object::decode_checkpoint()
{
   if ( ownership != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::decode_checkpoint():%d Object: %s.\n",
                          __LINE__, get_name() );
      }
      ownership->decode_checkpoint();
   }
}

void Object::free_checkpoint()
{
   if ( ownership != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::free_checkpoint():%d Object: %s.\n",
                          __LINE__, get_name() );
      }
      ownership->free_checkpoint();
   }
}

void Object::set_core_job_cycle_time(
   double const cycle_time )
{
   for ( int i = 0; i < attr_count; ++i ) {
      attributes[i].determine_cycle_ratio( cycle_time );
   }
}

void Object::set_name(
   char const *new_name )
{
   // Delete the existing memory used by the name.
   if ( this->name != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( name ) ) ) {
         message_publish( MSG_WARNING, "Object::set_name():%d WARNING failed to delete Trick Memory for 'name'\n",
                          __LINE__ );
      }
   }

   // Allocate appropriate size string and copy data.
   if ( new_name != NULL ) {
      this->name = trick_MM->mm_strdup( new_name );
   } else {
      this->name = trick_MM->mm_strdup( "" );
   }
}

void Object::build_attribute_map()
{
   thla_attribute_map.clear();
   for ( int i = 0; i < attr_count; ++i ) {
      thla_attribute_map[attributes[i].get_attribute_handle()] = &attributes[i];
   }
}

/*! @brief Return a copy of the federate's lookahead time.
 *  @return Lookahead time interval. */
Int64Interval Object::get_lookahead() const
{
   return manager->get_federate()->get_lookahead();
}

/*! @brief Get the currently granted federation HLA logical time.
 *  @return A copy of the granted HLA logical time. */
Int64Time Object::get_granted_time() const
{
   return manager->get_federate()->get_granted_time();
}

Attribute *Object::get_attribute(
   AttributeHandle const &attr_handle )
{
   // We use a map with the key being the AttributeHandle for fast lookups.
   AttributeMap::const_iterator iter = thla_attribute_map.find( attr_handle );
   return ( ( iter != thla_attribute_map.end() ) ? iter->second : NULL );
}

Attribute *Object::get_attribute(
   string const &attr_FOM_name )
{
   string fom_name_str;
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].get_FOM_name() != NULL ) {
         fom_name_str = attributes[i].get_FOM_name();

         if ( attr_FOM_name == fom_name_str ) {
            return ( &attributes[i] );
         }
      }
   }
   return NULL;
}

Attribute *Object::get_attribute(
   char const *attr_FOM_name )
{
   return ( attr_FOM_name != NULL ) ? get_attribute( string( attr_FOM_name ) ) : NULL;
}

void Object::stop_publishing_attributes()
{
   for ( int i = 0; i < attr_count; ++i ) {
      attributes[i].set_publish( false );
   }
}

void Object::stop_subscribing_attributes()
{
   for ( int i = 0; i < attr_count; ++i ) {
      attributes[i].set_subscribe( false );
   }
}

bool Object::any_attribute_published()
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_publish() ) {
         return true;
      }
   }
   return false; // No attribute published.
}

bool Object::any_attribute_subscribed()
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_subscribe() ) {
         return true;
      }
   }
   return false; // No attribute subscribed.
}

bool Object::any_locally_owned_attribute()
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_locally_owned() ) {
         return true;
      }
   }
   return false; // No attribute locally owned.
}

bool Object::any_locally_owned_published_attribute()
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_locally_owned() && attributes[i].is_publish() ) {
         return true;
      }
   }
   return false; // No attribute locally owned and published.
}

bool Object::any_locally_owned_published_attribute(
   DataUpdateEnum const attr_config )
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_locally_owned()
           && attributes[i].is_publish()
           && ( ( attributes[i].get_configuration() & attr_config ) == attr_config ) ) {
         return true;
      }
   }
   return false; // No attribute locally owned, published, for the given config.
}

// WARNING: Only call this function once per data cycle because of how the
// attribute data ready is determined per cycle.
bool Object::any_locally_owned_published_cyclic_data_ready_or_requested_attribute()
{
   bool any_ready = false;
   for ( int i = 0; i < attr_count; ++i ) {

      // We must check that a sub-rate is ready for every attribute to make sure
      // all sub-rate counters get updated correctly.
      bool data_cycle_ready = attributes[i].check_data_cycle_ready();

      if ( !any_ready
           && attributes[i].is_locally_owned()
           && attributes[i].is_publish()
           && ( attributes[i].is_update_requested()
                || ( ( data_cycle_ready
                       && ( ( attributes[i].get_configuration() & CONFIG_CYCLIC ) == CONFIG_CYCLIC ) ) ) ) ) {
         any_ready = true;
      }
   }
   return any_ready;
}

bool Object::any_locally_owned_published_zero_lookahead_or_requested_attribute()
{
   for ( int i = 0; i < attr_count; ++i ) {

      if ( attributes[i].is_locally_owned()
           && attributes[i].is_publish()
           && ( attributes[i].is_update_requested()
                || ( ( attributes[i].get_configuration() & CONFIG_ZERO_LOOKAHEAD ) == CONFIG_ZERO_LOOKAHEAD ) ) ) {
         return true;
      }
   }
   return false;
}

bool Object::any_locally_owned_published_blocking_io_attribute()
{
   for ( int i = 0; i < attr_count; ++i ) {

      if ( attributes[i].is_locally_owned()
           && attributes[i].is_publish()
           && ( ( attributes[i].get_configuration() & CONFIG_BLOCKING_IO ) == CONFIG_BLOCKING_IO ) ) {
         return true;
      }
   }
   return false;
}

// WARNING: Only call this function once per data cycle because of how the
// attribute data ready is determined per cycle.
bool Object::any_locally_owned_published_cyclic_data_ready_attribute()
{
   bool any_ready = false;
   for ( int i = 0; i < attr_count; ++i ) {

      // We must check that a sub-rate is ready for every attribute to make sure
      // all sub-rate counters get updated correctly.
      bool data_cycle_ready = attributes[i].check_data_cycle_ready();

      if ( !any_ready
           && attributes[i].is_locally_owned()
           && attributes[i].is_publish()
           && data_cycle_ready
           && ( ( attributes[i].get_configuration() & CONFIG_CYCLIC ) == CONFIG_CYCLIC ) ) {
         any_ready = true;
      }
   }
   return any_ready;
}

bool Object::any_locally_owned_published_requested_attribute()
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_locally_owned()
           && attributes[i].is_publish()
           && attributes[i].is_update_requested() ) {
         return true;
      }
   }
   return false; // No attribute locally owned, published, and requested.
}

bool Object::any_remotely_owned_subscribed_attribute()
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_remotely_owned() && attributes[i].is_subscribe() ) {
         return true;
      }
   }
   return false; // No attribute remotely owned and subscribed.
}

bool Object::any_remotely_owned_subscribed_attribute(
   DataUpdateEnum const attr_config )
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_remotely_owned()
           && attributes[i].is_subscribe()
           && ( ( attributes[i].get_configuration() & attr_config ) == attr_config ) ) {
         return true;
      }
   }
   return false; // No attribute remotely owned, subscribed for given config.
}

void Object::pack_requested_attribute_buffers()
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( attributes[i].is_update_requested() ) {
         attributes[i].pack_attribute_buffer();
      }
   }
}

void Object::pack_attribute_buffers(
   DataUpdateEnum const attr_config,
   bool const           include_requested )
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( ( include_requested && attributes[i].is_update_requested() )
           || ( attributes[i].get_configuration() & attr_config ) == attr_config ) {
         attributes[i].pack_attribute_buffer();
      }
   }
}

void Object::unpack_attribute_buffers(
   DataUpdateEnum const attr_config )
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( ( attributes[i].get_configuration() & attr_config ) == attr_config ) {
         attributes[i].unpack_attribute_buffer();
      }
   }
}

void Object::set_to_unblocking_cyclic_reads()
{
   this->blocking_cyclic_read       = false;
   this->first_blocking_cyclic_read = true;
}

void Object::set_attribute_ownership_acquired()
{
   MutexProtection auto_unlock_mutex( &ownership_mutex );
   this->ownership_acquired = true;
}

void Object::mark_changed()
{
   MutexProtection auto_unlock_mutex( &receive_mutex );
   this->changed = true;
}

void Object::mark_unchanged()
{
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &receive_mutex );
   this->changed = false;

   // Clear the change flag for each of the attributes as well.
   for ( int i = 0; i < attr_count; ++i ) {
      attributes[i].mark_unchanged();
   }
}

void Object::pull_ownership_upon_rejoin()
{
   // Make sure we have an Instance ID for the object, otherwise just return.
   if ( !is_instance_handle_valid() ) {
      message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d Object-Instance-Handle not set for '%s'.\n",
                       __LINE__, get_name() );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();

   // We need an RTI ambassador to be able to continue.
   if ( rti_amb == NULL ) {
      return;
   }

   // The Set of attribute handle to pull ownership of.
   AttributeHandleSet attr_hdl_set;

   // Lock the ownership mutex since we are processing the ownership push list.
   //
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection auto_unlock_mutex( &ownership_mutex );

   // Force the pull ownership of all attributes....
   for ( int i = 0; i < attr_count; ++i ) {

      try {
         // IEEE 1516.1-2010 section 7.18
         if ( attributes[i].is_publish()
              && attributes[i].is_locally_owned()
              && !rti_amb->isAttributeOwnedByFederate( this->instance_handle, attributes[i].get_attribute_handle() ) ) {

            // RTI tells us that this attribute is not owned by this federate. Add
            // attribute handle to the collection for the impending ownership pull.
            attr_hdl_set.insert( attributes[i].get_attribute_handle() );

            // Turn off the 'locally_owned' flag on this attribute since the RTI
            // just informed us that we do not own this attribute, regardless of
            // what the input.py file may have indicated.
            attributes[i].unmark_locally_owned();

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               message_publish( MSG_NORMAL, "Object::pull_ownership_upon_rejoin():%d \
Ownership check of Attribute '%s'->'%s' from object '%s' => RTI informed us that we DO NOT own it.\n",
                                __LINE__, get_FOM_name(), attributes[i].get_FOM_name(),
                                get_name() );
            }
         }
      } catch ( ObjectInstanceNotKnown const &e ) {
         message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: ObjectInstanceNotKnown \n",
                          __LINE__, attributes[i].get_FOM_name() );
      } catch ( AttributeNotDefined const &e ) {
         message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: AttributeNotDefined \n",
                          __LINE__, attributes[i].get_FOM_name() );
      } catch ( FederateNotExecutionMember const &e ) {
         message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: FederateNotExecutionMember \n",
                          __LINE__, attributes[i].get_FOM_name() );
      } catch ( SaveInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: SaveInProgress \n",
                          __LINE__, attributes[i].get_FOM_name() );
      } catch ( RestoreInProgress const &e ) {
         message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: RestoreInProgress \n",
                          __LINE__, attributes[i].get_FOM_name() );
      } catch ( RTIinternalError const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an RTIinternalError: %s\n",
                          __LINE__, attributes[i].get_FOM_name(), rti_err_msg.c_str() );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }

   // Make the request only if we do have any attributes for which we need to pull ownership.
   if ( attr_hdl_set.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::pull_ownership_upon_rejoin():%d No ownership \
requests were added for object '%s'.\n",
                          __LINE__, get_name() );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         message_publish( MSG_NORMAL, "Object::pull_ownership_upon_rejoin():%d Pulling ownership \
for Attributes of object '%s'.\n",
                          __LINE__, get_name() );
      }

      try {
         // IEEE 1516.1-2010 section 7.8
         rti_amb->attributeOwnershipAcquisition(
            this->instance_handle,
            attr_hdl_set,
            RTI1516_USERDATA( get_name(), strlen( get_name() ) + 1 ) );

      } catch ( RTI1516_EXCEPTION const &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
Unable to pull ownership for the attributes of object '%s' because of error: '%s'\n",
                          __LINE__, get_name(), rti_err_msg.c_str() );
      }

      Federate *federate = get_federate();

      int          i;
      int64_t      wallclock_time;
      SleepTimeout print_timer( federate->wait_status_time );
      SleepTimeout sleep_timer;

      // Perform a blocking loop until ownership of all locally owned published
      // attributes is restored...
      int ownership_counter = 0;
      while ( ownership_counter < (int)attr_hdl_set.size() ) {

         // Reset ownership count for this loop through all the attributes
         ownership_counter = 0;

         for ( i = 0; i < attr_count; ++i ) {
            try {
               // IEEE 1516.1-2010 section 7.18
               if ( attributes[i].is_publish()
                    && attributes[i].is_locally_owned()
                    && rti_amb->isAttributeOwnedByFederate( this->instance_handle, attributes[i].get_attribute_handle() ) ) {
                  ++ownership_counter;
               }
            } catch ( ObjectInstanceNotKnown const &e ) {
               message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: ObjectInstanceNotKnown \n",
                                __LINE__, attributes[i].get_FOM_name() );
            } catch ( AttributeNotDefined const &e ) {
               message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s'generated an EXCEPTION: AttributeNotDefined \n",
                                __LINE__, attributes[i].get_FOM_name() );
            } catch ( FederateNotExecutionMember const &e ) {
               message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: FederateNotExecutionMember \n",
                                __LINE__, attributes[i].get_FOM_name() );
            } catch ( SaveInProgress const &e ) {
               message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: SaveInProgress \n",
                                __LINE__, attributes[i].get_FOM_name() );
            } catch ( RestoreInProgress const &e ) {
               message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: RestoreInProgress \n",
                                __LINE__, attributes[i].get_FOM_name() );
            } catch ( RTIinternalError const &e ) {
               string rti_err_msg;
               StringUtilities::to_string( rti_err_msg, e.what() );
               message_publish( MSG_WARNING, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an RTIinternalError: %s\n",
                                __LINE__, attributes[i].get_FOM_name(),
                                rti_err_msg.c_str() );
            }
         } // end of 'for' loop

         // Check for shutdown.
         federate->check_for_shutdown_with_termination();

         sleep_timer.sleep();

         if ( ownership_counter < (int)attr_hdl_set.size() ) {

            // To be more efficient, we get the time once and share it.
            wallclock_time = sleep_timer.time();

            if ( sleep_timer.timeout( wallclock_time ) ) {
               sleep_timer.reset();
               if ( !federate->is_execution_member() ) {
                  ostringstream errmsg;
                  errmsg << "Object::pull_ownership_upon_rejoin():" << __LINE__
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
               message_publish( MSG_NORMAL, "Object::pull_ownership_upon_rejoin():%d Pulling ownership \
for Attributes of object '%s', waiting...\n",
                                __LINE__, get_name() );
            }
         }
      } // end of 'while' loop
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

bool Object::is_shutdown_called() const
{
   return ( ( this->manager != NULL ) ? manager->is_shutdown_called() : false );
}

void Object::initialize_thread_ID_array()
{
   // If the list of thread IDs was not specified in the input file then clear
   // out the thread ID array if it exists and return.
   if ( this->thread_ids == NULL ) {
      if ( this->thread_ids_array != NULL ) {
         if ( trick_MM->delete_var( static_cast< void * >( this->thread_ids_array ) ) ) {
            message_publish( MSG_WARNING, "Object::initialize_thread_ID_array():%d WARNING failed to delete Trick Memory for 'this->thread_ids_array'\n",
                             __LINE__ );
         }
         this->thread_ids_array       = NULL;
         this->thread_ids_array_count = 0;
      }
      return;
   }

   // Determine the total number of Trick threads (main + child).
   this->thread_ids_array_count = exec_get_num_threads();

   // Protect against the thread count being unexpectedly zero and should be
   // at least 1 for the Trick main thread.
   if ( this->thread_ids_array_count == 0 ) {
      this->thread_ids_array_count = 1;
   }

   // Allocate memory for the data cycle times per each thread.
   this->thread_ids_array = static_cast< bool * >(
      TMM_declare_var_1d( "bool", this->thread_ids_array_count ) );
   if ( this->thread_ids_array == NULL ) {
      ostringstream errmsg;
      errmsg << "Object::initialize_thread_ID_array():" << __LINE__
             << " ERROR: Could not allocate memory for 'thread_ids_array'"
             << " for requested size " << this->thread_ids_array_count
             << "!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }
   for ( unsigned int id = 0; id < this->thread_ids_array_count; ++id ) {
      this->thread_ids_array[id] = false;
   }

   // Break up the comma separated thread-IDs list into a vector.
   std::vector< std::string > thread_id_vec;
   StringUtilities::tokenize( this->thread_ids, thread_id_vec, "," );

   if ( thread_id_vec.empty() ) {

      // If no thread-IDs was specified for this object then default to the
      // Trick main thread.
      this->thread_ids_array[0] = true;

   } else {
      // Process each of the thread-ID's associated to this object and convert
      // from a string to an integer.
      for ( unsigned int k = 0; k < thread_id_vec.size(); ++k ) {

         // Convert the string to an integer.
         stringstream sstream;
         sstream << thread_id_vec[k];
         long long id;
         sstream >> id;

         if ( ( id >= 0 ) && ( id < this->thread_ids_array_count ) ) {
            this->thread_ids_array[id] = true;
         } else {
            ostringstream errmsg;
            errmsg << "Object::initialize_thread_ID_array():" << __LINE__
                   << " ERROR: For object '" << get_name()
                   << "', the Trick child thread-ID '" << thread_id_vec[k]
                   << "' specified in the input file is not valid because this"
                   << " Trick child thread does not exist in the S_define file!"
                   << " Valid Trick thread-ID range is 0 to "
                   << ( this->thread_ids_array_count - 1 )
                   << "!\n";
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      }
   }
}

/*! @brief Determine if this object is associated to the specified thread ID.
 * @return True associated to this thread ID.
 * @param thread_id Trick thread ID. */
bool Object::is_thread_associated(
   unsigned int const thread_id )
{
   if ( ( this->thread_ids_array_count == 0 ) && ( this->thread_ids != NULL ) ) {
      // Initialize the array of thread IDs associated to this object.
      initialize_thread_ID_array();
   }
   if ( this->thread_ids_array_count == 0 ) {
      // If no threads were associated in the input file for this object then
      // by default associate to the main Trick thread.
      return ( thread_id == 0 );
   }
   if ( thread_id >= this->thread_ids_array_count ) {
      return ( false );
   }
   return ( this->thread_ids_array[thread_id] );
}
