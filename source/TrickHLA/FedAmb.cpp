/*!
@file TrickHLA/FedAmb.cpp
@ingroup TrickHLA
@brief Provides methods called by the RTI Ambassador for simulation object,
interaction and time management.

@details Methods of objects of this class are not intended to be called from
the Trick S_define level.  However, this class is essentially a polymorphic
callback class provided to the RTI Ambassador.

\par<b>Assumptions and Limitations:</b>
- Derived class of abstract FederateAmbassador class to implement methods so
that RTI can call functions in the federate.
- Based on HelloWorld example code.
- None of the methods in this class are intended to be called from the Trick
S_define level.  However, an instance of this class can be declared in the
S_define.

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
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{FedAmb.cpp}

@revs_title
@revs_begin
@rev_entry{DMSO Programmer, DMSO, HLA, Mar 1998, --, HelloWorld Federate Ambassador.}
@rev_entry{Edwin Z. Crues, Titan Systems Corp., DIS, Feb 2002, --, HLA Ball Sim.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <iostream>
#include <sstream>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/FedAmb.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/Utilities.hh"
#ifdef THLA_OBJECT_TIME_LOGGING
#include "TrickHLA/BasicClock.hh"
#endif

using namespace std;
using namespace RTI1516_NAMESPACE;
using namespace TrickHLA;

/*!
 * @details In most cases, we would allocate and set default names in the
 * constructor.  However, since we want this class to be Input
 * Processor friendly, we cannot do that here since the Input
 * Processor may not have been initialized yet.  So, we have to
 * set the name information to NULL and then allocate and set the
 * defaults in the initialization job if not already set in the
 * input stream.
 * @job_class{initialization}
 */
FedAmb::FedAmb()
   : FederateAmbassador(),
     federate( NULL ),
     manager( NULL ),
#ifdef THLA_OBJECT_TIME_LOGGING
     clock(),
#endif
     federation_restore_status_response_context_switch( false ), // process, not echo.
     federation_restored_rebuild_federate_handle_set( false )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
FedAmb::~FedAmb() throw()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void FedAmb::setup(
   Federate &federate,
   Manager & manager )
{

   // Set the associated TrickHLA Federate and Manager references.
   this->federate = &federate;
   this->manager  = &manager;
}

/*!
 * @job_class{initialization}
 */
void FedAmb::initialize()
{
   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;

   // Check to make sure we have a reference to the TrickHLA::FedAmb.
   if ( federate == NULL ) {
      ostringstream errmsg;
      errmsg << "FedAmb::initialize():" << __LINE__
             << " Unexpected NULL TrickHLA::Federate." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
      return;
   }

   // Check to make sure we have a reference to the TrickHLA::Manager.
   if ( manager == NULL ) {
      ostringstream errmsg;
      errmsg << "FedAmb::initialize():" << __LINE__
             << " Unexpected NULL TrickHLA::Manager." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
      return;
   }

   // Generate a valid federate name.
   const char *fed_name = federate->get_federate_name();

   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::initialize():%d Federate:\"%s\"%c",
               __LINE__, fed_name, THLA_NEWLINE );
   }

   if ( ( fed_name == NULL ) || ( *fed_name == '\0' ) ) {
      ostringstream errmsg;
      errmsg << "FedAmb::initialize():" << __LINE__
             << " Unexpected NULL federate name." << THLA_ENDL;
      send_hs( stderr, (char *)errmsg.str().c_str() );
      exec_terminate( __FILE__, (char *)errmsg.str().c_str() );
      return;
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

bool FedAmb::should_print(
   const DebugLevelEnum & level,
   const DebugSourceEnum &code ) const
{
   if ( manager != NULL ) {
      return manager->should_print( level, code );
   }
   return true;
}

////////////////////////////////////
// Federation Management Services //
////////////////////////////////////

void FedAmb::connectionLost(
   wstring const &faultDescription ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   string faultMsg;
   StringUtilities::to_string( faultMsg, faultDescription );
   ostringstream msg;
   msg << "FedAmb::connectionLost():" << __LINE__
       << " Lost the connection to the Central RTI Component (CRC)."
       << " Reason:'" << faultMsg << "'."
       << " Terminating the simulation!" << THLA_ENDL;
   send_hs( stderr, (char *)msg.str().c_str() );
   exec_terminate( __FILE__, (char *)msg.str().c_str() );
}

void FedAmb::reportFederationExecutions(
   RTI1516_NAMESPACE::FederationExecutionInformationVector const &theFederationExecutionInformationList ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::reportFederationExecutions():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

void FedAmb::synchronizationPointRegistrationSucceeded(
   wstring const &label ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::synchronizationPointRegistrationSucceeded():%d Label:'%ls'%c",
               __LINE__, label.c_str(), THLA_NEWLINE );
   }

   federate->sync_point_registration_succeeded( label );
}

void FedAmb::synchronizationPointRegistrationFailed(
   wstring const &                                      label,
   RTI1516_NAMESPACE::SynchronizationPointFailureReason reason ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::synchronizationPointRegistrationFailed():%d Label:'%ls'%c",
               __LINE__, label.c_str(), THLA_NEWLINE );
   }

   bool not_unique = ( reason == RTI1516_NAMESPACE::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE );

   federate->sync_point_registration_failed( label, not_unique );
}

void FedAmb::announceSynchronizationPoint(
   wstring const &                              label,
   RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   federate->announce_sync_point( label, theUserSuppliedTag );
}

void FedAmb::federationSynchronized(
   wstring const &                             label,
   RTI1516_NAMESPACE::FederateHandleSet const &failedToSyncSet ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::federationSynchronized():%d Label:'%ls'%c",
               __LINE__, label.c_str(), THLA_NEWLINE );
   }

   federate->federation_synchronized( label );

   if ( !failedToSyncSet.empty() ) {
      FederateHandleSet::const_iterator iter;
      string                            strIds, id;
      for ( iter = failedToSyncSet.begin(); iter != failedToSyncSet.end(); ++iter ) {
         StringUtilities::to_string( id, *iter );
         strIds += id;
         strIds += " ";
      }
      send_hs( stderr, "FedAmb::federationSynchronized():%d ERROR: these \
federate handles failed to synchronize on sync-point '%ls': %s%c",
               __LINE__,
               label.c_str(), strIds.c_str(), THLA_NEWLINE );
   }
}

void FedAmb::initiateFederateSave(
   wstring const &label ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::initiateFederateSave():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->set_save_name( label );
   federate->set_start_to_save( true );
}

void FedAmb::initiateFederateSave(
   wstring const &                       label,
   RTI1516_NAMESPACE::LogicalTime const &theTime ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::initiateFederateSave():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->set_save_name( label );
   federate->set_start_to_save( true );
}

void FedAmb::federationSaved() throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::federationSaved():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->set_start_to_save( false );
   federate->set_save_completed();
   federate->federation_saved();
}

void FedAmb::federationNotSaved(
   RTI1516_NAMESPACE::SaveFailureReason theSaveFailureReason ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::federationNotSaved():%d %c",
               __LINE__, THLA_NEWLINE );
   }

   federate->print_save_failure_reason( theSaveFailureReason );

   // TODO: Do we need to the steps below to exit freeze mode?
   federate->set_start_to_save( false );
   federate->set_save_completed();
   federate->federation_saved();
}

void FedAmb::federationSaveStatusResponse(
   RTI1516_NAMESPACE::FederateHandleSaveStatusPairVector const &theFederateStatusVector ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::federationSaveStatusResponse():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->process_requested_federation_save_status( theFederateStatusVector );
}

void FedAmb::requestFederationRestoreSucceeded(
   wstring const &label ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::requestFederationRestoreSucceeded():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->set_restore_request_succeeded();
   federate->requested_federation_restore_status( true );
}

void FedAmb::requestFederationRestoreFailed(
   wstring const &label ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::requestFederationRestoreFailed():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->set_restore_request_failed();
   federate->requested_federation_restore_status( false );
}

void FedAmb::federationRestoreBegun() throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::federationRestoreBegun():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->set_restore_begun();
}

void FedAmb::initiateFederateRestore(
   wstring const &                   label,
   wstring const &                   federateName,
   RTI1516_NAMESPACE::FederateHandle handle ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string name;
      StringUtilities::to_string( name, federateName );
      send_hs( stdout, "FedAmb::initiateFederateRestore():%d for federate '%s'%c",
               __LINE__, name.c_str(), THLA_NEWLINE );
   }
   federate->set_start_to_restore( true );
   federate->set_restore_name( label );
}

void FedAmb::federationRestored() throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::federationRestored():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->set_restore_completed();
}

void FedAmb::federationNotRestored(
   RTI1516_NAMESPACE::RestoreFailureReason theRestoreFailureReason ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::federationNotRestored():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   federate->set_restore_failed();
   federate->print_restore_failure_reason( theRestoreFailureReason );
}

void FedAmb::federationRestoreStatusResponse(
   RTI1516_NAMESPACE::FederateRestoreStatusVector const &theFederateStatusVector ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::federationRestoreStatusResponse():%d %c",
               __LINE__, THLA_NEWLINE );
   }
   if ( federation_restore_status_response_context_switch == 0 ) { // process
      federate->process_requested_federation_restore_status( theFederateStatusVector );
   } else if ( federation_restore_status_response_context_switch == 1 ) { // echo
      federate->print_requested_federation_restore_status( theFederateStatusVector );
   }
}

/////////////////////////////////////
// Declaration Management Services //
/////////////////////////////////////

void FedAmb::startRegistrationForObjectClass(
   RTI1516_NAMESPACE::ObjectClassHandle theClass ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::startRegistrationForObjectClass():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

void FedAmb::stopRegistrationForObjectClass(
   RTI1516_NAMESPACE::ObjectClassHandle theClass ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::stopRegistrationForObjectClass():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

void FedAmb::turnInteractionsOn(
   RTI1516_NAMESPACE::InteractionClassHandle theHandle ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::turnInteractionsOn():%d %c",
            federate->get_federate_name(),
            __LINE__, THLA_NEWLINE );
}

void FedAmb::turnInteractionsOff(
   RTI1516_NAMESPACE::InteractionClassHandle theHandle ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::turnInteractionsOff():%d %c",
            federate->get_federate_name(),
            __LINE__, THLA_NEWLINE );
}

////////////////////////////////
// Object Management Services //
////////////////////////////////

// 6.3
void FedAmb::objectInstanceNameReservationSucceeded(
   wstring const &theObjectInstanceName ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( manager != NULL ) {
      if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string instance_name;
         StringUtilities::to_string( instance_name, theObjectInstanceName );
         send_hs( stdout, "FedAmb::objectInstanceNameReservationSucceeded():%d '%s'%c",
                  __LINE__, instance_name.c_str(), THLA_NEWLINE );
      }

      manager->object_instance_name_reservation_succeeded( theObjectInstanceName );
   }
}

// 6.3
void FedAmb::objectInstanceNameReservationFailed(
   wstring const &theObjectInstanceName ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( manager != NULL ) {
      if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string instance_name;
         StringUtilities::to_string( instance_name, theObjectInstanceName );
         send_hs( stdout, "FedAmb::objectInstanceNameReservationFailed():%d FAILED '%s'%c",
                  __LINE__, instance_name.c_str(), THLA_NEWLINE );
      }

      manager->object_instance_name_reservation_failed( theObjectInstanceName );
   }
}

// 6.6
void FedAmb::multipleObjectInstanceNameReservationSucceeded(
   set< wstring > const &theObjectInstanceNames ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( manager != NULL ) {

      set< wstring >::const_iterator iter;
      for ( iter = theObjectInstanceNames.begin();
            iter != theObjectInstanceNames.end(); ++iter ) {
         if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            string instance_name;
            StringUtilities::to_string( instance_name, *iter );
            send_hs( stdout, "FedAmb::objectInstanceNameReservationSucceeded():%d '%s'%c",
                     __LINE__, instance_name.c_str(), THLA_NEWLINE );
         }

         manager->object_instance_name_reservation_succeeded( *iter );
      }
   }
}

void FedAmb::multipleObjectInstanceNameReservationFailed(
   set< wstring > const &theObjectInstanceNames ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( manager != NULL ) {

      set< wstring >::const_iterator iter;
      for ( iter = theObjectInstanceNames.begin();
            iter != theObjectInstanceNames.end(); ++iter ) {
         if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            string instance_name;
            StringUtilities::to_string( instance_name, *iter );
            send_hs( stdout, "FedAmb::objectInstanceNameReservationFailed():%d FAILED '%s'%c",
                     __LINE__, instance_name.c_str(), THLA_NEWLINE );
         }

         manager->object_instance_name_reservation_failed( *iter );
      }
   }
}

// 6.5
void FedAmb::discoverObjectInstance(
   RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
   RTI1516_NAMESPACE::ObjectClassHandle    theObjectClass,
   wstring const &                         theObjectInstanceName ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string id_str, name_str;
      StringUtilities::to_string( id_str, theObject );
      StringUtilities::to_string( name_str, theObjectInstanceName );
      send_hs( stdout, "FedAmb::discoverObjectInstance():%d DISCOVERED '%s' Instance-ID:%s%c",
               __LINE__, name_str.c_str(), id_str.c_str(), THLA_NEWLINE );
   }

   if ( manager == NULL ) {
      if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, theObject );
         StringUtilities::to_string( name_str, theObjectInstanceName );
         send_hs( stdout, "FedAmb::discoverObjectInstance():%d Unexpected \
NULL Manager! Can't do anything with discovered object '%s' Instance-ID:%s%c",
                  __LINE__, name_str.c_str(), id_str.c_str(), THLA_NEWLINE );
      }
   } else if ( !manager->discover_object_instance( theObject, theObjectClass, theObjectInstanceName ) ) {
      if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, theObject );
         StringUtilities::to_string( name_str, theObjectInstanceName );
         send_hs( stderr, "FedAmb::discoverObjectInstance():%d Object '%s' with Instance-ID:%s is UNKNOWN to me!%c",
                  __LINE__, name_str.c_str(), id_str.c_str(), THLA_NEWLINE );
      }
   }
}

void FedAmb::discoverObjectInstance(
   RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
   RTI1516_NAMESPACE::ObjectClassHandle    theObjectClass,
   wstring const &                         theObjectInstanceName,
   RTI1516_NAMESPACE::FederateHandle       producingFederate ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string fed_id;
      StringUtilities::to_string( fed_id, producingFederate );
      send_hs( stdout, "FedAmb::discoverObjectInstance(producing \
federate '%s'):%d calling 'discoverObjectInstance' to finish the discovery.%c",
               fed_id.c_str(), __LINE__, THLA_NEWLINE );
   }

   discoverObjectInstance( theObject, theObjectClass, theObjectInstanceName );
}

// 6.7
void FedAmb::reflectAttributeValues(
   RTI1516_NAMESPACE::ObjectInstanceHandle           theObject,
   RTI1516_NAMESPACE::AttributeHandleValueMap const &theAttributeValues,
   RTI1516_NAMESPACE::VariableLengthData const &     theUserSuppliedTag,
   RTI1516_NAMESPACE::OrderType                      sentOrder,
   RTI1516_NAMESPACE::TransportationType             theType,
   RTI1516_NAMESPACE::SupplementalReflectInfo        theReflectInfo ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
#ifdef THLA_OBJECT_TIME_LOGGING
   // Get the current GMT time in seconds.
   double gmt_secs = clock.get_time();
#endif

   // Get the TrickHLA object for the given Object Instance Handle.
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( theObject ) : NULL;

   if ( trickhla_obj != NULL ) {
      if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         send_hs( stdout, "FedAmb:reflectAttributeValues():%d '%s'%c",
                  __LINE__, trickhla_obj->get_name(), THLA_NEWLINE );
      }
#ifdef THLA_OBJECT_TIME_LOGGING
      // Set the time the data was received in this callback.
      trickhla_obj->received_gmt_time = gmt_secs;
#endif

      // Pass the attribute values off to the object.
#if defined( THLA_QUEUE_REFLECTED_ATTRIBUTES )
      trickhla_obj->enqueue_data( (AttributeHandleValueMap &)theAttributeValues );
#else
      trickhla_obj->extract_data( (AttributeHandleValueMap &)theAttributeValues );
#endif
   } else if ( ( federate != NULL ) && federate->is_federate_instance_id( theObject ) ) {

      if ( federation_restored_rebuild_federate_handle_set ) {
         if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            string id_str;
            StringUtilities::to_string( id_str, theObject );
            send_hs( stdout, "FedAmb::reflectAttributeValues(%d elements):%d Rebuilding federate handle for Federate ID:%s%c",
                     (int)theAttributeValues.size(),
                     __LINE__, id_str.c_str(), THLA_NEWLINE );
         }
         federate->rebuild_federate_handles( theObject, theAttributeValues );
      } else {
         if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            string id_str;
            StringUtilities::to_string( id_str, theObject );
            send_hs( stdout, "FedAmb::reflectAttributeValues():%d Setting name for Federate ID:%s%c",
                     __LINE__, id_str.c_str(), THLA_NEWLINE );
         }
         federate->set_MOM_HLAfederate_instance_attributes( theObject, theAttributeValues );
      }
   } else if ( ( federate != NULL ) && federate->is_MOM_HLAfederation_instance_id( theObject ) ) {
      // This was an instance-ID for the Federation and not a federate.
      if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str;
         StringUtilities::to_string( id_str, theObject );
         send_hs( stdout, "FedAmb::reflectAttributeValues():%d Setting name for Federation ID:%s%c",
                  __LINE__, id_str.c_str(), THLA_NEWLINE );
      }
      federate->set_MOM_HLAfederation_instance_attributes( theObject, theAttributeValues );
   } else {
      if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str;
         StringUtilities::to_string( id_str, theObject );
         send_hs( stderr, "FedAmb::reflectAttributeValues():%d Received update to Unknown Object Instance, ID:%s%c",
                  __LINE__, id_str.c_str(), THLA_NEWLINE );
      }
   }
}

void FedAmb::reflectAttributeValues(
   RTI1516_NAMESPACE::ObjectInstanceHandle           theObject,
   RTI1516_NAMESPACE::AttributeHandleValueMap const &theAttributeValues,
   RTI1516_NAMESPACE::VariableLengthData const &     theUserSuppliedTag,
   RTI1516_NAMESPACE::OrderType                      sentOrder,
   RTI1516_NAMESPACE::TransportationType             theType,
   RTI1516_NAMESPACE::LogicalTime const &            theTime,
   RTI1516_NAMESPACE::OrderType                      receivedOrder,
   RTI1516_NAMESPACE::SupplementalReflectInfo        theReflectInfo ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
#ifdef THLA_OBJECT_TIME_LOGGING
   // Get the current GMT time in seconds.
   double gmt_secs = clock.get_time();
#endif

   // Get the TrickHLA object for the given Object Instance Handle.
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( theObject ) : NULL;

   if ( trickhla_obj != NULL ) {

      if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         Int64Time time;
         time.set( theTime );
         send_hs( stdout, "FedAmb:reflectAttributeValues():%d '%s' time:%f %c",
                  __LINE__, trickhla_obj->get_name(), time.get_double_time(),
                  THLA_NEWLINE );
      }

#ifdef THLA_OBJECT_TIME_LOGGING
      // Set the time the data was received in this callback.
      trickhla_obj->received_gmt_time = gmt_secs;
#endif

      // Pass the attribute values off to the object.
      trickhla_obj->set_last_update_time( theTime );
#if defined( THLA_QUEUE_REFLECTED_ATTRIBUTES )
      trickhla_obj->enqueue_data( (AttributeHandleValueMap &)theAttributeValues );
#else
      trickhla_obj->extract_data( (AttributeHandleValueMap &)theAttributeValues );
#endif
   } else {
      if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str;
         StringUtilities::to_string( id_str, theObject );
         send_hs( stderr, "FedAmb::reflectAttributeValues():%d Received update to Unknown Object Instance, ID:%s%c",
                  __LINE__, id_str.c_str(), THLA_NEWLINE );
      }
   }
}

void FedAmb::reflectAttributeValues(
   RTI1516_NAMESPACE::ObjectInstanceHandle           theObject,
   RTI1516_NAMESPACE::AttributeHandleValueMap const &theAttributeValues,
   RTI1516_NAMESPACE::VariableLengthData const &     theUserSuppliedTag,
   RTI1516_NAMESPACE::OrderType                      sentOrder,
   RTI1516_NAMESPACE::TransportationType             theType,
   RTI1516_NAMESPACE::LogicalTime const &            theTime,
   RTI1516_NAMESPACE::OrderType                      receivedOrder,
   RTI1516_NAMESPACE::MessageRetractionHandle        theHandle,
   RTI1516_NAMESPACE::SupplementalReflectInfo        theReflectInfo ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
#ifdef THLA_OBJECT_TIME_LOGGING
   // Get the current GMT time in seconds.
   double gmt_secs = clock.get_time();
#endif

   // Get the TrickHLA object for the given Object Instance Handle.
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( theObject ) : NULL;

   if ( trickhla_obj != NULL ) {

      if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         Int64Time time;
         time.set( theTime );
         send_hs( stdout, "FedAmb:reflectAttributeValues():%d '%s' time:%f %c",
                  __LINE__, trickhla_obj->get_name(), time.get_double_time(), THLA_NEWLINE );
      }

#ifdef THLA_OBJECT_TIME_LOGGING
      // Set the time the data was received in this callback.
      trickhla_obj->received_gmt_time = gmt_secs;
#endif

      // Pass the attribute values off to the object.
      trickhla_obj->set_last_update_time( theTime );
#if defined( THLA_QUEUE_REFLECTED_ATTRIBUTES )
      trickhla_obj->enqueue_data( (AttributeHandleValueMap &)theAttributeValues );
#else
      trickhla_obj->extract_data( (AttributeHandleValueMap &)theAttributeValues );
#endif
   } else {
      if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str;
         StringUtilities::to_string( id_str, theObject );
         send_hs( stderr, "FedAmb::reflectAttributeValues():%d Received update to Unknown Object Instance, ID:%s%c",
                  __LINE__, id_str.c_str(), THLA_NEWLINE );
      }
   }
}

void FedAmb::receiveInteraction(
   RTI1516_NAMESPACE::InteractionClassHandle         theInteraction,
   RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
   RTI1516_NAMESPACE::VariableLengthData const &     theUserSuppliedTag,
   RTI1516_NAMESPACE::OrderType                      sentOrder,
   RTI1516_NAMESPACE::TransportationType             theType,
   RTI1516_NAMESPACE::SupplementalReceiveInfo        theReceiveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( manager == NULL ) {
      send_hs( stderr, "FedAmb::receiveInteraction():%d NULL Manager!%c",
               __LINE__, THLA_NEWLINE );
   } else {
      Int64Time dummyTime;

      if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         send_hs( stderr, "FedAmb::receiveInteraction():%d %c",
                  __LINE__, THLA_NEWLINE );
      }

      // Process the interaction.
      manager->receive_interaction( theInteraction,
                                    (ParameterHandleValueMap &)theParameterValues,
                                    theUserSuppliedTag,
                                    dummyTime.get(),
                                    false );
   }
}

void FedAmb::receiveInteraction(
   RTI1516_NAMESPACE::InteractionClassHandle         theInteraction,
   RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
   RTI1516_NAMESPACE::VariableLengthData const &     theUserSuppliedTag,
   RTI1516_NAMESPACE::OrderType                      sentOrder,
   RTI1516_NAMESPACE::TransportationType             theType,
   RTI1516_NAMESPACE::LogicalTime const &            theTime,
   RTI1516_NAMESPACE::OrderType                      receivedOrder,
   RTI1516_NAMESPACE::SupplementalReceiveInfo        theReceiveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( manager == NULL ) {
      send_hs( stderr, "FedAmb::receiveInteraction():%d NULL Manager!%c",
               __LINE__, THLA_NEWLINE );
   } else {
      // Process the interaction.
      if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         send_hs( stderr, "FedAmb::receiveInteraction():%d %c",
                  __LINE__, THLA_NEWLINE );
      }

      manager->receive_interaction( theInteraction,
                                    (ParameterHandleValueMap &)theParameterValues,
                                    theUserSuppliedTag,
                                    theTime,
                                    receivedOrder == RTI1516_NAMESPACE::TIMESTAMP );
   }
}

void FedAmb::receiveInteraction(
   RTI1516_NAMESPACE::InteractionClassHandle         theInteraction,
   RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
   RTI1516_NAMESPACE::VariableLengthData const &     theUserSuppliedTag,
   RTI1516_NAMESPACE::OrderType                      sentOrder,
   RTI1516_NAMESPACE::TransportationType             theType,
   RTI1516_NAMESPACE::LogicalTime const &            theTime,
   RTI1516_NAMESPACE::OrderType                      receivedOrder,
   RTI1516_NAMESPACE::MessageRetractionHandle        theHandle,
   RTI1516_NAMESPACE::SupplementalReceiveInfo        theReceiveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( manager == NULL ) {
      send_hs( stderr, "FedAmb::receiveInteraction():%d NULL Manager!%c",
               __LINE__, THLA_NEWLINE );
   } else {
      if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         send_hs( stderr, "FedAmb::receiveInteraction():%d %c",
                  __LINE__, THLA_NEWLINE );
      }

      // Process the interaction.
      manager->receive_interaction( theInteraction,
                                    (ParameterHandleValueMap &)theParameterValues,
                                    theUserSuppliedTag,
                                    theTime,
                                    receivedOrder == RTI1516_NAMESPACE::TIMESTAMP );
   }
}

void FedAmb::removeObjectInstance(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag,
   RTI1516_NAMESPACE::OrderType                 sentOrder,
   RTI1516_NAMESPACE::SupplementalRemoveInfo    theRemoveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string id_str;
      StringUtilities::to_string( id_str, theObject );

      if ( theUserSuppliedTag.size() > 0 ) {
         const char *tag = (const char *)theUserSuppliedTag.data();
         send_hs( stdout, "FedAmb::removeObjectInstance():%d User-Supplied-Tag='%s' Instance-ID:%s Valid-ID:%s %c",
                  __LINE__, tag, id_str.c_str(),
                  ( theObject.isValid() ? "Yes" : "No" ), THLA_NEWLINE );
      } else {
         send_hs( stdout, "FedAmb::removeObjectInstance():%d Instance-ID:%s Valid-ID:%s %c",
                  __LINE__, id_str.c_str(),
                  ( theObject.isValid() ? "Yes" : "No" ), THLA_NEWLINE );
      }
   }

   // Remove the instance ID for a federate, which this function will test for.
   federate->remove_MOM_HLAfederate_instance_id( theObject );

   // Mark this object as deleted from the RTI.
   manager->mark_object_as_deleted_from_federation( theObject );
}

void FedAmb::removeObjectInstance(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag,
   RTI1516_NAMESPACE::OrderType                 sentOrder,
   RTI1516_NAMESPACE::LogicalTime const &       theTime,
   RTI1516_NAMESPACE::OrderType                 receivedOrder,
   RTI1516_NAMESPACE::SupplementalRemoveInfo    theRemoveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   // Remove the instance ID for a federate, which this function will test for.
   federate->remove_MOM_HLAfederate_instance_id( theObject );

   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string id_str;
      StringUtilities::to_string( id_str, theObject );

      if ( theUserSuppliedTag.size() > 0 ) {
         const char *tag = (const char *)theUserSuppliedTag.data();
         send_hs( stdout, "FedAmb::removeObjectInstance():%d tag='%s' Instance ID:%s%c",
                  __LINE__, tag, id_str.c_str(), THLA_NEWLINE );
      } else {
         send_hs( stdout, "FedAmb::removeObjectInstance():%d Instance ID:%s%c",
                  __LINE__, id_str.c_str(), THLA_NEWLINE );
      }
   }

   // Mark this object as deleted from the RTI.
   manager->mark_object_as_deleted_from_federation( theObject );
}

void FedAmb::removeObjectInstance(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag,
   RTI1516_NAMESPACE::OrderType                 sentOrder,
   RTI1516_NAMESPACE::LogicalTime const &       theTime,
   RTI1516_NAMESPACE::OrderType                 receivedOrder,
   RTI1516_NAMESPACE::MessageRetractionHandle   theHandle,
   RTI1516_NAMESPACE::SupplementalRemoveInfo    theRemoveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   // Remove the instance ID for a federate, which this function will test for.
   federate->remove_MOM_HLAfederate_instance_id( theObject );

   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string id_str;
      StringUtilities::to_string( id_str, theObject );

      if ( theUserSuppliedTag.size() > 0 ) {
         const char *tag = (const char *)theUserSuppliedTag.data();
         send_hs( stdout, "FedAmb::removeObjectInstance():%d tag='%s' Instance ID:%s%c",
                  __LINE__, tag, id_str.c_str(), THLA_NEWLINE );
      } else {
         send_hs( stdout, "FedAmb::removeObjectInstance():%d Instance ID:%s%c",
                  __LINE__, id_str.c_str(), THLA_NEWLINE );
      }
   }

   // Mark this object as deleted from the RTI.
   manager->mark_object_as_deleted_from_federation( theObject );
}

void FedAmb::attributesInScope(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::attributesInScope():%d %c",
            federate->get_federate_name(),
            __LINE__, THLA_NEWLINE );
}

void FedAmb::attributesOutOfScope(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::attributesOutOfScope():%d %c",
            federate->get_federate_name(),
            __LINE__, THLA_NEWLINE );
}

void FedAmb::provideAttributeValueUpdate(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes,
   RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( manager != NULL ) {
      manager->provide_attribute_update( theObject,
                                         (AttributeHandleSet &)theAttributes );
   }
}

void FedAmb::turnUpdatesOnForObjectInstance(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::turnUpdatesOnForObjectInstance():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

void FedAmb::turnUpdatesOnForObjectInstance(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes,
   wstring const &                              updateRateDesignator ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::turnUpdatesOnForObjectInstance():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

void FedAmb::turnUpdatesOffForObjectInstance(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::turnUpdatesOffForObjectInstance():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

void FedAmb::confirmAttributeTransportationTypeChange(
   RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
   RTI1516_NAMESPACE::AttributeHandleSet   theAttributes,
   RTI1516_NAMESPACE::TransportationType   theTransportation ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::confirmAttributeTransportationTypeChange():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

void FedAmb::reportAttributeTransportationType(
   RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
   RTI1516_NAMESPACE::AttributeHandle      theAttribute,
   RTI1516_NAMESPACE::TransportationType   theTransportation ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::reportAttributeTransportationType():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

void FedAmb::confirmInteractionTransportationTypeChange(
   RTI1516_NAMESPACE::InteractionClassHandle theInteraction,
   RTI1516_NAMESPACE::TransportationType     theTransportation ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::confirmInteractionTransportationTypeChange():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

void FedAmb::reportInteractionTransportationType(
   RTI1516_NAMESPACE::FederateHandle         federateHandle,
   RTI1516_NAMESPACE::InteractionClassHandle theInteraction,
   RTI1516_NAMESPACE::TransportationType     theTransportation ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::reportInteractionTransportationType():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

///////////////////////////////////
// Ownership Management Services //
///////////////////////////////////

// 7.4
void FedAmb::requestAttributeOwnershipAssumption(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &offeredAttributes,
   RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   const char *tag = (const char *)theUserSuppliedTag.data();
   if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::requestAttributeOwnershipAssumption():%d push request received, tag='%s'%c",
               __LINE__, tag, THLA_NEWLINE );
   }

   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( theObject ) : NULL;

   if ( trickhla_obj != NULL ) {

      AttributeHandleSet::const_iterator iter;

      bool any_attribute_not_recognized = false;
      bool any_attribute_already_owned  = false;
      bool any_attribute_not_published  = false;

      // To make the state of the attribute push_requested flag thread safe lock
      // the mutex now. This allows us to handle multiple simultaneous requests
      // to push attributes to our federate.
      trickhla_obj->lock();

      // Mark which attributes we can accept ownership of.
      for ( iter = offeredAttributes.begin(); iter != offeredAttributes.end(); ++iter ) {

         // Get the attribute object for the given attribute handle.
         Attribute *trick_hla_attr = trickhla_obj->get_attribute( *iter );

         // We can accept ownership of the attribute if our object contains it
         // as an attribute, is remotely owned, and we are setup to publish it.
         if ( ( trick_hla_attr != NULL ) && trick_hla_attr->is_remotely_owned() && trick_hla_attr->is_publish() ) {

            trick_hla_attr->set_push_requested( true );

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stdout, "FedAmb::requestAttributeOwnershipAssumption():%d\
\n   Attribute '%s'->'%s' of object '%s'.%c",
                        __LINE__,
                        trickhla_obj->get_FOM_name(),
                        trick_hla_attr->get_FOM_name(),
                        trickhla_obj->get_name(), THLA_NEWLINE );
            }
         } else if ( trick_hla_attr == NULL ) {

            // Handle the case where the attribute is not recognized.
            any_attribute_not_recognized = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stderr, "FedAmb::requestAttributeOwnershipAssumption():%d \
Attribute Not Recognized ERROR: Object '%s' with FOM name '%s'!%c",
                        __LINE__,
                        trickhla_obj->get_name(), trickhla_obj->get_FOM_name(),
                        THLA_NEWLINE );
            }
         } else if ( trick_hla_attr->is_locally_owned() ) {

            // Handle the case where the attribute is already owned.
            any_attribute_already_owned = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stderr, "FedAmb::requestAttributeOwnershipAssumption():%d \
Attribute Already Owned ERROR: Object '%s' with attribute '%s'->'%s'!%c",
                        __LINE__,
                        trickhla_obj->get_name(),
                        trickhla_obj->get_FOM_name(),
                        trick_hla_attr->get_FOM_name(), THLA_NEWLINE );
            }
         } else if ( !trick_hla_attr->is_publish() ) {

            // Handle the case where the attribute is not published.
            any_attribute_not_published = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stderr, "FedAmb::requestAttributeOwnershipAssumption():%d \
Attribute Not Published ERROR: Object '%s' with attribute '%s'->'%s'!%c",
                        __LINE__,
                        trickhla_obj->get_name(),
                        trickhla_obj->get_FOM_name(),
                        trick_hla_attr->get_FOM_name(), THLA_NEWLINE );
            }
         }
      }

      // Make sure to unlock the mutex.
      trickhla_obj->unlock();

      // Start the thread to service the grant of the push request.
      trickhla_obj->grant_push_request_pthread();

      // Now throw an exceptions for any detected error conditions.
      if ( any_attribute_not_recognized ) {
         throw FederateInternalError( L"FedAmb::requestAttributeOwnershipAssumption() \
Unknown Attribute for Object." );
      }
      if ( any_attribute_already_owned ) {
         throw FederateInternalError( L"FedAmb::requestAttributeOwnershipAssumption() \
Attribute for Object already owned." );
      }
      if ( any_attribute_not_published ) {
         throw FederateInternalError( L"FedAmb::requestAttributeOwnershipAssumption() \
Attribute for Object is not published." );
      }

   } else {
      string id_str;
      StringUtilities::to_string( id_str, theObject );
      send_hs( stdout, "FedAmb::requestAttributeOwnershipAssumption():%d \
Unknown object instance (ID:%s), push request rejected, tag='%s' %c",
               __LINE__, id_str.c_str(), tag, THLA_NEWLINE );

      throw FederateInternalError( L"FedAmb::requestAttributeOwnershipAssumption() Unknown object instance" );
   }
}

// 7.5
void FedAmb::requestDivestitureConfirmation(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &releasedAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( theObject ) : NULL;

   if ( trickhla_obj == NULL ) {
      string id_str;
      StringUtilities::to_string( id_str, theObject );
      send_hs( stdout, "FedAmb::requestDivestitureConfirmation():%d Unknown \
object instance (ID:%s), push request rejected.%c",
               __LINE__, id_str.c_str(),
               THLA_NEWLINE );

      throw FederateInternalError( L"FedAmb::requestDivestitureConfirmation() Unknown object instance." );
   }

   AttributeHandleSet::const_iterator iter;
   bool                               any_devist_requested         = false;
   bool                               any_attribute_not_recognized = false;
   bool                               any_attribute_not_owned      = false;

   // Mark which attributes we need mark for divestiture of ownership.
   for ( iter = releasedAttributes.begin(); iter != releasedAttributes.end(); ++iter ) {

      // Get the attribute object for the given attribute handle.
      Attribute *trick_hla_attr = trickhla_obj->get_attribute( *iter );

      // We want to divest ownership of the attribute if our object
      // contains it and it is locally owned.
      if ( ( trick_hla_attr != NULL ) && trick_hla_attr->is_locally_owned() ) {

         // Divest ownership of this attribute.
         trick_hla_attr->set_divest_requested( true );

         any_devist_requested = true;

         if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            send_hs( stdout, "FedAmb::requestDivestitureConfirmation():%d\
\n   Attribute '%s'->'%s' of object '%s'.%c",
                     __LINE__,
                     trickhla_obj->get_FOM_name(),
                     trick_hla_attr->get_FOM_name(),
                     trickhla_obj->get_name(), THLA_NEWLINE );
         }
      } else if ( trick_hla_attr == NULL ) {

         // Handle the case where the attribute is not recognized.
         any_attribute_not_recognized = true;

         if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            send_hs( stderr, "FedAmb::requestDivestitureConfirmation():%d \
Attribute Not Recognized ERROR: Object '%s' with FOM name '%s'!%c",
                     __LINE__,
                     trickhla_obj->get_name(), trickhla_obj->get_FOM_name(),
                     THLA_NEWLINE );
         }
      } else if ( trick_hla_attr->is_remotely_owned() ) {

         // Handle the case where the attribute is not owned.
         any_attribute_not_owned = true;

         if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            send_hs( stderr, "FedAmb::requestDivestitureConfirmation():%d \
Attribute Not Owned ERROR: Object '%s' with attribute '%s'->'%s'!%c",
                     __LINE__,
                     trickhla_obj->get_name(),
                     trickhla_obj->get_FOM_name(),
                     trick_hla_attr->get_FOM_name(), THLA_NEWLINE );
         }
      }
   }

   // Mark the divest request for the object last, since the trick thread is
   // keying off this value and we don't want a race condition for the
   // state of the attribute flags.
   if ( any_devist_requested ) {
      // Also indicate a divest request at the object level.
      trickhla_obj->set_divest_requested( true );
   }

   // Now throw an exceptions for any detected error conditions.
   if ( any_attribute_not_recognized ) {
      throw FederateInternalError( L"FedAmb::requestDivestitureConfirmation() \
Unknown Attribute for Object." );
   }
   if ( any_attribute_not_owned ) {
      throw FederateInternalError( L"FedAmb::requestDivestitureConfirmation() \
Attribute for Object Not Owned." );
   }
}

// 7.7
void FedAmb::attributeOwnershipAcquisitionNotification(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &securedAttributes,
   RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::attributeOwnershipAcquisitionNotification():%d %c",
               __LINE__, THLA_NEWLINE );
   }

   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( theObject ) : NULL;

   if ( trickhla_obj != NULL ) {
      AttributeHandleSet::const_iterator iter;
      bool                               any_attribute_acquired       = false;
      bool                               any_attribute_not_recognized = false;
      bool                               any_attribute_already_owned  = false;
      bool                               any_attribute_not_published  = false;

      // Mark which attributes we now locally own.
      for ( iter = securedAttributes.begin(); iter != securedAttributes.end(); ++iter ) {

         // Get the attribute for the given attribute handle.
         Attribute *trick_hla_attr = trickhla_obj->get_attribute( *iter );

         // Mark the attribute as locally owned if the object has it as an
         // attribute and is remotely owned, and we are setup to publish it.
         if ( ( trick_hla_attr != NULL ) && trick_hla_attr->is_remotely_owned() && trick_hla_attr->is_publish() ) {

            trick_hla_attr->mark_locally_owned();
            any_attribute_acquired = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stdout, "FedAmb::attributeOwnershipAcquisitionNotification():%d\
\n   ACQUIRED ownership of attribute '%s'->'%s' of object '%s'.%c",
                        __LINE__,
                        trickhla_obj->get_FOM_name(),
                        trick_hla_attr->get_FOM_name(),
                        trickhla_obj->get_name(), THLA_NEWLINE );
            }
         } else if ( trick_hla_attr == NULL ) {

            // Handle the case where the attribute is not recognized.
            any_attribute_not_recognized = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stderr, "FedAmb::attributeOwnershipAcquisitionNotification():%d \
Attribute Not Recognized ERROR: Object '%s' with FOM name '%s'!%c",
                        __LINE__,
                        trickhla_obj->get_name(), trickhla_obj->get_FOM_name(),
                        THLA_NEWLINE );
            }
         } else if ( trick_hla_attr->is_locally_owned() ) {

            // Handle the case where the attribute is already owned.
            any_attribute_already_owned = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stderr, "FedAmb::attributeOwnershipAcquisitionNotification():%d \
Attribute Already Owned ERROR: Object '%s' with attribute '%s'->'%s'!%c",
                        __LINE__,
                        trickhla_obj->get_name(),
                        trickhla_obj->get_FOM_name(),
                        trick_hla_attr->get_FOM_name(), THLA_NEWLINE );
            }
         } else if ( !trick_hla_attr->is_publish() ) {

            // Handle the case where the attribute is not published.
            any_attribute_not_published = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stderr, "FedAmb::attributeOwnershipAcquisitionNotification():%d \
Attribute Not Published ERROR: Object '%s' with attribute '%s'->'%s'!%c",
                        __LINE__,
                        trickhla_obj->get_name(),
                        trickhla_obj->get_FOM_name(),
                        trick_hla_attr->get_FOM_name(), THLA_NEWLINE );
            }
         }
      }

      // If we are doing blocking I/O we need to let the object know that it
      // now owns at least one attribute.
      if ( any_attribute_acquired ) {
         trickhla_obj->notify_attribute_ownership_changed();
      }

      // Now throw an exceptions for any detected error conditions.
      if ( any_attribute_not_recognized ) {
         throw FederateInternalError( L"FedAmb::attributeOwnershipAcquisitionNotification() \
Unknown Attribute for Object." );
      }
      if ( any_attribute_already_owned ) {
         throw FederateInternalError( L"FedAmb::attributeOwnershipAcquisitionNotification() \
Attribute for Object already owned." );
      }
      if ( any_attribute_not_published ) {
         throw FederateInternalError( L"FedAmb::attributeOwnershipAcquisitionNotification() \
Attribute for Object is not published." );
      }

   } else {
      send_hs( stderr, "FedAmb::attributeOwnershipAcquisitionNotification():%d \
Ownership acquisition rejected (object not found)%c",
               __LINE__, THLA_NEWLINE );

      throw FederateInternalError( L"FedAmb::attributeOwnershipAcquisitionNotification() Unknown object instance" );
   }
}

// 7.10
void FedAmb::attributeOwnershipUnavailable(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &releasedAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::attributeOwnershipUnavailable():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

// 7.11
void FedAmb::requestAttributeOwnershipRelease(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &candidateAttributes,
   RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   const char *tag = (const char *)theUserSuppliedTag.data();
   if ( should_print( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::requestAttributeOwnershipRelease():%d pull request received, tag='%s'%c",
               __LINE__, tag, THLA_NEWLINE );
   }
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( theObject ) : NULL;

   if ( trickhla_obj != NULL ) {

      AttributeHandleSet::const_iterator iter;
      bool                               any_pull_requested           = false;
      bool                               any_attribute_not_recognized = false;
      bool                               any_attribute_not_owned      = false;

      // Mark which attributes we now locally own.
      for ( iter = candidateAttributes.begin(); iter != candidateAttributes.end(); ++iter ) {

         // Get the attribute for the given attribute handle.
         Attribute *trick_hla_attr = trickhla_obj->get_attribute( *iter );

         // Set the attribute for a pull request if the object contains the
         // specified attribute and is locally owned.
         if ( ( trick_hla_attr != NULL ) && trick_hla_attr->is_locally_owned() ) {

            // Mark the attribute for the pull request.
            trick_hla_attr->set_pull_requested( true );

            any_pull_requested = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stdout, "FedAmb::requestAttributeOwnershipRelease():%d\
\n   Attribute '%s'->'%s' of object '%s'.%c",
                        __LINE__,
                        trickhla_obj->get_FOM_name(),
                        trick_hla_attr->get_FOM_name(),
                        trickhla_obj->get_name(), THLA_NEWLINE );
            }
         } else if ( trick_hla_attr == NULL ) {

            // Handle the case where the attribute is not recognized.
            any_attribute_not_recognized = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stderr, "FedAmb::requestAttributeOwnershipRelease():%d \
Attribute Not Recognized ERROR: Object '%s' with FOM name '%s'!%c",
                        __LINE__,
                        trickhla_obj->get_name(), trickhla_obj->get_FOM_name(),
                        THLA_NEWLINE );
            }
         } else if ( trick_hla_attr->is_remotely_owned() ) {

            // Handle the case where the attribute is not owned.
            any_attribute_not_owned = true;

            if ( should_print( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               send_hs( stderr, "FedAmb::requestAttributeOwnershipRelease():%d \
Attribute Not Owned ERROR: Object '%s' with attribute '%s'->'%s'!%c",
                        __LINE__,
                        trickhla_obj->get_name(),
                        trickhla_obj->get_FOM_name(),
                        trick_hla_attr->get_FOM_name(), THLA_NEWLINE );
            }
         }
      }

      // Mark the pull request for the object last since the trick thread is
      // keying off this value and we don't want a race condition for the
      // state of the attribute flags.
      if ( any_pull_requested ) {
         trickhla_obj->set_pull_requested( true );
      }

      // Now throw an exceptions for any detected error conditions.
      if ( any_attribute_not_recognized ) {
         throw FederateInternalError( L"FedAmb::requestAttributeOwnershipRelease() \
Unknown Attribute for Object." );
      }
      if ( any_attribute_not_owned ) {
         throw FederateInternalError( L"FedAmb::requestAttributeOwnershipRelease() \
Attribute for Object Not Owned." );
      }

   } else {
      send_hs( stderr, "FedAmb::requestAttributeOwnershipRelease():%d pull rejected (not found), tag='%s'%c",
               __LINE__, tag, THLA_NEWLINE );
      throw FederateInternalError( L"FedAmb::requestAttributeOwnershipRelease() Unknown object instance" );
   }
}

// 7.15
void FedAmb::confirmAttributeOwnershipAcquisitionCancellation(
   RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
   RTI1516_NAMESPACE::AttributeHandleSet const &releasedAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::confirmAttributeOwnershipAcquisitionCancellation():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

// 7.17
void FedAmb::informAttributeOwnership(
   RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
   RTI1516_NAMESPACE::AttributeHandle      theAttribute,
   RTI1516_NAMESPACE::FederateHandle       theOwner ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::informAttributeOwnership():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

// 7.17
void FedAmb::attributeIsNotOwned(
   RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
   RTI1516_NAMESPACE::AttributeHandle      theAttribute ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::attributeIsNotOwned():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

// 7.17
void FedAmb::attributeIsOwnedByRTI(
   RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
   RTI1516_NAMESPACE::AttributeHandle      theAttribute ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::attributeIsOwnedByRTI():%d %c",
            federate->get_federate_name(), __LINE__, THLA_NEWLINE );
}

//////////////////////////////
// Time Management Services //
//////////////////////////////

void FedAmb::timeRegulationEnabled(
   RTI1516_NAMESPACE::LogicalTime const &theFederateTime ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   // Set the appropriate time.
   federate->set_granted_time( theFederateTime );
   federate->set_requested_time( theFederateTime );

   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::timeRegulationEnabled():%d Federate \
\"%s\" Time granted to: %.12G %c",
               __LINE__, federate->get_federate_name(),
               federate->get_granted_time(), THLA_NEWLINE );
   }

   // Set the control flags after the print above to avoid a race condition with
   // the main Trick thread printing to the console when these flags are set.
   federate->set_time_advance_grant( true );
   federate->set_time_regulation_state( true );
}

void FedAmb::timeConstrainedEnabled(
   RTI1516_NAMESPACE::LogicalTime const &theFederateTime ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   // Set the appropriate time.
   federate->set_granted_time( theFederateTime );
   federate->set_requested_time( theFederateTime );

   if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      send_hs( stdout, "FedAmb::timeConstrainedEnabled():%d Federate \
\"%s\" Time granted to: %.12G %c",
               __LINE__, federate->get_federate_name(),
               federate->get_granted_time(), THLA_NEWLINE );
   }

   // Set the control flags after the print above to avoid a race condition with
   // the main Trick thread printing to the console when these flags are set.
   federate->set_time_advance_grant( true );
   federate->set_time_constrained_state( true );
}

void FedAmb::timeAdvanceGrant(
   RTI1516_NAMESPACE::LogicalTime const &theTime ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   Int64Time *int64Time = new Int64Time( theTime );

   // Ignore any granted time less than the requested time otherwise it will
   // break our concept of HLA time since we are using scheduled jobs for
   // processing HLA data sends, receives, etc and expected the next granted
   // time to match our requested time. Dan Dexter, 2/12/2007
   if ( *int64Time >= federate->get_requested_fed_time() ) {

      // Set the appropriate time control flags.
      federate->set_granted_time( theTime );
      federate->set_time_advance_grant( true );
   } else {
      if ( should_print( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         send_hs( stdout, "FedAmb::timeAdvanceGrant():%d\nFederate \"%s\" \
IGNORING GRANTED TIME %.12G because it is less then requested time %.12G %c",
                  __LINE__, federate->get_federate_name(),
                  int64Time->get_double_time(),
                  federate->get_requested_time(), THLA_NEWLINE );
      }
   }

   delete int64Time;
}

void FedAmb::requestRetraction(
   RTI1516_NAMESPACE::MessageRetractionHandle theHandle ) throw( RTI1516_NAMESPACE::FederateInternalError )
{
   send_hs( stderr, "This federate '%s' does not support this function: \
FedAmb::requestRetraction():%d %c",
            federate->get_federate_name(),
            __LINE__, THLA_NEWLINE );
}
