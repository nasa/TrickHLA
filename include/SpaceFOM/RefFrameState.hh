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

// SpaceFOM includes.
#include "RefFrameBase.hh"

namespace SpaceFOM
{

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
   /*! @brief Initialization constructor to set reference to data. */
   explicit RefFrameState( RefFrameData &ref_frame_data_ref );
   /*! @brief Destructor for the SpaceFOM RefFrameState class. */
   virtual ~RefFrameState();

   // Initialize the packing object.
   /*! @brief Set the reference to the reference frame data.
    *  @param ref_frame_data_ptr Pointer to the RefFrameData instance. */
   void configure( RefFrameData *ref_frame_data_ptr );

   /*! @brief Finish the initialization of the RefFrame. */
   virtual void initialize();

   /*! @brief Packs the packing data object from the working data object(s),
    *  @details Called from the pack() function to pack the data from the working
    *  data objects(s) into the pe_packing_data object.  */
   virtual void pack_from_working_data();

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  pe_packing_data object into the working data object(s). */
   virtual void unpack_into_working_data();

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
