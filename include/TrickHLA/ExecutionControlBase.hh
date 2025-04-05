/*!
@file TrickHLA/ExecutionControlBase.hh
@ingroup TrickHLA
@brief This class provides and abstract base class as the base implementation
for execution control.

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
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/CTETimelineBase.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionConfigurationBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/ScenarioTimeline.cpp}
@trick_link_dependency{../../source/TrickHLA/SimTimeline.cpp}
@trick_link_dependency{../../source/TrickHLA/SimTimeline.cpp}
@trick_link_dependency{../../source/TrickHLA/SyncPointManagerBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Timeline.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Oct 2019, --, Initial version.}
@revs_end

*/

#ifndef TRICKHLA_EXECUTION_CONTROL_BASE_HH
#define TRICKHLA_EXECUTION_CONTROL_BASE_HH

// System includes.
#include <cstdint>
#include <string>

#include "TrickHLA/CTETimelineBase.hh"
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/ScenarioTimeline.hh"
#include "TrickHLA/SimTimeline.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/SyncPointManagerBase.hh"
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

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Federate;
class Manager;
class Object;
class ExecutionConfigurationBase;

class ExecutionControlBase : public TrickHLA::SyncPointManagerBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__ExecutionControlBase();

  public:
   // Principal timelines for federation execution control.
   ScenarioTimeline *scenario_timeline; ///< @trick_units{--} The scenario timeline.
   SimTimeline      *sim_timeline;      ///< @trick_units{--} The simulation timeline.
   CTETimelineBase  *cte_timeline;      ///< @trick_units{--} The Central Timing Equipment (CTE) timeline.

   // These are the execution control roles available to a federate.
   bool use_preset_master; /**< @trick_units{--}
      Set to true to force the use of the preset value for the "master" flag.
      (default: false) */
   bool master;            /**< @trick_units{--}
      Is true when this federate is the "master" federate for the Multiphase
      initialization process. (default: false) */

   // TODO: May also want to change this into an STL Array.
   char *multiphase_init_sync_points; /**< @trick_units{--}
      Comma-separated list of multi-phase initialization sync-points. */

  public:
   /*! @brief Default constructor for the ExecutionControlBase class. */
   ExecutionControlBase();
   /*! @brief Initialization constructor for the ExecutionControlBase class.
    *  @param exec_config The associated ExecutionConfigurationBase class instance. */
   explicit ExecutionControlBase( ExecutionConfigurationBase &exec_config );
   /*! @brief Destructor for the ExecutionControlBase class. */
   virtual ~ExecutionControlBase() = 0;

   // Use implicit assignment operator.

  public:
   //
   // Execution Control initialization methods.
   //
   /*! @brief Setup the federate wide references in the ExecutionControl class
    * instance.
    * @param fed         Associated federate manager class instance.
    * @param mgr         Associated federate manager class instance.
    * @param exec_config Associated Execution Configuration Object (ExCO). */
   virtual void setup( TrickHLA::Federate                   &fed,
                       TrickHLA::Manager                    &mgr,
                       TrickHLA::ExecutionConfigurationBase &exec_config );
   /*! @brief Setup the federate wide references in the ExecutionControl class
    * instance.
    * @param fed Associated federate manager class instance.
    * @param mgr  Associated federate manager class instance. */
   virtual void setup( TrickHLA::Federate &fed,
                       TrickHLA::Manager  &mgr );
   /*! @brief Initialize the TrickHLA::ExecutionControlBase object instance. */
   virtual void initialize();
   /*! @brief Join federation execution process. */
   virtual void join_federation_process();
   /*! @brief Processes run before the multi-phase initialization begins. */
   virtual void pre_multi_phase_init_processes() = 0;
   /*! @brief Processes run after the multi-phase initialization ends. */
   virtual void post_multi_phase_init_processes() = 0;
   /*! @brief Execution control specific shutdown process. */
   virtual void shutdown() = 0;

   //
   // Execution Control support methods.
   //
   // Get the ExecutionControl type identification string.
   virtual std::string const &get_type() = 0;
   /*! Setup the ExecutionControl object Trick ref ATTRIBUTES. */
   virtual void setup_object_ref_attributes() = 0;
   /*! Setup the ExecutionControl interaction Trick ref ATTRIBUTES. */
   virtual void setup_interaction_ref_attributes() = 0;
   /*! Setup the ExecutionControl objects HLA RTI handles. */
   virtual void setup_object_RTI_handles() = 0;
   /*! Setup the ExecutionControl interaction HLA RTI handles. */
   virtual void setup_interaction_RTI_handles() = 0;
   /*! @brief The object instance name reservation succeeded for the given name.
    *  @return True if ExecutionConfiguration object name matched the object instance name.
    *  @param obj_instance_name Object instance name. */
   virtual bool object_instance_name_reservation_succeeded( std::wstring const &obj_instance_name );
   /*! @brief The object instance name reservation failed for the given name.
    *  @return True if ExecutionConfiguration object handled the failure.
    *  @param obj_instance_name Object instance name. */
   virtual bool object_instance_name_reservation_failed( std::wstring const &obj_instance_name );
   /*! Setup the ExecutionControl objects HLA RTI handles. */
   virtual void register_objects_with_RTI();
   /*! @brief Add a TrickHLA::Object to the manager object map.
    *  @param object TrickHLA::Object to add to the manager object map. */
   virtual void add_object_to_map( Object *object );
   /*! Setup the ExecutionControl interactions HLA RTI handles. */
   virtual void register_interactions_with_RTI()
   {
      return;
   }
   /*! @brief Is the specified sync-point label contained in the multiphase init
    *  sync-point list.
    *  @param sync_point_label Name of the synchronization point label.
    *  @return True if the multiphase init sync-point list contains the sync-point,
    *  false otherwise. */
   bool const contains_multiphase_init_sync_point( std::wstring const &sync_point_label );
   /*! Add initialization synchronization points to regulate startup. */
   virtual void add_initialization_sync_points() = 0;
   /*! Add user defined multiphase initialization synchronization points to
    * regulate regulate the multiphase initialization process. */
   virtual void add_multiphase_init_sync_points();
   /*! @brief Clear any remaining multiphase initialization synchronization points
    *  that have not been achieved and wait for the federation to be
    *  synchronized on it. */
   virtual void clear_multiphase_init_sync_points();
   /*! @brief Achieve all the user defined mulit-phase initialization
    *  synchronization points if they are not already achieved and are not
    *  one of the predefined ExecutionControl synchronization points. */
   virtual void achieve_all_multiphase_init_sync_points();
   /*! @brief Wait for all the user defined mulit-phase initialization
    *  synchronization points if they are not already achieved and are not
    *  one of the predefined ExecutionControl synchronization points. */
   virtual void wait_for_all_multiphase_init_sync_points();

   //*! @brief The RTI has announced the existence of a synchronization point.
   //*  @param label             Sync-point label.
   //*  @param user_supplied_tag Use supplied tag.*/
   ///*  virtual void sync_point_announced(
   //   std::wstring const     &label,
   //   RTI1516_USERDATA const &user_supplied_tag ); */

   /*! Publish the ExecutionControl objects and interactions. */
   virtual void publish() = 0;
   /*! Unpublish the ExecutionControl objects and interactions. */
   virtual void unpublish() = 0;
   /*! Subscribe to the ExecutionControl objects and interactions. */
   virtual void subscribe() = 0;
   /*! Unsubscribe the ExecutionControl objects and interactions. */
   virtual void unsubscribe() = 0;

   //
   // ExecutionControl runtime methods.
   //
   /*! @brief Send the ExecutionConfiguration data if we are the master federate. */
   virtual void send_execution_configuration();
   /*! @brief Receive the ExecutionConfiguration data from the master federate. */
   virtual void receive_execution_configuration();
   /*! @brief Send the attribute value requested data to the remote federates.
    *  @param update_time  Update time. */
   virtual void send_requested_data( Int64Time const &update_time );
   /*! @brief Handle the received cyclic data. */
   virtual void receive_cyclic_data();
   /*! @brief Requesting an attribute value update for the given object
    *  instance and attributes.
    *  @param theObject HLA object instance handle.
    *  @param theAttributes HLA attribute handle set. */
   virtual void provide_attribute_update( RTI1516_NAMESPACE::ObjectInstanceHandle const &theObject,
                                          RTI1516_NAMESPACE::AttributeHandleSet const   &theAttributes );
   /*! @brief Gets the TrickHLA Object for the specified RTI Object Instance Name.
    *  @return TrickHLA Object.
    *  @param obj_instance_name Object instance name. */
   virtual Object *get_trickhla_object( std::string const &obj_instance_name );
   /*! @brief Gets the TrickHLA Object for the specified RTI Object Instance Name.
    *  @return TrickHLA Object.
    *  @param obj_instance_name Object instance name. */
   virtual Object *get_trickhla_object( std::wstring const &obj_instance_name );
   /*! @brief Returns the first object that matches the specified Object-Class,
    * object instance name, and is not registered, i.e. the instance ID == 0.
    *  @return TrickHLA Object
    *  @param theObjectClass        RTI Object class type.
    *  @param theObjectInstanceName Object instance name. */
   virtual Object *get_unregistered_object(
      RTI1516_NAMESPACE::ObjectClassHandle const &theObjectClass,
      std::wstring const                         &theObjectInstanceName );
   /*! @brief Returns the first object that is remotely owned, has the same
    * Object-Class, is not registered, and does not have an Object Instance
    * Name associated with it.
    *  @return The associated TrickHLA::Object instance; otherwise NULL.
    *  @param theObjectClass RTI Object class type. */
   virtual Object *get_unregistered_remote_object(
      RTI1516_NAMESPACE::ObjectClassHandle const &theObjectClass );
   /*! @brief Identifies the object as deleted from the RTI.
    *  @param instance_id HLA object instance handle. */
   virtual bool mark_object_as_deleted_from_federation(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_id );
   /*! @brief Scheduled method used as a callback to identify if any objects
    * were deleted from the RTI. */
   virtual void process_deleted_objects();
   /*! @brief Test to see if ExecutionControl needs to wait on initialization data.
    *  @details Most ExecutionControl approaches require that we wait for the
    *  required initialization data. Currently, only the 'Simple' scheme does not.
    *  @return True if ExecutionControl needs to wait on the initialization data. */
   virtual bool wait_for_init_data()
   {
      return ( true );
   }
   /*! @brief Test to see if ExecutionControl needs to wait on initialization
    *  synchronization point.
    *  @details Most ExecutionControl approaches require that we wait for
    *  specific initialization synchronization points in sprecific orders.
    *  Currently, only the 'Simple' and 'DIS' scheme do not.
    *  @return True if ExecutionControl needs to wait on the initialization synchronization points. */
   virtual bool is_wait_for_init_sync_point_supported()
   {
      return ( true );
   }

   //
   // ExecutionControl interaction methods.
   //
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
      bool                                              received_as_TSO ) = 0;
   /*! @brief Send a mode transition request to the Master federate.
    *  @param requested_mode Requested mode. */
   virtual void send_mode_transition_interaction( ModeTransitionEnum requested_mode ) = 0;
   /*! @brief Process a new mode interaction.
    *  @return True if new mode interaction is successfully processed. */
   virtual bool process_mode_interaction() = 0;
   /*! @brief Get a comma separated list of interaction FOM names used.
    *  @return Comma separated list of interaction FOM names used. */
   virtual std::string get_interaction_FOM_names() = 0;
   /*! @brief Sets the next ExecutionControl run mode.
    *  @param exec_control Next ExecutionControl run mode. */
   virtual void set_next_execution_control_mode( ExecutionControlEnum exec_control ) = 0;
   /*! @brief Process changes from any received Execution Control Objects (ExCOs).
    *  @return True if mode transition occurred. */
   virtual bool process_execution_control_updates() = 0;

   //
   // Timeline access and management methods.
   //
   /*! @brief Set the Scenario Timeline.
    *  @param timeline Scenario timeline. */
   void set_scenario_timeline( ScenarioTimeline *timeline )
   {
      this->scenario_timeline = timeline;
   }

   /*! @brief Check to see if the Scenario Timeline exists.
    *  @return True if it exists, False otherwise. */
   bool does_scenario_timeline_exist() const
   {
      return ( scenario_timeline != NULL );
   }

   /*! @brief Check to see if the Simulation Timeline exists.
    *  @return True if it exists, False otherwise. */
   bool does_sim_timeline_exist() const
   {
      return ( sim_timeline != NULL );
   }

   /*! @brief Check to see if the CTE Timeline exists.
    *  @return True if it exists, False otherwise. */
   bool does_cte_timeline_exist() const
   {
      return ( cte_timeline != NULL );
   }

   /*! @brief Get the current simulation time from Simulation Timeline.
    *  @return The current simulation time in seconds. */
   double get_sim_time();

   /*! @brief Get the current Central Timing Equipment time from CTE Timeline.
    *  @return The current CTE time in seconds. */
   double get_cte_time();

   /*! @brief Get the current scenario time from Scenario Timeline.
    *  @return The current scenario time in seconds. */
   double get_scenario_time();

   /*! @brief Convert the a given scenario time into simulation time.
    *  @return Corresponding simulation time in seconds.
    *  @param scenario_time Scenario time to convert. */
   double convert_scenario_time_to_sim_time( double scenario_time )
   {
      return ( get_sim_time() + ( scenario_time - get_scenario_time() ) );
   }

   /*! @brief Convert the a given simulation time into scenario time.
    *  @return Corresponding scenario time in seconds.
    *  @param sim_time Simulation time to convert. */
   double convert_sim_time_to_scenario_time( double sim_time )
   {
      return ( get_scenario_time() + ( sim_time - get_sim_time() ) );
   }

   //
   // Mode management support methods.
   //
   /*! @brief Clear the Mode Transition Request flag, the requested execution
    * mode, and the current execution mode. */
   virtual void clear_mode_values();
   /*! @brief The run mode transition routine.
    *  @return Currently always returns True. */
   virtual bool run_mode_transition() = 0;
   /*! @brief Announce the pending freeze mode transition with an 'mtr_freeze' sync-point. */
   virtual void freeze_mode_announce() = 0;
   /*! @brief The freeze mode transition routine.
    *  @return Currently always returns False. */
   virtual bool freeze_mode_transition() = 0;
   /*! @brief Announce to the federation execution that a shutdown is occurring. */
   virtual void shutdown_mode_announce() = 0;
   /*! @brief The shutdown mode transition routine. */
   virtual void shutdown_mode_transition() = 0;
   /*! @brief Checks to see if shutdown has been commanded.
    *  @return True if shutdown has been announced, else False. */
   virtual bool check_for_shutdown();
   /*! @brief Checks to see if shutdown has been commanded and, if so, terminates the simulation.
    *  @return False if shutdown has NOT been announced. */
   virtual bool check_for_shutdown_with_termination();

   /*! @brief  Is the federate execution in initialization.
    *  @return True if federate is initializing, false otherwise. */
   virtual bool is_initializing()
   {
      return ( current_execution_control_mode == EXECUTION_CONTROL_INITIALIZING );
   }
   /*! @brief  Is the federate execution in initialization.
    *  @return True if federate is initializing, false otherwise. */
   virtual bool is_running()
   {
      return ( current_execution_control_mode == EXECUTION_CONTROL_RUNNING );
   }
   /*! @brief  Is the federate execution in initialization.
    *  @return True if federate is initializing, false otherwise. */
   virtual bool is_in_freeze()
   {
      return ( current_execution_control_mode == EXECUTION_CONTROL_FREEZE );
   }
   /*! @brief  Is the federate execution in initialization.
    *  @return True if federate is initializing, false otherwise. */
   virtual bool is_in_restart()
   {
      return ( current_execution_control_mode == EXECUTION_CONTROL_RESTART );
   }
   /*! @brief  Is the federate execution in initialization.
    *  @return True if federate is initializing, false otherwise. */
   virtual bool is_in_reconfig()
   {
      return ( current_execution_control_mode == EXECUTION_CONTROL_RECONFIG );
   }
   /*! @brief  Is the federate execution in initialization.
    *  @return True if federate is initializing, false otherwise. */
   virtual bool is_shutdown()
   {
      return ( current_execution_control_mode == EXECUTION_CONTROL_SHUTDOWN );
   }

   //
   // Federation freeze/pause management functions.
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

   /*! @brief Set that federation execution freeze has been announced.
    *  @param flag True for federate freeze announce; False otherwise. */
   void set_freeze_announced( bool const flag )
   {
      this->announce_freeze = flag;
   }

   /*! @brief Is the federation execution freeze announced.
    *  @return True for federate freeze announced; False otherwise. */
   bool is_freeze_announced()
   {
      return this->announce_freeze;
   }

   /*! @brief Set that federation execution freeze is pending flag.
    *  @param flag True for federate freeze pending; False otherwise. */
   void set_freeze_pending( bool const flag )
   {
      this->freeze_the_federation = flag;
   }

   /*! @brief Is the federation execution freeze pending.
    *  @return True for federate freeze is pending; False otherwise. */
   bool const is_freeze_pending()
   {
      return this->freeze_the_federation;
   }

   //
   // Functions for the freeze ExecutionControl methodology.
   //
   /*! @brief Check if we hit a pause sync point and need to go to freeze.
    *  @param check_pause_delta Check pause job delta time in seconds. */
   virtual void check_pause( double const check_pause_delta );
   /*! @brief Checking if we started in freeze.
    *  @param check_pause_delta Check pause job delta time in seconds. */
   virtual void check_pause_at_init( double const check_pause_delta );

   /*! @brief Set the mode transition requested flag. */
   virtual void set_mode_transition_requested()
   {
      this->mode_transition_requested = true;
   }
   /*! @brief Clear the mode transition requested flag. */
   virtual void clear_mode_transition_requested()
   {
      this->mode_transition_requested = false;
   }
   /*! @brief Determine if a mode transition has been requested.
    *  @return mode_change_requested True if a mode transition has been requested. */
   virtual bool is_mode_transition_requested()
   {
      return this->mode_transition_requested;
   }

   // Role determination methods.
   /*! @brief Query if there is a preset Master.
    *  @return True if there is a preset Master; False otherwise. */
   virtual bool is_master_preset() const
   {
      return this->use_preset_master;
   }
   /*! @brief Set this as the Master federate.
    *  @param master_flag True for a Master federate; False otherwise. */
   virtual void set_master( bool master_flag );
   /*! @brief Query if this is the Master federate.
    *  @return True if there is the Master; False otherwise. */
   virtual bool is_master() const
   {
      return this->master;
   }
   /*! @brief Determine if this federate is a late joining federate.
    *  @return True if this is a late joining federate. */
   virtual bool is_late_joiner()
   {
      return this->late_joiner;
   }
   /*! @brief Check if we have determine if this federate is a late joining federate.
    *  @return True if late joining status is determined. */
   virtual bool is_late_joiner_determined()
   {
      return this->late_joiner_determined;
   }

   // Execution mode access methods.
   /*! @brief Get the currently requested execution mode.
    *  @return The currently requested execution mode. */
   virtual ExecutionControlEnum get_requested_execution_control_mode()
   {
      return ( this->requested_execution_control_mode );
   }
   /*! @brief Set the currently requested execution mode.
    *  @param mode The requested execution mode. */
   virtual void set_requested_execution_control_mode( ExecutionControlEnum mode )
   {
      this->requested_execution_control_mode = mode;
   }
   /*! @brief Set the currently requested execution mode.
    *  @param mode The requested execution mode. */
   virtual void set_requested_execution_control_mode( int16_t mode )
   {
      this->requested_execution_control_mode = execution_control_int16_to_enum( mode );
   }
   /*! @brief Get the current execution mode.
    *  @return The current execution mode. */
   virtual ExecutionControlEnum get_current_execution_control_mode()
   {
      return ( this->current_execution_control_mode );
   }
   /*! @brief Set the current execution control mode.
    *  @param mode The current execution control mode. */
   virtual void set_current_execution_control_mode( ExecutionControlEnum mode )
   {
      this->current_execution_control_mode = mode;
   }

   //
   // Federation save and checkpoint
   /*! @brief Start the Federation save at the specified scenario time.
    *  @param freeze_scenario_time Scenario time to freeze.
    *  @param file_name            Checkpoint file name. */
   virtual void start_federation_save_at_scenario_time( double freeze_scenario_time, char const *file_name ) = 0;

   /*! @brief Convert the variables to a form Trick can checkpoint. */
   virtual void encode_checkpoint();

   /*! @brief Restore the state of this class from the Trick checkpoint. */
   virtual void decode_checkpoint();

   /*! @brief Clear/release the memory used for the checkpoint data structures. */
   virtual void free_checkpoint();

   //
   // Execution Control association methods.
   /*! @brief Set the reference to the associated TrickHLA::Federate.
    *  @param fed Associated TrickHLA::Federate. */
   virtual void set_federate( TrickHLA::Federate *fed )
   {
      this->federate = fed;
      // TODO: this->SyncPointManager.federate = fed;
   }
   /*! @brief Get the reference to the associated TrickHLA::Federate.
    *  @return Pointer to the associated TrickHLA::Federate. */
   virtual TrickHLA::Federate *get_federate()
   {
      return federate;
   }
   /*! @brief Get the reference to the associated TrickHLA::Manager.
    *  @return Pointer to the associated TrickHLA::Manager. */
   virtual TrickHLA::Manager *get_manager()
   {
      return manager;
   }
   /*! @brief Get the reference to the associated TrickHLA::ExecutionConfigurationBase object.
    *  @param exec_config Pointer to the associated TrickHLA::ExecutionConfigurationBase object. */
   virtual void set_execution_configuration( ExecutionConfigurationBase *exec_config )
   {
      execution_configuration = exec_config;
   }
   /*! @brief Get the reference to the associated TrickHLA::ExecutionConfigurationBase object.
    *  @return Pointer to the associated TrickHLA::ExecutionConfigurationBase object. */
   virtual ExecutionConfigurationBase *get_execution_configuration()
   {
      return execution_configuration;
   }
   /*! @brief Remove the ExecutionConfiguration instance from the federation execution. */
   virtual void remove_execution_configuration();
   /*! @brief Test is an execution configuration object is used.
    *  @return True if an execution configuration object is used. */
   virtual bool is_execution_configuration_used()
   {
      return ( execution_configuration != NULL );
   }

   // Freeze time management functions.
   /*! @brief Set the least common time step in seconds for the federation.
    *  @param lcts Least Common Time Step time in seconds. */
   virtual void set_least_common_time_step( double const lcts );

   /*! @brief Refresh the least common time step especially if the HLA base time units changed. */
   virtual void refresh_least_common_time_step();

   /*! @brief Get the value of the least common time step.
    *  @return The value of the least common time step. */
   virtual int64_t get_least_common_time_step()
   {
      return this->least_common_time_step;
   }

   /*! @brief Get the value of the least common time step.
    *  @return The value of the least common time step. */
   virtual bool const is_enabled_least_common_time_step()
   {
      return this->enable_least_common_time_step;
   }

   /*! @brief Set the time-padding used to offset the go to run time.
    *  @param t Time in seconds to pad for time based mode transitions. */
   virtual void set_time_padding( double const t );

   /*! @brief Get the time-padding used to offset the go to run time.
    *  @return Time in seconds to pad for time based mode transitions. */
   virtual double get_time_padding()
   {
      return this->time_padding;
   }
   /*! @brief Get the Federation Execution simulation time for freeze.
    *  @return Simulation time in seconds for the Federation Execution to go to freeze. */
   virtual double get_simulation_freeze_time()
   {
      return this->simulation_freeze_time;
   }
   /*! @brief Set the Federation Execution simulation time for freeze.
    *  @param freeze_time Simulation time in seconds for the Federation Execution to go to freeze. */
   virtual void set_simulation_freeze_time( double freeze_time )
   {
      this->simulation_freeze_time = freeze_time;
   }
   /*! @brief Get the Federation Execution scenario time for freeze.
    *  @return Scenario time in seconds for the Federation Execution to go to freeze. */
   virtual double get_scenario_freeze_time()
   {
      return this->scenario_freeze_time;
   }
   /*! @brief Set the Federation Execution scenario time for freeze.
    *  @param freeze_time Scenario time in seconds for the Federation Execution to go to freeze. */
   virtual void set_scenario_freeze_time( double freeze_time )
   {
      this->scenario_freeze_time = freeze_time;
   }

   //
   // Save and Restore
   /* @brief Determines if Save and Restore is supported by this ExecutionControl method.
    * @return True if Save and Restore is supported by this ExecutionControl method. */
   virtual bool is_save_and_restore_supported()
   {
      return ( false );
   }
   /*! @brief Checks if Save has been initiated by this ExecutionControl method.
    * @return True if Save is initiated and synchronized with the federation,
    * False if Save not supported. */
   virtual bool is_save_initiated()
   {
      return ( false );
   }
   /*! @brief Federates that did not announce the save, perform a save.
    * @return True if Save can proceed, False if not. */
   virtual bool perform_save()
   {
      return ( false );
   }
   /*! @brief Converts HLA sync points into something Trick can save in a checkpoint. */
   virtual void convert_loggable_sync_pts()
   {
      return;
   }
   /*! @brief Converts checkpointed sync points into HLA sync points. */
   virtual void reinstate_logged_sync_pts()
   {
      return;
   }

  protected:
   double time_padding; ///< @trick_units{s} Time in seconds to add to the go-to-run time.

   bool enable_least_common_time_step; /**< @trick_units{--} Enable the use of LCTS. */

   double least_common_time_step_seconds; /**< @trick_units{--} The LCTS in seconds. */

   int64_t least_common_time_step; /**< @trick_units{--}
      A 64 bit integer time that represents the base HLA Logical Time representation
      for the least common value of all the time step values in the federation
      execution (LCTS). This value is set by the Master Federate and does not
      change during the federation execution. This is used in the computation to
      find the next HLA Logical Time Boundary (HLTB) available to all federates
      in the federation execution. The basic equation is
            HLTB = ( floor(GALT/LCTS) + 1 ) * LCTS,
      where GALT is the greatest available logical time. This is used to
      synchronize the federates in a federation execution to be on a common
      logical time boundary. */

   ExecutionConfigurationBase *execution_configuration; /**< @trick_units{--}
      Associates TrickHLA::ExecutionConfigurationBase class object instance.
      Since this is an abstract class, the actual instance will be a concrete
      derived class instance (e.g. SRFOM:ExecutionControl). */

   bool                 mode_transition_requested;        ///< @trick_units{--} Flag to indicate a mode transition has been requested.
   ExecutionControlEnum requested_execution_control_mode; ///< @trick_units{--} The latest mode transition requested.
   ExecutionControlEnum current_execution_control_mode;   ///< @trick_units{--} Current SRFOM federate current execution mode.

   double next_mode_scenario_time; ///< @trick_units{s} Scenario time for mode transition.
   double next_mode_cte_time;      ///< @trick_units{s} CTE time for next managed mode transition.

   double simulation_freeze_time; ///< @trick_units{s} Trick simulation time for freeze.
   double scenario_freeze_time;   ///< @trick_units{s} Federation execution scenario time for freeze.

   bool announce_freeze;       ///< @trick_io{**} DANNY2.7 flag to indicate that this federate is announcing go to freeze mode
   bool freeze_the_federation; ///< @trick_io{**} DANNY2.7 flag to indicate the federation is going into freeze now

   bool late_joiner;            ///< @trick_units{--} Flag that this federate is a late joiner.
   bool late_joiner_determined; ///< @trick_units{--} Flag for late joiner determination.

   // Shortcuts to associated TrickHLA management and control objects.
   TrickHLA::Manager *manager; ///< @trick_io{**} Associated manager.

  private:
   // Do not allow the copy constructor.
   /*! @brief Copy constructor for ExecutionControlBase class.
    *  @details This constructor is private to prevent inadvertent copies. */
   ExecutionControlBase( ExecutionControlBase const &rhs );
   /*! @brief Assignment operator for ExecutionControlBase class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   ExecutionControlBase &operator=( ExecutionControlBase const &rhs );
};

} // namespace TrickHLA

#endif /* TRICKHLA_EXECUTION_CONTROL_BASE_HH */
