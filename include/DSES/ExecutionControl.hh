/*!
@file DSES/ExecutionControl.hh
@ingroup DSES
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

@python_module{DSES}

@tldh
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/DIS/ExecutionControl.cpp}
@trick_link_dependency{../../source/DIS/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, DSES development.}
@revs_end

*/

#ifndef _DSES_EXECUTON_CONTROL_HH_
#define _DSES_EXECUTON_CONTROL_HH_

// System includes.

// Trick include files.

// HLA include files.

// TrickHLA include files.
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Types.hh"

// DSES include files.
#include "DSES/ExecutionConfiguration.hh"
#include "DSES/Types.hh"

namespace DSES
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
   friend void init_attrDSES__ExecutionControl();

  public:
   /*! @brief Default constructor for the DSES ExecutionControl class. */
   ExecutionControl();
   /*! @brief Destructor for the DSES ExecutionControl class. */
   virtual ~ExecutionControl();

  public:
   /*! @brief Get the ExecutionControl type identification string.
    *  @return A constant reference to the type identification string.
    */
   virtual const std::string &get_type()
   {
      return ( type );
   }

   //
   // Execution Control initialization routines.
   // This is called by the TrickHLA::Federate::initialize routine.
   /*! @brief Execution Control initialization routine.
    *  @param fed The associated TrickHLA::Federate. */
   virtual void initialize();
   /*! Setup the Trick Ref ATTRIBUTES for ExecutionControl. */
   /*! @brief Join federation execution process. */
   virtual void join_federation_process();
   /*! @brief Process run before the multi-phase initialization begins. */
   virtual void pre_multi_phase_init_processes();
   /*! @brief Process run after the multi-phase initialization ends. */
   virtual void post_multi_phase_init_process();
   /*! @brief Execution control specific shutdown process. */
   virtual void shutdown();

   // DSES extensions to ExecutionControlBase.
   /*! @brief Determine if this federate is the Master for the federation. */
   void determine_federation_master();

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
   virtual void add_multiphase_init_sync_points();
   /*! @brief The RTI has announced the existence of a synchronization point.
    *  @param rti_ambassador    Reference to the HLA RTI Ambassador instance.
    *  @param label             Sync-point label.
    *  @param user_supplied_tag Use supplied tag.*/
   virtual void announce_sync_point(
      RTI1516_NAMESPACE::RTIambassador &rti_ambassador,
      std::wstring const &              label,
      RTI1516_USERDATA const &          user_supplied_tag );
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

   //
   // Execution Control support routines.
   /*! @brief The object instance name reservation failed for the given name.
    *  @return True if ExecutionConfiguration object handled the failure.
    *  @param obj_instance_name Object instance name. */
   virtual bool object_instance_name_reservation_failed( std::wstring const &obj_instance_name );

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
   virtual bool process_mode_interaction()
   {
      return process_mode_transition_request();
   };
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
   /*! @brief Check for exit from freeze.
    *  @return True if should exit from freeze. */
   virtual bool check_freeze_exit();

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

  protected:
   static const std::string type; ///< @trick_units{--} ExecutionControl type string.

   MTREnum pending_mtr; ///< @trick_units{--} Pending Mode Transition Requested.

   /*! @brief Return the relevant DSES::ExecutionConfiguration object.
    *  @return Pointer to the relevant DSES::ExecutionConfiguration object. */
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

} // namespace DSES

#endif /* _DSES_EXECUTON_CONTROL_HH_ */
