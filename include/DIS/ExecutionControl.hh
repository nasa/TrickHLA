/*!
@file DIS/ExecutionControl.hh
@ingroup DIS
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

@python_module{DIS}

@tldh
@trick_link_dependency{../../source/DIS/ExecutionControl.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/DIS/PausePointList.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, DIS development.}
@revs_end

*/

#ifndef _DIS_EXECUTON_CONTROL_HH_
#define _DIS_EXECUTON_CONTROL_HH_

// System includes.

// Trick include files.

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Types.hh"

// DIS include files.
#include "DIS/ExecutionConfiguration.hh"
#include "DIS/PausePointList.hh"
#include "DIS/Types.hh"

namespace DIS
{

class ExecutionControl : public TrickHLA::ExecutionControlBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrDIS__ExecutionControl();

  public:
   /*! @brief Default constructor for the DIS ExecutionControl class. */
   ExecutionControl();
   /*! @brief Destructor for the DIS ExecutionControl class. */
   virtual ~ExecutionControl();

  public:
   /*! @brief Get the ExecutionControl type identification string.
    *  @return A constant reference to the type identification string.
    */
   virtual const std::wstring &get_type() { return ( type ); }

   //
   // Execution Control initialization routines.
   // This is called by the TrickHLA::Federate::initialize routine.
   /*! @brief Execution Control initialization routine.
    *  @param fed The associated TrickHLA::Federate. */
   virtual void initialize( TrickHLA::Federate &fed );
   /*! @brief Join federation execution process. */
   virtual void join_federation_process();
   /*! @brief Process run before the multi-phase initialization begins. */
   virtual void pre_multi_phase_init_processes();
   /*! @brief Process run after the multi-phase initialization ends. */
   virtual void post_multi_phase_init_process();
   /*! @brief Execution control specific shutdown process. */
   virtual void shutdown();

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
      std::wstring const &              label,
      RTI1516_USERDATA const &          user_supplied_tag );
   /*! Clear any remaining multiphase initialization synchronization points
    *  that have not been achieved and wait for the federation to be
    *  synchronized on it. */
   virtual void clear_multiphase_init_sync_points();
   /*! Publish the ExecutionControl objects and interactions. */
   virtual void publish();
   /*! Unpublish the ExecutionControl objects and interactions. */
   virtual void unpublish();
   /*! Subscribe to the ExecutionControl objects and interactions. */
   virtual void subscribe();
   /*! Unsubscribe the ExecutionControl objects and interactions. */
   virtual void unsubscribe();

   //
   // Execution Control support routines.
   /*! @brief The object instance name reservation failed for the given name.
    *  @return True if ExecutionConfiguration object handled the failure.
    *  @param obj_instance_name Object instance name. */
   virtual bool object_instance_name_reservation_failed( std::wstring const &obj_instance_name );
   /*! @brief Callback from TrickHLA::FedAmb through TrickHLA::Federate for
    *  when registration of a synchronization point success.
    *  and is one of the sync-points created.
    *  @param label      Sync-point label. */
   virtual void sync_point_registration_succeeded( std::wstring const &label );
   /*! @brief Callback from TrickHLA::FedAmb through TrickHLA::Federate for
    *  when registration of a synchronization point fails.
    *  and is one of the sync-points created.
    *  @param label      Sync-point label.
    *  @param not_unique True if not unique label. */
   virtual void sync_point_registration_failed( std::wstring const &label, bool not_unique );

   // Execution Control initialization routines.
   /*! @brief Join federation execution process. */
   virtual void join_federation_process();
   /*! @brief Determine the federate role in the federation execution. */
   virtual void role_determination_process();
   /*! @brief Process to join the federation execution early in initialization. */
   virtual void early_joiner_hla_init_process();
   /*! @brief Process to determine is a federate is joining late in or after initialization. */
   virtual void late_joiner_hla_init_process();
   /*! @brief Process run before the multi-phase initialization begins. */
   virtual void pre_multi_phase_init_processes();
   /*! @brief Process run after the multi-phase initialization ends. */
   virtual void post_multi_phase_init_process();
   /*! @brief Execution control specific shutdown process. */
   virtual void shutdown();

   // DIS extensions to ExecutionControlBase.
   /*! @brief Determine if this federate is the Master for the federation. */
   void determine_federation_master();

   /*! @brief Test to see if ExecutionControl needs to wait on initialization
    *  synchronization point.
    *  @details Most ExecutionControl approaches require that we wait for
    *  specific initialization synchronization points in sprecific orders.
    *  Currently, only the 'Simple' and 'DIS' scheme do not.
    *  @return True if ExecutionControl needs to wait on the initialization synchronization points. */
   bool wait_on_init_sync_point() { return ( false ); }

   /*! @brief Sets the next ExCO run mode.
    *  @param exec_control Next Execution configuration run mode. */
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
   virtual bool process_mode_interaction() { return process_mode_transition_request(); };
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
   /*! @brief Check for exit from freeze. */
   virtual void check_freeze_exit();
   /*! @brief Announce to the federation execution that a shutdown is occurring. */
   virtual void shutdown_mode_announce();
   /*! @brief The shutdown mode transition routine. */
   virtual void shutdown_mode_transition();

   //
   // Federation freeze/pause management functions.
   //
   /*! @brief Check for exit from freeze. */
   virtual void check_freeze();
   /*! @brief Check if a Trick freeze was commanded; if we announced freeze,
    *  tell other federates to freeze. */
   virtual void enter_freeze();
   /*! @brief Check for exit from freeze.
    *  @return True if should exit from freeze. */
   virtual virtual bool check_freeze_exit();
   /*! @brief Routine to handle going from freeze to run; if we announced the
    * freeze, tell other federates to run. */
   virtual void exit_freeze();

   /*! @brief Add pause time.
    *  @param time Pause time.
    *  @param label Pause label (Synchronization point). */
   virtual void add_pause( TrickHLA::Int64Time *time, std::wstring const &label );

   virtual bool set_pending_mtr( MTREnum mtr_value );
   /*! @brief Determine if the Mode Transition Request (MTR) is valid given the current mode.
    *  @return True if valid, false otherwise.
    *  @param mtr_value Mode transition request. */
   virtual bool is_mtr_valid( MTREnum mtr_value );
   /*! @brief Translate MTR into a pending execution mode transition.
    *  @param mtr_value MTR value for next execution mode. */
   virtual void set_mode_request_from_mtr( MTREnum mtr_value );

   // Freeze time management functions.
   /*! @brief Set the time-padding used to offset the go to run time.
    *  @param t Time in seconds to pad for time based mode transitions. */
   virtual void set_time_padding( double t );

   //
   // Save and Restore
   /* @brief Determines if Save and Restore is supported by this ExecutionControl method.
    * @return True if Save and Restore is supported by this ExecutionControl method. */
   virtual bool is_save_and_restore_supported() { return ( true ); }
   /*! @brief Checks if Save has been initiated by this ExecutionControl method.
    * @return True if Save is initiated and synchronized with the federation,
    * False if Save not supported. */
   virtual bool is_save_initiated();

  protected:
   static const std::wstring type; ///< @trick_units{--} ExecutionControl type string.

   MTREnum pending_mtr; ///< @trick_units{--} Pending Mode Transition Requested.

   TrickHLA::Int64Time checktime;      ///< @trick_units{--} For DIS: Checking time to pause
   PausePointList      pause_sync_pts; ///< @trick_units{--} Synchronization points used for pausing the sim.

   /*! @brief Return the relevant DIS::ExecutionConfiguration object.
    *  @return Pointer to the relevant DIS::ExecutionConfiguration object. */
   ExecutionConfiguration *get_execution_configuration();

  private:
   // Do not allow the copy constructor.
   /*! @brief Copy constructor for ExecutionControl class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ExecutionControl( const ExecutionControl &rhs );
   /*! @brief Assignment operator for ExecutionControl class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ExecutionControl &operator=( const ExecutionControl &rhs );
};

} // namespace DIS

#endif /* _DIS_EXECUTON_CONTROL_HH_ */
