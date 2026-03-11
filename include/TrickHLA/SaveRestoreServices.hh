/*!
@file TrickHLA/SaveRestoreServices.hh
@ingroup TrickHLA
@brief This class provides basic HLA save and restore services for a federate.

@copyright Copyright 2026 United States Government as represented by the
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
@trick_link_dependency{../../source/TrickHLA/FedAmb.cpp}
@trick_link_dependency{../../source/TrickHLA/SaveRestoreServices.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/time/Int64Time.cpp}
@trick_link_dependency{../../source/TrickHLA/time/TimeManagement.cpp}
@trick_link_dependency{../../source/TrickHLA/time/TrickThreadCoordinator.cpp}
@trick_link_dependency{../../source/TrickHLA/utils/MutexLock.cpp}
@trick_link_dependency{../../source/TrickHLA/utils/MutexProtection.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2026, --, Refactor HLA Save and Restore services.}
@revs_end

*/

#ifndef TRICKHLA_SAVE_RESTORE_SERVICES_HH
#define TRICKHLA_SAVE_RESTORE_SERVICES_HH

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
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Interval.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/time/TimeManagement.hh"
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

/*
 * Enumerated type used to step through the restore process.
 */
typedef enum {
   NO_RESTORE                = 0,
   RESTORE_REQUEST_FAILED    = 1,
   RESTORE_REQUEST_SUCCEEDED = 2,
   INITIATE_RESTORE          = 3,
   RESTORE_IN_PROGRESS       = 4,
   RESTORE_COMPLETE          = 5,
   RESTORE_FAILED            = 6
} THLASaveRestoreProcEnum;

class SaveRestoreServices
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__SaveRestoreServices();

   //--------------------------------------------------------------------------
   // Public member functions.
   //--------------------------------------------------------------------------
  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA SaveRestoreServices class. */
   explicit SaveRestoreServices( Federate *fed );
   /*! @brief Destructor for the TrickHLA SaveRestoreServices class. */
   virtual ~SaveRestoreServices();

   /*! @brief Load the running federate names from the RTI. */
   void load_and_print_running_federate_names();

   /*! @brief Update running federates based on current known information. */
   void update_running_feds();

   /*! @brief Deallocate running federates based on current known information
    * in preparation for re-size. */
   void clear_running_feds();

   /*! @brief Grow the running_feds by one entry. */
   void add_a_single_entry_into_running_feds();

   /*! @brief Get the count of the currently running federates.
    *  @return Count of the currently running federates. */
   int get_running_feds_count() const
   {
      return running_feds_count;
   }

   /*! @brief Read the running_feds file, replacing the data in known federates
    * data structure.
    * @param file_name Checkpoint file name. */
   void read_running_feds_file( std::string const &file_name );

   /*! @brief Copies the contents of the checkpoint's list of federates into
    * known federates data structure. */
   void copy_running_feds_into_known_feds();

   /*! @brief Restore checkpoint.
    *  @param file_name Checkpoint file name. */
   void restore_checkpoint( std::string const &file_name );

   /*! @brief Inform the RTI of the success or failure of the federate restore. */
   void inform_RTI_of_restore_completion();

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
      return ( restore_process == RESTORE_REQUEST_FAILED );
   }

   /*! @brief Query if restore process restore request succeeded.
    *  @return True if succeeded, False otherwise. */
   bool has_restore_process_restore_request_succeeded() const
   {
      return ( restore_process == RESTORE_REQUEST_SUCCEEDED );
   }

   /*! @brief Query if restore request failed.
    *  @return True if failed, False otherwise. */
   bool has_restore_request_failed() const
   {
      return ( restore_process == RESTORE_REQUEST_FAILED );
   }

   /*! @brief Query if restore request succeeded.
    *  @return True if succeeded, False otherwise. */
   bool has_restore_request_succeeded() const
   {
      return ( restore_process == RESTORE_REQUEST_SUCCEEDED );
   }

   /*! @brief Get the announce save flag.
    *  @return The state of the announce save flag. */
   bool is_announce_save()
   {
      return announce_save;
   }

   /*! @brief Set the announce save flag. */
   void set_announce_save()
   {
      this->announce_save = true;
   }

   /*! @brief Set the announce save flag.
    *  @param flag State to set flag. */
   void set_announce_save( bool flag )
   {
      this->announce_save = flag;
   }

   /*! @brief Set the save completed state. */
   void set_save_completed()
   {
      this->save_completed = true;
      this->start_to_save  = false;
      this->publish_data   = true;
   }

   /*! @brief Get save completed flag state.
    *  @return True if flag set, false otherwise. */
   bool is_save_completed()
   {
      return save_completed;
   }

   /*! @brief Get the restore process state.
    *  @return The Restore process state. */
   THLASaveRestoreProcEnum get_restore_process()
   {
      return restore_process;
   }

   /*! @brief Set the restore process state.
    *  @param process_state The Restore process state. */
   void set_restore_process( THLASaveRestoreProcEnum process_state )
   {
      this->restore_process = process_state;
   }

   /*! @brief Save the restore process state as a previous value. */
   void preserve_restore_process()
   {
      this->prev_restore_process = restore_process;
   }

   /*! @brief Set the restore begun state. */
   void set_restore_begun()
   {
      this->restore_begun     = true;
      this->restore_completed = false;
      this->publish_data      = false;
   }

   /*! @brief Set the restore completed state. */
   void set_restore_completed()
   {
      this->restore_process   = RESTORE_COMPLETE;
      this->restore_completed = true;
      this->restore_begun     = false;
      this->start_to_restore  = false;
      this->publish_data      = true;
   }

   /*! @brief Set the restore failed state. */
   void set_restore_failed()
   {
      this->restore_process   = RESTORE_FAILED;
      this->restore_completed = true;
      this->restore_begun     = false;
      this->start_to_restore  = false;
      this->publish_data      = true;
   }

   /*! @brief Set the restore request failed state. */
   void set_restore_request_failed()
   {
      this->restore_process = RESTORE_REQUEST_FAILED;
   }

   /*! @brief Set the restore request succeeded state. */
   void set_restore_request_succeeded()
   {
      this->restore_process = RESTORE_REQUEST_SUCCEEDED;
   }

   /*! @brief Query if federate should publish data.
    *  @return True if data should be published; False otherwise. */
   bool should_publish_data() const
   {
      return publish_data;
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
   static void print_save_failure_reason( RTI1516_NAMESPACE::SaveFailureReason reason );

   /*! @brief Save the supplied checkpoint file name.
    * @param name Checkpoint file name. */
   void set_checkpoint_file_name( std::string const &name );

   /*! @brief Get the state of the save initiated flag.
    *  @return True if save has been initiated, false otherwise. */
   bool is_initiate_save_flag()
   {
      return initiate_save_flag;
   }

   /*! @brief Set the initiate save flag.
    *  @param state The initiate save flag state. */
   void set_initiate_save_flag( bool state )
   {
      this->initiate_save_flag = state;
   }

   /*! @brief Sets the Save filename and flag. */
   void initiate_save_announce();

   /*! @brief Sets the Save filename and flag.
    *  @param restore_name_label Restore file name. */
   void initiate_restore_announce( std::string const &restore_name_label );

   /*! @brief Sets the Save filename and flag.
    *  @return True if restore has been announced; False otherwise. */
   bool has_restore_been_announced() const
   {
      return restore_begun;
   }

   /*! @brief Informs of completion of federation restore. */
   void complete_restore();

   /*! @brief Converts HLA sync points into something Trick can save in a checkpoint. */
   void convert_sync_pts();

   /*! @brief Converts checkpointed sync points into HLA sync points. */
   void reinstate_logged_sync_pts();

   /*! @brief Set the start to save flag. */
   void set_start_to_save()
   {
      start_to_save = true;
   }

   /*! @brief Get the HLA save directory.
    * @return HLA save directory. */
   std::string const &get_HLA_save_directory()
   {
      return HLA_save_directory;
   }

   /*! @brief Check if HLA_save_directory is empty. If so, ask the EXECUTIVE
    * for info and build the absolute path of the RUN directory. */
   void check_HLA_save_directory();

   /*! @brief Set the federate has begun execution state. */
   void set_federate_has_begun_execution();

   /*! @brief Get the save label.
    *  @return Save label. */
   std::string const &get_save_label()
   {
      return save_label;
   }

   /*! @brief Get the name of the save.
    *  @return Save name as a wide string. */
   std::wstring get_save_name()
   {
      return save_name;
   }

   /*! @brief Set the name of the save.
    *  @param name Save name. */
   void set_save_name( std::wstring const &name )
   {
      this->save_name = name;
   }

   /*! @brief Get the name of the restore.
    *  @return Restore name. */
   std::wstring get_restore_name()
   {
      return restore_name;
   }

   /*! @brief Set the name of the restore.
    *  @param restore_label Restore name. */
   void set_restore_name( std::wstring const &restore_label )
   {
      this->restore_name = restore_label;
   }

   /*! @brief Get announce restore flag.
    *  @return True if restore announced, False otherwise. */
   bool is_announce_restore()
   {
      return announce_restore;
   }

   /*! @brief Set announce restore flag.
    *  @param flag announce restore flag. */
   void set_announce_restore( bool flag )
   {
      this->announce_restore = flag;
   }

   /*! @brief Get the state of the start-to-save flag.
    *  @return True is save is started, false otherwise.
    */
   bool is_start_to_save()
   {
      return start_to_save;
   }

   /*! @brief Set start to save flag.
    *  @param save_flag True if save started; False otherwise. */
   void set_start_to_save( bool const save_flag )
   {
      this->start_to_save = save_flag;
   }

   /*! @brief Query if federate has started a restore process.
    *  @return True if restore has started; False otherwise. */
   bool is_start_to_restore() const
   {
      return start_to_restore;
   }

   /*! @brief Set start to restore flag.
    *  @param restore_flag True if restore started; False otherwise. */
   void set_start_to_restore( bool const restore_flag )
   {
      this->start_to_restore = restore_flag;
   }

   /*! @brief Get restart state.
    *  @return True if in restart, False otherwise. */
   bool is_restart() const
   {
      return restart_flag;
   }

   /*! @brief Set restart flag.
    *  @param restart_now True for federate restart; False otherwise. */
   void set_restart( bool const restart_now )
   {
      this->restart_flag = restart_now;
   }

   /*! @brief Get restart configuration state.
    *  @return True if configuring restart, False otherwise. */
   bool is_restart_cfg() const
   {
      return restart_cfg_flag;
   }

   /*! @brief Set restart configuration flag.
    *  @param restart_cfg_now True for configuring restart; False otherwise. */
   void set_restart_cfg( bool const restart_cfg_now )
   {
      this->restart_cfg_flag = restart_cfg_now;
   }

   /*! @brief Dumps the contents of the running_feds object into the supplied
    *  file name with ".running_feds" appended to it.
    *  @param file_name Checkpoint file name. */
   void write_running_feds_file( std::string const &file_name );

   /*! @brief Request federation save from the RTI. */
   void request_federation_save();

  protected:
   Federate *federate;

   std::wstring save_name;    ///< @trick_io{**} Name for a save file
   std::wstring restore_name; ///< @trick_io{**} Name for a restore file

   std::string HLA_save_directory; ///< @trick_io{*i} @trick_units{--} HLA Save directory
   bool        initiate_save_flag; ///< @trick_io{**} Save announce flag

   THLASaveRestoreProcEnum restore_process;       ///< @trick_io{**} Where we are in the restore process
   THLASaveRestoreProcEnum prev_restore_process;  ///< @trick_io{**} previous state of the restore process
   bool                    initiate_restore_flag; ///< @trick_io{**} Restore announce flag
   bool                    restore_in_progress;   ///< @trick_io{**} Restore in progress flag
   bool                    restore_failed;        ///< @trick_io{**} Restore of the federate failed
   bool                    restore_is_imminent;   ///< @trick_io{**} Restore has been signalled by the Manager

   std::string save_label;            ///< @trick_io{**} Save label
   bool        announce_save;         ///< @trick_io{**} flag to indicate whether we have announced the federation save
   bool        save_label_generated;  ///< @trick_io{**} Save filename has been generated.
   bool        save_request_complete; ///< @trick_io{**} save status request complete
   bool        save_completed;        ///< @trick_io{**} Save completed.

   std::string restore_label;                               ///< @trick_io{**} Restore file label.
   bool        announce_restore;                            ///< @trick_io{**} flag to indicate whether we have announced the federation restore
   bool        restore_label_generated;                     ///< @trick_io{**} Restore filename has been generated.
   bool        restore_begun;                               ///< @trick_io{**} Restore begun
   bool        restore_request_complete;                    ///< @trick_io{**} restore status request complete
   bool        restore_completed;                           ///< @trick_io{**} Restore completed.
   bool        federation_restore_failed_callback_complete; ///< @trick_io{**} federation not restored callback complete

   bool federate_has_been_restarted; /**< @trick_io{**} SaveRestoreServices has restarted; so, do not restart again! */

   bool publish_data; /**< @trick_io{**} Default true. indicates if this federate's data & interactions should be processed. */

   // The SaveRestoreServicess known at execution time. This is loaded when we join the
   // federation and is automatically kept current when other federates
   // join / resign from the federation.
   int            running_feds_count;                    ///< @trick_units{--} Number of running SaveRestoreServicess (default: 0)
   KnownFederate *running_feds;                          ///< @trick_units{--} Checkpoint-able Array of running Federation SaveRestoreServicess
   int            running_feds_count_at_time_of_restore; ///< @trick_io{**} Number of running SaveRestoreServicess at the time of the restore (default: 0)

   std::string checkpoint_file_name; ///< @trick_io{*i} @trick_units{--} label to attach to sync point
   Flag        checkpoint_rt_itimer; ///< @trick_io{**} loaded checkpoint RT ITIMER

   bool execution_has_begun; ///< @trick_units{--} Flag to indicate if the federate has begun simulation execution.

   bool start_to_save;    ///< @trick_io{**} Save flag
   bool start_to_restore; ///< @trick_io{**} Restore flag
   bool restart_flag;     ///< @trick_io{**} Restart flag
   bool restart_cfg_flag; ///< @trick_io{**} Restart flag with new configuration

  private:
   // Do not allow the copy constructor or assignment operator.
   SaveRestoreServices();
   /*! @brief Copy constructor for SaveRestoreServices class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SaveRestoreServices( SaveRestoreServices const &rhs );

   /*! @brief Assignment operator for SaveRestoreServices class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SaveRestoreServices &operator=( SaveRestoreServices const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_SAVE_RESTORE_SERVICES_HH
