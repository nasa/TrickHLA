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
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, May 2026, --, Reformulation for SaveRestore state machine architecture.}
@revs_end

*/

#ifndef TRICKHLA_SAVE_RESTORE_SERVICES_HH
#define TRICKHLA_SAVE_RESTORE_SERVICES_HH

// System includes.
#include <cstddef>
#include <memory>
#include <set>
#include <string>

// Trick includes.
#include "trick/Flag.h"

// TrickHLA includes.
#include "TrickHLA/CheckpointConversionBase.hh"
#include "TrickHLA/CompileConfig.hh"
#include "TrickHLA/HLAStandardSupport.hh"
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
   RESTORE_ACTIVATE          = 1,
   RESTORE_REQUEST_STATUS    = 2,
   RESTORE_STATUS_COMPLETE   = 3,
   RESTORE_REQUESTED         = 4,
   RESTORE_REQUEST_FAILED    = 5,
   RESTORE_REQUEST_SUCCEEDED = 6,
   RESTORE_BEGUN             = 7,
   RESTORE_IN_PROGRESS       = 8,
   RESTORE_COMPLETE          = 9,
   RESTORE_FAILED            = 10,
   RESTORE_UNSUPPORTED       = 0xff
};

/*! @brief Convert a THLARestoreProcessEnum value into a string representation.
 *  @param restore_state Value to convert to string. */
std::string to_string( THLARestoreProcessEnum restore_state );

// Forward Declared Classes: Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Federate;
class ObjectServices;
class InteractionServices;
class TimeManagementServices;
class ExecutionControlBase;

class ScheduledSave
{
  public:
   std::wstring label;
   Int64Time    time;

   // Need a comparison operator for set ordering.  Ordering by time.
   bool operator<( ScheduledSave const &other )
   {
      return ( this->time < other.time ); // Sort by time.
   }
};

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
   friend class FedAmb;

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Federate class.
    *  @param fed Associated Federate instance. */
   explicit SaveRestoreServices( Federate &fed );
   /*! @brief Destructor for the TrickHLA SaveRestoreServices class. */
   virtual ~SaveRestoreServices() override;

   //--------------------------------------------------------------------------
   // Principal public SaveRestoreService functions.
   //--------------------------------------------------------------------------

   //
   // General support functions.
   //
   /*! @brief Set the HLA save directory.
    * @return Success of setting the HLA Save directory path.
    * @detail If a path isn't provided, then a default path is constructed. */
   bool set_HLA_save_directory()
   {
      return set_HLA_save_directory( "" );
   }

   /*! @brief Set the HLA save directory.
    * @return Success of setting the HLA Save directory path.
    * @detail If a path isn't provided, then a default path is constructed.
    * @param  path Full path to the HLA Save directory.  */
   bool set_HLA_save_directory( std::string const &path );

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

   //..........................................................................
   // Save functions.
   //..........................................................................

   /*! @brief Set the Save state.
    *  @param state Desired save state. */
   bool save_set_state( THLASaveProcessEnum state );

   /*! @brief Get the current Federate HLA Save state.
    *  @return Federate HLA Save state. */
   THLASaveProcessEnum save_get_state()
   {
      return ( save_state );
   }

   /*! @brief Set the Save label.
    *  @param label Desired save label. */
   void save_set_label( std::wstring const &label );

   /*! @brief Get the current Federate HLA Save label.
    *  @return Federate HLA Save lable. */
   std::wstring const &save_get_label()
   {
      return ( save_label );
   }

   /*! @brief Set the Save time.
    *  @param time Desired save time. */
   void save_set_time( Int64Time const &time );

   /*! @brief Get the current Federate HLA Save time.
    *  @return Federate HLA Save time. */
   Int64Time save_get_time()
   {
      return ( save_time );
   }

   /*! @brief Tell the federate to initiate a federation Save with the
    * associated Save label. */
   void save_request()
   {
      save_request( L"" );
   }

   /*! @brief Tell the federate to initiate a federation Save with the
    * associated Save label.
    *  @param label The HLA Save label. */
   void save_request( std::wstring const &label );

   /*! @brief Save this federate's state with the associated Save label. */
   void save()
   {
      save( L"" );
   }

   /*! @brief Save this federate's state with the associated Save label.
    *  @param label The HLA Save label. */
   void save( std::wstring const &label );

   /*! @brief Check if a Save is in progress and report.
    *  @return Returns true is the Save is in progress.  Otherwise, it
    *  returns false. */
   bool save_in_progress_check();

   /*! @brief The Federation Save process completed successfully. */
   void save_succeded();

   /*! @brief The Federation Save process did NOT complete successfully. */
   void save_failed();

   /*! @brief Write the joined federates file as part of the Save process.
    *  @detail This routine uses the ExecutionControl class call to map the
    *  HLA Save label into an identifiable file name.  If label is empty then
    *  this routine uses the current Save label. */
   bool write_joined_federates_to_file()
   {
      return write_joined_federates_to_file( L"" );
   }

   /*! @brief Write the joined federates file as part of the Save process.
    *  @detail This routine uses the ExecutionControl class call to map the
    *  HLA Save label into an identifiable file name.  If label is empty then
    *  this routine uses the current Save label.
    *  @param label The identifying Save label. */
   bool write_joined_federates_to_file( std::wstring const &label );

   /*! @brief Prints the reason for the federation save failure.
    * @param reason Save failure reason. */
   static void print_save_failure_reason( RTI1516_NAMESPACE::SaveFailureReason reason );

   /*! @brief Requests the status of the Federation Save. */
   void request_federation_save_status();

   /*! @brief Converts HLA sync points into something Trick can save in a checkpoint. */
   void convert_sync_pts();

   //..........................................................................
   // Restore functions.
   //..........................................................................

   /*! @brief Set the Restore label.
    *  @param label Desired Restore label. */
   void restore_set_label( std::wstring const &label );

   /*! @brief Get the current Federate HLA Save label.
    *  @return Federate HLA Save lable. */
   std::wstring const &restore_get_label()
   {
      return ( restore_label );
   }

   /*! @brief Read the known federates file.
    * @return True is read from file succeeded, False otherwise. */
   bool read_known_federates_from_file()
   {
      return read_known_federates_from_file( L"" );
   }

   /*! @brief Read the known federates file.
    * @return True is read from file succeeded, False otherwise.
    * @param label Restore label. */
   bool read_known_federates_from_file( std::wstring const &label );

   /*! @brief Set the Restore state.
    *  @param state Desired Restore state. */
   bool restore_set_state( THLARestoreProcessEnum state );

   /*! @brief Get the current Federate HLA Restore state.
    *  @return Federate HLA Restore state. */
   THLARestoreProcessEnum restore_get_state()
   {
      return ( restore_state );
   }

   /*! @brief Request that the Federation Restore with the associated Restore label.
    *  @param label The HLA Restore label. */
   void restore_request_status();

   /*! @brief Routine to check if the restore request status in complete. */
   void restore_waiting_for_request_status();

   /*! @brief Format a FederateRestore status response string. */
   static std::string to_string( RTI1516_NAMESPACE::FederateRestoreStatus const &restore_status );

   /*! @brief Format a FederateRestore status response string. */
   static std::string to_string( RTI1516_NAMESPACE::FederateRestoreStatusVector const &response );

   /*! @brief Request that the Federation Restore with the associated Restore label.
    *  @param label The HLA Restore label. */
   void restore_request( std::wstring const &label );

   /*! @brief Checks for Restore request success or failure. */
   void restore_waiting_for_request();

   /*! @brief Function called when a Restore request fails. */
   void restore_request_failed();

   /*! @brief Function called when a Restore request succeeds. */
   void restore_waiting_for_begun();

   /*! @brief Function called when a Restore has begun. */
   void restore_begun();

   /*! @brief Function called cyclicly while waiting for Restore initiated callback. */
   void restore_waiting_for_initiated();

   /*! @brief Function called when a Restore has been initiated. */
   void restore_initiated(
#if defined( IEEE_1516_2025 )
      std::wstring const                      &label,
      std::wstring const                      &federate_name,
      RTI1516_NAMESPACE::FederateHandle const &new_federate_handle );
#else
      std::wstring const               &label,
      std::wstring const               &federate_name,
      RTI1516_NAMESPACE::FederateHandle new_federate_handle );
#endif // IEEE_1516_2025

   /*! @brief Rebuild the HLA state after a checkpoint load. */
   void restore_after_checkpoint_load();

   /*! @brief Function called cyclicly checking on Restore process progress.
    *  @return Returns true if the Restore has been initiated. */
   bool restore_waiting_for_completion();

   /*! @brief Function called to inform the Federation that this federate has
    *  successfully completed a Trick checkpoint Restore. */
   void restore_success_notification();

   /*! @brief Function called to inform the Federation that this federate has
    *  failed to complete a Trick checkpoint Restore. */
   void restore_failed_notification();

   /*! @brief Function called when a Restore process succeeds. */
   void restore_succeded();

   /*! @brief Function called when a Restore process fails. */
   void restore_failed();

   /*! @brief Prints the reason for the federation restore failure.
    * @param reason Restore failure reason. */
   static void restore_failed_print_reason( RTI1516_NAMESPACE::RestoreFailureReason reason );

   /*! @brief Converts checkpointed sync points into HLA sync points. */
   void reinstate_logged_sync_pts();

   //..........................................................................
   // CheckpointConversionBase Interface.
   //..........................................................................
   /*! @brief Convert data to a form Trick can checkpoint. */
   virtual void convert_data_before_checkpoint() override
   {
      return;
   }

   /*! @brief Restore data structures after loading a Trick checkpoint. */
   virtual void restore_data_after_checkpoint() override
   {
      return;
   }

   /*! @brief Clear/release the memory used for the conversion data for the checkpoint. */
   virtual void free_converted_data_for_checkpoint() override
   {
      return;
   }

   //--------------------------------------------------------------------------
   // Potentially deprecated SaveRestoreService functions.
   //--------------------------------------------------------------------------

   /*! @brief Restore checkpoint.
    *  @param file_name Checkpoint file name. */
   void restore_checkpoint( std::string const &file_name );

   /*! @brief Restart the sim from a checkpoint. */
   void restart_checkpoint();

  protected:
   //
   // References to the Federate and associated services.
   //
   Federate               *federate;                ///< @trick_units{--} Associated TrickHLA::Federate.
   ObjectServices         *object_service;          ///< @trick_units{--} Associated ObjectServices.
   InteractionServices    *interaction_service;     ///< @trick_units{--} Associated InteractionServices.
   TimeManagementServices *time_management_service; ///< @trick_units{--} Associated TrickHLA::TimeManagementServices.
   ExecutionControlBase   *execution_control;       ///< @trick_units{--} Associated TrickHLA::ExecutionControlBase.

   // The SaveRestoreServices information known at execution time. This is
   // loaded when we join the federation and is automatically kept current when
   // other federates join / resign from the federation.
   std::string joined_federates_file_name; ///< @trick_io{**} File containing the names of the joined federates.

   // Save and Restore variables.
   std::string HLA_save_directory;     ///< @trick_units{--} HLA Save directory.
 
   // Save process variables.
   THLASaveProcessEnum save_state; ///< @trick_units{1} Where we are in the Save process.
   std::wstring        save_label; ///< @trick_units{--} Save label.
   Int64Time           save_time;  ///< @trick_units{--} HLA Logical Time for Save.

   // Restore process variables.
   THLARestoreProcessEnum            restore_state;  ///< @trick_io{**} Where we are in the restore process
   std::wstring                      restore_label;  ///< @trick_io{**} Restore label.
   std::wstring                      restore_name;   ///< @trick_io{**} Restored federate name.
   RTI1516_NAMESPACE::FederateHandle restore_handle; ///< @trick_io{**} Restored federate handle.

   RTI1516_NAMESPACE::FederateRestoreStatusVector restore_status_response; ///< @trick_io{**} Federation Restore status vector.

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
