/*!
@ingroup encoding
@file models/encoding/src/BasicData.cpp
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
@trick_link_dependency{encoding/src/BasicData.cpp}

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
#include "../include/BasicData.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
BasicData::BasicData()
   : BasicData( 0 )
{
   return;
}

BasicData::BasicData(
   int const offset )
{
   i32 = 1 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_i32[i] = i + 1 + offset;
   }

   int value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_i32[row][col] = value + offset;
         ++value;
      }
   }

   int const p_i32_size = 5 + offset;
   p_i32 = static_cast< int * >( TMM_declare_var_1d( "int", p_i32_size ) );
   for ( int i = 0; i < p_i32_size; ++i ) {
      p_i32[i] = i + 1 + offset;
   }
}

/*!
 * @job_class{shutdown}
 */
BasicData::~BasicData()
{
   return;
}

bool BasicData::compare(
   BasicData &data )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "BasicData::compare():" << __LINE__ << "\n";

   if ( this->i32 == data.i32 ) {
      msg << "this->i32 (" << this->i32 << ") == (" << data.i32 << ") data.i32\n";
   } else {
      msg << "this->i32 (" << this->i32 << ") != (" << data.i32 << ") data.i32\n";
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_i32[i] == data.vec3_i32[i] ) {
         msg << "this->vec3_i32[" << i << "] (" << this->vec3_i32[i]
             << ") == (" << data.vec3_i32[i] << ") data.vec3_i32[" << i << "]\n";
      } else {
         msg << "this->vec3_i32[" << i << "] (" << this->vec3_i32[i]
             << ") != (" << data.vec3_i32[i] << ") data.vec3_i32[" << i << "]\n";
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_i32[row][col] == data.m3x3_i32[row][col] ) {
            msg << "this->m3x3_i32[" << row << "][" << col << "] (" << this->m3x3_i32[row][col]
                << ") == (" << data.m3x3_i32[row][col] << ") data.m3x3_i32[" << row << "][" << col << "]\n";
         } else {
            msg << "this->m3x3_i32[" << row << "][" << col << "] (" << this->m3x3_i32[row][col]
                << ") != (" << data.m3x3_i32[row][col] << ") data.m3x3_i32[" << row << "][" << col << "]\n";
            equal_values = false;
         }
      }
   }

   int data1_p_i32_size = get_size( this->p_i32 );
   int data2_p_i32_size = get_size( data.p_i32 );
   if ( data1_p_i32_size != data2_p_i32_size ) {
      msg << "this->p_i32 size (" << data1_p_i32_size
          << ") != (" << data2_p_i32_size << ") data.p_i32 size\n";
      equal_values = false;
   } else {
      msg << "this->p_i32 size (" << data1_p_i32_size
          << ") == (" << data2_p_i32_size << ") data.p_i32 size\n";

      for ( int i = 0; i < data1_p_i32_size; ++i ) {
         if ( this->p_i32[i] == data.p_i32[i] ) {
            msg << "this->p_i32[" << i << "] (" << this->p_i32[i]
                << ") == (" << data.p_i32[i] << ") data.p_i32[" << i << "]\n";
         } else {
            msg << "this->p_i32[" << i << "] (" << this->p_i32[i]
                << ") != (" << data.p_i32[i] << ") data.p_i32[" << i << "]\n";
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

string BasicData::to_string()
{
   ostringstream msg;
   msg << "BasicData::print():" << __LINE__ << "\n"
       << "i32:" << i32 << "\n";

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_i32[" << i << "]:" << vec3_i32[i] << " ";
   }
   msg << "\n";

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_i32[" << row << "][" << col << "]:" << m3x3_i32[row][col] << " ";
      }
   }
   msg << "\n";

   int p_i32_size = get_size( p_i32 );
   msg << "p_i32 size:" << p_i32_size << "\n";
   for ( int i = 0; i < p_i32_size; ++i ) {
      msg << "p_i32[" << i << "]:" << p_i32[i] << " ";
   }
   msg << "\n";

   return msg.str();
}
