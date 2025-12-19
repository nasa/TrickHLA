/*!
@ingroup encoding
@file models/encoding/src/ULongData.cpp
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
@trick_link_dependency{encoding/src/ULongData.cpp}
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}

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

// Model include files.
#include "encoding/include/ULongData.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
ULongData::ULongData()
   : ULongData( 0 )
{
   return;
}

ULongData::ULongData(
   unsigned long const offset )
{
   _ulong = 1 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_ulong[i] = i + 1 + offset;
   }

   int value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_ulong[row][col] = value + offset;
         ++value;
      }
   }

   int const ptr_ulong_size = 5 + ( ( offset < 10 ) ? offset : 10 );

   ptr_ulong = static_cast< unsigned long * >( TMM_declare_var_1d( "unsigned long", ptr_ulong_size ) );
   for ( int i = 0; i < ptr_ulong_size; ++i ) {
      ptr_ulong[i] = i + 1 + offset;
   }
}

/*!
 * @job_class{shutdown}
 */
ULongData::~ULongData()
{
   return;
}

bool ULongData::compare(
   ULongData const &data,
   string          &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "ULongData::compare():" << __LINE__ << endl;

   if ( this->_ulong == data._ulong ) {
      msg << "this->long (" << this->_ulong << ") == (" << data._ulong << ") data.long" << endl;
   } else {
      msg << "this->long (" << this->_ulong << ") != (" << data._ulong << ") data.long" << endl;
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_ulong[i] == data.vec3_ulong[i] ) {
         msg << "this->vec3_ulong[" << i << "] (" << this->vec3_ulong[i]
             << ") == (" << data.vec3_ulong[i] << ") data.vec3_ulong[" << i << "]" << endl;
      } else {
         msg << "this->vec3_ulong[" << i << "] (" << this->vec3_ulong[i]
             << ") != (" << data.vec3_ulong[i] << ") data.vec3_ulong[" << i << "]" << endl;
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_ulong[row][col] == data.m3x3_ulong[row][col] ) {
            msg << "this->m3x3_ulong[" << row << "][" << col << "] (" << this->m3x3_ulong[row][col]
                << ") == (" << data.m3x3_ulong[row][col] << ") data.m3x3_ulong[" << row << "][" << col << "]" << endl;
         } else {
            msg << "this->m3x3_ulong[" << row << "][" << col << "] (" << this->m3x3_ulong[row][col]
                << ") != (" << data.m3x3_ulong[row][col] << ") data.m3x3_ulong[" << row << "][" << col << "]" << endl;
            equal_values = false;
         }
      }
   }

   int data1_ptr_ulong_size = get_size( this->ptr_ulong );
   int data2_ptr_ulong_size = get_size( data.ptr_ulong );
   if ( data1_ptr_ulong_size != data2_ptr_ulong_size ) {
      msg << "this->ptr_ulong size (" << data1_ptr_ulong_size
          << ") != (" << data2_ptr_ulong_size << ") data.ptr_ulong size" << endl;
      equal_values = false;
   } else {
      msg << "this->ptr_ulong size (" << data1_ptr_ulong_size
          << ") == (" << data2_ptr_ulong_size << ") data.ptr_ulong size" << endl;

      for ( int i = 0; i < data1_ptr_ulong_size; ++i ) {
         if ( this->ptr_ulong[i] == data.ptr_ulong[i] ) {
            msg << "this->ptr_ulong[" << i << "] (" << this->ptr_ulong[i]
                << ") == (" << data.ptr_ulong[i] << ") data.ptr_ulong[" << i << "]" << endl;
         } else {
            msg << "this->ptr_ulong[" << i << "] (" << this->ptr_ulong[i]
                << ") != (" << data.ptr_ulong[i] << ") data.ptr_ulong[" << i << "]" << endl;
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string ULongData::to_string()
{
   ostringstream msg;
   msg << "ULongData::to_string():" << __LINE__ << endl
       << "long:" << _ulong << endl;

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_ulong[" << i << "]:" << vec3_ulong[i] << " ";
   }
   msg << endl;

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_ulong[" << row << "][" << col << "]:" << m3x3_ulong[row][col] << " ";
      }
   }
   msg << endl;

   int ptr_ulong_size = get_size( ptr_ulong );
   msg << "ptr_ulong size:" << ptr_ulong_size << endl;
   for ( int i = 0; i < ptr_ulong_size; ++i ) {
      msg << "ptr_ulong[" << i << "]:" << ptr_ulong[i] << " ";
   }
   msg << endl;

   return msg.str();
}
