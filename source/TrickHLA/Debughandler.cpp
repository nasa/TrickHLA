/*!
@file TrickHLA/DebugHndler.cpp
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

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3 Titan Group, IMSim, Jan 2010, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2020, --, Rewrite to use static data and functions.}
@revs_end

*/

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Types.hh"

using namespace TrickHLA;

// Initialize the DebugHandler level and code section values.
DebugLevelEnum  DebugHandler::debug_level  = DEBUG_LEVEL_NO_TRACE;
DebugSourceEnum DebugHandler::code_section = DEBUG_SOURCE_ALL_MODULES;

/*!
 * @job_class{initialization}
 */
DebugHandler::DebugHandler()
{
   set( DEBUG_LEVEL_NO_TRACE, DEBUG_SOURCE_ALL_MODULES );
}

/*!
 * @job_class{shutdown}
 */
DebugHandler::~DebugHandler() // RETURN: -- None.
{
   return;
}

bool DebugHandler::print(
   const DebugLevelEnum & level,
   const DebugSourceEnum &code )
{
   return ( ( debug_level >= level ) && ( ( code_section & code ) != 0 ) );
}

void DebugHandler::set(
   const DebugLevelEnum & level,
   const DebugSourceEnum &code )
{
   debug_level  = level;
   code_section = code;
}