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

// Trick includes.

// TrickHLA includes.
#include "TrickHLA/utils/StringUtilities.hh"

using namespace std;
using namespace TrickHLA;

const std::string StringUtilities::BLACK_TEXT   = "\x1b[30m";
const std::string StringUtilities::RED_TEXT     = "\x1b[31m";
const std::string StringUtilities::GREEN_TEXT   = "\x1b[32m";
const std::string StringUtilities::YELLOW_TEXT  = "\x1b[33m";
const std::string StringUtilities::BLUE_TEXT    = "\x1b[34m";
const std::string StringUtilities::MAGENTA_TEXT = "\x1b[35m";
const std::string StringUtilities::CYAN_TEXT    = "\x1b[36m";
const std::string StringUtilities::WHITE_TEXT   = "\x1b[37m";
const std::string StringUtilities::DEFAULT_TEXT = "\x1b[0m";

