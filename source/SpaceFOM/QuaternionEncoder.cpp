/**
@file SpaceFOM/QuaternionEncoder.cpp
@ingroup SpaceFOM
@brief This file contains the methods for the QuaternionEncoder class

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{QuaternionEncoder.cpp}

@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, --, July 2018, NExSyS, Initial version}
@revs_end
*/

// System includes.
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/message_proto.h"
#include "trick/message_type.h"

// SpaceFOM includes.
#include "SpaceFOM/QuaternionData.hh"
#include "SpaceFOM/QuaternionEncoder.hh"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StandardsSupport.hh"
#include "TrickHLA/StringUtilities.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/EncodingExceptions.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/**
 * @job_class{initialization}
 */
QuaternionEncoder::QuaternionEncoder(
   QuaternionData &quat_data )
   : data( quat_data ),
     scalar_encoder( &data.scalar ),
     vector_encoder( HLAfloat64LE(), 3 ),
     encoder()
{
   // Build up the QuaternionData encoder.
   // DataType: AttitudeQuaternion
   // Encoding: HLAfixedRecord
   // FOM-Module: SISO_SpaceFOM_datatypes.xml
   //    field-name: scalar, dataType: Scalar, representation: HLAfloat64LE
   //    field-name: vector, dataType: Vector, dataType:(Scalar,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3

   // Build up the attitude quaternion encoder:
   // Quaternion scalar: The attitude quaternion scalar encoder was configured
   // in the constructor initialization list above.
   // Quaternion vector:
   for ( int i = 0; i < 3; ++i ) {
      vector[i].setDataPointer( &data.vector[i] );
      vector_encoder.setElementPointer( i, &vector[i] );
   }
   // Add the scalar and vector encoders to the quaternion encoder.
   encoder.appendElementPointer( &scalar_encoder );
   encoder.appendElementPointer( &vector_encoder );

   // Setup the TrickHLA buffer based on the size of the encoded fixed record.
   // We can do this here because the record is a fixed size all the time.
   set_byte_alignment( 1 );
   ensure_buffer_capacity( encoder.getEncodedLength() );

   // Initialize the variable length data to point to the buffer.
   encoded_data.setDataPointer( buffer, capacity );

   return;
}

/**
 * @job_class{scheduled}
 */
void QuaternionEncoder::encode() // Return: -- Nothing.
{

   // Encode the quaternion data into the VariableLengthData encoded data.
   try {
      encoder.encode( encoded_data );
   } catch ( RTI1516_NAMESPACE::EncoderException &e ) {
      ostringstream errmsg;
      std::string   what_s;
      StringUtilities::to_string( what_s, e.what() );
      errmsg << "SpaceFOM::QuaternionEncoder::encode():" << __LINE__
             << " Error: Encoder exception!" << std::endl;
      errmsg << what_s;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Copy the encoded data into the outgoing buffer.
   if ( get_capacity() >= encoded_data.size() ) {

      // Note that the encode operation above encodes into the encoded_data
      // VariableLengthData instance but into a different data area that the
      // one set in the constructor above.  So, we have to copy the encoded
      // data into the transmission buffer.
      memcpy( buffer, encoded_data.data(), encoded_data.size() ); // flawfinder: ignore

   } else {

      // Print message and terminate.
      ostringstream errmsg;
      errmsg << "SpaceFOM::QuaternionEncoder::encode():" << __LINE__
             << " Warning: Encoded data size does not match buffer!"
             << "    Encoded size: " << encoded_data.size()
             << " but Expected size: " << get_capacity();
      message_publish( MSG_WARNING, errmsg.str().c_str() );
   }

   return;
}

/**
 * @job_class{scheduled}
 */
void QuaternionEncoder::decode() // Return: -- Nothing.
{

   // Decode the quaternion fixed record.  This will decode the incoming data
   // directly into the QuaternionData instance passed into the constructor.
   try {
      encoder.decode( encoded_data );
   } catch ( RTI1516_NAMESPACE::EncoderException &e ) {
      ostringstream errmsg;
      std::string   what_s;
      StringUtilities::to_string( what_s, e.what() );
      errmsg << "SpaceFOM::QuaternionEncoder::decode():" << __LINE__
             << " Error: Encoder exception!" << std::endl;
      errmsg << what_s;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   return;
}
