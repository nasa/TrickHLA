/*!
@ingroup encoding
@file models/encoding/src/WCharData.cpp
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
@trick_link_dependency{encoding/src/WCharData.cpp}
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, May 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <sstream>
#include <string>

// Trick include files.
#include "trick/memorymanager_c_intf.h"

// Model include files.
#include "encoding/include/WCharData.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
WCharData::WCharData()
   : WCharData( 0 )
{
   return;
}

WCharData::WCharData(
   unsigned int const offset )
{
   _wchar = 1 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_wchar[i] = i + 1 + offset;
   }

   int value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_wchar[row][col] = value + offset;
         ++value;
      }
   }

   int const ptr_wchar_size = 5 + ( ( offset < 10 ) ? offset : 10 );

   ptr_wchar = static_cast< wchar_t * >( TMM_declare_var_1d( "wchar_t", ptr_wchar_size ) );
   for ( int i = 0; i < ptr_wchar_size; ++i ) {
      ptr_wchar[i] = i + 1 + offset;
   }
}

/*!
 * @job_class{shutdown}
 */
WCharData::~WCharData()
{
   return;
}

bool WCharData::compare(
   WCharData const &data,
   string          &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "WCharData::compare():" << __LINE__ << "\n";

   if ( this->_wchar == data._wchar ) {
      msg << "this->char (" << (int)this->_wchar << ") == (" << (int)data._wchar << ") data.char\n";
   } else {
      msg << "this->char (" << (int)this->_wchar << ") != (" << (int)data._wchar << ") data.char\n";
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_wchar[i] == data.vec3_wchar[i] ) {
         msg << "this->vec3_wchar[" << i << "] (" << (int)this->vec3_wchar[i]
             << ") == (" << (int)data.vec3_wchar[i] << ") data.vec3_wchar[" << i << "]\n";
      } else {
         msg << "this->vec3_wchar[" << i << "] (" << (int)this->vec3_wchar[i]
             << ") != (" << (int)data.vec3_wchar[i] << ") data.vec3_wchar[" << i << "]\n";
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_wchar[row][col] == data.m3x3_wchar[row][col] ) {
            msg << "this->m3x3_wchar[" << row << "][" << col << "] (" << (int)this->m3x3_wchar[row][col]
                << ") == (" << (int)data.m3x3_wchar[row][col] << ") data.m3x3_wchar[" << row << "][" << col << "]\n";
         } else {
            msg << "this->m3x3_wchar[" << row << "][" << col << "] (" << (int)this->m3x3_wchar[row][col]
                << ") != (" << (int)data.m3x3_wchar[row][col] << ") data.m3x3_wchar[" << row << "][" << col << "]\n";
            equal_values = false;
         }
      }
   }

   int data1_ptr_wchar_size = get_size( this->ptr_wchar );
   int data2_ptr_wchar_size = get_size( data.ptr_wchar );
   int min_ptr_wchar_size   = ( data1_ptr_wchar_size <= data2_ptr_wchar_size )
                                 ? data1_ptr_wchar_size
                                 : data2_ptr_wchar_size;
   if ( data1_ptr_wchar_size != data2_ptr_wchar_size ) {
      msg << "this->ptr_wchar size (" << data1_ptr_wchar_size
          << ") != (" << data2_ptr_wchar_size << ") data.ptr_wchar size\n";
      equal_values = false;
   } else {
      msg << "this->ptr_wchar size (" << data1_ptr_wchar_size
          << ") == (" << data2_ptr_wchar_size << ") data.ptr_wchar size\n";
   }
   for ( int i = 0; i < min_ptr_wchar_size; ++i ) {
      if ( this->ptr_wchar[i] == data.ptr_wchar[i] ) {
         msg << "this->ptr_wchar[" << i << "] (" << (int)this->ptr_wchar[i]
             << ") == (" << (int)data.ptr_wchar[i] << ") data.ptr_wchar[" << i << "]\n";
      } else {
         msg << "this->ptr_wchar[" << i << "] (" << (int)this->ptr_wchar[i]
             << ") != (" << (int)data.ptr_wchar[i] << ") data.ptr_wchar[" << i << "]\n";
         equal_values = false;
      }
   }

   explanation = msg.str();

   return equal_values;
}

string WCharData::to_string()
{
   ostringstream msg;
   msg << "WCharData::to_string():" << __LINE__ << "\n"
       << "wchar:" << (int)_wchar << "\n";

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_wchar[" << i << "]:" << (int)vec3_wchar[i] << " ";
   }
   msg << "\n";

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_wchar[" << row << "][" << col << "]:" << (int)m3x3_wchar[row][col] << " ";
      }
   }
   msg << "\n";

   int ptr_wchar_size = get_size( ptr_wchar );
   msg << "ptr_wchar size:" << ptr_wchar_size << "\n";
   for ( int i = 0; i < ptr_wchar_size; ++i ) {
      msg << "ptr_wchar[" << i << "]:" << (int)ptr_wchar[i] << " ";
   }
   msg << "\n";

   return msg.str();
}
