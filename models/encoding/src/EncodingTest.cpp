/*!
@ingroup encoding
@file models/encoding/src/EncodingTest.cpp
@brief This is a container class for general encoder test data.

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
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderFactory.cpp}
@trick_link_dependency{encoding/src/EncodingTest.cpp}
@trick_link_dependency{encoding/src/BasicData.cpp}
@trick_link_dependency{encoding/src/BasicData.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <sstream>
#include <string>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/integrator_c_intf.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/Types.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/EncoderFactory.hh"

// Model include files.
#include "../include/BasicData.hh"
#include "../include/EncodingTest.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
EncodingTest::EncodingTest()
{
   return;
}

/*!
 * @job_class{shutdown}
 */
EncodingTest::~EncodingTest()
{
   return;
}

void EncodingTest::test(
   string const &data1_trick_base_name,
   BasicData    &data1,
   string const &data2_trick_base_name,
   BasicData    &data2 )
{
   ostringstream msg1;
   msg1 << "EncodingTest::test():" << __LINE__ << "\n"
        << "BEFORE encode/decode:\n"
        << "Data1: " << data1.to_string()
        << "-----------------------------" << "\n"
        << "Data2: " << data2.to_string();
   message_publish( MSG_NORMAL, msg1.str().c_str() );

   EncodingEnum const rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   EncoderBase *data1_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".i32", rti_encoding );

   EncoderBase *data1_vec3_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_i32", rti_encoding );

   EncoderBase *data1_m3x3_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_i32", rti_encoding );

   EncoderBase *data1_p_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".p_i32", rti_encoding );

   EncoderBase *data2_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".i32", rti_encoding );

   EncoderBase *data2_vec3_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_i32", rti_encoding );

   EncoderBase *data2_m3x3_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_i32", rti_encoding );

   EncoderBase *data2_p_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".p_i32", rti_encoding );

   ostringstream msg2;
   msg2 << "EncodingTest::test():" << __LINE__ << "\n"
        << "     data1_i32_encoder: " << data1_i32_encoder->to_string() << "\n"
        << "data1_vec3_i32_encoder: " << data1_vec3_i32_encoder->to_string() << "\n"
        << "data1_m3x3_i32_encoder: " << data1_m3x3_i32_encoder->to_string() << "\n"
        << "   data1_p_i32_encoder: " << data1_p_i32_encoder->to_string() << "\n"
        << "     data2_i32_encoder: " << data2_i32_encoder->to_string() << "\n"
        << "data2_vec3_i32_encoder: " << data2_vec3_i32_encoder->to_string() << "\n"
        << "data2_m3x3_i32_encoder: " << data2_m3x3_i32_encoder->to_string() << "\n"
        << "   data2_p_i32_encoder: " << data2_p_i32_encoder->to_string() << "\n";
   message_publish( MSG_NORMAL, msg2.str().c_str() );

   data2_i32_encoder->decode( data1_i32_encoder->encode() );
   data2_vec3_i32_encoder->decode( data1_vec3_i32_encoder->encode() );
   data2_m3x3_i32_encoder->decode( data1_m3x3_i32_encoder->encode() );
   data2_p_i32_encoder->decode( data1_p_i32_encoder->encode() );

   if ( !data1.compare( data2 ) ) {
      message_publish( MSG_ERROR, "data1 != data2]n" );
   }

   ostringstream msg3;
   msg3 << "EncodingTest::test():" << __LINE__ << "\n"
        << "AFTER encode/decode:\n"
        << "Data1: " << data1.to_string()
        << "-----------------------------\n"
        << "Data2: " << data2.to_string();

   message_publish( MSG_NORMAL, msg3.str().c_str() );
}
