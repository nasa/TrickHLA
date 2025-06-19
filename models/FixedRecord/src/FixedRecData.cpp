/*!
@ingroup encoding
@file models/FixedRecord/src/FixedRecData.cpp
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
@trick_link_dependency{FixedRecord/src/FixedRecData.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2025, --, Initial implementation.}
@revs_end

*/

// System includes.
#include <string>
#include <sstream>

// FixedRecord model includes.
#include "FixedRecord/include/FixedRecData.hh"

using namespace std;
using namespace TrickHLAModel;

FixedRecData::FixedRecData()
   : field_1_string( NULL ),
     field_2_float64( 0.0 ),
     elem_1_string( NULL ),
     elem_2_float64( 0.0 ),
     element_1_count( 0 ),
     element_2_name( NULL )
{
   return;
}

FixedRecData::~FixedRecData()
{
   return;
}

bool FixedRecData::compare(
   FixedRecData const &data,
   string         &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "FixedRecData::compare():" << __LINE__ << "\n";

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

   // - field_1_string:  HLAunicodeString
   int data1_field_1_string_size = strlen( this->field_1_string );
   int data2_field_1_string_size = strlen( data.field_1_string );
   int min_field_1_string_size   = ( data1_field_1_string_size <= data2_field_1_string_size )
                                ? data1_field_1_string_size
                                : data2_field_1_string_size;
   if ( data1_field_1_string_size != data2_field_1_string_size ) {
      msg << "this->field_1_string size (" << data1_field_1_string_size
          << ") != (" << data2_field_1_string_size << ") data.field_1_string size\n";
      equal_values = false;
   } else {
      msg << "this->field_1_string size (" << data1_field_1_string_size
          << ") == (" << data2_field_1_string_size << ") data.field_1_string size\n";
   }
   for ( int i = 0; i < min_field_1_string_size; ++i ) {
      if ( this->field_1_string[i] == data.field_1_string[i] ) {
         msg << "this->field_1_string[" << i << "] ("
             << ( std::isprint( this->field_1_string[i] ) ? this->field_1_string[i] : ' ' )
             << ") == ("
             << ( std::isprint( data.field_1_string[i] ) ? data.field_1_string[i] : ' ' )
             << ") data.field_1_string[" << i << "]\n";
      } else {
         msg << "this->field_1_string[" << i << "] ("
             << ( std::isprint( this->field_1_string[i] ) ? this->field_1_string[i] : ' ' )
             << ") != ("
             << ( std::isprint( data.field_1_string[i] ) ? data.field_1_string[i] : ' ' )
             << ") data.field_1_string[" << i << "]\n";
         equal_values = false;
      }
   }

   // - field_2_float64: HLAfloat64LE
   if ( this->field_2_float64 == data.field_2_float64 ) {
      msg << "this->field_2_float64 (" << this->field_2_float64
            << ") == (" << data.field_2_float64 << ") data.field_2_float64\n";
   } else {
      msg << "this->field_2_float64 (" << this->field_2_float64
            << ") != (" << data.field_2_float64 << ") data.field_2_float64\n";
      equal_values = false;
   }

   // - field_3_rec:     MainFixedRecord
   //   + MainFixedRecord:  HLAfixedRecord
   //     - elem_1_string:  HLAunicodeString
   int data1_elem_1_string_size = strlen( this->elem_1_string );
   int data2_elem_1_string_size = strlen( data.elem_1_string );
   int min_elem_1_string_size   = ( data1_elem_1_string_size <= data2_elem_1_string_size )
                                ? data1_elem_1_string_size
                                : data2_elem_1_string_size;
   if ( data1_elem_1_string_size != data2_elem_1_string_size ) {
      msg << "this->elem_1_string size (" << data1_elem_1_string_size
          << ") != (" << data2_elem_1_string_size << ") data.elem_1_string size\n";
      equal_values = false;
   } else {
      msg << "this->elem_1_string size (" << data1_elem_1_string_size
          << ") == (" << data2_elem_1_string_size << ") data.elem_1_string size\n";
   }
   for ( int i = 0; i < min_elem_1_string_size; ++i ) {
      if ( this->elem_1_string[i] == data.elem_1_string[i] ) {
         msg << "this->elem_1_string[" << i << "] ("
             << ( std::isprint( this->elem_1_string[i] ) ? this->elem_1_string[i] : ' ' )
             << ") == ("
             << ( std::isprint( data.elem_1_string[i] ) ? data.elem_1_string[i] : ' ' )
             << ") data.elem_1_string[" << i << "]\n";
      } else {
         msg << "this->elem_1_string[" << i << "] ("
             << ( std::isprint( this->elem_1_string[i] ) ? this->elem_1_string[i] : ' ' )
             << ") != ("
             << ( std::isprint( data.elem_1_string[i] ) ? data.elem_1_string[i] : ' ' )
             << ") data.elem_1_string[" << i << "]\n";
         equal_values = false;
      }
   }

   // - field_3_rec:     MainFixedRecord
   //   + MainFixedRecord:  HLAfixedRecord
   //     - elem_2_float64: HLAfloat64LE
   if ( this->elem_2_float64 == data.elem_2_float64 ) {
      msg << "this->elem_2_float64 (" << this->elem_2_float64
            << ") == (" << data.elem_2_float64 << ") data.elem_2_float64\n";
   } else {
      msg << "this->elem_2_float64 (" << this->elem_2_float64
            << ") != (" << data.elem_2_float64 << ") data.elem_2_float64\n";
      equal_values = false;
   }

   //     - elem_3_record:  SecondaryFixedRecord
   //       + SecondaryFixedRecord: HLAfixedRecord
   //         - element_1_count: HLAinteger32LE
   if ( this->element_1_count == data.element_1_count ) {
      msg << "this->element_1_count (" << this->element_1_count
            << ") == (" << data.element_1_count << ") data.element_1_count\n";
   } else {
      msg << "this->element_1_count (" << this->element_1_count
            << ") != (" << data.element_1_count << ") data.element_1_count\n";
      equal_values = false;
   }

   //     - elem_3_record:  SecondaryFixedRecord
   //       + SecondaryFixedRecord: HLAfixedRecord
   //         - element_2_name:  HLAunicodeString
   int data1_element_2_name_size = strlen( this->element_2_name );
   int data2_element_2_name_size = strlen( data.element_2_name );
   int min_element_2_name_size   = ( data1_element_2_name_size <= data2_element_2_name_size )
                                ? data1_element_2_name_size
                                : data2_element_2_name_size;
   if ( data1_element_2_name_size != data2_element_2_name_size ) {
      msg << "this->element_2_name size (" << data1_element_2_name_size
          << ") != (" << data2_element_2_name_size << ") data.element_2_name size\n";
      equal_values = false;
   } else {
      msg << "this->element_2_name size (" << data1_element_2_name_size
          << ") == (" << data2_element_2_name_size << ") data.element_2_name size\n";
   }
   for ( int i = 0; i < min_element_2_name_size; ++i ) {
      if ( this->element_2_name[i] == data.element_2_name[i] ) {
         msg << "this->element_2_name[" << i << "] ("
             << ( std::isprint( this->element_2_name[i] ) ? this->element_2_name[i] : ' ' )
             << ") == ("
             << ( std::isprint( data.element_2_name[i] ) ? data.element_2_name[i] : ' ' )
             << ") data.element_2_name[" << i << "]\n";
      } else {
         msg << "this->element_2_name[" << i << "] ("
             << ( std::isprint( this->element_2_name[i] ) ? this->element_2_name[i] : ' ' )
             << ") != ("
             << ( std::isprint( data.element_2_name[i] ) ? data.element_2_name[i] : ' ' )
             << ") data.element_2_name[" << i << "]\n";
         equal_values = false;
      }
   }

   explanation = msg.str();

   return equal_values;
}

string FixedRecData::to_string()
{
   ostringstream msg;
   msg << "FixedRecData::to_string():" << __LINE__ << std::endl
       << "field_1_string:"
       << ((field_1_string != NULL ) ? field_1_string : "NULL") << std::endl
       << "field_2_float64:" << field_2_float64 << std::endl
       << "elem_1_string:"
       << ((elem_1_string != NULL ) ? elem_1_string : "NULL") << std::endl
       << "elem_2_float64:" << elem_2_float64 << std::endl
       << "element_1_count:" << element_1_count << std::endl
       << "element_2_name:"
       << ((element_2_name != NULL ) ? element_2_name : "NULL") << std::endl;
   return msg.str();
}
