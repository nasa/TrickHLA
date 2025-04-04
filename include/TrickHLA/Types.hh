/*!
@ingroup TrickHLA
@file TrickHLA/Types.hh
@brief Definition of the TrickHLA enumeration types and utilities.

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
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_TYPES_HH
#define TRICKHLA_TYPES_HH

// System include files.
#include <cstdint>
#include <limits.h>
#include <map>
#include <queue>
#include <string>
#include <vector>

// TrickHLA include files.
#include "TrickHLA/StandardsSupport.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

// Default/desired padding time in seconds.
#define THLA_PADDING_DEFAULT 0.5

namespace TrickHLA
{

static std::string const MULTIPHASE_INIT_SYNC_POINT_LIST = "Multiphase";

/*!
@enum HLABaseTimeEnum
@brief Define the HLA Logical Time base units supported.
*/
typedef enum {

   HLA_BASE_TIME_SECONDS          = 0,  ///< Range +/-292471208677.536 years with 1 second resolution.
   HLA_BASE_TIME_100_MILLISECONDS = 1,  ///< Range +/-29247120867.753 years with 100 millisecond resolution.
   HLA_BASE_TIME_10_MILLISECONDS  = 2,  ///< Range +/-2924712086.775 years with 10 millisecond resolution.
   HLA_BASE_TIME_MILLISECONDS     = 3,  ///< Range +/-292471208.677 years with 1 millisecond resolution.
   HLA_BASE_TIME_100_MICROSECONDS = 4,  ///< Range +/-29247120.867 years with 100 microsecond resolution.
   HLA_BASE_TIME_10_MICROSECONDS  = 5,  ///< Range +/-2924712.086 years with 10 microsecond resolution.
   HLA_BASE_TIME_MICROSECONDS     = 6,  ///< Range +/-292471.208 years with 1 microsecond resolution.
   HLA_BASE_TIME_100_NANOSECONDS  = 7,  ///< Range +/-29247.120 years with 100 nanosecond resolution.
   HLA_BASE_TIME_10_NANOSECONDS   = 8,  ///< Range +/-2924.712 years with 10 nanosecond resolution.
   HLA_BASE_TIME_NANOSECONDS      = 9,  ///< Range +/-292.471 years with 1 nanosecond resolution.
   HLA_BASE_TIME_100_PICOSECONDS  = 10, ///< Range +/-29.247 years with 100 picosecond resolution.
   HLA_BASE_TIME_10_PICOSECONDS   = 11, ///< Range +/-2.924 years with 10 picosecond resolution.
   HLA_BASE_TIME_PICOSECONDS      = 12, ///< Range +/-2562.047 hours with 1 picosecond resolution.
   HLA_BASE_TIME_100_FEMTOSECONDS = 13, ///< Range +/-256.204 hours with 100 femosecond resolution.
   HLA_BASE_TIME_10_FEMTOSECONDS  = 14, ///< Range +/-25.620 hours with 10 femosecond resolution.
   HLA_BASE_TIME_FEMTOSECONDS     = 15, ///< Range +/-2.562 hours with 1 femosecond resolution.
   HLA_BASE_TIME_100_ATTOSECONDS  = 16, ///< Range +/-922.337 seconds with 100 attosecond resolution.
   HLA_BASE_TIME_10_ATTOSECONDS   = 17, ///< Range +/-92.233 seconds with 10 attosecond resolution.
   HLA_BASE_TIME_ATTOSECONDS      = 18  ///< Range +/-9.223 seconds with 1 attosecond resolution.

} HLABaseTimeEnum;

/*!
@enum DataUpdateEnum
@brief Define the TrickHLA attribute update reflection type.
*/
typedef enum {

   CONFIG_NONE                          = 0x0001, ///< No configuration.
   CONFIG_INITIALIZE                    = 0x0002, ///< Dynamic simulation initialization.
   CONFIG_INTERMITTENT                  = 0x0004, ///< Intermittent updates.
   CONFIG_INITIALIZE_AND_INTERMITTENT   = CONFIG_INITIALIZE + CONFIG_INTERMITTENT,
   CONFIG_CYCLIC                        = 0x0008, ///< Cyclic updates.
   CONFIG_INITIALIZE_AND_CYCLIC         = CONFIG_INITIALIZE + CONFIG_CYCLIC,
   CONFIG_ZERO_LOOKAHEAD                = 0x0010, ///< Zero lookahead with Timestamp Order (TSO) data updates.
   CONFIG_INITIALIZE_AND_ZERO_LOOKAHEAD = CONFIG_INITIALIZE + CONFIG_ZERO_LOOKAHEAD,
   CONFIG_BLOCKING_IO                   = 0x0020, ///< Blocking I/O with Receiver Order (RO) data updates.
   CONFIG_INITIALIZE_AND_BLOCKING_IO    = CONFIG_INITIALIZE + CONFIG_BLOCKING_IO,
   CONFIG_MAX_VALUE                     = ( CONFIG_NONE
                        + CONFIG_INITIALIZE
                        + CONFIG_INTERMITTENT
                        + CONFIG_CYCLIC
                        + CONFIG_ZERO_LOOKAHEAD
                        + CONFIG_BLOCKING_IO ) ///< Maximum configuration bit field value.

} DataUpdateEnum;

/*!
@enum EncodingEnum
@brief Define the TrickHLA data encoding type.
*/
typedef enum {

   ENCODING_FIRST_VALUE    = 0, ///< Set to the First value in the enumeration.
   ENCODING_UNKNOWN        = 0, ///< Default encoding. The software automatically determines it for you. Otherwise, specify to one of the below values.
   ENCODING_BIG_ENDIAN     = 1, ///< Big Endian.
   ENCODING_LITTLE_ENDIAN  = 2, ///< Little Endian.
   ENCODING_LOGICAL_TIME   = 3, ///< 64-bit Big Endian encoded integer representing microseconds.
   ENCODING_C_STRING       = 4, ///< Null terminated C string (i.e. char *).
   ENCODING_UNICODE_STRING = 5, ///< Variable length HLA Unicode string encoding.
   ENCODING_ASCII_STRING   = 6, ///< Variable length HLA ASCII string encoding.
   ENCODING_OPAQUE_DATA    = 7, ///< Variable length HLA Opaque data for a "char *" type.
   ENCODING_BOOLEAN        = 8, ///< Boolean c++ type configured in the FOM to use HLAboolean HLA data type encoded as an HLAinteger32BE.
   ENCODING_NONE           = 9, ///< Fixed length array of data for "char *" type sent as is.
   ENCODING_LAST_VALUE     = 9  ///< Set to the Last value in the enumeration.

} EncodingEnum;

/*!
@enum TransportationEnum
@brief Define the TrickHLA data transportation type.
*/
typedef enum {

   TRANSPORT_FIRST_VALUE      = 0, ///< Set to the First value in the enumeration.
   TRANSPORT_SPECIFIED_IN_FOM = 0, ///< Indicates which attributes or interactions use the order specified in the FOM.
   TRANSPORT_TIMESTAMP_ORDER  = 1, ///< Indicates which attributes or interactions are Timestamp Order
   TRANSPORT_RECEIVE_ORDER    = 2, ///< Indicates which attributes or interactions are Receive Order
   TRANSPORT_LAST_VALUE       = 3  ///< Set to the Last value in the enumeration.

} TransportationEnum;

/*!
@enum LagCompensationEnum
@brief Define the TrickHLA latency (lag) compensation type.
*/
typedef enum {

   LAG_COMPENSATION_FIRST_VALUE  = 0, ///< Set to the First value in the enumeration.
   LAG_COMPENSATION_NONE         = 0, ///< No lag compensation.
   LAG_COMPENSATION_SEND_SIDE    = 1, ///< Send-side lag compensation.
   LAG_COMPENSATION_RECEIVE_SIDE = 2, ///< Receive-side lag compensation.
   LAG_COMPENSATION_LAST_VALUE   = 2  ///< Set to the Last value in the enumeration.

} LagCompensationEnum;

/*!
@enum DebugLevelEnum
@brief Define the TrickHLA level for debug messages.
*/
typedef enum {

   // NOTE: As the debug levels increase in numeric value, so does the amount of
   // output printed to console...
   DEBUG_LEVEL_NO_TRACE   = 0,  ///< Default: No TrickHLA output is displayed; user messages will still be printed.
   DEBUG_LEVEL_0_TRACE    = 0,  ///< Default: No TrickHLA output is displayed; user messages will still be printed.
   DEBUG_LEVEL_1_TRACE    = 1,  ///< Adds initialization complete and Time Advance Grant messages.
   DEBUG_LEVEL_2_TRACE    = 2,  ///< Adds initialization messages as well as the standard complement of execution messages.
   DEBUG_LEVEL_3_TRACE    = 3,  ///< Adds Ownership Transfer messages.
   DEBUG_LEVEL_4_TRACE    = 4,  ///< Adds HLA Time Advancement, Freeze job, and additional Shutdown job messages.
   DEBUG_LEVEL_5_TRACE    = 5,  ///< Adds additional HLA Time Advancement, Interaction, InitSyncPts and SyncPts messages.
   DEBUG_LEVEL_6_TRACE    = 6,  ///< Adds Packing/LagCompensation subclass messages.
   DEBUG_LEVEL_7_TRACE    = 7,  ///< Adds the names of all Attributes/Parameters sent to other federates.
   DEBUG_LEVEL_8_TRACE    = 8,  ///< Adds FederateAmbassador and RTI callback messages.
   DEBUG_LEVEL_9_TRACE    = 9,  ///< Adds Trick Ref-Attributes and RTI Handles (both during initialization).
   DEBUG_LEVEL_10_TRACE   = 10, ///< Adds internal state of all Attributes and Parameters.
   DEBUG_LEVEL_11_TRACE   = 11, ///< Adds buffer contents of all Attributes and Parameters.
   DEBUG_LEVEL_FULL_TRACE = 11  ///< Outputs All debug messages.

} DebugLevelEnum;

/*!
@enum DebugSourceEnum
@brief Define the TrickHLA source for debug messages.
*/
typedef enum {

   DEBUG_SOURCE_NO_MODULES         = 0x00000000, ///< No code selected for debug messages.
   DEBUG_SOURCE_FED_AMB            = 0x00000001, ///< Adds TrickHLA::FedAmb debug messages
   DEBUG_SOURCE_FEDERATE           = 0x00000002, ///< Adds TrickHLA::Federate debug messages
   DEBUG_SOURCE_MANAGER            = 0x00000004, ///< Adds TrickHLA::Manager debug messages
   DEBUG_SOURCE_OBJECT             = 0x00000008, ///< Adds TrickHLA::Object (and subclass) debug messages
   DEBUG_SOURCE_INTERACTION        = 0x00000010, ///< Adds TrickHLA::Interaction (and subclass) debug messages
   DEBUG_SOURCE_ATTRIBUTE          = 0x00000020, ///< Adds TrickHLA::Attribute debug messages
   DEBUG_SOURCE_PARAMETER          = 0x00000040, ///< Adds TrickHLA::Parameter debug messages
   DEBUG_SOURCE_SYNCPOINT          = 0x00000080, ///< Adds TrickHLA::SyncPoint debug messages
   DEBUG_SOURCE_OWNERSHIP          = 0x00000100, ///< Adds TrickHLA::OwnershipHandler debug messages
   DEBUG_SOURCE_PACKING            = 0x00000200, ///< Adds TrickHLA::Packing (and subclass) debug messages
   DEBUG_SOURCE_LAG_COMPENSATION   = 0x00000400, ///< Adds TrickHLA::LagCompensation (and subclass) debug messages
   DEBUG_SOURCE_EXECUTION_CONTROL  = 0x00000800, ///< Adds TrickHLA::ExecutionControl (and subclass) debug messages
   DEBUG_SOURCE_EXECUTION_CONFIG   = 0x00001000, ///< Adds TrickHLA::ExecutionConfiguration (and subclass) debug messages
   DEBUG_SOURCE_THREAD_COORDINATOR = 0x00002000, ///< Adds TrickHLA::TrickThreadCoordinator (and subclass) debug messages
   DEBUG_SOURCE_ALL_MODULES        = 0x7FFFFFFF  ///< Default: Add debug messages from all code modules

} DebugSourceEnum;

/*!
@enum FederateJoinEnum
@brief Define the TrickHLA federate join enumeration values.
*/
typedef enum {

   FEDERATE_JOIN_FIRST_VALUE = 0, ///< Set to the First value in the enumeration.
   FEDERATE_JOIN_NOMINAL     = 0, ///< Normal Federate Execution (neither late joiner nor federate restore).
   FEDERATE_JOIN_EARLY       = 0, ///< Early joining Federate.
   FEDERATE_JOIN_LATE        = 1, ///< Late Joining Federate
   FEDERATE_JOIN_RESTORING   = 2, ///< Federate Restore
   FEDERATE_JOIN_UNKNOWN     = 3, ///< Unknown Federate state
   FEDERATE_JOIN_LAST_VALUE  = 3  ///< Set to the Last value in the enumeration.

} FederateJoinEnum;

/*!
@emum InteractionTypeEnum
@brief The following enum is used to distinguish between user-defined and
built-in interactions. This is needed to distinguish between the interactions
defined in the user's model CODE and the interaction(s) built into TrickHLA.

Update this enum when adding more built-in interactions into TrickHLA.
*/
typedef enum {

   INTERACTION_TYPE_UNDEFINED      = 0, ///< Undefined interaction type.
   INTERACTION_TYPE_USER_DEFINED   = 1, ///< Interaction must be defined by the user in the input file.
   INTERACTION_TYPE_BUILTIN_FREEZE = 2, ///< Freeze Interaction internal to TrickHLA.
   INTERACTION_TYPE_BUILTIN_MTR    = 3  ///< MTR Interaction internal to TrickHLA.

} InteractionTypeEnum;

/*!
@enum ExecutionControlEnum
@brief Define the TrickHLA execution control enumeration values.
*/
typedef enum {

   EXECUTION_CONTROL_FIRST_VALUE   = 0, ///< Set to the First value in the enumeration.
   EXECUTION_CONTROL_UNINITIALIZED = 0, ///< Execution control state is uninitialized.
   EXECUTION_CONTROL_INITIALIZING  = 1, ///< Execution control state is initializing.
   EXECUTION_CONTROL_RUNNING       = 2, ///< Execution control state is running.
   EXECUTION_CONTROL_FREEZE        = 3, ///< Execution control state is freeze.
   EXECUTION_CONTROL_RESTART       = 4, ///< Execution control state is restart.
   EXECUTION_CONTROL_RECONFIG      = 5, ///< Execution control state is reconfigure.
   EXECUTION_CONTROL_SHUTDOWN      = 6, ///< Execution control state is shutdown.
   EXECUTION_CONTROL_LAST_VALUE    = 6  ///< Set to the Last value in the enumeration.

} ExecutionControlEnum;

/*!
@enum ModeTransitionEnum
@brief Define the TrickHLA Mode Transition state enumeration values.

The TrickHLA::ModeTransitionEnum enumeration defines the possible mode
transition for the TrickHLA::ExecutionControl executive. These mode requests
are important in the execution control process involving mode requests from
any federate participating in a controlled federation execution and usually
processed by the Master federate.
*/
// Define the Mode Transition enumeration values.
typedef enum {

   MODE_TRANSITION_FIRST_VALUE   = 0, ///< Not a valid mode transition.
   MODE_TRANSITION_UNINITIALIZED = 0, ///< Not a valid mode transition.
   MODE_TRANSITION_INITIALIZING  = 1, ///< Not a valid mode transition.
   MODE_TRANSITION_GOTO_RUN      = 2, ///< Mode transition to RUN mode.
   MODE_TRANSITION_GOTO_FREEZE   = 3, ///< Mode transition to FREEZE mode.
   MODE_TRANSITION_GOTO_RESTART  = 4, ///< Mode transition to RESTART mode.
   MODE_TRANSITION_GOTO_RECONFIG = 5, ///< Mode transition to RECONFIG mode.
   MODE_TRANSITION_GOTO_SHUTDOWN = 6, ///< Mode transition to SHUTDOWN mode.
   MODE_TRANSITION_LAST_VALUE    = 6  ///< Same as shutdown.

} ModeTransitionEnum;

/*!
@enum SyncPtStateEnum
@brief Define the TrickHLA synchronization point state enumeration values.

The SyncPtStateEnum enumeration defines the possible synchronization
point (sync-point) synchronization states for a TrickHLA based federate.
These sync-point states correspond directly to the sync-point states in HLA.
*/
typedef enum {

   SYNC_PT_STATE_FIRST_VALUE  = 0, ///< Set to the First value in the enumeration.
   SYNC_PT_STATE_ERROR        = 0, ///< Sync-point error.
   SYNC_PT_STATE_KNOWN        = 1, ///< Sync-point is known.
   SYNC_PT_STATE_REGISTERED   = 2, ///< Sync-point registered.
   SYNC_PT_STATE_ANNOUNCED    = 3, ///< Sync-point announced.
   SYNC_PT_STATE_ACHIEVED     = 4, ///< Sync-point achieved.
   SYNC_PT_STATE_SYNCHRONIZED = 5, ///< Sync-point synchronized.
   SYNC_PT_STATE_UNKNOWN      = 6, ///< Unknown state.
   SYNC_PT_STATE_LAST_VALUE   = 6  ///< Set to the Last value in the enumeration.

} SyncPtStateEnum;

/*!
@enum ThreadStateEnum
@brief Define the Trick child thread state for TrickHLA jobs.
*/
typedef enum {

   THREAD_STATE_DISABLED         = 0, ///< Thread will be disabled from being associated to TrickHLA.
   THREAD_STATE_NOT_ASSOCIATED   = 1, ///< Thread is not used (i.e. associated) to TrickHLA jobs.
   THREAD_STATE_RESET            = 2, ///< Thread used by TrickHLA jobs but state not set yet.
   THREAD_STATE_READY_TO_SEND    = 3, ///< Thread used by TrickHLA jobs ready to send data.
   THREAD_STATE_READY_TO_RECEIVE = 4  ///< Thread used by TrickHLA jobs ready to receive data.

} ThreadStateEnum;

/*!
@enum TimeAdvanceStateEnum
@brief Define the Trick child thread Time Advance Request (TAR) state.
*/
typedef enum {

   TIME_ADVANCE_RESET     = 0, ///< Reset state, before time advance request.
   TIME_ADVANCE_REQUESTED = 1, ///< Time Advance Requested.
   TIME_ADVANCE_GRANTED   = 2  ///< Time Advance Granted.

} TimeAdvanceStateEnum;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
typedef std::auto_ptr< RTI1516_NAMESPACE::RTIambassador > TrickRTIAmbPtr;
#pragma GCC diagnostic pop

typedef std::queue< RTI1516_NAMESPACE::AttributeHandleValueMap > HLAAttributeMapQueue;

typedef std::map< RTI1516_NAMESPACE::ObjectInstanceHandle, std::wstring > TrickHLAObjInstanceNameMap;

typedef std::map< std::string, unsigned int > TrickHLAObjInstanceNameIndexMap;

typedef std::vector< std::string > VectorOfStrings;

typedef std::vector< std::wstring > VectorOfWstrings;

//
// Helper methods for these enumerations.
//
// TrickHLA::ExecutionModeEnum methods.
/*! @brief Convert an ExecutionModeEnum value into a printable string.
 *  @return Execution control mode as a printable string.
 *  @param mode Execution configuration run mode enumeration value. */
std::string execution_control_enum_to_string( ExecutionControlEnum const mode );

/*! @brief Convert an ExecutionModeEnum value into a 16 bit integer.
 *  @return Execution control mode as a 16 bit integer representation.
 *  @param mode Execution control mode enumeration value. */
int16_t execution_control_enum_to_int16( ExecutionControlEnum const mode );

/*! @brief Convert a 16 bit integer to an ExecutionModeEnum value.
 *  @return Execution control mode as enumeration value.
 *  @param int_mode Execution control mode as integer. */
ExecutionControlEnum execution_control_int16_to_enum( int16_t const int_mode );

// TrickHLA::ModeTransitionEnum methods.
/*! @brief Convert an ModeTransitionEnum value into a printable string.
 *  @return Mode transition as a printable string.
 *  @param mode Mode transition enumeration value. */
std::string mode_transition_enum_to_string( ModeTransitionEnum const mode );

/*! @brief Convert an ModeTransitionEnum value into a 16 bit integer.
 *  @return Mode transition as a 16 bit integer representation.
 *  @param mode Mode transition enumeration value. */
int16_t mode_transition_enum_to_int16( ModeTransitionEnum const mode );

/*! @brief Convert a 16 bit integer to an ModeTransitionEnum value.
 *  @return Mode transition as enumeration value.
 *  @param int_mode Mode transition as integer. */
ModeTransitionEnum mode_transition_int16_to_enum( int16_t const int_mode );

// TrickHLA::SyncPtStateEnum methods
/*! @brief Convert a Synchronization Point State enum value into a printable string.
 *  @return TrickHLA sync point state as a printable string.
 *  @param state Sync point state enumeration value to convert. */
std::string sync_point_state_enum_to_string( SyncPtStateEnum const state );

/*! @brief Convert a Synchronization Point State enum value into a 16 bit integer.
 *  @return TrickHLA sync point state as a 16 bit integer.
 *  @param state Sync point state enumeration value to convert. */
int16_t sync_point_state_enum_to_int16( SyncPtStateEnum const state );

/*! @brief Convert an integer value to a Synchronization Point State enumeration value.
 *  @return TrickHLA Synchronization Point State enum value.
 *  @param int_state Sync point state value as a 16 bit integer. */
SyncPtStateEnum sync_point_state_int16_to_enum( int16_t const int_state );

} // namespace TrickHLA

#endif // TRICKHLA_TYPES_HH
