/*!
@file TrickHLA/FedAmb.cpp
@ingroup TrickHLA
@brief Provides methods called by the RTI Ambassador for simulation object,
interaction and time management.

@details Methods of objects of this class are not intended to be called from
the Trick S_define level. However, this class is essentially a polymorphic
callback class provided to the RTI Ambassador.

\par<b>Assumptions and Limitations:</b>
- Derived class of abstract FederateAmbassador class to implement methods so
that RTI can call functions in the federate.
- Based on HelloWorld example code.
- None of the methods in this class are intended to be called from the Trick
S_define level. However, an instance of this class can be declared in the
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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{FedAmb.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Manager.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{DMSO Programmer, DMSO, HLA, Mar 1998, --, HelloWorld Federate Ambassador.}
@rev_entry{Edwin Z. Crues, Titan Systems Corp., DIS, Feb 2002, --, HLA Ball Sim.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2025, --, Add HLA 4 support}
@revs_end

*/

// System includes.
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Manager.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"

#if defined( IEEE_1516_2025 )
#   include "TrickHLA/FedAmbHLA4.hh"
#else
// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"

#   include "TrickHLA/FedAmbHLA3.hh"
#endif // IEEE_1516_2025

// HLA include files.
#include "RTI/Enums.h"
#include "RTI/Exception.h"
#include "RTI/FederateAmbassador.h"
#include "RTI/Handle.h"
#include "RTI/Typedefs.h"
#include "RTI/VariableLengthData.h"
#if defined( IEEE_1516_2025 )
#   include "RTI/time/LogicalTime.h"
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details In most cases, we would allocate and set default names in the
 * constructor. However, since we want this class to be Input
 * Processor friendly, we cannot do that here since the Input
 * Processor may not have been initialized yet. So, we have to
 * set the name information to NULL and then allocate and set the
 * defaults in the initialization job if not already set in the
 * input stream.
 * @job_class{initialization}
 */
FedAmb::FedAmb()
   : FederateAmbassador(),
     federate( NULL ),
     manager( NULL ),
     federation_restore_status_response_context_switch( false ), // process, not echo.
     federation_restored_rebuild_federate_handle_set( false )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
FedAmb::~FedAmb()
{
   return;
}

/*!
 * @job_class{initialization}
 */
void FedAmb::setup(
   Federate &federate,
   Manager  &manager )
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
             << " ERROR: Unexpected NULL TrickHLA::Federate.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Check to make sure we have a reference to the TrickHLA::Manager.
   if ( manager == NULL ) {
      ostringstream errmsg;
      errmsg << "FedAmb::initialize():" << __LINE__
             << " ERROR: Unexpected NULL TrickHLA::Manager.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::initialize():%d Federate:\"%s\"\n",
                       __LINE__, federate->get_federate_name().c_str() );
   }

   if ( federate->get_federate_name().empty() ) {
      ostringstream errmsg;
      errmsg << "FedAmb::initialize():" << __LINE__
             << " ERROR: Unexpected empty federate name.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   TRICKHLA_VALIDATE_FPU_CONTROL_WORD;
}

////////////////////////////////////
// Federation Management Services //
////////////////////////////////////

void FedAmb::connectionLost(
   wstring const &faultDescription )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   string fault_msg;
   StringUtilities::to_string( fault_msg, faultDescription );
   ostringstream errmsg;
   errmsg << "FedAmb::connectionLost():" << __LINE__
          << " ERROR: Lost the connection to the Central RTI Component (CRC)."
          << " Reason:'" << fault_msg << "'."
          << " Terminating the simulation!\n";
   DebugHandler::terminate_with_message( errmsg.str() );
}

void FedAmb::reportFederationExecutions(
   FederationExecutionInformationVector const &report )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::reportFederationExecutions():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

#if defined( IEEE_1516_2025 )
void FedAmb::reportFederationExecutionMembers(
   std::wstring const                               &federationName,
   FederationExecutionMemberInformationVector const &report )
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::reportFederationExecutionMembers():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::reportFederationExecutionDoesNotExist(
   std::wstring const &federationName )
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::reportFederationExecutionDoesNotExist():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::federateResigned(
   std::wstring const &reasonForResignDescription )
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::federateResigned():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}
#endif // IEEE_1516_2025

void FedAmb::synchronizationPointRegistrationSucceeded(
   wstring const &label )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      message_publish( MSG_NORMAL, "FedAmb::synchronizationPointRegistrationSucceeded():%d Label:'%s'\n",
                       __LINE__, label_str.c_str() );
   }

   federate->sync_point_registration_succeeded( label );
}

void FedAmb::synchronizationPointRegistrationFailed(
   wstring const                    &label,
   SynchronizationPointFailureReason reason )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      message_publish( MSG_NORMAL, "FedAmb::synchronizationPointRegistrationFailed():%d Label:'%s'\n",
                       __LINE__, label_str.c_str() );
   }
   federate->sync_point_registration_failed( label, reason );
}

void FedAmb::announceSynchronizationPoint(
   wstring const            &label,
   VariableLengthData const &userSuppliedTag )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      message_publish( MSG_NORMAL, "FedAmb::announceSynchronizationPoint():%d Label:'%s'\n",
                       __LINE__, label_str.c_str() );
   }
   federate->announce_sync_point( label, userSuppliedTag );
}

void FedAmb::federationSynchronized(
   wstring const           &label,
   FederateHandleSet const &failedToSyncSet )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string label_str;
      StringUtilities::to_string( label_str, label );
      message_publish( MSG_NORMAL, "FedAmb::federationSynchronized():%d Label:'%s'\n",
                       __LINE__, label_str.c_str() );
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
      string label_str;
      StringUtilities::to_string( label_str, label );
      message_publish( MSG_WARNING, "FedAmb::federationSynchronized():%d ERROR: These \
federate handles failed to synchronize on sync-point '%s': %s\n",
                       __LINE__, label_str.c_str(), strIds.c_str() );
   }
}

void FedAmb::initiateFederateSave(
   wstring const &label )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::initiateFederateSave():%d \n",
                       __LINE__ );
   }
   federate->set_save_name( label );
   federate->set_start_to_save( true );
}

void FedAmb::initiateFederateSave(
   wstring const     &label,
   LogicalTime const &time )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      Int64Time i64time;
      i64time.set( time );
      message_publish( MSG_NORMAL, "FedAmb::initiateFederateSave():%d HLA-time:%.12G seconds.\n",
                       __LINE__, i64time.get_time_in_seconds() );
   }
   federate->set_save_name( label );
   federate->set_start_to_save( true );
}

void FedAmb::federationSaved()
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::federationSaved():%d \n",
                       __LINE__ );
   }
   federate->set_start_to_save( false );
   federate->set_save_completed();
   federate->federation_saved();
}

void FedAmb::federationNotSaved(
   SaveFailureReason reason )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::federationNotSaved():%d \n",
                       __LINE__ );
   }

   federate->print_save_failure_reason( reason );

   // TODO: Do we need to the steps below to exit freeze mode?
   federate->set_start_to_save( false );
   federate->set_save_completed();
   federate->federation_saved();
}

void FedAmb::federationSaveStatusResponse(
   FederateHandleSaveStatusPairVector const &response )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::federationSaveStatusResponse():%d \n",
                       __LINE__ );
   }
   federate->process_requested_federation_save_status( response );
}

void FedAmb::requestFederationRestoreSucceeded(
   wstring const &label )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::requestFederationRestoreSucceeded():%d \n",
                       __LINE__ );
   }
   federate->set_restore_request_succeeded();
   federate->requested_federation_restore_status( true );
}

void FedAmb::requestFederationRestoreFailed(
   wstring const &label )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::requestFederationRestoreFailed():%d \n",
                       __LINE__ );
   }
   federate->set_restore_request_failed();
   federate->requested_federation_restore_status( false );
}

void FedAmb::federationRestoreBegun()
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::federationRestoreBegun():%d \n",
                       __LINE__ );
   }
   federate->set_restore_begun();
}

void FedAmb::initiateFederateRestore(
#if defined( IEEE_1516_2025 )
   wstring const        &label,
   wstring const        &federateName,
   FederateHandle const &postRestoreFederateHandle )
#else
   wstring const &label,
   wstring const &federateName,
   FederateHandle postRestoreFederateHandle ) throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string name;
      StringUtilities::to_string( name, federateName );
      message_publish( MSG_NORMAL, "FedAmb::initiateFederateRestore():%d for federate '%s'\n",
                       __LINE__, name.c_str() );
   }
   federate->set_start_to_restore( true );
   federate->set_restore_name( label );
}

void FedAmb::federationRestored()
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::federationRestored():%d \n",
                       __LINE__ );
   }
   federate->set_restore_completed();
}

void FedAmb::federationNotRestored(
   RestoreFailureReason reason )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::federationNotRestored():%d \n",
                       __LINE__ );
   }
   federate->set_restore_failed();
   federate->print_restore_failure_reason( reason );
}

void FedAmb::federationRestoreStatusResponse(
   FederateRestoreStatusVector const &response )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::federationRestoreStatusResponse():%d \n",
                       __LINE__ );
   }
   if ( !this->federation_restore_status_response_context_switch ) {
      // process
      federate->process_requested_federation_restore_status( response );
   } else {
      // echo
      federate->print_requested_federation_restore_status( response );
   }
}

/////////////////////////////////////
// Declaration Management Services //
/////////////////////////////////////

void FedAmb::startRegistrationForObjectClass(
#if defined( IEEE_1516_2025 )
   ObjectClassHandle const &objectClass )
#else
   ObjectClassHandle objectClass ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::startRegistrationForObjectClass():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::stopRegistrationForObjectClass(
#if defined( IEEE_1516_2025 )
   ObjectClassHandle const &objectClass )
#else
   ObjectClassHandle objectClass ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::stopRegistrationForObjectClass():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::turnInteractionsOn(
#if defined( IEEE_1516_2025 )
   InteractionClassHandle const &interactionClass )
#else
   InteractionClassHandle interactionClass ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::turnInteractionsOn():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::turnInteractionsOff(
#if defined( IEEE_1516_2025 )
   InteractionClassHandle const &interactionClass )
#else
   InteractionClassHandle interactionClass ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::turnInteractionsOff():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

////////////////////////////////
// Object Management Services //
////////////////////////////////

// 6.3
void FedAmb::objectInstanceNameReservationSucceeded(
   wstring const &objectInstanceName )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( manager != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string instance_name;
         StringUtilities::to_string( instance_name, objectInstanceName );
         message_publish( MSG_NORMAL, "FedAmb::objectInstanceNameReservationSucceeded():%d '%s'\n",
                          __LINE__, instance_name.c_str() );
      }

      manager->object_instance_name_reservation_succeeded( objectInstanceName );
   }
}

// 6.3
void FedAmb::objectInstanceNameReservationFailed(
   wstring const &objectInstanceName )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( manager != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string instance_name;
         StringUtilities::to_string( instance_name, objectInstanceName );
         message_publish( MSG_NORMAL, "FedAmb::objectInstanceNameReservationFailed():%d FAILED '%s'\n",
                          __LINE__, instance_name.c_str() );
      }

      manager->object_instance_name_reservation_failed( objectInstanceName );
   }
}

// 6.6
void FedAmb::multipleObjectInstanceNameReservationSucceeded(
   set< wstring > const &objectInstanceNames )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( manager != NULL ) {

      set< wstring >::const_iterator iter;
      for ( iter = objectInstanceNames.begin();
            iter != objectInstanceNames.end(); ++iter ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            string instance_name;
            StringUtilities::to_string( instance_name, *iter );
            message_publish( MSG_NORMAL, "FedAmb::objectInstanceNameReservationSucceeded():%d '%s'\n",
                             __LINE__, instance_name.c_str() );
         }

         manager->object_instance_name_reservation_succeeded( *iter );
      }
   }
}

void FedAmb::multipleObjectInstanceNameReservationFailed(
   set< wstring > const &objectInstanceNames )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( manager != NULL ) {

      set< wstring >::const_iterator iter;
      for ( iter = objectInstanceNames.begin();
            iter != objectInstanceNames.end(); ++iter ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            string instance_name;
            StringUtilities::to_string( instance_name, *iter );
            message_publish( MSG_NORMAL, "FedAmb::objectInstanceNameReservationFailed():%d FAILED '%s'\n",
                             __LINE__, instance_name.c_str() );
         }

         manager->object_instance_name_reservation_failed( *iter );
      }
   }
}

#if defined( IEEE_1516_2010 )
void FedAmb::discoverObjectInstance(
   ObjectInstanceHandle objectInstance,
   ObjectClassHandle    objectClass,
   wstring const       &objectInstanceName ) throw( FederateInternalError )
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string id_str, name_str;
      StringUtilities::to_string( id_str, objectInstance );
      StringUtilities::to_string( name_str, objectInstanceName );
      message_publish( MSG_NORMAL, "FedAmb::discoverObjectInstance():%d DISCOVERED '%s' Instance-ID:%s\n",
                       __LINE__, name_str.c_str(), id_str.c_str() );
   }

   if ( manager == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, objectInstance );
         StringUtilities::to_string( name_str, objectInstanceName );
         message_publish( MSG_NORMAL, "FedAmb::discoverObjectInstance():%d Unexpected \
NULL Manager! Can't do anything with discovered object '%s' Instance-ID:%s\n",
                          __LINE__, name_str.c_str(), id_str.c_str() );
      }
   } else if ( !manager->discover_object_instance( objectInstance, objectClass, objectInstanceName ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, objectInstance );
         StringUtilities::to_string( name_str, objectInstanceName );
         message_publish( MSG_WARNING, "FedAmb::discoverObjectInstance():%d Object '%s' with Instance-ID:%s is UNKNOWN to me!\n",
                          __LINE__, name_str.c_str(), id_str.c_str() );
      }
   }
}
#endif // IEEE_1516_2010

void FedAmb::discoverObjectInstance(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   ObjectClassHandle const    &objectClass,
   wstring const              &objectInstanceName,
   FederateHandle const       &producingFederate )
#else
   ObjectInstanceHandle objectInstance,
   ObjectClassHandle    objectClass,
   wstring const       &objectInstanceName,
   FederateHandle       producingFederate ) throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string fed_id;
      StringUtilities::to_string( fed_id, producingFederate );
      message_publish( MSG_NORMAL, "FedAmb::discoverObjectInstance(producing \
federate '%s'):%d calling 'discoverObjectInstance' to finish the discovery.\n",
                       fed_id.c_str(), __LINE__ );
   }

   if ( manager == NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, objectInstance );
         StringUtilities::to_string( name_str, objectInstanceName );
         message_publish( MSG_NORMAL, "FedAmb::discoverObjectInstance():%d Unexpected \
NULL Manager! Can't do anything with discovered object '%s' Instance-ID:%s\n",
                          __LINE__, name_str.c_str(), id_str.c_str() );
      }
   } else if ( !manager->discover_object_instance( objectInstance, objectClass, objectInstanceName ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str, name_str;
         StringUtilities::to_string( id_str, objectInstance );
         StringUtilities::to_string( name_str, objectInstanceName );
         message_publish( MSG_WARNING, "FedAmb::discoverObjectInstance():%d Object '%s' with Instance-ID:%s is UNKNOWN to me!\n",
                          __LINE__, name_str.c_str(), id_str.c_str() );
      }
   }
}

void FedAmb::reflectAttributeValues(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const     &objectInstance,
   AttributeHandleValueMap const  &attributeValues,
   VariableLengthData const       &userSuppliedTag,
   TransportationTypeHandle const &transportationType,
   FederateHandle const           &producingFederate,
   RegionHandleSet const          *optionalSentRegions )
#else
   ObjectInstanceHandle           objectInstance,
   AttributeHandleValueMap const &attributeValues,
   VariableLengthData const      &userSuppliedTag,
   OrderType                      sentOrderType,
   TransportationType             transportationType,
   SupplementalReflectInfo        reflectInfo ) throw( FederateInternalError )
#endif
{
   // Get the TrickHLA object for the given Object Instance Handle.
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( objectInstance ) : NULL;

   if ( trickhla_obj != NULL ) {
      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         message_publish( MSG_NORMAL, "FedAmb:reflectAttributeValues():%d '%s'\n",
                          __LINE__, trickhla_obj->get_name().c_str() );
      }

      trickhla_obj->enqueue_data( const_cast< AttributeHandleValueMap & >( attributeValues ) );

#ifdef THLA_CHECK_SEND_AND_RECEIVE_COUNTS
      ++trickhla_obj->receive_count;
#endif
   } else if ( ( federate != NULL ) && federate->is_federate_instance_id( objectInstance ) ) {

      if ( federation_restored_rebuild_federate_handle_set ) {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            string id_str;
            StringUtilities::to_string( id_str, objectInstance );
            message_publish( MSG_NORMAL, "FedAmb::reflectAttributeValues(%d elements):%d Rebuilding federate handle for Federate ID:%s\n",
                             (int)attributeValues.size(),
                             __LINE__, id_str.c_str() );
         }
         federate->rebuild_federate_handles( objectInstance, attributeValues );
      } else {
         if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            string id_str;
            StringUtilities::to_string( id_str, objectInstance );
            message_publish( MSG_NORMAL, "FedAmb::reflectAttributeValues():%d Setting name for Federate ID:%s\n",
                             __LINE__, id_str.c_str() );
         }
         federate->set_MOM_HLAfederate_instance_attributes( objectInstance, attributeValues );
      }
   } else if ( ( federate != NULL ) && federate->is_MOM_HLAfederation_instance_id( objectInstance ) ) {
      // This was an instance-ID for the Federation and not a federate.
      if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str;
         StringUtilities::to_string( id_str, objectInstance );
         message_publish( MSG_NORMAL, "FedAmb::reflectAttributeValues():%d Setting name for Federation ID:%s\n",
                          __LINE__, id_str.c_str() );
      }
      federate->set_MOM_HLAfederation_instance_attributes( objectInstance, attributeValues );
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string handle_str;
         StringUtilities::to_string( handle_str, objectInstance );

         ostringstream summary;
         summary << "FedAmb::reflectAttributeValues():" << __LINE__
                 << " Received update to Unknown Object Instance:"
                 << handle_str << '\n';

         AttributeHandleValueMap::const_iterator attr_iter;
         for ( attr_iter = attributeValues.begin();
               attr_iter != attributeValues.end();
               ++attr_iter ) {
            StringUtilities::to_string( handle_str, attr_iter->first );
            summary << "   + Attribute-Handle:" << handle_str << '\n';
         }
         message_publish( MSG_NORMAL, summary.str().c_str() );
      }
   }
}

#if defined( IEEE_1516_2010 )
void FedAmb::reflectAttributeValues(
   ObjectInstanceHandle           objectInstance,
   AttributeHandleValueMap const &attributeValues,
   VariableLengthData const      &userSuppliedTag,
   OrderType                      sentOrderType,
   TransportationType             transportationType,
   LogicalTime const             &time,
   OrderType                      receivedOrderType,
   SupplementalReflectInfo        reflectInfo ) throw( FederateInternalError )
{
   // Get the TrickHLA object for the given Object Instance Handle.
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( objectInstance ) : NULL;

   if ( trickhla_obj != NULL ) {

      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         Int64Time i64time;
         i64time.set( time );
         message_publish( MSG_NORMAL, "FedAmb:reflectAttributeValues():%d '%s' HLA-time:%.12G seconds.\n",
                          __LINE__, trickhla_obj->get_name().c_str(), i64time.get_time_in_seconds() );
      }

      trickhla_obj->enqueue_data( const_cast< AttributeHandleValueMap & >( attributeValues ) );

#   ifdef THLA_CHECK_SEND_AND_RECEIVE_COUNTS
      ++trickhla_obj->receive_count;
#   endif
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str;
         StringUtilities::to_string( id_str, objectInstance );
         message_publish( MSG_WARNING, "FedAmb::reflectAttributeValues():%d Received update to Unknown Object Instance, ID:%s\n",
                          __LINE__, id_str.c_str() );
      }
   }
}
#endif // IEEE_1516_2010

void FedAmb::reflectAttributeValues(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const     &objectInstance,
   AttributeHandleValueMap const  &attributeValues,
   VariableLengthData const       &userSuppliedTag,
   TransportationTypeHandle const &transportationType,
   FederateHandle const           &producingFederate,
   RegionHandleSet const          *optionalSentRegions,
   LogicalTime const              &time,
   OrderType                       sentOrderType,
   OrderType                       receivedOrderType,
   MessageRetractionHandle const  *optionalRetraction )
#else
   ObjectInstanceHandle           objectInstance,
   AttributeHandleValueMap const &attributeValues,
   VariableLengthData const      &userSuppliedTag,
   OrderType                      sentOrderType,
   TransportationType             transportationType,
   LogicalTime const             &time,
   OrderType                      receivedOrderType,
   MessageRetractionHandle        optionalRetraction,
   SupplementalReflectInfo        reflectInfo ) throw( FederateInternalError )
#endif // IEEE_1516_2025
{
   // Get the TrickHLA object for the given Object Instance Handle.
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( objectInstance ) : NULL;

   if ( trickhla_obj != NULL ) {

      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         Int64Time i64time;
         i64time.set( time );
         message_publish( MSG_NORMAL, "FedAmb:reflectAttributeValues():%d '%s' HLA-time:%.12G seconds.\n",
                          __LINE__, trickhla_obj->get_name().c_str(), i64time.get_time_in_seconds() );
      }

      trickhla_obj->enqueue_data( const_cast< AttributeHandleValueMap & >( attributeValues ) );

#ifdef THLA_CHECK_SEND_AND_RECEIVE_COUNTS
      ++trickhla_obj->receive_count;
#endif
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         string id_str;
         StringUtilities::to_string( id_str, objectInstance );
         message_publish( MSG_WARNING, "FedAmb::reflectAttributeValues():%d Received update to Unknown Object Instance, ID:%s\n",
                          __LINE__, id_str.c_str() );
      }
   }
}

void FedAmb::receiveInteraction(
#if defined( IEEE_1516_2025 )
   InteractionClassHandle const   &interactionClass,
   ParameterHandleValueMap const  &parameterValues,
   VariableLengthData const       &userSuppliedTag,
   TransportationTypeHandle const &transportationType,
   FederateHandle const           &producingFederate,
   RegionHandleSet const          *optionalSentRegions )
#else
   InteractionClassHandle         interactionClass,
   ParameterHandleValueMap const &parameterValues,
   VariableLengthData const      &userSuppliedTag,
   OrderType                      sentOrderType,
   TransportationType             transportationType,
   SupplementalReceiveInfo        receiveInfo ) throw( FederateInternalError )
#endif // IEEE_1516_2025
{
   if ( manager == NULL ) {
      message_publish( MSG_WARNING, "FedAmb::receiveInteraction():%d NULL Manager!\n",
                       __LINE__ );
   } else {
      Int64Time dummyTime;

      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         message_publish( MSG_WARNING, "FedAmb::receiveInteraction():%d \n",
                          __LINE__ );
      }

      // Process the interaction.
      manager->receive_interaction( interactionClass,
                                    const_cast< ParameterHandleValueMap & >( parameterValues ),
                                    userSuppliedTag,
                                    dummyTime.get(),
                                    false );
   }
}

#if defined( IEEE_1516_2010 )
void FedAmb::receiveInteraction(
   InteractionClassHandle         interactionClass,
   ParameterHandleValueMap const &parameterValues,
   VariableLengthData const      &userSuppliedTag,
   OrderType                      sentOrderType,
   TransportationType             transportationType,
   LogicalTime const             &time,
   OrderType                      receivedOrderType,
   SupplementalReceiveInfo        receiveInfo ) throw( FederateInternalError )
{
   if ( manager == NULL ) {
      message_publish( MSG_WARNING, "FedAmb::receiveInteraction():%d NULL Manager!\n",
                       __LINE__ );
   } else {
      // Process the interaction.
      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         Int64Time i64time;
         i64time.set( time );
         message_publish( MSG_WARNING, "FedAmb::receiveInteraction():%d HLA-time:%.12G seconds.\n",
                          __LINE__, i64time.get_time_in_seconds() );
      }

      manager->receive_interaction( interactionClass,
                                    const_cast< ParameterHandleValueMap & >( parameterValues ),
                                    userSuppliedTag,
                                    time,
                                    ( receivedOrderType == RTI1516_NAMESPACE::TIMESTAMP ) );
   }
}
#endif // IEEE_1516_2010

void FedAmb::receiveInteraction(
#if defined( IEEE_1516_2025 )
   InteractionClassHandle const   &interactionClass,
   ParameterHandleValueMap const  &parameterValues,
   VariableLengthData const       &userSuppliedTag,
   TransportationTypeHandle const &transportationType,
   FederateHandle const           &producingFederate,
   RegionHandleSet const          *optionalSentRegions,
   LogicalTime const              &time,
   OrderType                       sentOrderType,
   OrderType                       receivedOrderType,
   MessageRetractionHandle const  *optionalRetraction )
#else
   InteractionClassHandle         interactionClass,
   ParameterHandleValueMap const &parameterValues,
   VariableLengthData const      &userSuppliedTag,
   OrderType                      sentOrderType,
   TransportationType             transportationType,
   LogicalTime const             &time,
   OrderType                      receivedOrderType,
   MessageRetractionHandle        optionalRetraction,
   SupplementalReceiveInfo        receiveInfo ) throw( FederateInternalError )
#endif // IEEE_1516_2025
{
   if ( manager == NULL ) {
      message_publish( MSG_WARNING, "FedAmb::receiveInteraction():%d NULL Manager!\n",
                       __LINE__ );
   } else {
      if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
         Int64Time i64time;
         i64time.set( time );
         message_publish( MSG_WARNING, "FedAmb::receiveInteraction():%d HLA-time:%.12G seconds.\n",
                          __LINE__, i64time.get_time_in_seconds() );
      }

      // Process the interaction.
      manager->receive_interaction( interactionClass,
                                    const_cast< ParameterHandleValueMap & >( parameterValues ),
                                    userSuppliedTag,
                                    time,
                                    ( receivedOrderType == RTI1516_NAMESPACE::TIMESTAMP ) );
   }
}

#if defined( IEEE_1516_2025 )
void FedAmb::receiveDirectedInteraction(
   InteractionClassHandle const   &interactionClass,
   ObjectInstanceHandle const     &objectInstance,
   ParameterHandleValueMap const  &parameterValues,
   VariableLengthData const       &userSuppliedTag,
   TransportationTypeHandle const &transportationType,
   FederateHandle const           &producingFederate )
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::receiveDirectedInteraction():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::receiveDirectedInteraction(
   InteractionClassHandle const   &interactionClass,
   ObjectInstanceHandle const     &objectInstance,
   ParameterHandleValueMap const  &parameterValues,
   VariableLengthData const       &userSuppliedTag,
   TransportationTypeHandle const &transportationType,
   FederateHandle const           &producingFederate,
   LogicalTime const              &time,
   OrderType                       sentOrderType,
   OrderType                       receivedOrderType,
   MessageRetractionHandle const  *optionalRetraction )
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::receiveDirectedInteraction():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}
#endif // IEEE_1516_2025

void FedAmb::removeObjectInstance(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   VariableLengthData const   &userSuppliedTag,
   FederateHandle const       &producingFederate )
#else
   ObjectInstanceHandle      objectInstance,
   VariableLengthData const &userSuppliedTag,
   OrderType                 sentOrderType,
   SupplementalRemoveInfo    removeInfo ) throw( FederateInternalError )
#endif // IEEE_1516_2025
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string id_str;
      StringUtilities::to_string( id_str, objectInstance );

      if ( userSuppliedTag.size() > 0 ) {
         char const *tag = static_cast< char const * >( userSuppliedTag.data() );
         message_publish( MSG_NORMAL, "FedAmb::removeObjectInstance():%d User-Supplied-Tag='%s' Instance-ID:%s Valid-ID:%s \n",
                          __LINE__, tag, id_str.c_str(),
                          ( objectInstance.isValid() ? "Yes" : "No" ) );
      } else {
         message_publish( MSG_NORMAL, "FedAmb::removeObjectInstance():%d Instance-ID:%s Valid-ID:%s \n",
                          __LINE__, id_str.c_str(),
                          ( objectInstance.isValid() ? "Yes" : "No" ) );
      }
   }

   // Remove the instance ID for a federate, which this function will test for.
   federate->remove_MOM_HLAfederate_instance_id( objectInstance );

   // Mark this object as deleted from the RTI.
   manager->mark_object_as_deleted_from_federation( objectInstance );
}

#if defined( IEEE_1516_2010 )
void FedAmb::removeObjectInstance(
   ObjectInstanceHandle      objectInstance,
   VariableLengthData const &userSuppliedTag,
   OrderType                 sentOrderType,
   LogicalTime const        &time,
   OrderType                 receivedOrderType,
   SupplementalRemoveInfo    removeInfo ) throw( FederateInternalError )
{
   // Remove the instance ID for a federate, which this function will test for.
   federate->remove_MOM_HLAfederate_instance_id( objectInstance );

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string id_str;
      StringUtilities::to_string( id_str, objectInstance );
      Int64Time i64time;
      i64time.set( time );

      if ( userSuppliedTag.size() > 0 ) {
         char const *tag = static_cast< char const * >( userSuppliedTag.data() );
         message_publish( MSG_NORMAL, "FedAmb::removeObjectInstance():%d tag='%s' Instance-ID:%s HLA-time:%.12G seconds.\n",
                          __LINE__, tag, id_str.c_str(), i64time.get_time_in_seconds() );
      } else {
         message_publish( MSG_NORMAL, "FedAmb::removeObjectInstance():%d Instance-ID:%s HLA-time:%.12G seconds.\n",
                          __LINE__, id_str.c_str(), i64time.get_time_in_seconds() );
      }
   }

   // Mark this object as deleted from the RTI.
   manager->mark_object_as_deleted_from_federation( objectInstance );
}
#endif // IEEE_1516_2010

void FedAmb::removeObjectInstance(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const    &objectInstance,
   VariableLengthData const      &userSuppliedTag,
   FederateHandle const          &producingFederate,
   LogicalTime const             &time,
   OrderType                      sentOrderType,
   OrderType                      receivedOrderType,
   MessageRetractionHandle const *optionalRetraction )
#else
   ObjectInstanceHandle      objectInstance,
   VariableLengthData const &userSuppliedTag,
   OrderType                 sentOrderType,
   LogicalTime const        &time,
   OrderType                 receivedOrderType,
   MessageRetractionHandle   optionalRetraction,
   SupplementalRemoveInfo    removeInfo ) throw( FederateInternalError )
#endif // IEEE_1516_2025
{
   // Remove the instance ID for a federate, which this function will test for.
   federate->remove_MOM_HLAfederate_instance_id( objectInstance );

   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      string id_str;
      StringUtilities::to_string( id_str, objectInstance );

      Int64Time i64time;
      i64time.set( time );

      if ( userSuppliedTag.size() > 0 ) {
         char const *tag = static_cast< char const * >( userSuppliedTag.data() );
         message_publish( MSG_NORMAL, "FedAmb::removeObjectInstance():%d tag='%s' Instance-ID:%s HLA-time:%.12G seconds.\n",
                          __LINE__, tag, id_str.c_str(), i64time.get_time_in_seconds() );
      } else {
         message_publish( MSG_NORMAL, "FedAmb::removeObjectInstance():%d Instance-ID:%s HLA-time:%.12G seconds.\n",
                          __LINE__, id_str.c_str(), i64time.get_time_in_seconds() );
      }
   }

   // Mark this object as deleted from the RTI.
   manager->mark_object_as_deleted_from_federation( objectInstance );
}

void FedAmb::attributesInScope(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &attributes ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::attributesInScope():%d \n",
                    federate->get_federate_name().c_str(),
                    __LINE__ );
}

void FedAmb::attributesOutOfScope(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &attributes ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::attributesOutOfScope():%d \n",
                    federate->get_federate_name().c_str(),
                    __LINE__ );
}

void FedAmb::provideAttributeValueUpdate(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes,
   VariableLengthData const   &userSuppliedTag )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &attributes,
   VariableLengthData const &userSuppliedTag ) throw( FederateInternalError )
#endif
{
   if ( manager != NULL ) {
      manager->provide_attribute_update( objectInstance,
                                         const_cast< AttributeHandleSet & >( attributes ) );
   }
}

void FedAmb::turnUpdatesOnForObjectInstance(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &attributes ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::turnUpdatesOnForObjectInstance():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::turnUpdatesOnForObjectInstance(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes,
   wstring const              &updateRateDesignator )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &attributes,
   wstring const            &updateRateDesignator ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::turnUpdatesOnForObjectInstance():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::turnUpdatesOffForObjectInstance(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &attributes ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::turnUpdatesOffForObjectInstance():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::confirmAttributeTransportationTypeChange(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const     &objectInstance,
   AttributeHandleSet const       &attributes,
   TransportationTypeHandle const &transportationType )
#else
   ObjectInstanceHandle objectInstance,
   AttributeHandleSet   attributes,
   TransportationType   transportationType ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::confirmAttributeTransportationTypeChange():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::reportAttributeTransportationType(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const     &objectInstance,
   AttributeHandle const          &attribute,
   TransportationTypeHandle const &transportationType )
#else
   ObjectInstanceHandle objectInstance,
   AttributeHandle      attribute,
   TransportationType   transportationType ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::reportAttributeTransportationType():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::confirmInteractionTransportationTypeChange(
#if defined( IEEE_1516_2025 )
   InteractionClassHandle const   &interactionClass,
   TransportationTypeHandle const &transportationType )
#else
   InteractionClassHandle interactionClass,
   TransportationType     transportationType ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::confirmInteractionTransportationTypeChange():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::reportInteractionTransportationType(
#if defined( IEEE_1516_2025 )
   FederateHandle const           &federateHandle,
   InteractionClassHandle const   &interactionClass,
   TransportationTypeHandle const &transportationType )
#else
   FederateHandle         federateHandle,
   InteractionClassHandle interactionClass,
   TransportationType     transportationType ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::reportInteractionTransportationType():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

///////////////////////////////////
// Ownership Management Services //
///////////////////////////////////

void FedAmb::requestAttributeOwnershipAssumption(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &offeredAttributes,
   VariableLengthData const   &userSuppliedTag )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &offeredAttributes,
   VariableLengthData const &userSuppliedTag ) throw( FederateInternalError )
#endif
{
   char const *tag = static_cast< char const * >( userSuppliedTag.data() );
   if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::requestAttributeOwnershipAssumption():%d push request received, tag='%s'\n",
                       __LINE__, tag );
   }

   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( objectInstance ) : NULL;

   if ( trickhla_obj != NULL ) {

      bool any_attribute_not_recognized = false;
      bool any_attribute_already_owned  = false;
      bool any_attribute_not_published  = false;

      AttributeHandleSet::const_iterator iter;

      // To make the state of the attribute push_requested flag thread safe lock
      // the mutex now. This allows us to handle multiple simultaneous requests
      // to push attributes to our federate.
      {
         // When auto_unlock_mutex goes out of scope it automatically unlocks the
         // mutex even if there is an exception.
         MutexProtection auto_unlock_mutex( &trickhla_obj->push_mutex );

         // Mark which attributes we can accept ownership of.
         for ( iter = offeredAttributes.begin(); iter != offeredAttributes.end(); ++iter ) {

            // Get the attribute object for the given attribute handle.
            Attribute *trick_hla_attr = trickhla_obj->get_attribute( *iter );

            // We can accept ownership of the attribute if our object contains it
            // as an attribute, is remotely owned, and we are setup to publish it.
            if ( ( trick_hla_attr != NULL ) && trick_hla_attr->is_remotely_owned() && trick_hla_attr->is_publish() ) {

               trick_hla_attr->set_push_requested( true );

               if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
                  message_publish( MSG_NORMAL, "FedAmb::requestAttributeOwnershipAssumption():%d\
\n   Attribute '%s'->'%s' of object '%s'.\n",
                                   __LINE__,
                                   trickhla_obj->get_FOM_name().c_str(),
                                   trick_hla_attr->get_FOM_name().c_str(),
                                   trickhla_obj->get_name().c_str() );
               }
            } else if ( trick_hla_attr == NULL ) {

               // Handle the case where the attribute is not recognized.
               any_attribute_not_recognized = true;

               if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
                  message_publish( MSG_WARNING, "FedAmb::requestAttributeOwnershipAssumption():%d \
Attribute Not Recognized ERROR: Object '%s' with FOM name '%s'!\n",
                                   __LINE__, trickhla_obj->get_name().c_str(), trickhla_obj->get_FOM_name().c_str() );
               }
            } else if ( trick_hla_attr->is_locally_owned() ) {

               // Handle the case where the attribute is already owned.
               any_attribute_already_owned = true;

               if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
                  message_publish( MSG_WARNING, "FedAmb::requestAttributeOwnershipAssumption():%d \
Attribute Already Owned ERROR: Object '%s' with attribute '%s'->'%s'!\n",
                                   __LINE__,
                                   trickhla_obj->get_name().c_str(),
                                   trickhla_obj->get_FOM_name().c_str(),
                                   trick_hla_attr->get_FOM_name().c_str() );
               }
            } else if ( !trick_hla_attr->is_publish() ) {

               // Handle the case where the attribute is not published.
               any_attribute_not_published = true;

               if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
                  message_publish( MSG_WARNING, "FedAmb::requestAttributeOwnershipAssumption():%d \
Attribute Not Published ERROR: Object '%s' with attribute '%s'->'%s'!\n",
                                   __LINE__,
                                   trickhla_obj->get_name().c_str(),
                                   trickhla_obj->get_FOM_name().c_str(),
                                   trick_hla_attr->get_FOM_name().c_str() );
               }
            }
         }

         // Unlock the mutex when auto_unlock_mutex goes out of scope.
      }

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
      StringUtilities::to_string( id_str, objectInstance );
      message_publish( MSG_NORMAL, "FedAmb::requestAttributeOwnershipAssumption():%d \
Unknown object instance (ID:%s), push request rejected, tag='%s' \n",
                       __LINE__, id_str.c_str(), tag );

      throw FederateInternalError( L"FedAmb::requestAttributeOwnershipAssumption() Unknown object instance" );
   }
}

void FedAmb::requestDivestitureConfirmation(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &releasedAttributes,
   VariableLengthData const   &userSuppliedTag )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &releasedAttributes ) throw( FederateInternalError )
#endif
{
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( objectInstance ) : NULL;

   if ( trickhla_obj == NULL ) {
      string id_str;
      StringUtilities::to_string( id_str, objectInstance );
      message_publish( MSG_NORMAL, "FedAmb::requestDivestitureConfirmation():%d Unknown \
object instance (ID:%s), push request rejected.\n",
                       __LINE__, id_str.c_str() );

      throw FederateInternalError( L"FedAmb::requestDivestitureConfirmation() Unknown object instance." );
   }

   bool any_devist_requested         = false;
   bool any_attribute_not_recognized = false;
   bool any_attribute_not_owned      = false;

   AttributeHandleSet::const_iterator iter;

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

         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            message_publish( MSG_NORMAL, "FedAmb::requestDivestitureConfirmation():%d\
\n   Attribute '%s'->'%s' of object '%s'.\n",
                             __LINE__,
                             trickhla_obj->get_FOM_name().c_str(),
                             trick_hla_attr->get_FOM_name().c_str(),
                             trickhla_obj->get_name().c_str() );
         }
      } else if ( trick_hla_attr == NULL ) {

         // Handle the case where the attribute is not recognized.
         any_attribute_not_recognized = true;

         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            message_publish( MSG_WARNING, "FedAmb::requestDivestitureConfirmation():%d \
Attribute Not Recognized ERROR: Object '%s' with FOM name '%s'!\n",
                             __LINE__, trickhla_obj->get_name().c_str(), trickhla_obj->get_FOM_name().c_str() );
         }
      } else if ( trick_hla_attr->is_remotely_owned() ) {

         // Handle the case where the attribute is not owned.
         any_attribute_not_owned = true;

         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
            message_publish( MSG_WARNING, "FedAmb::requestDivestitureConfirmation():%d \
Attribute Not Owned ERROR: Object '%s' with attribute '%s'->'%s'!\n",
                             __LINE__,
                             trickhla_obj->get_name().c_str(),
                             trickhla_obj->get_FOM_name().c_str(),
                             trick_hla_attr->get_FOM_name().c_str() );
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

void FedAmb::attributeOwnershipAcquisitionNotification(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &securedAttributes,
   VariableLengthData const   &userSuppliedTag )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &securedAttributes,
   VariableLengthData const &userSuppliedTag ) throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::attributeOwnershipAcquisitionNotification():%d \n",
                       __LINE__ );
   }

   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( objectInstance ) : NULL;

   if ( trickhla_obj != NULL ) {

      bool any_attribute_acquired       = false;
      bool any_attribute_not_recognized = false;
      bool any_attribute_already_owned  = false;
      bool any_attribute_not_published  = false;

      AttributeHandleSet::const_iterator iter;

      // Mark which attributes we now locally own.
      for ( iter = securedAttributes.begin(); iter != securedAttributes.end(); ++iter ) {

         // Get the attribute for the given attribute handle.
         Attribute *trick_hla_attr = trickhla_obj->get_attribute( *iter );

         // Mark the attribute as locally owned if the object has it as an
         // attribute and is remotely owned, and we are setup to publish it.
         if ( ( trick_hla_attr != NULL ) && trick_hla_attr->is_remotely_owned() && trick_hla_attr->is_publish() ) {

            trick_hla_attr->mark_locally_owned();
            any_attribute_acquired = true;

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               message_publish( MSG_NORMAL, "FedAmb::attributeOwnershipAcquisitionNotification():%d\
\n   ACQUIRED ownership of attribute '%s'->'%s' of object '%s'.\n",
                                __LINE__,
                                trickhla_obj->get_FOM_name().c_str(),
                                trick_hla_attr->get_FOM_name().c_str(),
                                trickhla_obj->get_name().c_str() );
            }
         } else if ( trick_hla_attr == NULL ) {

            // Handle the case where the attribute is not recognized.
            any_attribute_not_recognized = true;

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               message_publish( MSG_WARNING, "FedAmb::attributeOwnershipAcquisitionNotification():%d \
Attribute Not Recognized ERROR: Object '%s' with FOM name '%s'!\n",
                                __LINE__, trickhla_obj->get_name().c_str(), trickhla_obj->get_FOM_name().c_str() );
            }
         } else if ( trick_hla_attr->is_locally_owned() ) {

            // Handle the case where the attribute is already owned.
            any_attribute_already_owned = true;

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               message_publish( MSG_WARNING, "FedAmb::attributeOwnershipAcquisitionNotification():%d \
Attribute Already Owned ERROR: Object '%s' with attribute '%s'->'%s'!\n",
                                __LINE__,
                                trickhla_obj->get_name().c_str(),
                                trickhla_obj->get_FOM_name().c_str(),
                                trick_hla_attr->get_FOM_name().c_str() );
            }
         } else if ( !trick_hla_attr->is_publish() ) {

            // Handle the case where the attribute is not published.
            any_attribute_not_published = true;

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               message_publish( MSG_WARNING, "FedAmb::attributeOwnershipAcquisitionNotification():%d \
Attribute Not Published ERROR: Object '%s' with attribute '%s'->'%s'!\n",
                                __LINE__,
                                trickhla_obj->get_name().c_str(),
                                trickhla_obj->get_FOM_name().c_str(),
                                trick_hla_attr->get_FOM_name().c_str() );
            }
         }
      }

      // If we are doing blocking I/O we need to let the object know that it
      // now owns at least one attribute.
      if ( any_attribute_acquired ) {
         trickhla_obj->set_attribute_ownership_acquired();
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
      message_publish( MSG_WARNING, "FedAmb::attributeOwnershipAcquisitionNotification():%d \
Ownership acquisition rejected (object not found)\n",
                       __LINE__ );

      throw FederateInternalError( L"FedAmb::attributeOwnershipAcquisitionNotification() Unknown object instance" );
   }
}

void FedAmb::attributeOwnershipUnavailable(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes,
   VariableLengthData const   &userSuppliedTag )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &releasedAttributes ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::attributeOwnershipUnavailable():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::requestAttributeOwnershipRelease(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &candidateAttributes,
   VariableLengthData const   &userSuppliedTag )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &candidateAttributes,
   VariableLengthData const &userSuppliedTag ) throw( FederateInternalError )
#endif
{
   char const *tag = static_cast< char const * >( userSuppliedTag.data() );
   if ( DebugHandler::show( DEBUG_LEVEL_8_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::requestAttributeOwnershipRelease():%d pull request received, tag='%s'\n",
                       __LINE__, tag );
   }
   Object *trickhla_obj = ( manager != NULL ) ? manager->get_trickhla_object( objectInstance ) : NULL;

   if ( trickhla_obj != NULL ) {

      bool any_pull_requested           = false;
      bool any_attribute_not_recognized = false;
      bool any_attribute_not_owned      = false;

      AttributeHandleSet::const_iterator iter;

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

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               message_publish( MSG_NORMAL, "FedAmb::requestAttributeOwnershipRelease():%d\
\n   Attribute '%s'->'%s' of object '%s'.\n",
                                __LINE__,
                                trickhla_obj->get_FOM_name().c_str(),
                                trick_hla_attr->get_FOM_name().c_str(),
                                trickhla_obj->get_name().c_str() );
            }
         } else if ( trick_hla_attr == NULL ) {

            // Handle the case where the attribute is not recognized.
            any_attribute_not_recognized = true;

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               message_publish( MSG_WARNING, "FedAmb::requestAttributeOwnershipRelease():%d \
Attribute Not Recognized ERROR: Object '%s' with FOM name '%s'!\n",
                                __LINE__, trickhla_obj->get_name().c_str(), trickhla_obj->get_FOM_name().c_str() );
            }
         } else if ( trick_hla_attr->is_remotely_owned() ) {

            // Handle the case where the attribute is not owned.
            any_attribute_not_owned = true;

            if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
               message_publish( MSG_WARNING, "FedAmb::requestAttributeOwnershipRelease():%d \
Attribute Not Owned ERROR: Object '%s' with attribute '%s'->'%s'!\n",
                                __LINE__,
                                trickhla_obj->get_name().c_str(),
                                trickhla_obj->get_FOM_name().c_str(),
                                trick_hla_attr->get_FOM_name().c_str() );
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
      message_publish( MSG_WARNING, "FedAmb::requestAttributeOwnershipRelease():%d pull rejected (not found), tag='%s'\n",
                       __LINE__, tag );
      throw FederateInternalError( L"FedAmb::requestAttributeOwnershipRelease() Unknown object instance" );
   }
}

void FedAmb::confirmAttributeOwnershipAcquisitionCancellation(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes )
#else
   ObjectInstanceHandle      objectInstance,
   AttributeHandleSet const &releasedAttributes ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::confirmAttributeOwnershipAcquisitionCancellation():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::informAttributeOwnership(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes,
   FederateHandle const       &owner )
#else
   ObjectInstanceHandle objectInstance,
   AttributeHandle      attribute,
   FederateHandle       owner ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::informAttributeOwnership():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::attributeIsNotOwned(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes )
#else
   ObjectInstanceHandle objectInstance,
   AttributeHandle      attribute ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::attributeIsNotOwned():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

void FedAmb::attributeIsOwnedByRTI(
#if defined( IEEE_1516_2025 )
   ObjectInstanceHandle const &objectInstance,
   AttributeHandleSet const   &attributes )
#else
   ObjectInstanceHandle objectInstance,
   AttributeHandle      attribute ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::attributeIsOwnedByRTI():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

//////////////////////////////
// Time Management Services //
//////////////////////////////

void FedAmb::timeRegulationEnabled(
   LogicalTime const &time )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::timeRegulationEnabled():%d Federate \"%s\" \n",
                       __LINE__, federate->get_federate_name().c_str() );
   }
   federate->set_time_regulation_enabled( time );
}

void FedAmb::timeConstrainedEnabled(
   LogicalTime const &time )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   if ( DebugHandler::show( DEBUG_LEVEL_2_TRACE, DEBUG_SOURCE_FED_AMB ) ) {
      message_publish( MSG_NORMAL, "FedAmb::timeConstrainedEnabled():%d Federate \"%s\" Time granted to: %.12G \n",
                       __LINE__, federate->get_federate_name().c_str(),
                       federate->get_granted_time().get_time_in_seconds() );
   }
   federate->set_time_constrained_enabled( time );
}

#if defined( IEEE_1516_2025 )
void FedAmb::flushQueueGrant(
   LogicalTime const &time,
   LogicalTime const &optimisticTime )
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::flushQueueGrant():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}
#endif // IEEE_1516_2025

void FedAmb::timeAdvanceGrant(
   LogicalTime const &time )
#if defined( IEEE_1516_2010 )
   throw( FederateInternalError )
#endif
{
   federate->set_time_advance_granted( time );
}

void FedAmb::requestRetraction(
#if defined( IEEE_1516_2025 )
   MessageRetractionHandle const &retraction )
#else
   MessageRetractionHandle retraction ) throw( FederateInternalError )
#endif
{
   message_publish( MSG_WARNING, "This federate '%s' does not support this function: \
FedAmb::requestRetraction():%d \n",
                    federate->get_federate_name().c_str(), __LINE__ );
}

#if !defined( IEEE_1516_2025 )
// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#   pragma GCC diagnostic pop
#endif
