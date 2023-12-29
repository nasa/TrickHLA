/*!
@file models/sine/include/SineObjectDeleted.hh
@ingroup TrickHLAModel
@brief Callback class the user writes to do something once the object has been
deleted from the RTI.

@copyright Copyright 2020 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{TrickHLAModel}

@tldh
@trick_link_dependency{../../../source/TrickHLA/Object.cpp}
@trick_link_dependency{sine/src/SineObjectDeleted.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, June 2008, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_MODLE_SINE_OBJECT_DELETED_HH
#define TRICKHLA_MODLE_SINE_OBJECT_DELETED_HH

// Trick include files.
#include "TrickHLA/Object.hh"
#include "TrickHLA/ObjectDeleted.hh"

namespace TrickHLAModel
{

class SineObjectDeleted : public TrickHLA::ObjectDeleted
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__SineObjectDeleted();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel SineObjectDeleted class. */
   SineObjectDeleted();
   /*! @brief Destructor for the TrickHLAModel SineObjectDeleted class. */
   virtual ~SineObjectDeleted();

   //-----------------------------------------------------------------
   // These functions must be defined to make this a concrete class.
   //-----------------------------------------------------------------

   /*! @brief Callback routine implementation to report that this object has
    *  been deleted from the RTI. */
   void deleted();

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SineObjectDeleted class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SineObjectDeleted( SineObjectDeleted const &rhs );
   /*! @brief Assignment operator for SineObjectDeleted class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SineObjectDeleted &operator=( SineObjectDeleted const &rhs );
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODLE_SINE_OBJECT_DELETED_HH: Do NOT put anything after this line!
