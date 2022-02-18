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
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"

// TrickHLA include files.
#include "TrickHLA/Item.hh"
#include "TrickHLA/ParameterItem.hh"

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
   int                                          parameter_index,
   RTI1516_NAMESPACE::VariableLengthData const *param_value )
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
         data = (unsigned char *)TMM_declare_var_1d( "unsigned char", (int)size );
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
      TMM_delete_var_a( data );
      data  = NULL;
      size  = 0;
      index = -1;
   }
}
