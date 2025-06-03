/*!
@ingroup encoding
@file models/encoding/src/LongData.cpp
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
@trick_link_dependency{encoding/src/LongData.cpp}
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
#include "../include/LongData.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
LongData::LongData()
   : LongData( 0 )
{
   return;
}

LongData::LongData(
   int const offset )
{
   _long = 1 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_long[i] = i + 1 + offset;
   }

   int value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_long[row][col] = value + offset;
         ++value;
      }
   }

   int const ptr_long_size = 5 + offset;

   ptr_long = static_cast< long * >( TMM_declare_var_1d( "long", ptr_long_size ) );
   for ( int i = 0; i < ptr_long_size; ++i ) {
      ptr_long[i] = i + 1 + offset;
   }
}

/*!
 * @job_class{shutdown}
 */
LongData::~LongData()
{
   return;
}

bool LongData::compare(
   LongData const &data,
   string         &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "LongData::compare():" << __LINE__ << "\n";

   if ( this->_long == data._long ) {
      msg << "this->long (" << this->_long << ") == (" << data._long << ") data.long\n";
   } else {
      msg << "this->long (" << this->_long << ") != (" << data._long << ") data.long\n";
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_long[i] == data.vec3_long[i] ) {
         msg << "this->vec3_long[" << i << "] (" << this->vec3_long[i]
             << ") == (" << data.vec3_long[i] << ") data.vec3_long[" << i << "]\n";
      } else {
         msg << "this->vec3_long[" << i << "] (" << this->vec3_long[i]
             << ") != (" << data.vec3_long[i] << ") data.vec3_long[" << i << "]\n";
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_long[row][col] == data.m3x3_long[row][col] ) {
            msg << "this->m3x3_long[" << row << "][" << col << "] (" << this->m3x3_long[row][col]
                << ") == (" << data.m3x3_long[row][col] << ") data.m3x3_long[" << row << "][" << col << "]\n";
         } else {
            msg << "this->m3x3_long[" << row << "][" << col << "] (" << this->m3x3_long[row][col]
                << ") != (" << data.m3x3_long[row][col] << ") data.m3x3_long[" << row << "][" << col << "]\n";
            equal_values = false;
         }
      }
   }

   int data1_ptr_long_size = get_size( this->ptr_long );
   int data2_ptr_long_size = get_size( data.ptr_long );
   if ( data1_ptr_long_size != data2_ptr_long_size ) {
      msg << "this->ptr_long size (" << data1_ptr_long_size
          << ") != (" << data2_ptr_long_size << ") data.ptr_long size\n";
      equal_values = false;
   } else {
      msg << "this->ptr_long size (" << data1_ptr_long_size
          << ") == (" << data2_ptr_long_size << ") data.ptr_long size\n";

      for ( int i = 0; i < data1_ptr_long_size; ++i ) {
         if ( this->ptr_long[i] == data.ptr_long[i] ) {
            msg << "this->ptr_long[" << i << "] (" << this->ptr_long[i]
                << ") == (" << data.ptr_long[i] << ") data.ptr_long[" << i << "]\n";
         } else {
            msg << "this->ptr_long[" << i << "] (" << this->ptr_long[i]
                << ") != (" << data.ptr_long[i] << ") data.ptr_long[" << i << "]\n";
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string LongData::to_string()
{
   ostringstream msg;
   msg << "LongData::to_string():" << __LINE__ << "\n"
       << "long:" << _long << "\n";

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_long[" << i << "]:" << vec3_long[i] << " ";
   }
   msg << "\n";

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_long[" << row << "][" << col << "]:" << m3x3_long[row][col] << " ";
      }
   }
   msg << "\n";

   int ptr_long_size = get_size( ptr_long );
   msg << "ptr_long size:" << ptr_long_size << "\n";
   for ( int i = 0; i < ptr_long_size; ++i ) {
      msg << "ptr_long[" << i << "]:" << ptr_long[i] << " ";
   }
   msg << "\n";

   return msg.str();
}
