/*!
@file TrickHLA/utils/StringUtilities.cpp
@ingroup TrickHLA
@brief Implementation of the TrickHLA utilities.

@copyright Copyright 2025 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{../Types.cpp}
@trick_link_dependency{Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, May 2025, --, Implementation needed for static color strings.}
@revs_end

*/

// System includes.
#include <string>

// TrickHLA includes.
#include "TrickHLA/utils/StringUtilities.hh"

using namespace TrickHLA;

std::string const StringUtilities::BLACK_TEXT   = "\x1b[30m";
std::string const StringUtilities::RED_TEXT     = "\x1b[31m";
std::string const StringUtilities::GREEN_TEXT   = "\x1b[32m";
std::string const StringUtilities::YELLOW_TEXT  = "\x1b[33m";
std::string const StringUtilities::BLUE_TEXT    = "\x1b[34m";
std::string const StringUtilities::MAGENTA_TEXT = "\x1b[35m";
std::string const StringUtilities::CYAN_TEXT    = "\x1b[36m";
std::string const StringUtilities::WHITE_TEXT   = "\x1b[37m";
std::string const StringUtilities::DEFAULT_TEXT = "\x1b[0m";
