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
@trick_link_dependency{../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../source/TrickHLA/Utilities.cpp}
@trick_link_dependency{../source/TrickHLA/Conditional.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, June 2006, --, Initial implementation.}
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, March 2019, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef _TRICKHLA_STRING_UTIL_HH_
#define _TRICKHLA_STRING_UTIL_HH_

// System include files.
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Trick include files.
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"

// HLA include files.
#include "TrickHLA/StandardsSupport.hh"
#include RTI1516_HEADER

// TrickHLA Model include files.
#include "TrickHLA/CompileConfig.hh"

// Whitespace characters: space (' '), tab ('\t'), carriage  return ('\r'),
// newline ('\n'), form-feed ('\f'), and vertical tab ('\v').
#define WHITESPACE_CHARS " \t\r\n\f\v"

namespace TrickHLA
{

class StringUtilities
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__StringUtilities();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA StringUtilities class. */
   StringUtilities(){};

   /*! @brief Destructor for the TrickHLA StringUtilities class. */
   virtual ~StringUtilities(){};

  public:
   /*! @brief C (char *) string to C++ wide string conversion routine.
    *  @param output The output wide string.
    *  @param input  The input C string. */
   static void to_wstring(
      std::wstring &output,
      const char *  input )
   {
      std::string s = ( input != NULL ) ? input : "";
      output.assign( s.begin(), s.end() );
   }

   /*! @brief C++ string to C++ wide string conversion routine.
    *  @param output The output wide string.
    *  @param input  The input C++ string. */
   static void to_wstring(
      std::wstring &     output,
      std::string const &input )
   {
      output.assign( input.begin(), input.end() );
   }

   /*! @brief C++ wide string to C++ string conversion routine.
    *  @param output The output C++ string.
    *  @param input  The input wide string. */
   static void to_string(
      std::string &       output,
      std::wstring const &input )
   {
      output.assign( input.begin(), input.end() );
   }

   /*! @brief C++ wide string to C (char *) string conversion routine with the
    * string being placed into Trick memory space.
    *  @details Make sure to use ip_free() to free the memory otherwise you
    *  could end up with a memory leak.
    *  @return C string.
    *  @param input The input wide string. */
   static char *ip_strdup_wstring(
      std::wstring const &input )
   {
      std::string s;
      s.assign( input.begin(), input.end() );
      return TMM_strdup( (char *)s.c_str() );
   }

   /*! @brief HLA RTI User Data to printable C++ string conversion routine.
    *  @param output The output C++ string with only printable characters.
    *  @param data   User supplied tag */
   static void to_printable_string(
      std::string &           output,
      RTI1516_USERDATA const &data )
   {
      output.assign( (const char *)data.data(), data.size() );
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
      std::string &           output,
      RTI1516_USERDATA const &data )
   {
      output.assign( (const char *)data.data(), data.size() );
   }

   /*! @brief Convert a federate handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Federate handle. */
   static void to_string(
      std::string &                     output,
      RTI1516_NAMESPACE::FederateHandle handle )
   {
      to_string( output, handle.encode() );
   }

   /*! @brief Convert an interaction class handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Class handle. */
   static void to_string(
      std::string &                             output,
      RTI1516_NAMESPACE::InteractionClassHandle handle )
   {
      to_string( output, handle.encode() );
   }

   /*! @brief Convert an interaction class handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Parameter Handle. */
   static void to_string(
      std::string &                      output,
      RTI1516_NAMESPACE::ParameterHandle handle )
   {
      to_string( output, handle.encode() );
   }

   /*! @brief Convert an object instance handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Instance handle. */
   static void to_string(
      std::string &                           output,
      RTI1516_NAMESPACE::ObjectInstanceHandle handle )
   {
      to_string( output, handle.encode() );
   }

   /*! @brief Convert an object class handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Class handle. */
   static void to_string(
      std::string &                        output,
      RTI1516_NAMESPACE::ObjectClassHandle handle )
   {
      to_string( output, handle.encode() );
   }

   /*! @brief Convert an attribute handle to a C string representation.
    *  @param output The output C++ string.
    *  @param handle Attribute handle. */
   static void to_string(
      std::string &                      output,
      RTI1516_NAMESPACE::AttributeHandle handle )
   {
      to_string( output, handle.encode() );
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
      std::string const &         str,
      std::vector< std::string > &tokens,
      std::string const &         delimiters = "," )
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
      std::string const &          str,
      std::vector< std::wstring > &tokens,
      std::string const &          delimiters = "," )
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
   /*! @brief Copy constructor for StringUtilities class.
    *  @details This constructor is private to prevent inadvertent copies. */
   StringUtilities( const StringUtilities &rhs );

   /*! @brief Assignment operator for StringUtilities class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   StringUtilities &operator=( const StringUtilities &rhs );
};

} // namespace TrickHLA

#endif // _TRICKHLA_STRING_UTIL_HH_: Do NOT put anything after this line!
