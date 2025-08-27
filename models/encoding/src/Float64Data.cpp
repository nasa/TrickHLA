/*!
@ingroup encoding
@file models/encoding/src/Float64Data.cpp
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
@trick_link_dependency{encoding/src/Float64Data.cpp}
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
#include "encoding/include/Float64Data.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
Float64Data::Float64Data()
   : Float64Data( 0 )
{
   return;
}

Float64Data::Float64Data(
   unsigned int const offset )
{
   f64 = 1.0 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_f64[i] = i + 1.0 + offset;
   }

   double value = 1.0;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_f64[row][col] = value + offset;
         value += 1.0;
      }
   }

   int const ptr_f64_size = 5 + ( ( offset < 10 ) ? offset : 10 );

   ptr_f64 = static_cast< double * >( TMM_declare_var_1d( "double", ptr_f64_size ) );
   for ( int i = 0; i < ptr_f64_size; ++i ) {
      ptr_f64[i] = i + 1.0 + offset;
   }
}

/*!
 * @job_class{shutdown}
 */
Float64Data::~Float64Data()
{
   return;
}

bool Float64Data::compare(
   Float64Data const &data,
   string            &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "Float64Data::compare():" << __LINE__ << endl;

   if ( this->f64 == data.f64 ) {
      msg << "this->f64 (" << this->f64 << ") == (" << data.f64 << ") data.f64" << endl;
   } else {
      msg << "this->f64 (" << this->f64 << ") != (" << data.f64 << ") data.f64" << endl;
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_f64[i] == data.vec3_f64[i] ) {
         msg << "this->vec3_f64[" << i << "] (" << this->vec3_f64[i]
             << ") == (" << data.vec3_f64[i] << ") data.vec3_f64[" << i << "]" << endl;
      } else {
         msg << "this->vec3_f64[" << i << "] (" << this->vec3_f64[i]
             << ") != (" << data.vec3_f64[i] << ") data.vec3_f64[" << i << "]" << endl;
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_f64[row][col] == data.m3x3_f64[row][col] ) {
            msg << "this->m3x3_f64[" << row << "][" << col << "] (" << this->m3x3_f64[row][col]
                << ") == (" << data.m3x3_f64[row][col] << ") data.m3x3_f64[" << row << "][" << col << "]" << endl;
         } else {
            msg << "this->m3x3_f64[" << row << "][" << col << "] (" << this->m3x3_f64[row][col]
                << ") != (" << data.m3x3_f64[row][col] << ") data.m3x3_f64[" << row << "][" << col << "]" << endl;
            equal_values = false;
         }
      }
   }

   int data1_ptr_f64_size = get_size( this->ptr_f64 );
   int data2_ptr_f64_size = get_size( data.ptr_f64 );
   if ( data1_ptr_f64_size != data2_ptr_f64_size ) {
      msg << "this->ptr_f64 size (" << data1_ptr_f64_size
          << ") != (" << data2_ptr_f64_size << ") data.ptr_f64 size" << endl;
      equal_values = false;
   } else {
      msg << "this->ptr_f64 size (" << data1_ptr_f64_size
          << ") == (" << data2_ptr_f64_size << ") data.ptr_f64 size" << endl;

      for ( int i = 0; i < data1_ptr_f64_size; ++i ) {
         if ( this->ptr_f64[i] == data.ptr_f64[i] ) {
            msg << "this->ptr_f64[" << i << "] (" << this->ptr_f64[i]
                << ") == (" << data.ptr_f64[i] << ") data.ptr_f64[" << i << "]" << endl;
         } else {
            msg << "this->ptr_f64[" << i << "] (" << this->ptr_f64[i]
                << ") != (" << data.ptr_f64[i] << ") data.ptr_f64[" << i << "]" << endl;
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string Float64Data::to_string()
{
   ostringstream msg;
   msg << "Float64Data::to_string():" << __LINE__ << endl
       << "f64:" << f64 << endl;

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_f64[" << i << "]:" << vec3_f64[i] << " ";
   }
   msg << endl;

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_f64[" << row << "][" << col << "]:" << m3x3_f64[row][col] << " ";
      }
   }
   msg << endl;

   int ptr_f64_size = get_size( ptr_f64 );
   msg << "ptr_f64 size:" << ptr_f64_size << endl;
   for ( int i = 0; i < ptr_f64_size; ++i ) {
      msg << "ptr_f64[" << i << "]:" << ptr_f64[i] << " ";
   }
   msg << endl;

   return msg.str();
}
