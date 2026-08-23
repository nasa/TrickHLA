/*!
@file TrickHLA/DebugHandler.cpp
@ingroup TrickHLA
@brief Multi-level debug reporter.

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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{utils/StringUtilities.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3 Titan Group, IMSim, Jan 2010, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2020, --, Rewrite to use static data and functions.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, August 2026, --, Added support for HLA Exceptions.}
@revs_end

*/

// System includes.
#include <cstdlib>
#include <string>

// Trick includes.
#include "trick/exec_proto.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/utils/StringUtilities.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Exception.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif // IEEE_1516_2025

using namespace std;
using namespace TrickHLA;

// Initialize the DebugHandler level and code section values.
DebugLevelEnum  DebugHandler::debug_level  = DEBUG_LEVEL_NO_TRACE;
DebugSourceEnum DebugHandler::code_section = DEBUG_SOURCE_ALL_MODULES;

void DebugHandler::set(
   DebugLevelEnum const  level,
   DebugSourceEnum const code )
{
   if ( level <= DEBUG_LEVEL_NO_TRACE ) {
      debug_level = DEBUG_LEVEL_NO_TRACE;
   } else if ( level >= DEBUG_LEVEL_FULL_TRACE ) {
      debug_level = DEBUG_LEVEL_FULL_TRACE;
   } else {
      debug_level = level;
   }
   if ( code <= DEBUG_SOURCE_NO_MODULES ) {
      code_section = DEBUG_SOURCE_NO_MODULES;
   } else if ( code >= DEBUG_SOURCE_ALL_MODULES ) {
      code_section = DEBUG_SOURCE_ALL_MODULES;
   } else {
      code_section = code;
   }
}

void DebugHandler::print_message(
   string const      &message,
   MESSAGE_TYPE const msg_level )
{
   message_publish( msg_level, message.c_str() );
}

void DebugHandler::print_message(
   string const      &pretty_func_name,
   size_t const       line_number,
   string const      &message,
   MESSAGE_TYPE const msg_level )
{
   string msg = StringUtilities::format( pretty_func_name, line_number );

   switch ( msg_level ) {
      case MSG_NORMAL:
         msg.append( " " );
         break;
      case MSG_INFO:
         msg.append( " INFO: " );
         break;
      case MSG_WARNING:
         msg.append( " WARNING: " );
         break;
      case MSG_ERROR:
         msg.append( " ERROR: " );
         break;
      case MSG_DEBUG:
         msg.append( " DEBUG: " );
         break;
      case MSG_PLAYBACK:
         msg.append( " PLAYBACK: " );
         break;
      default:
         msg.append( " " );
         break;
   }
   msg.append( message );

   message_publish( msg_level, msg.c_str() );
}

void DebugHandler::print_exception(
   string const                       &pretty_func_name,
   size_t const                        line_number,
   RTI1516_NAMESPACE::Exception const &e )
{
   string rti_exception_msg;
   StringUtilities::to_string( rti_exception_msg, e.what() );

   string const errmsg = StringUtilities::format( pretty_func_name, line_number )
                            .append( " WARNING: RTI Exception '" )
                            .append( rti_exception_msg )
                            .append( "'\n" );
   message_publish( MSG_WARNING, errmsg.c_str() );
}

void DebugHandler::terminate(
   string const &message,
   int const     exit_code )
{
   message_publish( MSG_ERROR, message.c_str() );
   exec_terminate( __FILE__, message.c_str() );
   exit( exit_code );
}

void DebugHandler::terminate(
   string const &pretty_func_name,
   size_t const  line_number,
   string const &message,
   int const     exit_code )
{
   string const errmsg = StringUtilities::format( pretty_func_name, line_number )
                            .append( " ERROR: " )
                            .append( message );

   DebugHandler::terminate( errmsg, exit_code );
}

void DebugHandler::terminate(
   string const                       &pretty_func_name,
   size_t const                        line_number,
   RTI1516_NAMESPACE::Exception const &e,
   int const                           exit_code )
{
   string rti_exception_msg;
   StringUtilities::to_string( rti_exception_msg, e.what() );

   string const errmsg = StringUtilities::format( pretty_func_name, line_number )
                            .append( " ERROR: RTI Exception '" )
                            .append( rti_exception_msg )
                            .append( "'\n" );

   DebugHandler::terminate( errmsg, exit_code );
}
