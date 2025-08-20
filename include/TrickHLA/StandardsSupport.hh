/*!
@file TrickHLA/StandardsSupport.hh
@ingroup TrickHLA
@brief This header file provides the TrickHLA support necessary to hide the
differences between the different HLA Standards that implement the Runtime
Infrastructure (RTI).

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

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, February 2009, --, HLA Standards Support}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2025, --, Added HLA 4 support}
@revs_end

*/

#ifndef TRICKHLA_STANDARDS_SUPPORT_HH
#define TRICKHLA_STANDARDS_SUPPORT_HH

// Default to the IEEE 1516-2010 Standard if no HLA standard is defined.
#if !defined( IEEE_1516_2010 ) && !defined( IEEE_1516_2025 )
#   define IEEE_1516_2010
#endif

// Insert a compile time error if more than one HLA Standard is defined.
#if defined( IEEE_1516_2010 ) && defined( IEEE_1516_2025 )
#   error "ERROR: Only one of IEEE_1516_2010 or IEEE_1516_2025 can be defined!"
#endif

#if defined( IEEE_1516_2010 )
// Define the namespace for the "HLA Evolved" IEEE 1516-2010 Standard.
#   define RTI1516_NAMESPACE rti1516e
#elif defined( IEEE_1516_2025 )
// Define the namespace for the "HLA 4" IEEE 1516-2025 Standard.
#   define RTI1516_NAMESPACE rti1516_2025
#else
#   error "ERROR: Unsupported HLA IEEE 1516 Standard!"
#endif

#endif // TRICKHLA_STANDARDS_SUPPORT_HH
