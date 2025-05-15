/*!
@file TrickHLA/Int32FixedArrayEncoder.cpp
@ingroup TrickHLA
@brief This class represents the base encoder implementation.

\par<b>Assumptions and Limitations:</b>
- Only primitive types and static arrays of primitive type are supported for now.

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
@trick_link_dependency{Int32FixedArrayEncoder.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/trick_byteswap.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/Utilities.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/Int32FixedArrayEncoder.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/HLAfixedArray.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details The endianess of the computer is determined as part of the
 * Int32FixedArrayEncoder construction process.
 * @job_class{initialization}
 */
Int32FixedArrayEncoder::Int32FixedArrayEncoder(
   string const      &trick_variable_name,
   EncodingEnum const hla_encoding,
   REF2              *r2 )
   : EncoderBase( trick_variable_name,
                  hla_encoding,
                  r2 )
{
   Int32FixedArrayEncoder::initialize();
}

/*!
 * @details The buffer and ref2 values are freed and nulled.
 * @job_class{shutdown}
 */
Int32FixedArrayEncoder::~Int32FixedArrayEncoder()
{
   return;
}

void Int32FixedArrayEncoder::initialize()
{
   if ( ref2 == NULL ) {
      EncoderBase::initialize();
   }

   if ( ( rti_encoding != ENCODING_LITTLE_ENDIAN )
        && ( rti_encoding != ENCODING_BIG_ENDIAN ) ) {
      ostringstream errmsg;
      errmsg << "Int32FixedArrayEncoder::initialize():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << trick_name
             << "' the HLA encoding specified (" << rti_encoding
             << ") must be either ENCODING_LITTLE_ENDIAN or ENCODING_BIG_ENDIAN!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   bool const valid_type = ( ( ref2->attr->type == TRICK_INTEGER ) && ( sizeof( int ) == sizeof( Integer32 ) ) )
                           || ( ( ref2->attr->type == TRICK_LONG ) && ( sizeof( long ) == sizeof( Integer32 ) ) );
   if ( !valid_type ) {
      ostringstream errmsg;
      errmsg << "Int32FixedArrayEncoder::initialize():" << __LINE__
             << " ERROR: Trick type for the '" << trick_name
             << "' simulation variable (type:"
             << Utilities::get_trick_type_string( ref2->attr->type )
             << ") is not the expected type '"
             << Utilities::get_trick_type_string( TRICK_INTEGER ) << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // This encoder is only for a primitive type.
   if ( is_array ) {
      ostringstream errmsg;
      errmsg << "Int32FixedArrayEncoder::initialize():" << __LINE__
             << " ERROR: Trick ref-attributes for '" << trick_name
             << "' the variable must be a primitive and not an array!\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }
}
