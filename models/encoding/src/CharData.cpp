/*!
@ingroup encoding
@file models/encoding/src/CharData.cpp
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
@trick_link_dependency{encoding/src/CharData.cpp}
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cctype>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/memorymanager_c_intf.h"

// TrickHLA include files.
#include "TrickHLA/StringUtilities.hh"

// Model include files.
#include "encoding/include/CharData.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
CharData::CharData()
   : CharData( 0 )
{
   return;
}

CharData::CharData(
   unsigned int const offset )
{
   _char = 1 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_char[i] = i + 1 + offset;
   }

   int value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_char[row][col] = value + offset;
         ++value;
      }
   }

   string str = "str-" + std::to_string( 1 + offset );
#if 1
   ptr_char = StringUtilities::mm_strdup_string( str );
#else
   int const ptr_char_size = str.size() + 1;

   ptr_char = static_cast< char * >( TMM_declare_var_1d( "char", ptr_char_size ) );
   for ( int i = 0; i < str.size(); ++i ) {
      ptr_char[i] = str.at( i );
   }
   ptr_char[ptr_char_size - 1] = '\0'; // Must be NULL terminated
#endif
}

/*!
 * @job_class{shutdown}
 */
CharData::~CharData()
{
   return;
}

bool CharData::compare(
   CharData const &data,
   string         &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "CharData::compare():" << __LINE__ << endl;

   if ( this->_char == data._char ) {
      msg << "this->char (" << (int)this->_char << ") == (" << (int)data._char << ") data.char" << endl;
   } else {
      msg << "this->char (" << (int)this->_char << ") != (" << (int)data._char << ") data.char" << endl;
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_char[i] == data.vec3_char[i] ) {
         msg << "this->vec3_char[" << i << "] (" << (int)this->vec3_char[i]
             << ") == (" << (int)data.vec3_char[i] << ") data.vec3_char[" << i << "]" << endl;
      } else {
         msg << "this->vec3_char[" << i << "] (" << (int)this->vec3_char[i]
             << ") != (" << (int)data.vec3_char[i] << ") data.vec3_char[" << i << "]" << endl;
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_char[row][col] == data.m3x3_char[row][col] ) {
            msg << "this->m3x3_char[" << row << "][" << col << "] (" << (int)this->m3x3_char[row][col]
                << ") == (" << (int)data.m3x3_char[row][col] << ") data.m3x3_char[" << row << "][" << col << "]" << endl;
         } else {
            msg << "this->m3x3_char[" << row << "][" << col << "] (" << (int)this->m3x3_char[row][col]
                << ") != (" << (int)data.m3x3_char[row][col] << ") data.m3x3_char[" << row << "][" << col << "]" << endl;
            equal_values = false;
         }
      }
   }

   int data1_ptr_char_size = strlen( this->ptr_char ); // flawfinder: ignore
   int data2_ptr_char_size = strlen( data.ptr_char );  // flawfinder: ignore
   int min_ptr_char_size   = ( data1_ptr_char_size <= data2_ptr_char_size )
                                ? data1_ptr_char_size
                                : data2_ptr_char_size;
   if ( data1_ptr_char_size != data2_ptr_char_size ) {
      msg << "this->ptr_char size (" << data1_ptr_char_size
          << ") != (" << data2_ptr_char_size << ") data.ptr_char size" << endl;
      equal_values = false;
   } else {
      msg << "this->ptr_char size (" << data1_ptr_char_size
          << ") == (" << data2_ptr_char_size << ") data.ptr_char size" << endl;
   }
   for ( int i = 0; i < min_ptr_char_size; ++i ) {
      if ( this->ptr_char[i] == data.ptr_char[i] ) {
         msg << "this->ptr_char[" << i << "] ("
             << ( std::isprint( this->ptr_char[i] ) ? this->ptr_char[i] : ' ' )
             << ") == ("
             << ( std::isprint( data.ptr_char[i] ) ? data.ptr_char[i] : ' ' )
             << ") data.ptr_char[" << i << "]" << endl;
      } else {
         msg << "this->ptr_char[" << i << "] ("
             << ( std::isprint( this->ptr_char[i] ) ? this->ptr_char[i] : ' ' )
             << ") != ("
             << ( std::isprint( data.ptr_char[i] ) ? data.ptr_char[i] : ' ' )
             << ") data.ptr_char[" << i << "]" << endl;
         equal_values = false;
      }
   }

   explanation = msg.str();

   return equal_values;
}

string CharData::to_string()
{
   ostringstream msg;
   msg << "CharData::to_string():" << __LINE__ << endl
       << "char:" << (int)_char << endl;

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_char[" << i << "]:" << (int)vec3_char[i] << " ";
   }
   msg << endl;

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_char[" << row << "][" << col << "]:" << (int)m3x3_char[row][col] << " ";
      }
   }
   msg << endl;

   int ptr_char_size = get_size( ptr_char );
   msg << "ptr_char size:" << ptr_char_size << endl;
   for ( int i = 0; i < ptr_char_size; ++i ) {
      msg << "ptr_char[" << i << "]:" << (int)ptr_char[i] << " ";
   }
   msg << endl;

   return msg.str();
}
