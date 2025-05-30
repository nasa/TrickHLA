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
#include "../include/StringData.hh"

using namespace std;
using namespace TrickHLA;
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
   _string   = "string-" + std::to_string( value ); // cppcheck-suppress [useInitializationList]

   for ( int i = 0; i < 3; ++i ) {
      value          = i + 1 + offset;
      vec3_string[i] = "string-" + std::to_string( value );
   }

   int cnt = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         value                 = cnt + offset;
         m3x3_string[row][col] = "string-" + std::to_string( value );
         ++cnt;
      }
   }

   int num_items = 1 + offset; // Number of std::string in array.
   ptr_string    = static_cast< std::string * >( TMM_declare_var_1d( "std::string", num_items ) );
   for ( int i = 0; i < num_items; ++i ) {
      value         = i + 1 + offset;
      ptr_string[i] = "string-" + std::to_string( value );
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
   StringData  &data,
   std::string &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "StringData::compare():" << __LINE__ << "\n";

   if ( this->_string == data._string ) {
      msg << "this->string '" << this->_string << "' == '" << data._string << "' data.string\n";
   } else {
      msg << "this->string '" << this->_string << "' != '" << data._string << "' data.string\n";
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_string[i] == data.vec3_string[i] ) {
         msg << "this->vec3_string[" << i << "] '" << this->vec3_string[i]
             << "' == '" << data.vec3_string[i] << "' data.vec3_string[" << i << "]\n";
      } else {
         msg << "this->vec3_string[" << i << "] '" << this->vec3_string[i]
             << "' != '" << data.vec3_string[i] << "' data.vec3_string[" << i << "]\n";
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_string[row][col] == data.m3x3_string[row][col] ) {
            msg << "this->m3x3_string[" << row << "][" << col << "] '" << this->m3x3_string[row][col]
                << "' == '" << data.m3x3_string[row][col] << "' data.m3x3_string[" << row << "][" << col << "]\n";
         } else {
            msg << "this->m3x3_string[" << row << "][" << col << "] '" << this->m3x3_string[row][col]
                << "' != '" << data.m3x3_string[row][col] << "' data.m3x3_string[" << row << "][" << col << "]\n";
            equal_values = false;
         }
      }
   }

   int const data1_num_items = get_size( this->ptr_string );
   int const data2_num_items = get_size( data.ptr_string );
   int const min_num_items   = ( data1_num_items <= data2_num_items )
                                  ? data1_num_items
                                  : data2_num_items;

   if ( data1_num_items != data2_num_items ) {
      msg << "this->ptr_string num-items (" << data1_num_items
          << ") != (" << data2_num_items << ") data.ptr_string num-items\n";
      equal_values = false;
   } else {
      msg << "this->ptr_string num-items (" << data1_num_items
          << ") == (" << data2_num_items << ") data.ptr_string num-items\n";
   }
   for ( int i = 0; i < min_num_items; ++i ) {
      if ( this->ptr_string[i].compare( data.ptr_string[i] ) == 0 ) {
         msg << "this->ptr_string[" << i << "]:'" << this->ptr_string[i]
             << "' == '" << data.ptr_string[i] << "':data.ptr_string[" << i << "]\n";
      } else {
         msg << "*this->ptr_string[" << i << "]:'" << this->ptr_string[i]
             << "' != '" << data.ptr_string[i] << "':data.ptr_string[" << i << "]\n";
         equal_values = false;
      }
   }

   explanation = msg.str();

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

   int num_items = get_size( this->ptr_string );
   msg << "ptr_string size:" << num_items << "\n";
   for ( int i = 0; i < num_items; ++i ) {
      msg << "ptr_string[" << i << "]:" << this->ptr_string[i] << "\n";
   }
   msg << "\n";

   return msg.str();
}
