/*!
@ingroup encoding
@file models/encoding/src/Enum16Data.cpp
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
@trick_link_dependency{encoding/src/Enum16Data.cpp}
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, Dec 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string>

// Trick include files.
#include "trick/memorymanager_c_intf.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"

// Model include files.
#include "encoding/include/Enum16Data.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
Enum16Data::Enum16Data()
   : Enum16Data( 0 )
{
   return;
}

Enum16Data::Enum16Data(
   unsigned int const offset )
{
   enum16 = ( offset == 0 ) ? Int16Enum::one16 : Int16Enum::two16;

   for ( int i = 0; i < 3; ++i ) {
      vec3_enum16[i] = ( ( ( i + 1 + offset ) % 2 ) == 0 ) ? Int16Enum::one16 : Int16Enum::two16;
   }

   int value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_enum16[row][col] = ( ( ( value + offset ) % 2 ) == 0 ) ? Int16Enum::one16 : Int16Enum::two16;
         ++value;
      }
   }

   int ptr_enum16_size = 5 + ( ( offset < 10 ) ? offset : 10 );

   ptr_enum16 = static_cast< TrickHLAModel::Int16Enum * >(
      TMM_declare_var_1d( "TrickHLAModel::Int16Enum", ptr_enum16_size ) );
   if ( ptr_enum16 == NULL ) {
      ostringstream errmsg;
      errmsg << "Enum16Data::Enum16Data():" << __LINE__
             << " ERROR: Failed to allocate memory for ptr_enum16!" << endl;
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
   }
   for ( int i = 0; i < ptr_enum16_size; ++i ) {
      ptr_enum16[i] = ( ( ( i + 1 + offset ) % 2 ) == 0 ) ? Int16Enum::one16 : Int16Enum::two16;
   }
}

/*!
 * @job_class{shutdown}
 */
Enum16Data::~Enum16Data()
{
   return;
}

bool Enum16Data::compare(
   Enum16Data const &data,
   string           &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "Enum16Data::compare():" << __LINE__ << endl;

   if ( this->enum16 == data.enum16 ) {
      msg << "this->enum16 (" << (long long)this->enum16 << ") == (" << (long long)data.enum16 << ") data.enum16" << endl;
   } else {
      msg << "this->enum16 (" << (long long)this->enum16 << ") != (" << (long long)data.enum16 << ") data.enum16" << endl;
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_enum16[i] == data.vec3_enum16[i] ) {
         msg << "this->vec3_enum16[" << i << "] (" << (long long)this->vec3_enum16[i]
             << ") == (" << (long long)data.vec3_enum16[i] << ") data.vec3_enum16[" << i << "]" << endl;
      } else {
         msg << "this->vec3_enum16[" << i << "] (" << (long long)this->vec3_enum16[i]
             << ") != (" << (long long)data.vec3_enum16[i] << ") data.vec3_enum16[" << i << "]" << endl;
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_enum16[row][col] == data.m3x3_enum16[row][col] ) {
            msg << "this->m3x3_enum16[" << row << "][" << col << "] (" << (long long)this->m3x3_enum16[row][col]
                << ") == (" << (long long)data.m3x3_enum16[row][col] << ") data.m3x3_enum16[" << row << "][" << col << "]" << endl;
         } else {
            msg << "this->m3x3_enum16[" << row << "][" << col << "] (" << (long long)this->m3x3_enum16[row][col]
                << ") != (" << (long long)data.m3x3_enum16[row][col] << ") data.m3x3_enum16[" << row << "][" << col << "]" << endl;
            equal_values = false;
         }
      }
   }

   int data1_ptr_enum16_size = get_size( this->ptr_enum16 );
   int data2_ptr_enum16_size = get_size( data.ptr_enum16 );
   if ( data1_ptr_enum16_size != data2_ptr_enum16_size ) {
      msg << "this->ptr_enum16 size (" << data1_ptr_enum16_size
          << ") != (" << data2_ptr_enum16_size << ") data.ptr_enum16 size" << endl;
      equal_values = false;
   } else {
      msg << "this->ptr_enum16 size (" << data1_ptr_enum16_size
          << ") == (" << data2_ptr_enum16_size << ") data.ptr_enum16 size" << endl;

      for ( int i = 0; i < data1_ptr_enum16_size; ++i ) {
         if ( this->ptr_enum16[i] == data.ptr_enum16[i] ) {
            msg << "this->ptr_enum16[" << i << "] (" << (long long)this->ptr_enum16[i]
                << ") == (" << (long long)data.ptr_enum16[i] << ") data.ptr_enum16[" << i << "]" << endl;
         } else {
            msg << "this->ptr_enum16[" << i << "] (" << (long long)this->ptr_enum16[i]
                << ") != (" << (long long)data.ptr_enum16[i] << ") data.ptr_enum16[" << i << "]" << endl;
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string Enum16Data::to_string()
{
   ostringstream msg;
   msg << "Enum16Data::to_string():" << __LINE__ << endl
       << "enum16:" << (long long)enum16 << endl;

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_enum16[" << i << "]:" << (long long)vec3_enum16[i] << " ";
   }
   msg << endl;

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_enum16[" << row << "][" << col << "]:" << (long long)m3x3_enum16[row][col] << " ";
      }
   }
   msg << endl;

   int ptr_enum16_size = get_size( ptr_enum16 );
   msg << "ptr_enum16 size:" << ptr_enum16_size << endl;
   for ( int i = 0; i < ptr_enum16_size; ++i ) {
      msg << "ptr_enum16[" << i << "]:" << (long long)ptr_enum16[i] << " ";
   }
   msg << endl;

   return msg.str();
}
