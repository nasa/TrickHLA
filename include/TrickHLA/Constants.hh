/*!
@file TrickHLA/Constants.hh
@ingroup TrickHLA
@brief Define the global time constants used in TrickHLA.

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

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../source/TrickHLA/Int64Interval.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_CONSTANTS_HH
#define TRICKHLA_CONSTANTS_HH

// System include files.
#include <stdint.h>

namespace TrickHLA
{

// Reference the global static representations in Int64Interval.cpp
extern int64_t const MICROS_MULTIPLIER;        ///< @trick_units{--}
extern int64_t const MAX_VALUE_IN_MICROS;      ///< @trick_units{us}
extern double const  MAX_LOGICAL_TIME_SECONDS; ///< @trick_units{s}

} // namespace TrickHLA

#endif // TRICKHLA_CONSTANTS_HH
