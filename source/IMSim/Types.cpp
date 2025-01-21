/*!
@file IMSim/Types.cpp
@ingroup IMSim
@brief Implementation of the TrickHLA IMSim types utility functions.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <string>

// IMSim include files.
#include "IMSim/Types.hh"

using namespace std;
using namespace IMSim;

string IMSim::execution_mode_enum_to_string(
   ExecutionModeEnum mode )
{
   switch ( mode ) {
      case EXECUTION_MODE_UNINITIALIZED: {
         return "EXEC_MODE_UNINITIALIZED";
      }
      case EXECUTION_MODE_INITIALIZING: {
         return "EXEC_MODE_INITIALIZING";
      }
      case EXECUTION_MODE_RUNNING: {
         return "EXEC_MODE_RUNNING";
      }
      case EXECUTION_MODE_FREEZE: {
         return "EXECUTION_MODE_FREEZE";
      }
      case EXECUTION_MODE_SHUTDOWN: {
         return "EXEC_MODE_SHUTDOWN";
      }
      default: {
         return "UNKNOWN IMSim Execution Mode";
      }
   }
   return "UNKNOWN IMSim Execution Mode";
}

int16_t IMSim::execution_mode_enum_to_int16(
   ExecutionModeEnum mode )
{
   return (int16_t)mode;
}

ExecutionModeEnum IMSim::execution_mode_int16_to_enum(
   int16_t int_mode )
{
   switch ( int_mode ) {
      case 0: {
         return EXECUTION_MODE_UNINITIALIZED;
      }
      case 1: {
         return EXECUTION_MODE_INITIALIZING;
      }
      case 2: {
         return EXECUTION_MODE_RUNNING;
      }
      case 3: {
         return EXECUTION_MODE_FREEZE;
      }
      case 4: {
         return EXECUTION_MODE_SHUTDOWN;
      }
      default: {
         return EXECUTION_MODE_UNINITIALIZED;
      }
   }
   return EXECUTION_MODE_UNINITIALIZED;
}

TrickHLA::ExecutionControlEnum IMSim::to_execution_control_enum(
   ExecutionModeEnum mode )
{
   switch ( mode ) {
      case EXECUTION_MODE_UNINITIALIZED: {
         return TrickHLA::EXECUTION_CONTROL_UNINITIALIZED;
      }
      case EXECUTION_MODE_INITIALIZING: {
         return TrickHLA::EXECUTION_CONTROL_INITIALIZING;
      }
      case EXECUTION_MODE_RUNNING: {
         return TrickHLA::EXECUTION_CONTROL_RUNNING;
      }
      case EXECUTION_MODE_FREEZE: {
         return TrickHLA::EXECUTION_CONTROL_FREEZE;
      }
      case EXECUTION_MODE_SHUTDOWN: {
         return TrickHLA::EXECUTION_CONTROL_SHUTDOWN;
      }
      default: {
         return TrickHLA::EXECUTION_CONTROL_UNINITIALIZED;
      }
   }
   return TrickHLA::EXECUTION_CONTROL_UNINITIALIZED;
}

ExecutionModeEnum IMSim::from_execution_contorl_enum(
   TrickHLA::ExecutionControlEnum mode )
{
   switch ( mode ) {
      case TrickHLA::EXECUTION_CONTROL_UNINITIALIZED: {
         return EXECUTION_MODE_UNINITIALIZED;
      }
      case TrickHLA::EXECUTION_CONTROL_INITIALIZING: {
         return EXECUTION_MODE_INITIALIZING;
      }
      case TrickHLA::EXECUTION_CONTROL_RUNNING: {
         return EXECUTION_MODE_RUNNING;
      }
      case TrickHLA::EXECUTION_CONTROL_FREEZE: {
         return EXECUTION_MODE_FREEZE;
      }
      case TrickHLA::EXECUTION_CONTROL_SHUTDOWN: {
         return EXECUTION_MODE_SHUTDOWN;
      }
      case TrickHLA::EXECUTION_CONTROL_RESTART: {
         return EXECUTION_MODE_UNINITIALIZED;
      }
      case TrickHLA::EXECUTION_CONTROL_RECONFIG: {
         return EXECUTION_MODE_UNINITIALIZED;
      }
      default: {
         return EXECUTION_MODE_UNINITIALIZED;
      }
   }
   return EXECUTION_MODE_UNINITIALIZED;
}

string IMSim::mtr_enum_to_string(
   MTREnum mtr_enum )
{
   switch ( mtr_enum ) {
      case MTR_UNINITIALIZED: {
         return "MTR_UNINITIALIZED";
      }
      case MTR_INITIALIZING: {
         return "MTR_INITIALIZING";
      }
      case MTR_GOTO_RUN: {
         return "MTR_GOTO_RUN";
      }
      case MTR_GOTO_FREEZE: {
         return "MTR_GOTO_FREEZE";
      }
      case MTR_GOTO_SHUTDOWN: {
         return "MTR_GOTO_SHUTDOWN";
      }
      default: {
         return "MTR_UNKNOWN";
      }
   }
   return "MTR_UNKNOWN";
}

int16_t IMSim::mtr_enum_to_int16(
   MTREnum mtr_enum )
{
   return (int16_t)mtr_enum;
}

MTREnum IMSim::mtr_int16_to_enum(
   int16_t mtr_int )
{
   switch ( mtr_int ) {
      case 0: {
         return MTR_UNINITIALIZED;
      }
      case 1: {
         return MTR_INITIALIZING;
      }
      case 2: {
         return MTR_GOTO_RUN;
      }
      case 3: {
         return MTR_GOTO_FREEZE;
      }
      case 4: {
         return MTR_GOTO_SHUTDOWN;
      }
      default: {
         return MTR_UNINITIALIZED;
      }
   }
   return MTR_UNINITIALIZED;
}
