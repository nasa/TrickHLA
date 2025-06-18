/*!
@file TrickHLA/encoding/FixedRecordEncoder.cpp
@ingroup TrickHLA
@brief This class represents the fixed record encoder implementation.

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
@trick_link_dependency{FixedRecordEncoder.cpp}
@trick_link_dependency{EncoderBase.cpp}
@trick_link_dependency{../DebugHandler.cpp}
@trick_link_dependency{../Int64BaseTime.cpp}
@trick_link_dependency{../Types.cpp}


@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstddef>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/attributes.h"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/parameter_types.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/Int64BaseTime.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/FixedRecordEncoder.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/HLAfixedRecord.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

FixedRecordEncoder::FixedRecordEncoder()
   : EncoderBase()
{
   this->data_encoder = new HLAfixedRecord();
}

FixedRecordEncoder::~FixedRecordEncoder()
{
   return;
}

void FixedRecordEncoder::update_before_encode()
{
   HLAfixedRecord *fixed_rec = dynamic_cast< HLAfixedRecord * >( this->data_encoder );
   update_before_encode( fixed_rec );
}

void FixedRecordEncoder::update_before_encode(
   HLAfixedRecord *fixed_rec )
{
   if ( fixed_rec != NULL ) {
      for ( int i = 0; i < fixed_rec->size(); ++i ) {
         try {
            DataElement &data_elem = const_cast< DataElement & >(
               dynamic_cast< DataElement const & >( fixed_rec->get( i ) ) );

            EncoderBase &element_encoder = dynamic_cast< EncoderBase & >( data_elem );
            element_encoder.update_before_encode();
         } catch ( std::bad_cast &e ) {
            ostringstream errmsg;
            errmsg << "FixedRecordEncoder::update_before_encode():" << __LINE__
                   << " ERROR: Unexpected encoder that does not extend EncoderBase!"
                   << std::endl;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      }
   }
}

void FixedRecordEncoder::update_after_decode()
{
   HLAfixedRecord *fixed_rec = dynamic_cast< HLAfixedRecord * >( this->data_encoder );
   update_after_decode( fixed_rec );
}

void FixedRecordEncoder::update_after_decode(
   HLAfixedRecord *fixed_rec )
{
   if ( fixed_rec != NULL ) {
      for ( int i = 0; i < fixed_rec->size(); ++i ) {
         try {
            DataElement &data_elem = const_cast< DataElement & >(
               dynamic_cast< DataElement const & >( fixed_rec->get( i ) ) );

            EncoderBase &element_encoder = dynamic_cast< EncoderBase & >( data_elem );
            element_encoder.update_after_decode();
         } catch ( std::bad_cast &e ) {
            ostringstream errmsg;
            errmsg << "FixedRecordEncoder::update_after_decode():" << __LINE__
                   << " ERROR: Unexpected encoder that does not extend EncoderBase!"
                   << std::endl;
            DebugHandler::terminate_with_message( errmsg.str() );
         }
      }
   }
}
