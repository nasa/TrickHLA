/*!
@file IMSim/ExecutionControl.hh
@ingroup IMSim
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

@python_module{IMSim}

@tldh
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/Interaction.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/IMSim/ExecutionControl.cpp}
@trick_link_dependency{../../source/IMSim/FreezeInteractionHandler.cpp}
@trick_link_dependency{../../source/IMSim/PausePointList.cpp}
@trick_link_dependency{../../source/IMSim/Typs.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, IMSim development.}
@revs_end

*/

#ifndef IMSIM_EXECUTON_CONTROL_HH
#define IMSIM_EXECUTON_CONTROL_HH

// System include files.
#include <string>

// TrickHLA include files.
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/Interaction.hh"
#include "TrickHLA/Types.hh"

// IMSim include files.Interaction.hh"
#include "IMSim/ExecutionConfiguration.hh"
#include "IMSim/FreezeInteractionHandler.hh"
#include "IMSim/PausePointList.hh"
#include "IMSim/Types.hh"

namespace IMSim
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
   friend void init_attrIMSim__ExecutionControl();

  public:
   /*! @brief Default constructor for the IMSim ExecutionControl class. */
   ExecutionControl();
   /*! @brief Destructor for the IMSim ExecutionControl class. */
   virtual ~ExecutionControl();

  public:
   /*! @brief Get the ExecutionControl type identification string.
    *  @return A constant reference to the type identification string.
    */
   virtual std::string const &get_type()
   {
      return ( type );
   }

   // Execution Control initialization routine.
   // This is called by the TrickHLA::Federate::initialize routine.
   /*! @brief Execution Control initialization routine.
    *  @param federate The associated TrickHLA::Federate. */
   virtual void initialize();
   /*! @brief Join federation execution process. */
   virtual void join_federation_process();
   /*! @brief Process run before the multi-phase initialization begins. */
   virtual void pre_multi_phase_init_processes();
   /*! @brief Process run after the multi-phase initialization ends. */
   virtual void post_multi_phase_init_process();
   /*! @brief Execution control specific shutdown process. */
   virtual void shutdown();

   // IMSim extensions to Exection Control.
   /*! @brief Determine if this federate is late in joining the federation or is
    * to restore itself. This call blocks until it has determined if the
    * federate is late or not or when its been cleared to restore.
    * @return Initialization Federate State:\n
    *         0 -- normal execution (neither late joiner nor federate restore),\n
    *         1 -- late joiner,\n
    *         2 -- federate restore. */
   TrickHLA::FederateJoinEnum determine_if_late_joining_or_restoring_federate();

   //
   // Execution Control support routines.routines.
   /*! Setup the ExecutionControl object Trick ref ATTRIBUTES. */
   virtual void setup_object_ref_attributes();
   /*! Setup the ExecutionControl interaction Trick ref ATTRIBUTES. */
   virtual void setup_interaction_ref_attributes();
   /*! Setup the ExecutionControl objects HLA RTI handles. */
   virtual void setup_object_RTI_handles();
   /*! Setup the ExecutionControl interaction HLA RTI handles. */
   virtual void setup_interaction_RTI_handles();
   /*! Add initialization synchronization points to regulate startup. */
   virtual void add_multiphase_init_sync_points();
   /*! @brief The RTI has announced the existence of a synchronization point.
    *  @param rti_ambassador    Reference to the HLA RTI Ambassador instance.
    *  @param label             Sync-point label.
    *  @param user_supplied_tag Use supplied tag.*/
   virtual void announce_sync_point(
      RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
      std::wstring const               &label,
      RTI1516_USERDATA const           &user_supplied_tag );
   /*! @brief Achieve all the user defined mulit-phase initialization
    *  synchronization points if they are not already achieved and are not
    *  one of the predefined ExecutionControl synchronization points.
    *  @param rti_ambassador Reference to the HLA RTI Ambassador instance. */
   void achieve_all_multiphase_init_sync_points( RTI1516_NAMESPACE::RTIambassador &rti_ambassador );
   /*! @brief Wait for all the user defined mulit-phase initialization
    *  synchronization points if they are not already achieved and are not
    *  one of the predefined ExecutionControl synchronization points. */
   void wait_for_all_multiphase_init_sync_points();
   /*! Publish the ExecutionControl objects and interactions. */
   virtual void publish();
   /*! Unpublish the ExecutionControl objects and interactions. */
   virtual void unpublish();
   /*! Subscribe to the ExecutionControl objects and interactions. */
   virtual void subscribe();
   /*! Unsubscribe the ExecutionControl objects and interactions. */
   virtual void unsubscribe();

   // IMSim extensions to Exection Control.
   /*! @brief Mark the given synchronization point as synchronized in the federation.
    *  @return True if synchronization point label is valid.
    *  @param label The synchronization point label. */
   virtual bool mark_synchronized( std::wstring const &label );

   //
   // ExecutionControl runtime routines.
   /*! @brief Process all received interactions by calling in turn each
    * interaction handler that is subscribed to the interaction.
    * @param theInteraction     Interaction handle.
    * @param theParameterValues Parameter values.
    * @param theUserSuppliedTag Users tag.
    * @param theTime            HLA time for the interaction.
    * @param received_as_TSO    True if interaction was received by RTI as TSO. */
   virtual void receive_interaction(
      RTI1516_NAMESPACE::InteractionClassHandle const  &theInteraction,
      RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
      RTI1516_USERDATA const                           &theUserSuppliedTag,
      RTI1516_NAMESPACE::LogicalTime const             &theTime,
      bool const                                        received_as_TSO );
   /*! @brief Send a mode transition request to the Master federate.
    *  @param requested_mode Requested mode. */
   virtual void send_mode_transition_interaction( TrickHLA::ModeTransitionEnum requested_mode );
   /*! @brief Sets the next ExecutionControl run mode.
    *  @param exec_control Next ExecutionControl run mode. */
   virtual void set_next_execution_control_mode( TrickHLA::ExecutionControlEnum exec_control );
   /*! @brief Process changes from any received Execution Control Objects (ExCOs).
    *  @return True if mode change occurred. */
   virtual bool process_execution_control_updates();

   // Mode transition routines.
   /*! @brief Check to see if a new MTR is valid.
    *  @return True if new MTR is valid. */
   virtual bool check_mode_transition_request();
   /*! @brief Process a new mode interaction.
    *  @return True if new mode interaction is successfully processed. */
   virtual bool process_mode_interaction();
   /*! @brief Process a new Mode Transition Request (MTR).
    *  @return True if new MTR is successfully processed. */
   virtual bool process_mode_transition_request();
   /*! @brief Clear the Mode Transition Request flag, the requested execution
    * mode, and the current execution mode. */
   virtual void clear_mode_values();
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
   // Federation freeze/pause management functions.
   //
   /*! @brief Check if a Trick freeze was commanded; if we announced freeze,
    *  tell other federates to freeze. */
   virtual void enter_freeze();
   /*! @brief Check for exit from freeze.
    *  @return True if should exit from freeze. */
   virtual virtual bool check_freeze_exit();
   /*! @brief Routine to handle going from freeze to run; if we announced the
    * freeze, tell other federates to run. */
   virtual void exit_freeze();
   /*! @brief Routine to handle ExecutionControl specific action needed to un-freeze. */
   virtual void un_freeze()
   {
      return;
   }

   //
   // FIXME: These pause functions should be worked into the general freeze
   // ExecutionControl methodology.
   /*! @brief Check if we hit a pause sync point and need to go to freeze.
    *  @param check_pause_delta Check pause job delta time in seconds. */
   virtual void check_pause( double const check_pause_delta );

   /*! @brief Checking if we started in freeze.
    *  @param check_pause_delta Check pause job delta time in seconds. */
   void check_pause_at_init( double const check_pause_delta );

   virtual bool set_pending_mtr( MTREnum mtr_value );
   /*! @brief Determine if the Mode Transition Request (MTR) is valid given the current mode.
    *  @return True if valid, false otherwise.
    *  @param mtr_value Mode transition request. */
   virtual bool is_mtr_valid( MTREnum mtr_value );
   /*! @brief Translate MTR into a pending execution mode transition.
    *  @param mtr_value MTR value for next execution mode. */
   virtual void set_mode_request_from_mtr( MTREnum mtr_value );

   //
   // Federation save and checkpoint
   //
   // Federation save and checkpoint
   /*! @brief Start the Federation save at the specified scenario time.
    *  @param freeze_scenario_time Scenario time to freeze.
    *  @param file_name            Checkpoint file name. */
   virtual void start_federation_save_at_scenario_time( double freeze_scenario_time, char const *file_name );

   //
   // Federation freeze/pause management functions.
   //
   // IMSim extensions.
   /*! @brief Adds a freeze interaction time into freeze scenario time collection.
    *  @param t Scenario time to freeze the simulation in seconds. */
   virtual void add_freeze_scenario_time( double t );
   /*! @brief Trigger a FREEZE interaction from the FreezeInteractionHandler
    * and updated the supplied time with the time computed by the
    * FreezeInteractionHandler.
    *  @param freeze_scenario_time Scenario freeze time. */
   virtual void trigger_freeze_interaction( double &freeze_scenario_time );
   /*! @brief Checks for a freeze interaction time from the freeze sim time collection.
    *  @return True if freeze time found; False otherwise. */
   virtual bool check_freeze_time();
   /*! @brief Checks for scenario freeze times.
    *  @return True is time to go to freeze; False otherwise. */
   virtual bool check_scenario_freeze_time();

   /*! @brief Add pause time.
    *  @param time Pause time.
    *  @param label Pause label (Synchronization point). */
   virtual void add_pause( TrickHLA::Int64Time *time, std::wstring const &label );

   /*! @brief Clear a pause time by label.
    *  @param label Pause label (Synchronization point). */
   virtual void clear_pause( std::wstring const &label );

   // Freeze time management functions.
   /*! @brief Set the time-padding used to offset the go to run time.
    *  @param t Time in seconds to pad for time based mode transitions. */
   virtual void set_time_padding( double t );

   //
   // Save and Restore
   /* @brief Determines if Save and Restore is supported by this ExecutionControl method.
    * @return True if Save and Restore is supported by this ExecutionControl method. */
   virtual bool is_save_and_restore_supported()
   {
      return ( true );
   }
   /*! @brief Checks if Save has been initiated by this ExecutionControl method.
    * @return True if Save is initiated and synchronized with the federation,
    * False if Save not supported. */
   virtual bool is_save_initiated();
   /*! @brief Federates that did not announce the save, perform a save.
    * @return True if Save can proceed, False if not. */
   virtual bool perform_save()
   {
      return ( false );
   }
   /*! @brief Converts HLA sync points into something Trick can save in a checkpoint. */
   void convert_loggable_sync_pts();
   /*! @brief Converts checkpointed sync points into HLA sync points. */
   void reinstate_logged_sync_pts();

  protected:
   static std::string const type; ///< @trick_units{--} ExecutionControl type string.

   MTREnum pending_mtr; ///< @trick_units{--} Pending Mode Transition Requested.

   int                             freeze_inter_count;         ///< @trick_io{**} Number of TrickHLA Freeze Interactions.
   TrickHLA::Interaction          *freeze_interaction;         ///< @trick_io{**} Interaction to FREEZE the sim at a specified time.MTRInteractionHandler   mtr_interaction_handler; ///< @trick_units{--} SRFOM MTR interaction handler.
   IMSim::FreezeInteractionHandler freeze_interaction_handler; ///< @trick_units{--} Freeze interaction handler.

   FreezeTimeSet freeze_scenario_times; ///< @trick_io{**} collection of scenario times when we must enter FREEZE mode

   TrickHLA::Int64Time checktime;      ///< @trick_units{--} For DIS: Checking time to pause
   PausePointList      pause_sync_pts; ///< @trick_units{--} Synchronization points used for pausing the sim.

   /*! @brief Return the relevant IMSim::ExecutionConfiguration object.
    *  @return Pointer to the relevant IMSim::ExecutionConfiguration object. */
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

} // namespace IMSim

#endif /* IMSIM_EXECUTON_CONTROL_HH */
