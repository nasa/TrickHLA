/**
@file SpaceFOM/SpaceTimeCoordinateEncoder.cpp
@ingroup SpaceFOM
@brief This file contains the methods for the SpaceTimeCoordinate encoder class

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
@trick_link_dependency{SpaceTimeCoordinateEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, --, May 2016, NExSyS, Initial version}
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
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/StringUtilities.hh"

// Model include files.
#include "SpaceFOM/SpaceTimeCoordinateEncoder.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA Encoder helper includes.
#include RTI1516_HEADER
#include "RTI/VariableLengthData.h"
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;
using namespace SpaceFOM;

/**
 * @job_class{initialization}
 */
SpaceTimeCoordinateEncoder::SpaceTimeCoordinateEncoder(
   SpaceTimeCoordinateData &stc_data )
   : data( stc_data ),
     position_encoder( HLAfloat64LE(), 3 ),
     velocity_encoder( HLAfloat64LE(), 3 ),
     trans_state_encoder(),
     quat_scalar_encoder( &data.att.scalar ),
     quat_vector_encoder( HLAfloat64LE(), 3 ),
     quat_encoder(),
     ang_vel_encoder( HLAfloat64LE(), 3 ),
     rot_state_encoder(),
     time_encoder( &data.time ),
     encoder()
{
   // Build up the encoders.
   // ObjectClass: ReferenceFrame, FOM-Module: SISO_SpaceFOM_environment.xml
   //   Attribute-Name: state, dataType: SpaceTimeCoordinateState, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //     field-Name: translational_state, dataType: ReferenceFrameTranslation, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //       field-Name: position, dataType: PositionVectordata, dataType: (Length,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   //       field-Name: velocity, dataType: VelocityVector, dataType: (Velocity,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   //     field-Name: rotational_state, dataType: ReferenceFrameRotation, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //       field-Name: attitude_quaternion, DataType: AttitudeQuaternion, encoding: HLAfixedRecord, FOM-Module: SISO_SpaceFOM_datatypes.xml
   //         field-name: scalar, dataType: Scalar, representation: HLAfloat64LE
   //         field-name: vector, dataType: Vector, dataType:(Scalar,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality: 3
   //       field-Name: angular_velocity, dataType: AngularVelocityVector
   //         AngularVelocityVector: dataType:(AngularRate,representation:HLAfloat64LE), encoding:HLAfixedArray, cardinality:3
   //

   // Build up the Translational state encoder.
   for ( int i = 0; i < 3; ++i ) {
      // Position.
      position[i].setDataPointer( &data.pos[i] );
      position_encoder.setElementPointer( i, &position[i] );
      // Velocity.
      velocity[i].setDataPointer( &data.vel[i] );
      velocity_encoder.setElementPointer( i, &velocity[i] );
   }
   // Add position and velocity encoders to the translational state encoder.
   trans_state_encoder.appendElementPointer( &position_encoder );
   trans_state_encoder.appendElementPointer( &velocity_encoder );

   // Build up the Rotational state encoder.
   // Quaternion:
   // Quaternion scalar: The attitude quaternion scalar encoder was configured
   // in the constructor initialization list above.
   // Quaternion vector:
   for ( int i = 0; i < 3; ++i ) {
      quat_vector[i].setDataPointer( &data.att.vector[i] );
      quat_vector_encoder.setElementPointer( i, &quat_vector[i] );
   }
   // Add the scalar and vector encoders to the quaternion encoder.
   quat_encoder.appendElementPointer( &quat_scalar_encoder );
   quat_encoder.appendElementPointer( &quat_vector_encoder );

   // Angular velocity:
   for ( int i = 0; i < 3; ++i ) {
      angular_velocity[i].setDataPointer( &data.ang_vel[i] );
      ang_vel_encoder.setElementPointer( i, &angular_velocity[i] );
   }
   // Add attitude quaternion and angular velocity encoders to the
   // rotational state encoder.
   rot_state_encoder.appendElementPointer( &quat_encoder );
   rot_state_encoder.appendElementPointer( &ang_vel_encoder );

   // Build up the time encoder.
   // NOTE: The time encoder was configured in the constructor initialization
   // list above.

   // Add the translation and rotational state encoders to space/time encoder.
   encoder.appendElementPointer( &trans_state_encoder );
   encoder.appendElementPointer( &rot_state_encoder );
   encoder.appendElementPointer( &time_encoder );

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
void SpaceTimeCoordinateEncoder::encode()
{
   // Encode the STC data into the VariableLengthData encoded data.
   try {
      encoder.encode( encoded_data );
   } catch ( rti1516e::EncoderException & e ) {
      ostringstream errmsg;
      std::string what_s;
      StringUtilities::to_string( what_s, e.what() );
      errmsg << "SpaceFOM::SpaceTimeCoordinateEncoder::encode():" << __LINE__
             << " Error: Encoder exception!" << std::endl;
      errmsg << what_s;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   // Copy the encoded data into the buffer.
   if ( get_capacity() >= encoded_data.size() ) {

      // Note that the encode operation above encodes into the encoded_data
      // VariableLengthData instance but into a different data area that the
      // one set in the constructor above.  So, we have to copy the encoded
      // data into the transmission buffer.
      memcpy( buffer, encoded_data.data(), encoded_data.size() );

   } else {

      // Print message and terminate.
      ostringstream errmsg;
      errmsg << "SpaceFOM::SpaceTimeCoordinateEncoder::encode():" << __LINE__
             << " WARNING: Encoded data size does not match buffer!"
             << "    Encoded size: " << encoded_data.size()
             << " but Expected size: " << get_capacity();
      message_publish( MSG_WARNING, errmsg.str().c_str() );

   }

   return;
}

/**
 * @job_class{scheduled}
 */
void SpaceTimeCoordinateEncoder::decode()
{

   // Decode the STC fixed record.  This will decode the incoming data
   // directly into the STC data instance passed into the constructor.
   try {
      encoder.decode( encoded_data );
   } catch( rti1516e::EncoderException & e ) {
      ostringstream errmsg;
      std::string what_s;
      StringUtilities::to_string( what_s, e.what() );
      errmsg << "SpaceFOM::SpaceTimeCoordinateEncoder::decode():" << __LINE__
             << " Error: Encoder exception!" << std::endl;
      errmsg << what_s;
      // Print message and terminate.
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }

   return;
}
