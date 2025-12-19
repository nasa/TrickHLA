/*!
@file TrickHLA/Object.hh
@ingroup TrickHLA
@brief This class represents an HLA object that is managed by Trick.

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
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Conditional.cpp}
@trick_link_dependency{../../source/TrickHLA/ElapsedTimeStats.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/LagCompensation.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/MutexProtection.cpp}
@trick_link_dependency{../../source/TrickHLA/ObjectDeletedHandler.cpp}
@trick_link_dependency{../../source/TrickHLA/OwnershipHandler.cpp}
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/TrickHLA/ReflectedAttributesQueue.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/time/Int64Interval.cpp}
@trick_link_dependency{../../source/TrickHLA/time/Int64Time.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2006, --, DSES Created Object}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_OBJECT_HH
#define TRICKHLA_OBJECT_HH

// System includes.
#include <map>
#include <string>

// TrickHLA inlcudes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/BasicClock.hh"
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/CompileConfig.hh" // NOLINT(misc-include-cleaner)
#include "TrickHLA/ElapsedTimeStats.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/ReflectedAttributesQueue.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Interval.hh"
#include "TrickHLA/time/Int64Time.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Handle.h"
#include "RTI/RTI1516.h"
#include "RTI/Typedefs.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

// Special handling of SWIG limitations for forward declarations.
#ifdef SWIG
#   include "TrickHLA/Attribute.hh"
#else
namespace TrickHLA
{
// NOTE: This forward declaration of TrickHLA::Attribute is here to go with
// the #ifdef SWIG include. Normally, it would go with the other forward
// declarations below.
class Attribute;
} // namespace TrickHLA
#endif // SWIG

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Manager;
class Federate;
class Conditional;
class Packing;
class OwnershipHandler;
class ObjectDeletedHandler;
class LagCompensation;

class Object : public CheckpointConversionBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Object();

   //----------------------------- USER VARIABLES -----------------------------
   // Public data in this section are for either use within a users simulation
   // or must be configured by the user.
  public:
   // For use by a user to determine when the data has changed. Clearing this
   // flag to false is up to the user.
   bool data_changed; ///< @trick_units{--} Flag to indicate data changes.

   // The variables below this point are configured by the user in either the
   // input or modified-data file.
   std::string name;          ///< @trick_units{--} Object Instance Name.
   bool        name_required; ///< @trick_units{--} True (default) to require an object instance name be specified by you, or false to use the instance name automatically assigned by the RTI.

   std::string FOM_name; ///< @trick_units{--} FOM name for the object.

   bool create_HLA_instance; ///< @trick_units{--} Set to true to create an HLA named instance of this object.

   bool required; ///< @trick_units{--} Flag indicating object is required at federation start ( default: true )

   bool blocking_cyclic_read; ///< @trick_units{--} True to block in receive_cyclic_data() for data to be received.

   std::string thread_ids; ///< @trick_units{--} Comma separated list of Trick child thread IDs associated to this object.

   int        attr_count; ///< @trick_units{--} Number of object attributes.
   Attribute *attributes; ///< @trick_units{--} Array of object attributes.

   LagCompensation    *lag_comp;      ///< @trick_units{--} Lag compensation object.
   LagCompensationEnum lag_comp_type; ///< @trick_units{--} Type of lag compensation.

   Packing *packing; ///< @trick_units{--} Data pack/unpack object.

   OwnershipHandler *ownership; ///< @trick_units{--} Manages attribute ownership.

   ObjectDeletedHandler *deleted; ///< @trick_units{--} Object Deleted callback object.

   Conditional *conditional; ///< @trick_units{--} Handler for a conditional attribute

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Object class. */
   Object();
   /*! @brief Destructor for the TrickHLA Object class. */
   virtual ~Object();

   //-----------------------------------------------------------------
   // Post-constructor initialization stuff
   //-----------------------------------------------------------------
   /*! @brief Initializes the TrickHLA Object.
    *  @param trickhla_mgr The TrickHLA::Manager instance. */
   virtual void initialize( Manager *manager );

   /*! @brief Gets the a pointer to our federate.
    *  @return Pointer to TrickHLA::Federate instance. */
   Federate *get_federate();

   //-----------------------------------------------------------------
   // HLA
   //-----------------------------------------------------------------
   /*! @brief Publishes Object attributes. */
   void publish_object_attributes();

   /*! @brief Unpublishes all object attributes. */
   void unpublish_all_object_attributes();

   /*! @brief Subscribe to Object attributes. */
   void subscribe_to_object_attributes();

   /*! @brief Unsubscribe from all the Object attributes. */
   void unsubscribe_all_object_attributes();

   /*! @brief Reserves a unique object instance name with the RTI. */
   void reserve_object_name_with_RTI();

   /*! @brief Waits for the reservation of the HLA object instance name with the RTI. */
   void wait_for_object_name_reservation();

   /*! @brief Creates an HLA object instance and registers it with the RTI,
    * but only if we own it locally. */
   void register_object_with_RTI();

   /*! @brief Waits for the registration of this HLA object instance with the RTI. */
   void wait_for_object_registration();

   /*! @brief Setup the preferred order for the locally owned attributes. */
   void setup_preferred_order_with_RTI();

   /*! @brief Sets the core job cycle time used by the multi-rate attributes.
    *  @param cycle_time The core job cycle time in seconds. */
   void set_core_job_cycle_time( double const cycle_time );

   /*! @brief Marks this object as deleted from the RTI and sets all attributes as non-local. */
   void remove_object_instance();

   /*! @brief This object instance has been deleted from the RTI, so process
    * the delete action by calling the users delete notification callback. */
   void process_deleted_object();

   //-----------------------------------------------------------------
   // I/O with remote federates.
   // If we own the data, then we can send it out to other federates.
   // (HLA refers to this as generating "updates".)  If we do not
   // own the data, then we can receive new values from other federates.
   // (HLA refers to this as "reflecting" the remote values.)
   //
   // NOTES:
   // Our send methods effectively do nothing if we don't own the data.
   // Likewise, the receive method effectively does nothing if we do
   // own the data.
   //
   // There is copying involved. Before the data is sent, the specified
   // data source is copied to a byte buffer and a byteswap is performed
   // if the byteorder of the local data does not match the byteorder used
   // to send the data through the RTI.
   //
   // Likewise incoming data are copied to the internal byte buffer. Only
   // when the data is requested will any needed byteswap will be performed.
   //-----------------------------------------------------------------
   /*! @brief Send the requested attribute value updates using the current simulation and lookahead times. */
   void send_requested_data();

   /*! @brief Send the requested attribute value updates.
    *  @param update_time The time to HLA Logical Time to update the attributes to. */
   void send_requested_data( Int64Time const &update_time );

   /*! @brief Send the cyclic and requested attribute value updates.
    *  @param update_time The time to HLA Logical Time to update the attributes to. */
   void send_cyclic_and_requested_data( Int64Time const &update_time );

   /*! @brief Send the zero-lookahead attribute value updates.
    *  @param update_time The time to HLA Logical Time to update the attributes to. */
   void send_zero_lookahead_and_requested_data( Int64Time const &update_time );

   /*! @brief Send the blocking I/O attribute value updates as Receive Order. */
   void send_blocking_io_data();

   /*! @brief Send initialization data to remote HLA federates. */
   void send_init_data();

   /*! @brief Receive initialization data from remote Federates. */
   void receive_init_data();

   /*! @brief Handle the received cyclic data. */
   void receive_cyclic_data();

   /*! @brief Handle the received zero-lookaehad data. */
   void receive_zero_lookahead_data();

   /*! @brief Handle the received blocking I/O data. */
   void receive_blocking_io_data();

   /*! @brief Request an update to the attributes for this object. */
   void request_attribute_value_update();

   /*! @brief Requesting an attribute update for the specified attributes.
    *  @param theAttributes The specified attributes. */
   void provide_attribute_update( RTI1516_NAMESPACE::AttributeHandleSet const &theAttributes );

   /*! @brief Enqueue the reflected attributes.
    *  @param theAttributes Attributes data. */
   void enqueue_data( RTI1516_NAMESPACE::AttributeHandleValueMap const &theAttributes );

   /*! @brief This function decoded the received encoded attributes.
    *  @param theAttributes Attributes data.
    *  @return True if successfully decoded data, false otherwise. */
   bool decode( RTI1516_NAMESPACE::AttributeHandleValueMap &theAttributes );

   /*! @brief Remove this object instance from the RTI/Federation. */
   void remove();

   //-----------------------------------------------------------------
   // Ownership related stuff
   // Do WE own this data or does someone else own it?
   //-----------------------------------------------------------------

   /*! @brief This function releases ownership of the attributes for this object. */
   void release_ownership();

   /*! @brief This function pulls ownership for this object. */
   void pull_ownership();

   /*! @brief Pull ownership of the named object instance at initialization.
    *  @param attribute_list Comma separated list of attributes FOM names. */
   void pull_ownership_at_init( std::string const &attribute_list );

   /*! @brief Wait to handle the remote request to Pull ownership object
    *  attributes to this federate. */
   void handle_pulled_ownership_at_init();

   /*! @brief This function pulls ownership for all published attributes when
    * the federate rejoins an already running federation. */
   void pull_ownership_upon_rejoin();

   /*! @brief This function grants a pull request for this object. */
   void grant_pull_request();

   /*! @brief This function pushes ownership for this object. */
   void push_ownership();

   /*! @brief Push ownership of the named object instance at initialization.
    *  @param attribute_list Comma separated list of attributes FOM names. */
   void push_ownership_at_init( std::string const &attribute_list );

   /*! @brief Wait to handle the remote request to Push ownership object
    *  attributes to this federate. */
   void handle_pushed_ownership_at_init();

   /*! @brief This function grants a previously "recorded" push request for
    * this object. */
   void grant_push_request();

   /*! @brief This function starts a thread to service the push request grant. */
   void grant_push_request_pthread();

   /*! @brief This function handles the RTI call for the negotiated attribute
    * ownership divestiture.
    *  @param attr_hdl_set Attributes. */
   void negotiated_attribute_ownership_divestiture( RTI1516_NAMESPACE::AttributeHandleSet *attr_hdl_set );

   /*! @brief Setup the checkpoint data structures. */
   virtual void encode_checkpoint();

   /*! @brief If an ownership_transfer object has been created by the user,
    * trigger it's restore() method is re-establish the pull / push
    * AttributeOwnershipMaps. */
   virtual void decode_checkpoint();

   /*! @brief Clears out the push / pull checkpoint-able queues. */
   virtual void free_checkpoint();

   // Instance methods

   /*! @brief Get the object instance name.
    *  @return Object instance name. */
   std::string const &get_name() const
   {
      return name;
   }

   /*! @brief Check if an object instance name is required.
    *  @return True is the object instance name is required. */
   bool is_name_required() const
   {
      return name_required;
   }

   /*! @brief Check if an object instance name is registered.
    *  @return True is the object instance name is registered. */
   bool is_name_registered() const
   {
      return name_registered;
   }

   /*! @brief Set the name registration status as true (registered). */
   void set_name_registered()
   {
      this->name_registered = true;
   }

   /*! @brief Set the name registration status as false (not registered). */
   void set_name_unregistered()
   {
      this->name_registered = false;
   }

   /*! @brief Get the FOM name for this object.
    *  @return The FOM name for this object. */
   std::string const &get_FOM_name() const
   {
      return FOM_name;
   }

   /*! @brief Get the HLA Object class handle for this object.
    *  @return The HLA ObjectClassHandle for this object. */
   RTI1516_NAMESPACE::ObjectClassHandle get_class_handle() const
   {
      return class_handle;
   }

   /*! @brief Set the HLA Object class handle for this object.
    *  @param id The HLA ObjectClassHandle for this object. */
   void set_class_handle( RTI1516_NAMESPACE::ObjectClassHandle const &id )
   {
      this->class_handle = id;
   }

   /*! @brief Check if the HLA Object instance handle is valid for this object.
    *  @return True if the HLA ObjectIntanceHandle is valid for the object instance. */
   bool is_instance_handle_valid() const
   {
      return ( instance_handle.isValid() );
   }

   /*! @brief Get the HLA Object instance handle for this object instance.
    *  @return The HLA ObjectInstanceHandle for this object instance. */
   RTI1516_NAMESPACE::ObjectInstanceHandle get_instance_handle() const
   {
      return instance_handle;
   }

   /*! @brief Set the HLA Object instance handle for this object instance.
    *  @param id The HLA ObjectInstanceHandle for this object instance. */
   void set_instance_handle( RTI1516_NAMESPACE::ObjectInstanceHandle const &id )
   {
      this->instance_handle = id;
   }

   /*! @brief Check if the HLA Object instance handle is valid for this object.
    *  @param id The HLA ObjectClassHandle to test.
    *  @return True if the HLA ObjectIntanceHandle matches the one for this object instance. */
   bool is_instance_handle( RTI1516_NAMESPACE::ObjectInstanceHandle const &id ) const
   {
      return ( id == instance_handle );
   }

   /*! @brief Set the HLA Object instance handle and name for this object instance.
    *  @param id The HLA ObjectInstanceHandle for this object instance.
    *  @param instance_name The associated object instance name. */
   void set_instance_handle_and_name( RTI1516_NAMESPACE::ObjectInstanceHandle const &id,
                                      std::wstring const                            &instance_name )
   {
      set_instance_handle( id );
      std::string instance_name_str;
      StringUtilities::to_string( instance_name_str, instance_name );
      set_name( instance_name_str );
      set_name_registered();
   }

   /*! @brief Check if the object instance has been created.
    *  @return True if the object instance has been created. */
   bool is_create_HLA_instance() const
   {
      return this->create_HLA_instance;
   }

   /*! @brief Set the object instance creation status.
    *  @param create The associated object instance creation status; True
    *  if the object has been created, otherwise False. */
   void set_create_HLA_instance( bool create )
   {
      create_HLA_instance = create;
   }

   /*! @brief Check if the object is a required object instance.
    *  @return True if this is a required object instance. */
   bool is_required() const
   {
      return this->required;
   }

   /*! @brief Set this object instance as required.
    *  @param required_obj True if object instance is required, false otherwise. */
   void set_required( bool const required_obj )
   {
      this->required = required_obj;
   }

   /*! @brief Stops publishing data for the object attributes by setting the
    * attribute publish state to false. */
   void stop_publishing_attributes();

   /*! @brief Stops subscribing to object attribute data by setting the
    * attribute subscribe state to false. */
   void stop_subscribing_attributes();

   /*! @brief Check an attribute update has been requested for an attribute
    * associated with this object instance.
    *  @return True is an attribute update has been requested. */
   bool is_attribute_update_requested() const
   {
      return this->attr_update_requested;
   }

   /*! @brief Determines if any attribute is published.
    *  @return True for any published attribute. */
   bool any_attribute_published();

   /*! @brief Determines if any attribute is being subscribed to.
    *  @return True any subscribed attribute. */
   bool any_attribute_subscribed();

   /*! @brief Determines if any attribute is locally owned.
    *  @return True for any locally owned attribute. */
   bool any_locally_owned_attribute();

   /*! @brief Determines if any attribute is locally owned and published.
    *  @return True for any locally owned and published attribute. */
   bool any_locally_owned_published_attribute();

   /*! @brief Determines if any attribute is locally owned and published for
    * the given attribute configuration.
    *  @return True for any locally owned and published attribute.
    *  @param attr_config Attribute configuration. */
   bool any_locally_owned_published_attribute( DataUpdateEnum const attr_config );

   /*! @brief Determines if any attribute is locally owned, published, has
    * a cycle-time that is ready for a cyclic send or is requested for update.
    *  @return True for any locally owned, published attribute with a sub-rate that is ready or requested for update. */
   bool any_locally_owned_published_cyclic_data_ready_or_requested_attribute();

   /*! @brief Determines if any attribute is locally owned, published, has
    * a zero-lookahead that is ready to be sent or is requested for update.
    *  @return True for any locally owned, published attribute or is requested for update. */
   bool any_locally_owned_published_zero_lookahead_or_requested_attribute();

   /*! @brief Determines if any attribute is locally owned, published, is
    * configured for blocking I/O and is ready to be sent or is requested for
    * update.
    *  @return True for any locally owned, published attribute or is requested for update. */
   bool any_locally_owned_published_blocking_io_attribute();

   /*! @brief Determines if any attribute is locally owned, published, and has
    * a cycle-time that is ready for a cyclic send.
    *  @return True for any locally owned and published attribute with a sub-rate that is ready. */
   bool any_locally_owned_published_cyclic_data_ready_attribute();

   /*! @brief Determines if any attribute update is locally owned and published
    * at initialization.
    *  @return True for any locally owned and published initialization attribute. */
   bool any_locally_owned_published_init_attribute()
   {
      return any_locally_owned_published_attribute( CONFIG_INITIALIZE );
   }

   /*! @brief Determines if any attribute update is locally owned and published
    * for the attribute value update.
    *  @return True for any locally owned and published attribute. */
   bool any_locally_owned_published_requested_attribute();

   /*! @brief Determines if any attribute is remotely owned and subscribed to.
    *  @return True for any remotely owned and subscribed attribute. */
   bool any_remotely_owned_subscribed_attribute();

   /*! @brief Determines if any attribute specified is remotely owned and
    * subscribed to for the given attribute configuration.
    *  @return True for any remotely owned, subscribed attribute.
    *  @param attr_config Attribute configuration. */
   bool any_remotely_owned_subscribed_attribute( DataUpdateEnum const attr_config );

   /*! @brief Determines if any cyclically updated attributes are remotely
    * owned and subscribed.
    *  @return True if there are any remotely owned, subscribed cyclically
    *  updated attributes. */
   bool any_remotely_owned_subscribed_cyclic_attribute()
   {
      return any_remotely_owned_subscribed_attribute( CONFIG_CYCLIC );
   }

   /*! @brief Determines if any zero-lookahead updated attributes are remotely
    * owned and subscribed.
    *  @return True if there are any remotely owned, subscribed zero-lookahead
    *  updated attributes. */
   bool any_remotely_owned_subscribed_zero_lookahead_attribute()
   {
      return any_remotely_owned_subscribed_attribute( CONFIG_ZERO_LOOKAHEAD );
   }

   /*! @brief Determines if any blocking I/O updated attributes are remotely
    * owned and subscribed.
    *  @return True if there are any remotely owned, subscribed blocking I/O
    *  updated attributes. */
   bool any_remotely_owned_subscribed_blocking_io_attribute()
   {
      return any_remotely_owned_subscribed_attribute( CONFIG_BLOCKING_IO );
   }

   /*! @brief Determines if any initialization updated attributes are remotely
    * owned and subscribed.
    *  @return True if there are any remotely owned, subscribed updated
    *  attributes at initialization. */
   bool any_remotely_owned_subscribed_init_attribute()
   {
      return any_remotely_owned_subscribed_attribute( CONFIG_INITIALIZE );
   }

   /*! @brief Turn off local flag in all attributes. */
   void mark_all_attributes_as_nonlocal();

   /*! @brief Used by the TrickHLA extension to determine if the object data changed.
    *  @return True if object data has changed. */
   bool is_changed()
   {
      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &receive_mutex );

      if ( !changed && !reflected_attributes_queue.empty() ) {
         // The 'changed' flag is set when the data is decoded.
         decode( reflected_attributes_queue.front() );
         reflected_attributes_queue.pop();
      }
      return changed;
   }

   /*! @brief Mark the data as changed, and notify any waiting thread that
    * there is a change. */
   void mark_changed();

   /*! @brief Mark the data as unchanged, and clear the change flag for all
    * the attributes as well. */
   void mark_unchanged();

   /*! @brief Set to unblocking cyclic reads and notify any waiting threads. */
   void set_to_unblocking_cyclic_reads();

   /*! @brief Notify any waiting threads of a change in attribute ownership,
    * which could affect blocking reads. */
   void set_attribute_ownership_acquired();

   /*! @brief Set the Lag Compensation type for object attribute updates.
    *  @param lag_type Desired lag compensation type. */
   void set_lag_compensation_type( LagCompensationEnum lag_type )
   {
      this->lag_comp_type = lag_type;
   }

   /*! @brief Get the current Lag Compensation type for this object instance
    * attribute updates.
    *  @return The current Lag Compensation type. */
   LagCompensationEnum get_lag_compensation_type() const
   {
      return this->lag_comp_type;
   }

   /*! @brief Set the ownership divestiture requested flag.
    *  @param request The desired divesiture request state. */
   void set_divest_requested( bool request )
   {
      MutexProtection auto_unlock_mutex( &ownership_mutex );
      this->divest_requested = request;
   }

   /*! @brief Set ownership pull requested flag.
    *  @param request The desired pull request state. */
   void set_pull_requested( bool request )
   {
      MutexProtection auto_unlock_mutex( &ownership_mutex );
      this->pull_requested = request;
   }

   /*! @brief Return a copy of the federate's lookahead time.
    *  @return Lookahead time interval. */
   Int64Interval get_lookahead() const;

   /*! @brief Get the currently granted federation HLA logical time.
    *  @return A copy of the granted HLA logical time. */
   Int64Time get_granted_time() const;

   /*! @brief Gets the attribute for the given HLA Attribute-Handle.
    *  @return Associated TrickHLA::Attribute.
    *  @param attr_handle Attribute ID. */
   Attribute *get_attribute( RTI1516_NAMESPACE::AttributeHandle const &attr_handle );

   /*! @brief Gets the attribute for the given FOM name.
    *  @return Associated TrickHLA::Attribute.
    *  @param attr_FOM_name Attribute FOM name. */
   Attribute *get_attribute( std::string const &attr_FOM_name );

   /*! @brief Get the count of the number of attributes associated with this object.
    *  @return The number of attributes associated with this object. */
   int get_attribute_count() const
   {
      return attr_count;
   }

   /*! @brief Get the list of attributes associated with this object.
    *  @return The array of attributes associated with this object. */
   Attribute *get_attributes()
   {
      return attributes;
   }

   /*! @brief Build the attribute map, which will be used for quickly looking
    * up an attribute given its AttributeHandle. */
   void build_attribute_map();

   /*! @brief Get the attribute FOM names.
    *  @return A vector of strings containing the attribute FOM names. */
   VectorOfStrings const get_attribute_FOM_names() const // cppcheck-suppress [returnByReference]
   {
      return attribute_FOM_names;
   }

   /*! @brief Check if federate is shutdown function was called.
    *  @return True if the manager is shutting down the federate. */
   bool is_shutdown_called() const;

   /*! @brief Create a name value pair set, aka attribute handle value pair,
    * for the attributes that were requested for this object. */
   void create_requested_attribute_set();

   /*! @brief Create a name value pair set, aka attribute handle value pair,
    * for the attributes of this object.
    * @param required_config Attribute configuration required in order to send data
    * @param include_requested True to also included requested attributes */
   void create_attribute_set( DataUpdateEnum const required_config, bool const include_requested );

   /*! @brief Initialize the thread ID array based on the users 'thread_ids' input.*/
   void initialize_thread_ID_array();

   /*! @brief Determine if this object is associated to the specified thread ID.
    * @return True associated to this thread ID.
    * @param thread_id Trick thread ID. */
   bool is_thread_associated( unsigned int const thread_id );

   unsigned int thread_ids_array_count; ///< @trick_units{--} Size of the thread IDs array.
   bool        *thread_ids_array;       ///< @trick_units{--} Array index is the thread ID and the value is true if the thread is associated to this object.

   bool process_object_deleted_from_RTI; ///< @trick_units{--} Flag that is true when to process the object has been deleted from the RTI.
   bool object_deleted_from_RTI;         ///< @trick_units{--} Flag that is true when this object has been deleted from the RTI.

   MutexLock push_mutex;      ///< @trick_io{**} Mutex to lock thread over push attribute ownership sections.
   MutexLock ownership_mutex; ///< @trick_io{**} Mutex to lock thread over attribute ownership code sections.
   MutexLock send_mutex;      ///< @trick_io{**} Mutex to lock thread over send data sections.
   MutexLock receive_mutex;   ///< @trick_io{**} Mutex to lock thread over receive data sections.

  protected:
   /*! @brief Gets the RTI Ambassador.
    *  @return Pointer to associated HLA RTIambassador instance. */
   RTI1516_NAMESPACE::RTIambassador *get_RTI_ambassador();

   BasicClock clock; ///< @trick_units{--} Clock time object.

   bool name_registered; ///< @trick_units{--} True if the object instance name is registered.

   bool changed; ///< @trick_units{--} Flag indicating the data has changed.

   bool attr_update_requested; ///< @trick_units{--} Flag to indicate an attribute updated was requested by another federate.

   bool removed_instance; ///< @trick_units{--} Flag to indicate if object instance was removed from RTI.

   bool first_blocking_cyclic_read; ///< @trick_units{--} True if this is the first call to receive_cyclic_data for data to be received.

   bool any_attribute_FOM_specified_order; ///< @trick_units{--} True if any attribute is the FOM specified order.
   bool any_attribute_timestamp_order;     ///< @trick_units{--} True if any attribute is timestamp order.

   bool pull_requested;     ///< @trick_units{--} Has someone asked to own us?
   bool divest_requested;   ///< @trick_units{--} Are we releasing ownership?
   bool ownership_acquired; ///< @trick_units{--} True when attribute ownership changed.

   VectorOfStrings attribute_FOM_names; ///< @trick_io{**} String array containing the Attribute FOM names.

   Manager *manager; ///< @trick_units{--} Reference to the TrickHLA Manager.

   RTI1516_NAMESPACE::RTIambassador *rti_ambassador; ///< @trick_io{**} Reference to the RTI ambassador.

   RTI1516_NAMESPACE::AttributeHandleValueMap *attribute_values_map; ///< @trick_io{**} Map of attributes that will be sent as an update to other federates.

   ReflectedAttributesQueue reflected_attributes_queue; ///< @trick_io{**} Queue of reflected attributes.

   AttributeMap thla_attribute_map; ///< @trick_io{**} Map of the Attribute's, key is the AttributeHandle.

   RTI1516_NAMESPACE::ObjectClassHandle    class_handle;    ///< @trick_io{**} HLA Object Class handle.
   RTI1516_NAMESPACE::ObjectInstanceHandle instance_handle; ///< @trick_io{**} HLA Object Instance handle.

  public:
#ifdef TRICKHLA_CHECK_SEND_AND_RECEIVE_COUNTS
   uint64_t send_count;    ///< @trick_units{--} Number of times data from this object was sent.
   uint64_t receive_count; ///< @trick_units{--} Number of times data for this object was received.
#endif

   ElapsedTimeStats elapsed_time_stats; ///< @trick_units{--} Statistics of elapsed times between cyclic data reads.

  private:
   /*! @brief Sets the new value of the name attribute.
    *  @param new_name New name for the object instance. */
   void set_name( std::string const &new_name );

   /*! @brief Set the name of the object and mark it as changed.
    *  @param new_name The new name of the object. */
   void set_name_and_mark_changed( std::string const &new_name )
   {
      set_name( new_name );
      mark_changed();
   }

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Object class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Object( Object const &rhs );
   /*! @brief Assignment operator for Object class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Object &operator=( Object const &rhs );
};

// The Key is the object instance handle.
typedef std::map< RTI1516_NAMESPACE::ObjectInstanceHandle, Object * > ObjectInstanceMap; ///< trick_io{**} Map of TrickHLA objects.

typedef struct {
   Object                                *trick_hla_obj; ///< trick_io{**} Pointer to TrickHLA object.
   RTI1516_NAMESPACE::AttributeHandleSet *handle_set;    ///< trick_io{**} Pointer to attribute handle set to divest ownership of.
} DivestThreadArgs;

} // namespace TrickHLA

#endif // TRICKHLA_OBJECT_HH
