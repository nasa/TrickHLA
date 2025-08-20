/*!
@file TrickHLA/FedAmbHLA4.hh
@ingroup TrickHLA
@brief Provides methods called by the HLA RTI Ambassador for IEEE 1516-2025.

This class is essentially a polymorphic callback class provided to the
RTI Ambassador. It provides methods called by the RTI Ambassador for
simulation object, interaction and time management.

The methods in this class fill out the required virtual methods of the
rti1516_2025::FederateAmbassador abstract class to enable the FedAmb class
to be instantiated. While this class is instantiable, for almost all practical
applications, a simulation developer will want to overload the necessary
attribute and interaction callback functions to make things work properly for
their particular federation needs.

\par<b>Assumptions and Limitations:</b>
- Derived class of abstract FederateAmbassador class to implement methods so
that RTI can call functions in the federate.

@copyright Copyright 2025 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../source/TrickHLA/FedAmb.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2025, --, Add HLA 4 support}
@revs_end

*/

#ifndef TRICKHLA_FED_AMB_HLA4_HH
#define TRICKHLA_FED_AMB_HLA4_HH

// System includes.
#include <set>
#include <string>

// TrickHLA includes.
#include "FedAmbBase.hh"
#include "StandardsSupport.hh"

#if defined( IEEE_1516_2025 )

// HLA include files.
#   include "RTI/Enums.h"
#   include "RTI/FederateAmbassador.h"
#   include "RTI/Handle.h"
#   include "RTI/RTI1516.h"
#   include "RTI/Typedefs.h"
#   include "RTI/time/LogicalTime.h"

namespace TrickHLA
{

class FedAmb : public rti1516_2025::FederateAmbassador, public FedAmbBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__FedAmb();

  public:
   /*! @brief Default constructor for the TrickHLA FedAmb class. */
   FedAmb();
   /*! @brief Destructor for the TrickHLA FedAmb class. */
   virtual ~FedAmb();

   /*! @brief Initialize the TrickHLA Federate Ambassador instance for this
    *  Federation Execution. */
   virtual void initialize();

   // *****************************************************************
   // The following methods fill out the required virtual methods of
   // the rti1516_2025::FederateAmbassador abstract class to
   // enable the FedAmb class to be instantiated. While this class can
   // be instantiated, for almost all practical applications, a
   // simulation developer will want to overload the necessary
   // attribute and interaction callback functions to make things work
   // properly for their particular federation needs.
   // *****************************************************************

   ////////////////////////////////////
   // Federation Management Services //
   ////////////////////////////////////

   // 4.4
   virtual void connectionLost(
      std::wstring const &faultDescription );

   // 4.8
   virtual void reportFederationExecutions(
      rti1516_2025::FederationExecutionInformationVector const &report );

   // 4.10
   virtual void reportFederationExecutionMembers(
      std::wstring const                                             &federationName,
      rti1516_2025::FederationExecutionMemberInformationVector const &report );

   // 4.11
   virtual void reportFederationExecutionDoesNotExist(
      std::wstring const &federationName );

   // 4.13
   virtual void federateResigned(
      std::wstring const &reasonForResignDescription );

   // 4.15
   virtual void synchronizationPointRegistrationSucceeded(
      std::wstring const &label );

   // 4.15
   virtual void synchronizationPointRegistrationFailed(
      std::wstring const                             &label,
      rti1516_2025::SynchronizationPointFailureReason reason );

   // 4.16
   virtual void announceSynchronizationPoint(
      std::wstring const                     &label,
      rti1516_2025::VariableLengthData const &userSuppliedTag );

   // 4.18
   virtual void federationSynchronized(
      std::wstring const                    &label,
      rti1516_2025::FederateHandleSet const &failedToSyncSet );

   // 4.20
   virtual void initiateFederateSave(
      std::wstring const &label );

   // 4.20
   virtual void initiateFederateSave(
      std::wstring const              &label,
      rti1516_2025::LogicalTime const &time );

   // 4.23
   virtual void federationSaved();

   // 4.23
   virtual void federationNotSaved(
      rti1516_2025::SaveFailureReason reason );

   // 4.26
   virtual void federationSaveStatusResponse(
      rti1516_2025::FederateHandleSaveStatusPairVector const &response );

   // 4.28
   virtual void requestFederationRestoreSucceeded(
      std::wstring const &label );

   // 4.28
   virtual void requestFederationRestoreFailed(
      std::wstring const &label );

   // 4.29
   virtual void federationRestoreBegun();

   // 4.30
   virtual void initiateFederateRestore(
      std::wstring const                 &label,
      std::wstring const                 &federateName,
      rti1516_2025::FederateHandle const &postRestoreFederateHandle );

   // 4.32
   virtual void federationRestored();

   // 4.32
   virtual void federationNotRestored(
      rti1516_2025::RestoreFailureReason reason );

   // 4.35
   virtual void federationRestoreStatusResponse(
      rti1516_2025::FederateRestoreStatusVector const &response );

   /////////////////////////////////////
   // Declaration Management Services //
   /////////////////////////////////////

   // 5.14
   virtual void startRegistrationForObjectClass(
      rti1516_2025::ObjectClassHandle const &objectClass );

   // 5.15
   virtual void stopRegistrationForObjectClass(
      rti1516_2025::ObjectClassHandle const &objectClass );

   // 5.16
   virtual void turnInteractionsOn(
      rti1516_2025::InteractionClassHandle const &interactionClass );

   // 5.17
   virtual void turnInteractionsOff(
      rti1516_2025::InteractionClassHandle const &interactionClass );

   ////////////////////////////////
   // Object Management Services //
   ////////////////////////////////

   // 6.3
   virtual void objectInstanceNameReservationSucceeded(
      std::wstring const &objectInstanceName );

   // 6.3
   virtual void objectInstanceNameReservationFailed(
      std::wstring const &objectInstanceName );

   // 6.6
   virtual void multipleObjectInstanceNameReservationSucceeded(
      std::set< std::wstring > const &objectInstanceNames );

   // 6.6
   virtual void multipleObjectInstanceNameReservationFailed(
      std::set< std::wstring > const &objectInstanceNames );

   // 6.9
   virtual void discoverObjectInstance(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::ObjectClassHandle const    &objectClass,
      std::wstring const                       &objectInstanceName,
      rti1516_2025::FederateHandle const       &producingFederate );

   // 6.11
   virtual void reflectAttributeValues(
      rti1516_2025::ObjectInstanceHandle const     &objectInstance,
      rti1516_2025::AttributeHandleValueMap const  &attributeValues,
      rti1516_2025::VariableLengthData const       &userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const &transportationType,
      rti1516_2025::FederateHandle const           &producingFederate,
      rti1516_2025::RegionHandleSet const          *optionalSentRegions );

   // 6.11
   virtual void reflectAttributeValues(
      rti1516_2025::ObjectInstanceHandle const     &objectInstance,
      rti1516_2025::AttributeHandleValueMap const  &attributeValues,
      rti1516_2025::VariableLengthData const       &userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const &transportationType,
      rti1516_2025::FederateHandle const           &producingFederate,
      rti1516_2025::RegionHandleSet const          *optionalSentRegions,
      rti1516_2025::LogicalTime const              &time,
      rti1516_2025::OrderType                       sentOrderType,
      rti1516_2025::OrderType                       receivedOrderType,
      rti1516_2025::MessageRetractionHandle const  *optionalRetraction );

   // 6.13
   virtual void receiveInteraction(
      rti1516_2025::InteractionClassHandle const   &interactionClass,
      rti1516_2025::ParameterHandleValueMap const  &parameterValues,
      rti1516_2025::VariableLengthData const       &userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const &transportationType,
      rti1516_2025::FederateHandle const           &producingFederate,
      rti1516_2025::RegionHandleSet const          *optionalSentRegions );

   // 6.13
   virtual void receiveInteraction(
      rti1516_2025::InteractionClassHandle const   &interactionClass,
      rti1516_2025::ParameterHandleValueMap const  &parameterValues,
      rti1516_2025::VariableLengthData const       &userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const &transportationType,
      rti1516_2025::FederateHandle const           &producingFederate,
      rti1516_2025::RegionHandleSet const          *optionalSentRegions,
      rti1516_2025::LogicalTime const              &time,
      rti1516_2025::OrderType                       sentOrderType,
      rti1516_2025::OrderType                       receivedOrderType,
      rti1516_2025::MessageRetractionHandle const  *optionalRetraction );

   // 6.15
   virtual void receiveDirectedInteraction(
      rti1516_2025::InteractionClassHandle const   &interactionClass,
      rti1516_2025::ObjectInstanceHandle const     &objectInstance,
      rti1516_2025::ParameterHandleValueMap const  &parameterValues,
      rti1516_2025::VariableLengthData const       &userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const &transportationType,
      rti1516_2025::FederateHandle const           &producingFederate );

   // 6.15
   virtual void receiveDirectedInteraction(
      rti1516_2025::InteractionClassHandle const   &interactionClass,
      rti1516_2025::ObjectInstanceHandle const     &objectInstance,
      rti1516_2025::ParameterHandleValueMap const  &parameterValues,
      rti1516_2025::VariableLengthData const       &userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const &transportationType,
      rti1516_2025::FederateHandle const           &producingFederate,
      rti1516_2025::LogicalTime const              &time,
      rti1516_2025::OrderType                       sentOrderType,
      rti1516_2025::OrderType                       receivedOrderType,
      rti1516_2025::MessageRetractionHandle const  *optionalRetraction );

   // 6.17
   virtual void removeObjectInstance(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::VariableLengthData const   &userSuppliedTag,
      rti1516_2025::FederateHandle const       &producingFederate );

   // 6.17
   virtual void removeObjectInstance(
      rti1516_2025::ObjectInstanceHandle const    &objectInstance,
      rti1516_2025::VariableLengthData const      &userSuppliedTag,
      rti1516_2025::FederateHandle const          &producingFederate,
      rti1516_2025::LogicalTime const             &time,
      rti1516_2025::OrderType                      sentOrderType,
      rti1516_2025::OrderType                      receivedOrderType,
      rti1516_2025::MessageRetractionHandle const *optionalRetraction );

   // 6.19
   virtual void attributesInScope(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes );

   // 6.20
   virtual void attributesOutOfScope(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes );

   // 6.22
   virtual void provideAttributeValueUpdate(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes,
      rti1516_2025::VariableLengthData const   &userSuppliedTag );

   // 6.23
   virtual void turnUpdatesOnForObjectInstance(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes );

   // 6.23
   virtual void turnUpdatesOnForObjectInstance(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes,
      std::wstring const                       &updateRateDesignator );

   // 6.24
   virtual void turnUpdatesOffForObjectInstance(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes );

   // 6.26
   virtual void confirmAttributeTransportationTypeChange(
      rti1516_2025::ObjectInstanceHandle const     &objectInstance,
      rti1516_2025::AttributeHandleSet const       &attributes,
      rti1516_2025::TransportationTypeHandle const &transportationType );

   // 6.29
   virtual void reportAttributeTransportationType(
      rti1516_2025::ObjectInstanceHandle const     &objectInstance,
      rti1516_2025::AttributeHandle const          &attribute,
      rti1516_2025::TransportationTypeHandle const &transportationType );

   // 6.31
   virtual void confirmInteractionTransportationTypeChange(
      rti1516_2025::InteractionClassHandle const   &interactionClass,
      rti1516_2025::TransportationTypeHandle const &transportationType );

   // 6.33
   virtual void reportInteractionTransportationType(
      rti1516_2025::FederateHandle const           &federateHandle,
      rti1516_2025::InteractionClassHandle const   &interactionClass,
      rti1516_2025::TransportationTypeHandle const &transportationType );

   ///////////////////////////////////
   // Ownership Management Services //
   ///////////////////////////////////

   // 7.4
   virtual void requestAttributeOwnershipAssumption(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &offeredAttributes,
      rti1516_2025::VariableLengthData const   &userSuppliedTag );

   // 7.5
   virtual void requestDivestitureConfirmation(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &releasedAttributes,
      rti1516_2025::VariableLengthData const   &userSuppliedTag );

   // 7.7
   virtual void attributeOwnershipAcquisitionNotification(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &securedAttributes,
      rti1516_2025::VariableLengthData const   &userSuppliedTag );

   // 7.10
   virtual void attributeOwnershipUnavailable(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes,
      rti1516_2025::VariableLengthData const   &userSuppliedTag );

   // 7.11
   virtual void requestAttributeOwnershipRelease(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &candidateAttributes,
      rti1516_2025::VariableLengthData const   &userSuppliedTag );

   // 7.16
   virtual void confirmAttributeOwnershipAcquisitionCancellation(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes );

   // 7.18
   virtual void informAttributeOwnership(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes,
      rti1516_2025::FederateHandle const       &owner );

   // 7.18
   virtual void attributeIsNotOwned(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes );

   // 7.18
   virtual void attributeIsOwnedByRTI(
      rti1516_2025::ObjectInstanceHandle const &objectInstance,
      rti1516_2025::AttributeHandleSet const   &attributes );

   //////////////////////////////
   // Time Management Services //
   //////////////////////////////

   // 8.3
   virtual void timeRegulationEnabled(
      rti1516_2025::LogicalTime const &time );

   // 8.6
   virtual void timeConstrainedEnabled(
      rti1516_2025::LogicalTime const &time );

   // 8.13
   virtual void flushQueueGrant(
      rti1516_2025::LogicalTime const &time,
      rti1516_2025::LogicalTime const &optimisticTime );

   // 8.14
   virtual void timeAdvanceGrant(
      rti1516_2025::LogicalTime const &time );

   // 8.23
   virtual void requestRetraction(
      rti1516_2025::MessageRetractionHandle const &retraction );

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for FedAmb class.
    *  @details This constructor is private to prevent inadvertent copies. */
   FedAmb( FedAmb const &rhs );
   /*! @brief Assignment operator for FedAmb class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   FedAmb &operator=( FedAmb const &rhs );
};

} // namespace TrickHLA

#endif // IEEE_1516_2025

#endif // TRICKHLA_FED_AMB_HLA4_HH -- Do NOT put anything after this line.
