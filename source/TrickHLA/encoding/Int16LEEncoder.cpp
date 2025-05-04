/*!
@file TrickHLA/Int16LEEncoder.cpp
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
@trick_link_dependency{Int16LEEncoder.cpp}
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
#include "TrickHLA/encoding/Int16LEEncoder.hh"

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
#include "RTI/encoding/HLAopaqueData.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @details The endianess of the computer is determined as part of the
 * Int16LEEncoder construction process.
 * @job_class{initialization}
 */
Int16LEEncoder::Int16LEEncoder(
   std::string const &trick_variable_name,
   std::string const &fom_variable_name,
   EncodingEnum       hla_encoding )
   : trick_name( trick_variable_name ),
     fom_name( fom_variable_name ),
     rti_encoding( hla_encoding ),
     ref2( NULL ),
     is_array( false ),
     is_1d_array( false ),
     is_static_array( false ),
     encoder( NULL ),
     initialized( false )
{
   initialize();
}

/*!
 * @details The buffer and ref2 values are freed and nulled.
 * @job_class{shutdown}
 */
Int16LEEncoder::~Int16LEEncoder()
{
   if ( ref2 != NULL ) {
      free( ref2 );
      ref2 = NULL;
   }

   if ( encoder != NULL ) {
      delete encoder;
      encoder = NULL;
   }
}

void Int16LEEncoder::initialize()
{
   if ( ref2 != NULL ) {
      free( ref2 );
   }
   ref2 = ref_attributes( trick_name.c_str() );

   // Determine if we had an error getting the ref-attributes.
   if ( ref2 == NULL ) {
      ostringstream errmsg;
      errmsg << "Int16LEEncoder::initialize():" << __LINE__
             << " ERROR: For FOM name '" << fom_name << "', Error retrieving"
             << " Trick ref-attributes for '" << trick_name << "'. Please check"
             << " your input or modified-data files to make sure the object"
             << " attribute Trick name is correctly specified. If '"
             << trick_name << "' is an inherited variable then make"
             << " sure the base class uses either the 'public' or 'protected'"
             << " access level for the variable.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }

   // For now, we do not support more than a 1-D array that is dynamic
   // (i.e. a pointer such as char *). If the size of the last indexed
   // attribute is zero then it is a pointer and not static.
   is_array        = ( ref2->attr->num_index > 0 );
   is_1d_array     = ( ref2->attr->num_index == 1 );
   is_static_array = is_array && ( ref2->attr->index[ref2->attr->num_index - 1].size != 0 );

   if ( ref2->attr->type != TRICK_WSTRING ) {
      ostringstream errmsg;
      errmsg << "Int16LEEncoder::initialize():" << __LINE__
             << " ERROR: For FOM name '" << fom_name
             << "', the Trick type for the '" << trick_name
             << "' simulation variable (type:"
             << Utilities::get_trick_type_string( ref2->attr->type )
             << ") is not the expected type '"
             << Utilities::get_trick_type_string( TRICK_WSTRING )
             << "'.\n";
      DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Cases:
   // 1) wchar_t     !is_array
   // 2) wchar_t*    is_1d_array
   // 3) wchar_t[10] is_static_array
   // 4) wstring     is_1d_array

   switch ( rti_encoding ) {
      case ENCODING_OPAQUE_DATA: {
         int num_bytes = get_size( ref2->address );
         this->encoder = new HLAopaqueData( static_cast< Octet * >( ref2->address ), num_bytes );
         break;
      }
      case ENCODING_UNICODE_STRING: {
         if ( !is_array ) {
            this->encoder = new HLAunicodeChar();
         } else {
            this->encoder = new HLAunicodeString();
         }
         break;
      }
      default: {
         // ERROR
         break;
      }
   }

   this->initialized = true;
}
