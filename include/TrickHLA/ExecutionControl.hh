/*!
@file TrickHLA/ExecutionControl.hh
@ingroup TrickHLA
@brief This class provides and abstract base class as the base implementation
for TrickHLA simple execution control.

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
@trick_link_dependency{../../source/TrickHLA/ExecutionControl.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionConfiguration.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, TrickHLA development.}
@revs_end

*/

#ifndef TRICKHLA_EXECUTON_CONTROL_HH
#define TRICKHLA_EXECUTON_CONTROL_HH

// System includes.
#include <cstdint>
#include <string>

// TrickHLA include files.
#include "TrickHLA/ExecutionConfiguration.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA Encoder helper includes.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

namespace TrickHLA
{

class ExecutionControl : public TrickHLA::ExecutionControlBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ExecutionControl();

  public:
   /*! @brief Default constructor for the TrickHLA ExecutionControl class. */
   ExecutionControl();
   /*! @brief Initialization constructor for the ExecutionControl class.
    *  @param exec_config The associated ExecutionControl class instance. */
   explicit ExecutionControl( TrickHLA::ExecutionConfiguration &exec_config );
   /*! @brief Destructor for the TrickHLA ExecutionControl class. */
   virtual ~ExecutionControl();

  public:
   /*! @brief Get the ExecutionControl type identification string.
    *  @return A constant reference to the type identification string.
    */
   virtual std::string const &get_type()
   {
      return ( type );
   }

   //
   // Execution Control initialization routines.
   /*! @brief Execution Control initialization routine. */
   virtual void initialize();
   /*! @brief Join federation execution process. */
   virtual void join_federation_process(); // cppcheck-suppress [uselessOverride]
   /*! @brief Process run before the multi-phase initialization begins. */
   virtual void pre_multi_phase_init_processes();
   /*! @brief Process run after the multi-phase initialization ends. */
   virtual void post_multi_phase_init_processes();
   /*! @brief Execution control specific shutdown process. */
   virtual void shutdown();

   //
   // Execution Control support routines.
   /*! Setup the ExecutionControl object Trick ref ATTRIBUTES. */
   virtual void setup_object_ref_attributes();
   /*! Setup the ExecutionControl interaction Trick ref ATTRIBUTES. */
   virtual void setup_interaction_ref_attributes();
   /*! Setup the ExecutionControl objects HLA RTI handles. */
   virtual void setup_object_RTI_handles();
   /*! Setup the ExecutionControl interaction HLA RTI handles. */
   virtual void setup_interaction_RTI_handles();
   /*! Add initialization synchronization points to regulate startup. */
   virtual void add_initialization_sync_points();
   /*! Add multiphase initialization synchronization points to regulate startup. */
   virtual void add_multiphase_init_sync_points();
   /*! Clear any remaining multiphase initialization synchronization points
    *  that have not been achieved and wait for the federation to be
    *  synchronized on it. */
   virtual void clear_multiphase_init_sync_points();
   /*! @brief The RTI has announced the existence of a synchronization point.
    *  @param label             Sync-point label.
    *  @param user_supplied_tag Use supplied tag.*/
   virtual void sync_point_announced( // cppcheck-suppress [uselessOverride]
      std::wstring const     &label,
      RTI1516_USERDATA const &user_supplied_tag );

   /*! Publish the ExecutionControl objects and interactions. */
   virtual void publish();
   /*! Unpublish the ExecutionControl objects and interactions. */
   virtual void unpublish();
   /*! Subscribe to the ExecutionControl objects and interactions. */
   virtual void subscribe();
   /*! Unsubscribe the ExecutionControl objects and interactions. */
   virtual void unsubscribe();
   /*! @brief Test to see if ExecutionControl needs to wait for initialization data.
    *  @details Most ExecutionControl approaches require that we wait for the
    *  required initialization data. Currently, only the 'Simple' scheme does not.
    *  @return True if ExecutionControl needs to wait on the initialization data. */
   bool wait_for_init_data()
   {
      return ( false );
   }
   /*! @brief Test to see if ExecutionControl needs to wait for the initialization
    *  synchronization point.
    *  @details Most ExecutionControl approaches require that we wait for
    *  specific initialization synchronization points in sprecific orders.
    *  Currently, only the 'Simple' and 'DIS' scheme do not.
    *  @return True if ExecutionControl needs to wait on the initialization synchronization points. */
   bool is_wait_for_init_sync_point_supported()
   {
      return ( false );
   }

   //
   // ExecutionControl runtime routines.
   /*! @brief Process all received interactions by calling in turn each
    * interaction handler that is subscribed to the interaction.
    * @param theInteraction     Interaction handle.
    * @param theParameterValues Parameter values.
    * @param theUserSuppliedTag Users tag.
    * @param theTime            HLA time for the interaction.
    * @param received_as_TSO    True if interaction was received by RTI as TSO. */
   virtual bool receive_interaction(
      RTI1516_NAMESPACE::InteractionClassHandle const  &theInteraction,
      RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
      RTI1516_USERDATA const                           &theUserSuppliedTag,
      RTI1516_NAMESPACE::LogicalTime const             &theTime,
      bool const                                        received_as_TSO );

   /*! @brief Send a mode transition request to the Master federate.
    *  @param requested_mode Requested mode. */
   virtual void send_mode_transition_interaction( TrickHLA::ModeTransitionEnum requested_mode );
   /*! @brief Process a new mode interaction.
    *  @return True if new mode interaction is successfully processed. */
   virtual bool process_mode_interaction()
   {
      return true;
   };
   /*! @brief Get a comma separated list of interaction FOM names used.
    *  @return Comma separated list of interaction FOM names used. */
   virtual std::string get_interaction_FOM_names()
   {
      // No interactions used by this execution control.
      return "";
   }
   /*! @brief Sets the next ExecutionControl run mode.
    *  @param exec_control Next ExecutionControl run mode. */
   virtual void set_next_execution_control_mode( TrickHLA::ExecutionControlEnum exec_control );
   /*! @brief Process changes from any received Execution Control Objects (ExCOs).
    *  @return True if mode change occurred. */
   virtual bool process_execution_control_updates();

   //
   // Mode management support routines.
   /*! @brief The run mode transition routine.
    *  @return Currently always returns True. */
   virtual bool run_mode_transition();
   /*! @brief Announce the pending freeze mode transition with an 'mtr_freeze' sync-point. */
   virtual void freeze_mode_announce();
   /*! @brief The freeze mode transition routine.
    *  @return Currently always returns False. */
   virtual bool freeze_mode_transition();
   /*! @brief Announce to the federation execution that a shutdown is occurring. */
   virtual void shutdown_mode_announce();
   /*! @brief The shutdown mode transition routine. */
   virtual void shutdown_mode_transition();

   //
   // Freeze time management functions.
   /*! @brief Set the least common time step in seconds for the federation.
    *  @param lcts Least Common Time Step time in seconds. */
   virtual void set_least_common_time_step( double const lcts );

   /*! @brief Refresh the least common time step especially if the HLA base time units changed. */
   virtual void refresh_least_common_time_step();

   /*! @brief Set the time-padding used to offset the go to run time.
    *  @param t Time in seconds to pad for time based mode transitions. */
   virtual void set_time_padding( double t );

   //
   // Federation save and checkpoint
   /*! @brief Start the Federation save at the specified scenario time.
    *  @param freeze_scenario_time Scenario time to freeze.
    *  @param file_name            Checkpoint file name. */
   virtual void start_federation_save_at_scenario_time( double      freeze_scenario_time,
                                                        char const *file_name );

  protected:
   static std::string const type; ///< @trick_units{--} ExecutionControl type string.

   /*! @brief Return the relevant TrickHLA::ExecutionConfiguration object.
    *  @return Pointer to the relevant TrickHLA::ExecutionConfiguration object. */
   ExecutionConfiguration *get_execution_configuration();

  private:
   // Do not allow the copy constructor.
   /*! @brief Copy constructor for ExecutionControl class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ExecutionControl( ExecutionControl const &rhs );
   /*! @brief Assignment operator for ExecutionControl class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ExecutionControl &operator=( ExecutionControl const &rhs );
};

} // namespace TrickHLA

#endif /* TRICKHLA_EXECUTON_CONTROL_HH */
