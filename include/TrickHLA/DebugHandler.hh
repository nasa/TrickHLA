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
@revs_end
*/

#ifndef TRICKHLA_DEBUG_HANDLER_HH
#define TRICKHLA_DEBUG_HANDLER_HH

// System include files.
#include <string>

// TrickHLA Model include files.
#include "TrickHLA/Types.hh"

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
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Initialization constructor for the TrickHLA DebugHandler class. */
   DebugHandler();
   /*! @brief Destructor for the TrickHLA DebugHandler class. */
   virtual ~DebugHandler();

   /*! @brief Conditional test to see if a debug message should be shown.
    *  @return Returns true if the requested message should be printed.
    *  @param level Debug level of incoming message.
    *  @param code  Debug code source area of the incoming message. */
   static bool const show( DebugLevelEnum const level, DebugSourceEnum const code );

   /*! @brief Set the debug level and code-section.
    *  @param level Debug level of incoming message.
    *  @param code  Debug code source area of the incoming message. */
   static void set( DebugLevelEnum const level, DebugSourceEnum const code );

   /*! @brief Print the message then shutdown by calling exec_terminate().
    *  @param message Error message to print to standard error.
    *  @param exit_code the exit code to use with a default of 1. */
   static void terminate_with_message( std::string const &message,
                                       int const          exit_code = 1 );

  public:
   static DebugLevelEnum  debug_level;  ///< @trick_units{--} Maximum debug report level requested by the user, default: THLA_NO_TRACE
   static DebugSourceEnum code_section; ///< @trick_units{--} Code section(s) for which to activate debug messages, default: THLA_ALL_MODULES
};

} // namespace TrickHLA

#endif // TRICKHLA_DEBUG_HANDLER_HH: Do NOT put anything after this line!
