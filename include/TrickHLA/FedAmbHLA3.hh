/*!
@file TrickHLA/FedAmbHLA3.hh
@ingroup TrickHLA
@brief Provides methods called by the HLA RTI Ambassador for IEEE 1515-2010.

This class is essentially a polymorphic callback class provided to the
RTI Ambassador. It provides methods called by the RTI Ambassador for
simulation object, interaction and time management.

The methods in this class fill out the required virtual methods of the
rti1516e::FederateAmbassador abstract class to enable the FedAmb class
to be instantiated. While this class is instantiable, for almost all practical
applications, a simulation developer will want to overload the necessary
attribute and interaction callback functions to make things work properly for
their particular federation needs.

\par<b>Assumptions and Limitations:</b>
- Derived class of abstract FederateAmbassador class to implement methods so
that RTI can call functions in the federate.

@copyright Copyright 2019 United States Government as represented by the
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
@rev_entry{DMSO Programmer, DMSO, HLA, Mar 1998, --, HelloWorld Federate Ambassador.}
@rev_entry{Edwin Z. Crues, Titan Systems Corp., DIS, Feb 2002, --, HLA Ball Sim.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2025, --, Add HLA 4 support}
@revs_end

*/

#ifndef TRICKHLA_FED_AMB_HLA3_HH
#define TRICKHLA_FED_AMB_HLA3_HH

// System includes.
#include <set>
#include <string>

// TrickHLA includes.
#include "FedAmbBase.hh"
#include "HLAStandardSupport.hh"

#if defined( IEEE_1516_2010 )

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"

// HLA include files.
#   include "RTI/Enums.h"
#   include "RTI/FederateAmbassador.h"
#   include "RTI/RTI1516.h"
#   include "RTI/Typedefs.h"

namespace TrickHLA
{

class FedAmb : public rti1516e::FederateAmbassador, public FedAmbBase
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
   // the rti1516e::FederateAmbassador abstract class to
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
      std::wstring const &faultDescription ) throw( rti1516e::FederateInternalError );

   // 4.8
   virtual void reportFederationExecutions(
      rti1516e::FederationExecutionInformationVector const &
         report ) throw( rti1516e::FederateInternalError );

   // 4.7
   virtual void
   synchronizationPointRegistrationSucceeded(
      std::wstring const &label ) throw( rti1516e::FederateInternalError );

   // 4.7
   virtual void
   synchronizationPointRegistrationFailed(
      std::wstring const                         &label,
      rti1516e::SynchronizationPointFailureReason reason ) throw( rti1516e::FederateInternalError );

   // 4.8
   virtual void
   announceSynchronizationPoint(
      std::wstring const                 &label,
      rti1516e::VariableLengthData const &userSuppliedTag ) throw( rti1516e::FederateInternalError );

   // 4.10
   virtual void
   federationSynchronized(
      std::wstring const                &label,
      rti1516e::FederateHandleSet const &failedToSyncSet ) throw( rti1516e::FederateInternalError );

   // 4.12
   virtual void initiateFederateSave(
      std::wstring const &label ) throw( rti1516e::FederateInternalError );

   // 4.12
   virtual void
   initiateFederateSave(
      std::wstring const          &label,
      rti1516e::LogicalTime const &time ) throw( rti1516e::FederateInternalError );

   // 4.15
   virtual void federationSaved() throw( rti1516e::FederateInternalError );

   // 4.15
   virtual void
   federationNotSaved(
      rti1516e::SaveFailureReason reason ) throw( rti1516e::FederateInternalError );

   // 4.17
   virtual void federationSaveStatusResponse(
      rti1516e::FederateHandleSaveStatusPairVector const &
         response ) throw( rti1516e::FederateInternalError );

   // 4.19
   virtual void requestFederationRestoreSucceeded(
      std::wstring const &label ) throw( rti1516e::FederateInternalError );

   // 4.19
   virtual void requestFederationRestoreFailed(
      std::wstring const &label ) throw( rti1516e::FederateInternalError );

   // 4.20
   virtual void federationRestoreBegun() throw( rti1516e::FederateInternalError );

   // 4.21
   virtual void
   initiateFederateRestore(
      std::wstring const      &label,
      std::wstring const      &federateName,
      rti1516e::FederateHandle postRestoreFederateHandle ) throw( rti1516e::FederateInternalError );

   // 4.23
   virtual void federationRestored() throw( rti1516e::FederateInternalError );

   // 4.23
   virtual void
   federationNotRestored(
      rti1516e::RestoreFailureReason reason ) throw( rti1516e::FederateInternalError );

   // 4.25
   virtual void federationRestoreStatusResponse(
      rti1516e::FederateRestoreStatusVector const &
         response ) throw( rti1516e::FederateInternalError );

   /////////////////////////////////////
   // Declaration Management Services //
   /////////////////////////////////////

   // 5.10
   virtual void
   startRegistrationForObjectClass(
      rti1516e::ObjectClassHandle objectClass ) throw( rti1516e::FederateInternalError );

   // 5.11
   virtual void
   stopRegistrationForObjectClass(
      rti1516e::ObjectClassHandle objectClass ) throw( rti1516e::FederateInternalError );

   // 5.12
   virtual void
   turnInteractionsOn(
      rti1516e::InteractionClassHandle interactionClass ) throw( rti1516e::FederateInternalError );

   // 5.13
   virtual void
   turnInteractionsOff(
      rti1516e::InteractionClassHandle interactionClass ) throw( rti1516e::FederateInternalError );

   ////////////////////////////////
   // Object Management Services //
   ////////////////////////////////

   // 6.3
   virtual void
   objectInstanceNameReservationSucceeded(
      std::wstring const &objectInstanceName ) throw( rti1516e::FederateInternalError );

   // 6.3
   virtual void
   objectInstanceNameReservationFailed(
      std::wstring const &objectInstanceName ) throw( rti1516e::FederateInternalError );

   virtual void
   multipleObjectInstanceNameReservationSucceeded(
      std::set< std::wstring > const &objectInstanceNames ) throw( rti1516e::FederateInternalError );

   virtual void
   multipleObjectInstanceNameReservationFailed(
      std::set< std::wstring > const &objectInstanceNames ) throw( rti1516e::FederateInternalError );

   // 6.5
   virtual void
   discoverObjectInstance(
      rti1516e::ObjectInstanceHandle objectInstance,
      rti1516e::ObjectClassHandle    objectClass,
      std::wstring const            &objectInstanceName ) throw( rti1516e::FederateInternalError );

   virtual void
   discoverObjectInstance(
      rti1516e::ObjectInstanceHandle objectInstance,
      rti1516e::ObjectClassHandle    objectClass,
      std::wstring const            &objectInstanceName,
      rti1516e::FederateHandle       producingFederate ) throw( rti1516e::FederateInternalError );

   // 6.7
   virtual void
   reflectAttributeValues(
      rti1516e::ObjectInstanceHandle           objectInstance,
      rti1516e::AttributeHandleValueMap const &attributeValues,
      rti1516e::VariableLengthData const      &userSuppliedTag,
      rti1516e::OrderType                      sentOrderType,
      rti1516e::TransportationType             transportationType,
      rti1516e::SupplementalReflectInfo        reflectInfo ) throw( rti1516e::FederateInternalError );

   virtual void
   reflectAttributeValues(
      rti1516e::ObjectInstanceHandle           objectInstance,
      rti1516e::AttributeHandleValueMap const &attributeValues,
      rti1516e::VariableLengthData const      &userSuppliedTag,
      rti1516e::OrderType                      sentOrderType,
      rti1516e::TransportationType             transportationType,
      rti1516e::LogicalTime const             &time,
      rti1516e::OrderType                      receivedOrderType,
      rti1516e::SupplementalReflectInfo        reflectInfo ) throw( rti1516e::FederateInternalError );

   virtual void
   reflectAttributeValues(
      rti1516e::ObjectInstanceHandle           objectInstance,
      rti1516e::AttributeHandleValueMap const &attributeValues,
      rti1516e::VariableLengthData const      &userSuppliedTag,
      rti1516e::OrderType                      sentOrderType,
      rti1516e::TransportationType             transportationType,
      rti1516e::LogicalTime const             &time,
      rti1516e::OrderType                      receivedOrderType,
      rti1516e::MessageRetractionHandle        optionalRetraction,
      rti1516e::SupplementalReflectInfo        reflectInfo ) throw( rti1516e::FederateInternalError );

   // 6.9
   virtual void
   receiveInteraction(
      rti1516e::InteractionClassHandle         interactionClass,
      rti1516e::ParameterHandleValueMap const &parameterValues,
      rti1516e::VariableLengthData const      &userSuppliedTag,
      rti1516e::OrderType                      sentOrderType,
      rti1516e::TransportationType             transportationType,
      rti1516e::SupplementalReceiveInfo        receiveInfo ) throw( rti1516e::FederateInternalError );

   virtual void
   receiveInteraction(
      rti1516e::InteractionClassHandle         interactionClass,
      rti1516e::ParameterHandleValueMap const &parameterValues,
      rti1516e::VariableLengthData const      &userSuppliedTag,
      rti1516e::OrderType                      sentOrderType,
      rti1516e::TransportationType             transportationType,
      rti1516e::LogicalTime const             &time,
      rti1516e::OrderType                      receivedOrderType,
      rti1516e::SupplementalReceiveInfo        receiveInfo ) throw( rti1516e::FederateInternalError );

   virtual void
   receiveInteraction(
      rti1516e::InteractionClassHandle         interactionClass,
      rti1516e::ParameterHandleValueMap const &parameterValues,
      rti1516e::VariableLengthData const      &userSuppliedTag,
      rti1516e::OrderType                      sentOrderType,
      rti1516e::TransportationType             transportationType,
      rti1516e::LogicalTime const             &time,
      rti1516e::OrderType                      receivedOrderType,
      rti1516e::MessageRetractionHandle        optionalRetraction,
      rti1516e::SupplementalReceiveInfo        receiveInfo ) throw( rti1516e::FederateInternalError );

   // 6.11
   virtual void
   removeObjectInstance(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::VariableLengthData const &userSuppliedTag,
      rti1516e::OrderType                 sentOrderType,
      rti1516e::SupplementalRemoveInfo    removeInfo ) throw( rti1516e::FederateInternalError );

   virtual void
   removeObjectInstance(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::VariableLengthData const &userSuppliedTag,
      rti1516e::OrderType                 sentOrderType,
      rti1516e::LogicalTime const        &time,
      rti1516e::OrderType                 receivedOrderType,
      rti1516e::SupplementalRemoveInfo    removeInfo ) throw( rti1516e::FederateInternalError );

   virtual void
   removeObjectInstance(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::VariableLengthData const &userSuppliedTag,
      rti1516e::OrderType                 sentOrderType,
      rti1516e::LogicalTime const        &time,
      rti1516e::OrderType                 receivedOrderType,
      rti1516e::MessageRetractionHandle   optionalRetraction,
      rti1516e::SupplementalRemoveInfo    removeInfo ) throw( rti1516e::FederateInternalError );

   // 6.15
   virtual void
   attributesInScope(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes ) throw( rti1516e::FederateInternalError );

   // 6.16
   virtual void
   attributesOutOfScope(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes ) throw( rti1516e::FederateInternalError );

   // 6.18
   virtual void
   provideAttributeValueUpdate(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes,
      rti1516e::VariableLengthData const &userSuppliedTag ) throw( rti1516e::FederateInternalError );

   // 6.19
   virtual void
   turnUpdatesOnForObjectInstance(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes ) throw( rti1516e::FederateInternalError );

   virtual void
   turnUpdatesOnForObjectInstance(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes,
      std::wstring const                 &updateRateDesignator ) throw( rti1516e::FederateInternalError );

   // 6.20
   virtual void
   turnUpdatesOffForObjectInstance(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes ) throw( rti1516e::FederateInternalError );

   // 6.24
   virtual void
   confirmAttributeTransportationTypeChange(
      rti1516e::ObjectInstanceHandle objectInstance,
      rti1516e::AttributeHandleSet   attributes,
      rti1516e::TransportationType   transportationType ) throw( rti1516e::FederateInternalError );

   // 6.26
   virtual void
   reportAttributeTransportationType(
      rti1516e::ObjectInstanceHandle objectInstance,
      rti1516e::AttributeHandle      attribute,
      rti1516e::TransportationType   transportationType ) throw( rti1516e::FederateInternalError );
#
   // 6.28
   virtual void
   confirmInteractionTransportationTypeChange(
      rti1516e::InteractionClassHandle interactionClass,
      rti1516e::TransportationType     transportationType ) throw( rti1516e::FederateInternalError );

   // 6.30
   virtual void
   reportInteractionTransportationType(
      rti1516e::FederateHandle         federateHandle,
      rti1516e::InteractionClassHandle interactionClass,
      rti1516e::TransportationType     transportationType ) throw( rti1516e::FederateInternalError );

   ///////////////////////////////////
   // Ownership Management Services //
   ///////////////////////////////////

   // 7.4
   virtual void
   requestAttributeOwnershipAssumption(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes,
      rti1516e::VariableLengthData const &userSuppliedTag ) throw( rti1516e::FederateInternalError );

   // 7.5
   virtual void
   requestDivestitureConfirmation(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &releasedAttributes ) throw( rti1516e::FederateInternalError );

   virtual void
   attributeOwnershipAcquisitionNotification(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &securedAttributes,
      rti1516e::VariableLengthData const &userSuppliedTag ) throw( rti1516e::FederateInternalError );

   // 7.10
   virtual void
   attributeOwnershipUnavailable(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes ) throw( rti1516e::FederateInternalError );

   // 7.11
   virtual void
   requestAttributeOwnershipRelease(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes,
      rti1516e::VariableLengthData const &userSuppliedTag ) throw( rti1516e::FederateInternalError );

   // 7.15
   virtual void
   confirmAttributeOwnershipAcquisitionCancellation(
      rti1516e::ObjectInstanceHandle      objectInstance,
      rti1516e::AttributeHandleSet const &attributes ) throw( rti1516e::FederateInternalError );

   // 7.17
   virtual void
   informAttributeOwnership(
      rti1516e::ObjectInstanceHandle objectInstance,
      rti1516e::AttributeHandle      attribute,
      rti1516e::FederateHandle       owner ) throw( rti1516e::FederateInternalError );

   virtual void
   attributeIsNotOwned(
      rti1516e::ObjectInstanceHandle objectInstance,
      rti1516e::AttributeHandle      attribute ) throw( rti1516e::FederateInternalError );

   virtual void
   attributeIsOwnedByRTI(
      rti1516e::ObjectInstanceHandle objectInstance,
      rti1516e::AttributeHandle      attribute ) throw( rti1516e::FederateInternalError );

   //////////////////////////////
   // Time Management Services //
   //////////////////////////////

   // 8.3
   virtual void
   timeRegulationEnabled(
      rti1516e::LogicalTime const &time ) throw( rti1516e::FederateInternalError );

   // 8.6
   virtual void
   timeConstrainedEnabled(
      rti1516e::LogicalTime const &time ) throw( rti1516e::FederateInternalError );

   // 8.13
   virtual void
   timeAdvanceGrant(
      rti1516e::LogicalTime const &time ) throw( rti1516e::FederateInternalError );

   // 8.22
   virtual void
   requestRetraction(
      rti1516e::MessageRetractionHandle retraction ) throw( rti1516e::FederateInternalError );

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

#   pragma GCC diagnostic pop
#endif // IEEE_1516_2010

#endif // TRICKHLA_FED_AMB_HLA3_HH -- Do NOT put anything after this line.
