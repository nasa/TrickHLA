/*!
@file models/sine/include/SineOwnershipHandler.hh
@ingroup TrickHLAModel
@brief Ownership transfer for the HLA object attributes.

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
@trick_link_dependency{../source/TrickHLA/Object.cpp}
@trick_link_dependency{../source/TrickHLA/OwnershipHandler.cpp}
@trick_link_dependency{sine/src/SineOwnershipHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Dan Dexter, NASA ER7, TrickHLA, August 2006, --, Version 2 origin.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2020, --, Version 3 rewrite.}
@revs_end

*/

#ifndef TRICKHLA_MODEL_SINE_OWNERSHIP_HANDLER_HH
#define TRICKHLA_MODEL_SINE_OWNERSHIP_HANDLER_HH

// TrickHLA include files.
#include "TrickHLA/Object.hh"
#include "TrickHLA/OwnershipHandler.hh"

namespace TrickHLAModel
{

class SineOwnershipHandler : public TrickHLA::OwnershipHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrTrickHLAModel__SineOwnershipHandler();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel SineOwnershipHandler class. */
   SineOwnershipHandler();
   /*! @brief Destructor for the TrickHLAModel SineOwnershipHandler class. */
   virtual ~SineOwnershipHandler();

   /*! @brief Initialization callback as part of the TrickHLA::OwnershipHandler functions.
    *  @param obj Object associated with this OwnershipHandler class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for SineOwnershipHandler class.
    *  @details This constructor is private to prevent inadvertent copies. */
   SineOwnershipHandler( SineOwnershipHandler const &rhs );
   /*! @brief Assignment operator for SineOwnershipHandler class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   SineOwnershipHandler &operator=( SineOwnershipHandler const &rhs );
};

} // namespace TrickHLAModel

#endif // TRICKHLA_MODEL_SINE_OWNERSHIP_HANDLER_HH: Do NOT put anything after this line!
