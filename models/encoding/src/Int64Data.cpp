/*!
@ingroup encoding
@file models/encoding/src/Int64Data.cpp
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
@trick_link_dependency{encoding/src/Int64Data.cpp}
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
#include "encoding/include/Int64Data.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
Int64Data::Int64Data()
   : Int64Data( 0 )
{
   return;
}

Int64Data::Int64Data(
   unsigned long long const offset )
{
   i64 = 1 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_i64[i] = i + 1 + offset;
   }

   long long value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_i64[row][col] = value + offset;
         ++value;
      }
   }

   int const ptr_i64_size = 5 + ( ( offset < 10 ) ? offset : 10 );

   ptr_i64 = static_cast< long long * >( TMM_declare_var_1d( "long long", ptr_i64_size ) );
   for ( int i = 0; i < ptr_i64_size; ++i ) {
      ptr_i64[i] = i + 1 + offset;
   }
}

/*!
 * @job_class{shutdown}
 */
Int64Data::~Int64Data()
{
   return;
}

bool Int64Data::compare(
   Int64Data const &data,
   string          &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "Int64Data::compare():" << __LINE__ << "\n";

   if ( this->i64 == data.i64 ) {
      msg << "this->i64 (" << this->i64 << ") == (" << data.i64 << ") data.i64\n";
   } else {
      msg << "this->i64 (" << this->i64 << ") != (" << data.i64 << ") data.i64\n";
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_i64[i] == data.vec3_i64[i] ) {
         msg << "this->vec3_i64[" << i << "] (" << this->vec3_i64[i]
             << ") == (" << data.vec3_i64[i] << ") data.vec3_i64[" << i << "]\n";
      } else {
         msg << "this->vec3_i64[" << i << "] (" << this->vec3_i64[i]
             << ") != (" << data.vec3_i64[i] << ") data.vec3_i64[" << i << "]\n";
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_i64[row][col] == data.m3x3_i64[row][col] ) {
            msg << "this->m3x3_i64[" << row << "][" << col << "] (" << this->m3x3_i64[row][col]
                << ") == (" << data.m3x3_i64[row][col] << ") data.m3x3_i64[" << row << "][" << col << "]\n";
         } else {
            msg << "this->m3x3_i64[" << row << "][" << col << "] (" << this->m3x3_i64[row][col]
                << ") != (" << data.m3x3_i64[row][col] << ") data.m3x3_i64[" << row << "][" << col << "]\n";
            equal_values = false;
         }
      }
   }

   int data1_ptr_i64_size = get_size( this->ptr_i64 );
   int data2_ptr_i64_size = get_size( data.ptr_i64 );
   if ( data1_ptr_i64_size != data2_ptr_i64_size ) {
      msg << "this->ptr_i64 size (" << data1_ptr_i64_size
          << ") != (" << data2_ptr_i64_size << ") data.ptr_i64 size\n";
      equal_values = false;
   } else {
      msg << "this->ptr_i64 size (" << data1_ptr_i64_size
          << ") == (" << data2_ptr_i64_size << ") data.ptr_i64 size\n";

      for ( int i = 0; i < data1_ptr_i64_size; ++i ) {
         if ( this->ptr_i64[i] == data.ptr_i64[i] ) {
            msg << "this->ptr_i64[" << i << "] (" << this->ptr_i64[i]
                << ") == (" << data.ptr_i64[i] << ") data.ptr_i64[" << i << "]\n";
         } else {
            msg << "this->ptr_i64[" << i << "] (" << this->ptr_i64[i]
                << ") != (" << data.ptr_i64[i] << ") data.ptr_i64[" << i << "]\n";
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string Int64Data::to_string()
{
   ostringstream msg;
   msg << "Int64Data::to_string():" << __LINE__ << "\n"
       << "i64:" << i64 << "\n";

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_i64[" << i << "]:" << vec3_i64[i] << " ";
   }
   msg << "\n";

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_i64[" << row << "][" << col << "]:" << m3x3_i64[row][col] << " ";
      }
   }
   msg << "\n";

   int ptr_i64_size = get_size( ptr_i64 );
   msg << "ptr_i64 size:" << ptr_i64_size << "\n";
   for ( int i = 0; i < ptr_i64_size; ++i ) {
      msg << "ptr_i64[" << i << "]:" << ptr_i64[i] << " ";
   }
   msg << "\n";

   return msg.str();
}
