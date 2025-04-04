/*!
@file TrickHLA/InteractionItem.cpp
@ingroup TrickHLA
@brief This class represents a queue for holding HLA Interactions of either
Timestamp Order (TSO) or Receive Order (RO).

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
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{InteractionItem.cpp}
@trick_link_dependency{MutexLock.cpp}
@trick_link_dependency{MutexProtection.cpp}
@trick_link_dependency{Parameter.cpp}
@trick_link_dependency{ParameterItem.cpp}
@trick_link_dependency{Types.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, May 2007, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System include files.
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>

// Trick include files.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"
#include "trick/message_proto.h"

// TrickHLA include files.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/InteractionItem.hh"
#include "TrickHLA/MutexLock.hh"
#include "TrickHLA/MutexProtection.hh"
#include "TrickHLA/Parameter.hh"
#include "TrickHLA/ParameterItem.hh"
#include "TrickHLA/Types.hh"

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
 * @job_class{initialization}
 */
InteractionItem::InteractionItem() // RETURN: -- None.
   : index( -1 ),
     parameter_queue(),
     interaction_type( INTERACTION_TYPE_UNDEFINED ),
     parm_items_count( 0 ),
     parm_items( NULL ),
     user_supplied_tag_size( 0 ),
     user_supplied_tag( NULL ),
     order_is_TSO( false ),
     time()
{
   return;
}

/*!
 * @job_class{initialization}
 */
InteractionItem::InteractionItem(
   int const                      inter_index,
   InteractionTypeEnum const      inter_type,
   int const                      param_count,
   Parameter                     *parameters,
   ParameterHandleValueMap const &theParameterValues,
   RTI1516_USERDATA const        &theUserSuppliedTag )
   : index( inter_index ),
     parameter_queue(),
     interaction_type( inter_type ),
     parm_items_count( 0 ),
     parm_items( NULL ),
     order_is_TSO( false ),
     time()
{
   // Decode the Interaction values into this Item.
   initialize( inter_type, param_count, parameters, theParameterValues, theUserSuppliedTag );
}

/*!
 * @job_class{initialization}
 */
InteractionItem::InteractionItem(
   int const                      inter_index,
   InteractionTypeEnum const      inter_type,
   int const                      param_count,
   Parameter                     *parameters,
   ParameterHandleValueMap const &theParameterValues,
   RTI1516_USERDATA const        &theUserSuppliedTag,
   LogicalTime const             &theTime )
   : index( inter_index ),
     parameter_queue(),
     interaction_type( inter_type ),
     parm_items_count( 0 ),
     parm_items( NULL ),
     order_is_TSO( true ),
     time()
{
   time.set( theTime );

   // Decode the Interaction values into this Item.
   initialize( inter_type, param_count, parameters, theParameterValues, theUserSuppliedTag );
}

/*!
 * @job_class{shutdown}
 */
InteractionItem::~InteractionItem()
{
   if ( user_supplied_tag != NULL ) {
      if ( trick_MM->is_alloced( static_cast< void * >( user_supplied_tag ) )
           && trick_MM->delete_var( static_cast< void * >( user_supplied_tag ) ) ) {
         message_publish( MSG_WARNING, "InteractionItem::~InteractionItem():%d WARNING failed to delete Trick Memory for 'user_supplied_tag'\n",
                          __LINE__ );
      }
      user_supplied_tag      = NULL;
      user_supplied_tag_size = 0;
   }
   clear_parm_items();
}

/*!
 * @job_class{initialization}
 */
void InteractionItem::initialize(
   InteractionTypeEnum const      inter_type,
   int const                      param_count,
   Parameter                     *parameters,
   ParameterHandleValueMap const &theParameterValues,
   RTI1516_USERDATA const        &theUserSuppliedTag )
{
   this->interaction_type = inter_type;

   // Decode all the parameters from the map.
   for ( int p = 0; p < param_count; ++p ) {
      // Note that we are using a const_iterator since this map does not support
      // an iterator.
      ParameterHandleValueMap::const_iterator param_iter;

      // Get the parameter from the map.
      param_iter = theParameterValues.find( parameters[p].get_parameter_handle() );

      if ( param_iter != theParameterValues.end() ) {
         ParameterItem *item;
         item = new ParameterItem( p, &( param_iter->second ) );
         parameter_queue.push( item );
      }
   }

   // Free the Trick allocated memory for the user supplied tag.
   if ( user_supplied_tag != NULL ) {
      if ( trick_MM->is_alloced( static_cast< void * >( user_supplied_tag ) )
           && trick_MM->delete_var( static_cast< void * >( user_supplied_tag ) ) ) {
         message_publish( MSG_WARNING, "InteractionItem::initialize():%d WARNING failed to delete Trick Memory for 'user_supplied_tag'\n",
                          __LINE__ );
      }
      user_supplied_tag = NULL;
   }

   // Put the user supplied tag into a buffer.
   user_supplied_tag_size = theUserSuppliedTag.size();
   if ( user_supplied_tag_size != 0 ) {
      user_supplied_tag = static_cast< unsigned char * >( TMM_declare_var_1d( "unsigned char", user_supplied_tag_size ) );
      memcpy( user_supplied_tag, theUserSuppliedTag.data(), user_supplied_tag_size );
   }
}

void InteractionItem::checkpoint_queue()
{
   if ( parameter_queue.size() != 0 ) {
      parm_items_count = parameter_queue.size();

      parm_items = reinterpret_cast< ParameterItem * >(
         alloc_type( parm_items_count, "TrickHLA::ParameterItem" ) );
      if ( parm_items == NULL ) {
         ostringstream errmsg;
         errmsg << "InteractionItem::checkpoint_queue():" << __LINE__
                << " ERROR: Failed to allocate enough memory for a parm_items linear"
                << " array of " << parm_items_count << " elements\n";
         DebugHandler::terminate_with_message( errmsg.str() );
      }

      // When auto_unlock_mutex goes out of scope it automatically unlocks the
      // mutex even if there is an exception.
      MutexProtection auto_unlock_mutex( &parameter_queue.mutex );

      for ( int i = 0; i < parm_items_count; ++i ) {

         ParameterItem *item = static_cast< ParameterItem * >( parameter_queue.front() );

         parm_items[i].index = item->index;
         parm_items[i].size  = item->size;
         if ( item->size == 0 ) {
            parm_items[i].data = NULL;
         } else {
            parm_items[i].data = static_cast< unsigned char * >( TMM_declare_var_1d( "unsigned char", item->size ) );
            memcpy( parm_items[i].data, item->data, item->size );
         }

         // Now that we extracted the data from the parameter-item, point to the
         // next element in the queue, without popping!
         parameter_queue.next( item );
      }

      // auto_unlock_mutex releases mutex lock here
   }
}

void InteractionItem::clear_parm_items()
{
   if ( ( parm_items_count != 0 ) && ( parm_items != NULL ) ) {
      for ( int i = 0; i < parm_items_count; ++i ) {
         parm_items[i].clear();
      }
      if ( trick_MM->is_alloced( static_cast< void * >( parm_items ) )
           && trick_MM->delete_var( static_cast< void * >( parm_items ) ) ) {
         message_publish( MSG_WARNING, "InteractionItem::clear_parm_items():%d WARNING failed to delete Trick Memory for 'parm_items'\n",
                          __LINE__ );
      }
      parm_items       = NULL;
      parm_items_count = 0;
   }
}

void InteractionItem::restore_queue()
{
   if ( parm_items_count != 0 ) {

      for ( int i = 0; i < parm_items_count; ++i ) {

         ParameterItem *item = new ParameterItem();

         item->index = parm_items[i].index;
         item->size  = parm_items[i].size;
         if ( parm_items[i].size == 0 ) {
            item->data = NULL;
         } else {
            item->data = static_cast< unsigned char * >( TMM_declare_var_1d( "unsigned char", parm_items[i].size ) );
            memcpy( item->data, parm_items[i].data, parm_items[i].size );
         }

         parameter_queue.push( item );
      }
   }
}
