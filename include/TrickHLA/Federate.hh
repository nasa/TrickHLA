/*!
@file TrickHLA/Federate.hh
@ingroup TrickHLA
@brief This class provides basic services for connecting a Trick based
simulation in to a HLA based distributed simulation environment.

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
@trick_link_dependency{../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/FedAmb.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/SaveRestoreServices.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/time/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/time/TimeManagementServices.cpp}
@trick_link_dependency{../../source/TrickHLA/time/TrickThreadCoordinator.cpp}
@trick_link_dependency{../../source/TrickHLA/utils/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/utils/MutexProtection.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, DSES, Sept 2005, --, DSES Test Sim.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support & test.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_FEDERATE_HH
#define TRICKHLA_FEDERATE_HH

// System includes.
#include <map>
#include <memory>
#include <string>

// Trick includes.
#include "trick/Flag.h"

// TrickHLA includes.
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/KnownFederate.hh"
#include "TrickHLA/SaveRestoreServices.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Interval.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/time/TimeManagementServices.hh"
#include "TrickHLA/time/TrickThreadCoordinator.hh"
#include "TrickHLA/utils/MutexLock.hh"
#include "TrickHLA/utils/MutexProtection.hh"

#if defined( IEEE_1516_2025 )
#   include "TrickHLA/FedAmbHLA4.hh"
#else
#   include "TrickHLA/FedAmbHLA3.hh"
#endif // IEEE_1516_2025

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Enums.h"
#include "RTI/Handle.h"
#include "RTI/RTI1516.h"
#include "RTI/Typedefs.h"
#include "RTI/VariableLengthData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Manager;
class ExecutionControlBase;

class Federate : public TimeManagementServices, public SaveRestoreServices
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Federate();

   // Allow the save and restore services access to the Federate protected
   // and private data.
   friend class SaveRestoreServices;

   //----------------------------- USER VARIABLES -----------------------------
   // The variables below are configured by the user in the input files.
   //--------------------------------------------------------------------------
  public:
   std::string name;            ///< @trick_units{--} The federate name.
   std::string type;            ///< @trick_units{--} The federate type.
   std::string federation_name; ///< @trick_units{--} Federation execution name.

   std::string rti_address; ///< @trick_units{--} RTI address of the form host:port.

   std::string local_settings; /**< @trick_units{--}
      Vendor specific HLA-Evolved local settings for the connect API.
      Pitch RTI: "crcHost = 192.168.1.1\ncrcPort = 8989" \n
      MAK RTI: "(setqb RTI_tcpForwarderAddr \"192.168.1.1\") (setqb RTI_distributedForwarderPort 5000)" */

   std::string FOM_modules; /**< @trick_units{--}
      FOM filename for the IEEE 1516-2000 and SISO-STD-004.1-2004 standards,
      or a comma separated list of FOM-module filenames for IEEE 1516-2010. */
   std::string MIM_module;  /**< @trick_units{--}
      Filename for the MOM and Initialization Module (MIM) for HLA IEEE 1516-2010. */

   FederateJoinConstraintsEnum join_constraint; ///< @trick_units{--} The Join constraints for this federate.

   // The Federates known to be in the Federation, and specified in the input files.
   // TODO: change this to be an STL Array.
   bool           enable_known_feds; ///< @trick_units{--} Enable use of known Federates list (default: true)
   int            known_feds_count;  ///< @trick_units{--} Number of required Federates (default: 0)
   KnownFederate *known_feds;        ///< @trick_units{--} Array of all the known Federates in the simulation.

   DebugLevelEnum  debug_level;  ///< @trick_units{--} Maximum debug report level requested by the user, default: THLA_NO_TRACE
   DebugSourceEnum code_section; ///< @trick_units{--} Code section(s) for which to activate debug messages, default: THLA_ALL_MODULES

   double wait_status_time; ///< @trick_units{s} How long to wait in a spin-lock in seconds before we print a status message.

   bool can_rejoin_federation; /**< @trick_units{--}
      Enables this federate to resign in a way to allow re-joining of the
      federation at a later time. */

   double freeze_delay_frames; /**< @trick_units{--}
      For DIS: Number of lookahead_time frames to delay when freeze issued so
      all feds freeze together. */

   bool unfreeze_after_save; /**< @trick_units{--}
      Flag to indicate that we should go to run immediately after a save. */

   //--------------------------------------------------------------------------

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Federate class. */
   Federate();
   /*! @brief Destructor for the TrickHLA Federate class. */
   virtual ~Federate();

   /*! @brief Print the TrickHLA version string. */
   static void print_version();

   // Federation initialization functions.
   /*! @brief Check, and if necessary, fix the FPU Control Word. */
   void fix_FPU_control_word();

   /*! @brief Setup the required class instance associations.
    *  @param federate_amb               Associated federate ambassador class instance.
    *  @param federate_manager           Associated federate manager class instance.
    *  @param federate_execution_control Associated federate execution control class instance. */
   void setup( FedAmb               &federate_amb,
               Manager              &federate_manager,
               ExecutionControlBase &federate_execution_control );

   /*! @brief Initialization the debug settings. */
   void initialize_debug();

   /*! @brief Composite initialization routine for an object instance of a Federate class. */
   void initialize();

   FederateJoinConstraintsEnum get_join_constraint()
   {
      return this->join_constraint;
   }

   void set_join_constraint( FederateJoinConstraintsEnum const fed_join_constraint )
   {
      this->join_constraint = fed_join_constraint;
   }

   double get_wait_status_time()
   {
      return this->wait_status_time;
   }

   /*! @brief Begin the pre-multiphase initialization process of standing up
    * the federate in the federation execution. */
   void pre_multiphase_initialization();

   /*! @brief Complete the post-multiphase initialization startup process prior
    * to the federation execution going into run. */
   void post_multiphase_initialization();

   //! @brief Create the RTI ambassador and connect to the RTI.
   void create_RTI_ambassador_and_connect();

   //! @brief Create and then join the Federation.
   void create_and_join_federation();

   //! @brief Enable asynchronous delivery of messages for this federate.
   void enable_async_delivery();

   //
   // Federation synchronization and synchronization point functions.
   //
   /*! @brief The RTI has announced the existence of a synchronization point.
    *  @param label             Sync-point label.
    *  @param user_supplied_tag Use supplied tag.*/
   void announce_sync_point( std::wstring const                          &label,
                             RTI1516_NAMESPACE::VariableLengthData const &user_supplied_tag );

   /*! @brief Marks a synchronization point as registered in the federation.
    *  @param label Sync-point label. */
   void sync_point_registration_succeeded( std::wstring const &label );

   /*! @brief Callback from TrickHLA::FedAmb through for
    *  when registration of a synchronization point fails.
    *  and is one of the sync-points created.
    *  @param label  Sync-point label.
    *  @param reason Reason for failure. */
   void sync_point_registration_failed( std::wstring const                                  &label,
                                        RTI1516_NAMESPACE::SynchronizationPointFailureReason reason );

   /*! @brief Marks a synchronization point as synchronized with the federation.
    *  @param label Sync-point label. */
   void federation_synchronized( std::wstring const &label );

   /*! @brief Wait for all the required federates to joined the federation.
    *  @return A non-empty string whrn there is a problem. */
   std::string wait_for_required_federates_to_join();

   /*! @brief Get a const reference to the joined federate handles.
    *  @return Pointer to associated federation execution name. */
   RTI1516_NAMESPACE::FederateHandleSet const &get_joined_federate_handles()
   {
      return joined_federate_handles;
   }

   //
   // Management Object Model (MOM) interfaces.
   //
   /*! @brief Initialize the MOM interface handles. */
   void initialize_MOM_handles();

   /*! @brief Request names of joined federates from the MOM. */
   void ask_MOM_for_federate_names();

   /*! @brief Unsubscribe from all MOM federate class attributes. */
   void unsubscribe_all_HLAfederate_class_attributes_from_MOM();

   /*! @brief Unsubscribe from all MOM federation class attributes. */
   void unsubscribe_all_HLAfederation_class_attributes_from_MOM();

   /*! @brief Get the auto-provide switch status as a string.
    *  @param auto_provide The Auto-provide switch status.
    *  @return The auto-provide switch status as a string. */
   static std::string get_auto_provide_status_string( int const auto_provide )
   {
      if ( auto_provide < 0 ) {
         return ( "Unknown" );
      }
      return ( ( auto_provide > 0 ) ? "Yes" : "No" );
   }

   /*! @brief Ask MOM for the current "auto-provide" setting from the switches table. */
   void ask_MOM_for_auto_provide_setting();

   /*! @brief Update the MOM "auto-provide" setting from the switches table
    * with the setting.
    *  @param enable True to enable Auto-provide and false to disable. */
   void enable_MOM_auto_provide_setting( bool enable );

   /*! @brief Backup the current "auto-provide" setting from the switches
    * table then disable auto-provide if it was enabled. */
   void backup_auto_provide_setting_from_MOM_then_disable();

   /*! @brief Restore the backed up "auto-provide" state to the MOM. */
   void restore_orig_MOM_auto_provide_setting();

   // TODO: Consider renaming "instance_id" functions from ID to handle.

   /*! @brief Add the specified Federate instance ID to the list of discovered federates.
    * @param instance_hndl Federate instance to add. */
   void add_federate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_hndl );

   /*! @brief Remove the specified Federate instance ID from the list of discovered federates.
    * @param instance_hndl Federate instance to remove. */
   void remove_federate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_hndl );

   /*! @brief Deallocate running federates based on current known information
    * in preparation for re-size. */
   void clear_known_feds();

   /*! @brief Add the specified MOM HLAfederate instance ID to the list of
    * discovered federates.
    *  @param instance_hndl Object instance handle.
    *  @param instance_name Object instance Name. */
   void add_MOM_HLAfederate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_hndl,
                                         std::wstring const                            &instance_name );

   /*! @brief Remove the specified Federate instance ID to the list of
    * discovered federates.
    *  @param instance_hndl Object instance handle. */
   void remove_MOM_HLAfederate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_hndl );

   /*! @brief Perform setup for federate save.
    * Delegates to Execution Control interface. */
   void setup_checkpoint();

   /*! @brief Federates that did not announce the save, perform a checkpoint.
    *  Delegates to Execution Control interface. */
   void perform_checkpoint();

   /*! @brief Perform setup for federate restore.
    *  Delegates to Execution Control interface.*/
   void setup_restore();

   /*! @brief Federates that did not announce the restore, perform a restore.
    *  Delegates to Execution Control interface. */
   void perform_restore();

   /*! @brief Checks for the existence 'startup' initialization sync point as
    *  an indication if this federate is running.
    *  @return True if federate is running; False otherwise. */
   bool is_federate_executing() const;

   /*! @brief Checks to see if shutdown has been commanded.
    *  @return True if shutdown has been announced, else False. */
   bool check_for_shutdown();

   /*! @brief Checks to see if shutdown has been commanded and, if so, terminates the simulation.
    *  @return False if shutdown has NOT been announced. */
   bool check_for_shutdown_with_termination();

   /*! @brief Check if federate is shutdown function was called.
    *  @return True if the federate is shutting down the federate. */
   bool is_shutdown_called() const
   {
      return this->shutdown_called;
   }

   /*! @brief Ask for all federate handles from MOM after a checkpoint reload. */
   void restore_federate_handles_from_MOM();

   /*! @brief Reloads the federate handle set from the MOM after a checkpoint reload.
    *  @param instance_hndl Object instance handle.
    *  @param values        Attribute values. */
   void rebuild_federate_handles( RTI1516_NAMESPACE::ObjectInstanceHandle const    &instance_hndl,
                                  RTI1516_NAMESPACE::AttributeHandleValueMap const &values );

   /*! @brief Send zero lookahead or requested data for the specified object instance.
    *  @param obj_instance_name Object instance name to send data for. */
   void send_zero_lookahead_and_requested_data( std::string const &obj_instance_name );

   /*! @brief Blocking function call to wait to receive the zero lookahead
    *  data for the specified object instance.
    *  @param obj_instance_name Object instance name to wait for data. */
   void wait_to_receive_zero_lookahead_data( std::string const &obj_instance_name );

   /*! @brief Send blocking I/O or requested data for the specified object instance.
    *  @param obj_instance_name Object instance name to send data for. */
   void send_blocking_io_data( std::string const &obj_instance_name );

   /*! @brief Blocking function call to wait to receive the blocking I/O data
    *  for the specified object instance.
    *  @param obj_instance_name Object instance name to wait for data. */
   void wait_to_receive_blocking_io_data( std::string const &obj_instance_name );

   /*! @brief Set federate execution startup state.
    *  @param flag True for federate started; False otherwise. */
   void set_startup( bool const flag )
   {
      this->got_startup_sync_point = flag;
   }

   //=======================================================================

   //
   // Clean up / shutdown functions.
   //
   /*! @brief Shutdown the federate. */
   void shutdown();

   /*! @brief Resign from the federation. */
   void resign();

   /*! @brief Resign from the federation in a way that permits rejoining later. */
   void resign_so_we_can_rejoin();

   /*! @brief Destroy the federation if this is the last federate. */
   void destroy();

   /*! @brief Destroy the federation if it was orphaned from a previous
    * simulation run that did not shutdown cleanly. */
   void destroy_orphaned_federation();

   /*! @brief Determine if the specified instance ID is for one of the discovered federates.
    *  @return True if ID is for a federate.
    *  @param id MOM HLAfederate instance ID. */
   bool is_federate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle const &id );

   /*! @brief Decode the specified encoded Federate Handle.
    *  @return Federate Handle.
    *  @param encoded_handle encoded Federate Handle */
   RTI1516_NAMESPACE::FederateHandle decode_federate_handle( RTI1516_NAMESPACE::VariableLengthData const &encoded_handle );

   //
   // MOM HLAfederate class and attributes.
   //
   /*! @brief Check with the MOM if the is an HLAfederate class.
    *  @return True if a MOM HLAfederate class.
    *  @param federate_class Object class handle to check. */
   bool is_MOM_HLAfederate_class( RTI1516_NAMESPACE::ObjectClassHandle const &federate_class ) const
   {
      return ( federate_class == this->MOM_HLAfederate_class_handle );
   }

   /*! @brief Get the federate class handle for this federate from the MOM.
    *  @return The federate ObjectClassHandle. */
   RTI1516_NAMESPACE::ObjectClassHandle get_MOM_HLAfederate_class_handle() const
   {
      return MOM_HLAfederate_class_handle;
   }

   /*! @brief Set the Federates name given the instance ID as well as the
    * FederateHandle ID associated with the Federate instance.
    * @param id     Object instance handle.
    * @param values Attribute values. */
   void set_MOM_HLAfederate_instance_attributes( RTI1516_NAMESPACE::ObjectInstanceHandle const    &id,
                                                 RTI1516_NAMESPACE::AttributeHandleValueMap const &values );

   /*! @brief Set all the federate MOM instance handles by using the previously
    * saved named for the MOM object instance associated with the federate. */
   void set_all_federate_MOM_instance_handles_by_name();

   /*! @brief Get the federate MOM object instance names so that we can recover
    * the MOM instance handles associated with each federate when a checkpoint
    * restore happens. */
   void determine_federate_MOM_object_instance_names();

   /*! @brief Determine if the specified instance handle is an MOM
    *  HLAfederation instance.
    *  @return True if ID is for a federate; False otherwise.
    *  @param instance_hndl Federate instance handle. */
   bool is_MOM_HLAfederation_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_hndl );

   /*! @brief Add the specified MOM HLAfederation instance handle to the list
    *  of running federates.
    *  @param instance_hndl Object instance handle. */
   void add_MOM_HLAfederation_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_hndl );

   /*! @brief Remove the specified MOM HLAfederation instance handle from the
    *  list of running federates.
    *  @param instance_hndl Object instance handle. */
   void remove_MOM_HLAfederation_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle const &instance_hndl );

   /*! @brief Query if the an object class handle is a federation class.
    *  @return True if class handle is a federation class; False otherwise.
    *  @param class_hndl HLA object class handle to test. */
   bool is_MOM_HLAfederation_class( RTI1516_NAMESPACE::ObjectClassHandle const &class_hndl ) const
   {
      return ( class_hndl == this->MOM_HLAfederation_class_handle );
   }

   /*! @brief Set the Federation ID given the instance ID as well as the
    * FederateHandle ID associated with the Federation instance.
    * @param instance_hndl Object instance handle.
    * @param values        Attribute values. */
   void set_MOM_HLAfederation_instance_attributes( RTI1516_NAMESPACE::ObjectInstanceHandle const    &instance_hndl,
                                                   RTI1516_NAMESPACE::AttributeHandleValueMap const &values );

   //
   // Routines to return federation state values.
   //
   /*! @brief Get the pointer to the associated TrickHLA Federate Ambassador instance.
    *  @return Pointer to associated TrickHLA::FedAmb. */
   FedAmb *get_fed_ambassador()
   {
      return this->federate_ambassador;
   }

   /*! @brief Get the pointer to the associated TrickHLA::Manager instance.
    *  @return Pointer to associated TrickHLA::Manager. */
   Manager *get_manager()
   {
      return this->manager;
   }

   /*! @brief Get the pointer to the associated TrickHLA::Manager instance.
    *  @return Pointer to associated TrickHLA::Manager. */
   ExecutionControlBase *get_execution_control()
   {
      return this->execution_control;
   }

   /*! @brief Get the pointer to the associated federate name.
    *  @return Pointer to associated federate name. */
   std::string const &get_federate_name() const
   {
      return this->name;
   }

   /*! @brief Get the pointer to the associated federate type.
    *  @return Pointer to associated federate type. */
   std::string const &get_federate_type() const
   {
      return this->type;
   }

   /*! @brief Get the pointer to the associated federation execution name.
    *  @return Pointer to associated federation execution name. */
   std::string const &get_federation_name() const
   {
      return this->federation_name;
   }

   // Routines to set federation state values.
   /*! @brief Set the name of the federation execution.
    *  @param exec_name Federation execution name. */
   void set_federation_name( std::string const &exec_name );

   // Checkpoint restart initialization.
   /*! @brief Perform initialization after a restart. */
   virtual void restart_initialization();

   /*! @brief Query if federate can rejoin federation.
    *  @return True if federate can rejoin; False otherwise. */
   bool federate_can_rejoin_federation() const
   {
      return this->can_rejoin_federation;
   }

   /*! @brief Query if a federate is required at startup.
    *  @return True if federate is required at startup; False otherwise.
    *  @param fed_name Name of potentially required federate. */
   bool is_a_required_startup_federate( std::wstring const &fed_name );

   /*! @brief Query if the federation was created by this federate.
    *  @return True if created by this federate; False otherwise. */
   bool is_federation_created_by_federate() const
   {
      return this->federation_created_by_federate;
   }

   /*! @brief Set connection to the RTI as lost. */
   void set_connection_lost();

   /*! @brief Is the federate an execution member, which means is it connected
    * and joined to a federation execution.
    *  @return True if the federate is execution member; False otherwise. */
   bool is_execution_member();

   //
   // Federation freeze management functions.
   //

   /*! @brief Routine to handle going from run to freeze. */
   void freeze_init();

   /*! @brief Check for exit from freeze. */
   void check_freeze();

   /*! @brief Check if a Trick freeze was commanded; if we announced freeze,
    *  tell other federates to freeze. */
   void enter_freeze();

   /*! @brief Routine to handle going from freeze to run; if we announced the
    * freeze, tell other federates to run. */
   void exit_freeze();

   /*! @brief Unfreeze the simulation. */
   static void un_freeze();

  private:
   // Federation state variables.
   //
   RTI1516_NAMESPACE::FederateHandle federate_id;                    ///< @trick_io{**} Federate ID.
   bool                              federation_created_by_federate; ///< @trick_io{**} Federate successfully created the federation if True.
   bool                              federation_exists;              ///< @trick_io{**} Federation exists.
   bool                              federation_joined;              ///< @trick_io{**} Federate joined federation flag.
   bool                              all_federates_joined;           ///< @trick_units{--} Master check for all federates joined.
   bool                              connected;                      ///< @trick_units{--} True if connected to RTI, False otherwise.

   bool shutdown_called; ///< @trick_units{--} Flag to indicate shutdown has been called.

  private:
   bool got_startup_sync_point;     ///< @trick_units{--} "startup" Sync-Point has been created. For DIS compatibility
   bool make_copy_of_run_directory; ///< @trick_units{--} Make a backup of RUN directory before restarting the federation via federation manager (default: false).

   RTI1516_NAMESPACE::ObjectClassHandle MOM_HLAfederation_class_handle;      ///< @trick_io{**} MOM Federation class handle.
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAfederatesInFederation_handle; ///< @trick_io{**} MOM attribute handle to Federate-count.
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAautoProvide_handle;           ///< @trick_io{**} MOM AutoProvide attribute handle.
   TrickHLAObjInstanceNameMap           MOM_HLAfederation_instance_name_map; ///< @trick_io{**} Map of the MOM HLAfederation instances.
   int                                  auto_provide_setting;                ///< @trick_units{--} MOM Federation wide HLAautoProvide setting.
   int                                  orig_auto_provide_setting;           ///< @trick_units{--} Original MOM Federation wide HLAautoProvide setting when we joined the federation.

   RTI1516_NAMESPACE::ObjectClassHandle MOM_HLAfederate_class_handle; ///< @trick_io{**} MOM Federate class handle.
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAfederateType_handle;   ///< @trick_io{**} MOM attribute handle to Federate type (a.k.a name in IEEE 1516-2000).
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAfederateName_handle;   ///< @trick_io{**} MOM attribute handle to Federate name.
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAfederate_handle;       ///< @trick_io{**} MOM attribute handle to Federate-Handle.

   TrickHLAObjInstanceNameMap MOM_HLAfederate_instance_name_map; ///< @trick_io{**} Map of the MOM HLAfederate instances name map.

   MutexLock                            joined_federate_mutex;    ///< @trick_io{**} Mutex to lock thread over critical code sections.
   TrickHLAObjInstanceNameMap           joined_federate_name_map; ///< @trick_io{**} Map of the federate instances and corresponding names.
   RTI1516_NAMESPACE::FederateHandleSet joined_federate_handles;  ///< @trick_io{**} FederateHandles of joined federates.
   VectorOfWstrings                     joined_federate_names;    ///< @trick_io{**} Names of the joined federates.

   RTI1516_NAMESPACE::InteractionClassHandle MOM_HLAsetSwitches_class_handle; ///< @trick_io{**} MOM HLAsetSwitches class handle.
   RTI1516_NAMESPACE::ParameterHandle        MOM_HLAautoProvide_param_handle; ///< @trick_io{**} MOM HLAautoProvide parameter handle.

   // Federation required associations.
   //
   FedAmb               *federate_ambassador; ///< @trick_units{--} Federate ambassador.
   Manager              *manager;             ///< @trick_units{--} Associated TrickHLA Federate Manager.
   ExecutionControlBase *execution_control;   /**< @trick_units{--} Execution control object. This has to point to an allocated execution control class that inherits from the ExecutionControlBase interface class. For instance SRFOM::ExecutionControl. */

  private:
   /*! @brief Subscribe to the specified attributes for the given class handle.
    *  @param class_handle   Class handle.
    *  @param attribute_list Attributes handles. */
   void subscribe_attributes( RTI1516_NAMESPACE::ObjectClassHandle const  &class_handle,
                              RTI1516_NAMESPACE::AttributeHandleSet const &attribute_list );

   /*! @brief Unsubscribe from the specified attributes for the given class handle.
    *  @param class_handle   Class handle.
    *  @param attribute_list Attributes handles. */
   void unsubscribe_attributes( RTI1516_NAMESPACE::ObjectClassHandle const  &class_handle,
                                RTI1516_NAMESPACE::AttributeHandleSet const &attribute_list );

   /*! @brief Request an update to the specified attributes for the given object class handle.
    *  @param class_handle   Class handle.
    *  @param attribute_list Attributes handles. */
   void request_attribute_update( RTI1516_NAMESPACE::ObjectClassHandle const  &class_handle,
                                  RTI1516_NAMESPACE::AttributeHandleSet const &attribute_list );

   /*! @brief Publish Interaction class.
    *  @param class_handle Interaction class handle. */
   void publish_interaction_class( RTI1516_NAMESPACE::InteractionClassHandle const &class_handle );

   /*! @brief Unpublish Interaction class.
    *  @param class_handle Interaction class handle. */
   void unpublish_interaction_class( RTI1516_NAMESPACE::InteractionClassHandle const &class_handle );

   /*! @brief Send the Interaction for the specified interaction class and parameter list.
    *  @param class_handle   Interaction class handle.
    *  @param parameter_list Parameter values in a map. */
   void send_interaction( RTI1516_NAMESPACE::InteractionClassHandle const  &class_handle,
                          RTI1516_NAMESPACE::ParameterHandleValueMap const &parameter_list );

   //
   // Internal initialization utilities (invoked by initialize())
   //
   /*! @brief Create the simulation federation if it does not already exist. */
   void create_federation();

   /*! @brief Join a federation.
    *  @param federate_name Name of this federate.
    *  @param federate_type Type for this federate. */
   void join_federation( std::string const &federate_name,
                         std::string const &federate_type );

   /*! @brief Determine if the specified federate name is a required federate.
    *  @return True if a name of required federate, otherwise false.
    *  @param federate_name Federate name to test. */
   bool is_required_federate( std::wstring const &federate_name );

   /*! @brief Determine if the specified federate name is a joined federate.
    *  @return True if a name of joined federate, otherwise false.
    *  @param federate_name Federate name to test. */
   bool is_joined_federate( std::string const &federate_name );

   /*! @brief Determine if the specified federate name is a joined federate.
    *  @return True if a name of joined federate, otherwise false.
    *  @param federate_name Federate name to test. */
   bool is_joined_federate( std::wstring const &federate_name );

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Federate class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Federate( Federate const &rhs );

   /*! @brief Assignment operator for Federate class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Federate &operator=( Federate const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_FEDERATE_HH
