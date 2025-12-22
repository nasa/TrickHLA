/*!
@ingroup encoding
@file models/encoding/src/Enum64Data.cpp
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
@trick_link_dependency{encoding/src/Enum64Data.cpp}
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
#include "encoding/include/Enum64Data.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
Enum64Data::Enum64Data()
   : Enum64Data( 0 )
{
   return;
}

Enum64Data::Enum64Data(
   unsigned int const offset )
{
   enum64 = ( offset == 0 ) ? Int64Enum::one64 : Int64Enum::two64;

   for ( int i = 0; i < 3; ++i ) {
      vec3_enum64[i] = ( ( ( i + 1 + offset ) % 2 ) == 0 ) ? Int64Enum::one64 : Int64Enum::two64;
   }

   int value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_enum64[row][col] = ( ( ( value + offset ) % 2 ) == 0 ) ? Int64Enum::one64 : Int64Enum::two64;
         ++value;
      }
   }

   int ptr_enum64_size = 5 + ( ( offset < 10 ) ? offset : 10 );

   ptr_enum64 = static_cast< TrickHLAModel::Int64Enum * >(
      TMM_declare_var_1d( "TrickHLAModel::Int64Enum", ptr_enum64_size ) );
   if ( ptr_enum64 == NULL ) {
      ostringstream errmsg;
      errmsg << "Enum64Data::Enum64Data():" << __LINE__
             << " ERROR: Failed to allocate memory for ptr_enum64!" << endl;
      TrickHLA::DebugHandler::terminate_with_message( errmsg.str() );
      return;
   }
   for ( int i = 0; i < ptr_enum64_size; ++i ) {
      ptr_enum64[i] = ( ( ( i + 1 + offset ) % 2 ) == 0 ) ? Int64Enum::one64 : Int64Enum::two64;
   }
}

/*!
 * @job_class{shutdown}
 */
Enum64Data::~Enum64Data()
{
   return;
}

bool Enum64Data::compare(
   Enum64Data const &data,
   string           &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "Enum64Data::compare():" << __LINE__ << endl;

   if ( this->enum64 == data.enum64 ) {
      msg << "this->enum64 (" << (long long)this->enum64 << ") == (" << (long long)data.enum64 << ") data.enum64" << endl;
   } else {
      msg << "this->enum64 (" << (long long)this->enum64 << ") != (" << (long long)data.enum64 << ") data.enum64" << endl;
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_enum64[i] == data.vec3_enum64[i] ) {
         msg << "this->vec3_enum64[" << i << "] (" << (long long)this->vec3_enum64[i]
             << ") == (" << (long long)data.vec3_enum64[i] << ") data.vec3_enum64[" << i << "]" << endl;
      } else {
         msg << "this->vec3_enum64[" << i << "] (" << (long long)this->vec3_enum64[i]
             << ") != (" << (long long)data.vec3_enum64[i] << ") data.vec3_enum64[" << i << "]" << endl;
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_enum64[row][col] == data.m3x3_enum64[row][col] ) {
            msg << "this->m3x3_enum64[" << row << "][" << col << "] (" << (long long)this->m3x3_enum64[row][col]
                << ") == (" << (long long)data.m3x3_enum64[row][col] << ") data.m3x3_enum64[" << row << "][" << col << "]" << endl;
         } else {
            msg << "this->m3x3_enum64[" << row << "][" << col << "] (" << (long long)this->m3x3_enum64[row][col]
                << ") != (" << (long long)data.m3x3_enum64[row][col] << ") data.m3x3_enum64[" << row << "][" << col << "]" << endl;
            equal_values = false;
         }
      }
   }

   int data1_ptr_enum64_size = get_size( this->ptr_enum64 );
   int data2_ptr_enum64_size = get_size( data.ptr_enum64 );
   if ( data1_ptr_enum64_size != data2_ptr_enum64_size ) {
      msg << "this->ptr_enum64 size (" << data1_ptr_enum64_size
          << ") != (" << data2_ptr_enum64_size << ") data.ptr_enum64 size" << endl;
      equal_values = false;
   } else {
      msg << "this->ptr_enum64 size (" << data1_ptr_enum64_size
          << ") == (" << data2_ptr_enum64_size << ") data.ptr_enum64 size" << endl;

      for ( int i = 0; i < data1_ptr_enum64_size; ++i ) {
         if ( this->ptr_enum64[i] == data.ptr_enum64[i] ) {
            msg << "this->ptr_enum64[" << i << "] (" << (long long)this->ptr_enum64[i]
                << ") == (" << (long long)data.ptr_enum64[i] << ") data.ptr_enum64[" << i << "]" << endl;
         } else {
            msg << "this->ptr_enum64[" << i << "] (" << (long long)this->ptr_enum64[i]
                << ") != (" << (long long)data.ptr_enum64[i] << ") data.ptr_enum64[" << i << "]" << endl;
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string Enum64Data::to_string()
{
   ostringstream msg;
   msg << "Enum64Data::to_string():" << __LINE__ << endl
       << "enum64:" << (long long)enum64 << endl;

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_enum64[" << i << "]:" << (long long)vec3_enum64[i] << " ";
   }
   msg << endl;

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_enum64[" << row << "][" << col << "]:" << (long long)m3x3_enum64[row][col] << " ";
      }
   }
   msg << endl;

   int ptr_enum64_size = get_size( ptr_enum64 );
   msg << "ptr_enum64 size:" << ptr_enum64_size << endl;
   for ( int i = 0; i < ptr_enum64_size; ++i ) {
      msg << "ptr_enum64[" << i << "]:" << (long long)ptr_enum64[i] << " ";
   }
   msg << endl;

   return msg.str();
}
