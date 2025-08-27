/*!
@ingroup encoding
@file models/encoding/src/UInt16Data.cpp
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
@trick_link_dependency{encoding/src/UInt16Data.cpp}
@trick_link_dependency{../../../source/TrickHLA/DebugHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, August 2025, --, Initial implementation.}
@revs_end

*/

// System include files.
#include <sstream>
#include <string>

// Trick include files.
#include "trick/memorymanager_c_intf.h"

// Model include files.
#include "encoding/include/UInt16Data.hh"

using namespace std;
using namespace TrickHLAModel;

/*!
 * @job_class{initialization}
 */
UInt16Data::UInt16Data()
   : UInt16Data( 0 )
{
   return;
}

UInt16Data::UInt16Data(
   unsigned short const offset )
{
   ui16 = 1 + offset;

   for ( int i = 0; i < 3; ++i ) {
      vec3_ui16[i] = i + 1 + offset;
   }

   unsigned short value = 1;
   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         m3x3_ui16[row][col] = value + offset;
         ++value;
      }
   }

   int const ptr_ui16_size = 5 + ( ( offset < 10 ) ? offset : 10 );

   ptr_ui16 = static_cast< unsigned short * >( TMM_declare_var_1d( "unsigned short", ptr_ui16_size ) );
   for ( int i = 0; i < ptr_ui16_size; ++i ) {
      ptr_ui16[i] = i + 1 + offset;
   }
}

/*!
 * @job_class{shutdown}
 */
UInt16Data::~UInt16Data()
{
   return;
}

bool UInt16Data::compare(
   UInt16Data const &data,
   string           &explanation )
{
   bool equal_values = true;

   ostringstream msg;
   msg << "UInt16Data::compare():" << __LINE__ << endl;

   if ( this->ui16 == data.ui16 ) {
      msg << "this->ui16 (" << this->ui16 << ") == (" << data.ui16 << ") data.ui16" << endl;
   } else {
      msg << "this->ui16 (" << this->ui16 << ") != (" << data.ui16 << ") data.ui16" << endl;
      equal_values = false;
   }

   for ( int i = 0; i < 3; ++i ) {
      if ( this->vec3_ui16[i] == data.vec3_ui16[i] ) {
         msg << "this->vec3_ui16[" << i << "] (" << this->vec3_ui16[i]
             << ") == (" << data.vec3_ui16[i] << ") data.vec3_ui16[" << i << "]" << endl;
      } else {
         msg << "this->vec3_ui16[" << i << "] (" << this->vec3_ui16[i]
             << ") != (" << data.vec3_ui16[i] << ") data.vec3_ui16[" << i << "]" << endl;
         equal_values = false;
      }
   }

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         if ( this->m3x3_ui16[row][col] == data.m3x3_ui16[row][col] ) {
            msg << "this->m3x3_ui16[" << row << "][" << col << "] (" << this->m3x3_ui16[row][col]
                << ") == (" << data.m3x3_ui16[row][col] << ") data.m3x3_ui16[" << row << "][" << col << "]" << endl;
         } else {
            msg << "this->m3x3_ui16[" << row << "][" << col << "] (" << this->m3x3_ui16[row][col]
                << ") != (" << data.m3x3_ui16[row][col] << ") data.m3x3_ui16[" << row << "][" << col << "]" << endl;
            equal_values = false;
         }
      }
   }

   int data1_ptr_ui16_size = get_size( this->ptr_ui16 );
   int data2_ptr_ui16_size = get_size( data.ptr_ui16 );
   if ( data1_ptr_ui16_size != data2_ptr_ui16_size ) {
      msg << "this->ptr_ui16 size (" << data1_ptr_ui16_size
          << ") != (" << data2_ptr_ui16_size << ") data.ptr_ui16 size" << endl;
      equal_values = false;
   } else {
      msg << "this->ptr_ui16 size (" << data1_ptr_ui16_size
          << ") == (" << data2_ptr_ui16_size << ") data.ptr_ui16 size" << endl;

      for ( int i = 0; i < data1_ptr_ui16_size; ++i ) {
         if ( this->ptr_ui16[i] == data.ptr_ui16[i] ) {
            msg << "this->ptr_ui16[" << i << "] (" << this->ptr_ui16[i]
                << ") == (" << data.ptr_ui16[i] << ") data.ptr_ui16[" << i << "]" << endl;
         } else {
            msg << "this->ptr_ui16[" << i << "] (" << this->ptr_ui16[i]
                << ") != (" << data.ptr_ui16[i] << ") data.ptr_ui16[" << i << "]" << endl;
            equal_values = false;
         }
      }
   }

   explanation = msg.str();

   return equal_values;
}

string UInt16Data::to_string()
{
   ostringstream msg;
   msg << "UInt16Data::to_string():" << __LINE__ << endl
       << "ui16:" << ui16 << endl;

   for ( int i = 0; i < 3; ++i ) {
      msg << "vec3_ui16[" << i << "]:" << vec3_ui16[i] << " ";
   }
   msg << endl;

   for ( int row = 0; row < 3; ++row ) {
      for ( int col = 0; col < 3; ++col ) {
         msg << "m3x3_ui16[" << row << "][" << col << "]:" << m3x3_ui16[row][col] << " ";
      }
   }
   msg << endl;

   int ptr_ui16_size = get_size( ptr_ui16 );
   msg << "ptr_ui16 size:" << ptr_ui16_size << endl;
   for ( int i = 0; i < ptr_ui16_size; ++i ) {
      msg << "ptr_ui16[" << i << "]:" << ptr_ui16[i] << " ";
   }
   msg << endl;

   return msg.str();
}
