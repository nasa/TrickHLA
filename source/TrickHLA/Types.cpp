/*!
@file TrickHLA/Types.cpp
@ingroup TrickHLA
@brief Implementation of the TrickHLA types utility functions.

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
#include <cstdint>
#include <limits>
#include <string>

// TrickHLA include files.
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;

string TrickHLA::encoding_enum_to_string(
   EncodingEnum const encoding )
{
   switch ( encoding ) {
      case ENCODING_BIG_ENDIAN: {
         return "ENCODING_BIG_ENDIAN";
      }
      case ENCODING_LITTLE_ENDIAN: {
         return "ENCODING_LITTLE_ENDIAN";
      }
      case ENCODING_LOGICAL_TIME: {
         return "ENCODING_LOGICAL_TIME";
      }
      case ENCODING_C_STRING: {
         return "ENCODING_C_STRING";
      }
      case ENCODING_UNICODE_STRING: {
         return "ENCODING_UNICODE_STRING";
      }
      case ENCODING_ASCII_STRING: {
         return "ENCODING_ASCII_STRING";
      }
      case ENCODING_OPAQUE_DATA: {
         return "ENCODING_OPAQUE_DATA";
      }
      case ENCODING_BOOLEAN: {
         return "ENCODING_BOOLEAN";
      }
      case ENCODING_NONE: {
         return "ENCODING_NONE";
      }
      case ENCODING_UNICODE_CHAR: {
         return "ENCODING_UNICODE_CHAR";
      }
      case ENCODING_ASCII_CHAR: {
         return "ENCODING_ASCII_CHAR";
      }
      default: {
         return "UNKNOWN TrickHLA Encoding";
      }
   }
   return "UNKNOWN TrickHLA Encoding";
}

string TrickHLA::execution_control_enum_to_string(
   ExecutionControlEnum const mode )
{
   switch ( mode ) {
      case EXECUTION_CONTROL_UNINITIALIZED: {
         return "EXECUTION_CONTROL_UNINITIALIZED";
      }
      case EXECUTION_CONTROL_INITIALIZING: {
         return "EXECUTION_CONTROL_INITIALIZING";
      }
      case EXECUTION_CONTROL_RUNNING: {
         return "EXECUTION_CONTROL_RUNNING";
      }
      case EXECUTION_CONTROL_FREEZE: {
         return "EXECUTION_CONTROL_FREEZE";
      }
      case EXECUTION_CONTROL_RESTART: {
         return "EXECUTION_CONTROL_RESTART";
      }
      case EXECUTION_CONTROL_RECONFIG: {
         return "EXECUTION_CONTROL_RECONFIG";
      }
      case EXECUTION_CONTROL_SHUTDOWN: {
         return "EXECUTION_CONTROL_SHUTDOWN";
      }
      default: {
         return "UNKNOWN TrickHLA Execution Mode";
      }
   }
   return "UNKNOWN TrickHLA Execution Mode";
}

int16_t TrickHLA::execution_control_enum_to_int16(
   ExecutionControlEnum const mode )
{
   return (int16_t)mode;
}

ExecutionControlEnum TrickHLA::execution_control_int16_to_enum(
   int16_t const int_mode )
{
   switch ( int_mode ) {
      case 0: {
         return EXECUTION_CONTROL_UNINITIALIZED;
      }
      case 1: {
         return EXECUTION_CONTROL_INITIALIZING;
      }
      case 2: {
         return EXECUTION_CONTROL_RUNNING;
      }
      case 3: {
         return EXECUTION_CONTROL_FREEZE;
      }
      case 4: {
         return EXECUTION_CONTROL_RESTART;
      }
      case 5: {
         return EXECUTION_CONTROL_RECONFIG;
      }
      case 6: {
         return EXECUTION_CONTROL_SHUTDOWN;
      }
      default: {
         return EXECUTION_CONTROL_UNINITIALIZED;
      }
   }
   return EXECUTION_CONTROL_UNINITIALIZED;
}

string TrickHLA::mode_transition_enum_to_string(
   ModeTransitionEnum const mode )
{
   switch ( mode ) {
      case MODE_TRANSITION_UNINITIALIZED: {
         return "MODE_TRANSITION_UNINITIALIZED";
      }
      case MODE_TRANSITION_INITIALIZING: {
         return "MODE_TRANSITION_INITIALIZING";
      }
      case MODE_TRANSITION_GOTO_RUN: {
         return "MODE_TRANSITION_GOTO_RUN";
      }
      case MODE_TRANSITION_GOTO_FREEZE: {
         return "MODE_TRANSITION_GOTO_FREEZE";
      }
      case MODE_TRANSITION_GOTO_RESTART: {
         return "MODE_TRANSITION_GOTO_RESTART";
      }
      case MODE_TRANSITION_GOTO_RECONFIG: {
         return "MODE_TRANSITION_GOTO_RECONFIG";
      }
      case MODE_TRANSITION_GOTO_SHUTDOWN: {
         return "MODE_TRANSITION_GOTO_SHUTDOWN";
      }
      default: {
         return "UNKNOWN TrickHLA Mode Transition";
      }
   }
   return "UNKNOWN TrickHLA Mode Transition";
}

int16_t TrickHLA::mode_transition_enum_to_int16(
   ModeTransitionEnum const mode )
{
   return (int16_t)mode;
}

ModeTransitionEnum TrickHLA::mode_transition_int16_to_enum(
   int16_t const int_mode )
{
   switch ( int_mode ) {
      case 0: {
         return MODE_TRANSITION_UNINITIALIZED;
      }
      case 1: {
         return MODE_TRANSITION_INITIALIZING;
      }
      case 2: {
         return MODE_TRANSITION_GOTO_RUN;
      }
      case 3: {
         return MODE_TRANSITION_GOTO_FREEZE;
      }
      case 4: {
         return MODE_TRANSITION_GOTO_RESTART;
      }
      case 5: {
         return MODE_TRANSITION_GOTO_RECONFIG;
      }
      case 6: {
         return MODE_TRANSITION_GOTO_SHUTDOWN;
      }
      default: {
         return MODE_TRANSITION_UNINITIALIZED;
      }
   }
   return MODE_TRANSITION_UNINITIALIZED;
}

string TrickHLA::sync_point_state_enum_to_string(
   SyncPtStateEnum const state )
{
   switch ( state ) {
      case SYNC_PT_STATE_ERROR: {
         return "SYNC_PT_STATE_ERROR";
      }
      case SYNC_PT_STATE_KNOWN: {
         return "SYNC_PT_STATE_KNOWN";
      }
      case SYNC_PT_STATE_REGISTERED: {
         return "SYNC_PT_STATE_REGISTERED";
      }
      case SYNC_PT_STATE_ANNOUNCED: {
         return "SYNC_PT_STATE_ANNOUNCED";
      }
      case SYNC_PT_STATE_ACHIEVED: {
         return "SYNC_PT_STATE_ACHIEVED";
      }
      case SYNC_PT_STATE_SYNCHRONIZED: {
         return "SYNC_PT_STATE_SYNCHRONIZED";
      }
      default: {
         return "SYNC_PT_STATE_UNKNOWN";
      }
   }
   return "SYNC_PT_STATE_UNKNOWN";
}

int16_t TrickHLA::sync_point_state_enum_to_int16(
   SyncPtStateEnum const state )
{
   return (int16_t)state;
}

SyncPtStateEnum TrickHLA::sync_point_state_int16_to_enum(
   int16_t const int_state )
{
   switch ( int_state ) {
      case 0: {
         return SYNC_PT_STATE_ERROR;
      }
      case 1: {
         return SYNC_PT_STATE_KNOWN;
      }
      case 2: {
         return SYNC_PT_STATE_REGISTERED;
      }
      case 3: {
         return SYNC_PT_STATE_ANNOUNCED;
      }
      case 4: {
         return SYNC_PT_STATE_ACHIEVED;
      }
      case 5: {
         return SYNC_PT_STATE_SYNCHRONIZED;
      }
      default: {
         return SYNC_PT_STATE_UNKNOWN;
      }
   }
   return SYNC_PT_STATE_UNKNOWN;
}
