/*!
@file TrickHLA/ParameterItem.cpp
@ingroup TrickHLA
@brief This class represents a queue for holding HLA parameters.

@copyright Copyright 2019 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@tldh
@trick_link_dependency{Item.cpp}
@trick_link_dependency{ParameterItem.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA/ER7, TrickHLA, Feb 2009, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/Item.hh"
#include "TrickHLA/ParameterItem.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
// HLA include files.
#include RTI1516_HEADER
#pragma GCC diagnostic pop

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
@job_class{initialization}
*/
ParameterItem::ParameterItem()
   : index( -1 ),
     size( 0 ),
     data( NULL )
{
   return;
}

/*!
@job_class{initialization}
*/
ParameterItem::ParameterItem(
   int const                 parameter_index,
   VariableLengthData const *param_value )
   : index( parameter_index ),
     size( 0 ),
     data( NULL )
{
   if ( param_value != NULL ) {
      // Put the user supplied tag into a buffer.
      size = param_value->size();
      if ( size == 0 ) {
         data = NULL;
      } else {
         data = static_cast< unsigned char * >( TMM_declare_var_1d( "unsigned char", size ) );
         memcpy( data, param_value->data(), size );
      }
   }
}

/*!
@job_class{shutdown}
*/
ParameterItem::~ParameterItem()
{
   clear();
}

void ParameterItem::clear()
{
   if ( data != NULL ) {
      if ( trick_MM->delete_var( static_cast< void * >( data ) ) ) {
         message_publish( MSG_WARNING, "ParameterItem::clear():%d WARNING failed to delete Trick Memory for 'data'\n", __LINE__ );
      }
      data  = NULL;
      size  = 0;
      index = -1;
   }
}
