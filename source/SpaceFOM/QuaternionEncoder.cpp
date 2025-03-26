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

// System include files.
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/StandardsSupport.hh"

// Model include files.
#include "SpaceFOM/QuaternionEncoder.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
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
   // Build up the encoders.
   // ObjectClass: ReferenceFrame, FOM-Module: SISO_SpaceFOM_environment.xml
   //   Attribute-Name: attitude_quaternion, DataType: AttitudeQuaternion, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //      field-name: scalar, dataType: Scalar, representation: HLAfloat64LE
   //      field-name: vector, dataType: Vector, dataType:(Scalar,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   //

   // Build up the attitude quaternion encoder.
   // Quaternion:
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
}

/**
 * @job_class{scheduled}
 */
void QuaternionEncoder::encode() // Return: -- Nothing.
{
   // Encode the data into the reference frame buffer.
   VariableLengthData encoded_data = encoder.encode();

   // Copy the encoded data into the buffer.
   if ( get_capacity() >= encoded_data.size() ) {
      memcpy( buffer, encoded_data.data(), encoded_data.size() );
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
   // The Encoder helps operate on VariableLengthData so create one using the
   // buffered HLA data we received through the TrickHLA callback.
   VariableLengthData encoded_data = VariableLengthData( buffer, capacity );

   // Use the HLA encoder helpers to decode the reference frame fixed record.
   encoder.decode( encoded_data );

   return;
}
