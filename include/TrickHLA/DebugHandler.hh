/*!
@file TrickHLA/DebugHandler.hh
@ingroup TrickHLA
@brief Multi-level debug reporter.

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
@trick_link_dependency{../../source/TrickHLA/DebugHandler.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3 Titan Group, IMSim, Jan 2010, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, July 2020, --, Rewrite to use static data and functions.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, August 2026, --, Added support for HLA Exceptions.}
@revs_end
*/

#ifndef TRICKHLA_DEBUG_HANDLER_HH
#define TRICKHLA_DEBUG_HANDLER_HH

// System includes.
#include <string>

// Trick includes.
#include "trick/message_type.h"

// TrickHLA includes.
#include "TrickHLA/HLAStandardSupport.hh"
#include "TrickHLA/Types.hh"

// C++11 deprecated dynamic exception specifications for a function so we need
// to silence the warnings coming from the IEEE 1516 declared functions.
// This should work for both GCC and Clang.
#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// HLA include files.
#include "RTI/Exception.h"

#if defined( IEEE_1516_2010 )
#   pragma GCC diagnostic pop
#endif // IEEE_1516_2025

namespace TrickHLA
{

class DebugHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__DebugHandler();

  public:
   /*! @brief Destructor for the TrickHLA DebugHandler class. */
   virtual ~DebugHandler()
   {
      return;
   }

   /*! @brief Conditional test to see if a debug message should be shown.
    *  @return Returns true if the requested message should be printed.
    *  @param level Debug level of incoming message.
    *  @param code  Debug code source area of the incoming message. */
   static bool show( DebugLevelEnum const level, DebugSourceEnum const code )
   {
      return ( ( debug_level >= level ) && ( ( code_section & code ) != 0 ) );
   }

   /*! @brief Set the debug level and code-section.
    *  @param level Debug level of incoming message.
    *  @param code  Debug code source area of the incoming message. */
   static void set( DebugLevelEnum const level, DebugSourceEnum const code );

   /*! @brief Print the message for the given Trick message level.
    *  @param message The message to print to the console.
    *  @param msg_level Trick message level. */
   static void print_message( std::string const &message,
                              MESSAGE_TYPE const msg_level = MSG_NORMAL );

   /*! @brief Print the message for the given Trick message level and function name.
    *  @param pretty_func_name The pretty function name from the __PRETTY_FUNCTION__ macro.
    *  @param line_number The line number of the calling code.
    *  @param message The message to print to the console.
    *  @param msg_level Trick message level. */
   static void print_message( std::string const &pretty_func_name,
                              size_t const       line_number,
                              std::string const &message,
                              MESSAGE_TYPE const msg_level = MSG_NORMAL );

   /*! @brief Print the exception warning message.
    *  @param pretty_func_name The pretty function name from the __PRETTY_FUNCTION__ macro.
    *  @param line_number The line number of the calling code.
    *  @param e The exception. */
   static void print_exception( std::string const                  &pretty_func_name,
                                size_t const                        line_number,
                                RTI1516_NAMESPACE::Exception const &e );

   /*! @brief Print the message then shutdown by calling exec_terminate().
    *  @param message Error message to print to the console.
    *  @param exit_code the exit code to use with a default of 1. */
   static void terminate( std::string const &message,
                          int const          exit_code = 1 );

   /*! @brief Print the message then shutdown by calling exec_terminate().
    *  @param pretty_func_name The pretty function name from the __PRETTY_FUNCTION__ macro.
    *  @param line_number The line number of the calling code.
    *  @param message Error message to print to the console.
    *  @param exit_code the exit code to use with a default of 1. */
   static void terminate( std::string const &pretty_func_name,
                          size_t const       line_number,
                          std::string const &message,
                          int const          exit_code = 1 );

   /*! @brief Print the exception error message then shutdown by calling exec_terminate().
    *  @param pretty_func_name The pretty function name from the __PRETTY_FUNCTION__ macro.
    *  @param line_number The line number of the calling code.
    *  @param e The exception.
    *  @param exit_code the exit code to use with a default of 1. */
   static void terminate( std::string const                  &pretty_func_name,
                          size_t const                        line_number,
                          RTI1516_NAMESPACE::Exception const &e,
                          int const                           exit_code = 1 );

  public:
   static DebugLevelEnum  debug_level;  ///< @trick_units{--} Maximum debug report level requested by the user, default: THLA_NO_TRACE
   static DebugSourceEnum code_section; ///< @trick_units{--} Code section(s) for which to activate debug messages, default: THLA_ALL_MODULES

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Default constructor for the TrickHLA DebugHandler class. */
   DebugHandler();

   /*! @brief Copy constructor for DebugHandler class.
    *  @details This constructor is private to prevent inadvertent copies. */
   DebugHandler( DebugHandler const &rhs );

   /*! @brief Assignment operator for DebugHandler class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   DebugHandler &operator=( DebugHandler const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_DEBUG_HANDLER_HH: Do NOT put anything after this line!
