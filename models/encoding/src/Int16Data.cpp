/*!
@ingroup encoding
@file models/encoding/src/Int16Data.cpp
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
@trick_link_dependency{encoding/src/Int16Data.cpp}
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
#include "encoding/include/Int16Data.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
Int16Data::Int16Data()
   : Int16Data( 0 )
{
   return;
}

Int16Data::Int16Data(
   unsigned short const offset )
{
   i16 = 1 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_i16[i] = i + 1 + offset;
   }

   short value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_i16[row][col] = value + offset;
         ++value;
      }
   }

   int const ptr_i16_size = 5 + ( ( offset < 10 ) ? offset : 10 );

   ptr_i16 = static_cast< short * >( TMM_declare_var_1d( "short", ptr_i16_size ) );
   for ( int i = 0; i < ptr_i16_size; ++i ) {
      ptr_i16[i] = i + 1 + offset;
   }
}

/*!
 * @job_class{shutdown}
 */
Int16Data::~Int16Data()
{
   return;
}

bool Int16Data::compare(
   Int16Data const &data,
   string          &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "Int16Data::compare():" << __LINE__ << endl;

   if ( this->i16 == data.i16 ) {
      msg << "this->i16 (" << this->i16 << ") == (" << data.i16 << ") data.i16" << endl;
   } else {
      msg << "this->i16 (" << this->i16 << ") != (" << data.i16 << ") data.i16" << endl;
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_i16[i] == data.vec3_i16[i] ) {
         msg << "this->vec3_i16[" << i << "] (" << this->vec3_i16[i]
             << ") == (" << data.vec3_i16[i] << ") data.vec3_i16[" << i << "]" << endl;
      } else {
         msg << "this->vec3_i16[" << i << "] (" << this->vec3_i16[i]
             << ") != (" << data.vec3_i16[i] << ") data.vec3_i16[" << i << "]" << endl;
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_i16[row][col] == data.m3x3_i16[row][col] ) {
            msg << "this->m3x3_i16[" << row << "][" << col << "] (" << this->m3x3_i16[row][col]
                << ") == (" << data.m3x3_i16[row][col] << ") data.m3x3_i16[" << row << "][" << col << "]" << endl;
         } else {
            msg << "this->m3x3_i16[" << row << "][" << col << "] (" << this->m3x3_i16[row][col]
                << ") != (" << data.m3x3_i16[row][col] << ") data.m3x3_i16[" << row << "][" << col << "]" << endl;
            equal_values = false;
         }
      }
   }

   int data1_ptr_i16_size = get_size( this->ptr_i16 );
   int data2_ptr_i16_size = get_size( data.ptr_i16 );
   if ( data1_ptr_i16_size != data2_ptr_i16_size ) {
      msg << "this->ptr_i16 size (" << data1_ptr_i16_size
          << ") != (" << data2_ptr_i16_size << ") data.ptr_i16 size" << endl;
      equal_values = false;
   } else {
      msg << "this->ptr_i16 size (" << data1_ptr_i16_size
          << ") == (" << data2_ptr_i16_size << ") data.ptr_i16 size" << endl;

      for ( int i = 0; i < data1_ptr_i16_size; ++i ) {
         if ( this->ptr_i16[i] == data.ptr_i16[i] ) {
            msg << "this->ptr_i16[" << i << "] (" << this->ptr_i16[i]
                << ") == (" << data.ptr_i16[i] << ") data.ptr_i16[" << i << "]" << endl;
         } else {
            msg << "this->ptr_i16[" << i << "] (" << this->ptr_i16[i]
                << ") != (" << data.ptr_i16[i] << ") data.ptr_i16[" << i << "]" << endl;
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string Int16Data::to_string()
{
   ostringstream msg;
   msg << "Int16Data::to_string():" << __LINE__ << endl
       << "i16:" << i16 << endl;

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_i16[" << i << "]:" << vec3_i16[i] << " ";
   }
   msg << endl;

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_i16[" << row << "][" << col << "]:" << m3x3_i16[row][col] << " ";
      }
   }
   msg << endl;

   int ptr_i16_size = get_size( ptr_i16 );
   msg << "ptr_i16 size:" << ptr_i16_size << endl;
   for ( int i = 0; i < ptr_i16_size; ++i ) {
      msg << "ptr_i16[" << i << "]:" << ptr_i16[i] << " ";
   }
   msg << endl;

   return msg.str();
}
