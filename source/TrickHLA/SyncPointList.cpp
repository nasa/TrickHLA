/*!
@file TrickHLA/SyncPointList.cpp
@ingroup TrickHLA
@brief This class will manage different lists of HLA synchronization points. It
       is intended for this class to be extended by an Execution Control class.

@copyright Copyright 2024 United States Government as represented by the
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
@trick_link_dependency{SyncPoint.cpp}
@trick_link_dependency{SyncPointList.cpp}
@trick_link_dependency{SyncPointTimed.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{time/Int64Time.cpp}
@trick_link_dependency{utils/Utilities.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, June 2024, --, Initial implementation.}
@revs_end

*/

// System includes.
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/MemoryServices.hh"
#include "TrickHLA/SyncPoint.hh"
#include "TrickHLA/SyncPointList.hh"
#include "TrickHLA/SyncPointTimed.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/utils/StringUtilities.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/VariableLengthData.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

using namespace RTI1516_NAMESPACE;
using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
SyncPointList::SyncPointList()
   : list(),
     list_name()
{
   return;
}

/*!
 * @job_class{initialization}
 */
SyncPointList::SyncPointList(
   string const &name )
   : list(),
     list_name( name )
{
   return;
}

/*!
 * @details This is a pure virtual destructor.
 * @job_class{shutdown}
 */
SyncPointList::~SyncPointList()
{
   clear();
}

void SyncPointList::set_list_name(
   string const &name )
{
   this->list_name = name;
}

string &SyncPointList::get_list_name()
{
   return this->list_name;
}

void SyncPointList::clear()
{
   // Clear/remove everything from the list.
   while ( !list.empty() ) {
      MemoryServices::delete_var( list.back() );
      list.back() = nullptr;
      list.pop_back();
   }
}

SyncPoint *SyncPointList::get(
   wstring const &label )
{
#if defined( TRICKHLA_USE_STL_ALGORITHM )
   auto found_it = std::find_if( list.begin(), list.end(),
                                 [&label]( SyncPoint *sp ) -> bool {
                                    return ( label.compare( sp->get_label() ) == 0 );
                                 } );
   return ( ( found_it != list.end() ) ? *found_it : nullptr );
#else
   for ( SyncPoint *sp : list ) {
      if ( label.compare( sp->get_label() ) == 0 ) {
         return sp;
      }
   }
   return nullptr;
#endif // TRICKHLA_USE_STL_ALGORITHM
}

bool SyncPointList::add(
   wstring const &label )
{
   string label_str;
   StringUtilities::to_string( label_str, label );

   if ( contains( label ) ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::add():" << __LINE__
             << " ERROR: The sync-point label '" << label_str
             << "' has already been added!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Add the sync-point to the corresponding named list.
   // Using a named allocation makes the checkpoint file easier to work with.
   size_t       cdims[]         = { 1 };
   string const sync_point_name = string( "SyncPoint_" ) + label_str;
   SyncPoint   *sp              = nullptr;
   sp                           = MemoryServices::declare_var( sp,
                                                               "TrickHLA::SyncPoint",
                                                               0,
                                                               sync_point_name,
                                                               1,
                                                               cdims );

   if ( sp == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::add():" << __LINE__
             << " ERROR: Cannot allocate Trick Managed Memory for TrickHLA::SyncPoint with label '"
             << label_str << "'!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   sp->set_label( label );
   list.push_back( sp );

   return true;
}

bool SyncPointList::add(
   wstring const   &label,
   Int64Time const &time )
{
   string label_str;
   StringUtilities::to_string( label_str, label );

   if ( contains( label ) ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::add():" << __LINE__
             << " ERROR: The sync-point label '" << label_str
             << "' has already been added!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   // Add the sync-point to the corresponding named list.
   size_t          cdims[]         = { 1 };
   string const    sync_point_name = string( "SyncPointTimed_" ) + label_str;
   SyncPointTimed *sp              = nullptr;
   sp                              = MemoryServices::declare_var( sp,
                                                                  "TrickHLA::SyncPointTimed",
                                                                  0,
                                                                  sync_point_name,
                                                                  1,
                                                                  cdims );
   if ( sp == nullptr ) {
      ostringstream errmsg;
      errmsg << "SyncPointList::add():" << __LINE__
             << " ERROR: Cannot allocate Trick Managed Memory for TrickHLA::SyncPointTimed with label '"
             << label_str << "' and time " << time.to_string() << "!\n";
      DebugHandler::terminate( errmsg.str() );
      return false;
   }

   sp->set_label( label );
   sp->set_time( time );
   list.push_back( sp );

   return true;
}

bool SyncPointList::contains(
   wstring const &label )
{
#if defined( TRICKHLA_USE_STL_ALGORITHM )
   return std::any_of( list.begin(), list.end(),
                       [&label]( SyncPoint const *sp ) -> bool {
                          return ( label.compare( sp->get_label() ) == 0 );
                       } );
#else
   for ( SyncPoint const *sp : list ) {
      if ( label.compare( sp->get_label() ) == 0 ) {
         return true;
      }
   }
   return false;
#endif // TRICKHLA_USE_STL_ALGORITHM
}

bool SyncPointList::is_registered(
   wstring const &label )
{
   SyncPoint const *sp = get( label );
   return ( ( sp != nullptr ) && sp->is_registered() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPointList::mark_registered(
   wstring const &label )
{
   SyncPoint *sp = get( label );
   if ( sp != nullptr ) {
      sp->mark_registered();
      return true;
   }
   return false;
}

bool SyncPointList::is_announced(
   wstring const &label )
{
   SyncPoint const *sp = get( label );
   return ( ( sp != nullptr ) && sp->is_announced() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPointList::mark_announced(
   wstring const            &label,
   VariableLengthData const &user_supplied_tag )
{
   SyncPoint *sp = get( label );
   if ( sp != nullptr ) {
      sp->mark_announced( user_supplied_tag );
      return true;
   }
   return false;
}

bool SyncPointList::is_achieved(
   wstring const &label )
{
   SyncPoint const *sp = get( label );
   return ( ( sp != nullptr ) && sp->is_achieved() );
}

bool SyncPointList::is_synchronized(
   wstring const &label )
{
   SyncPoint const *sp = get( label );
   return ( ( sp != nullptr ) && sp->is_synchronized() );
}

bool SyncPointList::is_all_synchronized()
{
#if defined( TRICKHLA_USE_STL_ALGORITHM )
   if ( std::any_of( list.begin(), list.end(),
                     []( SyncPoint const *sp ) -> bool {
                        return !sp->is_synchronized();
                     } ) ) {
      return false;
   }
#else
   for ( SyncPoint const *sp : list ) {
      if ( !sp->is_synchronized() ) {
         return false;
      }
   }
#endif // TRICKHLA_USE_STL_ALGORITHM

   // Can only be synchronized if the list was not empty and we had no
   // unsynchronized sync-points.
   return ( !empty() );
}

/*!
 * @job_class{initialization}
 */
bool SyncPointList::mark_synchronized(
   wstring const &label )
{
   SyncPoint *sp = get( label );
   if ( sp != nullptr ) {

      // Mark the synchronization point at achieved which indicates the
      // federation is synchronized on the synchronization point.
      sp->mark_synchronized();
      return true;
   }
   return false;
}

string SyncPointList::to_string()
{
   unsigned int list_index = 0;

   ostringstream msg;

   msg << "SyncPointList::to_string():" << __LINE__
       << " List:'" << get_list_name() << "' List-size:" << list.size() << "\n";
   for ( SyncPoint *sp : list ) {
      msg << list_index++ << ":'" << get_list_name() << "' Sync-point:"
          << sp->to_string() << "\n";
   }
   return msg.str();
}

string SyncPointList::to_string(
   wstring const &label )
{
   SyncPoint *sp = get( label );
   if ( sp != nullptr ) {
      return sp->to_string();
   }

   string label_str;
   StringUtilities::to_string( label_str, label );
   ostringstream msg;
   msg << "SyncPointList::to_string():" << __LINE__
       << " Unknown sync-point label: '" << label_str << "'\n";
   return msg.str();
}

/*! @brief Encode the variables to a form Trick can checkpoint. */
void SyncPointList::convert_data_before_checkpoint()
{
   for ( SyncPoint *sp : list ) {
      sp->convert_data_before_checkpoint();
   }
}

/*! @brief Decode the state of this class from the Trick checkpoint. */
void SyncPointList::restore_data_after_checkpoint()
{
   for ( SyncPoint *sp : list ) {
      sp->restore_data_after_checkpoint();
   }
}

/*! @brief Free/release the memory used for the checkpoint data structures. */
void SyncPointList::free_converted_data_for_checkpoint()
{
   for ( SyncPoint *sp : list ) {
      sp->free_converted_data_for_checkpoint();
   }
}
