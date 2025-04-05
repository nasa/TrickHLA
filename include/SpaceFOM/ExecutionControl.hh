/*!
@file SpaceFOM/ExecutionControl.hh
@ingroup SpaceFOM
@brief This class provides and abstract base class as the base implementation
for managing mode transitions.

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

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Interaction.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPointManagerBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/SpaceFOM/ExecutionConfiguration.cpp}
@trick_link_dependency{../../source/SpaceFOM/ExecutionControl.cpp}
@trick_link_dependency{../../source/SpaceFOM/MTRInteractionHandler.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, SpaceFOM development.}
@revs_end

*/

#ifndef SPACEFOM_EXECUTON_CONTROL_HH
#define SPACEFOM_EXECUTON_CONTROL_HH

// System include files.
#include <cstdint>
#include <string>

// Trick include files.

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/SyncPointManagerBase.hh"
#include "TrickHLA/Types.hh"

// SpaceFOM include files.
#include "SpaceFOM/ExecutionConfiguration.hh"
#include "SpaceFOM/MTRInteractionHandler.hh"
#include "SpaceFOM/RefFrameBase.hh"
#include "SpaceFOM/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA Encoder helper includes.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

namespace SpaceFOM
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
   friend void init_attrSpaceFOM__ExecutionControl();

  public:
   bool designated_late_joiner; /**< @trick_units{--} Flag set by the user to
      indicate this federate is a designated late joiner, default is false. */

   // These are the execution control roles available to a federate.
   bool pacing; /**< @trick_units{--} Is true when this federate is
                     the "pacing". (default: false) */

   bool root_frame_pub; /**< @trick_units{--} Is true when this federate is
      the "root reference frame publisher" federate for the Multiphase
      initialization process. (default: false) */

   // The SpaceFOM uses a reference frame tree. This is the root frame.
   RefFrameBase *root_ref_frame; /**< @trick_units{--} Reference to the
                                       root reference frame object instance. */

  public:
   /*! @brief Default constructor for the SpaceFOM ExecutionControl class. */
   ExecutionControl();
   /*! @brief Initialization constructor for the ExecutionControl class.
    *  @param exec_config The associated ExecutionControl class instance. */
   explicit ExecutionControl( SpaceFOM::ExecutionConfiguration &exec_config );
   /*! @brief Destructor for the SpaceFOM ExecutionControl class. */
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
   // This is called by the TrickHLA::Federate::initialize routine.
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

   // SpaceFOM extensions.
   /*! @brief Determine the federate role in the federation execution. */
   virtual void role_determination_process();
   /*! @brief Process to join the federation execution early in initialization. */
   virtual void early_joiner_hla_init_process();
   /*! @brief Designated later joiner federate initialization process. */
   virtual void designated_late_joiner_init_process();
   /*! @brief Late joiner federate HLA initialization process. */
   virtual void late_joiner_hla_init_process();

   //
   // Execution Control support routines.
   /*! @brief Setup the ExecutionControl object Trick ref ATTRIBUTES. */
   virtual void setup_object_ref_attributes();
   /*! @brief Setup the ExecutionControl interaction Trick ref ATTRIBUTES. */
   virtual void setup_interaction_ref_attributes();
   /*! @brief Setup the ExecutionControl objects HLA RTI handles. */
   virtual void setup_object_RTI_handles();
   /*! @brief Setup the ExecutionControl interaction HLA RTI handles. */
   virtual void setup_interaction_RTI_handles();
   /*! Add initialization synchronization points to regulate startup. */
   virtual void add_initialization_sync_points();
   /*! @brief The RTI has announced the existence of a synchronization point.
    *  @param label             Sync-point label.
    *  @param user_supplied_tag Use supplied tag.*/
   virtual void sync_point_announced(
      std::wstring const     &label,
      RTI1516_USERDATA const &user_supplied_tag );

   /*! @brief Publish the ExecutionControl objects and interactions. */
   virtual void publish();
   /*! @brief Unpublish the ExecutionControl objects and interactions. */
   virtual void unpublish();
   /*! @brief Subscribe to the ExecutionControl objects and interactions. */
   virtual void subscribe();
   /*! @brief Unsubscribe the ExecutionControl objects and interactions. */
   virtual void unsubscribe();

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
   virtual bool process_mode_interaction();
   /*! @brief Get a comma separated list of interaction FOM names used.
    *  @return Comma separated list of interaction FOM names used. */
   virtual std::string get_interaction_FOM_names()
   {
      // Only have one interaction used by this execution control.
      return ( mtr_interaction != NULL ) ? mtr_interaction->get_FOM_name() : "";
   }
   /*! @brief Sets the next ExecutionControl run mode.
    *  @param exec_control Next ExecutionControl run mode. */
   virtual void set_next_execution_control_mode( TrickHLA::ExecutionControlEnum exec_control );
   /*! @brief Process changes from any received Execution Control Objects (ExCOs).
    *  @return True if mode change occurred. */
   virtual bool process_execution_control_updates();

   // SpaceFOM extensions.
   /*! @brief Process to determine the federation execution epoch and root reference frame. */
   virtual void epoch_and_root_frame_discovery_process();
   /*! @brief Waits for the synchronization of the root_frame_discovered
    * synchronization point. */
   void wait_for_root_frame_discovered_synchronization();
   /*! @brief Send a mode transition request to the Master federate.
    *  @param requested_mode Requested mode. */
   void send_MTR_interaction( MTREnum requested_mode );
   /*! @brief Check to see if a new MTR is valid.
    *  @return True if new MTR is valid. */
   virtual bool check_mode_transition_request();
   /*! @brief Process a new Mode Transition Request (MTR).
    *  @return True if new MTR is successfully processed. */
   virtual bool process_mode_transition_request();
   /*! @brief Send the root reference frame initialization data. */
   void send_init_root_ref_frame();
   /*! @brief Wait to receive the root reference frame initialization data. */
   void receive_init_root_ref_frame();
   /*! @brief Send the root reference frame data. */
   void send_root_ref_frame();
   /*! @brief Wait to receive the the root reference frame data. */
   void receive_root_ref_frame();

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
   /*! @brief Checks to see if shutdown has been commanded.
    *  @return True if shutdown has been announced, else False. */
   virtual bool check_for_shutdown();
   /*! @brief Checks to see if shutdown has been commanded and, if so, terminates the simulation.
    *  @return False if shutdown has NOT been announced. */
   virtual bool check_for_shutdown_with_termination();

   //
   // Federation freeze management functions.
   //
   /*! @brief Routine to handle going from run to freeze. */
   virtual void freeze_init();
   /*! @brief Check if a Trick freeze was commanded; if we announced freeze,
    *  tell other federates to freeze. */
   virtual void enter_freeze();
   /*! @brief Check for exit from freeze.
    *  @return True if should exit from freeze. */
   virtual bool check_freeze_exit();
   /*! @brief Routine to handle going from freeze to run; if we announced the
    * freeze, tell other federates to run. */
   virtual void exit_freeze();

   // SpaceFOM extensions.
   virtual bool set_pending_mtr( MTREnum mtr_value );
   /*! @brief Determine if the Mode Transition Request (MTR) is valid given the current mode.
    *  @return True if valid, false otherwise.
    *  @param mtr_value Mode transition request. */
   virtual bool is_mtr_valid( MTREnum mtr_value );
   /*! @brief Translate MTR into a pending execution mode transition.
    *  @param mtr_value MTR value for next execution mode. */
   virtual void set_mode_request_from_mtr( MTREnum mtr_value );

   // Role determination methods.
   // SpaceFOM Extensions.
   /*! @brief Query if this is the Pacing federate.
    *  @return True if there is the Pacing federate; False otherwise. */
   bool is_pacing() const
   {
      return this->pacing;
   }
   /*! @brief Query if this is the Root Reference Frame Publisher federate.
    *  @return True if there is the Root Reference Frame Publisher federate; False otherwise. */
   bool is_root_frame_publisher() const
   {
      return this->root_frame_pub;
   }
   /*! @brief Is this federate a designated late joiner federate.
    *  @return true if a designated later joiner federate. */
   bool is_designated_late_joiner()
   {
      return this->designated_late_joiner;
   }

   //
   // Federation save and checkpoint
   //
   // Federation save and checkpoint
   /*! @brief Start the Federation save at the specified scenario time.
    *  @param freeze_scenario_time Scenario time to freeze.
    *  @param file_name            Checkpoint file name. */
   virtual void start_federation_save_at_scenario_time( double freeze_scenario_time, char const *file_name );

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

  protected:
   static std::string const type; ///< @trick_units{--} ExecutionControl type string.

   MTREnum pending_mtr; ///< @trick_units{--} Pending Mode Transition Requested.

   TrickHLA::Interaction *mtr_interaction;         ///< @trick_units{--} SpaceFOM Mode Transition Request (MTR) interaction.
   MTRInteractionHandler  mtr_interaction_handler; ///< @trick_units{--} SpaceFOM MTR interaction handler.

   /*! @brief Return the relevant SpaceFOM::ExecutionConfiguration object.
    *  @return Pointer to the relevant SpaceFOM::ExecutionConfiguration object. */
   ExecutionConfiguration *get_execution_configuration();

  private:
   // Do not allow the copy constructor.
   /*! @brief Copy constructor for ExecutionControl class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ExecutionControl( ExecutionControl const &rhs );
   /*! @brief Assignment operator for ExecutionControl class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ExecutionControl &operator=( ExecutionControl const &rhs );

   /*! Print clock summary debug info. */
   void print_clock_summary( std::string const &msg );
};

} // namespace SpaceFOM

#endif /* SPACEFOM_EXECUTON_CONTROL_HH */
