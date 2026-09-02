/*!
@file TrickHLA/OwnershipHandler.cpp
@ingroup TrickHLA
@brief This class represents ownership transfer of HLA attributes for a
specific object.

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
@trick_link_dependency{Attribute.cpp}
@trick_link_dependency{DebugHandler.cpp}
@trick_link_dependency{ExecutionControlBase.cpp}
@trick_link_dependency{Federate.cpp}
@trick_link_dependency{Object.cpp}
@trick_link_dependency{OwnershipHandler.cpp}
@trick_link_dependency{Types.cpp}
@trick_link_dependency{time/Int64BaseTime.cpp}
@trick_link_dependency{time/Int64Interval.cpp}
@trick_link_dependency{time/Int64Time.cpp}
@trick_link_dependency{utils/MutexLock.cpp}
@trick_link_dependency{utils/MutexProtection.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, December 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

// System includes.
#include <cstddef>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

// Trick includes.
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/Attribute.hh"
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/ExecutionControlBase.hh"
#include "TrickHLA/Federate.hh"
#include "TrickHLA/MemoryServices.hh"
#include "TrickHLA/Object.hh"
#include "TrickHLA/OwnershipHandler.hh"
#include "TrickHLA/OwnershipItem.hh"
#include "TrickHLA/Types.hh"
#include "TrickHLA/time/Int64Interval.hh"
#include "TrickHLA/time/Int64Time.hh"
#include "TrickHLA/utils/MutexProtection.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @job_class{initialization}
 */
OwnershipHandler::OwnershipHandler()
   : object( nullptr ),
     pull_requests(),
     push_requests(),
     pull_items_cnt( 0 ),
     pull_items( nullptr ),
     push_items_cnt( 0 ),
     push_items( nullptr )
{
   return;
}

/*!
 * @job_class{shutdown}
 */
OwnershipHandler::~OwnershipHandler()
{
   return;
}

void OwnershipHandler::convert_data_before_checkpoint()
{
   // Lock the ownership mutex since we are processing the ownership list.
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &object->ownership_mutex );

   AttributeOwnershipMap::const_iterator owner_map_iter;
   THLAAttributeMap::const_iterator      attrib_iter;

   pull_items_cnt = 0;
   push_items_cnt = 0;

   // Count the # of pull_request elements that need to be checkpointed
   if ( !pull_requests.empty() ) {

      // Count the number of elements to allocate
      for ( owner_map_iter = pull_requests.begin(); owner_map_iter != pull_requests.end(); ++owner_map_iter ) {
         pull_items_cnt += owner_map_iter->second->size();
      }
   }

   // If there are any pull_request entries, encode them to get checkpointed.
   if ( pull_items_cnt > 0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OWNERSHIP ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      string( "Checkpointing " ).append( std::to_string( pull_items_cnt ) ).append( " pull_request elements.\n" ) );
      }
      pull_items = MemoryServices::declare_var( pull_items, pull_items_cnt );
      if ( pull_items == nullptr ) {
         DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__,
                                  "Could not allocate memory for pull_items (array of OwnershipItem type)!\n" );
      }

      // FIXME: Is this needed anymore?
      // Now, encode them to get checkpointed.
      int count = 0;
      for ( owner_map_iter = pull_requests.begin(); owner_map_iter != pull_requests.end(); ++owner_map_iter ) {
         double const      curr_time = owner_map_iter->first;
         THLAAttributeMap *tMap      = owner_map_iter->second;
         for ( attrib_iter = tMap->begin(); attrib_iter != tMap->end(); ++attrib_iter ) {
            pull_items[count].time     = curr_time;
            pull_items[count].FOM_name = MemoryServices::cstrdup( attrib_iter->first.c_str() );
            ++count;
         }
      }
   }

   // Count the # of push_request elements that need to be checkpointed
   if ( !push_requests.empty() ) {

      // count the number of elements to allocate
      for ( owner_map_iter = push_requests.begin(); owner_map_iter != push_requests.end(); ++owner_map_iter ) {
         push_items_cnt += owner_map_iter->second->size();
      }
   }

   // If there are any push_request entries, encode them to get checkpointed.
   if ( push_items_cnt > 0 ) {
      if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OWNERSHIP ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      string( "Checkpointing " ).append( std::to_string( push_items_cnt ) ).append( " push_request elements.\n" ) );
      }
      push_items = MemoryServices::declare_var( push_items, push_items_cnt );
      if ( push_items == nullptr ) {
         DebugHandler::terminate( __PRETTY_FUNCTION__, __LINE__,
                                  "Could not allocate memory for push_items (array of OwnershipItem type)!\n" );
      }

      // Now, encode them to get checkpointed.
      int count = 0;
      for ( owner_map_iter = push_requests.begin(); owner_map_iter != push_requests.end(); ++owner_map_iter ) {
         double const      curr_time = owner_map_iter->first;
         THLAAttributeMap *tMap      = owner_map_iter->second;
         for ( attrib_iter = tMap->begin(); attrib_iter != tMap->end(); ++attrib_iter ) {
            push_items[count].time     = curr_time;
            push_items[count].FOM_name = MemoryServices::cstrdup( attrib_iter->first.c_str() );
            ++count;
         }
      }
   }
}

void OwnershipHandler::restore_data_after_checkpoint()
{
   THLAAttributeMap                     *attr_map;
   AttributeOwnershipMap::const_iterator ownership_iter;

   // Lock the ownership mutex since we are processing the ownership list.
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &object->ownership_mutex );

   // Decode all the ownership-items in the pull_items.
   if ( pull_items_cnt > 0 ) {

      for ( int count = 0; count < pull_items_cnt; ++count ) {

         double const time = pull_items[count].time;
         ownership_iter    = pull_requests.find( time );

         // Determine if there is an existing attribute map for the time. If not, add one.
         if ( ownership_iter == pull_requests.end() ) {
            attr_map            = new THLAAttributeMap();
            pull_requests[time] = attr_map;
         } else {
            attr_map = ownership_iter->second;
         }

         // Get the attribute from the Object
         Attribute *attribute = get_attribute( pull_items[count].FOM_name );

         // Add the attribute to the map, using the FOM_name as the key.
         attr_map->insert( make_pair( pull_items[count].FOM_name, attribute ) );

         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OWNERSHIP ) ) {
            DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                         string( "Restoring ownership pull item attribute '" ).append( pull_items[count].FOM_name ).append( "'\n" ) );
         }
      }
   }

   // Decode all the ownership-items in the push_items.
   if ( push_items_cnt > 0 ) {

      for ( int count = 0; count < push_items_cnt; ++count ) {

         double const time = push_items[count].time;
         ownership_iter    = push_requests.find( time );

         // Determine if there is an existing attribute map for the time. If not, add one.
         if ( ownership_iter == push_requests.end() ) {
            attr_map            = new THLAAttributeMap();
            push_requests[time] = attr_map;
         } else {
            attr_map = ownership_iter->second;
         }

         // Get the attribute from the Object
         Attribute *attribute = get_attribute( push_items[count].FOM_name );

         // Add the attribute to the map, using the FOM_name as the key.
         attr_map->insert( make_pair( push_items[count].FOM_name, attribute ) );

         if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OWNERSHIP ) ) {
            DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                         string( "Restoring ownership push item attribute '" ).append( push_items[count].FOM_name ).append( "'\n" ) );
         }
      }
   }
}

void OwnershipHandler::free_converted_data_for_checkpoint()
{
   // If there are any pull_request entries, delete them
   if ( pull_items_cnt > 0 ) {
      for ( int i = 0; i < pull_items_cnt; ++i ) {
         pull_items[i].clear();
      }
      if ( !MemoryServices::delete_var( pull_items ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      "Failed to delete Trick Memory for 'pull_items'\n", MSG_WARNING );
      }
      pull_items     = nullptr;
      pull_items_cnt = 0;
   }

   // If there are any push_request entries, delete them
   if ( push_items_cnt > 0 ) {
      for ( int i = 0; i < push_items_cnt; ++i ) {
         push_items[i].clear();
      }
      if ( !MemoryServices::delete_var( push_items ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      "Failed to delete Trick Memory for 'push_items'\n", MSG_WARNING );
      }
      push_items     = nullptr;
      push_items_cnt = 0;
   }
}

void OwnershipHandler::initialize_callback(
   Object *obj )
{
   this->object = obj;
}

string OwnershipHandler::get_object_name() const
{
   return ( ( this->object != nullptr ) ? object->get_name() : "" );
}

string OwnershipHandler::get_object_FOM_name() const
{
   return ( ( this->object != nullptr ) ? object->get_FOM_name() : "" );
}

int OwnershipHandler::get_attribute_count() const
{
   return ( ( this->object != nullptr ) ? object->get_attribute_count() : 0 );
}

VectorOfStrings const OwnershipHandler::get_attribute_FOM_names() const
{
   return ( ( this->object != nullptr ) ? object->get_attribute_FOM_names() : VectorOfStrings() );
}

Attribute *OwnershipHandler::get_attribute(
   string const &attribute_FOM_name )
{
   return ( ( object != nullptr ) ? object->get_attribute( attribute_FOM_name ) : nullptr );
}

bool OwnershipHandler::is_locally_owned(
   string const &attribute_FOM_name )
{
   Attribute const *attribute = get_attribute( attribute_FOM_name );
   return ( ( attribute != nullptr ) ? attribute->is_locally_owned() : false );
}

bool OwnershipHandler::is_remotely_owned(
   string const &attribute_FOM_name )
{
   Attribute const *attribute = get_attribute( attribute_FOM_name );
   return ( ( attribute != nullptr ) ? attribute->is_remotely_owned() : false );
}

bool OwnershipHandler::is_published(
   string const &attribute_FOM_name )
{
   Attribute const *attribute = get_attribute( attribute_FOM_name );
   return ( ( attribute != nullptr ) ? attribute->is_publish() : false );
}

bool OwnershipHandler::is_subscribed(
   string const &attribute_FOM_name )
{
   Attribute const *attribute = get_attribute( attribute_FOM_name );
   return ( ( attribute != nullptr ) ? attribute->is_subscribe() : false );
}

void OwnershipHandler::pull_ownership()
{
   pull_ownership( std::numeric_limits< double >::lowest() );
}

void OwnershipHandler::pull_ownership(
   double time )
{
   if ( this->object == nullptr ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OWNERSHIP ) ) {
      ostringstream msg;
      msg << "(Object:'" << object->get_name()
          << "', Pull at time:" << setprecision( 18 ) << time
          << ") scenario-time:" << setprecision( 18 ) << get_scenario_time()
          << ", granted_time:" << setprecision( 18 ) << get_granted_time().get_time_in_seconds()
          << ", lookahead:" << setprecision( 18 ) << get_lookahead().get_time_in_seconds() << "\n";
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
   }

   THLAAttributeMap *attr_map;

   // Lock the ownership mutex since we are processing the ownership list.
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &object->ownership_mutex );

   // Find the attribute map for the specified time.
   AttributeOwnershipMap::const_iterator const attr_map_iter = pull_requests.find( time );

   // Determine if there is an existing attribute map for the time, if not add one.
   if ( attr_map_iter == pull_requests.end() ) {
      attr_map            = new THLAAttributeMap();
      pull_requests[time] = attr_map;
   } else {
      attr_map = attr_map_iter->second;
   }

   int const  attr_cnt   = object->get_attribute_count();
   Attribute *attributes = object->get_attributes();

   // The user has requested to pull the ownership of all the attributes.
   for ( int i = 0; i < attr_cnt; ++i ) {

      // Use the Attribute FOM name as the key instead of the AttributeHandle
      // because the handle may not have been initialized yet.
      string const key( attributes[i].get_FOM_name() );

      // Add the attribute to the map.
      attr_map->insert( make_pair( key, &attributes[i] ) );
   }
}

void OwnershipHandler::pull_ownership( // RETURN: -- None.
   string const &attribute_FOM_name )  // IN: -- Attribute FOM name.
{
   pull_ownership( attribute_FOM_name, std::numeric_limits< double >::lowest() );
}

void OwnershipHandler::pull_ownership(
   std::string const &attribute_FOM_name,
   double             time )
{
   // Find the attribute for the given attribute FOM name.
   Attribute *attribute = get_attribute( attribute_FOM_name );

   // Just return if the attribute was not found.
   if ( attribute == nullptr ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OWNERSHIP ) ) {
      ostringstream msg;
      msg << "(Object:'" << ( ( object != nullptr ) ? object->get_name() : "NULL" )
          << "', Attribute:'" << attribute_FOM_name
          << "', Pull at time:" << setprecision( 18 ) << time
          << ") scenario-time:" << setprecision( 18 ) << get_scenario_time()
          << ", granted_time:" << setprecision( 18 ) << get_granted_time().get_time_in_seconds()
          << ", lookahead:" << setprecision( 18 ) << get_lookahead().get_time_in_seconds() << "\n";
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
   }

   THLAAttributeMap *attr_map;

   // Lock the ownership mutex since we are processing the ownership list.
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &object->ownership_mutex );

   // Find the attribute map for the specified time.
   AttributeOwnershipMap::const_iterator const attr_map_iter = pull_requests.find( time );

   // Determine if there is an existing attribute map for the time, if not add one.
   if ( attr_map_iter == pull_requests.end() ) {
      attr_map            = new THLAAttributeMap();
      pull_requests[time] = attr_map;
   } else {
      attr_map = attr_map_iter->second;
   }

   // Use the Attribute FOM name as the key instead of the AttributeHandle
   // because it may not have been initialized yet.
   string const key( attribute->get_FOM_name() );

   // Add the attribute to the map.
   attr_map->insert( make_pair( key, attribute ) );
}

void OwnershipHandler::push_ownership()
{
   push_ownership( std::numeric_limits< double >::lowest() );
}

void OwnershipHandler::push_ownership(
   double const time )
{
   if ( this->object == nullptr ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OWNERSHIP ) ) {
      ostringstream msg;
      msg << "(Object:'" << object->get_name()
          << "', Push at time:" << setprecision( 18 ) << time
          << ") scenario-time:" << setprecision( 18 ) << get_scenario_time()
          << ", granted_time:" << setprecision( 18 ) << get_granted_time().get_time_in_seconds()
          << ", lookahead:" << setprecision( 18 ) << get_lookahead().get_time_in_seconds() << "\n";
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
   }

   THLAAttributeMap *attr_map;

   // Lock the ownership mutex since we are processing the ownership list.
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &object->ownership_mutex );

   // Find the attribute map for the specified time.
   AttributeOwnershipMap::const_iterator const attr_map_iter = push_requests.find( time );

   // Determine if there is an existing attribute map for the time, if not add one.
   if ( attr_map_iter == push_requests.end() ) {
      attr_map            = new THLAAttributeMap();
      push_requests[time] = attr_map;
   } else {
      attr_map = attr_map_iter->second;
   }

   int const  attr_cnt   = object->get_attribute_count();
   Attribute *attributes = object->get_attributes();

   // The user has requested to push the ownership of all the attributes.
   for ( int i = 0; i < attr_cnt; ++i ) {

      // Use the Attribute FOM name as the key instead of the AttribyteHandle
      // because it may not have been initialized yet.
      string const key( attributes[i].get_FOM_name() );

      // Add the attribute to the map.
      attr_map->insert( make_pair( key, &attributes[i] ) );
   }
}

void OwnershipHandler::push_ownership(
   string const &attribute_FOM_name )
{
   push_ownership( attribute_FOM_name, std::numeric_limits< double >::lowest() );
}

void OwnershipHandler::push_ownership(
   string const &attribute_FOM_name,
   double        time )
{
   // Find the attribute for the given attribute FOM name.
   Attribute *attribute = get_attribute( attribute_FOM_name );

   // Just return if the attribute was not found.
   if ( attribute == nullptr ) {
      return;
   }

   if ( DebugHandler::show( DEBUG_LEVEL_3_TRACE, DEBUG_SOURCE_OWNERSHIP ) ) {
      ostringstream msg;
      msg << "(Object:'" << ( ( object != nullptr ) ? object->get_name() : "NULL" )
          << "', Attribute:'" << attribute_FOM_name
          << "', Push at time:" << setprecision( 18 ) << time
          << ") scenario-time:" << setprecision( 18 ) << get_scenario_time()
          << ", granted_time:" << setprecision( 18 ) << get_granted_time().get_time_in_seconds()
          << ", lookahead:" << setprecision( 18 ) << get_lookahead().get_time_in_seconds() << "\n";
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str() );
   }

   THLAAttributeMap *attr_map;

   // Lock the ownership mutex since we are processing the ownership list.
   // When auto_unlock_mutex goes out of scope it automatically unlocks the
   // mutex even if there is an exception.
   MutexProtection const auto_unlock_mutex( &object->ownership_mutex );

   // Find the attribute map for the specified time.
   AttributeOwnershipMap::const_iterator const attr_map_iter = push_requests.find( time );

   // Determine if there is an existing attribute map for the time, if not add one.
   if ( attr_map_iter == push_requests.end() ) {
      attr_map            = new THLAAttributeMap();
      push_requests[time] = attr_map;
   } else {
      attr_map = attr_map_iter->second;
   }

   // Use the Attribute FOM name as the key instead of the AttributeHandle
   // because it may not have been initialized yet.
   string const key( attribute->get_FOM_name() );

   // Add the attribute to the map.
   attr_map->insert( make_pair( key, attribute ) );
}

Int64Interval const &OwnershipHandler::get_lookahead() const
{
   return object->get_federate()->get_lookahead();
}

Int64Time const &OwnershipHandler::get_granted_time() const
{
   return object->get_federate()->get_granted_time();
}

double OwnershipHandler::get_scenario_time() const
{
   if ( object != nullptr ) {
      Federate *fed = object->get_federate();
      if ( fed != nullptr ) {
         ExecutionControlBase const *exec_control = fed->get_execution_control();
         if ( exec_control != nullptr ) {
            return exec_control->get_scenario_time();
         }
      }
   }
   return std::numeric_limits< double >::lowest();
}

double OwnershipHandler::get_cte_time() const
{
   if ( object != nullptr ) {
      Federate *fed = object->get_federate();
      if ( fed != nullptr ) {
         ExecutionControlBase const *exec_control = fed->get_execution_control();
         if ( exec_control != nullptr ) {
            return exec_control->get_cte_time();
         }
      }
   }
   return std::numeric_limits< double >::lowest();
}
