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
@revs_end

*/

#ifndef _TRICKHLA_STANDARDS_SUPPORT_HH_
#define _TRICKHLA_STANDARDS_SUPPORT_HH_

// Default to the IEEE-1516.1-2010 Standard if no HLA standard was defined.
#if ( !defined( IEEE_1516_2010 ) && !defined( IEEE_1516_2020 ) )
#define IEEE_1516_2010
#endif

// Insert a compile time error if HLA Standard 2020 is defined.
#if ( defined( IEEE_1516_2020 ) )
#error "ERROR: IEEE_1516_2020 standard is not yet supported!"
#endif

// Insert a compile time error if more than one HLA Standard is defined.
#if ( defined( IEEE_1516_2010 ) && defined( IEEE_1516_2020 ) )
#error "ERROR: Only one of IEEE_1516_2010 or IEEE_1516_2020 can be defined!"
#endif

// Define the RTI header and namespace for HLA IEEE-1516.1-2010 Standard, which
// is also known as "HLA Evolved".
#ifdef IEEE_1516_2010
#define RTI1516_HEADER "RTI/RTI1516.h"
#define RTI1516_NAMESPACE rti1516e
#define RTI1516_USERDATA rti1516e::VariableLengthData
#define RTI1516_EXCEPTION rti1516e::Exception
#endif

// The CERTI Runtime Infrastructure (RTI) does not support some of the
// IEEE 1516 APIs.
#ifdef CERTI_RTI
#ifdef IEEE_1516_2010
#define UNSUPPORTED_RTI_NAME_API
#define UNSUPPORTED_RTI_VERSION_API
#define UNSUPPORTED_TIME_MANAGEMENT_API

// Set a default RTI name and version.
#ifdef UNSUPPORTED_RTI_NAME_API
#define RTI_NAME "CERTI"
#endif
#ifdef UNSUPPORTED_RTI_VERSION_API
#define RTI_VERSION "Unknown"
#endif
#endif
#endif

#endif // _TRICKHLA_STANDARDS_SUPPORT_HH_
