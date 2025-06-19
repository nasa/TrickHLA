/*!
@ingroup encoding
@file models/encoding/src/BoolData.cpp
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
@trick_link_dependency{encoding/src/BoolData.cpp}

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
#include "encoding/include/BoolData.hh"

using namespace std;
using namespace TrickHLA;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
BoolData::BoolData()
   : BoolData( 0 )
{
   return;
}

BoolData::BoolData(
   int const offset )
{
   _bool = ( ( ( 1 + offset ) % 2 ) == 0 );

   for ( int i = 0; i < 3; ++i ) {
      vec3_bool[i] = ( ( ( i + 1 + offset ) % 2 ) == 0 );
   }

   int value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_bool[row][col] = ( ( ( value + offset ) % 2 ) == 0 );
         ++value;
      }
   }

   int const ptr_bool_size = 5 + offset;

   ptr_bool = static_cast< bool * >( TMM_declare_var_1d( "bool", ptr_bool_size ) );
   for ( int i = 0; i < ptr_bool_size; ++i ) {
      ptr_bool[i] = ( ( ( i + 1 + offset ) % 2 ) == 0 );
   }
}

/*!
 * @job_class{shutdown}
 */
BoolData::~BoolData()
{
   return;
}

bool BoolData::compare(
   BoolData const &data,
   string         &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "BoolData::compare():" << __LINE__ << "\n";

   if ( this->_bool == data._bool ) {
      msg << "this->bool (" << this->_bool << ") == (" << data._bool << ") data.bool\n";
   } else {
      msg << "this->bool (" << this->_bool << ") != (" << data._bool << ") data.bool\n";
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_bool[i] == data.vec3_bool[i] ) {
         msg << "this->vec3_bool[" << i << "] (" << this->vec3_bool[i]
             << ") == (" << data.vec3_bool[i] << ") data.vec3_bool[" << i << "]\n";
      } else {
         msg << "this->vec3_bool[" << i << "] (" << this->vec3_bool[i]
             << ") != (" << data.vec3_bool[i] << ") data.vec3_bool[" << i << "]\n";
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_bool[row][col] == data.m3x3_bool[row][col] ) {
            msg << "this->m3x3_bool[" << row << "][" << col << "] (" << this->m3x3_bool[row][col]
                << ") == (" << data.m3x3_bool[row][col] << ") data.m3x3_bool[" << row << "][" << col << "]\n";
         } else {
            msg << "this->m3x3_bool[" << row << "][" << col << "] (" << this->m3x3_bool[row][col]
                << ") != (" << data.m3x3_bool[row][col] << ") data.m3x3_bool[" << row << "][" << col << "]\n";
            equal_values = false;
         }
      }
   }

   int data1_ptr_bool_size = get_size( this->ptr_bool );
   int data2_ptr_bool_size = get_size( data.ptr_bool );
   if ( data1_ptr_bool_size != data2_ptr_bool_size ) {
      msg << "this->ptr_bool size (" << data1_ptr_bool_size
          << ") != (" << data2_ptr_bool_size << ") data.ptr_bool size\n";
      equal_values = false;
   } else {
      msg << "this->ptr_bool size (" << data1_ptr_bool_size
          << ") == (" << data2_ptr_bool_size << ") data.ptr_bool size\n";

      for ( int i = 0; i < data1_ptr_bool_size; ++i ) {
         if ( this->ptr_bool[i] == data.ptr_bool[i] ) {
            msg << "this->ptr_bool[" << i << "] (" << this->ptr_bool[i]
                << ") == (" << data.ptr_bool[i] << ") data.ptr_bool[" << i << "]\n";
         } else {
            msg << "this->ptr_bool[" << i << "] (" << this->ptr_bool[i]
                << ") != (" << data.ptr_bool[i] << ") data.ptr_bool[" << i << "]\n";
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string BoolData::to_string()
{
   ostringstream msg;
   msg << "BoolData::to_string():" << __LINE__ << "\n"
       << "bool:" << _bool << "\n";

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_bool[" << i << "]:" << vec3_bool[i] << " ";
   }
   msg << "\n";

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_bool[" << row << "][" << col << "]:" << m3x3_bool[row][col] << " ";
      }
   }
   msg << "\n";

   int ptr_bool_size = get_size( ptr_bool );
   msg << "ptr_bool size:" << ptr_bool_size << "\n";
   for ( int i = 0; i < ptr_bool_size; ++i ) {
      msg << "ptr_bool[" << i << "]:" << ptr_bool[i] << " ";
   }
   msg << "\n";

   return msg.str();
}
