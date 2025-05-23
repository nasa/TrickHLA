/*!
@ingroup encoding
@file models/encoding/src/StringData.cpp
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
@trick_link_dependency{encoding/src/StringData.cpp}

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
#include "../include/StringData.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
StringData::StringData()
   : StringData( 0 )
{
   return;
}

StringData::StringData(
   int const offset )
{
   int value = 1 + offset;
   _string = "s-" + std::to_string( value );

   for ( int i = 0; i < 3; ++i ) {
      value = i + 1 + offset;
      vec3_string[i] = "s-" + std::to_string( value );
   }

   int cnt = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         value = cnt + offset;
         m3x3_string[row][col] = "s-" + std::to_string( value );
         ++cnt;
      }
   }

   int const ptr_string_size = 5 + offset;

   ptr_string = static_cast< string * >( TMM_declare_var_1d( "std::string", ptr_string_size ) );
   for ( int i = 0; i < ptr_string_size; ++i ) {
      value = i + 1 + offset;
      ptr_string[i] = "s-" + std::to_string( value );
   }
}

/*!
 * @job_class{shutdown}
 */
StringData::~StringData()
{
   return;
}

bool StringData::compare(
   StringData &data )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "StringData::compare():" << __LINE__ << "\n";

   if ( this->_string == data._string ) {
      msg << "this->string (" << this->_string << ") == (" << data._string << ") data.string\n";
   } else {
      msg << "this->string (" << this->_string << ") != (" << data._string << ") data.string\n";
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_string[i] == data.vec3_string[i] ) {
         msg << "this->vec3_string[" << i << "] (" << this->vec3_string[i]
             << ") == (" << data.vec3_string[i] << ") data.vec3_string[" << i << "]\n";
      } else {
         msg << "this->vec3_string[" << i << "] (" << this->vec3_string[i]
             << ") != (" << data.vec3_string[i] << ") data.vec3_string[" << i << "]\n";
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_string[row][col] == data.m3x3_string[row][col] ) {
            msg << "this->m3x3_string[" << row << "][" << col << "] (" << this->m3x3_string[row][col]
                << ") == (" << data.m3x3_string[row][col] << ") data.m3x3_string[" << row << "][" << col << "]\n";
         } else {
            msg << "this->m3x3_string[" << row << "][" << col << "] (" << this->m3x3_string[row][col]
                << ") != (" << data.m3x3_string[row][col] << ") data.m3x3_string[" << row << "][" << col << "]\n";
            equal_values = false;
         }
      }
   }

   int data1_ptr_string_size = get_size( this->ptr_string );
   int data2_ptr_string_size = get_size( data.ptr_string );
   int min_ptr_string_size   = ( data1_ptr_string_size <= data2_ptr_string_size )
                                ? data1_ptr_string_size
                                : data2_ptr_string_size;
   if ( data1_ptr_string_size != data2_ptr_string_size ) {
      msg << "this->ptr_string size (" << data1_ptr_string_size
          << ") != (" << data2_ptr_string_size << ") data.ptr_string size\n";
      equal_values = false;
   } else {
      msg << "this->ptr_string size (" << data1_ptr_string_size
          << ") == (" << data2_ptr_string_size << ") data.ptr_string size\n";
   }
   for ( int i = 0; i < min_ptr_string_size; ++i ) {
      if ( this->ptr_string[i] == data.ptr_string[i] ) {
         msg << "this->ptr_string[" << i << "] (" << this->ptr_string[i]
             << ") == (" << data.ptr_string[i] << ") data.ptr_string[" << i << "]\n";
      } else {
         msg << "this->ptr_string[" << i << "] (" << this->ptr_string[i]
             << ") != (" << data.ptr_string[i] << ") data.ptr_string[" << i << "]\n";
         equal_values = false;
      }
   }

   if ( equal_values ) {
      message_publish( MSG_NORMAL, msg.str().c_str() );
   } else {
      message_publish( MSG_ERROR, msg.str().c_str() );
   }

   return equal_values;
}

string StringData::to_string()
{
   ostringstream msg;
   msg << "StringData::to_string():" << __LINE__ << "\n"
       << "string:" << _string << "\n";

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_string[" << i << "]:" << vec3_string[i] << " ";
   }
   msg << "\n";

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_string[" << row << "][" << col << "]:" << m3x3_string[row][col] << " ";
      }
   }
   msg << "\n";

   int ptr_string_size = get_size( ptr_string );
   msg << "ptr_string size:" << ptr_string_size << "\n";
   for ( int i = 0; i < ptr_string_size; ++i ) {
      msg << "ptr_string[" << i << "]:" << ptr_string[i] << " ";
   }
   msg << "\n";

   return msg.str();
}
