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
#include <limits>
#include <stdint.h>
#include <string>

// TrickHLA include files.
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;

string TrickHLA::execution_control_enum_to_string(
   ExecutionControlEnum mode )
{
   switch ( mode ) {

      case EXECUTION_CONTROL_UNINITIALIZED:
         return "EXECUTION_CONTROL_UNINITIALIZED";
         break;

      case EXECUTION_CONTROL_INITIALIZING:
         return "EXECUTION_CONTROL_INITIALIZING";
         break;

      case EXECUTION_CONTROL_RUNNING:
         return "EXECUTION_CONTROL_RUNNING";
         break;

      case EXECUTION_CONTROL_FREEZE:
         return "EXECUTION_CONTROL_FREEZE";
         break;

      case EXECUTION_CONTROL_RESTART:
         return "EXECUTION_CONTROL_RESTART";
         break;

      case EXECUTION_CONTROL_RECONFIG:
         return "EXECUTION_CONTROL_RECONFIG";
         break;

      case EXECUTION_CONTROL_SHUTDOWN:
         return "EXECUTION_CONTROL_SHUTDOWN";
         break;

      default:
         return "UNKNOWN TrickHLA Execution Mode";
         break;
   }

   return "UNKNOWN TrickHLA Execution Mode";
}

int16_t TrickHLA::execution_control_enum_to_int16(
   ExecutionControlEnum mode )
{

   return (int16_t)mode;
}

ExecutionControlEnum TrickHLA::execution_control_int16_to_enum(
   int16_t int_mode )
{
   switch ( int_mode ) {

      case 0:
         return EXECUTION_CONTROL_UNINITIALIZED;
         break;

      case 1:
         return EXECUTION_CONTROL_INITIALIZING;
         break;

      case 2:
         return EXECUTION_CONTROL_RUNNING;
         break;

      case 3:
         return EXECUTION_CONTROL_FREEZE;
         break;

      case 4:
         return EXECUTION_CONTROL_RESTART;
         break;

      case 5:
         return EXECUTION_CONTROL_RECONFIG;
         break;

      case 6:
         return EXECUTION_CONTROL_SHUTDOWN;
         break;

      default:
         return EXECUTION_CONTROL_UNINITIALIZED;
         break;
   }

   return EXECUTION_CONTROL_UNINITIALIZED;
}

string TrickHLA::mode_transition_enum_to_string(
   ModeTransitionEnum mode )
{
   switch ( mode ) {

      case MODE_TRANSITION_UNINITIALIZED:
         return "MODE_TRANSITION_UNINITIALIZED";
         break;

      case MODE_TRANSITION_INITIALIZING:
         return "MODE_TRANSITION_INITIALIZING";
         break;

      case MODE_TRANSITION_GOTO_RUN:
         return "MODE_TRANSITION_GOTO_RUN";
         break;

      case MODE_TRANSITION_GOTO_FREEZE:
         return "MODE_TRANSITION_GOTO_FREEZE";
         break;

      case MODE_TRANSITION_GOTO_RESTART:
         return "MODE_TRANSITION_GOTO_RESTART";
         break;

      case MODE_TRANSITION_GOTO_RECONFIG:
         return "MODE_TRANSITION_GOTO_RECONFIG";
         break;

      case MODE_TRANSITION_GOTO_SHUTDOWN:
         return "MODE_TRANSITION_GOTO_SHUTDOWN";
         break;

      default:
         return "UNKNOWN TrickHLA Mode Transition";
         break;
   }

   return "UNKNOWN TrickHLA Mode Transition";
}

int16_t TrickHLA::mode_transition_enum_to_int16(
   ModeTransitionEnum mode )
{

   return (int16_t)mode;
}

ModeTransitionEnum TrickHLA::mode_transition_int16_to_enum(
   int16_t int_mode )
{
   switch ( int_mode ) {

      case 0:
         return MODE_TRANSITION_UNINITIALIZED;
         break;

      case 1:
         return MODE_TRANSITION_INITIALIZING;
         break;

      case 2:
         return MODE_TRANSITION_GOTO_RUN;
         break;

      case 3:
         return MODE_TRANSITION_GOTO_FREEZE;
         break;

      case 4:
         return MODE_TRANSITION_GOTO_RESTART;
         break;

      case 5:
         return MODE_TRANSITION_GOTO_RECONFIG;
         break;

      case 6:
         return MODE_TRANSITION_GOTO_SHUTDOWN;
         break;

      default:
         return MODE_TRANSITION_UNINITIALIZED;
         break;
   }

   return MODE_TRANSITION_UNINITIALIZED;
}

string TrickHLA::sync_pnt_state_enum_to_string(
   SyncPtStateEnum state )
{

   switch ( state ) {

      case SYNC_PT_STATE_ERROR:
         return "SYNC_PT_STATE_ERROR";
         break;

      case SYNC_PT_STATE_EXISTS:
         return "SYNC_PT_STATE_EXISTS";
         break;

      case SYNC_PT_STATE_REGISTERED:
         return "SYNC_PT_STATE_REGISTERED";
         break;

      case SYNC_PT_STATE_ANNOUNCED:
         return "SYNC_PT_STATE_ANNOUNCED";
         break;

      case SYNC_PT_STATE_ACHIEVED:
         return "SYNC_PT_STATE_ACHIEVED";
         break;

      case SYNC_PT_STATE_SYNCHRONIZED:
         return "SYNC_PT_STATE_SYNCHRONIZED";
         break;

      default:
         return "SYNC_PT_STATE_UNKNOWN";
         break;
   }

   return "SYNC_PT_STATE_UNKNOWN";
}

int16_t TrickHLA::sync_pnt_state_enum_to_int16(
   SyncPtStateEnum state )
{
   return (int16_t)state;
}

SyncPtStateEnum TrickHLA::sync_pnt_state_int16_to_enum(
   int16_t int_state )
{

   switch ( int_state ) {

      case 0:
         return SYNC_PT_STATE_ERROR;
         break;

      case 1:
         return SYNC_PT_STATE_EXISTS;
         break;

      case 2:
         return SYNC_PT_STATE_REGISTERED;
         break;

      case 3:
         return SYNC_PT_STATE_ANNOUNCED;
         break;

      case 4:
         return SYNC_PT_STATE_ACHIEVED;
         break;

      case 5:
         return SYNC_PT_STATE_SYNCHRONIZED;
         break;

      default:
         return SYNC_PT_STATE_UNKNOWN;
         break;
   }

   return SYNC_PT_STATE_UNKNOWN;
}
