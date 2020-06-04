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

// IMSim include files.
#include "IMSim/Types.hh"

using namespace std;
using namespace IMSim;

string IMSim::execution_mode_enum_to_string(
   ExecutionModeEnum mode )
{
   switch ( mode ) {

   case EXECUTION_MODE_UNINITIALIZED:
      return "EXEC_MODE_UNINITIALIZED";
      break;

   case EXECUTION_MODE_INITIALIZING:
      return "EXEC_MODE_INITIALIZING";
      break;

   case EXECUTION_MODE_RUNNING:
      return "EXEC_MODE_RUNNING";
      break;

   case EXECUTION_MODE_FREEZE:
      return "EXECUTION_MODE_FREEZE";
      break;

   case EXECUTION_MODE_SHUTDOWN:
      return "EXEC_MODE_SHUTDOWN";
      break;

   default:
      return "UNKNOWN IMSim Execution Mode";
      break;
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

   case 0:
      return EXECUTION_MODE_UNINITIALIZED;
      break;

   case 1:
      return EXECUTION_MODE_INITIALIZING;
      break;

   case 2:
      return EXECUTION_MODE_RUNNING;
      break;

   case 3:
      return EXECUTION_MODE_FREEZE;
      break;

   case 4:
      return EXECUTION_MODE_SHUTDOWN;
      break;

   default:
      return EXECUTION_MODE_UNINITIALIZED;
      break;
   }

   return EXECUTION_MODE_UNINITIALIZED;
}

TrickHLA::ExecutionControlEnum IMSim::to_execution_control_enum(
   ExecutionModeEnum mode )
{
   switch ( mode ) {

   case EXECUTION_MODE_UNINITIALIZED:
      return TrickHLA::EXECUTION_CONTROL_UNINITIALIZED;
      break;

   case EXECUTION_MODE_INITIALIZING:
      return TrickHLA::EXECUTION_CONTROL_INITIALIZING;
      break;

   case EXECUTION_MODE_RUNNING:
      return TrickHLA::EXECUTION_CONTROL_RUNNING;
      break;

   case EXECUTION_MODE_FREEZE:
      return TrickHLA::EXECUTION_CONTROL_FREEZE;
      break;

   case EXECUTION_MODE_SHUTDOWN:
      return TrickHLA::EXECUTION_CONTROL_SHUTDOWN;
      break;

   default:
      return TrickHLA::EXECUTION_CONTROL_UNINITIALIZED;
      break;
   }

   return TrickHLA::EXECUTION_CONTROL_UNINITIALIZED;
}

ExecutionModeEnum IMSim::from_execution_contorl_enum(
   TrickHLA::ExecutionControlEnum mode )
{
   switch ( mode ) {

   case TrickHLA::EXECUTION_CONTROL_UNINITIALIZED:
      return EXECUTION_MODE_UNINITIALIZED;
      break;

   case TrickHLA::EXECUTION_CONTROL_INITIALIZING:
      return EXECUTION_MODE_INITIALIZING;
      break;

   case TrickHLA::EXECUTION_CONTROL_RUNNING:
      return EXECUTION_MODE_RUNNING;
      break;

   case TrickHLA::EXECUTION_CONTROL_FREEZE:
      return EXECUTION_MODE_FREEZE;
      break;

   case TrickHLA::EXECUTION_CONTROL_SHUTDOWN:
      return EXECUTION_MODE_SHUTDOWN;
      break;

   case TrickHLA::EXECUTION_CONTROL_RESTART:
      return EXECUTION_MODE_UNINITIALIZED;
      break;

   case TrickHLA::EXECUTION_CONTROL_RECONFIG:
      return EXECUTION_MODE_UNINITIALIZED;
      break;

   default:
      return EXECUTION_MODE_UNINITIALIZED;
      break;
   }

   return EXECUTION_MODE_UNINITIALIZED;
}

string IMSim::mtr_enum_to_string(
   MTREnum mtr_enum )
{

   switch ( mtr_enum ) {

   case MTR_UNINITIALIZED:
      return "MTR_UNINITIALIZED";
      break;

   case MTR_INITIALIZING:
      return "MTR_INITIALIZING";
      break;

   case MTR_GOTO_RUN:
      return "MTR_GOTO_RUN";
      break;

   case MTR_GOTO_FREEZE:
      return "MTR_GOTO_FREEZE";
      break;

   case MTR_GOTO_SHUTDOWN:
      return "MTR_GOTO_SHUTDOWN";
      break;

   default:
      return "MTR_UNKNOWN";
      break;
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

   case 0:
      return MTR_UNINITIALIZED;
      break;

   case 1:
      return MTR_INITIALIZING;
      break;

   case 2:
      return MTR_GOTO_RUN;
      break;

   case 3:
      return MTR_GOTO_FREEZE;
      break;

   case 4:
      return MTR_GOTO_SHUTDOWN;
      break;

   default:
      return MTR_UNINITIALIZED;
      break;
   }

   return MTR_UNINITIALIZED;
}

string IMSim::pause_pnt_state_enum_to_string(
   PausePointStateEnum state )
{

   switch ( state ) {

   case PAUSE_POINT_STATE_ERROR:
      return "PAUSE_POINT_STATE_ERROR";
      break;

   case PAUSE_POINT_STATE_PENDING:
      return "PAUSE_POINT_STATE_PENDING";
      break;

   case PAUSE_POINT_STATE_ACKNOWLEDGED:
      return "PAUSE_POINT_STATE_ACKNOWLEDGED";
      break;

   case PAUSE_POINT_STATE_RUN:
      return "PAUSE_POINT_STATE_RUN";
      break;

   case PAUSE_POINT_STATE_EXIT:
      return "SYNC_PNT_STATE_ACHIEVED";
      break;

   case PAUSE_POINT_STATE_RESTART:
      return "PAUSE_POINT_STATE_RESTART";
      break;

   case PAUSE_POINT_STATE_RECONFIG:
      return "PAUSE_POINT_STATE_RECONFIG";
      break;

   case PAUSE_POINT_STATE_EXIT:
      return "SYNC_PNT_STATE_SYNCHRONIZED";
      break;

   default:
      return "PAUSE_POINT_STATE_UNKNOWN";
      break;
   }

   return "PAUSE_POINT_STATE_UNKNOWN";
}

int16_t IMSim::pause_pnt_state_enum_to_int16(
   PausePointStateEnum state )
{
   return (int16_t)state;
}

PausePointStateEnum IMSim::pause_pnt_state_int16_to_enum(
   int16_t int_state )
{

   switch ( int_state ) {

   case 0:
      return PAUSE_POINT_STATE_ERROR;
      break;

   case 1:
      return PAUSE_POINT_STATE_PENDING;
      break;

   case 2:
      return PAUSE_POINT_STATE_ACKNOWLEDGED;
      break;

   case 3:
      return PAUSE_POINT_STATE_RUN;
      break;

   case 4:
      return PAUSE_POINT_STATE_FREEZE;
      break;

   case 5:
      return PAUSE_POINT_STATE_EXIT;
      break;

   case 6:
      return PAUSE_POINT_STATE_RESTART;
      break;

   case 7:
      return PAUSE_POINT_STATE_RECONFIG;
      break;

   default:
      return PAUSE_POINT_STATE_UNKNOWN;
      break;
   }

   return PAUSE_POINT_STATE_UNKNOWN;
}
