/*!
@file SpaceFOM/RefFrameOwnershipHandler.hh
@ingroup SpaceFOM
@brief Ownership transfer for the HLA object attributes of a SpaceFOM
RefFrame.

@copyright Copyright 2023 United States Government as represented by the
Administrator of the National Aeronautics and Space Administration.
No copyright is claimed in the United States under Title 17, U.S. Code.
All Other Rights Reserved.

\par<b>Responsible Organization</b>
Simulation and Graphics Branch, Mail Code ER7\n
Software, Robotics & Simulation Division\n
NASA, Johnson Space Center\n
2101 NASA Parkway, Houston, TX  77058

@trick_parse{everything}

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/OwnershipHandler.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameOwnershipHandler.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, November 2023, --, Initial version.}
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_OWNERSHIP_HANDLER_HH
#define SPACEFOM_REF_FRAME_OWNERSHIP_HANDLER_HH

// TrickHLA include files.
#include "TrickHLA/Object.hh"
#include "TrickHLA/OwnershipHandler.hh"

namespace SpaceFOM
{

class RefFrameOwnershipHandler : public TrickHLA::OwnershipHandler
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__RefFrameOwnershipHandler();

  public:
   //
   // Public constructors and destructor.
   //
   /*! @brief Default constructor for the TrickHLAModel RefFrameOwnershipHandler class. */
   RefFrameOwnershipHandler();
   /*! @brief Destructor for the TrickHLAModel RefFrameOwnershipHandler class. */
   virtual ~RefFrameOwnershipHandler();

   /*! @brief Initialization callback as part of the TrickHLA::OwnershipHandler functions.
    *  @param obj Object associated with this OwnershipHandler class. */
   virtual void initialize_callback( TrickHLA::Object *obj );

  private:
   // Do not allow the copy constructor or assignment operator.
   /*! @brief Copy constructor for RefFrameOwnershipHandler class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RefFrameOwnershipHandler( RefFrameOwnershipHandler const &rhs );
   /*! @brief Assignment operator for RefFrameOwnershipHandler class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RefFrameOwnershipHandler &operator=( RefFrameOwnershipHandler const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_OWNERSHIP_HANDLER_HH: Do NOT put anything after this line!
