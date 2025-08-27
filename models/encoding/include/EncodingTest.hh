/*!
@ingroup encoding
@file models/encoding/include/EncodingTest.hh
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

@trick_parse{everything}

@python_module{TrickHLAModel}

@tldh
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
@trick_link_dependency{encoding/src/ULongData.cpp}
@trick_link_dependency{encoding/src/StringData.cpp}
@trick_link_dependency{encoding/src/WCharData.cpp}
@trick_link_dependency{encoding/src/WStringData.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_ENCODING_TEST_HH
#define TRICKHLA_MODEL_ENCODING_TEST_HH

// System includes.
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"

// TrickHLA includes.
#include "TrickHLA/Types.hh"

#include "BoolData.hh"
#include "CharData.hh"
#include "Float32Data.hh"
#include "Float64Data.hh"
#include "Int16Data.hh"
#include "Int32Data.hh"
#include "Int64Data.hh"
#include "LongData.hh"
#include "StringData.hh"
#include "UInt16Data.hh"
#include "UInt32Data.hh"
#include "UInt64Data.hh"
#include "ULongData.hh"
#include "WCharData.hh"
#include "WStringData.hh"

#include "../../FixedRecord/include/FixedRecData.hh"

namespace TrickHLAModel
{

class EncodingTest
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__EncodingTest();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel EncodingTest class. */
   EncodingTest();
   /*! @brief Destructor for the TrickHLAModel EncodingTest class. */
   virtual ~EncodingTest();

   void char_test( std::string const           &data1_trick_base_name,
                   CharData                    &data1,
                   std::string const           &data2_trick_base_name,
                   CharData                    &data2,
                   TrickHLA::EncodingEnum const rti_encoding );

   void string_test( std::string const           &data1_trick_base_name,
                     StringData                  &data1,
                     std::string const           &data2_trick_base_name,
                     StringData                  &data2,
                     TrickHLA::EncodingEnum const rti_encoding );

   void wchar_test( std::string const           &data1_trick_base_name,
                    WCharData                   &data1,
                    std::string const           &data2_trick_base_name,
                    WCharData                   &data2,
                    TrickHLA::EncodingEnum const rti_encoding );

   void wstring_test( std::string const           &data1_trick_base_name,
                      WStringData                 &data1,
                      std::string const           &data2_trick_base_name,
                      WStringData                 &data2,
                      TrickHLA::EncodingEnum const rti_encoding );

   void int16_test( std::string const           &data1_trick_base_name,
                    Int16Data                   &data1,
                    std::string const           &data2_trick_base_name,
                    Int16Data                   &data2,
                    TrickHLA::EncodingEnum const rti_encoding );

   void uint16_test( std::string const           &data1_trick_base_name,
                     UInt16Data                  &data1,
                     std::string const           &data2_trick_base_name,
                     UInt16Data                  &data2,
                     TrickHLA::EncodingEnum const rti_encoding );

   void int32_test( std::string const           &data1_trick_base_name,
                    Int32Data                   &data1,
                    std::string const           &data2_trick_base_name,
                    Int32Data                   &data2,
                    TrickHLA::EncodingEnum const rti_encoding );

   void uint32_test( std::string const           &data1_trick_base_name,
                     UInt32Data                  &data1,
                     std::string const           &data2_trick_base_name,
                     UInt32Data                  &data2,
                     TrickHLA::EncodingEnum const rti_encoding );

   void int64_test( std::string const           &data1_trick_base_name,
                    Int64Data                   &data1,
                    std::string const           &data2_trick_base_name,
                    Int64Data                   &data2,
                    TrickHLA::EncodingEnum const rti_encoding );

   void uint64_test( std::string const           &data1_trick_base_name,
                     UInt64Data                  &data1,
                     std::string const           &data2_trick_base_name,
                     UInt64Data                  &data2,
                     TrickHLA::EncodingEnum const rti_encoding );

   void long_test( std::string const           &data1_trick_base_name,
                   LongData                    &data1,
                   std::string const           &data2_trick_base_name,
                   LongData                    &data2,
                   TrickHLA::EncodingEnum const rti_encoding );

   void ulong_test( std::string const           &data1_trick_base_name,
                    ULongData                   &data1,
                    std::string const           &data2_trick_base_name,
                    ULongData                   &data2,
                    TrickHLA::EncodingEnum const rti_encoding );

   void float32_test( std::string const           &data1_trick_base_name,
                      Float32Data                 &data1,
                      std::string const           &data2_trick_base_name,
                      Float32Data                 &data2,
                      TrickHLA::EncodingEnum const rti_encoding );

   void float64_test( std::string const           &data1_trick_base_name,
                      Float64Data                 &data1,
                      std::string const           &data2_trick_base_name,
                      Float64Data                 &data2,
                      TrickHLA::EncodingEnum const rti_encoding );

   void bool_test( std::string const           &data1_trick_base_name,
                   BoolData                    &data1,
                   std::string const           &data2_trick_base_name,
                   BoolData                    &data2,
                   TrickHLA::EncodingEnum const rti_encoding );

   void fixed_record_test( std::string const &data1_trick_base_name,
                           FixedRecData      &data1,
                           std::string const &data2_trick_base_name,
                           FixedRecData      &data2 );

   void fixed_record_attribute_test( std::string const &data1_trick_base_name,
                                     FixedRecData      &data1,
                                     std::string const &data2_trick_base_name,
                                     FixedRecData      &data2 );

   void float64_logical_time_test( std::string const &data1_trick_base_name,
                                   Float64Data       &data1,
                                   std::string const &data2_trick_base_name,
                                   Float64Data       &data2 );
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_ENCODING_TEST_HH: Do NOT put anything after this line!
