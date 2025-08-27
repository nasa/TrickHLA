/*!
@ingroup encoding
@file models/encoding/src/WStringData.cpp
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
@trick_link_dependency{encoding/src/WStringData.cpp}

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

// TrickHLA include files.
#include "TrickHLA/StringUtilities.hh"

// Model include files.
#include "encoding/include/WStringData.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
WStringData::WStringData()
   : WStringData( 0 )
{
   return;
}

WStringData::WStringData(
   unsigned int const offset )
{
   // ISSUE: MemoryManager:ERROR:size of type "std::wstring" not found.

   int value = 1 + offset;
   _wstring  = L"wstring-" + std::to_wstring( value ); // cppcheck-suppress [useInitializationList]

   for ( int i = 0; i < 3; ++i ) {
      value           = i + 1 + offset;
      vec3_wstring[i] = L"wstring-" + std::to_wstring( value );
   }

   int cnt = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         value                  = cnt + offset;
         m3x3_wstring[row][col] = L"wstring-" + std::to_wstring( value );
         ++cnt;
      }
   }

   int const ptr_wstring_size = 5 + ( ( offset < 10 ) ? offset : 10 );

   // ISSUE: MemoryManager:ERROR:size of type "std::wstring" not found.
   ptr_wstring = static_cast< std::wstring * >( TMM_declare_var_1d( "std::wstring", ptr_wstring_size ) );
   for ( int i = 0; i < ptr_wstring_size; ++i ) {
      value          = i + 1 + offset;
      ptr_wstring[i] = L"wstring-" + std::to_wstring( value );
   }
}

/*!
 * @job_class{shutdown}
 */
WStringData::~WStringData()
{
   return;
}

bool WStringData::compare(
   WStringData const &data,
   string            &explanation )
{
   bool equal_values = true;

   wstringstream msg;
   msg << "WStringData::compare():" << __LINE__ << endl;

   if ( this->_wstring == data._wstring ) {
      msg << "this->string (" << this->_wstring << ") == (" << data._wstring << ") data.string" << endl;
   } else {
      msg << "this->string (" << this->_wstring << ") != (" << data._wstring << ") data.string" << endl;
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_wstring[i] == data.vec3_wstring[i] ) {
         msg << "this->vec3_wstring[" << i << "] (" << this->vec3_wstring[i]
             << ") == (" << data.vec3_wstring[i] << ") data.vec3_wstring[" << i << "]" << endl;
      } else {
         msg << "this->vec3_wstring[" << i << "] (" << this->vec3_wstring[i]
             << ") != (" << data.vec3_wstring[i] << ") data.vec3_wstring[" << i << "]" << endl;
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_wstring[row][col] == data.m3x3_wstring[row][col] ) {
            msg << "this->m3x3_wstring[" << row << "][" << col << "] (" << this->m3x3_wstring[row][col]
                << ") == (" << data.m3x3_wstring[row][col] << ") data.m3x3_wstring[" << row << "][" << col << "]" << endl;
         } else {
            msg << "this->m3x3_wstring[" << row << "][" << col << "] (" << this->m3x3_wstring[row][col]
                << ") != (" << data.m3x3_wstring[row][col] << ") data.m3x3_wstring[" << row << "][" << col << "]" << endl;
            equal_values = false;
         }
      }
   }

   int data1_ptr_wstring_size = get_size( this->ptr_wstring );
   int data2_ptr_wstring_size = get_size( data.ptr_wstring );
   int min_ptr_wstring_size   = ( data1_ptr_wstring_size <= data2_ptr_wstring_size )
                                   ? data1_ptr_wstring_size
                                   : data2_ptr_wstring_size;
   if ( data1_ptr_wstring_size != data2_ptr_wstring_size ) {
      msg << "this->ptr_wstring size (" << data1_ptr_wstring_size
          << ") != (" << data2_ptr_wstring_size << ") data.ptr_wstring size" << endl;
      equal_values = false;
   } else {
      msg << "this->ptr_wstring size (" << data1_ptr_wstring_size
          << ") == (" << data2_ptr_wstring_size << ") data.ptr_wstring size" << endl;
   }
   for ( int i = 0; i < min_ptr_wstring_size; ++i ) {
      if ( this->ptr_wstring[i] == data.ptr_wstring[i] ) {
         msg << "this->ptr_wstring[" << i << "] (" << this->ptr_wstring[i]
             << ") == (" << data.ptr_wstring[i] << ") data.ptr_wstring[" << i << "]" << endl;
      } else {
         msg << "this->ptr_wstring[" << i << "] (" << this->ptr_wstring[i]
             << ") != (" << data.ptr_wstring[i] << ") data.ptr_wstring[" << i << "]" << endl;
         equal_values = false;
      }
   }

   // Convert from wide-string.
   StringUtilities::to_string( explanation, msg.str() );

   return equal_values;
}

string WStringData::to_string()
{
   string msg;
   StringUtilities::to_string( msg, this->to_wstring() );
   return msg;
}

wstring WStringData::to_wstring()
{
   wstringstream msg;
   msg << "WStringData::to_wstring():" << __LINE__ << endl
       << "string:" << _wstring << endl;

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_wstring[" << i << "]:" << vec3_wstring[i] << " ";
   }
   msg << endl;

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_wstring[" << row << "][" << col << "]:" << m3x3_wstring[row][col] << " ";
      }
   }
   msg << endl;

   int ptr_wstring_size = get_size( ptr_wstring );
   msg << "ptr_wstring size:" << ptr_wstring_size << endl;
   for ( int i = 0; i < ptr_wstring_size; ++i ) {
      msg << "ptr_wstring[" << i << "]:" << ptr_wstring[i] << " ";
   }
   msg << endl;

   return msg.str();
}
