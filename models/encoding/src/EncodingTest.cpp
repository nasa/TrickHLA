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
@trick_link_dependency{../../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../../source/TrickHLA/RecordElement.cpp}
@trick_link_dependency{../../../source/TrickHLA/Types.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderBase.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/EncoderFactory.cpp}
@trick_link_dependency{../../../source/TrickHLA/encoding/FixedRecordEncoder.cpp}
@trick_link_dependency{encoding/src/EncodingTest.cpp}
@trick_link_dependency{encoding/src/BoolData.cpp}
@trick_link_dependency{encoding/src/Float32Data.cpp}
@trick_link_dependency{encoding/src/Float64Data.cpp}
@trick_link_dependency{encoding/src/CharData.cpp}
@trick_link_dependency{encoding/src/Int16Data.cpp}
@trick_link_dependency{encoding/src/Int32Data.cpp}
@trick_link_dependency{encoding/src/Int64Data.cpp}
@trick_link_dependency{encoding/src/UInt16Data.cpp}
@trick_link_dependency{encoding/src/UInt32Data.cpp}
@trick_link_dependency{encoding/src/UInt64Data.cpp}
@trick_link_dependency{encoding/src/LongData.cpp}
@trick_link_dependency{encoding/src/StringData.cpp}
@trick_link_dependency{encoding/src/WCharData.cpp}
@trick_link_dependency{encoding/src/WStringData.cpp}
@trick_link_dependency{FixedRecord/src/FixedRecData.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <ostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA include files.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/RecordElement.hh"
#include "TrickHLA/StringUtilities.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/encoding/EncoderBase.hh"
#include "TrickHLA/encoding/EncoderFactory.hh"
#include "TrickHLA/encoding/FixedRecordEncoder.hh"

// Model include files.
#include "FixedRecord/include/FixedRecData.hh"
#include "encoding/include/BoolData.hh"
#include "encoding/include/CharData.hh"
#include "encoding/include/EncodingTest.hh"
#include "encoding/include/Float32Data.hh"
#include "encoding/include/Float64Data.hh"
#include "encoding/include/Int16Data.hh"
#include "encoding/include/Int32Data.hh"
#include "encoding/include/Int64Data.hh"
#include "encoding/include/LongData.hh"
#include "encoding/include/StringData.hh"
#include "encoding/include/UInt16Data.hh"
#include "encoding/include/UInt32Data.hh"
#include "encoding/include/UInt64Data.hh"
#include "encoding/include/ULongData.hh"
#include "encoding/include/WCharData.hh"
#include "encoding/include/WStringData.hh"

// C++11 deprecated dynamic exception specifications for a function so we
// need to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/VariableLengthData.h"
#include "RTI/encoding/BasicDataElements.h"
#include "RTI/encoding/DataElement.h"
#include "RTI/encoding/HLAfixedArray.h"
#include "RTI/encoding/HLAfixedRecord.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
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
   string const      &data1_trick_base_name,
   CharData          &data1,
   string const      &data2_trick_base_name,
   CharData          &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::char_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum char_rti_encoding;
   switch ( rti_encoding ) {
      case TrickHLA::ENCODING_UNICODE_STRING:
      case TrickHLA::ENCODING_ASCII_STRING:
      case TrickHLA::ENCODING_OPAQUE_DATA:
      case TrickHLA::ENCODING_NONE: {
         char_rti_encoding = TrickHLA::ENCODING_ASCII_CHAR;
         break;
      }
      default: {
         char_rti_encoding = rti_encoding;
         break;
      }
   }

   EncoderBase *data1_char_encoder = EncoderFactory::create(
      data1_trick_base_name + "._char", char_rti_encoding );

   EncoderBase *data1_vec3_char_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_char", char_rti_encoding );

   EncoderBase *data1_m3x3_char_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_char", char_rti_encoding );

   EncoderBase *data1_ptr_char_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_char", rti_encoding ) );

   EncoderBase *data2_char_encoder = EncoderFactory::create(
      data2_trick_base_name + "._char", char_rti_encoding );

   EncoderBase *data2_vec3_char_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_char", char_rti_encoding );

   EncoderBase *data2_m3x3_char_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_char", char_rti_encoding );

   EncoderBase *data2_ptr_char_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_char", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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

   ostringstream encode_msg;
   encode_msg << "EncodingTest::char_test():" << __LINE__ << endl;

   VariableLengthData encoded_data1_char = data1_char_encoder->encode();
   data2_char_encoder->decode( encoded_data1_char );
   encode_msg << " Encoded data1_char size:" << encoded_data1_char.size()
              << " Encoded-length:" << data1_char_encoder->getEncodedLength()
              << endl;

   VariableLengthData encoded_data1_vec3 = data1_vec3_char_encoder->encode();
   data2_vec3_char_encoder->decode( encoded_data1_vec3 );
   encode_msg << " Encoded data1_vec3 size:" << encoded_data1_vec3.size()
              << " Encoded-length:" << data1_vec3_char_encoder->getEncodedLength()
              << endl;

   VariableLengthData encoded_data1_m3x3 = data1_m3x3_char_encoder->encode();
   data2_m3x3_char_encoder->decode( encoded_data1_m3x3 );
   encode_msg << " Encoded data1_m3x3 size:" << encoded_data1_m3x3.size()
              << " Encoded-length:" << data1_m3x3_char_encoder->getEncodedLength()
              << endl;

   data1_ptr_char_encoder->update_before_encode();
   VariableLengthData encoded_data1_ptr_char = data1_ptr_char_encoder->encode();
   data2_ptr_char_encoder->decode( encoded_data1_ptr_char );
   data2_ptr_char_encoder->update_after_decode();

   encode_msg << " Encoded data1_ptr_char size:" << encoded_data1_ptr_char.size()
              << " Encoded-length:" << data1_ptr_char_encoder->getEncodedLength()
              << endl;

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      message_publish( MSG_NORMAL, encode_msg.str().c_str() );
   }

   ostringstream compare_msg;
   if ( char_rti_encoding == rti_encoding ) {
      compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";
   } else {
      compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ", "
                  << encoding_enum_to_string( char_rti_encoding ) << ") ";
   }

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "char_data1 == char_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "char_data1 != char_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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
   string const      &data1_trick_base_name,
   StringData        &data1,
   string const      &data2_trick_base_name,
   StringData        &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::string_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncodingEnum basic_rti_encoding;
   switch ( rti_encoding ) {
      case TrickHLA::ENCODING_UNICODE_STRING: {
         basic_rti_encoding = TrickHLA::ENCODING_ASCII_STRING;
         break;
      }
      default: {
         basic_rti_encoding = rti_encoding;
         break;
      }
   }

   EncoderBase *data1_string_encoder = EncoderFactory::create(
      data1_trick_base_name + "._string", rti_encoding );

   EncoderBase *data1_vec3_string_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_string", basic_rti_encoding );

   EncoderBase *data1_m3x3_string_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_string", basic_rti_encoding );

   EncoderBase *data1_ptr_string_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_string", basic_rti_encoding ) );

   EncoderBase *data2_string_encoder = EncoderFactory::create(
      data2_trick_base_name + "._string", rti_encoding );

   EncoderBase *data2_vec3_string_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_string", basic_rti_encoding );

   EncoderBase *data2_m3x3_string_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_string", basic_rti_encoding );

   EncoderBase *data2_ptr_string_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_string", basic_rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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

   data1_string_encoder->update_before_encode();
   data2_string_encoder->decode( data1_string_encoder->encode() );
   data2_string_encoder->update_after_decode();

   data2_vec3_string_encoder->decode( data1_vec3_string_encoder->encode() );
   data2_m3x3_string_encoder->decode( data1_m3x3_string_encoder->encode() );

   data1_ptr_string_encoder->update_before_encode();
   data2_ptr_string_encoder->decode( data1_ptr_string_encoder->encode() );
   data2_ptr_string_encoder->update_after_decode();

   ostringstream compare_msg;
   if ( basic_rti_encoding == rti_encoding ) {
      compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";
   } else {
      compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ", "
                  << encoding_enum_to_string( basic_rti_encoding ) << ") ";
   }

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "string_data1 == string_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "string_data1 != string_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << endl;

      int const data1_string_size = data1_string_encoder->get_data_size();
      int const data2_string_size = data2_string_encoder->get_data_size();
      msg3 << data1_string_encoder->to_string() << endl
           << "   data1_string_encoder->get_data_size():" << data1_string_size << endl
           << "   data1._string.size():" << data1._string.size() << endl
           << data2_string_encoder->to_string() << endl
           << "   data2_string_encoder->get_data_size():" << data2_string_size << endl
           << "   data2._string.size():" << data2._string.size() << endl;

      int const data1_vec3_string_size = data1_vec3_string_encoder->get_data_size();
      int const data2_vec3_string_size = data2_vec3_string_encoder->get_data_size();
      msg3 << data1_vec3_string_encoder->to_string() << endl
           << "   data1_vec3_string_encoder->get_data_size():" << data1_vec3_string_size << endl
           << data2_vec3_string_encoder->to_string() << endl
           << "   data2_vec3_string_encoder->get_data_size():" << data2_vec3_string_size << endl;

      int const data1_m3x3_string_size = data1_m3x3_string_encoder->get_data_size();
      int const data2_m3x3_string_size = data2_m3x3_string_encoder->get_data_size();
      msg3 << data1_m3x3_string_encoder->to_string() << endl
           << "   data1_m3x3_string_encoder->get_data_size():" << data1_m3x3_string_size << endl
           << data2_m3x3_string_encoder->to_string() << endl
           << "   data2_m3x3_string_encoder->get_data_size():" << data2_m3x3_string_size << endl;

      int const data1_ptr_string_size = data1_ptr_string_encoder->get_data_size();
      int const data2_ptr_string_size = data2_ptr_string_encoder->get_data_size();
      msg3 << data1_ptr_string_encoder->to_string() << endl
           << "   data1_ptr_string_encoder->get_data_size():" << data1_ptr_string_size << endl
           << data2_ptr_string_encoder->to_string() << endl
           << "   data2_ptr_string_encoder->get_data_size():" << data2_ptr_string_size << endl;

      message_publish( MSG_DEBUG, msg3.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg4;
      msg4 << "EncodingTest::string_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg4.str().c_str() );
   }
}

void EncodingTest::wchar_test(
   string const      &data1_trick_base_name,
   WCharData         &data1,
   string const      &data2_trick_base_name,
   WCharData         &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::wchar_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_wchar_encoder = EncoderFactory::create(
      data1_trick_base_name + "._wchar", rti_encoding );

   EncoderBase *data1_vec3_wchar_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_wchar", rti_encoding );

   EncoderBase *data1_m3x3_wchar_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_wchar", rti_encoding );

   EncoderBase *data1_ptr_wchar_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_wchar", rti_encoding ) );

   EncoderBase *data2_wchar_encoder = EncoderFactory::create(
      data2_trick_base_name + "._wchar", rti_encoding );

   EncoderBase *data2_vec3_wchar_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_wchar", rti_encoding );

   EncoderBase *data2_m3x3_wchar_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_wchar", rti_encoding );

   EncoderBase *data2_ptr_wchar_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_wchar", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg2;
      msg2 << "EncodingTest::wchar_test():" << __LINE__ << "\n"
           << "     data1_wchar_encoder: " << data1_wchar_encoder->to_string() << "\n"
           << "data1_vec3_wchar_encoder: " << data1_vec3_wchar_encoder->to_string() << "\n"
           << "data1_m3x3_wchar_encoder: " << data1_m3x3_wchar_encoder->to_string() << "\n"
           << " data1_ptr_wchar_encoder: " << data1_ptr_wchar_encoder->to_string() << "\n"
           << "     data2_wchar_encoder: " << data2_wchar_encoder->to_string() << "\n"
           << "data2_vec3_wchar_encoder: " << data2_vec3_wchar_encoder->to_string() << "\n"
           << "data2_m3x3_wchar_encoder: " << data2_m3x3_wchar_encoder->to_string() << "\n"
           << " data2_ptr_wchar_encoder: " << data2_ptr_wchar_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_wchar_encoder->decode( data1_wchar_encoder->encode() );
   data2_vec3_wchar_encoder->decode( data1_vec3_wchar_encoder->encode() );
   data2_m3x3_wchar_encoder->decode( data1_m3x3_wchar_encoder->encode() );

   data1_ptr_wchar_encoder->update_before_encode();
   data2_ptr_wchar_encoder->decode( data1_ptr_wchar_encoder->encode() );
   data2_ptr_wchar_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "wchar_data1 == wchar_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "wchar_data1 != wchar_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::wchar_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::wstring_test(
   string const      &data1_trick_base_name,
   WStringData       &data1,
   string const      &data2_trick_base_name,
   WStringData       &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::wstring_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_wstring_encoder = EncoderFactory::create(
      data1_trick_base_name + "._wstring", rti_encoding );

   EncoderBase *data1_vec3_wstring_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_wstring", rti_encoding );

   EncoderBase *data1_m3x3_wstring_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_wstring", rti_encoding );

   EncoderBase *data1_ptr_wstring_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_wstring", rti_encoding ) );

   EncoderBase *data2_wstring_encoder = EncoderFactory::create(
      data2_trick_base_name + "._wstring", rti_encoding );

   EncoderBase *data2_vec3_wstring_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_wstring", rti_encoding );

   EncoderBase *data2_m3x3_wstring_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_wstring", rti_encoding );

   EncoderBase *data2_ptr_wstring_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_wstring", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg2;
      msg2 << "EncodingTest::wstring_test():" << __LINE__ << "\n"
           << "     data1_wstring_encoder: " << data1_wstring_encoder->to_string() << "\n"
           << "data1_vec3_wstring_encoder: " << data1_vec3_wstring_encoder->to_string() << "\n"
           << "data1_m3x3_wstring_encoder: " << data1_m3x3_wstring_encoder->to_string() << "\n"
           << " data1_ptr_wstring_encoder: " << data1_ptr_wstring_encoder->to_string() << "\n"
           << "     data2_wstring_encoder: " << data2_wstring_encoder->to_string() << "\n"
           << "data2_vec3_wstring_encoder: " << data2_vec3_wstring_encoder->to_string() << "\n"
           << "data2_m3x3_wstring_encoder: " << data2_m3x3_wstring_encoder->to_string() << "\n"
           << " data2_ptr_wstring_encoder: " << data2_ptr_wstring_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_wstring_encoder->decode( data1_wstring_encoder->encode() );
   data2_vec3_wstring_encoder->decode( data1_vec3_wstring_encoder->encode() );
   data2_m3x3_wstring_encoder->decode( data1_m3x3_wstring_encoder->encode() );

   data1_ptr_wstring_encoder->update_before_encode();
   data2_ptr_wstring_encoder->decode( data1_ptr_wstring_encoder->encode() );
   data2_ptr_wstring_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "wstring_data1 == wstring_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "wstring_data1 != wstring_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::wstring_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::int16_test(
   string const      &data1_trick_base_name,
   Int16Data         &data1,
   string const      &data2_trick_base_name,
   Int16Data         &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::int16_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_i16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".i16", rti_encoding );

   EncoderBase *data1_vec3_i16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_i16", rti_encoding );

   EncoderBase *data1_m3x3_i16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_i16", rti_encoding );

   EncoderBase *data1_ptr_i16_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_i16", rti_encoding ) );

   EncoderBase *data2_i16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".i16", rti_encoding );

   EncoderBase *data2_vec3_i16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_i16", rti_encoding );

   EncoderBase *data2_m3x3_i16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_i16", rti_encoding );

   EncoderBase *data2_ptr_i16_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_i16", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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

   data1_ptr_i16_encoder->update_before_encode();
   data2_ptr_i16_encoder->decode( data1_ptr_i16_encoder->encode() );
   data1_ptr_i16_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "int16_data1 == int16_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "int16_data1 != int16_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::int16_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::uint16_test(
   string const      &data1_trick_base_name,
   UInt16Data        &data1,
   string const      &data2_trick_base_name,
   UInt16Data        &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::uint16_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_ui16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ui16", rti_encoding );

   EncoderBase *data1_vec3_ui16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_ui16", rti_encoding );

   EncoderBase *data1_m3x3_ui16_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_ui16", rti_encoding );

   EncoderBase *data1_ptr_ui16_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_ui16", rti_encoding ) );

   EncoderBase *data2_ui16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ui16", rti_encoding );

   EncoderBase *data2_vec3_ui16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_ui16", rti_encoding );

   EncoderBase *data2_m3x3_ui16_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_ui16", rti_encoding );

   EncoderBase *data2_ptr_ui16_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_ui16", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg2;
      msg2 << "EncodingTest::uint16_test():" << __LINE__ << "\n"
           << "     data1_ui16_encoder: " << data1_ui16_encoder->to_string() << "\n"
           << "data1_vec3_ui16_encoder: " << data1_vec3_ui16_encoder->to_string() << "\n"
           << "data1_m3x3_ui16_encoder: " << data1_m3x3_ui16_encoder->to_string() << "\n"
           << " data1_ptr_ui16_encoder: " << data1_ptr_ui16_encoder->to_string() << "\n"
           << "     data2_ui16_encoder: " << data2_ui16_encoder->to_string() << "\n"
           << "data2_vec3_ui16_encoder: " << data2_vec3_ui16_encoder->to_string() << "\n"
           << "data2_m3x3_ui16_encoder: " << data2_m3x3_ui16_encoder->to_string() << "\n"
           << " data2_ptr_ui16_encoder: " << data2_ptr_ui16_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_ui16_encoder->decode( data1_ui16_encoder->encode() );
   data2_vec3_ui16_encoder->decode( data1_vec3_ui16_encoder->encode() );
   data2_m3x3_ui16_encoder->decode( data1_m3x3_ui16_encoder->encode() );

   data1_ptr_ui16_encoder->update_before_encode();
   data2_ptr_ui16_encoder->decode( data1_ptr_ui16_encoder->encode() );
   data1_ptr_ui16_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "uint16_data1 == uint16_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "uint16_data1 != uint16_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::uint16_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::int32_test(
   string const      &data1_trick_base_name,
   Int32Data         &data1,
   string const      &data2_trick_base_name,
   Int32Data         &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::int32_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".i32", rti_encoding );

   EncoderBase *data1_vec3_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_i32", rti_encoding );

   EncoderBase *data1_m3x3_i32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_i32", rti_encoding );

   EncoderBase *data1_ptr_i32_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_i32", rti_encoding ) );

   EncoderBase *data2_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".i32", rti_encoding );

   EncoderBase *data2_vec3_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_i32", rti_encoding );

   EncoderBase *data2_m3x3_i32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_i32", rti_encoding );

   EncoderBase *data2_ptr_i32_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_i32", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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

   data1_ptr_i32_encoder->update_before_encode();
   data2_ptr_i32_encoder->decode( data1_ptr_i32_encoder->encode() );
   data2_ptr_i32_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "int32_data1 == int32_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "int32_data1 != int32_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::int32_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::uint32_test(
   string const      &data1_trick_base_name,
   UInt32Data        &data1,
   string const      &data2_trick_base_name,
   UInt32Data        &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::uint32_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_ui32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ui32", rti_encoding );

   EncoderBase *data1_vec3_ui32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_ui32", rti_encoding );

   EncoderBase *data1_m3x3_ui32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_ui32", rti_encoding );

   EncoderBase *data1_ptr_ui32_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_ui32", rti_encoding ) );

   EncoderBase *data2_ui32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ui32", rti_encoding );

   EncoderBase *data2_vec3_ui32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_ui32", rti_encoding );

   EncoderBase *data2_m3x3_ui32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_ui32", rti_encoding );

   EncoderBase *data2_ptr_ui32_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_ui32", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg2;
      msg2 << "EncodingTest::uint32_test():" << __LINE__ << "\n"
           << "     data1_ui32_encoder: " << data1_ui32_encoder->to_string() << "\n"
           << "data1_vec3_ui32_encoder: " << data1_vec3_ui32_encoder->to_string() << "\n"
           << "data1_m3x3_ui32_encoder: " << data1_m3x3_ui32_encoder->to_string() << "\n"
           << " data1_ptr_ui32_encoder: " << data1_ptr_ui32_encoder->to_string() << "\n"
           << "     data2_ui32_encoder: " << data2_ui32_encoder->to_string() << "\n"
           << "data2_vec3_ui32_encoder: " << data2_vec3_ui32_encoder->to_string() << "\n"
           << "data2_m3x3_ui32_encoder: " << data2_m3x3_ui32_encoder->to_string() << "\n"
           << " data2_ptr_ui32_encoder: " << data2_ptr_ui32_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_ui32_encoder->decode( data1_ui32_encoder->encode() );
   data2_vec3_ui32_encoder->decode( data1_vec3_ui32_encoder->encode() );
   data2_m3x3_ui32_encoder->decode( data1_m3x3_ui32_encoder->encode() );

   data1_ptr_ui32_encoder->update_before_encode();
   data2_ptr_ui32_encoder->decode( data1_ptr_ui32_encoder->encode() );
   data2_ptr_ui32_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "uint32_data1 == uint32_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "uint32_data1 != uint32_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::uint32_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::int64_test(
   string const      &data1_trick_base_name,
   Int64Data         &data1,
   string const      &data2_trick_base_name,
   Int64Data         &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::int64_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_i64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".i64", rti_encoding );

   EncoderBase *data1_vec3_i64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_i64", rti_encoding );

   EncoderBase *data1_m3x3_i64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_i64", rti_encoding );

   EncoderBase *data1_ptr_i64_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_i64", rti_encoding ) );

   EncoderBase *data2_i64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".i64", rti_encoding );

   EncoderBase *data2_vec3_i64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_i64", rti_encoding );

   EncoderBase *data2_m3x3_i64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_i64", rti_encoding );

   EncoderBase *data2_ptr_i64_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_i64", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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

   data1_ptr_i64_encoder->update_before_encode();
   data2_ptr_i64_encoder->decode( data1_ptr_i64_encoder->encode() );
   data2_ptr_i64_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "int64_data1 == int64_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "int64_data1 != int64_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::int64_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::uint64_test(
   string const      &data1_trick_base_name,
   UInt64Data        &data1,
   string const      &data2_trick_base_name,
   UInt64Data        &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::uint64_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_ui64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".ui64", rti_encoding );

   EncoderBase *data1_vec3_ui64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_ui64", rti_encoding );

   EncoderBase *data1_m3x3_ui64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_ui64", rti_encoding );

   EncoderBase *data1_ptr_ui64_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_ui64", rti_encoding ) );

   EncoderBase *data2_ui64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".ui64", rti_encoding );

   EncoderBase *data2_vec3_ui64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_ui64", rti_encoding );

   EncoderBase *data2_m3x3_ui64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_ui64", rti_encoding );

   EncoderBase *data2_ptr_ui64_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_ui64", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg2;
      msg2 << "EncodingTest::uint64_test():" << __LINE__ << "\n"
           << "     data1_ui64_encoder: " << data1_ui64_encoder->to_string() << "\n"
           << "data1_vec3_ui64_encoder: " << data1_vec3_ui64_encoder->to_string() << "\n"
           << "data1_m3x3_ui64_encoder: " << data1_m3x3_ui64_encoder->to_string() << "\n"
           << " data1_ptr_ui64_encoder: " << data1_ptr_ui64_encoder->to_string() << "\n"
           << "     data2_ui64_encoder: " << data2_ui64_encoder->to_string() << "\n"
           << "data2_vec3_ui64_encoder: " << data2_vec3_ui64_encoder->to_string() << "\n"
           << "data2_m3x3_ui64_encoder: " << data2_m3x3_ui64_encoder->to_string() << "\n"
           << " data2_ptr_ui64_encoder: " << data2_ptr_ui64_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_ui64_encoder->decode( data1_ui64_encoder->encode() );
   data2_vec3_ui64_encoder->decode( data1_vec3_ui64_encoder->encode() );
   data2_m3x3_ui64_encoder->decode( data1_m3x3_ui64_encoder->encode() );

   data1_ptr_ui64_encoder->update_before_encode();
   data2_ptr_ui64_encoder->decode( data1_ptr_ui64_encoder->encode() );
   data2_ptr_ui64_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "uint64_data1 == uint64_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "uint64_data1 != uint64_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::uint64_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::long_test(
   string const      &data1_trick_base_name,
   LongData          &data1,
   string const      &data2_trick_base_name,
   LongData          &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::long_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_long_encoder = EncoderFactory::create(
      data1_trick_base_name + "._long", rti_encoding );

   EncoderBase *data1_vec3_long_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_long", rti_encoding );

   EncoderBase *data1_m3x3_long_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_long", rti_encoding );

   EncoderBase *data1_ptr_long_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_long", rti_encoding ) );

   EncoderBase *data2_long_encoder = EncoderFactory::create(
      data2_trick_base_name + "._long", rti_encoding );

   EncoderBase *data2_vec3_long_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_long", rti_encoding );

   EncoderBase *data2_m3x3_long_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_long", rti_encoding );

   EncoderBase *data2_ptr_long_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_long", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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

   data1_ptr_long_encoder->update_before_encode();
   data2_ptr_long_encoder->decode( data1_ptr_long_encoder->encode() );
   data2_ptr_long_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "long_data1 == long_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "long_data1 != long_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::long_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::ulong_test(
   string const      &data1_trick_base_name,
   ULongData         &data1,
   string const      &data2_trick_base_name,
   ULongData         &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::ulong_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_ulong_encoder = EncoderFactory::create(
      data1_trick_base_name + "._ulong", rti_encoding );

   EncoderBase *data1_vec3_ulong_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_ulong", rti_encoding );

   EncoderBase *data1_m3x3_ulong_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_ulong", rti_encoding );

   EncoderBase *data1_ptr_ulong_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_ulong", rti_encoding ) );

   EncoderBase *data2_ulong_encoder = EncoderFactory::create(
      data2_trick_base_name + "._ulong", rti_encoding );

   EncoderBase *data2_vec3_ulong_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_ulong", rti_encoding );

   EncoderBase *data2_m3x3_ulong_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_ulong", rti_encoding );

   EncoderBase *data2_ptr_ulong_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_ulong", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg2;
      msg2 << "EncodingTest::ulong_test():" << __LINE__ << "\n"
           << "     data1_ulong_encoder: " << data1_ulong_encoder->to_string() << "\n"
           << "data1_vec3_ulong_encoder: " << data1_vec3_ulong_encoder->to_string() << "\n"
           << "data1_m3x3_ulong_encoder: " << data1_m3x3_ulong_encoder->to_string() << "\n"
           << " data1_ptr_ulong_encoder: " << data1_ptr_ulong_encoder->to_string() << "\n"
           << "     data2_ulong_encoder: " << data2_ulong_encoder->to_string() << "\n"
           << "data2_vec3_ulong_encoder: " << data2_vec3_ulong_encoder->to_string() << "\n"
           << "data2_m3x3_ulong_encoder: " << data2_m3x3_ulong_encoder->to_string() << "\n"
           << " data2_ptr_ulong_encoder: " << data2_ptr_ulong_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_ulong_encoder->decode( data1_ulong_encoder->encode() );
   data2_vec3_ulong_encoder->decode( data1_vec3_ulong_encoder->encode() );
   data2_m3x3_ulong_encoder->decode( data1_m3x3_ulong_encoder->encode() );

   data1_ptr_ulong_encoder->update_before_encode();
   data2_ptr_ulong_encoder->decode( data1_ptr_ulong_encoder->encode() );
   data2_ptr_ulong_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "ulong_data1 == ulong_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "ulong_data1 != ulong_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::ulong_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::float32_test(
   string const      &data1_trick_base_name,
   Float32Data       &data1,
   string const      &data2_trick_base_name,
   Float32Data       &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::float32_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_f32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".f32", rti_encoding );

   EncoderBase *data1_vec3_f32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_f32", rti_encoding );

   EncoderBase *data1_m3x3_f32_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_f32", rti_encoding );

   EncoderBase *data1_ptr_f32_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_f32", rti_encoding ) );

   EncoderBase *data2_f32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".f32", rti_encoding );

   EncoderBase *data2_vec3_f32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_f32", rti_encoding );

   EncoderBase *data2_m3x3_f32_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_f32", rti_encoding );

   EncoderBase *data2_ptr_f32_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_f32", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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

   data1_ptr_f32_encoder->update_before_encode();
   data2_ptr_f32_encoder->decode( data1_ptr_f32_encoder->encode() );
   data2_ptr_f32_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "float32_data1 == float32_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "float32_data1 != float32_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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
   string const      &data1_trick_base_name,
   Float64Data       &data1,
   string const      &data2_trick_base_name,
   Float64Data       &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::float64_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_f64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".f64", rti_encoding );

   EncoderBase *data1_vec3_f64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_f64", rti_encoding );

   EncoderBase *data1_m3x3_f64_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_f64", rti_encoding );

   EncoderBase *data1_ptr_f64_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_f64", rti_encoding ) );

   EncoderBase *data2_f64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".f64", rti_encoding );

   EncoderBase *data2_vec3_f64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_f64", rti_encoding );

   EncoderBase *data2_m3x3_f64_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_f64", rti_encoding );

   EncoderBase *data2_ptr_f64_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_f64", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
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

   data1_ptr_f64_encoder->update_before_encode();
   data2_ptr_f64_encoder->decode( data1_ptr_f64_encoder->encode() );
   data2_ptr_f64_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "float64_data1 == float64_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "float64_data1 != float64_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::float64_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::bool_test(
   string const      &data1_trick_base_name,
   BoolData          &data1,
   string const      &data2_trick_base_name,
   BoolData          &data2,
   EncodingEnum const rti_encoding )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::fixed_record_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   EncoderBase *data1_bool_encoder = EncoderFactory::create(
      data1_trick_base_name + "._bool", rti_encoding );

   EncoderBase *data1_vec3_bool_encoder = EncoderFactory::create(
      data1_trick_base_name + ".vec3_bool", rti_encoding );

   EncoderBase *data1_m3x3_bool_encoder = EncoderFactory::create(
      data1_trick_base_name + ".m3x3_bool", rti_encoding );

   EncoderBase *data1_ptr_bool_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data1_trick_base_name + ".ptr_bool", rti_encoding ) );

   EncoderBase *data2_bool_encoder = EncoderFactory::create(
      data2_trick_base_name + "._bool", rti_encoding );

   EncoderBase *data2_vec3_bool_encoder = EncoderFactory::create(
      data2_trick_base_name + ".vec3_bool", rti_encoding );

   EncoderBase *data2_m3x3_bool_encoder = EncoderFactory::create(
      data2_trick_base_name + ".m3x3_bool", rti_encoding );

   EncoderBase *data2_ptr_bool_encoder = dynamic_cast< EncoderBase * >( EncoderFactory::create(
      data2_trick_base_name + ".ptr_bool", rti_encoding ) );

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg2;
      msg2 << "EncodingTest::bool_test():" << __LINE__ << "\n"
           << "     data1_bool_encoder: " << data1_bool_encoder->to_string() << "\n"
           << "data1_vec3_bool_encoder: " << data1_vec3_bool_encoder->to_string() << "\n"
           << "data1_m3x3_bool_encoder: " << data1_m3x3_bool_encoder->to_string() << "\n"
           << " data1_ptr_bool_encoder: " << data1_ptr_bool_encoder->to_string() << "\n"
           << "     data2_bool_encoder: " << data2_bool_encoder->to_string() << "\n"
           << "data2_vec3_bool_encoder: " << data2_vec3_bool_encoder->to_string() << "\n"
           << "data2_m3x3_bool_encoder: " << data2_m3x3_bool_encoder->to_string() << "\n"
           << " data2_ptr_bool_encoder: " << data2_ptr_bool_encoder->to_string() << "\n";
      message_publish( MSG_NORMAL, msg2.str().c_str() );
   }

   data2_bool_encoder->decode( data1_bool_encoder->encode() );
   data2_vec3_bool_encoder->decode( data1_vec3_bool_encoder->encode() );
   data2_m3x3_bool_encoder->decode( data1_m3x3_bool_encoder->encode() );

   data1_ptr_bool_encoder->update_before_encode();
   data2_ptr_bool_encoder->decode( data1_ptr_bool_encoder->encode() );
   data2_ptr_bool_encoder->update_after_decode();

   ostringstream compare_msg;
   compare_msg << "(" << encoding_enum_to_string( rti_encoding ) << ") ";

   string explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "bool_data1 == bool_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "bool_data1 != bool_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::bool_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::fixed_record_test(
   string const &data1_trick_base_name,
   FixedRecData &data1,
   string const &data2_trick_base_name,
   FixedRecData &data2 )
{
   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg1;
      msg1 << "========================================\n"
           << "EncodingTest::fixed_record_test():" << __LINE__ << "\n"
           << "BEFORE encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------" << "\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg1.str().c_str() );
   }

   // FixedRecordTest.xml:
   // MainFixedRecObject
   // - field_1_string:  HLAunicodeString
   // - field_2_float64: HLAfloat64LE
   // - field_3_rec:     MainFixedRecord
   //   + MainFixedRecord:  HLAfixedRecord
   //     - elem_1_string:  HLAunicodeString
   //     - elem_2_float64: HLAfloat64LE
   //     - elem_3_record:  SecondaryFixedRecord
   //       + SecondaryFixedRecord: HLAfixedRecord
   //         - element_1_count: HLAinteger32LE
   //         - element_2_name:  HLAunicodeString

   data1.field_1_string  = StringUtilities::mm_strdup_string( "data1.field_1_string" );
   data1.field_2_float64 = 1.0;
   data1.elem_1_string   = StringUtilities::mm_strdup_string( "data1.elem_1_string" );
   data1.elem_2_float64  = 2.0;
   data1.element_1_count = 3;
   data1.element_2_name  = StringUtilities::mm_strdup_string( "data1.element_2_name" );

   data2.field_1_string  = StringUtilities::mm_strdup_string( "data2.field_1_string:test" );
   data2.field_2_float64 = 10.0;
   data2.elem_1_string   = StringUtilities::mm_strdup_string( "data2.elem_1_string:test" );
   data2.elem_2_float64  = 20.0;
   data2.element_1_count = 30;
   data2.element_2_name  = StringUtilities::mm_strdup_string( "data2.element_2_name:test" );

   // Data 1 Encoders --------------------------------------------
   // MainFixedRecObject
   // - field_1_string:  HLAunicodeString
   // - field_2_float64: HLAfloat64LE
   FixedRecordEncoder *top1_fixed_rec_encoder = new FixedRecordEncoder();
   HLAfixedRecord     *top1_fixed_rec         = dynamic_cast< HLAfixedRecord * >( top1_fixed_rec_encoder->data_encoder );

   top1_fixed_rec->appendElementPointer( EncoderFactory::create(
      data1_trick_base_name + ".field_1_string", ENCODING_UNICODE_STRING ) );

   top1_fixed_rec->appendElementPointer( EncoderFactory::create(
      data1_trick_base_name + ".field_2_float64", ENCODING_LITTLE_ENDIAN ) );

   //   + MainFixedRecord:  HLAfixedRecord
   //     - elem_1_string:  HLAunicodeString
   //     - elem_2_float64: HLAfloat64LE
   //     - elem_3_record:  SecondaryFixedRecord
   FixedRecordEncoder *main1_fixed_rec_encoder = new FixedRecordEncoder();
   HLAfixedRecord     *main1_fixed_rec         = dynamic_cast< HLAfixedRecord * >( main1_fixed_rec_encoder->data_encoder );

   main1_fixed_rec->appendElementPointer( EncoderFactory::create(
      data1_trick_base_name + ".elem_1_string", ENCODING_UNICODE_STRING ) );

   main1_fixed_rec->appendElementPointer( EncoderFactory::create(
      data1_trick_base_name + ".elem_2_float64", ENCODING_LITTLE_ENDIAN ) );

   top1_fixed_rec->appendElementPointer( main1_fixed_rec_encoder );

   //       + SecondaryFixedRecord: HLAfixedRecord
   //         - element_1_count: HLAinteger32LE
   //         - element_2_name:  HLAunicodeString
   FixedRecordEncoder *sec1_fixed_rec_encoder = new FixedRecordEncoder();
   HLAfixedRecord     *sec1_fixed_rec         = dynamic_cast< HLAfixedRecord * >( sec1_fixed_rec_encoder->data_encoder );

   sec1_fixed_rec->appendElementPointer( EncoderFactory::create(
      data1_trick_base_name + ".element_1_count", ENCODING_LITTLE_ENDIAN ) );

   sec1_fixed_rec->appendElementPointer( EncoderFactory::create(
      data1_trick_base_name + ".element_2_name", ENCODING_UNICODE_STRING ) );

   main1_fixed_rec->appendElementPointer( sec1_fixed_rec_encoder );

   // Data 2 Encoders --------------------------------------------
   // MainFixedRecObject
   // - field_1_string:  HLAunicodeString
   // - field_2_float64: HLAfloat64LE
   FixedRecordEncoder *top2_fixed_rec_encoder = new FixedRecordEncoder();
   HLAfixedRecord     *top2_fixed_rec         = dynamic_cast< HLAfixedRecord * >( top2_fixed_rec_encoder->data_encoder );

   top2_fixed_rec->appendElementPointer( EncoderFactory::create(
      data2_trick_base_name + ".field_1_string", ENCODING_UNICODE_STRING ) );

   top2_fixed_rec->appendElementPointer( EncoderFactory::create(
      data2_trick_base_name + ".field_2_float64", ENCODING_LITTLE_ENDIAN ) );

   //   + MainFixedRecord:  HLAfixedRecord
   //     - elem_1_string:  HLAunicodeString
   //     - elem_2_float64: HLAfloat64LE
   //     - elem_3_record:  SecondaryFixedRecord
   FixedRecordEncoder *main2_fixed_rec_encoder = new FixedRecordEncoder();
   HLAfixedRecord     *main2_fixed_rec         = dynamic_cast< HLAfixedRecord * >( main2_fixed_rec_encoder->data_encoder );

   main2_fixed_rec->appendElementPointer( EncoderFactory::create(
      data2_trick_base_name + ".elem_1_string", ENCODING_UNICODE_STRING ) );

   main2_fixed_rec->appendElementPointer( EncoderFactory::create(
      data2_trick_base_name + ".elem_2_float64", ENCODING_LITTLE_ENDIAN ) );

   top2_fixed_rec->appendElementPointer( main2_fixed_rec_encoder );

   //       + SecondaryFixedRecord: HLAfixedRecord
   //         - element_1_count: HLAinteger32LE
   //         - element_2_name:  HLAunicodeString
   FixedRecordEncoder *sec2_fixed_rec_encoder = new FixedRecordEncoder();
   HLAfixedRecord     *sec2_fixed_rec         = dynamic_cast< HLAfixedRecord * >( sec2_fixed_rec_encoder->data_encoder );

   sec2_fixed_rec->appendElementPointer( EncoderFactory::create(
      data2_trick_base_name + ".element_1_count", ENCODING_LITTLE_ENDIAN ) );

   sec2_fixed_rec->appendElementPointer( EncoderFactory::create(
      data2_trick_base_name + ".element_2_name", ENCODING_UNICODE_STRING ) );

   main2_fixed_rec->appendElementPointer( sec2_fixed_rec_encoder );

   // Test the encode/decode.
   top1_fixed_rec_encoder->update_before_encode();
#if 0 // Two different techniques.
   VariableLengthData encoded_data = top1_fixed_rec_encoder->encode();
   top2_fixed_rec_encoder->decode( encoded_data );
#else
   top1_fixed_rec_encoder->encode( top1_fixed_rec_encoder->data );
   top2_fixed_rec_encoder->decode( top1_fixed_rec_encoder->data );
#endif
   top2_fixed_rec_encoder->update_after_decode();

   ostringstream compare_msg;
   string        explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "fixed_rec_data1 == fixed_rec_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "fixed_rec_data1 != fixed_rec_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::fixed_record_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}

void EncodingTest::fixed_record_attribute_test(
   std::string const &data1_trick_base_name,
   FixedRecData      &data1,
   std::string const &data2_trick_base_name,
   FixedRecData      &data2 )
{
   data1.field_1_string  = StringUtilities::mm_strdup_string( "data1.field_1_string" );
   data1.field_2_float64 = 1.0;
   data1.elem_1_string   = StringUtilities::mm_strdup_string( "data1.elem_1_string" );
   data1.elem_2_float64  = 2.0;
   data1.element_1_count = 3;
   data1.element_2_name  = StringUtilities::mm_strdup_string( "data1.element_2_name" );

   data2.field_1_string  = StringUtilities::mm_strdup_string( "data2.field_1_string:test" );
   data2.field_2_float64 = 10.0;
   data2.elem_1_string   = StringUtilities::mm_strdup_string( "data2.elem_1_string:test" );
   data2.elem_2_float64  = 20.0;
   data2.element_1_count = 30;
   data2.element_2_name  = StringUtilities::mm_strdup_string( "data2.element_2_name:test" );

   // MainFixedRec
   // - field_1_string:  HLAunicodeString
   // - field_2_float64: HLAfloat64LE
   // - field_3_rec:     MainFixedRecord
   //   + MainFixedRecord:  HLAfixedRecord
   //     - elem_1_string:  HLAunicodeString
   //     - elem_2_float64: HLAfloat64LE
   //     - elem_3_record:  SecondaryFixedRecord
   //       + SecondaryFixedRecord: HLAfixedRecord
   //         - element_1_count: HLAinteger32LE
   //         - element_2_name:  HLAunicodeString

   Attribute *attr_data1       = static_cast< Attribute * >( TMM_declare_var_1d( "TrickHLA::Attribute", 1 ) );
   attr_data1[0].FOM_name      = StringUtilities::mm_strdup_string( "MainFixedRec" );
   attr_data1[0].rti_encoding  = ENCODING_FIXED_RECORD;
   attr_data1[0].element_count = 3;
   attr_data1[0].elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", attr_data1[0].element_count ) );

   attr_data1[0].elements[0].trick_name   = StringUtilities::mm_strdup_string( data1_trick_base_name + ".field_1_string" );
   attr_data1[0].elements[0].rti_encoding = ENCODING_UNICODE_STRING;

   attr_data1[0].elements[1].trick_name   = StringUtilities::mm_strdup_string( data1_trick_base_name + ".field_2_float64" );
   attr_data1[0].elements[1].rti_encoding = ENCODING_LITTLE_ENDIAN;

   // field_3_rec
   attr_data1[0].elements[2].rti_encoding  = ENCODING_FIXED_RECORD;
   attr_data1[0].elements[2].element_count = 3;
   attr_data1[0].elements[2].elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", attr_data1[0].elements[2].element_count ) );

   attr_data1[0].elements[2].elements[0].trick_name   = StringUtilities::mm_strdup_string( data1_trick_base_name + ".elem_1_string" );
   attr_data1[0].elements[2].elements[0].rti_encoding = ENCODING_UNICODE_STRING;

   attr_data1[0].elements[2].elements[1].trick_name   = StringUtilities::mm_strdup_string( data1_trick_base_name + ".elem_2_float64" );
   attr_data1[0].elements[2].elements[1].rti_encoding = ENCODING_LITTLE_ENDIAN;

   // elem_3_record
   attr_data1[0].elements[2].elements[2].rti_encoding  = ENCODING_FIXED_RECORD;
   attr_data1[0].elements[2].elements[2].element_count = 2;
   attr_data1[0].elements[2].elements[2].elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", attr_data1[0].elements[2].elements[2].element_count ) );

   attr_data1[0].elements[2].elements[2].elements[0].trick_name   = StringUtilities::mm_strdup_string( data1_trick_base_name + ".element_1_count" );
   attr_data1[0].elements[2].elements[2].elements[0].rti_encoding = ENCODING_LITTLE_ENDIAN;

   attr_data1[0].elements[2].elements[2].elements[1].trick_name   = StringUtilities::mm_strdup_string( data1_trick_base_name + ".element_2_name" );
   attr_data1[0].elements[2].elements[2].elements[1].rti_encoding = ENCODING_UNICODE_STRING;

   attr_data1[0].initialize_element_encoder();

   Attribute *attr_data2       = static_cast< Attribute * >( TMM_declare_var_1d( "TrickHLA::Attribute", 1 ) );
   attr_data2[0].FOM_name      = StringUtilities::mm_strdup_string( "MainFixedRec" );
   attr_data2[0].rti_encoding  = ENCODING_FIXED_RECORD;
   attr_data2[0].element_count = 3;
   attr_data2[0].elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", attr_data2[0].element_count ) );

   attr_data2[0].elements[0].trick_name   = StringUtilities::mm_strdup_string( data2_trick_base_name + ".field_1_string" );
   attr_data2[0].elements[0].rti_encoding = ENCODING_UNICODE_STRING;

   attr_data2[0].elements[1].trick_name   = StringUtilities::mm_strdup_string( data2_trick_base_name + ".field_2_float64" );
   attr_data2[0].elements[1].rti_encoding = ENCODING_LITTLE_ENDIAN;

   // field_3_rec
   attr_data2[0].elements[2].rti_encoding  = ENCODING_FIXED_RECORD;
   attr_data2[0].elements[2].element_count = 3;
   attr_data2[0].elements[2].elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", attr_data2[0].elements[2].element_count ) );

   attr_data2[0].elements[2].elements[0].trick_name   = StringUtilities::mm_strdup_string( data2_trick_base_name + ".elem_1_string" );
   attr_data2[0].elements[2].elements[0].rti_encoding = ENCODING_UNICODE_STRING;

   attr_data2[0].elements[2].elements[1].trick_name   = StringUtilities::mm_strdup_string( data2_trick_base_name + ".elem_2_float64" );
   attr_data2[0].elements[2].elements[1].rti_encoding = ENCODING_LITTLE_ENDIAN;

   // elem_3_record
   attr_data2[0].elements[2].elements[2].rti_encoding  = ENCODING_FIXED_RECORD;
   attr_data2[0].elements[2].elements[2].element_count = 2;
   attr_data2[0].elements[2].elements[2].elements      = static_cast< RecordElement * >( TMM_declare_var_1d( "TrickHLA::RecordElement", attr_data2[0].elements[2].elements[2].element_count ) );

   attr_data2[0].elements[2].elements[2].elements[0].trick_name   = StringUtilities::mm_strdup_string( data2_trick_base_name + ".element_1_count" );
   attr_data2[0].elements[2].elements[2].elements[0].rti_encoding = ENCODING_LITTLE_ENDIAN;

   attr_data2[0].elements[2].elements[2].elements[1].trick_name   = StringUtilities::mm_strdup_string( data2_trick_base_name + ".element_2_name" );
   attr_data2[0].elements[2].elements[2].elements[1].rti_encoding = ENCODING_UNICODE_STRING;

   attr_data2[0].initialize_element_encoder();

   VariableLengthData encoded_data = attr_data1[0].encode();

   attr_data2[0].decode( encoded_data );

   ostringstream compare_msg;
   string        explanation;

   if ( data1.compare( data2, explanation ) ) {
      compare_msg << "attribute_fixed_rec_data1 == attribute_fixed_rec_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_INFO, compare_msg.str().c_str() );
   } else {
      compare_msg << "attribute_fixed_rec_data1 != attribute_fixed_rec_data2\n";
      if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_1_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
         compare_msg << explanation;
      }
      message_publish( MSG_ERROR, compare_msg.str().c_str() );
   }

   if ( DebugHandler::show( TrickHLA::DEBUG_LEVEL_2_TRACE, TrickHLA::DEBUG_SOURCE_HLA_ENCODERS ) ) {
      ostringstream msg3;
      msg3 << "EncodingTest::fixed_record_attribute_test():" << __LINE__ << "\n"
           << "AFTER encode/decode:\n"
           << "Data1: " << data1.to_string()
           << "-----------------------------\n"
           << "Data2: " << data2.to_string();
      message_publish( MSG_NORMAL, msg3.str().c_str() );
   }
}
