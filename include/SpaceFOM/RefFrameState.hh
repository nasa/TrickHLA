/*!
@file SpaceFOM/RefFrameState.hh
@ingroup SpaceFOM
@brief This class provides data packing for the SpaceFOM Reference Frames.

This is the base implementation for the Space Reference FOM (SpaceFOM)
interface to the Reference Frame object. This needs to be available
to the SpaceFOM initialization process for the root reference frame
discovery step in the initialization process.

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

@python_module{SpaceFOM}

@tldh
@trick_link_dependency{../../source/TrickHLA/Attribute.cpp}
@trick_link_dependency{../../source/TrickHLA/Object.cpp}
@trick_link_dependency{../../source/TrickHLA/Manager.cpp}
@trick_link_dependency{../../source/TrickHLA/Packing.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameBase.cpp}
@trick_link_dependency{../../source/SpaceFOM/RefFrameState.cpp}
@trick_link_dependency{../../source/SpaceFOM/SpaceTimeCoordinateEncoder.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, June 2023, --, Refactored to use pure virtual base class.}
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_STATE_HH
#define SPACEFOM_REF_FRAME_STATE_HH

// System include files.
#include <string>

// TrickHLA include files.
#include "TrickHLA/Packing.hh"

// SpaceFOM include files.
#include "SpaceFOM/RefFrameBase.hh"
#include "SpaceFOM/RefFrameData.h"
#include "SpaceFOM/SpaceTimeCoordinateEncoder.hh"

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
namespace TrickHLA
{
class Attribute;
class Object;
class Manager;
} // namespace TrickHLA

namespace SpaceFOM
{

// Forward Declared Classes:  Since these classes are only used as references
// through pointers, these classes are included as forward declarations. This
// helps to limit issues with recursive includes.
class ExecutionControl;

class RefFrameState : public SpaceFOM::RefFrameBase
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__RefFrameState();

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the SpaceFOM RefFrameState class. */
   RefFrameState();
   /*! @brief Destructor for the SpaceFOM RefFrameState class. */
   virtual ~RefFrameState();

   // Initialize the packing object.
   /*! @brief Set the reference to the reference frame data.
    *  @param ref_frame_data_ptr Pointer to the RefFrameData instance. */
   void initialize( RefFrameData *ref_frame_data_ptr );

   // From the TrickHLA::Packing class.
   /*! @brief Called to pack the data before the data is sent to the RTI. */
   virtual void pack();

   // From the TrickHLA::Packing class.
   /*! @brief Called to unpack the data after data is received from the RTI. */
   virtual void unpack();

  protected:
   RefFrameData *ref_frame_data; ///< @trick_units{--} Reference frame data.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for RefFrameState class.
    *  @details This constructor is private to prevent inadvertent copies. */
   RefFrameState( RefFrameState const &rhs );
   /*! @brief Assignment operator for RefFrameState class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   RefFrameState &operator=( RefFrameState const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_STATE_HH: Do NOT put anything after this line!
