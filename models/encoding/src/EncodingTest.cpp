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
@trick_link_dependency{encoding/src/Float32Data.cpp}
@trick_link_dependency{encoding/src/Float64Data.cpp}
@trick_link_dependency{encoding/src/CharData.cpp}
@trick_link_dependency{encoding/src/Int16Data.cpp}
@trick_link_dependency{encoding/src/Int32Data.cpp}
@trick_link_dependency{encoding/src/Int64Data.cpp}
@trick_link_dependency{encoding/src/LongData.cpp}
@trick_link_dependency{encoding/src/StringData.cpp}

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

// TrickHLA include files.
#include "TrickHLA/Types.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/EncoderFactory.hh"

// Model include files.
#include "../include/CharData.hh"
#include "../include/EncodingTest.hh"
#include "../include/Float32Data.hh"
#include "../include/Float64Data.hh"
#include "../include/Int16Data.hh"
#include "../include/Int32Data.hh"
#include "../include/Int64Data.hh"
#include "../include/LongData.hh"
#include "../include/StringData.hh"

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

void EncodingTest::char_test(
   string const &data1_trick_base_name,
   CharData     &data1,
   string const &data2_trick_base_name,
   CharData     &data2,
   bool const    verbose )
{
   if ( verbose ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::char_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum const rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   EncoderBase *data1_char_encoder = EncoderFactory::create(
      data1_trick_base_name + "._char", rti_encoding );

   EncoderBase *data1_vec3_char_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_char", rti_encoding );

   EncoderBase *data1_m3x3_char_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_char", rti_encoding );

   EncoderBase *data1_ptr_char_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ptr_char", rti_encoding );

   EncoderBase *data2_char_encoder = EncoderFactory::create(
      data2_trick_base_name + "._char", rti_encoding );

   EncoderBase *data2_vec3_char_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_char", rti_encoding );

   EncoderBase *data2_m3x3_char_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_char", rti_encoding );

   EncoderBase *data2_ptr_char_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ptr_char", rti_encoding );

   if ( verbose ) {
      ostringstream msg2;
      msg2 << "EncodingTest::char_test():" << __LINE__ << "\n"
           << "     data1_char_encoder: " << data1_char_encoder->to_string() << "\n"
           << "data1_vec3_char_encoder: " << data1_vec3_char_encoder->to_string() << "\n"
           << "data1_m3x3_char_encoder: " << data1_m3x3_char_encoder->to_string() << "\n"
           << " data1_ptr_char_encoder: " << data1_ptr_char_encoder->to_string() << "\n"
           << "     data2_char_encoder: " << data2_char_encoder->to_string() << "\n"
           << "data2_vec3_char_encoder: " << data2_vec3_char_encoder->to_string() << "\n"
           << "data2_m3x3_char_encoder: " << data2_m3x3_char_encoder->to_string() << "\n"
           << " data2_ptr_char_encoder: " << data2_ptr_char_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_char_encoder->decode( data1_char_encoder->encode() );
   data2_vec3_char_encoder->decode( data1_vec3_char_encoder->encode() );
   data2_m3x3_char_encoder->decode( data1_m3x3_char_encoder->encode() );
   data2_ptr_char_encoder->decode( data1_ptr_char_encoder->encode() );

   if ( data1.compare( data2 ) ) {
      message_publish( MSG_INFO, "char_data1 == char_data2\n" );
   } else {
      message_publish( MSG_ERROR, "char_data1 != char_data2\n" );
   }

   if ( verbose ) {
      ostringstream msg3;
      msg3 << "EncodingTest::char_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::string_test(
   string const &data1_trick_base_name,
   StringData   &data1,
   string const &data2_trick_base_name,
   StringData   &data2,
   bool const    verbose )
{
   if ( verbose ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::string_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum const rti_encoding = TrickHLA::ENCODING_ASCII_STRING;

   EncoderBase *data1_string_encoder = EncoderFactory::create(
      data1_trick_base_name + "._string", rti_encoding );

   EncoderBase *data1_vec3_string_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_string", rti_encoding );

   EncoderBase *data1_m3x3_string_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_string", rti_encoding );

   EncoderBase *data1_ptr_string_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ptr_string", rti_encoding );

   EncoderBase *data2_string_encoder = EncoderFactory::create(
      data2_trick_base_name + "._string", rti_encoding );

   EncoderBase *data2_vec3_string_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_string", rti_encoding );

   EncoderBase *data2_m3x3_string_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_string", rti_encoding );

   EncoderBase *data2_ptr_string_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ptr_string", rti_encoding );

   if ( verbose ) {
      ostringstream msg2;
      msg2 << "EncodingTest::string_test():" << __LINE__ << "\n"
           << "     data1_string_encoder: " << data1_string_encoder->to_string() << "\n"
           << "data1_vec3_string_encoder: " << data1_vec3_string_encoder->to_string() << "\n"
           << "data1_m3x3_string_encoder: " << data1_m3x3_string_encoder->to_string() << "\n"
           << " data1_ptr_string_encoder: " << data1_ptr_string_encoder->to_string() << "\n"
           << "     data2_string_encoder: " << data2_string_encoder->to_string() << "\n"
           << "data2_vec3_string_encoder: " << data2_vec3_string_encoder->to_string() << "\n"
           << "data2_m3x3_string_encoder: " << data2_m3x3_string_encoder->to_string() << "\n"
           << " data2_ptr_string_encoder: " << data2_ptr_string_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_string_encoder->decode( data1_string_encoder->encode() );
   data2_vec3_string_encoder->decode( data1_vec3_string_encoder->encode() );
   data2_m3x3_string_encoder->decode( data1_m3x3_string_encoder->encode() );
   data2_ptr_string_encoder->decode( data1_ptr_string_encoder->encode() );

   if ( data1.compare( data2 ) ) {
      message_publish( MSG_INFO, "char_data1 == char_data2\n" );
   } else {
      message_publish( MSG_ERROR, "char_data1 != char_data2\n" );
   }

   if ( verbose ) {
      ostringstream msg3;
      msg3 << "EncodingTest::string_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::int16_test(
   string const &data1_trick_base_name,
   Int16Data    &data1,
   string const &data2_trick_base_name,
   Int16Data    &data2,
   bool const    verbose )
{
   if ( verbose ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::int16_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum const rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   EncoderBase *data1_i16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".i16", rti_encoding );

   EncoderBase *data1_vec3_i16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_i16", rti_encoding );

   EncoderBase *data1_m3x3_i16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_i16", rti_encoding );

   EncoderBase *data1_ptr_i16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ptr_i16", rti_encoding );

   EncoderBase *data2_i16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".i16", rti_encoding );

   EncoderBase *data2_vec3_i16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_i16", rti_encoding );

   EncoderBase *data2_m3x3_i16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_i16", rti_encoding );

   EncoderBase *data2_ptr_i16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ptr_i16", rti_encoding );

   if ( verbose ) {
      ostringstream msg2;
      msg2 << "EncodingTest::int16_test():" << __LINE__ << "\n"
           << "     data1_i16_encoder: " << data1_i16_encoder->to_string() << "\n"
           << "data1_vec3_i16_encoder: " << data1_vec3_i16_encoder->to_string() << "\n"
           << "data1_m3x3_i16_encoder: " << data1_m3x3_i16_encoder->to_string() << "\n"
           << " data1_ptr_i16_encoder: " << data1_ptr_i16_encoder->to_string() << "\n"
           << "     data2_i16_encoder: " << data2_i16_encoder->to_string() << "\n"
           << "data2_vec3_i16_encoder: " << data2_vec3_i16_encoder->to_string() << "\n"
           << "data2_m3x3_i16_encoder: " << data2_m3x3_i16_encoder->to_string() << "\n"
           << " data2_ptr_i16_encoder: " << data2_ptr_i16_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_i16_encoder->decode( data1_i16_encoder->encode() );
   data2_vec3_i16_encoder->decode( data1_vec3_i16_encoder->encode() );
   data2_m3x3_i16_encoder->decode( data1_m3x3_i16_encoder->encode() );
   data2_ptr_i16_encoder->decode( data1_ptr_i16_encoder->encode() );

   if ( data1.compare( data2 ) ) {
      message_publish( MSG_INFO, "int16_data1 == int16_data2\n" );
   } else {
      message_publish( MSG_ERROR, "int16_data1 != int16_data2\n" );
   }

   if ( verbose ) {
      ostringstream msg3;
      msg3 << "EncodingTest::int16_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::int32_test(
   string const &data1_trick_base_name,
   Int32Data    &data1,
   string const &data2_trick_base_name,
   Int32Data    &data2,
   bool const    verbose )
{
   if ( verbose ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::int32_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum const rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   EncoderBase *data1_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".i32", rti_encoding );

   EncoderBase *data1_vec3_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_i32", rti_encoding );

   EncoderBase *data1_m3x3_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_i32", rti_encoding );

   EncoderBase *data1_ptr_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ptr_i32", rti_encoding );

   EncoderBase *data2_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".i32", rti_encoding );

   EncoderBase *data2_vec3_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_i32", rti_encoding );

   EncoderBase *data2_m3x3_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_i32", rti_encoding );

   EncoderBase *data2_ptr_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ptr_i32", rti_encoding );

   if ( verbose ) {
      ostringstream msg2;
      msg2 << "EncodingTest::int32_test():" << __LINE__ << "\n"
           << "     data1_i32_encoder: " << data1_i32_encoder->to_string() << "\n"
           << "data1_vec3_i32_encoder: " << data1_vec3_i32_encoder->to_string() << "\n"
           << "data1_m3x3_i32_encoder: " << data1_m3x3_i32_encoder->to_string() << "\n"
           << " data1_ptr_i32_encoder: " << data1_ptr_i32_encoder->to_string() << "\n"
           << "     data2_i32_encoder: " << data2_i32_encoder->to_string() << "\n"
           << "data2_vec3_i32_encoder: " << data2_vec3_i32_encoder->to_string() << "\n"
           << "data2_m3x3_i32_encoder: " << data2_m3x3_i32_encoder->to_string() << "\n"
           << " data2_ptr_i32_encoder: " << data2_ptr_i32_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_i32_encoder->decode( data1_i32_encoder->encode() );
   data2_vec3_i32_encoder->decode( data1_vec3_i32_encoder->encode() );
   data2_m3x3_i32_encoder->decode( data1_m3x3_i32_encoder->encode() );
   data2_ptr_i32_encoder->decode( data1_ptr_i32_encoder->encode() );

   if ( data1.compare( data2 ) ) {
      message_publish( MSG_INFO, "int32_data1 == int32_data2\n" );
   } else {
      message_publish( MSG_ERROR, "int32_data1 != int32_data2\n" );
   }

   if ( verbose ) {
      ostringstream msg3;
      msg3 << "EncodingTest::int32_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::int64_test(
   string const &data1_trick_base_name,
   Int64Data    &data1,
   string const &data2_trick_base_name,
   Int64Data    &data2,
   bool const    verbose )
{
   if ( verbose ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::int64_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum const rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   EncoderBase *data1_i64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".i64", rti_encoding );

   EncoderBase *data1_vec3_i64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_i64", rti_encoding );

   EncoderBase *data1_m3x3_i64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_i64", rti_encoding );

   EncoderBase *data1_ptr_i64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ptr_i64", rti_encoding );

   EncoderBase *data2_i64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".i64", rti_encoding );

   EncoderBase *data2_vec3_i64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_i64", rti_encoding );

   EncoderBase *data2_m3x3_i64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_i64", rti_encoding );

   EncoderBase *data2_ptr_i64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ptr_i64", rti_encoding );

   if ( verbose ) {
      ostringstream msg2;
      msg2 << "EncodingTest::int64_test():" << __LINE__ << "\n"
           << "     data1_i64_encoder: " << data1_i64_encoder->to_string() << "\n"
           << "data1_vec3_i64_encoder: " << data1_vec3_i64_encoder->to_string() << "\n"
           << "data1_m3x3_i64_encoder: " << data1_m3x3_i64_encoder->to_string() << "\n"
           << " data1_ptr_i64_encoder: " << data1_ptr_i64_encoder->to_string() << "\n"
           << "     data2_i64_encoder: " << data2_i64_encoder->to_string() << "\n"
           << "data2_vec3_i64_encoder: " << data2_vec3_i64_encoder->to_string() << "\n"
           << "data2_m3x3_i64_encoder: " << data2_m3x3_i64_encoder->to_string() << "\n"
           << " data2_ptr_i64_encoder: " << data2_ptr_i64_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_i64_encoder->decode( data1_i64_encoder->encode() );
   data2_vec3_i64_encoder->decode( data1_vec3_i64_encoder->encode() );
   data2_m3x3_i64_encoder->decode( data1_m3x3_i64_encoder->encode() );
   data2_ptr_i64_encoder->decode( data1_ptr_i64_encoder->encode() );

   if ( data1.compare( data2 ) ) {
      message_publish( MSG_INFO, "int64_data1 == int64_data2\n" );
   } else {
      message_publish( MSG_ERROR, "int64_data1 != int64_data2\n" );
   }

   if ( verbose ) {
      ostringstream msg3;
      msg3 << "EncodingTest::int64_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::long_test(
   string const &data1_trick_base_name,
   LongData     &data1,
   string const &data2_trick_base_name,
   LongData     &data2,
   bool const    verbose )
{
   if ( verbose ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::long_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum const rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   EncoderBase *data1_long_encoder = EncoderFactory::create(
      data1_trick_base_name + "._long", rti_encoding );

   EncoderBase *data1_vec3_long_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_long", rti_encoding );

   EncoderBase *data1_m3x3_long_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_long", rti_encoding );

   EncoderBase *data1_ptr_long_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ptr_long", rti_encoding );

   EncoderBase *data2_long_encoder = EncoderFactory::create(
      data2_trick_base_name + "._long", rti_encoding );

   EncoderBase *data2_vec3_long_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_long", rti_encoding );

   EncoderBase *data2_m3x3_long_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_long", rti_encoding );

   EncoderBase *data2_ptr_long_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ptr_long", rti_encoding );

   if ( verbose ) {
      ostringstream msg2;
      msg2 << "EncodingTest::long_test():" << __LINE__ << "\n"
           << "     data1_long_encoder: " << data1_long_encoder->to_string() << "\n"
           << "data1_vec3_long_encoder: " << data1_vec3_long_encoder->to_string() << "\n"
           << "data1_m3x3_long_encoder: " << data1_m3x3_long_encoder->to_string() << "\n"
           << " data1_ptr_long_encoder: " << data1_ptr_long_encoder->to_string() << "\n"
           << "     data2_long_encoder: " << data2_long_encoder->to_string() << "\n"
           << "data2_vec3_long_encoder: " << data2_vec3_long_encoder->to_string() << "\n"
           << "data2_m3x3_long_encoder: " << data2_m3x3_long_encoder->to_string() << "\n"
           << " data2_ptr_long_encoder: " << data2_ptr_long_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_long_encoder->decode( data1_long_encoder->encode() );
   data2_vec3_long_encoder->decode( data1_vec3_long_encoder->encode() );
   data2_m3x3_long_encoder->decode( data1_m3x3_long_encoder->encode() );
   data2_ptr_long_encoder->decode( data1_ptr_long_encoder->encode() );

   if ( data1.compare( data2 ) ) {
      message_publish( MSG_INFO, "long_data1 == long_data2\n" );
   } else {
      message_publish( MSG_ERROR, "long_data1 != long_data2\n" );
   }

   if ( verbose ) {
      ostringstream msg3;
      msg3 << "EncodingTest::long_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::float32_test(
   string const &data1_trick_base_name,
   Float32Data  &data1,
   string const &data2_trick_base_name,
   Float32Data  &data2,
   bool const    verbose )
{
   if ( verbose ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::float32_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum const rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   EncoderBase *data1_f32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".f32", rti_encoding );

   EncoderBase *data1_vec3_f32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_f32", rti_encoding );

   EncoderBase *data1_m3x3_f32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_f32", rti_encoding );

   EncoderBase *data1_ptr_f32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ptr_f32", rti_encoding );

   EncoderBase *data2_f32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".f32", rti_encoding );

   EncoderBase *data2_vec3_f32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_f32", rti_encoding );

   EncoderBase *data2_m3x3_f32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_f32", rti_encoding );

   EncoderBase *data2_ptr_f32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ptr_f32", rti_encoding );

   if ( verbose ) {
      ostringstream msg2;
      msg2 << "EncodingTest::float32_test():" << __LINE__ << "\n"
           << "     data1_f32_encoder: " << data1_f32_encoder->to_string() << "\n"
           << "data1_vec3_f32_encoder: " << data1_vec3_f32_encoder->to_string() << "\n"
           << "data1_m3x3_f32_encoder: " << data1_m3x3_f32_encoder->to_string() << "\n"
           << " data1_ptr_f32_encoder: " << data1_ptr_f32_encoder->to_string() << "\n"
           << "     data2_f32_encoder: " << data2_f32_encoder->to_string() << "\n"
           << "data2_vec3_f32_encoder: " << data2_vec3_f32_encoder->to_string() << "\n"
           << "data2_m3x3_f32_encoder: " << data2_m3x3_f32_encoder->to_string() << "\n"
           << " data2_ptr_f32_encoder: " << data2_ptr_f32_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_f32_encoder->decode( data1_f32_encoder->encode() );
   data2_vec3_f32_encoder->decode( data1_vec3_f32_encoder->encode() );
   data2_m3x3_f32_encoder->decode( data1_m3x3_f32_encoder->encode() );
   data2_ptr_f32_encoder->decode( data1_ptr_f32_encoder->encode() );

   if ( data1.compare( data2 ) ) {
      message_publish( MSG_INFO, "float32_data1 == float32_data2\n" );
   } else {
      message_publish( MSG_ERROR, "float32_data1 != float32_data2\n" );
   }

   if ( verbose ) {
      ostringstream msg3;
      msg3 << "EncodingTest::float32_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::float64_test(
   string const &data1_trick_base_name,
   Float64Data  &data1,
   string const &data2_trick_base_name,
   Float64Data  &data2,
   bool const    verbose )
{
   if ( verbose ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::float64_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum const rti_encoding = TrickHLA::ENCODING_LITTLE_ENDIAN;

   EncoderBase *data1_f64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".f64", rti_encoding );

   EncoderBase *data1_vec3_f64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_f64", rti_encoding );

   EncoderBase *data1_m3x3_f64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_f64", rti_encoding );

   EncoderBase *data1_ptr_f64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ptr_f64", rti_encoding );

   EncoderBase *data2_f64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".f64", rti_encoding );

   EncoderBase *data2_vec3_f64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_f64", rti_encoding );

   EncoderBase *data2_m3x3_f64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_f64", rti_encoding );

   EncoderBase *data2_ptr_f64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ptr_f64", rti_encoding );

   if ( verbose ) {
      ostringstream msg2;
      msg2 << "EncodingTest::float64_test():" << __LINE__ << "\n"
           << "     data1_f64_encoder: " << data1_f64_encoder->to_string() << "\n"
           << "data1_vec3_f64_encoder: " << data1_vec3_f64_encoder->to_string() << "\n"
           << "data1_m3x3_f64_encoder: " << data1_m3x3_f64_encoder->to_string() << "\n"
           << " data1_ptr_f64_encoder: " << data1_ptr_f64_encoder->to_string() << "\n"
           << "     data2_f64_encoder: " << data2_f64_encoder->to_string() << "\n"
           << "data2_vec3_f64_encoder: " << data2_vec3_f64_encoder->to_string() << "\n"
           << "data2_m3x3_f64_encoder: " << data2_m3x3_f64_encoder->to_string() << "\n"
           << " data2_ptr_f64_encoder: " << data2_ptr_f64_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_f64_encoder->decode( data1_f64_encoder->encode() );
   data2_vec3_f64_encoder->decode( data1_vec3_f64_encoder->encode() );
   data2_m3x3_f64_encoder->decode( data1_m3x3_f64_encoder->encode() );
   data2_ptr_f64_encoder->decode( data1_ptr_f64_encoder->encode() );

   if ( data1.compare( data2 ) ) {
      message_publish( MSG_INFO, "float64_data1 == float64_data2\n" );
   } else {
      message_publish( MSG_ERROR, "float64_data1 != float64_data2\n" );
   }

   if ( verbose ) {
      ostringstream msg3;
      msg3 << "EncodingTest::float64_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}
