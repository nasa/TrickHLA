/*!
@ingroup JEOD
@file JEOD/JEODRefFrameState.hh
@brief This class provides data packing for the SpaceFOM Reference Frames and
the interface with a JEOD Reference Frame State instance.

This is the base implementation for the Space Reference FOM (SpaceFOM)
interface to the SpaceFOM Reference Frame object.  This class provides the
interface code to encode and decode SpaceFOM Reference Frame Objects and
JEOD Reference Frame State instances. This needs to be available to the
SpaceFOM initialization process for the root reference frame discovery step
in the initialization process.

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
@trick_link_dependency{../../source/SpaceFOM/RefFrameBase.cpp}
@trick_link_dependency{../../source/JEOD/JEODRefFrameState.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, May 2023, --, Based off of RefFrameBase.}
@revs_end

*/

#ifndef SPACEFOM_JEOD_REF_FRAME_STATE_HH
#define SPACEFOM_JEOD_REF_FRAME_STATE_HH

// SpaceFOM includes.
#include "SpaceFOM/RefFrameBase.hh"
#include "SpaceFOM/SpaceTimeCoordinateData.hh"

namespace jeod
{
class RefFrameState;
class TimeTT;
} /* namespace jeod */

namespace SpaceFOM
{

class JEODRefFrameState : public SpaceFOM::RefFrameBase
{

   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exists - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__JEODRefFrameState();

  public:
   // Public constructors and destructors.
   /*! @brief Default constructor for the SpaceFOM JEODRefFrameState class. */
   JEODRefFrameState();
   /*! @brief Initialization constructor to set reference to data.
    *  @param time_tt_in Pointer to the jeod::TimeTT instance.
    *  @param ref_frame_state_ref Reference to the RefFrameState instance. */
   JEODRefFrameState( jeod::TimeTT        &time_tt_in,
                      jeod::RefFrameState &ref_frame_state_ref );
   /*! @brief Destructor for the SpaceFOM JEODRefFrameState class. */
   virtual ~JEODRefFrameState();

   // Initialize the packing object.
   /*! @brief Set the reference to the reference frame data. */
   virtual void initialize();

   /*! @brief Access function to set the HLA federation scenario time.
    *  @param new_time Associated federation scenario time. */
   virtual void set_time( double new_time )
   {
      packing_data.state.time = new_time;
   }

   /*! @brief Access function to get the HLA federation scenario time.
    *  @return Associated federation scenario time. */
   virtual double get_time()
   {
      return packing_data.state.time;
   }

   /*! @brief Packs the packing data object from the working data object(s),
    *  @details Called from the pack() function to pack the data from the working
    *  data objects(s) into the pe_packing_data object.  */
   virtual void pack_from_working_data();

   /*! @brief Unpacks the packing data object into the working data object(s),
    *  @details Called from the unpack() function to unpack the data in the
    *  pe_packing_data object into the working data object(s). */
   virtual void unpack_into_working_data();

   /*! @brief Set the references to time and the reference frame data.
    *  @param time_tt_in Pointer to the jeod::TimeTT instance.
    *  @param ref_frame_state_ptr Pointer to the RefFrameState instance. */
   void set_data( jeod::TimeTT        *time_tt_in,
                  jeod::RefFrameState *ref_frame_state_ptr );

  protected:
   jeod::TimeTT        *time_tt;         ///< trick_units{--}  JEOD TT time standard.
   jeod::RefFrameState *ref_frame_state; ///< @trick_units{--} JEOD reference frame state.

  private:
   // This object is not copyable
   /*! @brief Copy constructor for JEODRefFrameState class.
    *  @details This constructor is private to prevent inadvertent copies. */
   JEODRefFrameState( JEODRefFrameState const &rhs );
   /*! @brief Assignment operator for JEODRefFrameState class.
    *  @details This assignment operator is private to prevent inadvertent copies. */
   JEODRefFrameState &operator=( JEODRefFrameState const &rhs );
};

} // namespace SpaceFOM

#endif // SPACEFOM_JEOD_REF_FRAME_STATE_HH: Do NOT put anything after this line!
