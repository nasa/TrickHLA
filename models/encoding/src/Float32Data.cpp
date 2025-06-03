/*!
@ingroup encoding
@file models/encoding/src/Float32Data.cpp
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
@trick_link_dependency{encoding/src/Float32Data.cpp}
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
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/integrator_c_intf.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"
#include "trick/trick_math.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"

// Model include files.
#include "../include/Float32Data.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
Float32Data::Float32Data()
   : Float32Data( 0 )
{
   return;
}

Float32Data::Float32Data(
   int const offset )
{
   f32 = 1.0 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_f32[i] = i + 1.0 + offset;
   }

   float value = 1.0;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_f32[row][col] = value + offset;
         value += 1.0;
      }
   }

   int const ptr_f32_size = 5 + offset;

   ptr_f32 = static_cast< float * >( TMM_declare_var_1d( "float", ptr_f32_size ) );
   for ( int i = 0; i < ptr_f32_size; ++i ) {
      ptr_f32[i] = i + 1.0 + offset;
   }
}

/*!
 * @job_class{shutdown}
 */
Float32Data::~Float32Data()
{
   return;
}

bool Float32Data::compare(
   Float32Data const &data,
   string            &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "Float32Data::compare():" << __LINE__ << "\n";

   if ( this->f32 == data.f32 ) {
      msg << "this->f32 (" << this->f32 << ") == (" << data.f32 << ") data.f32\n";
   } else {
      msg << "this->f32 (" << this->f32 << ") != (" << data.f32 << ") data.f32\n";
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_f32[i] == data.vec3_f32[i] ) {
         msg << "this->vec3_f32[" << i << "] (" << this->vec3_f32[i]
             << ") == (" << data.vec3_f32[i] << ") data.vec3_f32[" << i << "]\n";
      } else {
         msg << "this->vec3_f32[" << i << "] (" << this->vec3_f32[i]
             << ") != (" << data.vec3_f32[i] << ") data.vec3_f32[" << i << "]\n";
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_f32[row][col] == data.m3x3_f32[row][col] ) {
            msg << "this->m3x3_f32[" << row << "][" << col << "] (" << this->m3x3_f32[row][col]
                << ") == (" << data.m3x3_f32[row][col] << ") data.m3x3_f32[" << row << "][" << col << "]\n";
         } else {
            msg << "this->m3x3_f32[" << row << "][" << col << "] (" << this->m3x3_f32[row][col]
                << ") != (" << data.m3x3_f32[row][col] << ") data.m3x3_f32[" << row << "][" << col << "]\n";
            equal_values = false;
         }
      }
   }

   int data1_ptr_f32_size = get_size( this->ptr_f32 );
   int data2_ptr_f32_size = get_size( data.ptr_f32 );
   if ( data1_ptr_f32_size != data2_ptr_f32_size ) {
      msg << "this->ptr_f32 size (" << data1_ptr_f32_size
          << ") != (" << data2_ptr_f32_size << ") data.ptr_f32 size\n";
      equal_values = false;
   } else {
      msg << "this->ptr_f32 size (" << data1_ptr_f32_size
          << ") == (" << data2_ptr_f32_size << ") data.ptr_f32 size\n";

      for ( int i = 0; i < data1_ptr_f32_size; ++i ) {
         if ( this->ptr_f32[i] == data.ptr_f32[i] ) {
            msg << "this->ptr_f32[" << i << "] (" << this->ptr_f32[i]
                << ") == (" << data.ptr_f32[i] << ") data.ptr_f32[" << i << "]\n";
         } else {
            msg << "this->ptr_f32[" << i << "] (" << this->ptr_f32[i]
                << ") != (" << data.ptr_f32[i] << ") data.ptr_f32[" << i << "]\n";
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string Float32Data::to_string()
{
   ostringstream msg;
   msg << "Float32Data::to_string():" << __LINE__ << "\n"
       << "f32:" << f32 << "\n";

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_f32[" << i << "]:" << vec3_f32[i] << " ";
   }
   msg << "\n";

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_f32[" << row << "][" << col << "]:" << m3x3_f32[row][col] << " ";
      }
   }
   msg << "\n";

   int ptr_f32_size = get_size( ptr_f32 );
   msg << "ptr_f32 size:" << ptr_f32_size << "\n";
   for ( int i = 0; i < ptr_f32_size; ++i ) {
      msg << "ptr_f32[" << i << "]:" << ptr_f32[i] << " ";
   }
   msg << "\n";

   return msg.str();
}
