/*!
@file TrickHLA/MemoryServices.cpp
@ingroup TrickHLA
@brief Memory management services implementation class for TrickHLA.

@detail This class provide the Trick-based memory management services used by
the TrickHLA source code for all memory management tasks.

@copyright Copyright 2026 United States Government as represented by the
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
@trick_link_dependency{../../../source/TrickHLA/MemoryServices.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2026, --, Initial implementation.}
@revs_end

*/

// System includes.
#include <cstddef>

// TrickHLA includes.
#include "TrickHLA/MemoryServices.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @details This is a pure virtual destructor.
 * @job_class{shutdown}
 */
MemoryServices::~MemoryServices()
{
   return;
}

void *MemoryServices::declare_var( char const *declaration )
{
   return ( NULL );
}

void *MemoryServices::declare_var( char const *enh_type_spec, int n_elems )
{
   return ( NULL );
}
