/*!
@file TrickHLA/Conditional.hh
@ingroup TrickHLA
@brief This class provides a user API for the handling of a CONDITIONAL
attribute.

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
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Conditional.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3 Titan Group, IMSim, Oct 2009, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end
*/

#ifndef _TRICKHLA_CONDITIONAL_HH_
#define _TRICKHLA_CONDITIONAL_HH_

namespace TrickHLA
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations.  This
// helps to limit issues with recursive includes.
class Attribute;

class Conditional
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet).  This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Conditional();

  public:
   //-----------------------------------------------------------------
   // Constructors / destructors
   //-----------------------------------------------------------------
   /*! @brief Default constructor for the TrickHLA Conditional class. */
   Conditional();
   /*! @brief Destructor for the TrickHLA Conditional class. */
   virtual ~Conditional();

   /*!
   @brief Default implementation of a virtual function, returning true.
   @return Always returns boolean true.
   @param attr Pointer to attribute.
   */
   virtual bool should_send( Attribute *attr );
};

} // namespace TrickHLA

#endif // _TRICKHLA_CONDITIONAL_HH_: Do NOT put anything after this line!
