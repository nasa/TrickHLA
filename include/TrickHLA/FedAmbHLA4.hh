/*!
@file TrickHLA/FedAmbHLA4.hh
@ingroup TrickHLA
@brief Provides methods called by the HLA RTI Ambassador for IEEE 1516-2025.

This class is essentially a polymorphic callback class provided to the
RTI Ambassador. It provides methods called by the RTI Ambassador for
simulation object, interaction and time management.

The methods in this class fill out the required virtual methods of the
RTI1516_NAMESPACE::FederateAmbassador abstract class to enable the FedAmb class
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

// TrickHLA inlcudes.
#include "StandardsSupport.hh"

// HLA include files.
#include "RTI/Enums.h"
#include "RTI/FederateAmbassador.h"
#include "RTI/Handle.h"
#include "RTI/RTI1516.h"
#include "RTI/Typedefs.h"
#include "RTI/time/LogicalTime.h"

#if !defined( IEEE_1516_2025 )
#   error This FedAmb header file is only valid for IEEE 1516-2025!
#endif

namespace TrickHLA
{

// Forward Declared Classes: Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Federate;
class Manager;

class FedAmb : public RTI1516_NAMESPACE::FederateAmbassador
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

  protected:
   Federate *federate; ///< @trick_units{--} Associated TrickHLA::Federate.
   Manager  *manager;  ///< @trick_units{--} Associated TrickHLA::Manager.

  public:
   /*! @brief Default constructor for the TrickHLA FedAmb class. */
   FedAmb();
   /*! @brief Destructor for the TrickHLA FedAmb class. */
   virtual ~FedAmb();

   Manager *get_manager()
   {
      return this->manager;
   }

   /*! @brief Setup the required class instance associations.
    *  @param federate  Associated TrickHLA::Federate class instance.
    *  @param manager   Associated TrickHLA::Manager class instance. */
   void setup( Federate &federate,
               Manager  &manager );

   /*! @brief Initialize the TrickHLA Federate Ambassador instance for this
    *  Federation Execution. */
   virtual void initialize();

   // *****************************************************************
   // The following methods fill out the required virtual methods of
   // the RTI1516_NAMESPACE::FederateAmbassador abstract class to
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
      RTI1516_NAMESPACE::FederationExecutionInformationVector const &report );

   // 4.10
   virtual void reportFederationExecutionMembers(
      std::wstring const                                                  &federationName,
      RTI1516_NAMESPACE::FederationExecutionMemberInformationVector const &report );

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
      std::wstring const                                  &label,
      RTI1516_NAMESPACE::SynchronizationPointFailureReason reason );

   // 4.16
   virtual void announceSynchronizationPoint(
      std::wstring const                          &label,
      RTI1516_NAMESPACE::VariableLengthData const &userSuppliedTag );

   // 4.18
   virtual void federationSynchronized(
      std::wstring const                         &label,
      RTI1516_NAMESPACE::FederateHandleSet const &failedToSyncSet );

   // 4.20
   virtual void initiateFederateSave(
      std::wstring const &label );

   // 4.20
   virtual void initiateFederateSave(
      std::wstring const                   &label,
      RTI1516_NAMESPACE::LogicalTime const &time );

   // 4.23
   virtual void federationSaved();

   // 4.23
   virtual void federationNotSaved(
      RTI1516_NAMESPACE::SaveFailureReason reason );

   // 4.26
   virtual void federationSaveStatusResponse(
      RTI1516_NAMESPACE::FederateHandleSaveStatusPairVector const &response );

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
      std::wstring const                      &label,
      std::wstring const                      &federateName,
      RTI1516_NAMESPACE::FederateHandle const &postRestoreFederateHandle );

   // 4.32
   virtual void federationRestored();

   // 4.32
   virtual void federationNotRestored(
      RTI1516_NAMESPACE::RestoreFailureReason reason );

   // 4.35
   virtual void federationRestoreStatusResponse(
      RTI1516_NAMESPACE::FederateRestoreStatusVector const &response );

   /////////////////////////////////////
   // Declaration Management Services //
   /////////////////////////////////////

   // 5.14
   virtual void startRegistrationForObjectClass(
      RTI1516_NAMESPACE::ObjectClassHandle const &objectClass );

   // 5.15
   virtual void stopRegistrationForObjectClass(
      RTI1516_NAMESPACE::ObjectClassHandle const &objectClass );

   // 5.16
   virtual void turnInteractionsOn(
      RTI1516_NAMESPACE::InteractionClassHandle const &interactionClass );

   // 5.17
   virtual void turnInteractionsOff(
      RTI1516_NAMESPACE::InteractionClassHandle const &interactionClass );

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
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::ObjectClassHandle const    &objectClass,
      std::wstring const                            &objectInstanceName,
      RTI1516_NAMESPACE::FederateHandle const       &producingFederate );

   // 6.11
   virtual void reflectAttributeValues(
      RTI1516_NAMESPACE::ObjectInstanceHandle const     &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleValueMap const  &attributeValues,
      RTI1516_NAMESPACE::VariableLengthData const       &userSuppliedTag,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType,
      RTI1516_NAMESPACE::FederateHandle const           &producingFederate,
      RTI1516_NAMESPACE::RegionHandleSet const          *optionalSentRegions );

   // 6.11
   virtual void reflectAttributeValues(
      RTI1516_NAMESPACE::ObjectInstanceHandle const     &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleValueMap const  &attributeValues,
      RTI1516_NAMESPACE::VariableLengthData const       &userSuppliedTag,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType,
      RTI1516_NAMESPACE::FederateHandle const           &producingFederate,
      RTI1516_NAMESPACE::RegionHandleSet const          *optionalSentRegions,
      RTI1516_NAMESPACE::LogicalTime const              &time,
      RTI1516_NAMESPACE::OrderType                       sentOrderType,
      RTI1516_NAMESPACE::OrderType                       receivedOrderType,
      RTI1516_NAMESPACE::MessageRetractionHandle const  *optionalRetraction );

   // 6.13
   virtual void receiveInteraction(
      RTI1516_NAMESPACE::InteractionClassHandle const   &interactionClass,
      RTI1516_NAMESPACE::ParameterHandleValueMap const  &parameterValues,
      RTI1516_NAMESPACE::VariableLengthData const       &userSuppliedTag,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType,
      RTI1516_NAMESPACE::FederateHandle const           &producingFederate,
      RTI1516_NAMESPACE::RegionHandleSet const          *optionalSentRegions );

   // 6.13
   virtual void receiveInteraction(
      RTI1516_NAMESPACE::InteractionClassHandle const   &interactionClass,
      RTI1516_NAMESPACE::ParameterHandleValueMap const  &parameterValues,
      RTI1516_NAMESPACE::VariableLengthData const       &userSuppliedTag,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType,
      RTI1516_NAMESPACE::FederateHandle const           &producingFederate,
      RTI1516_NAMESPACE::RegionHandleSet const          *optionalSentRegions,
      RTI1516_NAMESPACE::LogicalTime const              &time,
      RTI1516_NAMESPACE::OrderType                       sentOrderType,
      RTI1516_NAMESPACE::OrderType                       receivedOrderType,
      RTI1516_NAMESPACE::MessageRetractionHandle const  *optionalRetraction );

   // 6.15
   virtual void receiveDirectedInteraction(
      RTI1516_NAMESPACE::InteractionClassHandle const   &interactionClass,
      RTI1516_NAMESPACE::ObjectInstanceHandle const     &objectInstance,
      RTI1516_NAMESPACE::ParameterHandleValueMap const  &parameterValues,
      RTI1516_NAMESPACE::VariableLengthData const       &userSuppliedTag,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType,
      RTI1516_NAMESPACE::FederateHandle const           &producingFederate );

   // 6.15
   virtual void receiveDirectedInteraction(
      RTI1516_NAMESPACE::InteractionClassHandle const   &interactionClass,
      RTI1516_NAMESPACE::ObjectInstanceHandle const     &objectInstance,
      RTI1516_NAMESPACE::ParameterHandleValueMap const  &parameterValues,
      RTI1516_NAMESPACE::VariableLengthData const       &userSuppliedTag,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType,
      RTI1516_NAMESPACE::FederateHandle const           &producingFederate,
      RTI1516_NAMESPACE::LogicalTime const              &time,
      RTI1516_NAMESPACE::OrderType                       sentOrderType,
      RTI1516_NAMESPACE::OrderType                       receivedOrderType,
      RTI1516_NAMESPACE::MessageRetractionHandle const  *optionalRetraction );

   // 6.17
   virtual void removeObjectInstance(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::VariableLengthData const   &userSuppliedTag,
      RTI1516_NAMESPACE::FederateHandle const       &producingFederate );

   // 6.17
   virtual void removeObjectInstance(
      RTI1516_NAMESPACE::ObjectInstanceHandle const    &objectInstance,
      RTI1516_NAMESPACE::VariableLengthData const      &userSuppliedTag,
      RTI1516_NAMESPACE::FederateHandle const          &producingFederate,
      RTI1516_NAMESPACE::LogicalTime const             &time,
      RTI1516_NAMESPACE::OrderType                      sentOrderType,
      RTI1516_NAMESPACE::OrderType                      receivedOrderType,
      RTI1516_NAMESPACE::MessageRetractionHandle const *optionalRetraction );

   // 6.19
   virtual void attributesInScope(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes );

   // 6.20
   virtual void attributesOutOfScope(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes );

   // 6.22
   virtual void provideAttributeValueUpdate(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes,
      RTI1516_NAMESPACE::VariableLengthData const   &userSuppliedTag );

   // 6.23
   virtual void turnUpdatesOnForObjectInstance(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes );

   // 6.23
   virtual void turnUpdatesOnForObjectInstance(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes,
      std::wstring const                            &updateRateDesignator );

   // 6.24
   virtual void turnUpdatesOffForObjectInstance(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes );

   // 6.26
   virtual void confirmAttributeTransportationTypeChange(
      RTI1516_NAMESPACE::ObjectInstanceHandle const     &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const       &attributes,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType );

   // 6.29
   virtual void reportAttributeTransportationType(
      RTI1516_NAMESPACE::ObjectInstanceHandle const     &objectInstance,
      RTI1516_NAMESPACE::AttributeHandle const          &attribute,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType );

   // 6.31
   virtual void confirmInteractionTransportationTypeChange(
      RTI1516_NAMESPACE::InteractionClassHandle const   &interactionClass,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType );

   // 6.33
   virtual void reportInteractionTransportationType(
      RTI1516_NAMESPACE::FederateHandle const           &federateHandle,
      RTI1516_NAMESPACE::InteractionClassHandle const   &interactionClass,
      RTI1516_NAMESPACE::TransportationTypeHandle const &transportationType );

   ///////////////////////////////////
   // Ownership Management Services //
   ///////////////////////////////////

   // 7.4
   virtual void requestAttributeOwnershipAssumption(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &offeredAttributes,
      RTI1516_NAMESPACE::VariableLengthData const   &userSuppliedTag );

   // 7.5
   virtual void requestDivestitureConfirmation(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &releasedAttributes,
      RTI1516_NAMESPACE::VariableLengthData const   &userSuppliedTag );

   // 7.7
   virtual void attributeOwnershipAcquisitionNotification(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &securedAttributes,
      RTI1516_NAMESPACE::VariableLengthData const   &userSuppliedTag );

   // 7.10
   virtual void attributeOwnershipUnavailable(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes,
      RTI1516_NAMESPACE::VariableLengthData const   &userSuppliedTag );

   // 7.11
   virtual void requestAttributeOwnershipRelease(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &candidateAttributes,
      RTI1516_NAMESPACE::VariableLengthData const   &userSuppliedTag );

   // 7.16
   virtual void confirmAttributeOwnershipAcquisitionCancellation(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes );

   // 7.18
   virtual void informAttributeOwnership(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes,
      RTI1516_NAMESPACE::FederateHandle const       &owner );

   // 7.18
   virtual void attributeIsNotOwned(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes );

   // 7.18
   virtual void attributeIsOwnedByRTI(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &objectInstance,
      RTI1516_NAMESPACE::AttributeHandleSet const   &attributes );

   //////////////////////////////
   // Time Management Services //
   //////////////////////////////

   // 8.3
   virtual void timeRegulationEnabled(
      RTI1516_NAMESPACE::LogicalTime const &time );

   // 8.6
   virtual void timeConstrainedEnabled(
      RTI1516_NAMESPACE::LogicalTime const &time );

   // 8.13
   virtual void flushQueueGrant(
      RTI1516_NAMESPACE::LogicalTime const &time,
      RTI1516_NAMESPACE::LogicalTime const &optimisticTime );

   // 8.14
   virtual void timeAdvanceGrant(
      RTI1516_NAMESPACE::LogicalTime const &time );

   // 8.23
   virtual void requestRetraction(
      RTI1516_NAMESPACE::MessageRetractionHandle const &retraction );

   /*! @brief Switch to echo (versus process) in a federationRestoreStatusResponse() callback... */
   void set_federation_restore_status_response_to_echo()
   {
      federation_restore_status_response_context_switch = true;
   }
   /*! @brief Switch to process (versus echo) in a federationRestoreStatusResponse() callback... */
   void set_federation_restore_status_response_to_process()
   {
      federation_restore_status_response_context_switch = false;
   }

   /*! @brief Enable the option to rebuild the federate handle set after a federation restore. */
   void set_federation_restored_rebuild_federate_handle_set()
   {
      federation_restored_rebuild_federate_handle_set = true;
   }
   /*! @brief Disable the option to rebuild the federate handle set after a federation restore. */
   void reset_federation_restored_rebuild_federate_handle_set()
   {
      federation_restored_rebuild_federate_handle_set = false;
   }

  private:
   bool federation_restore_status_response_context_switch;
   bool federation_restored_rebuild_federate_handle_set;

   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for FedAmb class.
    *  @details This constructor is private to prevent inadvertent copies. */
   FedAmb( FedAmb const &rhs );
   /*! @brief Assignment operator for FedAmb class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   FedAmb &operator=( FedAmb const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_FED_AMB_HLA4_HH -- Do NOT put anything after this line.
