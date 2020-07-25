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
@trick_link_dependency{../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../source/TrickHLA/Int64Time.cpp}
@trick_link_dependency{../source/TrickHLA/FedAmb.cpp}
@trick_link_dependency{../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, DSES, Sept 2005, --, DSES Test Sim.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, Jan 2019, --, SRFOM support & test.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_FEDERATE_HH_
#define _TRICKHLA_FEDERATE_HH_

// System includes.
#include <string>

// Trick include files.
#include "trick/Flag.h"

// TrickHLA include files.
#include "TrickHLA/Int64Time.hh"
#include "TrickHLA/KnownFederate.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"

// HLA include files.
#include RTI1516_HEADER

// FIXME: What do we do for Trick 10 to get the command-line arguments for the
// Federate::restart_federate() job? DDexter 11/7/11

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations.  This
// helps to limit issues with recursive includes.
class Manager;
class FedAmb;
class ExecutionControlBase;

/*
 * Enumerated type used to step through the restore process.
 */
typedef enum {
   No_Restore                = 0,
   Restore_Request_Failed    = 1,
   Restore_Request_Succeeded = 2,
   Initiate_Restore          = 3,
   Restore_In_Progress       = 4,
   Restore_Complete          = 5,
   Restore_Failed            = 6
} THLASaveRestoreProcEnum;

class Federate
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Federate();

   //----------------------------- USER VARIABLES -----------------------------
   // The variables below are configured by the user in the input files.
   //--------------------------------------------------------------------------
  public:
   char *name;            ///< @trick_units{--} The federate name.
   char *type;            ///< @trick_units{--} The federate type.
   char *federation_name; ///< @trick_units{--} Federation execution name.

   char *local_settings; /**< @trick_units{--}
      Vendor specific HLA-Evolved local settings for the connect API.
      Pitch RTI: "crcHost = 192.168.1.1\ncrcPort = 8989" \n
      MAK RTI: "(setqb RTI_tcpForwarderAddr \"192.168.1.1\") (setqb RTI_distributedForwarderPort 5000)" */

   char *FOM_modules; /**< @trick_units{--}
      FOM filename for the IEEE 1516-2000 and SISO-STD-004.1-2004 standards,
      or a comma separated list of FOM-module filenames for IEEE 1516-2010. */
   char *MIM_module;  /**< @trick_units{--}
      Filename for the MOM and Initialization Module (MIM) for HLA IEEE 1516-2010. */

   // FIXME: Is this really needed?
   // This is only used for checkpointing and restart.
   double lookahead_time; ///< @trick_units{s} The HLA lookahead time in seconds.

   bool time_regulating;  ///< @trick_units{--} HLA Time Regulation flag (default: true).
   bool time_constrained; ///< @trick_units{--} HLA Time Constrained flag (default: true).
   bool time_management;  ///< @trick_units{--} Enable HLA Time Management flag (default: true).

   // The Federates known to be in the Federation, and specified in the input files.
   // TODO: change this to be an STL Array.
   bool           enable_known_feds; ///< @trick_units{--} Enable use of known Federates list (default: true)
   int            known_feds_count;  ///< @trick_units{--} Number of required Federates (default: 0)
   KnownFederate *known_feds;        ///< @trick_units{--} Array of all the known Federates in the simulation.

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
   ~Federate();

   /*! @brief Print the TrickHLA version string. */
   void print_version() const;

   // Federation initialization functions.
   /*! @brief Check, and if necessary, fix the FPU Control Word. */
   void fix_FPU_control_word();

   /*! @brief Setup the required class instance associations.
    *  @param federate_amb               Associated federate ambassador class instance.
    *  @param federate_manager           Associated federate manager class instance.
    *  @param federate_execution_control Associated federate execution control class instance. */
   void setup( FedAmb &              federate_amb,
               Manager &             federate_manager,
               ExecutionControlBase &federate_execution_control );

   /*! @brief Composite initialization routine for an object instance of a Federate class. */
   void initialize();

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
   /*! @brief Register a generic synchronization point; i.e. not a multiphase init sync-point.
    *  @param label Sync-point label.
    *  @param time  Optional Sync-point time in seconds. */
   void register_generic_sync_point( std::wstring const &label, double time = -1.0 );

   /*! @brief Achieve the specified sync-point and wait for the federation to
    *  be synchronized on it.
    *  @param label Sync-point label. */
   void achieve_and_wait_for_synchronization( std::wstring const &label );

   /*! @brief Achieve the specified sync-point and do NOT wait for the
    *  federation to be synchronized on it.
    *  @param label Sync-point label. */
   void achieve_synchronization_point( std::wstring const &label );

   /*! @brief The RTI has announced the existence of a synchronization point.
    *  @param label             Sync-point label.
    *  @param user_supplied_tag Use supplied tag.*/
   void announce_sync_point( std::wstring const &    label,
                             RTI1516_USERDATA const &user_supplied_tag );

   /*! @brief Marks a synchronization point as registered in the federation.
    *  @param label Sync-point label. */
   void sync_point_registration_succeeded( std::wstring const &label );

   /*! @brief Callback from TrickHLA::FedAmb through for
    *  when registration of a synchronization point fails.
    *  and is one of the sync-points created.
    *  @param label      Sync-point label.
    *  @param not_unique True if not unique label. */
   void sync_point_registration_failed( std::wstring const &label, bool not_unique );

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
   void add_federate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle instance_hndl );

   /*! @brief Remove the specified Federate instance ID from the list of discovered federates.
    * @param instance_hndl Federate instance to remove. */
   void remove_federate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle instance_hndl );

   //
   // BEGIN: checkpoint / restore code
   //
   /*! @brief Load the running federate names from the RTI. */
   void load_and_print_running_federate_names();

   /*! @brief Deallocate running federates based on current known information
    * in preparation for re-size. */
   void clear_running_feds();

   /*! @brief Update running federates based on current known information. */
   void update_running_feds();

   /*! @brief Grow the running_feds by one entry. */
   void add_a_single_entry_into_running_feds();

   /*! @brief Get the count of the currently running federates.
    *  @return Count of the currently running federates. */
   int get_running_feds_count() const
   {
      return running_feds_count;
   }

   /*! @brief Add the specified MOM HLAfederate instance ID to the list of
    * discovered federates.
    *  @param instance_hndl Object instance handle.
    *  @param instance_name Object instance Name. */
   void add_MOM_HLAfederate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle instance_hndl,
                                         std::wstring const &                    instance_name );

   /*! @brief Remove the specified Federate instance ID to the list of
    * discovered federates.
    *  @param instance_hndl Object instance handle. */
   void remove_MOM_HLAfederate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle instance_hndl );

   /*! @brief Perform setup for federate save. */
   void setup_checkpoint();

   /*! @brief Federates that did not announce the save, perform a checkpoint. */
   void perform_checkpoint();

   /*! @brief Complete federate save. */
   void post_checkpoint();

   /*! @brief Perform setup for federate restore. */
   void setup_restore();

   /*! @brief Federates that did not announce the restore, perform a restore. */
   void perform_restore();

   /*! @brief Complete federate restore and prepare to restart execution. */
   void post_restore();

   /*! @brief Returns true if HLA save and restore is supported by the user
    *  specified simulation initialization scheme.
    *  @return True if HLA save and restore are supported, false otherwise. */
   bool is_HLA_save_and_restore_supported();

   /*! @brief Restore checkpoint.
    *  @param file_name Checkpoint file name. */
   void restore_checkpoint( const std::string &file_name );

   /*! @brief Inform the RTI of the success or failure of the federate restore. */
   void inform_RTI_of_restore_completion();

   /*! @brief Read the running_feds file, replacing the data in known federates
    * data structure.
    * @param file_name Checkpoint file name. */
   void read_running_feds_file( const std::string &file_name ) throw( const char * );

   /*! @brief Copies the contents of the checkpoint's list of federates into
    * known federates data structure. */
   void copy_running_feds_into_known_feds();

   /*! @brief Restart the sim from a checkpoint. */
   void restart_checkpoint();

   /*! @brief Federation save completed. */
   void federation_saved();

   /*! @brief Federation restore completed. */
   void federation_restored();

   /*! @brief Blocks until the federation restore has begun. */
   void wait_for_federation_restore_begun();

   /*! @brief Blocks until the federation is ready to restore. */
   void wait_until_federation_is_ready_to_restore();

   /*! @brief Blocks until the federation restore is complete.
    *  @return Empty string if successful, descriptive string on failure. */
   std::string wait_for_federation_restore_to_complete();

   /*! @brief Blocks until the RTI responds with a federation request request
    * success / failure. */
   void wait_for_restore_request_callback();

   /*! @brief Blocks until the RTI responds with a federation status of the
    * restore is complete. */
   void wait_for_restore_status_to_complete();

   /*! @brief Blocks until the RTI responds with a federation status of the
    * save is complete. */
   void wait_for_save_status_to_complete();

   /*! @brief Blocks until the RTI responds with a federation not restored
    * callback via the federate ambassador. */
   void wait_for_federation_restore_failed_callback_to_complete();

   /*! @brief Requests the status of the Federation Save. */
   void request_federation_save_status();

   /*! @brief Requests the status of the Federation Restore. */
   void request_federation_restore_status();

   /*! @brief Query if restore process restore request failed.
    *  @return True if failed, False otherwise. */
   bool has_restore_process_restore_request_failed() const
   {
      return ( restore_process == Restore_Request_Failed );
   }

   /*! @brief Query if restore process restore request succeeded.
    *  @return True if succeeded, False otherwise. */
   bool has_restore_process_restore_request_succeeded() const
   {
      return ( restore_process == Restore_Request_Succeeded );
   }

   /*! @brief Query if restore request failed.
    *  @return True if failed, False otherwise. */
   bool has_restore_request_failed() const
   {
      return ( restore_process == Restore_Request_Failed );
   }

   /*! @brief Query if restore request succeeded.
    *  @return True if succeeded, False otherwise. */
   bool has_restore_request_succeeded() const
   {
      return ( restore_process == Restore_Request_Succeeded );
   }

   /*! @brief Set the announce save flag. */
   void set_announce_save()
   {
      announce_save = true;
   }

   /*! @brief Set the save completed state. */
   void set_save_completed()
   {
      save_completed = true;
      start_to_save  = false;
      publish_data   = true;
   }

   /*! @brief Set the restore begun state. */
   void set_restore_begun()
   {
      restore_begun     = true;
      restore_completed = false;
      publish_data      = false;
   }

   /*! @brief Set the restore completed state. */
   void set_restore_completed()
   {
      restore_process   = Restore_Complete;
      restore_completed = true;
      restore_begun     = false;
      start_to_restore  = false;
      publish_data      = true;
   }

   /*! @brief Set the restore failed state. */
   void set_restore_failed()
   {
      restore_process   = Restore_Failed;
      restore_completed = true;
      restore_begun     = false;
      start_to_restore  = false;
      publish_data      = true;
   }

   /*! @brief Set the restore request failed state. */
   void set_restore_request_failed()
   {
      restore_process = Restore_Request_Failed;
   }

   /*! @brief Set the restore request succeeded state. */
   void set_restore_request_succeeded()
   {
      restore_process = Restore_Request_Succeeded;
   }

   /*! @brief Query if federate should publish data.
    *  @return True if data should be published; False otherwise. */
   bool should_publish_data() const
   {
      return publish_data;
   }

   /*! @brief Query if federate has started a restore process.
    *  @return True if restore has started; False otherwise. */
   bool is_start_to_restore() const
   {
      return this->start_to_restore;
   }

   /*! @brief Set the restore is imminent flag. */
   void set_restore_is_imminent()
   {
      this->restore_is_imminent = true;
   }

   /*! @brief Sets the Restore filename and flag.
    * @param status Restore success status from RTI. */
   void requested_federation_restore_status( bool status );

   /*! @brief Prints the federation restore status from the RTI.
    * @param status_vector Save status. */
   void print_requested_federation_restore_status(
      RTI1516_NAMESPACE::FederateRestoreStatusVector const &status_vector );

   /*! @brief Processes the federation restore status received from the RTI.
    * @param status_vector Save status. */
   void process_requested_federation_restore_status(
      RTI1516_NAMESPACE::FederateRestoreStatusVector const &status_vector );

   /*! @brief Processes the federation save status received from the RTI.
    * @param status_vector Save status. */
   void process_requested_federation_save_status(
      RTI1516_NAMESPACE::FederateHandleSaveStatusPairVector const &status_vector );

   /*! @brief Prints the reason for the federation restore failure.
    * @param reason Restore failure reason. */
   void print_restore_failure_reason( RTI1516_NAMESPACE::RestoreFailureReason reason );

   /*! @brief Prints the reason for the federation save failure.
    * @param reason Save failure reason. */
   void print_save_failure_reason( RTI1516_NAMESPACE::SaveFailureReason reason );

   /*! @brief Save the supplied checkpoint file name.
    * @param name Checkpoint file name. */
   void set_checkpoint_file_name( const std::string &name );

   /*! @brief Sets the Save filename and flag. */
   void initiate_save_announce();

   /*! @brief Sets the Save filename and flag.
    *  @param restore_name Restore file name. */
   void initiate_restore_announce( const std::string &restore_name_label );

   /*! @brief Sets the Save filename and flag.
    *  @return True if restore has been announced; False otherwise. */
   bool has_restore_been_announced() const
   {
      return restore_begun;
   }

   /*! @brief Informs of completion of federation restore. */
   void complete_restore();

   /*! @brief Checks for the existence 'startup' initialization sync point as
    *  an indication if this federate is running.
    *  @return True if federate is running; False otherwise. */
   bool is_federate_executing() const;

   /*! @brief Converts HLA sync points into something Trick can save in a checkpoint. */
   void convert_sync_pts();

   /*! @brief Converts checkpointed sync points into HLA sync points. */
   void reinstate_logged_sync_pts();

   /*! @brief Set the start to save flag. */
   void set_start_to_save()
   {
      start_to_save = true;
   }

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

   /*! @brief Check if HLA_save_directory is empty. If so, ask the EXECUTIVE
    * for info and build the absolute path of the RUN directory. */
   void check_HLA_save_directory();

   /*! @brief Set the federate has begun execution state. */
   void set_federate_has_begun_execution()
   {
      execution_has_begun = true;
      joined_federate_name_map.clear(); // clear out joined federate names
      check_HLA_save_directory();
   }

   /*! @brief Ask for all federate handles from MOM after a checkpoint reload. */
   void restore_federate_handles_from_MOM();

   /*! @brief Reloads the federate handle set from the MOM after a checkpoint reload.
    *  @param instance_hndl Object instance handle.
    *  @param values        Attribute values. */
   void rebuild_federate_handles( RTI1516_NAMESPACE::ObjectInstanceHandle           instance_hndl,
                                  RTI1516_NAMESPACE::AttributeHandleValueMap const &values );
   // END: checkpoint / restore code

   //
   // Time management initialization functions.
   //
   /*! @brief Setup this federate's constrained time management. */
   void setup_time_constrained();

   /*! @brief Setup this federate's regulate time management. */
   void setup_time_regulation();

   /*! @brief Setup this federate's time management. */
   void setup_time_management();

   //
   // Executive execution loop time functions.
   //
   /*! @brief Increment the requested time by the lookahead time and make a
    *  HLA time advance request. */
   void time_advance_request();

   /*! @brief Moves the federates time to the Greatest Available Logical Time
    * (GALT) that is an integer multiple of the Least-Common-Time-Step (LCTS)
    * time if we are time constrained and Not time regulating. */
   void time_advance_request_to_GALT();

   /*! @brief Move the requested time to an integer multiple of the Greatest
    *  Available Logical Time (GALT) and Least Common Time Step (LCTS). */
   void time_advance_request_to_GALT_LCTS_multiple();

   /*! @brief Wait for a HLA time-advance grant. */
   void wait_for_time_advance_grant();

   /*! @brief Wait for a HLA time-advance grant, but allow for an early exit
    * if it takes longer than time_out_tolerance (for SSTF).
    *  @param time_out_tolerance Timeout tolerance in nanoseconds. */
   void wait_for_time_advance_grant( int time_out_tolerance );

   /*! @brief Set federate execution startup state.
    *  @param flag True for federate started; False otherwise. */
   void set_startup( bool flag )
   {
      this->got_startup_sp = flag;
   }

   //=======================================================================
   // FIXME: Might consider moving these to ExecutionControl.
   /*! @brief Set that federation execution freeze has been announced.
    *  @param flag True for federate freeze announce; False otherwise. */
   void set_freeze_announced( bool flag )
   {
      this->announce_freeze = flag;
   }

   /*! @brief Get that federation execution freeze announced flag state.
    *  @return True for federate freeze announced; False otherwise. */
   bool get_freeze_announced()
   {
      return this->announce_freeze;
   }

   /*! @brief Get that federation execution freeze pending flag state.
    *  @return True for federate freeze is pending; False otherwise. */
   bool get_freeze_pending()
   {
      return this->freeze_the_federation;
   }

   /*! @brief Perform federation execution freeze process. */
   void unfreeze()
   {
      return this->un_freeze();
   }
   //=======================================================================

   //
   // Clean up / shutdown functions.
   //
   /*! @brief Shutdown the federate. */
   void shutdown();

   /*! @brief Shutdown this federate's time management. */
   void shutdown_time_management();

   // TODO: Consider renaming these "shutdown" routines to disable.
   /*! @brief Shutdown this federate's time constrained time management. */
   void shutdown_time_constrained();

   /*! @brief Shutdown this federate's time regulating time management. */
   void shutdown_time_regulating();

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
   bool is_federate_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle id );

   //
   // MOM HLAfederate class and attributes.
   //
   /*! @brief Check with the MOM if the is an HLAfederate class.
    *  @return True if a MOM HLAfederate class.
    *  @param federate_class Object class handle to check. */
   bool is_MOM_HLAfederate_class( RTI1516_NAMESPACE::ObjectClassHandle federate_class ) const
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
   void set_MOM_HLAfederate_instance_attributes( RTI1516_NAMESPACE::ObjectInstanceHandle           id,
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
   bool is_MOM_HLAfederation_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle instance_hndl );

   /*! @brief Add the specified MOM HLAfederation instance handle to the list
    *  of running federates.
    *  @param instance_hndl Object instance handle. */
   void add_MOM_HLAfederation_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle instance_hndl );

   /*! @brief Remove the specified MOM HLAfederation instance handle from the
    *  list of running federates.
    *  @param instance_hndl Object instance handle. */
   void remove_MOM_HLAfederation_instance_id( RTI1516_NAMESPACE::ObjectInstanceHandle instance_hndl );

   /*! @brief Query if the an object class handle is a federation class.
    *  @return True if class handle is a federation class; False otherwise.
    *  @param class_hndl HLA object class handle to test. */
   bool is_MOM_HLAfederation_class( RTI1516_NAMESPACE::ObjectClassHandle class_hndl ) const
   {
      return ( class_hndl == this->MOM_HLAfederation_class_handle );
   }

   /*! @brief Set the Federation ID given the instance ID as well as the
    * FederateHandle ID associated with the Federation instance.
    * @param instance_hndl Object instance handle.
    * @param values        Attribute values. */
   void set_MOM_HLAfederation_instance_attributes( RTI1516_NAMESPACE::ObjectInstanceHandle           instance_hndl,
                                                   RTI1516_NAMESPACE::AttributeHandleValueMap const &values );

   //
   // Routines to return federation state values.
   //
   /*! @brief Get the pointer to the associated HLA RTI Ambassador instance.
    *  @return Pointer to associated RTI Ambassador. */
   RTI1516_NAMESPACE::RTIambassador *get_RTI_ambassador()
   {
      return RTI_ambassador.get();
   }

   /*! @brief Get the pointer to the associated TrickHLA Federate Ambassador instance.
    *  @return Pointer to associated TrickHLA::FedAmb. */
   FedAmb *get_fed_ambassador()
   {
      return federate_ambassador;
   }

   /*! @brief Get the pointer to the associated TrickHLA::Manager instance.
    *  @return Pointer to associated TrickHLA::Manager. */
   Manager *get_manager()
   {
      return manager;
   }

   /*! @brief Get the pointer to the associated TrickHLA::Manager instance.
    *  @return Pointer to associated TrickHLA::Manager. */
   ExecutionControlBase *get_execution_control()
   {
      return execution_control;
   }

   /*! @brief Get the pointer to the associated federate name.
    *  @return Pointer to associated federate name. */
   const char *get_federate_name() const
   {
      return name;
   }

   /*! @brief Get the pointer to the associated federate type.
    *  @return Pointer to associated federate type. */
   const char *get_federate_type() const
   {
      return type;
   }

   /*! @brief Get the pointer to the associated federation execution name.
    *  @return Pointer to associated federation execution name. */
   const char *get_federation_name() const
   {
      return federation_name;
   }

   /*! @brief Get the current granted federation execution time in seconds.
    *  @return Current granted federation execution time in seconds. */
   double get_granted_time() const
   {
      return granted_time.get_time_in_seconds();
   }

   /*! @brief Get the requested federation execution time in seconds.
    *  @return Requested federation execution time in seconds. */
   double get_requested_time() const
   {
      return requested_time.get_time_in_seconds();
   }

   /*! @brief Get the current granted federation execution time.
    *  @return Reference to current granted federation execution time. */
   const Int64Time &get_granted_fed_time() const
   {
      return granted_time;
   }

   /*! @brief Get the requested federation execution time.
    *  @return Reference to requested federation execution time. */
   const Int64Time &get_requested_fed_time() const
   {
      return requested_time;
   }

   /*! @brief Get the current federate lookahead time.
    *  @return Reference to current federate lookahead time. */
   const Int64Interval &get_lookahead() const
   {
      return lookahead;
   }

   /*! @brief Get the current federate lookahead time in seconds.
    *  @return Current federate lookahead time in seconds. */
   const double get_lookahead_time() const
   {
      return lookahead_time;
   }

   /*! @brief Query of federate has a zero lookahead time.
    *  @return True if lookahead time is zero; Flase otherwise. */
   const bool is_zero_lookahead_time() const
   {
      return ( lookahead_time <= 0.0 );
   }

   /*! @brief Set the name of the save.
    *  @param save_label Save name. */
   void set_save_name( const std::wstring &save_label )
   {
      this->save_name = save_label;
   }

   /*! @brief Set the name of the restore.
    *  @param restore_label Restore name. */
   void set_restore_name( const std::wstring &restore_label )
   {
      this->restore_name = restore_label;
   }

   /*! @brief Get restart state.
    *  @return True if in restart, False otherwise. */
   bool get_restart() const
   {
      return this->restart_flag;
   }

   /*! @brief Get restart configuration state.
    *  @return True if configuring restart, False otherwise. */
   bool get_restart_cfg() const
   {
      return this->restart_cfg_flag;
   }

   /*! @brief Get stale data counter (DIS only).
    *  @param s Pointer to stale data counter. */
   void get_stale_data_counter( int *s )
   {
      *s = this->stale_data_counter;
   }

   // Routines to set federation state values.
   /*! @brief Set the name of the federation execution.
    *  @param exec_name Federation execution name. */
   void set_federation_name( const std::string &exec_name );

   /*! @brief Query if time advance has been greanted.
    *  @return True if time advance has been granted; False otherwise. */
   bool is_time_advance_granted() const
   {
      return time_adv_grant;
   }

   /*! @brief Set the time advance grant flag.
    *  @param grant_flag Status of time advance grant. */
   void set_time_advance_grant( const bool &grant_flag )
   {
      time_adv_grant = grant_flag;
   }

   /*! @brief Query if the federate is in a time regulating state.
    *  @return True if time regulating; False otherwise. */
   bool in_time_regulating_state() const
   {
      return this->time_regulating_state;
   }

   /*! @brief Set the state of time regulation.
    *  @param regulation_state Desired state of time regulation for this federate. */
   void set_time_regulation_state( const bool &regulation_state )
   {
      time_regulating_state = regulation_state;
   }

   /*! @brief Set the state of time constraint.
    *  @param constrained_state Desired state of time constraint for this federate. */
   void set_time_constrained_state( const bool &constrained_state )
   {
      time_constrained_state = constrained_state;
   }

   /*! @brief Sets the granted time from the specified double.
    *  @param time Granted time in seconds. */
   void set_granted_time( double time );

   /*! @brief Sets the granted time from the specified LogicalTime.
    *  @param time Granted time in HLA logical time. */
   void set_granted_time( const RTI1516_NAMESPACE::LogicalTime &time );

   /*! @brief Sets the requested time from the specified double.
    *  @param time Requested time in seconds. */
   void set_requested_time( double time );

   /*! @brief Sets the requested time from the specified LogicalTime.
    *  @param time Requested time in HLA logical time. */
   void set_requested_time( const RTI1516_NAMESPACE::LogicalTime &time );

   /*! @brief Sets the HLA lookahead time.
    *  @param value HLA lookahead time in seconds. */
   void set_lookahead( double value );

   /*! @brief Set start to save flag.
    *  @param save_flag True if save started; False otherwise. */
   void set_start_to_save( bool save_flag )
   {
      this->start_to_save = save_flag;
   }

   /*! @brief Set start to restore flag.
    *  @param restore_flag True if restore started; False otherwise. */
   void set_start_to_restore( bool restore_flag )
   {
      this->start_to_restore = restore_flag;
   }

   /*! @brief Set restart flag.
    *  @param restart_now True for federate restart; False otherwise. */
   void set_restart( bool restart_now )
   {
      this->restart_flag = restart_now;
   }

   /*! @brief Set restart configuration flag.
    *  @param restart_cfg_now True for configuring restart; False otherwise. */
   void set_restart_cfg( bool restart_cfg_now )
   {
      this->restart_cfg_flag = restart_cfg_now;
   }

   /*! @brief Query if time management is enabled.
    *  @return True if time management is enabled; False otherwise. */
   bool is_time_management_enabled() const
   {
      // Time management is enabled if the local time-management flag is set.
      return time_management;
   }

   // Checkpoint restart initialization.
   /*! @brief Perform initialization after a restart. */
   void restart_initialization();

   /*! @brief Query if federate can rejoin federation.
    *  @return True if federate can rejoin; False otherwise. */
   bool federate_can_rejoin_federation() const
   {
      return can_rejoin_federation;
   }

   /*! @brief Query if a federate is required at startup.
    *  @return True if federate is required at startup; False otherwise.
    *  @param fed_name Name of potentially required federate. */
   bool is_a_required_startup_federate( const std::wstring &fed_name );

   /*! @brief Query if the federation was created by this federate.
    *  @return True if created by this federate; False otherwise. */
   bool is_federation_created_by_federate() const
   {
      return federation_created_by_federate;
   }

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

  private:
   // Federation state variable.
   //
   RTI1516_NAMESPACE::FederateHandle federate_id;                    ///< @trick_io{**} Federate ID.
   bool                              federation_created_by_federate; ///< @trick_io{**} Federate successfully created the federation if True.
   bool                              federation_exists;              ///< @trick_io{**} Federation exists.
   bool                              federation_joined;              ///< @trick_io{**} Federate joined federation flag.
   bool                              all_federates_joined;           ///< @trick_units{--} Master check for all federates joined.

   Int64Interval lookahead; ///< @trick_units{--} Lookahead time for data.

   bool shutdown_called; ///< @trick_units{--} Flag to indicate shutdown has been called.

   std::wstring save_name;    ///< @trick_io{**} Name for a save file
   std::wstring restore_name; ///< @trick_io{**} Name for a restore file

   //-- BEGIN: checkpoint / restore data --
   std::string HLA_save_directory; ///< @trick_io{*i} @trick_units{--} HLA Save directory
   bool        initiate_save_flag; ///< @trick_io{**} Save announce flag

   THLASaveRestoreProcEnum restore_process;       ///< @trick_io{**} Where we are in the restore process
   THLASaveRestoreProcEnum prev_restore_process;  ///< @trick_io{**} previous state of the restore process
   bool                    initiate_restore_flag; ///< @trick_io{**} Restore announce flag
   bool                    restore_in_progress;   ///< @trick_io{**} Restore in progress flag
   bool                    restore_failed;        ///< @trick_io{**} Restore of the federate failed
   bool                    restore_is_imminent;   ///< @trick_io{**} Restore has been signalled by the Manager

   std::string save_label;            ///< @trick_io{**} Save file label
   bool        announce_save;         ///< @trick_io{**} flag to indicate whether we have announced the federation save
   bool        save_label_generated;  ///< @trick_io{**} Save filename has been generated.
   bool        save_request_complete; ///< @trick_io{**} save status request complete
   bool        save_completed;        ///< @trick_io{**} Save completed.

   int stale_data_counter; ///< @trick_units{--} For DIS only: Number of cycles since the last time we received data via HLA.

   std::string restore_label;                               ///< @trick_io{**} Restore file label.
   bool        announce_restore;                            ///< @trick_io{**} flag to indicate whether we have announced the federation restore
   bool        restore_label_generated;                     ///< @trick_io{**} Restore filename has been generated.
   bool        restore_begun;                               ///< @trick_io{**} Restore begun
   bool        restore_request_complete;                    ///< @trick_io{**} restore status request complete
   bool        restore_completed;                           ///< @trick_io{**} Restore completed.
   bool        federation_restore_failed_callback_complete; ///< @trick_io{**} federation not restored callback complete

   bool federate_has_been_restarted; /**< @trick_io{**} Federate has restarted; so, do not restart again! */

   bool publish_data; /**< @trick_io{**} Default true. indicates if this federate's data & interactions should be processed. */

   // The Federates known at execution time. This is loaded when we join the
   // federation and is automatically kept current when other federates
   // join / resign from the federation.
   int            running_feds_count;                    ///< @trick_units{--} Number of running Federates (default: 0)
   KnownFederate *running_feds;                          ///< @trick_units{--} Checkpoint-able Array of running Federation Federates
   int            running_feds_count_at_time_of_restore; ///< @trick_io{**} Number of running Federates at the time of the restore (default: 0)

   std::string checkpoint_file_name;  ///< @trick_io{*i} @trick_units{--} label to attach to sync point
   Flag        checkpoint_rt_itimer;  ///< @trick_io{**} loaded checkpoint RT ITIMER
   bool        announce_freeze;       ///< @trick_io{**} DANNY2.7 flag to indicate that this federate is announcing go to freeze mode
   bool        freeze_the_federation; ///< @trick_io{**} DANNY2.7 flag to indicate the federation is going into freeze now
   bool        execution_has_begun;   ///< @trick_units{--} flag to indicate if the federate has begun simulation execution.
   //-- END: checkpoint / restore data --

   // Federation time management data.
   //
   bool      time_adv_grant;   ///< @trick_units{--} Time advance grant flag.
   Int64Time granted_time;     ///< @trick_units{--} HLA time given by RTI
   Int64Time requested_time;   ///< @trick_units{--} requested/desired HLA time
   double    HLA_time;         ///< @trick_units{s}  Current HLA time.
   bool      start_to_save;    ///< @trick_io{**} Save flag
   bool      start_to_restore; ///< @trick_io{**} Restore flag
   bool      restart_flag;     ///< @trick_io{**} Restart flag
   bool      restart_cfg_flag; ///< @trick_io{**} Restart flag with new configuration

   // Fields related to the initial master/slave interactions.
   //
   bool time_regulating_state;  ///< @trick_units{--} Internal flag, federates HLA Time Regulation state (default: false).
   bool time_constrained_state; ///< @trick_units{--} Internal flag, federates HLA Time Constrained state (default: false).

   bool got_startup_sp;             ///< @trick_units{--} "startup" SP has been created. For DIS compatibility
   bool make_copy_of_run_directory; ///< @trick_units{--} Make a backup of RUN directory before restarting the federation via federation manager (default: false).

   RTI1516_NAMESPACE::ObjectClassHandle MOM_HLAfederation_class_handle;      ///< @trick_io{**} MOM Federation class handle.
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAfederatesInFederation_handle; ///< @trick_io{**} MOM attribute handle to Federate-count.
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAautoProvide_handle;           ///< @trick_io{**} MOM AutoProvide attribute handle.
   TrickHLAObjInstanceNameMap           mom_HLAfederation_instance_name_map; ///< @trick_io{**} Map of the MOM HLAfederation instances.
   int                                  auto_provide_setting;                ///< @trick_units{--} MOM Federation wide HLAautoProvide setting.
   int                                  orig_auto_provide_setting;           ///< @trick_units{--} Original MOM Federation wide HLAautoProvide setting when we joined the federation.

   RTI1516_NAMESPACE::ObjectClassHandle MOM_HLAfederate_class_handle;  ///< @trick_io{**} MOM Federate class handle.
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAfederateType_handle;    ///< @trick_io{**} MOM attribute handle to Federate type (a.k.a name in IEEE 1516-2000).
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAfederateName_handle;    ///< @trick_io{**} MOM attribute handle to Federate name.
   RTI1516_NAMESPACE::AttributeHandle   MOM_HLAfederate_handle;        ///< @trick_io{**} MOM attribute handle to Federate-Handle.
   TrickHLAObjInstanceNameMap           mom_HLAfederate_inst_name_map; ///< @trick_io{**} Map of the MOM HLAfederate instances name map.
   TrickHLAObjInstanceNameMap           joined_federate_name_map;      ///< @trick_io{**} Map of the federate instances.
   RTI1516_NAMESPACE::FederateHandleSet joined_federate_handles;       ///< @trick_io{**} FederateHandles of joined federates.
   VectorOfWstrings                     joined_federate_names;         ///< @trick_io{**} Names of the joined federates.

   RTI1516_NAMESPACE::InteractionClassHandle MOM_HLAsetSwitches_class_handle; ///< @trick_io{**} MOM HLAsetSwitches class handle.
   RTI1516_NAMESPACE::ParameterHandle        MOM_HLAautoProvide_param_handle; ///< @trick_io{**} MOM HLAautoProvide parameter handle.

   // Federation required associations.
   //
   TrickRTIAmbPtr        RTI_ambassador;      ///< @trick_io{**} RTI ambassador
   FedAmb *              federate_ambassador; ///< @trick_units{--} Federate ambassador.
   Manager *             manager;             ///< @trick_units{--} Associated TrickHLA Federate.
   ExecutionControlBase *execution_control;   /**< @trick_units{--} Execution control object. This has to point to an allocated execution control class that inherits from the ExecutionControlBase interface class. For instance SRFOM::ExecutionControl. */

  private:
   /*! @brief Dumps the contents of the running_feds object into the supplied
    *  file name with ".running_feds" appended to it.
    *  @param file_name Checkpoint file name. */
   void write_running_feds_file( const std::string &file_name ) throw( const char * );

   /*! @brief Request federation save from the RTI. */
   void request_federation_save();

   /*! @brief Subscribe to the specified attributes for the given class handle.
    *  @param class_handle   Class handle.
    *  @param attribute_list Attributes handles. */
   void subscribe_attributes( RTI1516_NAMESPACE::ObjectClassHandle         class_handle,
                              RTI1516_NAMESPACE::AttributeHandleSet const &attribute_list );

   /*! @brief Unsubscribe from the specified attributes for the given class handle.
    *  @param class_handle   Class handle.
    *  @param attribute_list Attributes handles. */
   void unsubscribe_attributes( RTI1516_NAMESPACE::ObjectClassHandle         class_handle,
                                RTI1516_NAMESPACE::AttributeHandleSet const &attribute_list );

   /*! @brief Request an update to the specified attributes for the given object class handle.
    *  @param class_handle   Class handle.
    *  @param attribute_list Attributes handles. */
   void request_attribute_update( RTI1516_NAMESPACE::ObjectClassHandle         class_handle,
                                  RTI1516_NAMESPACE::AttributeHandleSet const &attribute_list );

   /*! @brief Publish Interaction class.
    *  @param class_handle Interaction class handle. */
   void publish_interaction_class( RTI1516_NAMESPACE::InteractionClassHandle class_handle );

   /*! @brief Unpublish Interaction class.
    *  @param class_handle Interaction class handle. */
   void unpublish_interaction_class( RTI1516_NAMESPACE::InteractionClassHandle class_handle );

   /*! @brief Send the Interaction for the specified interaction class and parameter list.
    *  @param class_handle   Interaction class handle.
    *  @param parameter_list Parameter values in a map. */
   void send_interaction( RTI1516_NAMESPACE::InteractionClassHandle         class_handle,
                          RTI1516_NAMESPACE::ParameterHandleValueMap const &parameter_list );

   //
   // Internal initialization utilities (invoked by initialize())
   //
   /*! @brief Create the simulation federation if it does not already exist. */
   void create_federation();

   /*! @brief Join a federation.
    *  @param federate_name Name of this federate.
    *  @param federate_type Type for this federate. */
   void join_federation( const char *const federate_name, const char *const federate_type );

   /*! @brief Determine if the specified federate name is a required federate.
    *  @return True if a name of required federate, otherwise false.
    *  @param federate_name Federate name to test. */
   bool is_required_federate( const std::wstring &federate_name );

   /*! @brief Determine if the specified federate name is a joined federate.
    *  @return True if a name of joined federate, otherwise false.
    *  @param federate_name Federate name to test. */
   bool is_joined_federate( const char *federate_name );

   /*! @brief Determine if the specified federate name is a joined federate.
    *  @return True if a name of joined federate, otherwise false.
    *  @param federate_name Federate name to test. */
   bool is_joined_federate( const std::wstring &federate_name );

   /*! @brief Make the HLA time-advance request using the current requested_time value. */
   void perform_time_advance_request();

   //
   // Federation freeze management functions.
   //
   /*! @brief Unfreeze simulation. */
   void un_freeze();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Federate class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Federate( const Federate &rhs );

   /*! @brief Assignment operator for Federate class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Federate &operator=( const Federate &rhs );
};

} // namespace TrickHLA

#endif // _TRICKHLA_FEDERATE_HH_
