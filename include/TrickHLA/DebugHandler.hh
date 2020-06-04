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
@trick_link_dependency{../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3 Titan Group, IMSim, Jan 2010, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end
*/

#ifndef _TRICKHLA_DEBUG_HANDLER_HH_
#define _TRICKHLA_DEBUG_HANDLER_HH_

// System include files.

// Trick include files.

// HLA include files.

// TrickHLA Model include files.
#include "TrickHLA/Types.hh"

namespace TrickHLA
{

class DebugHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
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
   DebugHandler()
      : debug_level( DEBUG_LEVEL_NO_TRACE ), code_section( DEBUG_SOURCE_ALL_MODULES )
   {
   }
   /*! @brief Destructor for the TrickHLA DebugHandler class. */
   virtual ~DebugHandler()
   {
   }
   // Use implicit copy constructor and assignment operator.

   /*! @brief Get current debug level as an enumeration value.
    *  @return Current debug level as a DebugLevelEnum tag. */
   const DebugLevelEnum &get_debug_level() const { return debug_level; }

   /*! @brief Get the current debug level as an integer value.
    *  @return Current debug level as an integer value. */
   const int get_debug_level_as_int() const { return ( (int)debug_level ); }

   /*! @brief Get the code section for this debug handler.
    *  @return Debug handler code module as a DebugSourceEnum tag. */
   const DebugSourceEnum &get_code_section() const { return code_section; }

   /*! @brief Conditional test to see if a debug message should print.
    *  @return Returns true if the requested message should print level.
    *  @param level Debug level of incoming message.
    *  @param code  Debug code source area of the incoming message. */
   bool should_print(
      const DebugLevelEnum & level,
      const DebugSourceEnum &code ) const
   {
      return ( ( debug_level >= level ) && ( ( code_section & code ) != 0 ) );
   }

   /*! @brief Set the debug handler from existing debug handler.
    *  @param in Debug handler from which to copy the level and code section. */
   void set(
      const DebugHandler &in )
   {
      debug_level  = in.get_debug_level();
      code_section = in.get_code_section();
   }

  public:
   DebugLevelEnum  debug_level;  ///< @trick_units{--} Maximum debug report level requested by the user, default: THLA_NO_TRACE
   DebugSourceEnum code_section; ///< @trick_units{--} Code section(s) for which to activate debug messages, default: THLA_ALL_MODULES
};

} // namespace TrickHLA

#endif // _TRICKHLA_DEBUG_HANDLER_HH_: Do NOT put anything after this line!
