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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Int64Interval.cpp}
@trick_link_dependency{Int64Time.cpp}
@trick_link_dependency{LagCompensation.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{Object.cpp}
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
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <sstream>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/release.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/Constants.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
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
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"

// HLA include files.
#include RTI1516_HEADER

using namespace std;
using namespace RTI1516_NAMESPACE;
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
     first_blocking_cyclic_read( true ),
     attr_count( 0 ),
     attributes( NULL ),
     lag_comp( NULL ),
     lag_comp_type( LAG_COMPENSATION_NONE ),
     packing( NULL ),
     ownership( NULL ),
     deleted( NULL ),
     object_deleted_from_RTI( false ),
     mutex(),
     ownership_mutex(),
     clock(),
     name_registered( false ),
     changed( false ),
     attr_update_requested( false ),
     removed_instance( false ),
     any_attribute_FOM_specified_order( false ),
     any_attribute_timestamp_order( false ),
     last_update_time( 0.0 ),
     time_plus_lookahead( 0.0 ),
     pull_requested( false ),
     divest_requested( false ),
     attribute_FOM_names(),
     manager( NULL ),
     rti_ambassador( NULL ),
     thla_reflected_attributes_queue(),
     thla_attribute_map()
{
#ifdef THLA_OBJECT_TIME_LOGGING
   received_gmt_time = 0.0;
#endif

   // Make sure we allocate the map.
   this->attribute_values_map = new AttributeHandleValueMap();

#ifdef THLA_THREAD_WAIT_FOR_DATA
   pthread_mutex_init( &data_change_mutex, NULL );
   pthread_cond_init( &data_change_cv, NULL );
#endif
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

      if ( name != static_cast< char * >( NULL ) ) {
         if ( TMM_is_alloced( name ) ) {
            TMM_delete_var_a( name );
         }
         name = static_cast< char * >( NULL );
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

      // Make sure we destroy the mutex.
      (void)mutex.unlock();
      (void)ownership_mutex.unlock();

#ifdef THLA_THREAD_WAIT_FOR_DATA
      pthread_mutex_destroy( &data_change_mutex );
      pthread_cond_destroy( &data_change_cv );
#endif
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
      send_hs( stderr, "Object::initialize():%d Unexpected NULL Trick-HLA-Manager!%c",
               __LINE__, THLA_NEWLINE );
      exec_terminate( __FILE__, "Object::initialize() Unexpected NULL Trick-HLA-Manager!" );
      return;
   }
   this->manager = trickhla_mgr;

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      ostringstream msg;
      msg << "Object::initialize():" << __LINE__
          << " Name:'" << name << "' FOM_name:'" << FOM_name
          << "' create_HLA_instance:"
          << ( is_create_HLA_instance() ? "True" : "False" ) << THLA_ENDL;
      send_hs( stdout, (char *)msg.str().c_str() );
   }

   // Make sure we have a valid object instance name if the user has indicated
   // that we are creating the HLA instance or if the instance name is required.
   if ( ( is_create_HLA_instance() || is_name_required() ) && ( ( name == NULL ) || ( *name == '\0' ) ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " Missing Object Name. Please check your input or modified-data"
             << " files to make sure the object name is correctly specified."
             << " A valid object instance name is required if you are creating"
             << " an HLA instance of this object (i.e. 'create_HLA_instance'"
             << " field is set to true) or if the 'name_required' field is set"
             << " to true, which is the default." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   } else if ( name == NULL ) {
      // Make sure the name is at least not NULL.
      set_name( "" );
   }

   // Make sure we have a valid object FOM name.
   if ( ( FOM_name == NULL ) || ( *FOM_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " Object '" << name << "' is missing the Object FOM Name."
             << " Please check your input or modified-data files to make sure"
             << " the object FOM name is correctly specified." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // Do a bounds check on the 'lag_comp_type' value.
   if ( ( lag_comp_type < LAG_COMPENSATION_FIRST_VALUE ) || ( lag_comp_type > LAG_COMPENSATION_LAST_VALUE ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " For object '" << name << "', the Lag-Compensation Type"
             << " setting 'lag_comp_type' has a value that is out of the valid"
             << " range of " << LAG_COMPENSATION_FIRST_VALUE << " to "
             << LAG_COMPENSATION_LAST_VALUE << ". Please check your input"
             << " or modified-data files to make sure the 'lag_comp_type' value"
             << " is correctly specified." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // Make sure we have a lag compensation object if lag-compensation is specified.
   if ( ( lag_comp_type != LAG_COMPENSATION_NONE ) && ( lag_comp == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " For object '" << name << "', Lag-Compensation 'lag_comp_type'"
             << " is specified, but 'lag_comp' is NULL! Please check your input"
             << " or modified-data files to make sure the Lag-Compensation type"
             << " and object are correctly specified." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // If we have an attribute count but no attributes then let the user know.
   if ( ( attr_count > 0 ) && ( attributes == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " For object '" << name << "', the 'attr_count' is "
             << attr_count << " but no 'attributes' are"
             << " specified. Please check your input or modified-data files to"
             << " make sure the attributes are correctly specified."
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // If we have attributes but the attribute-count is invalid then let
   // the user know.
   if ( ( attr_count <= 0 ) && ( attributes != NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " For object '" << name << "', the 'attr_count' is "
             << attr_count << " but 'attributes' have been"
             << " specified. Please check your input or modified-data files to"
             << " make sure the attributes are correctly specified."
             << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // If the user specified a packing object then make sure it extends the
   // Packing virtual class.
   if ( ( packing != NULL ) && ( dynamic_cast< Packing * >( packing ) == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " For object '" << name << "',"
             << " the 'packing' setting does not point to a class that"
             << " extends the Packing class. Please check your input"
             << " or modified-data files to make sure the attributes are"
             << " correctly specified." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // If the user specified ownership handler object then make sure it extends
   // the OwnershipHandler virtual class.
   if ( ( ownership != NULL ) && ( dynamic_cast< OwnershipHandler * >( ownership ) == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " For object '" << name << "',"
             << " the 'ownership' setting does not point to a class that"
             << " extends the OwnershipHandler class. Please check"
             << " your input or modified-data files to make sure the"
             << " attributes are correctly specified." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // If the user specified a resignation identification object then make sure
   // it extends the ObjectDeleted virtual class.
   if ( ( deleted != NULL ) && ( dynamic_cast< ObjectDeleted * >( deleted ) == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " For object '" << name << "', the 'deleted' setting does not"
             << " point to a class that extends the ObjectDeleted"
             << " class. Please check your input or modified-data files to make"
             << " sure the attributes are correctly specified." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // Reset the TrickHLA Attributes count if it is negative or if there
   // are no attributes.
   if ( ( attr_count < 0 ) || ( attributes == NULL ) ) {
      attr_count = 0;
   }

   // Determine if any attribute is the FOM specified order.
   any_attribute_FOM_specified_order = false;
   for ( int i = 0; !any_attribute_FOM_specified_order && i < attr_count; i++ ) {
      if ( attributes[i].get_preferred_order() == TRANSPORT_SPECIFIED_IN_FOM ) {
         any_attribute_FOM_specified_order = true;
      }
   }

   // Determine if any attribute is Timestamp Order.
   any_attribute_timestamp_order = false;
   for ( int i = 0; !any_attribute_timestamp_order && i < attr_count; i++ ) {
      if ( attributes[i].get_preferred_order() == TRANSPORT_TIMESTAMP_ORDER ) {
         any_attribute_timestamp_order = true;
      }
   }

   // Build the string array of attributes FOM names.
   for ( int i = 0; i < attr_count; i++ ) {
      // Validate the FOM-name to make sure we don't have a  problem with the
      // list of names as well as get a difficult to debug runtime error for
      // the string constructor if we had a null FOM-name.
      if ( ( attributes[i].get_FOM_name() == NULL ) || ( *( attributes[i].get_FOM_name() ) == '\0' ) ) {
         ostringstream errmsg;
         errmsg << "Object::initialize():" << __LINE__
                << " Object with FOM Name '" << name << "' has a missing"
                << " Attribute FOM Name at array index " << i << ". Please"
                << " check your input or modified-data files to make sure the"
                << " object attribute FOM name is correctly specified."
                << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );
         exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
      }
      attribute_FOM_names.push_back( string( attributes[i].get_FOM_name() ) );
   }

   // Initialize the Packing-Handler.
   if ( packing != NULL ) {
      packing->initialize_callback( this );
   }

   // Initialize the Ownership-Handler.
   if ( ownership != NULL ) {
      ownership->initialize_callback( this );
   }

   // If the user specified a lag_comp object then make sure it extends the
   // LagCompensation virtual class.
   if ( ( lag_comp != NULL ) && ( dynamic_cast< LagCompensation * >( lag_comp ) == NULL ) ) {
      ostringstream errmsg;
      errmsg << "Object::initialize():" << __LINE__
             << " For object '" << name << "', the 'lag_comp' setting does not"
             << " point to a class that extends the LagCompensation"
             << " class. Please check your input or modified-data files to make"
             << " sure the attributes are correctly specified." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
   }

   // Initialize the Lag-Compensation.
   if ( lag_comp != NULL ) {
      lag_comp->initialize_callback( this );
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   return;
}

Federate *Object::get_federate()
{
   return ( ( this->manager != NULL ) ? this->manager->get_federate() : NULL );
}

RTI1516_NAMESPACE::RTIambassador *Object::get_RTI_ambassador()
{
   if ( rti_ambassador == NULL ) {
      // Macro to save the FPU Control Word register value.
      TRICKHLA_SAVE_FPU_CONTROL_WORD;

      // Get the Trick-Federate.
      Federate *trick_fed = get_federate();

      // Get the RTI-Ambassador.
      rti_ambassador = ( trick_fed != NULL ) ? trick_fed->get_RTI_ambassador()
                                             : static_cast< RTI1516_NAMESPACE::RTIambassador * >( NULL );

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

            Federate *trick_fed = get_federate();
            if ( trick_fed != NULL ) {
               // Only delete an object instance that has a valid instance handle.
               if ( is_instance_handle_valid() && trick_fed->is_execution_member() ) {

                  // Delete the object instance at a specific time if we are
                  // time-regulating.
                  if ( trick_fed->in_time_regulating_state() ) {
                     Int64Time new_time = get_update_time_plus_lookahead();
                     (void)rti_amb->deleteObjectInstance( instance_handle,
                                                          RTI1516_USERDATA( 0, 0 ),
                                                          new_time.get() );
                  } else {
                     (void)rti_amb->deleteObjectInstance( instance_handle,
                                                          RTI1516_USERDATA( 0, 0 ) );
                  }
               }
            }
         } catch ( DeletePrivilegeNotHeld &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            send_hs( stderr, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of DeletePrivilegeNotHeld Exception: '%s'%c",
                     __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
         } catch ( ObjectInstanceNotKnown &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            send_hs( stderr, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of ObjectInstanceNotKnown Exception: '%s'%c",
                     __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
         } catch ( FederateNotExecutionMember &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            send_hs( stderr, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of FederateNotExecutionMember Exception: '%s'%c",
                     __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
         } catch ( SaveInProgress &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            send_hs( stderr, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of SaveInProgress Exception: '%s'%c",
                     __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
         } catch ( RestoreInProgress &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            send_hs( stderr, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of RestoreInProgress Exception: '%s'%c",
                     __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
         } catch ( NotConnected &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            send_hs( stderr, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of NotConnected Exception: '%s'%c",
                     __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
         } catch ( RTIinternalError &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            send_hs( stderr, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of RTIinternalError Exception: '%s'%c",
                     __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
         } catch ( RTI1516_EXCEPTION &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            send_hs( stderr, "Object.remove():%d Could not delete object \
instance '%s' from RTI because of Exception: '%s'%c",
                     __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
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
   object_deleted_from_RTI = true;

   // Since the object has been deleted from the federation, make sure it is
   // no longer required in the federation. if a checkpoint is cut after this
   // object is removed, we need to correctly identify this object as not
   // required when restoring the saved federation...
   required = false;

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
      send_hs( stderr, "Object::remove_object_instance():%d Object '%s' Instance-ID:%s Valid-ID:%s %c",
               __LINE__, get_name(), id_str.c_str(),
               ( is_instance_handle_valid() ? "Yes" : "No" ), THLA_NEWLINE );
   }
}

void Object::process_deleted_object()
{
   if ( object_deleted_from_RTI ) {

      // Set the flag that callback may have been triggered, so that we
      // only process the deleted object once.
      object_deleted_from_RTI = false;

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         send_hs( stderr, "Object::process_deleted_object():%d Object '%s' Instance-ID:%s Valid-ID:%s %c",
                  __LINE__, get_name(), id_str.c_str(),
                  ( is_instance_handle_valid() ? "Yes" : "No" ), THLA_NEWLINE );
      }

      // If the callback class has been defined, call it...
      if ( ( deleted != NULL ) && ( dynamic_cast< ObjectDeleted * >( deleted ) != NULL ) ) {
         deleted->deleted( this );
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
          << __LINE__ << endl
          << "  Object:'" << get_name() << "'"
          << " FOM-Name:'" << get_FOM_name() << "'"
          << " Instance-ID:" << id_str;
   }
   for ( int i = 0; i < attr_count; i++ ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         msg << endl
             << "   " << ( i + 1 ) << "/" << attr_count
             << " FOM-Attribute:'" << attributes[i].get_FOM_name() << "'"
             << " Trick-Name:'" << attributes[i].get_trick_name() << "'"
             << " locally_owned: "
             << ( attributes[i].is_locally_owned() ? "Yes" : "No" );
      }

      if ( attributes[i].is_locally_owned() ) {
         attributes[i].unmark_locally_owned();

         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            msg << endl
                << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attributes[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attributes[i].get_trick_name() << "'"
                << " locally_owned: "
                << ( attributes[i].is_locally_owned() ? "Yes" : "No" );
         }
      }
   }
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      msg << THLA_ENDL;
      send_hs( stdout, (char *)msg.str().c_str() );
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
         send_hs( stdout, "Object::publish_object_attributes():%d For object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }

      try {
         AttributeHandleSet attrs;

         // Publish only the attributes we have the publish flag set for.
         for ( int i = 0; i < attr_count; i++ ) {
            if ( attributes[i].is_publish() ) {
               attrs.insert( attributes[i].get_attribute_handle() );
            }
         }

         if ( !attrs.empty() ) {
            rti_amb->publishObjectClassAttributes( this->class_handle, attrs );
         }
      } catch ( ObjectClassNotDefined &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::publish_object_attributes():%d ObjectClassNotDefined : '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( AttributeNotDefined &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::publish_object_attributes():%d ObjectClassNotDefined : '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( FederateNotExecutionMember &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::publish_object_attributes():%d FederateNotExecutionMember : '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::publish_object_attributes():%d SaveInProgress : '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::publish_object_attributes():%d RestoreInProgress : '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( NotConnected &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::publish_object_attributes():%d NotConnected : '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::publish_object_attributes():%d RTIinternalError : '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_EXCEPTION &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::publish_object_attributes():%d class attributes exception for '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      send_hs( stderr, "Object::publish_object_attributes():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
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
      send_hs( stderr, "Object::unpublish_all_object_attributes():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( any_attribute_published() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::unpublish_all_object_attributes():%d For object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }

      try {
         rti_amb->unpublishObjectClass( this->class_handle );
      } catch ( RTI1516_NAMESPACE::ObjectClassNotDefined &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unpublish_all_object_attributes():%d ObjectClassNotDefined '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::OwnershipAcquisitionPending &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unpublish_all_object_attributes():%d OwnershipAcquisitionPending '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unpublish_all_object_attributes():%d FederateNotExecutionMember '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::SaveInProgress &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unpublish_all_object_attributes():%d SaveInProgress '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unpublish_all_object_attributes():%d RestoreInProgress '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::NotConnected &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unpublish_all_object_attributes():%d NotConnected '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unpublish_all_object_attributes():%d RTIinternalError '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_EXCEPTION &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unpublish_all_object_attributes():%d exception '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
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
         send_hs( stdout, "Object::subscribe_to_object_attributes():%d For object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }

      try {
         // To actually subscribe we need to build an AttributeHandleSet
         // that contains a list of attribute type ids (AttributeHandle).

         AttributeHandleSet attrs;

         // Subscribe only to the attributes we have the subscribe flag set for.
         for ( int i = 0; i < attr_count; i++ ) {
            if ( attributes[i].is_subscribe() ) {
               attrs.insert( attributes[i].get_attribute_handle() );
            }
         }

         if ( !attrs.empty() ) {
            rti_amb->subscribeObjectClassAttributes( this->class_handle,
                                                     attrs,
                                                     true );
         }
      } catch ( ObjectClassNotDefined &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::subscribe_to_object_attributes():%d ObjectClassNotDefined : '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( AttributeNotDefined &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::subscribe_to_object_attributes():%d AttributeNotDefined : '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( FederateNotExecutionMember &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::subscribe_to_object_attributes():%d FederateNotExecutionMember : '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::subscribe_to_object_attributes():%d SaveInProgress : '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::subscribe_to_object_attributes():%d RestoreInProgress : '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( InvalidUpdateRateDesignator &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::subscribe_to_object_attributes():%d InvalidUpdateRateDesignator : '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( NotConnected &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::subscribe_to_object_attributes():%d NotConnected : '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::subscribe_to_object_attributes():%d RTIinternalError : '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_EXCEPTION &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::subscribe_to_object_attributes():%d exception '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      send_hs( stderr, "Object::subscribe_to_object_attributes():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
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
      send_hs( stderr, "Object::unsubscribe_all_object_attributes():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   if ( any_attribute_subscribed() ) {

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::unsubscribe_all_object_attributes():%d For object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }

      try {
         rti_amb->unsubscribeObjectClass( this->class_handle );
      } catch ( RTI1516_NAMESPACE::ObjectClassNotDefined &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unsubscribe_all_object_attributes():%d ObjectClassNotDefined '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::FederateNotExecutionMember &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unsubscribe_all_object_attributes():%d FederateNotExecutionMember '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::SaveInProgress &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unsubscribe_all_object_attributes():%d SaveInProgress '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::RestoreInProgress &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unsubscribe_all_object_attributes():%d RestoreInProgress '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::NotConnected &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unsubscribe_all_object_attributes():%d NotConnected '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_NAMESPACE::RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unsubscribe_all_object_attributes():%d RTIinternalError '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_EXCEPTION &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::unsubscribe_all_object_attributes():%d exception '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
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
      send_hs( stderr, "Object::reserve_object_name_with_RTI():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   // Reserving an object instance name with the RTI allows the object to be
   // recognized by name when discovered by other federates in the federation
   // execution. Note: We are notified in a callback if the name reservation
   // was successful or failed.
   if ( is_instance_handle_valid() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::reserve_object_name_with_RTI():%d \
WARNING: Object instance already exists so we will not reserve the instance name '%s' for it!%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::reserve_object_name_with_RTI():%d \
Requesting reservation of Object instance name '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }

      // Create the wide-string version of the object instance name.
      wstring ws_obj_name;
      StringUtilities::to_wstring( ws_obj_name, get_name() );

      try {
         rti_amb->reserveObjectInstanceName( ws_obj_name );
      } catch ( IllegalName &e ) {
         send_hs( stderr, "Object::reserve_object_name_with_RTI():%d IllegalName %c", __LINE__, THLA_NEWLINE );
      } catch ( FederateNotExecutionMember &e ) {
         send_hs( stderr, "Object::reserve_object_name_with_RTI():%d FederateNotExecutionMember %c", __LINE__, THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         send_hs( stderr, "Object::reserve_object_name_with_RTI():%d SaveInProgress %c", __LINE__, THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         send_hs( stderr, "Object::reserve_object_name_with_RTI():%d RestoreInProgress %c", __LINE__, THLA_NEWLINE );
      } catch ( NotConnected &e ) {
         send_hs( stderr, "Object::reserve_object_name_with_RTI():%d NotConnected %c", __LINE__, THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::reserve_object_name_with_RTI():%d RTIinternalError: '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_EXCEPTION &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         ostringstream errmsg;
         errmsg << "Object::reserve_object_name_with_RTI():" << __LINE__
                << " Exception reserving '" << get_name() << "': '"
                << rti_err_msg << "'.";
         send_hs( stderr, (char *)errmsg.str().c_str() );
         exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
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
void Object::wait_on_object_name_reservation()
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
      send_hs( stdout, "Object::wait_on_object_name_reservation():%d \
Waiting on reservation of Object Instance Name '%s'.%c",
               __LINE__, get_name(), THLA_NEWLINE );
   }

   Federate *trick_fed = get_federate();

   SleepTimeout sleep_timer( 10.0, 1000 );

   while ( !name_registered ) {
      (void)sleep_timer.sleep();

      if ( !name_registered && sleep_timer.timeout() ) {
         sleep_timer.reset();
         if ( !trick_fed->is_execution_member() ) {
            ostringstream errmsg;
            errmsg << "Object::wait_on_object_name_reservation():" << __LINE__
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

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      send_hs( stdout, "Object::wait_on_object_name_reservation():%d \
Object instance name '%s' is reserved.%c",
               __LINE__, get_name(), THLA_NEWLINE );
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
      send_hs( stderr, "Object::register_object_with_RTI():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   // Register object with the RTI. Registering an object with the RTI allows
   // the object to be discovered by other federates in the federation execution.
   if ( is_instance_handle_valid() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         send_hs( stdout, "Object::register_object_with_RTI():%d WARNING: \
Detected object already registered '%s' Instance-ID:%s%c",
                  __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
      }
   } else {
      try {
         if ( is_name_required() ) {
            wstring ws_obj_name;
            StringUtilities::to_wstring( ws_obj_name, get_name() );

            this->instance_handle = rti_amb->registerObjectInstance(
               this->class_handle,
               ws_obj_name );
         } else {
            this->instance_handle = rti_amb->registerObjectInstance(
               this->class_handle );
         }
      } catch ( ObjectInstanceNameInUse &e ) {
         send_hs( stderr, "Object::register_object_with_RTI():%d ObjectInstanceNameInUse %s %c",
                  __LINE__, get_name(), THLA_NEWLINE );
      } catch ( ObjectInstanceNameNotReserved &e ) {
         send_hs( stderr, "Object::register_object_with_RTI():%d ObjectInstanceNameNotReserved %s %c",
                  __LINE__, get_name(), THLA_NEWLINE );
      } catch ( ObjectClassNotDefined &e ) {
         send_hs( stderr, "Object::register_object_with_RTI():%d ObjectClassNotDefined %s %c",
                  __LINE__, get_name(), THLA_NEWLINE );
      } catch ( ObjectClassNotPublished &e ) {
         send_hs( stderr, "Object::register_object_with_RTI():%d ObjectClassNotPublished %s %c",
                  __LINE__, get_name(), THLA_NEWLINE );
      } catch ( FederateNotExecutionMember &e ) {
         send_hs( stderr, "Object::register_object_with_RTI():%d FederateNotExecutionMember %s %c",
                  __LINE__, get_name(), THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         send_hs( stderr, "Object::register_object_with_RTI():%d SaveInProgress %s %c",
                  __LINE__, get_name(), THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         send_hs( stderr, "Object::register_object_with_RTI():%d RestoreInProgress %s %c",
                  __LINE__, get_name(), THLA_NEWLINE );
      } catch ( NotConnected &e ) {
         send_hs( stderr, "Object::register_object_with_RTI():%d NotConnected %s %c",
                  __LINE__, get_name(), THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::register_object_with_RTI():%d % RTIinternalError: '%s'%c",
                  __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_EXCEPTION &e ) {
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         ostringstream errmsg;
         errmsg << "Object::register_object_with_RTI():" << __LINE__
                << " Exception registering '" << get_name() << "': '"
                << rti_err_msg << "'." << THLA_ENDL;
         send_hs( stderr, (char *)errmsg.str().c_str() );
         exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
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
         } catch ( ObjectInstanceNotKnown &e ) {
            send_hs( stderr, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: ObjectInstanceNotKnown %c",
                     __LINE__, THLA_NEWLINE );
         } catch ( FederateNotExecutionMember &e ) {
            send_hs( stderr, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: FederateNotExecutionMember %c",
                     __LINE__, THLA_NEWLINE );
         } catch ( NotConnected &e ) {
            send_hs( stderr, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: NotConnected %c",
                     __LINE__, THLA_NEWLINE );
         } catch ( RTIinternalError &e ) {
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            send_hs( stderr, "Object::register_object_with_RTI():%d rti_amb->getObjectInstanceName() ERROR: RTIinternalError: '%s'%c",
                     __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
         } catch ( RTI1516_EXCEPTION &e ) {
            // Macro to restore the saved FPU Control Word register value.
            TRICKHLA_RESTORE_FPU_CONTROL_WORD;
            TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
            string id_str;
            StringUtilities::to_string( id_str, instance_handle );
            string rti_err_msg;
            StringUtilities::to_string( rti_err_msg, e.what() );
            ostringstream errmsg;
            errmsg << "Object::register_object_with_RTI():" << __LINE__
                   << " Exception getting instance name for '" << get_name()
                   << "' ID:" << id_str << "  '" << rti_err_msg << "'." << THLA_ENDL;
            send_hs( stderr, (char *)errmsg.str().c_str() );
            exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
         }
         // Macro to restore the saved FPU Control Word register value.
         TRICKHLA_RESTORE_FPU_CONTROL_WORD;
         TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
      }

      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         string id_str;
         StringUtilities::to_string( id_str, instance_handle );
         send_hs( stdout, "Object::register_object_with_RTI():%d Registered '%s' Instance-ID:%s%c",
                  __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
      }
   }
}

/*!
 * @details Calling this function will block until the object instance is registered.
 * @job_class{initialization}
 */
void Object::wait_on_object_registration()
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      send_hs( stdout, "Object::wait_on_object_registration():%d Waiting on registration of '%s' for object '%s'.%c",
               __LINE__, FOM_name, get_name(), THLA_NEWLINE );
   }

   Federate *trick_fed = get_federate();

   SleepTimeout sleep_timer( 10.0, 1000 );

   while ( !is_instance_handle_valid() ) {
      (void)sleep_timer.sleep();

      if ( !is_instance_handle_valid() && sleep_timer.timeout() ) {
         sleep_timer.reset();
         if ( !trick_fed->is_execution_member() ) {
            ostringstream errmsg;
            errmsg << "Object::wait_on_object_registration():" << __LINE__
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

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      send_hs( stdout, "Object::wait_on_object_registration():%d Object \
instance '%s' for object '%s' is registered.%c",
               __LINE__, FOM_name,
               get_name(), THLA_NEWLINE );
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
      msg << "Object::setup_preferred_order_with_RTI():" << __LINE__ << endl
          << "--------- Setup Preferred-Order of Locally-Owned Attributes ---------" << endl
          << " Object:'" << get_name() << "'"
          << " FOM-Name:'" << get_FOM_name() << "'"
          << " Create HLA Instance:" << ( is_create_HLA_instance() ? "Yes" : "No" )
          << endl;
   }

   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();

   AttributeHandleSet TSO_attr_handle_set = AttributeHandleSet();
   AttributeHandleSet RO_attr_handle_set  = AttributeHandleSet();

   // Create the sets of Attribute handles for the Timestamp preferred order.
   for ( int i = 0; i < attr_count; i++ ) {
      if ( attributes[i].is_locally_owned() && ( attributes[i].get_preferred_order() == TRANSPORT_TIMESTAMP_ORDER ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            msg << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attributes[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attributes[i].get_trick_name() << "'"
                << " Preferred-Order:TIMESTAMP"
                << endl;
         }
         TSO_attr_handle_set.insert( attributes[i].get_attribute_handle() );
      }
   }

   // Create the sets of Attribute handles for the Receive preferred order.
   for ( int i = 0; i < attr_count; i++ ) {
      if ( attributes[i].is_locally_owned() && ( attributes[i].get_preferred_order() == TRANSPORT_RECEIVE_ORDER ) ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            msg << "   " << ( i + 1 ) << "/" << attr_count
                << " FOM-Attribute:'" << attributes[i].get_FOM_name() << "'"
                << " Trick-Name:'" << attributes[i].get_trick_name() << "'"
                << " Preferred-Order:RECEIVE"
                << endl;
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
      send_hs( stdout, (char *)msg.str().c_str() );
   }

   try {
      if ( !TSO_attr_handle_set.empty() ) {
         (void)rti_amb->changeAttributeOrderType( this->instance_handle,
                                                  TSO_attr_handle_set,
                                                  RTI1516_NAMESPACE::TIMESTAMP );
      }
      // Must free the memory
      TSO_attr_handle_set.clear();

      if ( !RO_attr_handle_set.empty() ) {
         (void)rti_amb->changeAttributeOrderType( this->instance_handle,
                                                  RO_attr_handle_set,
                                                  RTI1516_NAMESPACE::RECEIVE );
      }
      // Must free the memory
      RO_attr_handle_set.clear();

   } catch ( ObjectInstanceNotKnown &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::setup_preferred_order_with_RTI():%d Object instance not known for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( AttributeNotOwned &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::setup_preferred_order_with_RTI():%d attribute not owned for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( AttributeNotDefined &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::setup_preferred_order_with_RTI():%d attribute not defined for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( FederateNotExecutionMember &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::setup_preferred_order_with_RTI():%d federation not execution member for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( SaveInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::setup_preferred_order_with_RTI():%d save in progress for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( RestoreInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::setup_preferred_order_with_RTI():%d restore in progress for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( NotConnected &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::setup_preferred_order_with_RTI():%d not connected error for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( RTIinternalError &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::setup_preferred_order_with_RTI():%d RTI internal error for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( RTI1516_EXCEPTION &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object.setup_preferred_order_with_RTI():%d Exception: '%s' and instance_id=%s%c",
               __LINE__, rti_err_msg.c_str(), id_str.c_str(), THLA_NEWLINE );
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
   for ( int i = 0; i < attr_count; i++ ) {
      // Only include attributes that are remotely owned that are subscribed to.
      if ( attributes[i].is_remotely_owned() && attributes[i].is_subscribe() ) {
         attr_handle_set.insert( attributes[i].get_attribute_handle() );
      }
   }

   try {
      (void)rti_amb->requestAttributeValueUpdate( this->instance_handle,
                                                  attr_handle_set,
                                                  RTI1516_USERDATA( 0, 0 ) );
      // Must free the memory
      attr_handle_set.clear();

   } catch ( AttributeNotDefined &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::request_attribute_value_update():%d attribute not defined for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( ObjectInstanceNotKnown &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::request_attribute_value_update():%d object instance not known for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( FederateNotExecutionMember &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::request_attribute_value_update():%d federation not execution member for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( SaveInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::request_attribute_value_update():%d save in progress for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( RestoreInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::request_attribute_value_update():%d restore in progress for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( NotConnected &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::request_attribute_value_update():%d not connected error for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( RTIinternalError &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::request_attribute_value_update():%d RTI internal error for '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
   } catch ( RTI1516_EXCEPTION &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object.request_attribute_value_update():%d Exception: '%s' and instance_id=%s%c",
               __LINE__, get_name(), id_str.c_str(), THLA_NEWLINE );
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
   for ( int i = 0; i < attr_count; i++ ) {

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
         send_hs( stdout, "Object::provide_attribute_update():%d Object '%s'%c",
                  __LINE__, get_name(), THLA_NEWLINE );
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
      send_requested_data( exec_get_sim_time(), get_federate()->get_lookahead_time() );
   }
}

/*!
 * @job_class{scheduled}
 */
void Object::send_requested_data(
   double current_time,
   double cycle_time )
{
   // If no attribute update has been requested then just return.
   if ( !attr_update_requested ) {
      return;
   }

   // Make sure we clear the attribute update request flag because we only
   // want to send data once per request.
   attr_update_requested = false;

   // We can only send attribute updates for the attributes we own and are
   // configured to publish.
   if ( !any_locally_owned_published_requested_attribute() ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      send_hs( stdout, "Object::send_requested_data():%d Object '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   Federate *trick_fed = get_federate();

   RTIambassador *rti_amb = get_RTI_ambassador();

   // Time tag the data we're about to send out
   Int64Time t = trick_fed->get_granted_fed_time();
   set_last_update_time( t.get() );

   // Check for a zero lookahead time, which means the cycle_time (i.e. dt)
   // should be zero as well.
   if ( trick_fed->is_zero_lookahead_time() ) {
      cycle_time = 0.0;
   }

   // The update_time should be the current simulation time plus the cycle
   // time for this job.  Also, the dt value would then be the job cycle time
   // for this job for this function. 11/28/2006 DDexter
   Int64Time update_time( current_time + cycle_time );

   // Update the time_plus_lookahead value.
   (void)get_update_time_plus_lookahead();

   // Make sure the current_time + cycle_time is not less than the current
   // HLA time + lookahead time, which could happen due to floating-point
   // round-off.
   if ( update_time < time_plus_lookahead ) {
      update_time.set( time_plus_lookahead );
      cycle_time = time_plus_lookahead.get_time_in_seconds() - current_time;
   }

   // Do send side lag compensation.
   if ( ( lag_comp_type == LAG_COMPENSATION_SEND_SIDE ) && ( lag_comp != NULL ) ) {
      lag_comp->send_lag_compensation();
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
   } catch ( RTI1516_EXCEPTION &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::send_requested_data():%d Can not create attribute value/pair set: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   }

   // Determine if we need to send with a timestamp.
   bool send_with_timestamp = trick_fed->in_time_regulating_state() && ( any_attribute_FOM_specified_order || any_attribute_timestamp_order );

   try {
      // Send the Attributes to the federation.
      //
      // This call returns an event retraction handle but we don't support event
      // retraction so no need to store it.

      // TODO: Get the preferred order by parsing the FOM.
      //
      // Do not send any data if federate save / restore has begun (see
      // IEEE-1516.1-2000 sections 4.12, 4.20)
      if ( trick_fed->should_publish_data() ) {

         // The message will only be sent as TSO if our Federate is in
         // the HLA Time Regulating state. See IEEE-1516.1-2000,
         // Sections 6.6 and 8.1.1.
         if ( send_with_timestamp ) {
            if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               send_hs( stdout, "Object::send_requested_data():%d For object '%s', calling rti_amb->updateAttributeValues(TimestampOrder).%c",
                        __LINE__, get_name(), THLA_NEWLINE );
            }

            (void)rti_amb->updateAttributeValues( this->instance_handle,
                                                  *attribute_values_map,
                                                  RTI1516_USERDATA( 0, 0 ),
                                                  update_time.get() );
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               send_hs( stdout, "Object::send_requested_data():%d For object '%s', calling rti_amb->updateAttributeValues(Receive Order).%c",
                        __LINE__, get_name(), THLA_NEWLINE );
            }
            (void)rti_amb->updateAttributeValues( this->instance_handle,
                                                  *attribute_values_map,
                                                  RTI1516_USERDATA( 0, 0 ) );
         }
      }
   } catch ( InvalidLogicalTime &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::send_requested_data():%d invalid logical time \
exception for '%s' with error message '%s'.%c",
               __LINE__, get_name(),
               rti_err_msg.c_str(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: InvalidLogicalTime" << endl
             << "  instance_id=" << id_str << endl
             << "  sim-time=" << current_time << " ("
             << Int64Interval::to_microseconds( current_time ) << " microseconds)" << endl
             << "  cycle_time=" << cycle_time << " ("
             << Int64Interval::to_microseconds( cycle_time ) << " microseconds)" << endl
             << "  sim-time + cycle_time=" << ( current_time + cycle_time ) << " ("
             << Int64Interval::to_microseconds( current_time + cycle_time ) << " microseconds)" << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << " ("
             << trick_fed->get_granted_fed_time().get_time_in_micros() << " microseconds)" << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << " ("
             << trick_fed->get_lookahead().get_time_in_micros() << " microseconds)" << endl
             << "  update_time=" << update_time.get_time_in_seconds() << " ("
             << update_time.get_time_in_micros() << " microseconds)" << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << " ("
             << last_update_time.get_time_in_micros() << " microseconds)" << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << " ("
             << time_plus_lookahead.get_time_in_micros() << " microseconds)" << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( AttributeNotOwned &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr,
               "Object::send_requested_data():%d detected remote ownership for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: AttributeNotOwned" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( ObjectInstanceNotKnown &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr,
               "Object::send_requested_data():%d object instance not known for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: ObjectInstanceNotKnown" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( AttributeNotDefined &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_requested_data():%d attribute not defined for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: AttributeNotDefined" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( FederateNotExecutionMember &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_requested_data():%d federation not execution member for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception:FederateNotExecutionMember" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( SaveInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_requested_data():%d save in progress for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: SaveInProgress" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RestoreInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_requested_data():%d restore in progress for '%s'",
               __LINE__, get_name() );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: RestoreInProgress" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( NotConnected &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_requested_data():%d not connected error for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: NotConnected" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RTIinternalError &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_requested_data():%d RTI internal error for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " Exception: RTIinternalError" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object.send_requested_data():%d Exception: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_requested_data():" << __LINE__
             << " RTI1516_EXCEPTION" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @job_class{scheduled}
 */
void Object::send_cyclic_and_requested_data(
   double current_time,
   double cycle_time )
{
   // Make sure we clear the attribute update request flag because we only
   // want to send data once per request.
   attr_update_requested = false;

   // We can only send cyclic attribute updates for the attributes we own, are
   // configured to publish and the cycle-time is ready for a send or was requested.
   if ( !any_locally_owned_published_cyclic_data_ready_or_requested_attribute() ) {
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   Federate *trick_fed = get_federate();

   RTIambassador *rti_amb = get_RTI_ambassador();

   // Time tag the data we're about to send out
   Int64Time t = trick_fed->get_granted_fed_time();
   set_last_update_time( t.get() );

   // Check for a zero lookahead time, which means the cycle_time (i.e. dt)
   // should be zero as well.
   if ( trick_fed->is_zero_lookahead_time() ) {
      cycle_time = 0.0;
   }

   // The update_time should be the current simulation time plus the cycle
   // time for this job. Also, the dt value would then be the job cycle time
   // for this job for this function. 11/28/2006 DDexter
   Int64Time update_time( current_time + cycle_time );

   // Update the time_plus_lookahead value.
   (void)get_update_time_plus_lookahead();

   // Make sure the current_time + cycle_time is not less than the current
   // HLA time + lookahead time, which could happen due to floating-point
   // round-off.
   if ( update_time < time_plus_lookahead ) {
      update_time.set( time_plus_lookahead );
      cycle_time = time_plus_lookahead.get_time_in_seconds() - current_time;
   }

   // Do send side lag compensation.
   if ( ( lag_comp_type == LAG_COMPENSATION_SEND_SIDE ) && ( lag_comp != NULL ) ) {
      lag_comp->send_lag_compensation();
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
   } catch ( RTI1516_EXCEPTION &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d For object '%s', cannot create attribute value/pair set: '%s'%c",
               __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
   }

   // Make sure we don't send an empty attribute map to the other federates.
   if ( attribute_values_map->empty() ) {
      // Just return because we have no data to send.
      return;
   }

   // Determine if we need to send with a timestamp.
   bool send_with_timestamp = trick_fed->in_time_regulating_state() && ( any_attribute_FOM_specified_order || any_attribute_timestamp_order );

   try {
      /*
      ostringstream msg;
      msg << "-------------------------------------" << endl
          << "Object::send_cyclic_and_requested_data():" << __LINE__ << endl
          << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << " micros=" << trick_fed->get_granted_fed_time().get_time_in_micros() << endl
          << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << " micros=" << this->last_update_time.get_time_in_micros() << endl
          << "  update_time=" << update_time.get_time_in_seconds() << " micros=" << update_time.get_time_in_micros() << endl
          << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << " micros=" << trick_fed->get_lookahead().get_time_in_micros() << endl
          << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << " micros=" << this->time_plus_lookahead.get_time_in_micros() << endl
          << "-------------------------------------" << endl;
      send_hs( stdout, (char *)msg.str().c_str() );
*/

      // TODO: Get the preferred order by parsing the FOM.
      //
      if ( trick_fed->should_publish_data() ) {

         // The message will only be sent as TSO if our Federate is in the
         // HLA Time Regulating state and we have at least one attribute
         // with a preferred timestamp order. See IEEE-1516.1-2000,
         // Sections 6.6 and 8.1.1.
         if ( send_with_timestamp ) {

            if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               send_hs( stdout, "Object::send_cyclic_and_requested_data():%d For object '%s', calling rti_amb->updateAttributeValues(TimestampOrder).%c",
                        __LINE__, get_name(), THLA_NEWLINE );
            }

            // Send in Timestamp Order
            HLAinteger64Time timestamp = update_time.get();

            (void)rti_amb->updateAttributeValues( this->instance_handle,
                                                  *attribute_values_map,
                                                  RTI1516_USERDATA( 0, 0 ),
                                                  timestamp );
         } else {
            if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               send_hs( stdout, "Object::send_cyclic_and_requested_data():%d For object '%s', calling rti_amb->updateAttributeValues(ReceiveOrder).%c",
                        __LINE__, get_name(), THLA_NEWLINE );
            }

            // Send in Receive Order (i.e. with no timestamp).
            (void)rti_amb->updateAttributeValues( this->instance_handle,
                                                  *attribute_values_map,
                                                  RTI1516_USERDATA( 0, 0 ) );
         }
      }
   } catch ( InvalidLogicalTime &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d invalid logical time \
exception for '%s' with error message '%s'.%c",
               __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );

      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " Exception: InvalidLogicalTime" << endl
             << "  instance_id=" << id_str << endl
             << "  sim-time=" << current_time << " ("
             << Int64Interval::to_microseconds( current_time ) << " microseconds)" << endl
             << "  cycle_time=" << cycle_time << " ("
             << Int64Interval::to_microseconds( cycle_time ) << " microseconds)" << endl
             << "  sim-time + cycle_time=" << ( current_time + cycle_time ) << " ("
             << Int64Interval::to_microseconds( current_time + cycle_time ) << " microseconds)" << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << " ("
             << trick_fed->get_granted_fed_time().get_time_in_micros() << " microseconds)" << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << " ("
             << trick_fed->get_lookahead().get_time_in_micros() << " microseconds)" << endl
             << "  update_time=" << update_time.get_time_in_seconds() << " ("
             << update_time.get_time_in_micros() << " microseconds)" << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << " ("
             << this->last_update_time.get_time_in_micros() << " microseconds)" << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << " ("
             << this->time_plus_lookahead.get_time_in_micros() << " microseconds)" << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( AttributeNotOwned &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d detected remote ownership for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " Exception: AttributeNotOwned" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( ObjectInstanceNotKnown &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d object instance not known for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " Exception: ObjectInstanceNotKnown" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( AttributeNotDefined &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d attribute not defined for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " Exception: AttributeNotDefined" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( FederateNotExecutionMember &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d federation not execution member for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " Exception: FederateNotExecutionMember" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( SaveInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d save in progress for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " Exception: SaveInProgress" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RestoreInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d restore in progress for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " Exception: RestoreInProgress" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( NotConnected &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d not connected for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " Exception: NotConnected" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RTIinternalError &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_cyclic_and_requested_data():%d RTI internal error for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " Exception: RTIinternalError" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object.send_cyclic_and_requested_data():%d Exception: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_cyclic_and_requested_data():" << __LINE__
             << " RTI1516_EXCEPTION" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  update_time=" << update_time.get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

/*!
 * @details If the object is owned remotely, this function copies its internal
 * data into simulation object and marks the object as "unchanged". This data
 * was deposited by the reflect callback and marked as "changed". By marking it
 *  as unchanged, we avoid copying the same data over and over. If the object
 *  is locally owned, we shouldn't be receiving any remote data anyway and if
 *  we were to -- bogusly -- copy it to the internal byte buffer, we'd
 *  continually reset our local simulation.
 * @job_class{scheduled}
 */
void Object::receive_cyclic_data(
   double current_time,
   double cycle_time )
{
   // There must be some remotely owned attribute that we subscribe to in
   // order for us to receive it.
   if ( !any_remotely_owned_subscribed_cyclic_attribute() ) {
      return;
   }

   // If we are doing blocking reads, we need to skip the first one since it
   // occurs at the top of the frame before any data has been sent resulting
   // in deadlock until the read 10 second timer expires.
   if ( blocking_cyclic_read && first_blocking_cyclic_read ) {
      first_blocking_cyclic_read = false;

      // For the first read attempt, just return if no data has been received.
      if ( !is_changed() ) {
         return;
      }
   }

   // Block waiting for received data if the user has specified we must do so.
   if ( blocking_cyclic_read && !is_changed() ) {
      // NOTE: usleep() is causing a 1 millisecond saw-tooth pattern in the
      // latency between when we received the data in the FedAmb callback and
      // when we process that data here. Dan Dexter 2/13/2008
      //
      // We support 3 different techniques to block waiting for data.
      // 1) Thread wait on a conditional variable with a timeout.
      // 2) Thread wait on a conditional variable with no timeout.
      // 3) Spin-lock with timeout.

#ifdef THLA_THREAD_WAIT_FOR_DATA
#   ifdef THLA_THREAD_TIMED_WAIT_FOR_DATA
      //------------------------------------------------------------------------
      // Block waiting for data using a thread wait on a conditional variable
      // with a timeout.

      // Break the cycle time up into the whole and fractional parts.
      double           whole_secs;
      double           frac_secs;
      static const int NANOSEC_PER_SECOND = 1000000000;

#      ifdef THLA_10SEC_TIMEOUT_WHILE_WAITING_FOR_DATA
      // Use a 10 second timeout.
      frac_secs = modf( 10.0, &whole_secs );
#      else
      frac_secs = modf( cycle_time, &whole_secs );
#      endif // THLA_10SEC_TIMEOUT_WHILE_WAITING_FOR_DATA

      // Get the current time of day.
      gettimeofday( &timeofday, NULL );

      // Calculate the timeout from the cycle_time and time of day.
      cyclic_read_timeout.tv_sec  = timeofday.tv_sec + (long)whole_secs;
      cyclic_read_timeout.tv_nsec = ( timeofday.tv_usec * 1000 ) + (long)( frac_secs * NANOSEC_PER_SECOND );

      // Do a range check on the nanoseconds field.
      if ( cyclic_read_timeout.tv_nsec >= NANOSEC_PER_SECOND ) {
         cyclic_read_timeout.tv_sec += (int)( cyclic_read_timeout.tv_nsec / NANOSEC_PER_SECOND );
         cyclic_read_timeout.tv_nsec %= NANOSEC_PER_SECOND;
      }

      // Block waiting to receive the data or until we timeout.
      int retval = 0;
      pthread_mutex_lock( &data_change_mutex );
      while ( !is_changed()
              && ( retval == 0 )
              && blocking_cyclic_read
              && any_remotely_owned_subscribed_cyclic_attribute() ) {
         retval = pthread_cond_timedwait( &data_change_cv,
                                          &data_change_mutex,
                                          &cyclic_read_timeout );
      }
      pthread_mutex_unlock( &data_change_mutex );

#      ifdef HLA_PERFORMANCE_STUDY
      // Print an error message if we timed out.
      if ( retval == ETIMEDOUT ) {
         // FOR THE HLA PERFORMANCE STUDY ONLY: Mark changed so that we can
         // collect latency numbers taking the timeout into effect.
         mark_changed();
         send_hs( stderr, "Object::receive_cyclic_data():%d Timed out waiting for data.%c",
                  __LINE__, THLA_NEWLINE );
      }
#      else  // !HLA_PERFORMANCE_STUDY

      // Print an error message if we timed out.
      if ( retval == ETIMEDOUT ) {
         if ( is_changed() ) {
            send_hs( stderr, "Object::receive_cyclic_data():%d Received data at a timeout boundary.%c",
                     __LINE__, THLA_NEWLINE );
         } else {
            send_hs( stderr, "Object::receive_cyclic_data():%d Timed out waiting for data.%c",
                     __LINE__, THLA_NEWLINE );
         }
      }
#      endif // HLA_PERFORMANCE_STUDY

#   else  // !THLA_THREAD_TIMED_WAIT_FOR_DATA

      //------------------------------------------------------------------------
      // Block waiting for data using a thread wait on a conditional variable
      // with no timeout.
      // NOTE: We could be stuck in an infinite loop if no data ever comes in!

      // Thread wait on conditional variable with no timeout.
      pthread_mutex_lock( &data_change_mutex );
      while ( !is_changed()
              && blocking_cyclic_read
              && any_remotely_owned_subscribed_cyclic_attribute() ) {
         // NOTE: Need to timeout the pthread condition variable.
         pthread_cond_wait( &data_change_cv, &data_change_mutex );
      }
      pthread_mutex_unlock( &data_change_mutex );
#   endif // THLA_THREAD_TIMED_WAIT_FOR_DATA
#else     // !THLA_THREAD_WAIT_FOR_DATA
      //------------------------------------------------------------------------
      // Block waiting for data using a spin-lock that supports a timeout.

      double time = clock.get_time();

#   ifdef THLA_10SEC_TIMEOUT_WHILE_WAITING_FOR_DATA
      // Use a 10.0 second timeout.
      double timeout = time + 10.0;
#   else
      double timeout = time + cycle_time;
#   endif // THLA_10SEC_TIMEOUT_WHILE_WAITING_FOR_DATA

#   ifdef HLA_PERFORMANCE_STUDY
      // We use a different way to wait for the data for the HLA-performance
      // study because the assembly "nop" instruction causes less spikes in
      // the latency but at the cost of keeping the CPU at 100%.

      // Wait for the data to change by using a spin lock that can timeout.
      unsiged int i;
      while ( !is_changed()
              && ( time < timeout )
              && blocking_cyclic_read
              && any_remotely_owned_subscribed_cyclic_attribute() ) {

         // Wait for the data to change by executing NOP instructions.
         for ( i = 0; i < 100; ++i ) {
            // Execute "NOP" no-operation instruction (one-clock-cycle).
            // NOTE: Volatile keyword is used to keep gcc from optimizing the
            // line of assembly code and reducing the number of nop's.
            // The :: indicates we won't use any input or output operand.
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
         }
         if ( !is_changed() ) {
            // Get the current clock time.
            time = clock.get_time();
         }
      }
      if ( time >= timeout ) {
         // FOR THE HLA PERFORMANCE STUDY ONLY: Mark changed so that we can
         // collect latency numbers taking the timeout into effect.
         mark_changed();

         send_hs( stderr, "Object::receive_cyclic_data():%d Timed out \
waiting for data at time %f seconds with timeout time %f and simulation time %f.%c",
                  __LINE__, time, timeout, current_time, THLA_NEWLINE );
      }
#   else // !HLA_PERFORMANCE_STUDY

#      ifdef THLA_USLEEP_DELAY_FOR_SPIN_LOCK
      // On average using "usleep()" to wait for data is faster but at the
      // cost of latency spikes every once in a while. As a bonus the CPU
      // utilization will be much less using usleep() as compared to the
      // assembly "nop" instruction. If you care about hard realtime performance
      // (i.e. maximum latency) then use the assembly "nop" method, otherwise
      // usleep() will give better average performance mainly because it frees
      // up the CPU allowing the HLA callback thread a better chance to run.
      //
      // Wait for the data to change by using a spin lock that can timeout.
      SleepTimeout sleep_timer( 10.0, 1 );
      while ( !is_changed()
              && ( time < timeout )
              && blocking_cyclic_read
              && any_remotely_owned_subscribed_cyclic_attribute() ) {
         (void)sleep_timer.sleep();
         if ( !is_changed() ) {
            time = clock.get_time();
         }
      }
#      else  //!THLA_USLEEP_DELAY_FOR_SPIN_LOCK

      // Wait for the data to change by using a spin lock that can timeout.
      unsigned int i;
      while ( !is_changed()
              && ( time < timeout )
              && blocking_cyclic_read
              && any_remotely_owned_subscribed_cyclic_attribute() ) {
         for ( i = 0; i < 100; ++i ) {
            // Execute "NOP" no-operation instruction (one-clock-cycle).
            // NOTE: Volatile keyword is used to keep gcc from optimizing the
            // line of assembly code and reducing the number of nop's.
            // The :: indicates we won't use any input or output operand.
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
            asm volatile( "nop" :: );
         }
         if ( !is_changed() ) {
            time = clock.get_time();
         }
      }
#      endif //THLA_USLEEP_DELAY_FOR_SPIN_LOCK

      // Display a warning message if we timed out.
      if ( time >= timeout ) {
         if ( is_changed() ) {
            send_hs( stderr, "Object::receive_cyclic_data():%d Received \
data at a timeout boundary at %f seconds (time-of-day) with a timeout of %f \
seconds (time-of-day) and simulation time %f.%c",
                     __LINE__, time, timeout, current_time, THLA_NEWLINE );
         } else {
            send_hs( stderr, "Object::receive_cyclic_data():%d Timed out \
waiting for data at %f seconds (time-of-day) with a timeout of %f seconds \
(time-of-day) and simulation time %f.%c",
                     __LINE__, time, timeout, current_time, THLA_NEWLINE );
         }
      }
#   endif    // HLA_PERFORMANCE_STUDY
#endif       // THLA_THREAD_WAIT_FOR_DATA
   }

   // Process the data now that it has been received (i.e. changed).
   if ( is_changed() ) {

#if THLA_OBJ_DEBUG_RECEIVE
      Federate *trick_fed = get_federate();
      send_hs( stdout, "Object::receive_cyclic_data():%d for '%s' at t=%G%c",
               __LINE__, get_name(),
               trick_fed->get_granted_fed_time().get_time_in_seconds(), THLA_NEWLINE );
#endif

      // Unpack the buffer and copy the values to the object attributes.
      unpack_cyclic_attribute_buffers();

      // Unpack the data for the object if we have a packing object.
      if ( packing != NULL ) {
         packing->unpack();
      }

      // Do receive side lag compensation.
      if ( ( lag_comp_type == LAG_COMPENSATION_RECEIVE_SIDE ) && ( lag_comp != NULL ) ) {
         lag_comp->receive_lag_compensation();
      }

      // Mark the data as unchanged now that we have unpacked the buffer.
      mark_unchanged();
   }
#if THLA_OBJ_DEBUG_VALID_OBJECT_RECEIVE
   else if ( is_instance_handle_valid() && current_time > 0.0L ) {
      send_hs( stdout, "Object::receive_cyclic_data():%d NO new data for valid object '%s' at t=%G%c",
               __LINE__, get_name(),
               get_granted_fed_time().get_time_in_seconds(), THLA_NEWLINE );
   }
#endif
#if THLA_OBJ_DEBUG_RECEIVE
   else {
      send_hs( stdout, "Object::receive_cyclic_data():%d NO new data for '%s' at t=%G%c",
               __LINE__, get_name(), get_granted_fed_time().get_time_in_seconds(),
               THLA_NEWLINE );
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

   Federate *trick_fed = get_federate();

   RTIambassador *rti_amb = get_RTI_ambassador();

   // Update the time for the data we're about to send out
   Int64Time t = trick_fed->get_granted_fed_time();
   set_last_update_time( t.get() );

   // If we have a data packing object then pack the data now.
   if ( packing != NULL ) {
      packing->pack();
   }

   // Buffer the attribute values for the object.
   pack_init_attribute_buffers();

   try {
      // Create the map of "initialize" attribute values we will be updating.
      create_attribute_set( CONFIG_INITIALIZE );
   } catch ( RTI1516_EXCEPTION &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::send_init_data():%d For object '%s', can not create attribute value/pair set: '%s'%c",
               __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
   }

   try {
      // Do not send any data if federate save / restore has begun (see
      // IEEE-1516.1-2000 sections 4.12, 4.20)
      if ( trick_fed->should_publish_data() ) {

         if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            send_hs( stdout, "Object::send_init_data():%d For object '%s', calling rti_amb->updateAttributeValues(ReceiveOrder).%c",
                     __LINE__, get_name(), THLA_NEWLINE );
         }

         // Send the Attributes to the federation. This call returns an
         // event retraction handle but we don't support event retraction
         // so no need to store it.
         (void)rti_amb->updateAttributeValues( this->instance_handle,
                                               *attribute_values_map,
                                               RTI1516_USERDATA( 0, 0 ) );
      }
   } catch ( InvalidLogicalTime &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::send_init_data():%d invalid logical time exception for '%s' with error message '%s'.%c",
               __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );

      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: InvalidLogicalTime" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << " ("
             << trick_fed->get_granted_fed_time().get_time_in_micros() << " microseconds)" << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << " ("
             << trick_fed->get_lookahead().get_time_in_micros() << " microseconds)" << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << " ("
             << this->last_update_time.get_time_in_micros() << " microseconds)" << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << " ("
             << this->time_plus_lookahead.get_time_in_micros() << " microseconds)" << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( AttributeNotOwned &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_init_data():%d detected remote ownership for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: AttributeNotOwned" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( ObjectInstanceNotKnown &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_init_data():%d object instance not known for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: ObjectInstanceNotKnown" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( AttributeNotDefined &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_init_data():%d attribute not defined for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: AttributeNotDefined" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( FederateNotExecutionMember &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_init_data():%d federation not execution member for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: FederateNotExecutionMember" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( SaveInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_init_data():%d save in progress for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: SaveInProgress" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RestoreInProgress &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_init_data():%d restore in progress for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: RestoreInProgress" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( NotConnected &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_init_data():%d not connected error for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: NotConnected" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RTIinternalError &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      send_hs( stderr, "Object::send_init_data():%d RTI internal error for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: RTIinternalError" << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
   } catch ( RTI1516_EXCEPTION &e ) {
      string id_str;
      StringUtilities::to_string( id_str, instance_handle );
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object.send_init_data():%d Exception: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      ostringstream errmsg;
      errmsg << "Object::send_init_data():" << __LINE__
             << " Exception: " << endl
             << "  instance_id=" << id_str << endl
             << "  fed_time=" << trick_fed->get_granted_fed_time().get_time_in_seconds() << endl
             << "  lookahead=" << trick_fed->get_lookahead().get_time_in_seconds() << endl
             << "  last_update_time=" << this->last_update_time.get_time_in_seconds() << endl
             << "  fed+lookahead=" << this->time_plus_lookahead.get_time_in_seconds() << endl;
      send_hs( stderr, (char *)errmsg.str().c_str() );
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

      // Unpack the buffer and copy the values to the object attributes.
      unpack_init_attribute_buffers();

      // Unpack the data for the object if we have a packing object.
      if ( packing != NULL ) {
         packing->unpack();
      }

      // Mark the data as unchanged now that we have unpacked the buffer.
      mark_unchanged();
   }
#if THLA_OBJ_DEBUG_RECEIVE
   else {
      send_hs( stdout, "****** Object::receive_init_data():%d No initialize data for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
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

      // TODO: In a future version, pre-sort all the items we publish so
      // that we will not need to check the publish flag, for efficiency.
      //
      // Only include attributes that have been requested, we own, and we publish.
      if ( attributes[i].is_update_requested()
           && attributes[i].is_locally_owned()
           && attributes[i].is_publish() ) {

         // If there was a requested update for this attribute make sure
         // we clear the request flag.
         attributes[i].set_update_requested( false );

         if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            send_hs( stdout, "Object::create_requested_attribute_set():%d Adding '%s' to attribute map.%c",
                     __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
         }
         // Create the Attribute-Value from the buffered data.
         ( *attribute_values_map )[attributes[i].get_attribute_handle()] =
            attributes[i].get_attribute_value();
      }
   }
}

/*!
 * @job_class{scheduled}
 */
void Object::create_attribute_set(
   const DataUpdateEnum required_config,
   const bool           include_requested )
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

         // TODO: In a future version, pre-sort all the items we publish
         // so that we will not need to check the publish flag, for
         // efficiency.
         //
         // Only include attributes that have the required configuration,
         // we own, we publish, and the sub-rate says we are ready to
         // send or the attribute has been requested.
         if ( attributes[i].is_locally_owned()
              && attributes[i].is_publish()
              && ( ( include_requested && attributes[i].is_update_requested() )
                   || ( attributes[i].is_data_cycle_ready() && ( ( attributes[i].get_configuration() & required_config ) == required_config ) ) ) ) {

            // If there is no sub-classed TrickHLAConditional object for this
            // attribute or if the sub-classed TrickHLAConditional object
            // indicates that it should be sent, then add this attribute into\
            // the attribute map.
            if ( !attributes[i].has_conditional()
                 || attributes[i].get_conditional()->should_send( &attributes[i] ) ) {

               // If there was a requested update for this attribute make sure
               // we clear the request flag now since we are handling it here.
               attributes[i].set_update_requested( false );

               if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                  send_hs( stdout, "Object::create_attribute_set():%d For object '%s', adding '%s' to attribute map.%c",
                           __LINE__, get_name(), attributes[i].get_FOM_name(), THLA_NEWLINE );
               }
               // Create the Attribute-Value from the buffered data.
               ( *attribute_values_map )[attributes[i].get_attribute_handle()] =
                  attributes[i].get_attribute_value();
            }
         }
      }
   } else {
      for ( int i = 0; i < attr_count; ++i ) {

         // TODO: In a future version, pre-sort all the items we publish
         // so that we will not need to check the publish flag, for
         // efficiency.
         //
         // Only include attributes that have the required configuration,
         // we own, and we publish.
         if ( ( ( attributes[i].get_configuration() & required_config ) == required_config )
              && attributes[i].is_locally_owned()
              && attributes[i].is_publish() ) {

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

#if defined( THLA_QUEUE_REFLECTED_ATTRIBUTES )
/*!
 * @job_class{scheduled}
 */
void Object::enqueue_data(
   const AttributeHandleValueMap &theAttributes )
{
   thla_reflected_attributes_queue.push( theAttributes );
}
#endif // THLA_QUEUE_REFLECTED_ATTRIBUTES

/*!
 * @details This routine is called by the federate ambassador when new
 * attribute values come in for this object.
 * @job_class{scheduled}
 */
void Object::extract_data(
   AttributeHandleValueMap &theAttributes )
{
   // We need to iterate through the AttributeHandleValuePairSet
   // to extract each AttributeHandleValuePair. Based on the type
   // specified ( the value returned by getHandle() ) we need to
   // extract the data from the buffer that is returned by
   // getValue().

   AttributeHandleValueMap::iterator iter;
   bool                              attr_changed = false;

   for ( iter = theAttributes.begin(); iter != theAttributes.end(); ++iter ) {

      // Get the TrickHLA Attribute for the given AttributeHandle from the
      // attribute-handle-value map.
      // NOTE: The value of iter->first is of type AttributeHandle.
      Attribute *attr = get_attribute( iter->first );

      // Determine if this object has this attribute.
      if ( attr != NULL ) {

         // Place the RTI AttributeValue into the TrickHLA Attribute.
         attr->extract_data( &( iter->second ) );

         attr_changed = true;

      } else if ( DebugHandler::show( DEBUG_LEVEL_7_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         string id_str;
         StringUtilities::to_string( id_str, iter->first );
         send_hs( stderr, "Object::extract_data():%d WARNING: For \
Object '%s' with FOM name '%s', data was received for Attribute-ID:%s, which \
has not been configured for this object instance in the input file. Ignoring \
this attribute.%c",
                  __LINE__, name, FOM_name, id_str.c_str(), THLA_NEWLINE );
      }
   }

   // Set the change flag once all the attributes have been processed.
   if ( attr_changed ) {
      // Mark the data as being changed since the attribute changed.
      mark_changed();

      // Flag for user use to indicate the data changed.
      this->data_changed = true;
   }
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
      send_hs( stderr, "Object::release_ownership():%d Object-Instance-Handle not set for '%s'.%c",
               __LINE__, get_name(), THLA_NEWLINE );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   if ( rti_amb == NULL ) {
      send_hs( stderr, "Object::release_ownership():%d Unexpected NULL RTIambassador.%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

#if THLA_OBJ_OWNERSHIP_DEBUG
   send_hs( stdout, "Object::release_ownership():%d Attributes of Object '%s'.%c",
            __LINE__, get_name(), THLA_NEWLINE );
#endif

   // Now do one last check of the divist_requested flag with thread safety and
   // use braces to create scope for the mutex-protection to auto unlock the mutex.
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &ownership_mutex );

      // If there is an ownership_handler, tell it to convert the push / pull maps
      // into checkpoint-able data structures.
      if ( ownership != NULL ) {
         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            send_hs( stdout, "Object::release_ownership():%d Telling ownership handler to clear checkpoint.%c",
                     __LINE__, THLA_NEWLINE );
         }
         ownership->clear_checkpoint();
      }

      if ( !this->divest_requested ) {
#if THLA_OBJ_OWNERSHIP_DEBUG
         send_hs( stdout, "Object::release_ownership():%d NOTE: Another thread beat us to release Attributes of Object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
#endif

         // Another thread beat us to release the Attributes and is processing the
         // divest request so just return.
         return;
      }
      // Clear the flag now that we are servicing the divest request.
      this->divest_requested = false;

      // Unlock the ownership mutex as auto_unlock_mutex goes out of scope.
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
      send_hs( stdout, "Object::release_ownership():%d Attributes of Object '%s'.%c",
               __LINE__, get_name(), THLA_NEWLINE );
   }

   AttributeHandleSet attrs;

   // Create the list of attributes we can divest ownership of.
   for ( int i = 0; i < attr_count; i++ ) {
      // Only release ownership of the attributes we locally own.
      if ( attributes[i].is_divest_requested() && attributes[i].is_locally_owned() ) {
         attrs.insert( attributes[i].get_attribute_handle() );
      }

      // Clear the flag now that the attribute has been handled.
      attributes[i].set_divest_requested( false );
   }

   try {
      // IEEE 1516.1-2000 section 7.6
      rti_amb->confirmDivestiture( this->instance_handle,
                                   attrs,
                                   RTI1516_USERDATA( 0, 0 ) );

      AttributeHandleSet::iterator divest_iter;

      // For the attributes we just divested, mark them as being remotely owned.
      for ( divest_iter = attrs.begin(); divest_iter != attrs.end(); ++divest_iter ) {

         Attribute *trick_hla_attr = get_attribute( *divest_iter );

         if ( trick_hla_attr != NULL ) {

            // The attribute is now divested which means it is now remotely owned.
            trick_hla_attr->mark_remotely_owned();

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               send_hs( stdout, "Object::release_ownership():%d\
\n   DIVESTED Ownership of attribute '%s'->'%s' of object '%s'.%c",
                        __LINE__,
                        get_FOM_name(), trick_hla_attr->get_FOM_name(),
                        get_name(), THLA_NEWLINE );
            }
         }
      }
   } catch ( ObjectInstanceNotKnown &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated ObjectInstanceNotKnown: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( AttributeNotDefined &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated AttributeNotDefined: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( AttributeNotOwned &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated AttributeNotOwned: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( AttributeDivestitureWasNotRequested &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated AttributeDivestitureWasNotRequested: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( NoAcquisitionPending &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated NoAcquisitionPending: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( FederateNotExecutionMember &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated FederateNotExecutionMember: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( SaveInProgress &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated SaveInProgress: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( RestoreInProgress &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated RestoreInProgress: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( NotConnected &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated NotConnected: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( RTIinternalError &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated RTIinternalError: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   } catch ( RTI1516_EXCEPTION &e ) {
      string rti_err_msg;
      StringUtilities::to_string( rti_err_msg, e.what() );
      send_hs( stderr, "Object::release_ownership():%d call to \
RTIAmbassador::confirmDivestiture() generated Exception: '%s'%c",
               __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Clear the flag now that the attributes have been serviced.
   //   this->divest_requested = false;
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
      send_hs( stderr, "Object::pull_ownership():%d Object-Instance-Handle not set for '%s'.%c",
               __LINE__, get_name(), THLA_NEWLINE );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();

   // We need an RTI ambassador to be able to continue.
   if ( rti_amb == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::pull_ownership():%d Unexpected Null RTIambassador!%c",
                  __LINE__, THLA_NEWLINE );
      }
      return;
   }

   double current_time = 0.0;

   Federate *trick_fed = get_federate();

   // Use the HLA Federated Logical Time if time management is enabled.
   if ( ( trick_fed != NULL ) && trick_fed->is_time_management_enabled() ) {

      try {
         HLAinteger64Time HLAtime;
         rti_amb->queryLogicalTime( HLAtime );
         Int64Time fedTime = Int64Time( (int64_t)HLAtime.getTime() );

         // Get the current HLA logical time.
         current_time = fedTime.get_time_in_seconds();
      } catch ( FederateNotExecutionMember &e ) {
         send_hs( stderr, "Object::pull_ownership():%d EXCEPTION: FederateNotExecutionMember %c", __LINE__, THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         send_hs( stderr, "Object::pull_ownership():%d EXCEPTION: SaveInProgress %c", __LINE__, THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         send_hs( stderr, "Object::pull_ownership():%d EXCEPTION: RestoreInProgress %c", __LINE__, THLA_NEWLINE );
      } catch ( NotConnected &e ) {
         send_hs( stderr, "Object::pull_ownership():%d EXCEPTION: NotConnected %c", __LINE__, THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::pull_ownership():%d EXCEPTION: RTIinternalError: '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      // Use the simulation time for the current time.
      current_time = get_federate()->get_execution_control()->get_sim_time();
   }

   THLAAttributeMap *                    attr_map;
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

               Attribute *attr = attr_map_iter->second;

               // Determine if attribute ownership can be pulled.
               if ( attr->is_remotely_owned() && attr->is_publish() ) {

                  // We will try and pull ownership of this attribute.
                  attr_hdl_set.insert( attr->get_attribute_handle() );

                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     send_hs( stdout, "Object::pull_ownership():%d\
\n   Attribute '%s'->'%s' of object '%s'.%c",
                              __LINE__, get_FOM_name(),
                              attr->get_FOM_name(), get_name(), THLA_NEWLINE );
                  }
               } else {
                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     send_hs( stdout, "Object::pull_ownership():%d Can not \
pull Attribute '%s'->'%s' of object '%s' for time %G because it is either already \
owned or is not configured to be published.%c",
                              __LINE__, get_FOM_name(),
                              attr->get_FOM_name(), get_name(), pull_time,
                              THLA_NEWLINE );
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

      // Unlock the ownership mutex when auto_unlock_mutex goes out of scope.
   }

   // Make the request only if we have attributes to pull ownership of.
   if ( attr_hdl_set.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::pull_ownership():%d No active requests, \
%d scheduled request(s) pending for object '%s'.%c",
                  __LINE__,
                  (int)ownership->pull_requests.size(), get_name(), THLA_NEWLINE );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::pull_ownership():%d Pulling ownership \
for Attributes of object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }

      try {
         // IEEE 1516.1-2000 section 7.8
         rti_amb->attributeOwnershipAcquisition(
            this->instance_handle,
            attr_hdl_set,
            RTI1516_USERDATA( get_name(), strlen( get_name() ) + 1 ) );

      } catch ( RTI1516_EXCEPTION &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::pull_ownership():%d Unable to pull attributes of \
object '%s' because of error: '%s'%c",
                  __LINE__, get_name(), rti_err_msg.c_str(),
                  THLA_NEWLINE );
      }
      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }
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
      send_hs( stderr, "Object::grant_pull_request():%d Object-Instance-Handle not set for '%s'%c",
               __LINE__, get_name(), THLA_NEWLINE );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   if ( rti_amb == NULL ) {
      send_hs( stderr, "Object::grant_pull_request():%d Unexpected NULL RTIambassador!%c",
               __LINE__, THLA_NEWLINE );
      return;
   }

   AttributeHandleSet attrs_to_divest;

   // Determine which attributes to grant the pull request for.
   for ( int i = 0; i < attr_count; i++ ) {
      // Another federate is trying to pull ownership so grant the pull
      // request for only the attributes that have a pull request enabled
      // and that we locally own.
      if ( attributes[i].is_pull_requested() && attributes[i].is_locally_owned() ) {
         attrs_to_divest.insert( attributes[i].get_attribute_handle() );
      }

      // Clear the pull requested flag for the attribute.
      attributes[i].set_pull_requested( false );
   }

   if ( attrs_to_divest.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::grant_pull_request():%d No requested attributes to divest ownership of for object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }
   } else {

      try {
         // Divest ownership only if we have attributes we need to do this for.
         auto_ptr< AttributeHandleSet > divested_attrs( new AttributeHandleSet );
         AttributeHandleSet::iterator   divested_iter;

         // IEEE 1516.1-2000 section 7.12
         rti_amb->attributeOwnershipDivestitureIfWanted(
            this->instance_handle,
            attrs_to_divest,
            *divested_attrs );

         if ( divested_attrs->empty() ) {
            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               send_hs( stdout, "Object::grant_pull_request():%d \
No attributes Divested since no federate wanted them for object '%s'.%c",
                        __LINE__, get_name(), THLA_NEWLINE );
            }
         } else {
            // Process the list of attributes that were divisted by the RTI
            // and set the state of the ownership.
            for ( divested_iter = divested_attrs->begin();
                  divested_iter != divested_attrs->end(); ++divested_iter ) {

               // Get the attribute associated with the Attribute Handle.
               Attribute *trick_hla_attr = get_attribute( *divested_iter );

               // Mark the attribute as remotely owned since it was divested.
               if ( trick_hla_attr != NULL ) {

                  trick_hla_attr->mark_remotely_owned();

                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     send_hs( stdout, "Object::grant_pull_request():%d\
\n   DIVESTED Ownership for attribute '%s'->'%s' of object '%s'.%c",
                              __LINE__,
                              get_FOM_name(), trick_hla_attr->get_FOM_name(),
                              get_name(), THLA_NEWLINE );
                  }
               }
            }
         }
      } catch ( RTI1516_EXCEPTION &e ) {
         send_hs( stderr, "Object::grant_pull_request():%d Unable to grant \
pull request for Trick-HLA-Object '%s'%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }

   // Clear the flag now that the pull request has been serviced.
   this->pull_requested = false;
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
      send_hs( stdout, "Object::grant_push_request_pthread():%d Failed to create a thread!%c",
               __LINE__, THLA_NEWLINE );
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
      send_hs( stderr, "Object::grant_push_request():%d Object-Instance-Handle not set for '%s'.%c",
               __LINE__, get_name(), THLA_NEWLINE );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   // Get the RTI-Ambassador.
   RTIambassador *rti_amb = get_RTI_ambassador();

   if ( rti_amb != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::grant_push_request():%d Granting push request for '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }

      AttributeHandleSet attrs;

      // To make the state of the attribute push_requested thread safe we
      // lock the mutex now.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &mutex );

         // Another federate is trying to push the attribute ownership to us so
         // determine which attributes we will take ownership of.
         for ( int i = 0; i < attr_count; i++ ) {
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

            // IEEE 1516.1-2000 section 7.8
            rti_amb->attributeOwnershipAcquisition(
               this->instance_handle,
               attrs,
               RTI1516_USERDATA( get_name(), strlen( get_name() ) + 1 ) );
         } catch ( FederateOwnsAttributes &e ) {
            //            set_locally_owned();

            send_hs( stdout, "Object::grant_push_request():%d Already owns \
the attribute for object '%s'.%c",
                     __LINE__, get_name(), THLA_NEWLINE );
         } catch ( RTI1516_EXCEPTION &e ) {
            send_hs( stderr, "Object::grant_push_request():%d Unable to grant \
push request for object '%s'.%c",
                     __LINE__, get_name(), THLA_NEWLINE );
         }

      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
            send_hs( stdout, "Object::grant_push_request():%d No attributes \
available to acquire ownership for object '%s'.%c",
                     __LINE__, get_name(), THLA_NEWLINE );
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
      send_hs( stdout, "====== Object::ownership_divestiture_pthread_function():%d \
calling negotiated_attribute_ownership_divestiture()%c",
               __LINE__, THLA_NEWLINE );
#endif

      // Divest ownership of the specified set of attribute handles.
      divest_thread_args->trick_hla_obj->negotiated_attribute_ownership_divestiture(
         divest_thread_args->handle_set );

#if THLA_OBJ_OWNERSHIP_DEBUG
      send_hs( stdout, "====== Object::ownership_divestiture_pthread_function():%d \
returned from calling negotiated_attribute_ownership_divestiture()%c",
               __LINE__,
               THLA_NEWLINE );
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
         // IEEE 1516.1-2000 section 7.3
         rti_amb->negotiatedAttributeOwnershipDivestiture(
            this->instance_handle,
            *attr_hdl_set,
            RTI1516_USERDATA( get_name(), strlen( get_name() ) + 1 ) );
      } catch ( ObjectInstanceNotKnown &e ) {
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d ObjectInstanceNotKnown %c", __LINE__, THLA_NEWLINE );
      } catch ( AttributeNotDefined &e ) {
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d AttributeNotDefined %c", __LINE__, THLA_NEWLINE );
      } catch ( AttributeNotOwned &e ) {
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d AttributeNotOwned %c", __LINE__, THLA_NEWLINE );

         // TODO: Need to handle this exception, however every effort has been
         // made to make sure this does not happen.

         //            set_remotely_owned();
         // TODO: Determine if we need to set all attributes as remotely owned
         // if we get this exception? DDexter 6/26/2006
      } catch ( AttributeAlreadyBeingDivested &e ) {
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d AttributeAlreadyBeingDivested %c", __LINE__, THLA_NEWLINE );
      } catch ( FederateNotExecutionMember &e ) {
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d FederateNotExecutionMember %c", __LINE__, THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d SaveInProgress %c", __LINE__, THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         // Be patient.  Evidently it's in progress already (perhaps due to
         // a previous invocation of this function).
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d RestoreInProgress %c", __LINE__, THLA_NEWLINE );
      } catch ( NotConnected &e ) {
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d NotConnected %c", __LINE__, THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d RTIinternalError: '%s'%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      } catch ( RTI1516_EXCEPTION &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::negotiated_attribute_ownership_divestiture():%d Unable to push for '%s': '%s'%c",
                  __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
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
      send_hs( stderr, "Object::push_ownership():%d Object-Instance-Handle not set for '%s'.%c",
               __LINE__, get_name(), THLA_NEWLINE );
      return;
   }

   // Macro to save the FPU Control Word register value.
   TRICKHLA_SAVE_FPU_CONTROL_WORD;

   RTIambassador *rti_amb = get_RTI_ambassador();

   // We need an RTI ambassador to be able to continue.
   if ( rti_amb == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::push_ownership():%d Unexpected Null RTIambassador!%c",
                  __LINE__, THLA_NEWLINE );
      }
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      return;
   }

   // Get the current time.
   double current_time = 0.0;

   Federate *trick_fed = get_federate();

   // Use the HLA Federated Logical Time if time management is enabled.
   if ( ( trick_fed != NULL ) && trick_fed->is_time_management_enabled() ) {

      try {
         HLAinteger64Time HLAtime;
         rti_amb->queryLogicalTime( HLAtime );
         Int64Time fedTime = Int64Time( (int64_t)HLAtime.getTime() );

         // Get the current HLA time value.
         current_time = fedTime.get_time_in_seconds();
      } catch ( FederateNotExecutionMember &e ) {
         send_hs( stderr, "Object::push_ownership():%d EXCEPTION: FederateNotExecutionMember %c", __LINE__, THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         send_hs( stderr, "Object::push_ownership():%d EXCEPTION: SaveInProgress %c", __LINE__, THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         send_hs( stderr, "Object::push_ownership():%d EXCEPTION: RestoreInProgress %c", __LINE__, THLA_NEWLINE );
      } catch ( NotConnected &e ) {
         send_hs( stderr, "Object::push_ownership():%d EXCEPTION: NotConnected %c", __LINE__, THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::push_ownership():%d EXCEPTION: RTIinternalError: '%s'.%c",
                  __LINE__, rti_err_msg.c_str(), THLA_NEWLINE );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   } else {
      // Use the simulation time for the current time.
      current_time = get_federate()->get_execution_control()->get_sim_time();
   }

   THLAAttributeMap *                    attr_map;
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

               Attribute *attr = attr_map_iter->second;

               // Determine if attribute ownership can be pushed.
               if ( attr->is_locally_owned() ) {

                  // We are will try and push ownership of this attribute.
                  attr_hdl_set->insert( attr->get_attribute_handle() );

                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     send_hs( stdout, "Object::push_ownership():%d\
\n   Attribute '%s'->'%s' of object '%s'.%c",
                              __LINE__, get_FOM_name(),
                              attr->get_FOM_name(), get_name(), THLA_NEWLINE );
                  }
               } else {
                  if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
                     send_hs( stdout, "Object::push_ownership():%d Can not \
push Attribute '%s'->'%s' of object '%s' for time %G because it is either already \
owned or is not configured to be published.%c",
                              __LINE__, get_FOM_name(),
                              attr->get_FOM_name(), get_name(), push_time, THLA_NEWLINE );
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
      // Unlock the ownership mutex when auto_unlock_mutex goes out of scope.
   }

   // Determine if we have any attributes to push ownership of.
   if ( attr_hdl_set->empty() ) {
      // Make sure we delete the attribute handle set so that we don't have
      // a memory leak.
      delete attr_hdl_set;

      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::push_ownership():%d No active requests, \
%d scheduled request(s) pending for object '%s'.%c",
                  __LINE__,
                  (int)ownership->push_requests.size(), get_name(), THLA_NEWLINE );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::push_ownership():%d Pushing ownership \
for Attributes of object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
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
         exec_terminate( __FILE__, "Object::push_ownership() Failed to\
 create ownership divestiture pthread!" );
         exit( 0 );
      }
   }
}

void Object::setup_ownership_transfer_checkpointed_data()
{
   if ( ownership != static_cast< OwnershipHandler * >( NULL ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::setup_ownership_transfer_checkpointed_data():%d Object: %s.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }
      ownership->setup_checkpoint_requests();
   }
}

void Object::restore_ownership_transfer_checkpointed_data()
{
   if ( ownership != static_cast< OwnershipHandler * >( NULL ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::restore_ownership_transfer_checkpointed_data():%d Object: %s.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }
      ownership->restore_requests();
   }
}

void Object::set_core_job_cycle_time(
   double cycle_time )
{
   for ( int i = 0; i < attr_count; i++ ) {
      attributes[i].determine_cycle_ratio( cycle_time );
   }
}

void Object::set_name(
   const char *new_name )
{
   // Delete the existing memory used by the name.
   if ( this->name != static_cast< char * >( NULL ) ) {
      if ( TMM_is_alloced( name ) ) {
         TMM_delete_var_a( name );
      }
   }

   // Allocate appropriate size string and copy data.
   if ( new_name != NULL ) {
      this->name = TMM_strdup( (char *)new_name );
   } else {
      this->name = TMM_strdup( (char *)"" );
   }
}

void Object::build_attribute_map()
{
   thla_attribute_map.clear();
   for ( int i = 0; i < attr_count; i++ ) {
      thla_attribute_map[attributes[i].get_attribute_handle()] = &attributes[i];
   }
}

Attribute *Object::get_attribute(
   RTI1516_NAMESPACE::AttributeHandle attr_handle )
{
   // We use a map with the key being the AttributeHandle for fast lookups.
   AttributeMap::const_iterator iter = thla_attribute_map.find( attr_handle );
   return ( ( iter != thla_attribute_map.end() ) ? iter->second : static_cast< Attribute * >( NULL ) );
}

Attribute *Object::get_attribute(
   string attr_FOM_name )
{
   return get_attribute( attr_FOM_name.c_str() );
}

Attribute *Object::get_attribute(
   const char *attr_FOM_name )
{
   if ( attr_FOM_name != NULL ) {
      for ( int i = 0; i < attr_count; i++ ) {
         if ( strcmp( attr_FOM_name, attributes[i].get_FOM_name() ) == 0 ) {
            return ( &attributes[i] );
         }
      }
   }
   return NULL;
}

void Object::stop_publishing_attributes()
{
   for ( int i = 0; i < attr_count; i++ ) {
      attributes[i].set_publish( false );
   }
}

void Object::stop_subscribing_attributes()
{
   for ( int i = 0; i < attr_count; i++ ) {
      attributes[i].set_subscribe( false );
   }
}

bool Object::any_attribute_published()
{
   for ( int i = 0; i < attr_count; i++ ) {
      if ( attributes[i].is_publish() ) {
         return true;
      }
   }
   return false; // No attribute published.
}

bool Object::any_attribute_subscribed()
{
   for ( int i = 0; i < attr_count; i++ ) {
      if ( attributes[i].is_subscribe() ) {
         return true;
      }
   }
   return false; // No attribute subscribed.
}

bool Object::any_locally_owned_attribute()
{
   for ( int i = 0; i < attr_count; i++ ) {
      if ( attributes[i].is_locally_owned() ) {
         return true;
      }
   }
   return false; // No attribute locally owned.
}

bool Object::any_locally_owned_published_attribute()
{
   for ( int i = 0; i < attr_count; i++ ) {
      if ( attributes[i].is_locally_owned() && attributes[i].is_publish() ) {
         return true;
      }
   }
   return false; // No attribute locally owned and published.
}

bool Object::any_locally_owned_published_attribute(
   const DataUpdateEnum attr_config )
{
   for ( int i = 0; i < attr_count; i++ ) {
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
                || ( ( data_cycle_ready && ( ( attributes[i].get_configuration() & CONFIG_CYCLIC ) == CONFIG_CYCLIC ) ) ) ) ) {
         any_ready = true;
      }
   }
   return any_ready;
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
   const DataUpdateEnum attr_config )
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
   const DataUpdateEnum attr_config,
   const bool           include_requested )
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( ( include_requested && attributes[i].is_update_requested() )
           || ( attributes[i].get_configuration() & attr_config ) == attr_config ) {
         attributes[i].pack_attribute_buffer();
      }
   }
}

void Object::unpack_attribute_buffers(
   const DataUpdateEnum attr_config )
{
   for ( int i = 0; i < attr_count; ++i ) {
      if ( ( attributes[i].get_configuration() & attr_config ) == attr_config ) {
         attributes[i].unpack_attribute_buffer();
      }
   }
}

void Object::set_to_unblocking_cyclic_reads()
{
#ifdef THLA_THREAD_WAIT_FOR_DATA
   pthread_mutex_lock( &data_change_mutex );
#endif

   this->blocking_cyclic_read       = false;
   this->first_blocking_cyclic_read = true;

#ifdef THLA_THREAD_WAIT_FOR_DATA
   pthread_cond_signal( &data_change_cv );
   pthread_mutex_unlock( &data_change_mutex );
#endif
}

void Object::notify_attribute_ownership_changed()
{
#ifdef THLA_THREAD_WAIT_FOR_DATA
   pthread_mutex_lock( &data_change_mutex );
   pthread_cond_signal( &data_change_cv );
   pthread_mutex_unlock( &data_change_mutex );
#endif
}

void Object::mark_changed()
{
#ifdef THLA_THREAD_WAIT_FOR_DATA
   pthread_mutex_lock( &data_change_mutex );
#endif

   this->changed = true;

#ifdef THLA_THREAD_WAIT_FOR_DATA
   pthread_cond_signal( &data_change_cv );
   pthread_mutex_unlock( &data_change_mutex );
#endif
}

void Object::mark_unchanged()
{
#ifdef THLA_THREAD_WAIT_FOR_DATA
   pthread_mutex_lock( &data_change_mutex );
#endif

   this->changed = false;

   // Clear the change flag for each of the attributes as well.
   for ( int i = 0; i < attr_count; i++ ) {
      attributes[i].mark_unchanged();
   }

#ifdef THLA_THREAD_WAIT_FOR_DATA
   pthread_cond_signal( &data_change_cv );
   pthread_mutex_unlock( &data_change_mutex );
#endif
}

Int64Time const &Object::get_update_time_plus_lookahead()
{
   time_plus_lookahead = last_update_time;

   // Get the Trick-Federate.
   Federate *trick_fed = get_federate();
   if ( trick_fed != NULL ) {
      time_plus_lookahead += trick_fed->get_lookahead();
   } else {
      send_hs( stderr, "Object::get_update_time_plus_lookahead():%d Unexpected NULL Trick-Federate.%c",
               __LINE__, THLA_NEWLINE );
   }
   return ( time_plus_lookahead );
}

double Object::get_granted_time() const
{
   return ( ( manager != NULL ) ? manager->get_granted_time() : 0.0 );
}

/*!
 * @details  If the manager does not exist, -1.0 seconds is returned.
 */
Int64Interval Object::get_fed_lookahead() const
{
   if ( manager != NULL ) {
      return manager->get_fed_lookahead();
   } else {
      Int64Interval di( -1.0 );
      return di;
   }
}

/*!
 * @details If the manager does not exist, MAX_LOGICAL_TIME_SECONDS is returned.
 */
Int64Time Object::get_granted_fed_time() const
{
   if ( manager != NULL ) {
      return manager->get_granted_fed_time();
   } else {
      Int64Time dt( MAX_LOGICAL_TIME_SECONDS );
      return dt;
   }
}

void Object::pull_ownership_upon_rejoin()
{
   // Make sure we have an Instance ID for the object, otherwise just return.
   if ( !is_instance_handle_valid() ) {
      send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d Object-Instance-Handle not set for '%s'.%c",
               __LINE__, get_name(), THLA_NEWLINE );
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
   for ( int i = 0; i < attr_count; i++ ) {

      try {
         // IEEE 1516.1-2000 section 7.18
         if ( attributes[i].is_publish() && attributes[i].is_locally_owned() && !rti_amb->isAttributeOwnedByFederate( this->instance_handle, attributes[i].get_attribute_handle() ) ) {

            // RTI tells us that this attribute is not owned by this federate. Add
            // attribute handle to the collection for the impending ownership pull.
            attr_hdl_set.insert( attributes[i].get_attribute_handle() );

            // Turn off the 'locally_owned' flag on this attribute since the RTI
            // just informed us that we do not own this attribute, regardless of
            // what the input file may have indicated.
            attributes[i].unmark_locally_owned();

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
               send_hs( stdout, "Object::pull_ownership_upon_rejoin():%d \
Ownership check of Attribute '%s'->'%s' from object '%s' => RTI informed us that we DO NOT own it.%c",
                        __LINE__, get_FOM_name(), attributes[i].get_FOM_name(),
                        get_name(), THLA_NEWLINE );
            }
         }
      } catch ( ObjectInstanceNotKnown &e ) {
         send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: ObjectInstanceNotKnown %c",
                  __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
      } catch ( AttributeNotDefined &e ) {
         send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: AttributeNotDefined %c",
                  __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
      } catch ( FederateNotExecutionMember &e ) {
         send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: FederateNotExecutionMember %c",
                  __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
      } catch ( SaveInProgress &e ) {
         send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: SaveInProgress %c",
                  __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
      } catch ( RestoreInProgress &e ) {
         send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: RestoreInProgress %c",
                  __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
      } catch ( RTIinternalError &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an RTIinternalError: %s%c",
                  __LINE__, attributes[i].get_FOM_name(), rti_err_msg.c_str(),
                  THLA_NEWLINE );
      }

      // Macro to restore the saved FPU Control Word register value.
      TRICKHLA_RESTORE_FPU_CONTROL_WORD;
      TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
   }

   // Make the request only if we do have any attributes for which we need to pull ownership.
   if ( attr_hdl_set.empty() ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::pull_ownership_upon_rejoin():%d No ownership \
requests were added for object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OBJECT ) ) {
         send_hs( stdout, "Object::pull_ownership_upon_rejoin():%d Pulling ownership \
for Attributes of object '%s'.%c",
                  __LINE__, get_name(), THLA_NEWLINE );
      }

      try {
         // IEEE 1516.1-2000 section 7.8
         rti_amb->attributeOwnershipAcquisition(
            this->instance_handle,
            attr_hdl_set,
            RTI1516_USERDATA( get_name(), strlen( get_name() ) + 1 ) );

      } catch ( RTI1516_EXCEPTION &e ) {
         string rti_err_msg;
         StringUtilities::to_string( rti_err_msg, e.what() );
         send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
Unable to pull ownership for the attributes of object '%s' because of error: '%s'%c",
                  __LINE__, get_name(), rti_err_msg.c_str(), THLA_NEWLINE );
      }

      Federate *trick_fed = get_federate();

      SleepTimeout sleep_timer( 10.0, 1000 );

      // Perform a blocking loop until ownership of all locally owned published
      // attributes is restored...
      unsigned int ownership_counter = 0;
      while ( ownership_counter < attr_hdl_set.size() ) {

         // reset ownership count for this loop through all the attributes
         ownership_counter = 0;

         for ( int i = 0; i < attr_count; i++ ) {
            try {
               // IEEE 1516.1-2000 section 7.18
               if ( attributes[i].is_publish() && attributes[i].is_locally_owned() && rti_amb->isAttributeOwnedByFederate( this->instance_handle, attributes[i].get_attribute_handle() ) ) {
                  ownership_counter++;
               }
            } catch ( ObjectInstanceNotKnown &e ) {
               send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: ObjectInstanceNotKnown %c",
                        __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
            } catch ( AttributeNotDefined &e ) {
               send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s'generated an EXCEPTION: AttributeNotDefined %c",
                        __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
            } catch ( FederateNotExecutionMember &e ) {
               send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: FederateNotExecutionMember %c",
                        __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
            } catch ( SaveInProgress &e ) {
               send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: SaveInProgress %c",
                        __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
            } catch ( RestoreInProgress &e ) {
               send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an EXCEPTION: RestoreInProgress %c",
                        __LINE__, attributes[i].get_FOM_name(), THLA_NEWLINE );
            } catch ( RTIinternalError &e ) {
               string rti_err_msg;
               StringUtilities::to_string( rti_err_msg, e.what() );
               send_hs( stderr, "Object::pull_ownership_upon_rejoin():%d \
rti_amb->isAttributeOwnedByFederate() call for published attribute '%s' generated an RTIinternalError: %s%c",
                        __LINE__, attributes[i].get_FOM_name(),
                        rti_err_msg.c_str(), THLA_NEWLINE );
            }
         } // end of 'for' loop

         (void)sleep_timer.sleep();

         if ( ( ownership_counter < attr_hdl_set.size() ) && sleep_timer.timeout() ) {
            sleep_timer.reset();
            if ( !trick_fed->is_execution_member() ) {
               ostringstream errmsg;
               errmsg << "Object::pull_ownership_upon_rejoin():" << __LINE__
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
      } // end of 'while' loop
   }

   // Macro to restore the saved FPU Control Word register value.
   TRICKHLA_RESTORE_FPU_CONTROL_WORD;
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Unlock the ownership mutex when auto_unlock_mutex goes out of scope.
}

bool Object::is_shutdown_called() const
{
   return ( ( this->manager != NULL ) ? this->manager->is_shutdown_called() : false );
}
