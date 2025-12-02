/*!
@file TrickHLA/StringUtilities.hh
@ingroup TrickHLA
@brief String utilities.

@copyright Copyright 2019 United States Government as represented by the
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
@trick_link_dependency{../../source/TrickHLA/Federate.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_STRING_UTILITIES_HH
#define TRICKHLA_STRING_UTILITIES_HH

// System includes.
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/memorymanager_c_intf.h"
#include "trick/parameter_types.h"

// TrickHLA includes.
#include "TrickHLA/HLAStandardSupport.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Handle.h"
#include "RTI/RTI1516.h"
#include "RTI/VariableLengthData.h"
#if defined( IEEE_1516_2025 )
#   include "RTI/Enums.h"
#   include "RTI/Typedefs.h"
#endif

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif

// Whitespace characters: space (' '), tab ('\t'), carriage  return ('\r'),
// newline ('\n'), form-feed ('\f'), and vertical tab ('\v').
#define WHITESPACE_CHARS " \t\r\n\f\v"

namespace TrickHLA
{

class StringUtilities
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__StringUtilities();

  public:
   /*! @brief Destructor for the TrickHLA StringUtilities class. */
   virtual ~StringUtilities()
   {
      return;
   }

   /*! @brief Wide character string (i.e. wchar_t *) duplication in Trick memory.
    *  @param s The input wide string.
    *  @return The duplicate wide string.*/
   static wchar_t *tmm_wstrdup( wchar_t const *s )
   {
      int size = wcslen( s ) + 1;

      /** @li Allocate the duplicate character string */
      wchar_t *addr = static_cast< wchar_t * >( TMM_declare_var( TRICK_WCHAR, "", 0, "", 1, &size ) );

      /** @li Copy the contents of the original character string to the duplicate. */
      /** @li Return the address of the new allocation.*/
      return ( wcscpy( addr, s ) );
   }

   /*! @brief C (char *) string to C++ wide string conversion routine.
    *  @param output The output wide string.
    *  @param input  The input C string. */
   static void to_wstring(
      std::wstring &output,
      char const   *input )
   {
      std::string s = ( input != NULL ) ? input : "";
      output.assign( s.begin(), s.end() );
   }

   /*! @brief C++ string to C++ wide string conversion routine.
    *  @param output The output wide string.
    *  @param input  The input C++ string. */
   static void to_wstring(
      std::wstring      &output,
      std::string const &input )
   {
      output.assign( input.begin(), input.end() );
   }

   /*! @brief C++ wide string to C++ string conversion routine.
    *  @param output The output C++ string.
    *  @param input  The input wide string. */
   static void to_string(
      std::string        &output,
      std::wstring const &input )
   {
      output.assign( input.begin(), input.end() );
   }

   /*! @brief C++ string to C (char *) string conversion routine with the
    * string being placed into Trick memory space.
    *  @details Make sure to use ip_free() to free the memory otherwise you
    *  could end up with a memory leak.
    *  @return C string.
    *  @param input The input string. */
   static char *mm_strdup_string(
      std::string const &input )
   {
      return trick_MM->mm_strdup( const_cast< char * >( input.c_str() ) );
   }

   /*! @brief C++ wide string to C (char *) string conversion routine with the
    * string being placed into Trick memory space.
    *  @details Make sure to use ip_free() to free the memory otherwise you
    *  could end up with a memory leak.
    *  @return C string.
    *  @param input The input wide string. */
   static char *mm_strdup_wstring(
      std::wstring const &input )
   {
      std::string s;
      s.assign( input.begin(), input.end() );
      return trick_MM->mm_strdup( const_cast< char * >( s.c_str() ) );
   }

   /*! @brief HLA RTI User Data to printable C++ string conversion routine.
    *  @param output The output C++ string with only printable characters.
    *  @param data   User supplied tag */
   static void to_printable_string(
      std::string                                 &output,
      RTI1516_NAMESPACE::VariableLengthData const &data )
   {
      output.assign( static_cast< char const * >( data.data() ), data.size() );
      for ( size_t i = 0; i < output.size(); ++i ) {
         if ( !isprint( output[i] ) ) {
            output.replace( i, 1, 1, ' ' );
         }
      }
   }

   /*! @brief HLA RTI User Data to C++ string conversion routine.
    *  @param output The output C++ string.
    *  @param data   User supplied tag */
   static void to_string(
      std::string                                 &output,
      RTI1516_NAMESPACE::VariableLengthData const &data )
   {
      output.assign( static_cast< char const * >( data.data() ), data.size() );
   }

#if defined( IEEE_1516_2025 )
   static std::string to_string(
      RTI1516_NAMESPACE::RtiConfiguration const &rti_config )
   {
      std::string config_name;
      StringUtilities::to_string( config_name, rti_config.configurationName() );

      std::string config_rti_addr;
      StringUtilities::to_string( config_rti_addr, rti_config.rtiAddress() );

      std::string config_addl_settings;
      StringUtilities::to_string( config_addl_settings, rti_config.additionalSettings() );

      std::ostringstream msg;
      msg << " RTI Configuration" << std::endl
          << "         RTI config name: '" << config_name << "'" << std::endl
          << "  RTI config rti-address: '" << config_rti_addr << "'" << std::endl
          << "RTI config addl-settings: '" << config_addl_settings << "'";
      return msg.str();
   }

   static std::string to_string(
      RTI1516_NAMESPACE::ConfigurationResult const &config_result )
   {
      std::string additional_result_msg;
      switch ( config_result.additionalSettingsResult ) {
         case RTI1516_NAMESPACE::SETTINGS_IGNORED: {
            additional_result_msg = "SETTINGS_IGNORED";
            break;
         }
         case RTI1516_NAMESPACE::SETTINGS_FAILED_TO_PARSE: {
            additional_result_msg = "SETTINGS_FAILED_TO_PARSE";
            break;
         }
         case RTI1516_NAMESPACE::SETTINGS_APPLIED: {
            additional_result_msg = "SETTINGS_APPLIED";
            break;
         }
         default: {
            additional_result_msg = "SETTINGS_UNKNOWN";
            break;
         }
      }

      std::string result_msg;
      StringUtilities::to_string( result_msg, config_result.message );

      std::ostringstream msg;
      msg << " RTI Configuration Result" << std::endl
          << "        configuration used: " << ( config_result.configurationUsed ? "Yes" : "No" ) << std::endl
          << "              address used: " << ( config_result.addressUsed ? "Yes" : "No" ) << std::endl
          << "additional-settings result: " << additional_result_msg << std::endl
          << "     config result message: '" << result_msg << "'";
      return msg.str();
   }
#endif // IEEE_1516_2025

   /*! @brief Convert a federate handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Federate handle. */
   static void to_string(
      std::string                             &output,
      RTI1516_NAMESPACE::FederateHandle const &handle )
   {
      to_string( output, handle.toString() );
   }

   /*! @brief Convert an interaction class handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Class handle. */
   static void to_string(
      std::string                                     &output,
      RTI1516_NAMESPACE::InteractionClassHandle const &handle )
   {
      to_string( output, handle.toString() );
   }

   /*! @brief Convert an interaction class handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Parameter Handle. */
   static void to_string(
      std::string                              &output,
      RTI1516_NAMESPACE::ParameterHandle const &handle )
   {
      to_string( output, handle.toString() );
   }

   /*! @brief Convert an object instance handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Instance handle. */
   static void to_string(
      std::string                                   &output,
      RTI1516_NAMESPACE::ObjectInstanceHandle const &handle )
   {
      to_string( output, handle.toString() );
   }

   /*! @brief Convert an object class handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Class handle. */
   static void to_string(
      std::string                                &output,
      RTI1516_NAMESPACE::ObjectClassHandle const &handle )
   {
      to_string( output, handle.toString() );
   }

   /*! @brief Convert an attribute handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Attribute handle. */
   static void to_string(
      std::string                              &output,
      RTI1516_NAMESPACE::AttributeHandle const &handle )
   {
      to_string( output, handle.toString() );
   }

   /*! @brief Trim any leading or trailing whitespace from the string.
    *  @param s The string to trim. */
   static void trim_whitespace(
      std::string &s )
   {
      if ( s.empty() ) {
         return;
      }

      // Find the index to the first non-whitespace character.
      std::string::size_type startIndex = s.find_first_not_of( WHITESPACE_CHARS );

      // Determine if there are any non-whitespace characters in the string.
      if ( startIndex >= s.npos ) {
         // All whitespace, so make it an empty string.
         s.erase();
         s = "";
      } else {
         // Find the index to the last non-whitespace character.
         std::string::size_type endIndex = s.find_last_not_of( WHITESPACE_CHARS );

         // Only trim if we have leading or trailing whitespace.
         if ( ( startIndex > 0 ) || ( endIndex < ( s.size() - 1 ) ) ) {
            // Remove the leading and trailing whitespace.
            std::string tempString = s;
            s.erase();
            s = tempString.substr( startIndex, ( endIndex - startIndex + 1 ) );
         }
      }
   }

   /*! @brief Trim any leading or trailing whitespace from the wstring.
    *  @param s The wstring to trim. */
   static void trim_whitespace(
      std::wstring &s )
   {
      // declare a localized, wstring, version of the whitespace chars.
      std::wstring WSTRING_WHITESPACE_CHARS( L" \t\r\n\f\v" );

      if ( s.empty() ) {
         return;
      }

      // Find the index to the first non-whitespace character.
      std::wstring::size_type startIndex = s.find_first_not_of( WSTRING_WHITESPACE_CHARS );

      // Determine if there are any non-whitespace characters in the string.
      if ( startIndex >= s.npos ) {
         // All whitespace, so make it an empty string.
         s.erase();
         s = L"";
      } else {
         // Find the index to the last non-whitespace character.
         std::wstring::size_type endIndex = s.find_last_not_of( WSTRING_WHITESPACE_CHARS );

         // Only trim if we have leading or trailing whitspace.
         if ( ( startIndex > 0 ) || ( endIndex < ( s.size() - 1 ) ) ) {
            // Remove the leading and trailing whitespace.
            std::wstring tempString = s;
            s.erase();
            s = tempString.substr( startIndex, ( endIndex - startIndex + 1 ) );
         }
      }
   }

   /*! @brief Tokenize a given string for the specified delimiter characters.
    *  @param str        The input string.
    *  @param tokens     Tokens of the string.
    *  @param delimiters Delimiter characters for tokenizing the string. */
   static void tokenize(
      std::string const          &str,
      std::vector< std::string > &tokens,
      std::string const          &delimiters )
   {
      // Skip delimiters at the beginning.
      std::string::size_type lastPos = str.find_first_not_of( delimiters, 0 );

      // Find the first "non-delimiter".
      std::string::size_type pos = str.find_first_of( delimiters, lastPos );

      while ( ( pos != std::string::npos ) || ( lastPos != std::string::npos ) ) {

         // Found a token, add it to the vector of tokens.
         std::string s = str.substr( lastPos, pos - lastPos );

         // remove any leading or trailing whitespace
         trim_whitespace( s );

         // Add the token to the vector.
         if ( !s.empty() ) {
            tokens.push_back( s );
         }

         // Skip the delimiters.
         lastPos = str.find_first_not_of( delimiters, pos );

         // Find the next "non-delimiter".
         pos = str.find_first_of( delimiters, lastPos );
      }
   }

   /*! @brief Tokenize a given wstring for the specified delimiter characters.
    *  @param str        The input string.
    *  @param tokens     Tokens of the string.
    *  @param delimiters Delimiter characters for tokenizing the string. */
   static void tokenize(
      std::string const           &str,
      std::vector< std::wstring > &tokens,
      std::string const           &delimiters )
   {
      // Skip delimiters at the beginning.
      std::string::size_type lastPos = str.find_first_not_of( delimiters, 0 );

      // Find the first "non-delimiter".
      std::string::size_type pos = str.find_first_of( delimiters, lastPos );

      while ( ( pos != std::string::npos ) || ( lastPos != std::string::npos ) ) {

         // Found a token, add it to the vector of tokens.
         std::string  s = str.substr( lastPos, pos - lastPos );
         std::wstring ws( s.begin(), s.end() );

         // remove any leading or trailing whitespace
         trim_whitespace( ws );

         // Add the token to the vector.
         if ( !ws.empty() ) {
            tokens.push_back( ws );
         }

         // Skip the delimiters.
         lastPos = str.find_first_not_of( delimiters, pos );

         // Find the next "non-delimiter".
         pos = str.find_first_of( delimiters, lastPos );
      }
   }

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Default constructor for the TrickHLA StringUtilities class. */
   StringUtilities();

   /*! @brief Copy constructor for StringUtilities class.
    *  @details This constructor is private to prevent inadvertent copies. */
   StringUtilities( StringUtilities const &rhs );

   /*! @brief Assignment operator for StringUtilities class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   StringUtilities &operator=( StringUtilities const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_STRING_UTILITIES_HH: Do NOT put anything after this line!
