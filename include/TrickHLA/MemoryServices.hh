/*!
@file TrickHLA/MemoryServices.hh
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
@trick_link_dependency{../../source/TrickHLA/MemoryServices.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, July 2026, --, Initial implementation.}
@revs_end

*/

#ifndef TRICKHLA_MEMORY_SERVICES_HH
#define TRICKHLA_MEMORY_SERVICES_HH

// System includes.
#include <string>

namespace TrickHLA
{

class MemoryServices
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__MemoryServices();

  public:
   /*! @brief Destructor for the TrickHLA CheckpointConversionBase class. */
   virtual ~MemoryServices();

   /* @brief Allocate a contiguous region of memory as specified by an
    * allocation declaration string.
    * @param declaration - a type specifier followed by zero or more asterisks,
    * followed by an optional name, followed by zero or more bracketed integers.
    * @return - An address to the allocated memory or NULL on failure. */
   virtual void *declare_var( char const *declaration );

   /* @brief Allocate an anonymous, one dimensional array. The elements of the
    * array are specified by the enhanced-type-specifier. The length of the array
    * is specified by @b n_elems.
    * @param enh_type_spec - type specifier followed by zero or more asterisks.
    * @param n_elems - The number of items of the given type to allocate.
    * @return - An address to the allocated memory or NULL on failure.  */
   virtual void *declare_var( char const *enh_type_spec, int n_elems );

   /* @brief This is the general version of declare_var(), which allocates a contiguous
    * region of memory, for a (named or anonymous) variable, of the specified type
    * and dimension.
    * @detail An allocation (variable) may by named (@b var_name != "") or anonymous
    * (@b var_name == ""). Named allocations are checkpointed using their given name.
    * Anonymous allocations are checkpointed with generated names of the form:
    * @c trick_temp_#, where @c # is an integer.
    * @param type - TRICK_TYPE.
    * @param class_name - class or struct name if @b type is TRICK_STRUCTURED,
    * otherwise @b class_name should be "".
    * @param n_stars - number of asterisks in the variable declaration.
    * @param var_name - (optional) name of the allocation. ="" for anonymous allocations.
    * @param n_cdims - number of constrained/fixed dimensions. =0 for unarrayed variables.
    * @param cdims - array of dimension sizes.
    * @return - an address to the allocated memory or NULL on failure. */
   // virtual void* declare_var( TRICK_TYPE type, std::string class_name, int n_stars, std::string var_name, int n_cdims, int *cdims ) = 0;

   /* @brief A template function used to allocate memory for any recognized base type. */
   template < typename T >
   T declare_var( T type, std::string const &class_name, int n_stars, std::string const &var_name, int n_cdims, int *cdims );

   template < typename T >
   T declare_var( T type, std::string const &enh_type_spec, int n_elems );

   template < typename T >
   T declare_var( T type, std::string const &declaration );
};

// Allow external access to the TrickHLA::MemoryServices instance.
static MemoryServices *memory_services;

} // namespace TrickHLA

// TrickHLA includes.
#include "TrickHLA/MemoryServices_impl.hh"

#endif // TRICKHLA_MEMORY_SERVICES_HH
