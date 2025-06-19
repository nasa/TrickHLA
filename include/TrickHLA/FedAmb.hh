/*!
@file TrickHLA/FedAmb.hh
@ingroup TrickHLA
@brief Provides methods called by the HLA RTI Ambassador.

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
@revs_end

*/

#ifndef TRICKHLA_FED_AMB_HH
#define TRICKHLA_FED_AMB_HH

// System includes.
#include <set>
#include <string>

// TrickHLA inlcudes.
#include "StandardsSupport.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"

// HLA include files.
#include RTI1516_HEADER
#include "RTI/Enums.h"
#include "RTI/FederateAmbassador.h"
#include "RTI/Typedefs.h"

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
   virtual ~FedAmb() throw();

  public:
   Manager *get_manager()
   {
      return this->manager;
   }

  public:
   /*! @brief Setup the required class instance associations.
    *  @param federate  Associated TrickHLA::Federate class instance.
    *  @param manager   Associated TrickHLA::Manager class instance. */
   void setup( Federate &federate,
               Manager  &manager );

   /*! @brief Initialize the TrickHLA Federate Ambassador instance for this
    *  Federation Execution. */
   virtual void initialize();

   // *****************************************************************
   // The following methods fill out the required vitual methods of
   // the RTI1516_NAMESPACE::FederateAmbassador abstract class to enable the
   // FedAmb class to be instantiated. While this class is
   // instantiable, for almost all practical applications, a
   // simulation developer will want to overload the necessary
   // attribute and interaction callback functions to make things
   // work properly for their particular federation needs.
   // *****************************************************************

   ////////////////////////////////////
   // Federation Management Services //
   ////////////////////////////////////

   // 4.4
   virtual void connectionLost( std::wstring const &faultDescription ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.8
   virtual void reportFederationExecutions(
      RTI1516_NAMESPACE::FederationExecutionInformationVector const &
         theFederationExecutionInformationList ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.7
   virtual void
   synchronizationPointRegistrationSucceeded( std::wstring const &label ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   synchronizationPointRegistrationFailed( std::wstring const                                  &label,
                                           RTI1516_NAMESPACE::SynchronizationPointFailureReason reason ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.8
   virtual void
   announceSynchronizationPoint( std::wstring const                          &label,
                                 RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.10
   virtual void
   federationSynchronized( std::wstring const                         &label,
                           RTI1516_NAMESPACE::FederateHandleSet const &failedToSyncSet ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.12
   virtual void
   initiateFederateSave( std::wstring const &label ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   initiateFederateSave( std::wstring const                   &label,
                         RTI1516_NAMESPACE::LogicalTime const &theTime ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.15
   virtual void
   federationSaved() throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   federationNotSaved( RTI1516_NAMESPACE::SaveFailureReason theSaveFailureReason ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.17
   virtual void
   federationSaveStatusResponse(
      RTI1516_NAMESPACE::FederateHandleSaveStatusPairVector const &
         theFederateStatusVector ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.19
   virtual void
   requestFederationRestoreSucceeded( std::wstring const &label ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   requestFederationRestoreFailed( std::wstring const &label ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.20
   virtual void
   federationRestoreBegun() throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.21
   virtual void
   initiateFederateRestore( std::wstring const               &label,
                            std::wstring const               &federateName,
                            RTI1516_NAMESPACE::FederateHandle handle ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.23
   virtual void
   federationRestored() throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   federationNotRestored( RTI1516_NAMESPACE::RestoreFailureReason theRestoreFailureReason ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 4.25
   virtual void
   federationRestoreStatusResponse(
      RTI1516_NAMESPACE::FederateRestoreStatusVector const &
         theFederateRestoreStatusVector ) throw( RTI1516_NAMESPACE::FederateInternalError );

   /////////////////////////////////////
   // Declaration Management Services //
   /////////////////////////////////////

   // 5.10
   virtual void
   startRegistrationForObjectClass( RTI1516_NAMESPACE::ObjectClassHandle theClass ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 5.11
   virtual void
   stopRegistrationForObjectClass( RTI1516_NAMESPACE::ObjectClassHandle theClass ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 5.12
   virtual void
   turnInteractionsOn( RTI1516_NAMESPACE::InteractionClassHandle theHandle ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 5.13
   virtual void
   turnInteractionsOff( RTI1516_NAMESPACE::InteractionClassHandle theHandle ) throw( RTI1516_NAMESPACE::FederateInternalError );

   ////////////////////////////////
   // Object Management Services //
   ////////////////////////////////

   // 6.3
   virtual void
   objectInstanceNameReservationSucceeded( std::wstring const &
                                              theObjectInstanceName ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   objectInstanceNameReservationFailed( std::wstring const &
                                           theObjectInstanceName ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.6
   virtual void
   multipleObjectInstanceNameReservationSucceeded(
      std::set< std::wstring > const &theObjectInstanceNames ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   multipleObjectInstanceNameReservationFailed(
      std::set< std::wstring > const &theObjectInstanceNames ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.5
   virtual void
   discoverObjectInstance(
      RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
      RTI1516_NAMESPACE::ObjectClassHandle    theObjectClass,
      std::wstring const                     &theObjectInstanceName ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   discoverObjectInstance( RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
                           RTI1516_NAMESPACE::ObjectClassHandle    theObjectClass,
                           std::wstring const                     &theObjectInstanceName,
                           RTI1516_NAMESPACE::FederateHandle       producingFederate ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.7
   virtual void
   reflectAttributeValues( RTI1516_NAMESPACE::ObjectInstanceHandle           theObject,
                           RTI1516_NAMESPACE::AttributeHandleValueMap const &theAttributeValues,
                           RTI1516_NAMESPACE::VariableLengthData const      &theUserSuppliedTag,
                           RTI1516_NAMESPACE::OrderType                      sentOrder,
                           RTI1516_NAMESPACE::TransportationType             theType,
                           RTI1516_NAMESPACE::SupplementalReflectInfo        theReflectInfo ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   reflectAttributeValues( RTI1516_NAMESPACE::ObjectInstanceHandle           theObject,
                           RTI1516_NAMESPACE::AttributeHandleValueMap const &theAttributeValues,
                           RTI1516_NAMESPACE::VariableLengthData const      &theUserSuppliedTag,
                           RTI1516_NAMESPACE::OrderType                      sentOrder,
                           RTI1516_NAMESPACE::TransportationType             theType,
                           RTI1516_NAMESPACE::LogicalTime const             &theTime,
                           RTI1516_NAMESPACE::OrderType                      receivedOrder,
                           RTI1516_NAMESPACE::SupplementalReflectInfo        theReflectInfo ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   reflectAttributeValues( RTI1516_NAMESPACE::ObjectInstanceHandle           theObject,
                           RTI1516_NAMESPACE::AttributeHandleValueMap const &theAttributeValues,
                           RTI1516_NAMESPACE::VariableLengthData const      &theUserSuppliedTag,
                           RTI1516_NAMESPACE::OrderType                      sentOrder,
                           RTI1516_NAMESPACE::TransportationType             theType,
                           RTI1516_NAMESPACE::LogicalTime const             &theTime,
                           RTI1516_NAMESPACE::OrderType                      receivedOrder,
                           RTI1516_NAMESPACE::MessageRetractionHandle        theHandle,
                           RTI1516_NAMESPACE::SupplementalReflectInfo        theReflectInfo ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.9
   virtual void
   receiveInteraction( RTI1516_NAMESPACE::InteractionClassHandle         theInteraction,
                       RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
                       RTI1516_NAMESPACE::VariableLengthData const      &theUserSuppliedTag,
                       RTI1516_NAMESPACE::OrderType                      sentOrder,
                       RTI1516_NAMESPACE::TransportationType             theType,
                       RTI1516_NAMESPACE::SupplementalReceiveInfo        theReceiveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   receiveInteraction( RTI1516_NAMESPACE::InteractionClassHandle         theInteraction,
                       RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
                       RTI1516_NAMESPACE::VariableLengthData const      &theUserSuppliedTag,
                       RTI1516_NAMESPACE::OrderType                      sentOrder,
                       RTI1516_NAMESPACE::TransportationType             theType,
                       RTI1516_NAMESPACE::LogicalTime const             &theTime,
                       RTI1516_NAMESPACE::OrderType                      receivedOrder,
                       RTI1516_NAMESPACE::SupplementalReceiveInfo        theReceiveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   receiveInteraction( RTI1516_NAMESPACE::InteractionClassHandle         theInteraction,
                       RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
                       RTI1516_NAMESPACE::VariableLengthData const      &theUserSuppliedTag,
                       RTI1516_NAMESPACE::OrderType                      sentOrder,
                       RTI1516_NAMESPACE::TransportationType             theType,
                       RTI1516_NAMESPACE::LogicalTime const             &theTime,
                       RTI1516_NAMESPACE::OrderType                      receivedOrder,
                       RTI1516_NAMESPACE::MessageRetractionHandle        theHandle,
                       RTI1516_NAMESPACE::SupplementalReceiveInfo        theReceiveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.11
   virtual void
   removeObjectInstance( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                         RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag,
                         RTI1516_NAMESPACE::OrderType                 sentOrder,
                         RTI1516_NAMESPACE::SupplementalRemoveInfo    theRemoveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   removeObjectInstance( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                         RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag,
                         RTI1516_NAMESPACE::OrderType                 sentOrder,
                         RTI1516_NAMESPACE::LogicalTime const        &theTime,
                         RTI1516_NAMESPACE::OrderType                 receivedOrder,
                         RTI1516_NAMESPACE::SupplementalRemoveInfo    theRemoveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   removeObjectInstance( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                         RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag,
                         RTI1516_NAMESPACE::OrderType                 sentOrder,
                         RTI1516_NAMESPACE::LogicalTime const        &theTime,
                         RTI1516_NAMESPACE::OrderType                 receivedOrder,
                         RTI1516_NAMESPACE::MessageRetractionHandle   theHandle,
                         RTI1516_NAMESPACE::SupplementalRemoveInfo    theRemoveInfo ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.15
   virtual void
   attributesInScope( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                      RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.16
   virtual void
   attributesOutOfScope( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                         RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.18
   virtual void
   provideAttributeValueUpdate( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes,
                                RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.19
   virtual void
   turnUpdatesOnForObjectInstance( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                   RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   turnUpdatesOnForObjectInstance( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                   RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes,
                                   std::wstring const                          &updateRateDesignator ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.20
   virtual void
   turnUpdatesOffForObjectInstance( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                    RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.24
   virtual void
   confirmAttributeTransportationTypeChange( RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
                                             RTI1516_NAMESPACE::AttributeHandleSet   theAttributes,
                                             RTI1516_NAMESPACE::TransportationType   theTransportation ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.26
   virtual void
   reportAttributeTransportationType( RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
                                      RTI1516_NAMESPACE::AttributeHandle      theAttribute,
                                      RTI1516_NAMESPACE::TransportationType   theTransportation ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.28
   virtual void
   confirmInteractionTransportationTypeChange( RTI1516_NAMESPACE::InteractionClassHandle theInteraction,
                                               RTI1516_NAMESPACE::TransportationType     theTransportation ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 6.30
   virtual void
   reportInteractionTransportationType( RTI1516_NAMESPACE::FederateHandle         federateHandle,
                                        RTI1516_NAMESPACE::InteractionClassHandle theInteraction,
                                        RTI1516_NAMESPACE::TransportationType     theTransportation ) throw( RTI1516_NAMESPACE::FederateInternalError );

   ///////////////////////////////////
   // Ownership Management Services //
   ///////////////////////////////////

   // 7.4
   virtual void
   requestAttributeOwnershipAssumption( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                        RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes,
                                        RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 7.5
   virtual void
   requestDivestitureConfirmation( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                   RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 7.7
   virtual void
   attributeOwnershipAcquisitionNotification( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                              RTI1516_NAMESPACE::AttributeHandleSet const &securedAttributes,
                                              RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 7.10
   virtual void
   attributeOwnershipUnavailable( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                  RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 7.11
   virtual void
   requestAttributeOwnershipRelease( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                     RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes,
                                     RTI1516_NAMESPACE::VariableLengthData const &theUserSuppliedTag ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 7.15
   virtual void
   confirmAttributeOwnershipAcquisitionCancellation( RTI1516_NAMESPACE::ObjectInstanceHandle      theObject,
                                                     RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 7.17
   virtual void
   informAttributeOwnership( RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
                             RTI1516_NAMESPACE::AttributeHandle      theAttribute,
                             RTI1516_NAMESPACE::FederateHandle       theOwner ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   attributeIsNotOwned( RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
                        RTI1516_NAMESPACE::AttributeHandle      theAttribute ) throw( RTI1516_NAMESPACE::FederateInternalError );

   virtual void
   attributeIsOwnedByRTI( RTI1516_NAMESPACE::ObjectInstanceHandle theObject,
                          RTI1516_NAMESPACE::AttributeHandle      theAttribute ) throw( RTI1516_NAMESPACE::FederateInternalError );

   //////////////////////////////
   // Time Management Services //
   //////////////////////////////

   // 8.3
   virtual void
   timeRegulationEnabled( RTI1516_NAMESPACE::LogicalTime const &theFederateTime ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 8.6
   virtual void
   timeConstrainedEnabled( RTI1516_NAMESPACE::LogicalTime const &theFederateTime ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 8.13
   virtual void
   timeAdvanceGrant( RTI1516_NAMESPACE::LogicalTime const &theTime ) throw( RTI1516_NAMESPACE::FederateInternalError );

   // 8.22
   virtual void
   requestRetraction( RTI1516_NAMESPACE::MessageRetractionHandle theHandle ) throw( RTI1516_NAMESPACE::FederateInternalError );

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

#pragma GCC diagnostic pop

#endif // TRICKHLA_FED_AMB_HH -- Do NOT put anything after this line.
