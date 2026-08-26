/*!
@file TrickHLA/MemoryServices_impl.hh
@ingroup TrickHLA
@brief The template implementations for the Memory management services.

@copyright Copyright 2026 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLA}

@tldh
@trick_link_dependency{../../source/TrickHLA/MemoryServices.cpp}
@trick_link_dependency{../../source/TrickHLA/DebugHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2026, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MEMORY_SERVICES_IMPL_HH
#define TRICKHLA_MEMORY_SERVICES_IMPL_HH

// System includes.
#include <climits>
#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/MemoryServices.hh" // NOLINT(misc-header-include-cycle)

// Disable unused parameter compiler warnings. Works for both GCC and Clang.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class Attribute;
class Interaction;
class InteractionItem;
class OwnershipItem;
class Parameter;
class ParameterItem;
class RecordElement;

template < typename T >
T MemoryServices::declare_var(
   T                  type,
   std::string const &class_name,
   size_t             n_stars,
   std::string const &var_name,
   size_t             n_cdims,
   size_t            *cdims )
{
   TRICK_TYPE trick_type = TRICK_STRUCTURED;

   // Check to make sure the incoming type is a pointer.
   // If not, then return null.
   if ( !std::is_pointer< T >::value ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      std::string( "Type is not a pointer: " )
                                         .append( typeid( T ).name() )
                                         .append( "\n" ),
                                      MSG_WARNING );
      }
      return ( nullptr );
   }

   // NOTE: This function does not support the following Trick types:
   // TRICK_VOID, TRICK_FILE_PTR, TRICK_ENUMERATED, TRICK_OPAQUE_TYPE,
   // or TRICK_STL.  It will treat everything that it cannot match as
   // a basic type as TRICK_STRUCTURED.

   // Get the equivalent Trick type.
   trick_type = static_cast< TRICK_TYPE >( get_trick_type( type ) );

   // Check that there is a class_name with any STRUTURED Trick type.
   if ( trick_type == TRICK_STRUCTURED ) {
      // Trick has to have a class name for structured types.
      if ( class_name.empty() ) {
         if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
            std::ostringstream msg;
            msg << "Empty class_name for type allocation: "
                << typeid( T ).name() << "\n";
            DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                         std::string( "Empty class_name for type allocation: " )
                                            .append( typeid( T ).name() )
                                            .append( "\n" ),
                                         MSG_ERROR );
         }
      }
   }

   // Perform the allocation and return the allocated value.
   return ( static_cast< T >( trick_MM->declare_var( trick_type,
                                                     class_name,
                                                     (int)n_stars,
                                                     var_name,
                                                     (int)n_cdims,
                                                     reinterpret_cast< int * >( cdims ) ) ) );
}

template < typename T >
T MemoryServices::declare_var(
   T                  type,
   size_t             n_elems,
   std::string const &var_name )
{
   // Check to make sure the incoming type is a pointer.
   // If not, then return null.
   if ( !std::is_pointer< T >::value ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      std::string( "Type is not a pointer: " )
                                         .append( typeid( T ).name() )
                                         .append( "\n" ),
                                      MSG_WARNING );
      }
      return ( nullptr );
   }
   return ( declare_var( type, get_class_name( type ), 0, var_name, 1, &n_elems ) );
}

template < typename T >
T MemoryServices::declare_var(
   T      type,
   size_t n_elems )
{
   // Check to make sure the incoming type is a pointer.
   // If not, then return null.
   if ( !std::is_pointer< T >::value ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      std::string( "Type is not a pointer: " )
                                         .append( typeid( T ).name() )
                                         .append( "\n" ),
                                      MSG_WARNING );
      }
      return ( nullptr );
   }
   return ( declare_var( type, get_class_name( type ), 0, "", 1, &n_elems ) );
}

template < typename T >
T MemoryServices::declare_var(
   T                  type,
   std::string const &enh_type_spec,
   size_t             n_elems )
{
   // Check to make sure the incoming type is a pointer.
   // If not, then return null.
   if ( !std::is_pointer< T >::value ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      std::string( "Type is not a pointer: " )
                                         .append( typeid( T ).name() )
                                         .append( "\n" ),
                                      MSG_WARNING );
      }
      return ( nullptr );
   }
   return ( static_cast< T >( trick_MM->declare_var( enh_type_spec.c_str(), (int)n_elems ) ) );
}

template < typename T >
T MemoryServices::declare_var(
   T                  type,
   std::string const &declaration )
{
   // Check to make sure the incoming type is a pointer.
   // If not, then return null.
   if ( !std::is_pointer< T >::value ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      std::string( "Type is not a pointer: " )
                                         .append( typeid( T ).name() )
                                         .append( "\n" ),
                                      MSG_WARNING );
      }
      return ( nullptr );
   }
   return ( static_cast< T >( trick_MM->declare_var( declaration.c_str() ) ) );
}

template < typename T >
T MemoryServices::resize_array( T address, size_t n_elems )
{
   // Check to make sure the incoming type is a pointer.
   // If not, then return null.
   if ( !std::is_pointer< T >::value ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      std::string( "Type is not a pointer: " )
                                         .append( typeid( T ).name() )
                                         .append( "\n" ),
                                      MSG_WARNING );
      }
      return ( nullptr );
   }

   return ( static_cast< T >( trick_MM->resize_array( static_cast< void * >( address ), (int)n_elems ) ) );
}

template < typename T >
bool MemoryServices::delete_var( T addr )
{
   // Check to make sure the incoming type is a pointer.
   // If not, then return null.
   if ( !std::is_pointer< T >::value ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      std::string( "Type is not a pointer: " )
                                         .append( typeid( T ).name() )
                                         .append( "\n" ),
                                      MSG_WARNING );
      }
      return ( false );
   }

   // Delete the address in Trick memory.
   // NOTE: trick_MM->delete_var returns 0 on success and 1 on failure!
   if ( trick_MM->delete_var( static_cast< void * >( addr ) ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         std::ostringstream msg;
         msg << "Could not delete variable at address: " << std::hex << addr << "\n";
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__, msg.str(), MSG_WARNING );
      }
      return ( false );
   }

   return ( true );
}

template < typename T >
bool MemoryServices::is_alloced( T addr )
{
   // Check to make sure the incoming type is a pointer.
   // If not, then return false.
   if ( !std::is_pointer< T >::value ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      std::string( "Type is not a pointer: " )
                                         .append( typeid( T ).name() )
                                         .append( "\n" ),
                                      MSG_WARNING );
      }
      return ( false );
   }

   // Call the Trick Memory Manager function.
   return ( trick_MM->is_alloced( static_cast< void * >( addr ) ) );
}

template < typename T >
std::size_t MemoryServices::get_size( T addr )
{
   // Check to make sure the incoming type is a pointer.
   // If not, then return 0.
   if ( !std::is_pointer< T >::value ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                      std::string( "Type is not a pointer: " )
                                         .append( typeid( T ).name() )
                                         .append( "\n" ),
                                      MSG_WARNING );
      }
      return ( 0 );
   }

   // The Trick get_size() returns the number of elements in a dynamic array.
   int num_items = trick_MM->get_size( static_cast< void * >( addr ) );

   // If this isn't a dynamic array, get the information from the
   // Trick allocation information.
   if ( num_items <= 0 ) {
      // Get the allocation info that contains the variable address.
      ALLOC_INFO const *alloc_info = trick_MM->get_alloc_info_of( static_cast< void * >( addr ) );
      if ( alloc_info != nullptr ) {
         num_items = alloc_info->num;
      }
   }

   // Get a safe size value.
   std::size_t size = ( num_items >= 0 ) ? num_items : 0;

   return ( size );
}

template < typename T >
std::string MemoryServices::get_class_name( T type )
{
   // Get the Trick type.
   TRICK_TYPE trick_type = static_cast< TRICK_TYPE >( get_trick_type( type ) );
   if ( trick_type != TRICK_STRUCTURED ) {
      // Only the TRICK_STRUCTURED type requires a class name.
      return ( std::string( "" ) );
   }

   // Check for specific TrickHLA type matches.
   if ( std::is_same< T, TrickHLA::Attribute * >::value ) {
      return ( std::string( "TrickHLA::Attribute" ) );
   } else if ( std::is_same< T, TrickHLA::Interaction * >::value ) {
      return ( std::string( "TrickHLA::Interaction" ) );
   } else if ( std::is_same< T, TrickHLA::InteractionItem * >::value ) {
      return ( std::string( "TrickHLA::InteractionItem" ) );
   } else if ( std::is_same< T, TrickHLA::OwnershipItem * >::value ) {
      return ( std::string( "TrickHLA::OwnershipItem" ) );
   } else if ( std::is_same< T, TrickHLA::Parameter * >::value ) {
      return ( std::string( "TrickHLA::Parameter" ) );
   } else if ( std::is_same< T, TrickHLA::ParameterItem * >::value ) {
      return ( std::string( "TrickHLA::ParameterItem" ) );
   } else if ( std::is_same< T, TrickHLA::RecordElement * >::value ) {
      return ( std::string( "TrickHLA::RecordElement" ) );
   }

   if ( DebugHandler::show( DEBUG_LEVEL_4_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
      DebugHandler::print_message( __PRETTY_FUNCTION__, __LINE__,
                                   std::string( "Unrecognized TrickHLA type: " )
                                      .append( typeid( T ).name() )
                                      .append( "\n" ),
                                   MSG_WARNING );
   }

   return ( std::string( "" ) );
}

template < typename T >
int MemoryServices::get_trick_type( T type )
{
   TRICK_TYPE trick_type = TRICK_STRUCTURED;

   // NOTE: This function does not support the following Trick types:
   // TRICK_VOID, TRICK_FILE_PTR, TRICK_ENUMERATED, TRICK_OPAQUE_TYPE,
   // or TRICK_STL.  It will treat everything that it cannot match as
   // a basic type as TRICK_STRUCTURED.

   // Otherwise, match to a recognized Trick type.
   if ( std::is_same< T, char * >::value ) {
      trick_type = TRICK_CHARACTER;
   } else if ( std::is_same< T, unsigned char * >::value ) {
      trick_type = TRICK_UNSIGNED_CHARACTER;
   } else if ( std::is_same< T, char ** >::value ) {
      trick_type = TRICK_STRING;
   } else if ( std::is_same< T, short * >::value ) {
      trick_type = TRICK_SHORT;
   } else if ( std::is_same< T, unsigned short * >::value ) {
      trick_type = TRICK_UNSIGNED_SHORT;
   } else if ( std::is_same< T, int * >::value ) {
      trick_type = TRICK_INTEGER;
   } else if ( std::is_same< T, unsigned int * >::value ) {
      trick_type = TRICK_UNSIGNED_INTEGER;
   } else if ( std::is_same< T, long * >::value ) {
      trick_type = TRICK_LONG;
   } else if ( std::is_same< T, unsigned long * >::value ) {
      trick_type = TRICK_UNSIGNED_LONG;
   } else if ( std::is_same< T, float * >::value ) {
      trick_type = TRICK_FLOAT;
   } else if ( std::is_same< T, double * >::value ) {
      trick_type = TRICK_DOUBLE;
   } else if ( std::is_same< T, long long * >::value ) {
      trick_type = TRICK_LONG_LONG;
   } else if ( std::is_same< T, unsigned long long * >::value ) {
      trick_type = TRICK_UNSIGNED_LONG_LONG;
   } else if ( std::is_same< T, bool * >::value ) {
      trick_type = TRICK_BOOLEAN;
   } else if ( std::is_same< T, wchar_t * >::value ) {
      trick_type = TRICK_WCHAR;
   } else if ( std::is_same< T, wchar_t ** >::value ) {
      trick_type = TRICK_WSTRING;
   } else if ( std::is_same< T, void * >::value ) {
      trick_type = TRICK_VOID_PTR;
   } else {
      trick_type = TRICK_STRUCTURED;
   }

   return ( trick_type );
}

} // namespace TrickHLA

#pragma GCC diagnostic pop

#endif // TRICKHLA_MEMORY_SERVICES_IMPL_HH
