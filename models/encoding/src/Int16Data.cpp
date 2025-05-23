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

// Model include files.
#include "../include/Int16Data.hh"

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
   int const offset )
{
   i16 = 1 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_i16[i] = i + 1 + offset;
   }

   int value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_i16[row][col] = value + offset;
         ++value;
      }
   }

   int const ptr_i16_size = 5 + offset;

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
   Int16Data &data )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "Int16Data::compare():" << __LINE__ << "\n";

   if ( this->i16 == data.i16 ) {
      msg << "this->i16 (" << this->i16 << ") == (" << data.i16 << ") data.i16\n";
   } else {
      msg << "this->i16 (" << this->i16 << ") != (" << data.i16 << ") data.i16\n";
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_i16[i] == data.vec3_i16[i] ) {
         msg << "this->vec3_i16[" << i << "] (" << this->vec3_i16[i]
             << ") == (" << data.vec3_i16[i] << ") data.vec3_i16[" << i << "]\n";
      } else {
         msg << "this->vec3_i16[" << i << "] (" << this->vec3_i16[i]
             << ") != (" << data.vec3_i16[i] << ") data.vec3_i16[" << i << "]\n";
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_i16[row][col] == data.m3x3_i16[row][col] ) {
            msg << "this->m3x3_i16[" << row << "][" << col << "] (" << this->m3x3_i16[row][col]
                << ") == (" << data.m3x3_i16[row][col] << ") data.m3x3_i16[" << row << "][" << col << "]\n";
         } else {
            msg << "this->m3x3_i16[" << row << "][" << col << "] (" << this->m3x3_i16[row][col]
                << ") != (" << data.m3x3_i16[row][col] << ") data.m3x3_i16[" << row << "][" << col << "]\n";
            equal_values = false;
         }
      }
   }

   int data1_ptr_i16_size = get_size( this->ptr_i16 );
   int data2_ptr_i16_size = get_size( data.ptr_i16 );
   if ( data1_ptr_i16_size != data2_ptr_i16_size ) {
      msg << "this->ptr_i16 size (" << data1_ptr_i16_size
          << ") != (" << data2_ptr_i16_size << ") data.ptr_i16 size\n";
      equal_values = false;
   } else {
      msg << "this->ptr_i16 size (" << data1_ptr_i16_size
          << ") == (" << data2_ptr_i16_size << ") data.ptr_i16 size\n";

      for ( int i = 0; i < data1_ptr_i16_size; ++i ) {
         if ( this->ptr_i16[i] == data.ptr_i16[i] ) {
            msg << "this->ptr_i16[" << i << "] (" << this->ptr_i16[i]
                << ") == (" << data.ptr_i16[i] << ") data.ptr_i16[" << i << "]\n";
         } else {
            msg << "this->ptr_i16[" << i << "] (" << this->ptr_i16[i]
                << ") != (" << data.ptr_i16[i] << ") data.ptr_i16[" << i << "]\n";
            equal_values = false;
         }
      }
   }

   if ( equal_values ) {
      message_publish( MSG_NORMAL, msg.str().c_str() );
   } else {
      message_publish( MSG_ERROR, msg.str().c_str() );
   }

   return equal_values;
}

string Int16Data::to_string()
{
   ostringstream msg;
   msg << "Int16Data::to_string():" << __LINE__ << "\n"
       << "i16:" << i16 << "\n";

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_i16[" << i << "]:" << vec3_i16[i] << " ";
   }
   msg << "\n";

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_i16[" << row << "][" << col << "]:" << m3x3_i16[row][col] << " ";
      }
   }
   msg << "\n";

   int ptr_i16_size = get_size( ptr_i16 );
   msg << "ptr_i16 size:" << ptr_i16_size << "\n";
   for ( int i = 0; i < ptr_i16_size; ++i ) {
      msg << "ptr_i16[" << i << "]:" << ptr_i16[i] << " ";
   }
   msg << "\n";

   return msg.str();
}
