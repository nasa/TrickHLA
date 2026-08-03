/*!
@file TrickHLA/MemoryServices.cpp
@ingroup TrickHLA
@brief Memory management services implementation class for TrickHLA.

@detail This class provide the Trick-based memory management services used by
the TrickHLA source code for all memory management tasks.

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
@trick_link_dependency{../../../source/TrickHLA/MemoryServices.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2026, --, Initial implementation.}
@revs_end

*/

// System includes.
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <ostream>
#include <sstream>
#include <string>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/message_proto.h"
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/DebugHandler.hh"
#include "TrickHLA/MemoryServices.hh"
#include "TrickHLA/Types.hh"

using namespace std;
using namespace TrickHLA;

/*!
 * @details This is a pure virtual destructor.
 * @job_class{shutdown}
 */
MemoryServices::~MemoryServices()
{
   return;
}

void *MemoryServices::declare_var( char const *declaration )
{
   return ( trick_MM->declare_var( declaration ) );
}

void *MemoryServices::declare_var( char const *enh_type_spec, size_t n_elems )
{
   return ( trick_MM->declare_var( enh_type_spec, n_elems ) );
}

bool MemoryServices::delete_var( std::string const &var_name )
{
   // Delete the variable name in Trick memory.
   // NOTE: trick_MM->delete_var returns 0 on success and 1 on failure!
   if ( trick_MM->delete_var( const_cast< std::string & >( var_name ) ) ) {
      if ( DebugHandler::show( DEBUG_LEVEL_1_TRACE, DEBUG_SOURCE_MEMORY_SERVICES ) ) {
         ostringstream msg;
         msg << "MemoryServices::delete_var():" << __LINE__
             << " WARNING: Could not delete variable: " << var_name << "\n";
         message_publish( MSG_WARNING, msg.str().c_str() );
      }
      return ( false );
   }
   return ( true );
}

char *MemoryServices::cstrdup( char const *input )
{
   return ( trick_MM->mm_strdup( input ) );
}

char *MemoryServices::cstrdup( std::string const &input )
{
   return ( cstrdup( const_cast< char * >( input.c_str() ) ) );
}

wchar_t *MemoryServices::cwstrdup( char const *input )
{
   std::size_t const len  = strlen( input ) + 1;
   std::size_t       size = ( len <= INT_MAX ) ? (int)len : INT_MAX;
   std::size_t       ret;

   /** @li Allocate the duplicate character string */
   wchar_t *addr = nullptr;
   addr          = MemoryServices::declare_var( addr, "", 0, "", 1, &size );

   /** @li Copy the contents of the original character string to the duplicate. */
   ret = mbstowcs( addr, input, size );
   if ( ret == -1 ) {
      // Delete the allocated memory.
      MemoryServices::delete_var( addr );
      addr = nullptr;
   }

   /** @li Return the address of the new allocation.*/
   return ( addr );
}

wchar_t *MemoryServices::cwstrdup( wchar_t const *input )
{
   std::size_t const len  = wcslen( input ) + 1;
   std::size_t       size = ( len <= INT_MAX ) ? (int)len : INT_MAX;

   /** @li Allocate the duplicate character string */
   wchar_t *addr = nullptr;
   addr          = MemoryServices::declare_var( addr, "", 0, "", 1, &size );

   /** @li Copy the contents of the original character string to the duplicate. */
   /** @li Return the address of the new allocation.*/
   return ( wcscpy( addr, input ) );
}

wchar_t *MemoryServices::cwstrdup( std::wstring const &input )
{
   return ( cwstrdup( const_cast< wchar_t * >( input.c_str() ) ) );
}
