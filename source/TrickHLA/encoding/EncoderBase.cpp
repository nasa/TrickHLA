/*!
@file TrickHLA/encoding/EncoderBase.cpp
@ingroup TrickHLA
@brief This class represents the base encoder implementation.

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
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstddef>
#include <string>

// TrickHLA include files.
#include "TrickHLA/encoding/EncoderBase.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/encoding/DataElement.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace std;
using namespace TrickHLA;

EncoderBase::EncoderBase()
   : data_encoder( NULL ),
     data(),
     data_name()
{
   return;
}

EncoderBase::EncoderBase(
   string const &name )
   : data_encoder( NULL ),
     data(),
     data_name( name )
{
   return;
}

EncoderBase::~EncoderBase()
{
   if ( data_encoder != NULL ) {
      delete data_encoder;
      data_encoder = NULL;
   }
}
