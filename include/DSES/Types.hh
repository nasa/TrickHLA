/*!
@file DSES/Types.hh
@ingroup DSES
@brief Definition of the TrickHLA DSES enumeration types and utilities.

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
@trick_link_dependency{../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../source/DSES/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _DSES_TYPES_HH_
#define _DSES_TYPES_HH_

// System includes.
#include <stdint.h>
#include <string>

// TrickHLA include.
#include "TrickHLA/Types.hh"

namespace DSES
{

/*!
@enum ExecutionModeEnum
@brief Define the TrickHLA DSES execution mode enumeration values.

The ExecutionModeEnum enumeration defines the possible execution mode
state for a DSES compliant federate.  These mode states
are important in the execution control process commanded by a Master
federate and followed all other federates participating in a DSES
compliant federation execution.
*/
typedef enum {

   EXECUTION_MODE_FIRST_VALUE   = 0, ///< Same as uninitialized.
   EXECUTION_MODE_UNINITIALIZED = 0, ///< Execution mode UNINITIALIZEDD.
   EXECUTION_MODE_INITIALIZING  = 1, ///< Execution mode INITIALIZING.
   EXECUTION_MODE_RUNNING       = 2, ///< Execution mode RUNNING.
   EXECUTION_MODE_FREEZE        = 3, ///< Execution mode FREEZE.
   EXECUTION_MODE_SHUTDOWN      = 4, ///< Execution mode SHUTDOWN.
   EXECUITON_MODE_LAST_VALUE    = 4  ///< Same as shutdown.

} ExecutionModeEnum;

/*!
@enum MTREnum
@brief Define the TrickHLA DSES Mode Transition Request state enumeration values.

The MTREnum enumeration defines the possible mode transition requests (MTRs)
for a Space Reference FOM compliant federate.  These mode requests are
important in the execution control process involving mode requests from
any federate participating in a Space FOM compliant federation execution
and processed by the Master federate.
*/
// Define the Mode Transition Request state enumeration values.
typedef enum {

   MTR_FIRST_VALUE   = 0, ///< Not a valid mode transition.
   MTR_UNINITIALIZED = 0, ///< Not a valid mode transition.
   MTR_INITIALIZING  = 1, ///< Not a valid mode transition.
   MTR_GOTO_RUN      = 2, ///< Mode transition to RUN mode.
   MTR_GOTO_FREEZE   = 3, ///< Mode transition to FREEZE mode.
   MTR_GOTO_SHUTDOWN = 4, ///< Mode transition to SHUTDOWN mode.
   MTR_LAST_VALUE    = 4  ///< Same as shutdown.

} MTREnum;

// Helper methods for these enumerations.
/*! @brief Convert an ExecutionModeEnum value into a printable string.
 *  @return DSES execution mode as a printable string.
 *  @param mode Execution configuration run mode enumeration value. */
std::string execution_mode_enum_to_string( ExecutionModeEnum mode );
/*! @brief Convert an ExecutionModeEnum value into a 16 bit integer.
 *  @return DSES execution mode as a 16 bit integer representation.
 *  @param mode Execution configuration run mode enumeration value. */
int16_t execution_mode_enum_to_int16( ExecutionModeEnum mode );
/*! @brief Convert a 16 bit integer to an ExecutionModeEnum value.
 *  @return DSES execution mode as enumeration value.
 *  @param int_mode Execution configuration run mode as integer. */
ExecutionModeEnum execution_mode_int16_to_enum( int16_t int_mode );
/*! @brief Convert an DSES::ExecutionModeEnum value to a TrickHLA::ExecutionModeEnum value.
 *  @return DSES::ExecutionModeEnum as an equivalent TrickHLA::ExecutionModeEnum value.
 *  @param mode DSES::ExecutionModeEnum value to convert. */
TrickHLA::ExecutionControlEnum to_execution_control_enum( ExecutionModeEnum mode );
/*! @brief Convert a TrickHLA::ExecutionModeEnum value to an DSES::ExecutionModeEnum value.
 *  @return TrickHLA::ExecutionModeEnum as an equivalent DSES::ExecutionModeEnum value.
 *  @param mode TrickHLA::ExecutionModeEnum value to convert. */
ExecutionModeEnum from_execution_contorl_enum( TrickHLA::ExecutionControlEnum mode );

// Helper methods for these enumerations.
/*! @brief Convert a Mode Transition Request (MTR) enum value into a printable string.
 *  @return DSES mode transition request (MTR) as a printable string.
 *  @param mtr_enum MTR enumeration value to convert. */
std::string mtr_enum_to_string( MTREnum mtr_enum );
/*! @brief Convert a Mode Transition Request (MTR) enum value into a 16 bit integer.
 *  @return DSES Mode Transition Request (MTR) as a 16 bit integer.
 *  @param mtr_enum MTR enumeration values. */
int16_t mtr_enum_to_int16( MTREnum mtr_enum );
/*! @brief Convert a 16 bit integer into a Mode Transition Request (MTR) enum value.
 *  @return DSES mode transition request (MTR) as enumeration value.
 *  @param mtr_int MTR mode. */
MTREnum mtr_int16_to_enum( int16_t mtr_int );

} // namespace DSES

#endif // _DSES_TYPES_HH_: Do NOT put anything after this line!
