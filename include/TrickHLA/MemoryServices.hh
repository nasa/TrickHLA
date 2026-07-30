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
   ~MemoryServices();

   /*! @brief Allocate a contiguous region of memory as specified by an
    * allocation declaration string.
    * @param declaration A type specifier followed by zero or more asterisks,
    * followed by an optional name, followed by zero or more bracketed integers.
    * @return An address to the allocated memory or NULL on failure. */
   static void *declare_var( char const *declaration );

   /*! @brief Allocate an anonymous, one dimensional array. The elements of the
    * array are specified by the enhanced-type-specifier. The length of the array
    * is specified by @b n_elems.
    * @param enh_type_spec Type specifier followed by zero or more asterisks.
    * @param n_elems       The number of items of the given type to allocate.
    * @return An address to the allocated memory or NULL on failure.  */
   static void *declare_var( char const *enh_type_spec, size_t n_elems );

   /*! @brief Delete the memory at the named allocation address.
    * @param  var_name Name of the Trick memory allocation.
    * @return Returns true if deallocation succeeded, false otherwise. */
   static bool delete_var( std::string const & var_name );

   //--------------------------------------------------------------------------
   // String memory management functions.
   //--------------------------------------------------------------------------
   
   /*! @brief Allocate for a duplicate of an input C string (char*).
    * @param input The C string (char *) to be duplicated.
    * @return An address to the allocated C string or NULL on failure. */
   static char * cstrdup( const char * input );
   
   /*! @brief Allocate for a duplicate of an input C++ string (std::string).
    * @param input The string to be duplicated.
    * @return An address to the allocated C string or NULL on failure. */
   static char * cstrdup( std::string const &input );
   
   /*! @brief Allocate for a duplicate of an input C string (char*).
    * @param input The C wide string (wchar_t *) to be duplicated.
    * @return An address to the allocated C wide string or NULL on failure. */
   static wchar_t * cwstrdup( const char * input );
   
   /*! @brief Allocate for a duplicate of an input C wide string (char*).
    * @param input The C wide string (wchar_t *) to be duplicated.
    * @return An address to the allocated C wide string or NULL on failure. */
   static wchar_t * cwstrdup( const wchar_t * input );
   
   /*! @brief Allocate for a duplicate of an input C++ wide string (std::wstring).
    * @param input The wide string to be duplicated.
    * @return An address to the allocated C wide string or NULL on failure. */
   static wchar_t * cwstrdup( std::wstring const &input );

   //--------------------------------------------------------------------------
   // Template functions.
   //--------------------------------------------------------------------------

   /*! @brief This is the general version of declare_var(), which allocates a contiguous
    * region of memory, for a (named or anonymous) variable, of the specified type
    * and dimension.
    * @detail An allocation (variable) may by named (@b var_name != "") or anonymous
    * (@b var_name == ""). Named allocations are checkpointed using their given name.
    * Anonymous allocations are checkpointed with generated names of the form:
    * @c trick_temp_#, where @c # is an integer.
    * @tparam T          Template type parameter.
    * @param  type       Specified type to allocate.
    * @param  class_name Class or struct name if @b type is TRICK_STRUCTURED,
    * otherwise @b class_name should be "".
    * @param  n_stars    Number of asterisks in the variable declaration.
    * @param  var_name   Name of the allocation. ="" for anonymous allocations.
    * @param  n_cdims    Number of constrained/fixed dimensions. =0 for unarrayed variables.
    * @param  cdims      Array of dimension sizes.
    * @return An address to the allocated memory or NULL on failure. */
   template < typename T >
   static T declare_var( T type, std::string const &class_name, size_t n_stars, std::string const &var_name, size_t n_cdims, size_t *cdims );

   /*! @brief Allocate an array of a specified type instances.
    * @detail An allocation (variable) may by named (@b var_name != "") or anonymous
    * (@b var_name == ""). Named allocations are checkpointed using their given name.
    * Anonymous allocations are checkpointed with generated names of the form:
    * @c trick_temp_#, where @c # is an integer.
    * @tparam T        Template type parameter.
    * @param  type     Specified type to allocate.
    * @param  n_elems  Number of elements in the array.
    * @param  var_name Name of the allocation for Trick.  Anonymous allocation if empty.
    * @return An address to the allocated memory or NULL on failure. */
   template < typename T >
   static T declare_var( T type, size_t n_elems, std::string const &var_name );

   /*! @brief Allocate an array of a specified type instances.
    * @detail An allocation (variable) may by named (@b var_name != "") or anonymous
    * (@b var_name == ""). Named allocations are checkpointed using their given name.
    * Anonymous allocations are checkpointed with generated names of the form:
    * @c trick_temp_#, where @c # is an integer.
    * @tparam T        Template type parameter.
    * @param  type     Specified type to allocate.
    * @param  n_elems  Number of elements in the array.
    * @return An address to the allocated memory or NULL on failure. */
   template < typename T >
   static T declare_var( T type, size_t n_elems );

   /*! @brief Allocate a contiguous region of memory as specified by an
    * allocation declaration string.
    * @tparam T           Template type parameter.
    * @param  type        Specified type to allocate.
    * @param  declaration A type specifier followed by zero or more asterisks,
    * followed by an optional name, followed by zero or more bracketed integers.
    * @return An address to the allocated memory or NULL on failure. */
   template < typename T >
   static T declare_var( T type, std::string const &declaration );

   /*! @brief Allocate an anonymous, one dimensional array. The elements of the
    * array are specified by the enhanced-type-specifier. The length of the array
    * is specified by @b n_elems.
    * @tparam T             Template type parameter.
    * @param  type          Specified type to allocate.
    * @param  enh_type_spec Type specifier followed by zero or more asterisks.
    * @param  n_elems       The number of items of the given type to allocate.
    * @return An address to the allocated memory or NULL on failure.  */
   template < typename T >
   static T declare_var( T type, std::string const &enh_type_spec, size_t n_elems );

   /*! @brief Resize one-dimensional array by address.
    * @tparam T             Template type parameter.
    * @param  address       Address of array to be resized.
    * @param  n_elems       The number of items of the given type to allocate.
    * @return An address to the allocated memory or NULL on failure.  */
   template < typename T >
   static T resize_array( T address, size_t n_elems);

   /*! @brief Delete the memory at the specified address.
    * @tparam T    Template type parameter.
    * @param  addr Specified type to allocate.
    * @return Returns true if deallocation succeeded, false otherwise. */
   template < typename T >
   static bool delete_var( T addr );

   //--------------------------------------------------------------------------
   // Information functions.
   //--------------------------------------------------------------------------

   /*! @brief Test whether the given address is in Trick managed memory.
    * @tparam T       Template type parameter.
    * @param  address The address to be tested.
    * @return Returns true if allocation exists, false otherwise. */
   template < typename T >
   static bool is_alloced( T address);

   /*! @brief Get the size of the given allocation in Trick managed memory.
    * @tparam T       Template type parameter.
    * @param  address The address to be queried.
    * @return Returns the size of the allocation at that address. */
   template < typename T >
   static std::size_t get_size( T address );

   template < typename T >
   static std::string get_class_name( T type );

   template < typename T >
   static int get_trick_type( T type );

};

// Allow external access to the TrickHLA::MemoryServices instance.
static MemoryServices *memory_services;

} // namespace TrickHLA

// TrickHLA includes.
#include "TrickHLA/MemoryServices_impl.hh"

#endif // TRICKHLA_MEMORY_SERVICES_HH
