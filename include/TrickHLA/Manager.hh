/*!
@file TrickHLA/Manager.hh
@ingroup TrickHLA
@brief This class manages the interface between a Trick simulation and HLA.

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
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionConfigurationBase.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/ItemQueue.cpp}
@trick_link_dependency{../../source/TrickHLA/Interaction.cpp}
@trick_link_dependency{../../source/TrickHLA/InteractionItem.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2006, --, DSES Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support and testing.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_MANAGER_HH
#define TRICKHLA_MANAGER_HH

// System include files.
#include <cstdint>
#include <string>

// TrickHLA include files.
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/ItemQueue.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

// Special handling of SWIG limitations for forward declarations.
#ifdef SWIG
#   include "TrickHLA/Interaction.hh"
#else
namespace TrickHLA
{
// NOTE: This forward declaration of TrickHLA::Interaction and TrickHLA::Object
// are here to go with the #ifdef SWIG include. Normally, it would go with the
// other forward declarations below.
class Federate;
class Interaction;
class Object;
} // namespace TrickHLA
#endif // SWIG

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class InteractionItem;
class ExecutionConfigurationBase;

class Manager : public CheckpointConversionBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Manager();
   // DANNY federate needs to call some of manager's private methods during restore
   friend class Federate;

   //----------------------------- USER VARIABLES -----------------------------
   // The variables below this point are configured by the user in either the
   // input or modified-data files.
  public:
   int     obj_count; ///< @trick_units{--} Number of TrickHLA Objects.
   Object *objects;   ///< @trick_units{--} Array of TrickHLA object.

   int          inter_count;  ///< @trick_units{--} Number of TrickHLA Interactions.
   Interaction *interactions; ///< @trick_units{--} Array of TrickHLA Interactions.

   bool  restore_federation;          ///< @trick_io{*i} @trick_units{--} Flag indicating whether to trigger the restore
   char *restore_file_name;           ///< @trick_io{*i} @trick_units{--} File name, which will be the label name
   bool  initiated_a_federation_save; ///< @trick_io{**} Did this manager initiate the federation save?

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Manager class. */
   Manager();
   /*! @brief Destructor for the TrickHLA Manager class. */
   virtual ~Manager();

   /*! @brief Setup the required class instance associations.
    *  @param federate          Associated TrickHLA::Federate class instance.
    *  @param execution_control Associated ExecutionControl class instance. */
   void setup( Federate             &federate,
               ExecutionControlBase &execution_control );

   /*! @brief Initializes the federate using the a multiphase initialization
    * scheme, which must occur after the Federate and FedAmb have been
    * initialized. */
   void initialize();

   /*! @brief Perform initialization after a checkpoint restart. */
   void restart_initialization();

   /*! @brief Initialize the HLA delta time step which is the data cycle time. */
   void initialize_HLA_cycle_time();

   /*! @brief Verify the user specified object and interaction arrays and counts. */
   void verify_object_and_interaction_arrays();

   /*! @brief Checks to make sure the RTI is ready by making sure the
    * TrickHLA::Federate and TrickHLA:FedAmb exist and the RTI handles are
    * initialized.
    *  @return True if the RTI is ready, false otherwise.
    *  @param method_name The method/function name. */
   bool is_RTI_ready( char const *method_name );

   /*! @brief Check if this is a late joining federate.
    *  @return True if the is a late joining federate. */
   bool is_late_joining_federate() const
   {
      return execution_control->is_late_joiner();
   }

   /*! @brief Sends all the initialization data. */
   void send_init_data();

   /*! @brief Sends the initialization data for the specified object instance name.
    *  @param instance_name Name of object instance name to send data for. */
   void send_init_data( char const *instance_name );

   /*! @brief Wait to receive all the initialization data that is marked as required. */
   void receive_init_data();

   /*! @brief Wait to receive the initialization data for the specified object
    * instance name.
    * @param instance_name Name of object instance name to receive data for. */
   void receive_init_data( char const *instance_name );

   /*! @brief Clear any remaining initialization sync-points. */
   void clear_init_sync_points();

   /*! @brief Achieve then wait for the federation to become synchronized for
    * the specified sync-point label.
    *  @param sync_point_label Name of the synchronization point label. */
   void wait_for_init_sync_point( char const *sync_point_label );

   /*! @brief Request an update to the object attributes for the given object
    * instance name.
    *  @param instance_name Object instance name. */
   void request_data_update( std::wstring const &instance_name );

   /*! @brief Request an update to the object attributes for the given object
    * instance name.
    *  @param instance_name Object instance name. */
   void request_data_update( char const *instance_name );

   /*! @brief Send cyclic an requested atrributes data to the remote federates. */
   void send_cyclic_and_requested_data();

   /*! @brief Handle the received cyclic data. */
   void receive_cyclic_data();

   /*! @brief Process the object discovery.
    *  @return True if the instance was recognized, false otherwise.
    *  @param theObject             Instance handle to a Federate or Object instance.
    *  @param theObjectClass        Class of the object.
    *  @param theObjectInstanceName Name of the instance. */
   bool discover_object_instance( RTI1516_NAMESPACE::ObjectInstanceHandle const &theObject,
                                  RTI1516_NAMESPACE::ObjectClassHandle const    &theObjectClass,
                                  std::wstring const                            &theObjectInstanceName );

   /*! @brief Gets the TrickHLA Object for the specified RTI Object Instance Handle.
    *  @return TrickHLA Object.
    *  @param instance_id Object instance handle. */
   Object *get_trickhla_object( RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_id );

   /*! @brief Gets the TrickHLA Object for the specified RTI Object Instance Name.
    *  @return TrickHLA Object.
    *  @param obj_instance_name Object instance name. */
   Object *get_trickhla_object( std::string const &obj_instance_name );

   /*! @brief Gets the TrickHLA Object for the specified RTI Object Instance Name.
    *  @return TrickHLA Object.
    *  @param obj_instance_name Object instance name. */
   Object *get_trickhla_object( std::wstring const &obj_instance_name );

   /*! @brief The object instance name reservation succeeded for the given name.
    *  @param obj_instance_name Object instance name. */
   void object_instance_name_reservation_succeeded( std::wstring const &obj_instance_name );

   /*! @brief The object instance name reservation failed for the given name.
    *  @param obj_instance_name Object instance name. */
   void object_instance_name_reservation_failed( std::wstring const &obj_instance_name );

   /*! @brief Add a TrickHLA::Object to the manager object map.
    *  @param object TrickHLA::Object to add to the manager object map. */
   void add_object_to_map( Object *object );

   /*! @brief Get the pointer to the associated TrickHLA::Federate instance.
    *  @return Pointer to the associated TrickHLA::Federate instance. */
   Federate *get_federate()
   {
      return federate;
   }

   /*! @brief Get the pointer to the associated TrickHLA::ExecutionControlBase instance.
    *  @return Pointer to associated TrickHLA::ExecutionControlBase instance. */
   ExecutionControlBase *get_execution_control()
   {
      return this->execution_control;
   }

   /*! @brief Returns a pointer to the RTI ambassador, or NULL if one does not
    * exist yet.
    * @return Pointer to the RTI ambassador. */
   RTI1516_NAMESPACE::RTIambassador *get_RTI_ambassador();

   /*! @brief Return a copy of the federate's lookahead time.
    *  @return This federate's lookahead time interval. */
   Int64Interval get_lookahead() const;

   /*! @brief Return a copy of the granted HLA logical time.
    *  @return The granted federation time. */
   Int64Time get_granted_time() const;

   /*! @brief Return the granted HLA logical time in seconds.
    *  @return The granted federation time in seconds */
   int64_t const get_granted_base_time() const;

   // Interactions
   /*! @brief Process the received interactions. */
   void process_interactions();

   /*! @brief Process all received interactions by calling in turn each
    * interaction handler that is subscribed to the interaction.
    * @param theInteraction     Interaction handle.
    * @param theParameterValues Parameter values.
    * @param theUserSuppliedTag Users tag.
    * @param theTime            HLA time for the interaction.
    * @param received_as_TSO    True if interaction was received by RTI as TSO. */
   void receive_interaction(
      RTI1516_NAMESPACE::InteractionClassHandle const  &theInteraction,
      RTI1516_NAMESPACE::ParameterHandleValueMap const &theParameterValues,
      RTI1516_USERDATA const                           &theUserSuppliedTag,
      RTI1516_NAMESPACE::LogicalTime const             &theTime,
      bool const                                        received_as_TSO );

   /*! @brief Process the ownership requests. */
   void process_ownership();

   /*! @brief Identifies the object as deleted from the RTI.
    *  @param instance_id HLA object instance handle. */
   void mark_object_as_deleted_from_federation(
      RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_id );

   /*! @brief Scheduled method used as a callback to identify if any objects
    * were deleted from the RTI. */
   void process_deleted_objects();

   /*! @brief Start the federation save as soon as possible.
    *  @param file_name Checkpoint file name. */
   void start_federation_save( char const *file_name );

   /*! @brief Start the Federation save at the specified simulation time.
    *  @param freeze_sim_time Simulation time to freeze.
    *  @param file_name       Checkpoint file name. */
   void start_federation_save_at_sim_time( double freeze_sim_time, char const *file_name );

   /*! @brief Start the Federation save at the specified scenario time.
    *  @param freeze_scenario_time Scenario time to freeze.
    *  @param file_name            Checkpoint file name. */
   void start_federation_save_at_scenario_time( double freeze_scenario_time, char const *file_name );

   /*! @brief Encode/setup the checkpoint data structures. */
   virtual void encode_checkpoint();

   /*! @brief Restore the state of this class from the Trick checkpoint. */
   virtual void decode_checkpoint();

   /*! @brief Clear/release the memory used for the checkpoint data structures. */
   virtual void free_checkpoint();

   /*! @brief Publishes Object & Interaction classes and their member data. */
   void publish();

   /*! @brief Unpublish the Object & Interaction classes. */
   void unpublish();

   /*! @brief Subscribe to Object and Interaction classes and their member data. */
   void subscribe();

   /*! @brief Unubscribe from the Object and Interaction classes. */
   void unsubscribe();

   /*! @brief Publish and Subscribe to Object and Interaction classes and their
    * member data. */
   void publish_and_subscribe();

   /*! @brief Reserve the RTI object instance names with the RTI, but only for
    * the objects that are locally owned. */
   void reserve_object_names_with_RTI();

   /*! @brief Waits for the reservation of the RTI object instance names for the
    * locally owned objects. */
   void wait_for_reservation_of_object_names();

   /*! @brief Waits for the discovery of object instances. */
   void wait_for_discovery_of_objects();

   /*! @brief Checks if any object discoveries have occurred.
    *  @return True if this is a rejoining federate. */
   bool is_this_a_rejoining_federate();

   /*! @brief Creates an RTI object instance and registers it with the RTI,
    * but only for the objects that are locally owned. */
   void register_objects_with_RTI();

   /* @brief Waits for the registration of all the required RTI object
    * instances with the RTI. */
   void wait_for_registration_of_required_objects();

   /*! @brief Sets the RTI run-time type IDs/handles for the object, attributes,
    * interactions, and parameters. */
   void setup_all_RTI_handles();

   /*! @brief Sets the RTI run-time type IDs/handles for the object and attributes.
    *  @param data_obj_count Number of objects.
    *  @param data_objects   Simulation TrickHLA Objects. */
   void setup_object_RTI_handles( int const data_obj_count,
                                  Object   *data_objects );

   /*! @brief Sets the RTI run-time type IDs/handles for the specified
    * interactions and parameters.
    *  @param interactions_counter Number of interactions.
    *  @param in_interactions      Simulation TrickHLA Interaction objects. */
   void setup_interaction_RTI_handles( int const    interactions_counter,
                                       Interaction *in_interactions );

   /*! @brief Set up the Trick ref-attributes for the user specified objects,
    * attributes, interactions, and parameters. */
   void setup_all_ref_attributes();

   /*! @brief Setup the preferred order (TSO or RO) for all the object
    * attributes and interactions. */
   void setup_preferred_order_with_RTI();

   /*! @brief Requesting an attribute value update for the given object
    *  instance and attributes.
    *  @param theObject HLA object instance handle.
    *  @param theAttributes HLA attribute handle set. */
   void provide_attribute_update( RTI1516_NAMESPACE::ObjectInstanceHandle const &theObject,
                                  RTI1516_NAMESPACE::AttributeHandleSet const   &theAttributes );

   /*! @brief Get the TrickHLA::Object count.
    *  @return The number of registered TrickHLA::Object instances. */
   int get_object_count() const
   {
      return obj_count;
   }

   /*! @brief Get the array of TrickHLA::Object instances.
    *  @return Array of TrickHLA::Object instances. */
   Object *get_objects()
   {
      return objects;
   }

   /*! @brief Get the number of TrickHLA::Interactions.
    *  @return The number of TrickHLA::Interaction instances. */
   int get_interaction_count() const
   {
      return inter_count;
   }

   /*! @brief Get the array containing the TrickHLA::Interaction instances.
    *  @return Array of TrickHLA::Interaction instances. */
   Interaction *get_interactions()
   {
      return interactions;
   }

   /*! @brief Reset the manager as initialized. */
   void reset_mgr_initialized()
   {
      mgr_initialized            = false;
      federate_has_been_restored = true;
   }

   /*! @brief Check if the federate has been restored.
    *  @return True if the federate has been restored. */
   bool has_federate_been_restored() const
   {
      return federate_has_been_restored;
   }

   /*! @brief Set the execution configuration object.
    *  @param exec_config Pointer to the associated execution configuration object. */
   void set_execution_configuration( ExecutionConfigurationBase *exec_config )
   {
      execution_control->set_execution_configuration( exec_config );
   }

   /*! @brief Get the execution configuration object.
    *  @return Pointer to the associated execution configuration object. */
   ExecutionConfigurationBase *get_execution_configuration()
   {
      return execution_control->get_execution_configuration();
   }

   /*! @brief Test is an execution configuration object is used.
    *  @return True if an execution configuration object is used. */
   bool is_execution_configuration_used()
   {
      return execution_control->is_execution_configuration_used();
   }

   /*! @brief Check if federate is shutdown function was called.
    *  @return True if the manager is shutting down the federate. */
   bool is_shutdown_called() const;

   //
   // Private data.
   //
  private:
   ItemQueue interactions_queue; ///< @trick_io{**} Interactions queue.

   int              check_interactions_count; ///< @trick_units{--} Number of checkpointed interactions
   InteractionItem *check_interactions;       ///< @trick_units{--} checkpoint-able version of interactions_queue

   bool rejoining_federate; ///< @trick_units{--} Internal flag to indicate if the federate is rejoining the federation.
   bool restore_determined; ///< @trick_io{**} Internal flag to indicate that the restore status has been determined.
   bool restore_federate;   ///< @trick_io{**} Internal flag to indicate if the federate is to be restored

   bool mgr_initialized; ///< @trick_units{--} Internal flag to indicate Manager is initialized.

   MutexLock obj_discovery_mutex; ///< @trick_io{**} Mutex to lock thread over critical code sections.

   ObjectInstanceMap object_map; ///< @trick_io{**} Map of all the Objects this federate uses, the Key is the object instance-handle.

   TrickHLAObjInstanceNameIndexMap obj_name_index_map; ///< @trick_io{**} Map of object instance names to array index.

   bool federate_has_been_restored; ///< @trick_io{**} Federate has been restored. do not reserve the object names again!

   Federate *federate; ///< @trick_units{--} Associated TrickHLA Federate.

   ExecutionControlBase *execution_control; /**< @trick_units{--}
      Execution control object. This has to point to an allocated execution
      control class that inherits from the ExecutionControlBase interface
      class. For instance SRFOM::ExecutionControl. */

   //
   // Private member functions.
   //
  public:
   /*! @brief Initializes the federation execution control scheme, which must
    * occur after the TrickHLA::Federate and TrickHLA::FedAmb has been
    * initialized. */
   // void initialize_execution_control();

   /*! @brief Check to see if this federate is to be restored.
    *  @return federate restore state. */
   bool is_restore_determined() const
   {
      return restore_determined;
   }

   /*! @brief Set the federate to be restored state.
    *  @param state Restore state of the federate. */
   void set_restore_determined( bool state )
   {
      restore_determined = state;
      return;
   }

   /*! @brief Check to see if this is a restored federate.
    *  @return True is this is a restored federate, false otherwise. */
   bool is_restore_federate() const
   {
      return restore_federate;
   }

   /*! @brief Mark if this is a restored federate.
    *  @param state True is federate is restored, false otherwise. */
   void set_restore_federate( bool state )
   {
      restore_federate = state;
      return;
   }

   /*! @brief Set up the Trick ref-attributes for the user specified objects
    * and attributes.
    * @param data_obj_count Number of objects.
    * @param data_objects   Simulation TrickHLA::Objects. */
   void setup_object_ref_attributes( int const data_obj_count,
                                     Object   *data_objects );

   /*! @brief Set up the Trick ref-attributes for the user specified
    * interactions and parameters. */
   void setup_interaction_ref_attributes();

   /*! @brief Set all the object instance handles by using the object instance names. */
   void set_all_object_instance_handles_by_name();

   /*! @brief Set object instance handles by using the object instance names.
    *  @param data_obj_count Number of objects.
    *  @param data_objects   Simulation TrickHLA Objects. */
   void set_object_instance_handles_by_name( int const data_obj_count,
                                             Object   *data_objects );

   /*! @brief Returns the first object that matches the specified Object-Class,
    * object instance name, and is not registered, i.e. the instance ID == 0.
    *  @return TrickHLA Object
    *  @param theObjectClass        RTI Object class type.
    *  @param theObjectInstanceName Object instance name. */
   Object *get_unregistered_object(
      RTI1516_NAMESPACE::ObjectClassHandle const &theObjectClass,
      std::wstring const                         &theObjectInstanceName );

   /*! @brief Returns the first object that is remotely owned, has the same
    * Object-Class, is not registered, and does not have an Object Instance
    * Name associated with it.
    *  @return The associated TrickHLA::Object instance; otherwise NULL.
    *  @param theObjectClass RTI Object class type. */
   Object *get_unregistered_remote_object(
      RTI1516_NAMESPACE::ObjectClassHandle const &theObjectClass );

   // Ownership
   /*! @brief Pull ownership from the other federates if the pull ownership
    *  flag has been enabled. */
   void pull_ownership();

   /*! @brief Blocking function call to pull ownership of the named object
    *  instance at initialization.
    *  @param obj_instance_name Object instance name to pull ownership
    *  of for all attributes. */
   void pull_ownership_at_init( char const *obj_instance_name );

   /*! @brief Blocking function call to pull ownership of the named object
    *  instance at initialization.
    *  @param obj_instance_name Object instance name to pull ownership
    *  of for all attributes.
    *  @param attribute_list Comma separated list of attributes. */
   void pull_ownership_at_init( char const *obj_instance_name,
                                char const *attribute_list );

   /*! @brief Blocking function call to wait to handle the remote request to
    *  Pull ownership object attributes to this federate.
    *  @param obj_instance_name Object instance name to handle the remote
    *  pulled ownership attributes from. */
   void handle_pulled_ownership_at_init( char const *obj_instance_name );

   /*! @brief Pull ownership from the other federates when this federate has
    * rejoined the Federation. */
   void pull_ownership_upon_rejoin();

   /*! @brief Push ownership to the other federates if the push ownership flag
    * has been enabled. */
   void push_ownership();

   /*! @brief Blocking function call to push ownership of all the locally
    *  owned object attributes.
    *  @param obj_instance_name Object instance name to push ownership
    *  of for all attributes. */
   void push_ownership_at_init( char const *obj_instance_name );

   /*! @brief Blocking function call to push ownership of the named object
    *  instance at initialization.
    *  @param obj_instance_name Object instance name to push ownership
    *  of for all attributes.
    *  @param attribute_list Comma separated list of attribute FOM names. */
   void push_ownership_at_init( char const *obj_instance_name,
                                char const *attribute_list );

   /*! @brief Blocking function call to wait to handle the remote request to
    *  Push ownership object attributes to this federate.
    *  @param obj_instance_name Object instance name to handle the remote
    *  pushed ownership attributes from. */
   void handle_pushed_ownership_at_init( char const *obj_instance_name );

   /*! @brief Grant any request to pull the ownership. */
   void grant_pull_request();

   /*! @brief Release ownership if we have a request to divest. */
   void release_ownership();

   /*! @brief Tell the federate to initiate a save announce with the
    * user-supplied checkpoint name set for the current frame.
    *  @param file_name Checkpoint file name. */
   void initiate_federation_save( char const *file_name );

   //
   // Checkpoint / clear / restore any interactions
   //
   /*! @brief Decodes interactions queue into an InteractionItem linear array. */
   void encode_checkpoint_interactions();

   /*! @brief Decodes checkpoint InteractionItem linear arrays back into the
    * main interaction queue. */
   void decode_checkpoint_interactions();

   /*! @brief Free/clear InteractionItem checkpoint linear array. */
   void free_checkpoint_interactions();

   /*! @brief Echoes the contents of checkpoint InteractionItem linear array. */
   void print_checkpoint_interactions();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Manager class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Manager( Manager const &rhs );
   /*! @brief Assignment operator for Manager class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Manager &operator=( Manager const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_MANAGER_HH
