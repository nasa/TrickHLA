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
@trick_link_dependency{../../source/TrickHLA/ExecutionControlBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/SaveRestoreServices.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/TrickHLA/time/TimeManagementServices.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, March 2026, --, Refactor HLA Save and Restore services.}
@revs_end

*/

#ifndef TRICKHLA_SAVE_RESTORE_SERVICES_HH
#define TRICKHLA_SAVE_RESTORE_SERVICES_HH

// System includes.
#include <cstddef>
#include <map>
#include <memory>
#include <string>

// Trick includes.
#include "trick/Flag.h"

// TrickHLA includes.
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/KnownFederate.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Time.hh"

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
//#if defined( IEEE_1516_2025 )
//#   include "RTI/time/LogicalTime.h"
//#else
//#   include "RTI/LogicalTime.h"
//#endif // IEEE_1516_2025

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

namespace TrickHLA
{

/*
 * Enumerated type used to step through the Save process.
 */
enum class THLASaveProcessEnum : uint8_t {
   SAVE_NONE        = 0,
   SAVE_INITIATED   = 1,
   SAVE_REQUESTED   = 2,
   SAVE_IN_PROGRESS = 3,
   SAVE_COMPLETE    = 4,
   SAVE_FAILED      = 5,
   SAVE_UNSUPPORTED = 0xff
};

/*! @brief Convert a THLASaveProcessEnum value into a string representation.
 *  @param save_state Value to convert to a string. */
std::string to_string( THLASaveProcessEnum save_state );

/*
 * Enumerated type used to step through the Restore process.
 */
enum class THLARestoreProcessEnum : uint8_t {
   RESTORE_NONE              = 0,
   RESTORE_REQUESTED         = 1,
   RESTORE_REQUEST_FAILED    = 2,
   RESTORE_REQUEST_SUCCEEDED = 3,
   RESTORE_INITIATE          = 4,
   RESTORE_IN_PROGRESS       = 5,
   RESTORE_COMPLETE          = 6,
   RESTORE_FAILED            = 7,
   RESTORE_UNSUPPORTED       = 0xff
};

/*! @brief Convert a THLARestoreProcessEnum value into a string representation.
 *  @param restore_state Value to convert to string. */
std::string to_string( THLARestoreProcessEnum restore_state );

// Forward Declared Classes: Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Federate;
class TimeManagementServices;
class ExecutionControlBase;

class SaveRestoreServices : public CheckpointConversionBase
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

   // Allow the TrickHLA core classes to have direct access to protected
   // and private data.
   friend class Federate;
   friend class ExecutionControlBase;

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Federate class.
    *  @param fed Associated Federate instance. */
   explicit SaveRestoreServices( Federate &fed );
   /*! @brief Destructor for the TrickHLA SaveRestoreServices class. */
   virtual ~SaveRestoreServices();

   //--------------------------------------------------------------------------
   // Principal public SaveRestoreService functions.
   //--------------------------------------------------------------------------

   //
   // General support functions.
   //
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
   std::size_t get_running_feds_count() const
   {
      return running_feds_count;
   }

   /*! @brief Set the HLA save directory.
    * @return Success of setting the HLA Save directory path.
    * @detail If a path isn't provided, then a default path is constructed.
    * @param  path Full path to the HLA Save directory.  */
   bool set_HLA_save_directory( std::string const & path = "" );

   /*! @brief Get the HLA save directory.
    * @return HLA save directory. */
   std::string const &get_HLA_save_directory()
   {
      return HLA_save_directory;
   }

   /*! @brief Check if HLA_save_directory is empty. If so, ask the EXECUTIVE
    * for info and build the absolute path of the RUN directory.
    * @return Success of HLA Save directory path. */
   bool check_HLA_save_directory();

   //
   // CheckpointConversionBase Interface.
   //
   /*! @brief Convert data to a form Trick can checkpoint. */
   virtual void convert_data_before_checkpoint()
   {
      return;
   }

   /*! @brief Restore data structures after loading a Trick checkpoint. */
   virtual void restore_data_after_checkpoint()
   {
      return;
   }

   /*! @brief Clear/release the memory used for the conversion data for the checkpoint. */
   virtual void free_converted_data_for_checkpoint()
   {
      return;
   }

   //..........................................................................
   // Save functions.
   //..........................................................................

   /*! @brief Set if Trick Control Panel checkpoint Save is supported.
    *  @param state Desired save state. */
   void set_tcp_save_supported( bool status )
   {
      support_tcp_checkpoint = status;
      return;
   }

   /*! @brief Check if Trick Control Panel checkpoint Save is supported.
    *  @return TCP Save support status. */
   bool is_tcp_save_supported()
   {
      return( support_tcp_checkpoint );
   }

   /*! @brief Set the Save state.
    *  @param state Desired save state. */
   bool set_save_state( THLASaveProcessEnum state );

   /*! @brief Get the current Federate HLA Save state.
    *  @return Federate HLA Save state. */
   THLASaveProcessEnum get_save_state()
   {
      return( save_state );
   }

   /*! @brief Set the Save label.
    *  @param label Desired save label. */
   void set_save_label( std::wstring const & label );

   /*! @brief Get the current Federate HLA Save label.
    *  @return Federate HLA Save lable. */
   std::wstring get_save_label()
   {
      return( save_label );
   }

   /*! @brief Set the Save time.
    *  @param time Desired save time. */
   void set_save_time( Int64Time const & time );

   /*! @brief Get the current Federate HLA Save time.
    *  @return Federate HLA Save time. */
   Int64Time get_save_time()
   {
      return( save_time );
   }

   /*! @brief Tell the federate to initiate a federation Save with the
    * associated Save label.
    *  @param label The HLA Save label. */
   void save_request( std::wstring const &label = L"" );

   /*! @brief Save this federate's state with the associated Save label.
    *  @param label The HLA Save label. */
   void save( std::wstring const &label = L"" );

   /*! @brief Check if a Save is in progress and report.
    *  @return Returns true is the Save is in progress.  Otherwise, it
    *  returns false. */
   bool save_in_progress_check();

   /*! @brief The Federation Save process completed successfully. */
   void save_succeded();

   /*! @brief The Federation Save process did NOT complete successfully. */
   void save_failed();

   /*! @brief Dumps the contents of the running_feds object into the supplied
    *  file name with ".running_feds" appended to it.
    *  @param file_name Checkpoint file name. */
   void write_running_feds_file( std::string const &file_name );

   /*! @brief Prints the reason for the federation save failure.
    * @param reason Save failure reason. */
   static void print_save_failure_reason( RTI1516_NAMESPACE::SaveFailureReason reason );


   //..........................................................................
   // Restore functions.
   //..........................................................................

   /*! @brief Read the running_feds file, replacing the data in known federates
    * data structure.
    * @param file_name Checkpoint file name. */
   void read_running_feds_file( std::string const &file_name );


   //--------------------------------------------------------------------------
   // Potentially deprecated SaveRestoreService functions.
   //--------------------------------------------------------------------------

   //
   // Accessor functions.
   //

   /*! @brief Start the federation save as soon as possible.
    *  @param label The HLA Save label. */
   void start_federation_save( std::wstring const &label );

   /*! @brief Start the Federation save at the specified simulation time.
    *  @param set   Simulation elapsed time to save.
    *  @param label The HLA Save label. */
   void start_federation_save_at_SET( double              set,
                                      std::wstring const &label );

   /*! @brief Start the Federation save at the specified scenario time.
    *  @param sst   Scenario scenario time to save.
    *  @param label The HLA Save label. */
   void start_federation_save_at_SST( double              sst,
                                      std::wstring const &label );

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
      return ( restore_state == THLARestoreProcessEnum::RESTORE_REQUEST_FAILED );
   }

   /*! @brief Query if restore process restore request succeeded.
    *  @return True if succeeded, False otherwise. */
   bool has_restore_process_restore_request_succeeded() const
   {
      return ( restore_state == THLARestoreProcessEnum::RESTORE_REQUEST_SUCCEEDED );
   }

   /*! @brief Query if restore request failed.
    *  @return True if failed, False otherwise. */
   bool has_restore_request_failed() const
   {
      return ( restore_state == THLARestoreProcessEnum::RESTORE_REQUEST_FAILED );
   }

   /*! @brief Query if restore request succeeded.
    *  @return True if succeeded, False otherwise. */
   bool has_restore_request_succeeded() const
   {
      return ( restore_state == THLARestoreProcessEnum::RESTORE_REQUEST_SUCCEEDED );
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
   void set_save_completed();

   /*! @brief Get save completed flag state.
    *  @return True if flag set, false otherwise. */
   bool is_save_completed()
   {
      return save_completed;
   }

   /*! @brief Get the restore process state.
    *  @return The Restore process state. */
   THLARestoreProcessEnum get_restore_process()
   {
      return restore_state;
   }

   /*! @brief Set the restore process state.
    *  @param process_state The Restore process state. */
   void set_restore_process( THLARestoreProcessEnum process_state )
   {
      this->restore_state = process_state;
   }

   /*! @brief Save the restore process state as a previous value. */
   void preserve_restore_process()
   {
      this->prev_restore_process = restore_state;
   }

   /*! @brief Set the restore begun state. */
   void set_restore_begun();

   /*! @brief Set the restore completed state. */
   void set_restore_completed();

   /*! @brief Set the restore failed state. */
   void set_restore_failed();

   /*! @brief Set the restore request failed state. */
   void set_restore_request_failed()
   {
      this->restore_state = THLARestoreProcessEnum::RESTORE_REQUEST_FAILED;
   }

   /*! @brief Set the restore request succeeded state. */
   void set_restore_request_succeeded()
   {
      this->restore_state = THLARestoreProcessEnum::RESTORE_REQUEST_SUCCEEDED;
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
   void initiate_restore_announce( std::wstring const &restore_name_label );

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

   /*! @brief Set the federate has begun execution state. */
   void set_federate_has_begun_execution();

   /*! @brief Get the name of the save.
    *  @return Save name as a wide string. */
   std::wstring const &get_save_name()
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
   std::wstring const &get_restore_name()
   {
      return restore_name;
   }

   /*! @brief Set the name of the restore.
    *  @param name Restore label name. */
   void set_restore_name( std::wstring const &name )
   {
      this->restore_name = name;
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

   /*! @brief Request federation save from the RTI. */
   //void request_federation_save();


  protected:
   //
   // References to the Federate and associated services.
   //
   Federate               *federate;                ///< @trick_units{--} Associated TrickHLA::Federate.
   TimeManagementServices *time_management_service; ///< @trick_units{--} Associated TrickHLA::TimeManagementServices.
   ExecutionControlBase   *execution_control;       ///< @trick_units{--} Associated TrickHLA::ExecutionControlBase.

   // The SaveRestoreServices information known at execution time. This is
   // loaded when we join the federation and is automatically kept current when
   // other federates join / resign from the federation.
   std::size_t    running_feds_count;                    ///< @trick_units{--} Number of running SaveRestoreServicess (default: 0)
   KnownFederate *running_feds;                          ///< @trick_units{--} Checkpoint-able Array of running Federation SaveRestoreServicess
   std::size_t    running_feds_count_at_time_of_restore; ///< @trick_io{**} Number of running Federates at the time of the restore (default: 0)
   std::string    running_feds_file_name;                ///< @trick_io{**} File containing the names of the running federates.

   // Save and Restore variables.
   bool        support_tcp_checkpoint; ///< @trick_units{--} Support Save/Restore from Trick Control Panel.
   std::string checkpoint_file_name;   ///< @trick_units{--} Checkpoint file name generated from Save label.
   std::string HLA_save_directory;     ///< @trick_units{--} HLA Save directory.

   // Save process variables.
   THLASaveProcessEnum save_state; ///< @trick_units{1} Where we are in the Save process.
   std::wstring        save_label; ///< @trick_units{--} Save label.
   Int64Time           save_time;  ///< @trick_units{--} HLA Logical Time for Save.


   // Restore process variables.
   THLARestoreProcessEnum restore_state; ///< @trick_units{1} Where we are in the restore process
   std::wstring           restore_label; ///< @trick_units{--} Restore label.


   //*************************************************************************
   // Possibly deprecated variables.
   //*************************************************************************

   // Save variables
   std::wstring save_name;             ///< @trick_io{**} Name for a save file
   bool         initiate_save_flag;    ///< @trick_io{**} Save announce flag
   bool         announce_save;         ///< @trick_io{**} flag to indicate whether we have announced the federation save
   bool         save_label_generated;  ///< @trick_io{**} Save filename has been generated.
   bool         save_request_complete; ///< @trick_io{**} save status request complete
   bool         save_completed;        ///< @trick_io{**} Save completed.
   bool         start_to_save;         ///< @trick_io{**} Save flag
   bool         initiated_a_federation_save; ///< @trick_io{**} Did this federate initiate the federation save?

   bool unfreeze_after_save; /**< @trick_units{--} Flag to indicate that we should go to run immediately after a save. */

   // Restore variables.
  public:
   bool        restore_federation;          ///< @trick_io{*i} @trick_units{--} Flag indicating whether to trigger the restore.
   std::string restore_file_name;           ///< @trick_io{*i} @trick_units{--} Filename, which will be the label name.

  protected:
   std::wstring           restore_name;          ///< @trick_io{**} Name for a restore file
   THLARestoreProcessEnum prev_restore_process;  ///< @trick_io{**} previous state of the restore process
   bool initiate_restore_flag; ///< @trick_io{**} Restore announce flag
   bool restore_in_progress;   ///< @trick_io{**} Restore in progress flag
   bool restore_failed;        ///< @trick_io{**} Restore of the federate failed
   bool restore_is_imminent;   ///< @trick_io{**} Restore has been signaled by the Federate
   bool announce_restore;                            ///< @trick_io{**} flag to indicate whether we have announced the federation restore
   bool restore_label_generated;                     ///< @trick_io{**} Restore filename has been generated.
   bool restore_begun;                               ///< @trick_io{**} Restore begun
   bool restore_request_complete;                    ///< @trick_io{**} restore status request complete
   bool restore_completed;                           ///< @trick_io{**} Restore completed.
   bool federation_restore_failed_callback_complete; ///< @trick_io{**} federation not restored callback complete

   bool federate_has_been_restarted; /**< @trick_io{**} SaveRestoreServices has restarted; so, do not restart again! */

   bool start_to_restore; ///< @trick_io{**} Restore flag
   bool restart_flag;     ///< @trick_io{**} Restart flag
   bool restart_cfg_flag; ///< @trick_io{**} Restart flag with new configuration

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SaveRestoreServices class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SaveRestoreServices( SaveRestoreServices const &rhs );

   /*! @brief Assignment operator for SaveRestoreServices class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SaveRestoreServices &operator=( SaveRestoreServices const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_SAVE_RESTORE_SERVICES_HH
