/*!
@file TrickHLA/Packing.hh
@ingroup TrickHLA
@brief Definition of the TrickHLA Packing class.

This class is the abstract base class for packing before data is sent to
the RTI and unpacking just after data is received from the RTI.

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
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/TrickHLA/ObjectCallbackBase.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, L3 Titan Group, DSES, Sept 2006, --, Initial version.}
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Sept 2009, --, Updated Packing API.}
@rev_entry{Dan Dexter, NASA/ER7, IMSim, Oct 2009, --, Added get attribute function.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Dan Dexter, NASA ER6, TrickHLA, September 2025, --, Extends ObjectCallbackBase.}
@revs_end
*/

#ifndef TRICKHLA_PACKING_HH
#define TRICKHLA_PACKING_HH

// System includes.
#include <string>

// TrickHLA includes.
#include "TrickHLA/ObjectCallbackBase.hh"

namespace TrickHLA
{

class Packing : public ObjectCallbackBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLA__Packing();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLA Packing class. */
   Packing();
   /*! @brief Constructor for the TrickHLA Packing class with a name. */
   explicit Packing( std::string name );
   /*! @brief Destructor for the TrickHLA Packing class. */
   virtual ~Packing();

   //-----------------------------------------------------------------
   // These are virtual functions and must be defined by a full class.
   //-----------------------------------------------------------------

   /*! @brief Pack the data before being sent. */
   virtual void pack() = 0;

   /*! @brief Unpack the received data. */
   virtual void unpack() = 0;

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for Packing class.
    *  @details This constructor is private to prevent inadvertent copies. */
   Packing( Packing const &rhs );
   /*! @brief Assignment operator for Packing class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   Packing &operator=( Packing const &rhs );
};

} // namespace TrickHLA

#endif // TRICKHLA_PACKING_HH: Do NOT put anything after this line!
