/**
@file SpaceFOM/RefFrameDataState.hh
@ingroup SpaceFOM
@brief A simple class that contains the date fields required to describe the
dynamic state of a SpaceFOM Reference Frame.

@copyright Copyright 2024 United States Government as represented by the
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
@trick_link_dependency{../../source/SpaceFOM/RefFrameDataState.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, June 2024, --, Initial version }
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_DATA_STATE_HH
#define SPACEFOM_REF_FRAME_DATA_STATE_HH

// System include files.
#include <iostream>
#include <string>

// SpaceFOM include files.
#include "SpaceFOM/SpaceTimeCoordinateData.hh"

namespace SpaceFOM
{

class RefFrameDataState
{

  public:
   SpaceTimeCoordinateData state; ///< @trick_units{--} Space time coordinate state.

   double accel[3];     ///< @trick_units{m/s^2} Entity acceleration vector.
   double ang_accel[3]; ///< @trick_units{rad/s^2} Entity angular acceleration vector.

   // Default constructor.
   RefFrameDataState();

   // Copy constructor.
   /*! @brief Copy constructor for RefFrameDataState class.
    *  @param source Source data to copy from. */
   RefFrameDataState( RefFrameDataState const &source );

   // Destructor.
   /*! @brief Destructor for the SpaceFOM RefFrameDataState class. */
   virtual ~RefFrameDataState();

   /*! @brief Assignment operator for RefFrameDataState class.
    *  @param rhs Right operand data to copy from. */
   virtual RefFrameDataState &operator=( RefFrameDataState const &rhs );

   /*! @brief Copy the RefFrameDataState.
    *  @param source Source RefFrameDataState to copy from. */
   virtual void copy( RefFrameDataState const &source );

   /*! @brief Initialize the RefFrameDataState. */
   virtual void initialize();

   /*! @brief Set the time stamp associated with the RefFrameDataState.
    *  @param time Time stame value. */
   virtual void set_time( double time )
   {
      state.time = time;
   }

   /*! @brief Compute a new version of this RefFrameDataState but with respect
    *  to the parent frame of a specified frame.
    *  @detail This routine is used to chain together frames when going up
    *  the tree.  The output frame is the same frame expressed with respect to
    *  the parent frame of the 'to' frame.
    *  @return True on success, False on failure.
    *  @param frame_to Frame transformation between current parent (child) and the new parent frame.
    *  @param frame_p This frame state transformed into the new frame (parent). */
   bool transform_to_parent(
      RefFrameDataState const &frame_to,
      RefFrameDataState       *frame_out );

   /*! @brief Compute a new version of this RefFrameDataState but with respect
    *  to the child frame of a specified frame.
    *  @detail This routine is used to chain together frames when going down
    *  the tree.  The output frame is the same frame expressed with respect to
    *  the child frame of the 'to' frame.
    *  @return True on success, False on failure.
    *  @param frame_to Frame transformation between new parent (child) and the current parent frame.
    *  @param frame_c This frame state transformed into the new frame (child). */
   bool transform_to_child(
      RefFrameDataState const &frame_to,
      RefFrameDataState       *frame_out );

   /*! @brief Print out the reference frame state values.
    *  @param stream Output stream. */
   virtual void print_data( std::ostream &stream = std::cout ) const;
};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_DATA_STATE_HH: Do NOT put anything after this line!
