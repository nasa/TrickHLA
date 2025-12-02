/*!
@file TrickHLA/Conditional.hh
@ingroup TrickHLA
@brief This class provides a user API for the handling of a Condistional
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
@trick_link_dependency{../../source/TrickHLA/Conditional.cpp}
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/ObjectCallbackBase.cpp}
@trick_link_dependency{../../source/TrickHLA/Types.cpp}

@revs_title
@revs_begin
@rev_entry{Tony Varesic, L3 Titan Group, IMSim, Oct 2009, --, Initial version.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, November 2023, --, Added initialize_callback().}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, September 2025, --, Extends ObjectCallbackBase.}
@revs_end
*/

#ifndef TRICKHLA_CONDITIONAL_HH
#define TRICKHLA_CONDITIONAL_HH

// System includes.
#include <string>

// TrickHLA includes.
#include "TrickHLA/ObjectCallbackBase.hh"

namespace TrickHLA
{

class Attribute;

class Conditional : public ObjectCallbackBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
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
   /*! @brief Constructor for the TrickHLA Conditional class with a name. */
   explicit Conditional( std::string name );
   /*! @brief Destructor for the TrickHLA Conditional class. */
   virtual ~Conditional();

   /*! @brief Indicate true if the attribute data should be sent, false otherwise.
    *  @return True if the attribute data should be sent, false otherwise.
    *  @param attr Pointer to TrickHLA Attribute. */
   virtual bool should_send( Attribute *attr );

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Conditional class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Conditional( Conditional const &rhs );
   /*! @brief Assignment operator for Conditional class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Conditional &operator=( Conditional const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_CONDITIONAL_HH: Do NOT put anything after this line!
