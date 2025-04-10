/*!
@file TrickHLA/Version.hh
@ingroup TrickHLA
@brief Definition of the TrickHLA version tag.

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

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, December 2012, --, Version 2 origin.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, January 2015, --, Added release date.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@revs_end
*/

#ifndef TRICKHLA_VERSION_HH
#define TRICKHLA_VERSION_HH

// System include files.
#include <string>

namespace TrickHLA
{

// Version of the form: "Major.Minor.Patch"
static unsigned int const TRICKHLA_MAJOR_VERSION = 3;
static unsigned int const TRICKHLA_MINOR_VERSION = 1;
static unsigned int const TRICKHLA_PATCH_VERSION = 18;

// Release date of the form: "YYYY-MM-DD"
static std::string const TRICKHLA_RELEASE_DATE = "2025-04-10";

} // namespace TrickHLA

#endif // TRICKHLA_VERSION_HH
