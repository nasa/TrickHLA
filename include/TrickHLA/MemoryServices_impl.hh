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

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2026, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MEMORY_SERVICES_IMPL_HH
#define TRICKHLA_MEMORY_SERVICES_IMPL_HH

// System includes.
#include <string>
#include <type_traits>

// Trick includes.
#include "trick/MemoryManager.hh"

// TrickHLA includes.
#include "TrickHLA/MemoryServices.hh"

namespace TrickHLA
{

template < typename T >
T MemoryServices::declare_var(
   T                  type,
   std::string const &class_name,
   int                n_stars,
   std::string const &var_name,
   int                n_cdims,
   int               *cdims )
{
   TRICK_TYPE trick_type = TRICK_STRUCTURED;

   // Check to make sure the incoming type is a pointer.
   // If not, then return null.
   if ( !std::is_pointer< T >::value ) {
      return ( nullptr );
   }

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

   // Perform the allocation and return the allocated value.
   return ( static_cast< T >( trick_MM->declare_var( trick_type, class_name, n_stars, var_name, n_cdims, cdims ) ) );
}

template < typename T >
T MemoryServices::declare_var(
   T                  type,
   std::string const &enh_type_spec,
   int                n_elems )
{
   return ( static_cast< T >( trick_MM->declare_var( enh_type_spec.c_str(), n_elems ) ) );
}

template < typename T >
T MemoryServices::declare_var(
   T                  type,
   std::string const &declaration )
{
   return ( static_cast< T >( trick_MM->declare_var( declaration.c_str() ) ) );
}

} // namespace TrickHLA

#endif // TRICKHLA_MEMORY_SERVICES_IMPL_HH
