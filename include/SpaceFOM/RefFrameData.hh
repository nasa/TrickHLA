/**
@file SpaceFOM/RefFrameData.hh
@ingroup SpaceFOM
@brief A simple class that contains the date fields required to encode
and decode a SISO Space Reference FOM ReferenceFrame data type.

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
@trick_link_dependency{../../source/SpaceFOM/RefFrameData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, July 2018, --, Initial version }
@rev_entry{ Edwin Z. Crues, NASA ER7, TrickHLA, October 2023, --, Made into full class. }
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_DATA_HH
#define SPACEFOM_REF_FRAME_DATA_HH

#include "SpaceFOM/SpaceTimeCoordinateData.hh"

namespace SpaceFOM
{

class RefFrameData
{

  public:
   char *name;        ///< @trick_units{--} Name of the reference frame.
   char *parent_name; ///< @trick_units{--} Name of this frames parent frame.

   SpaceTimeCoordinateData state; ///< @trick_units{--} Space time coordinate state.

   double accel[3];     ///< @trick_units{m/s^2} Entity acceleration vector.
   double ang_accel[3]; ///< @trick_units{rad/s^2} Entity angular acceleration vector.

   // Default constructor.
   RefFrameData();

   // Copy constructor.
   /*! @brief Copy constructor for RefFrameData class.
    *  @param source Source data to copy from. */
   RefFrameData( const RefFrameData &source );

   // Destructor.
   /*! @brief Destructor for the SpaceFOM RefFrameData class. */
   virtual ~RefFrameData();

   /*! @brief Assignment operator for RefFrameData class.
    *  @param rhs Right operand data to copy from. */
   virtual RefFrameData &operator=( const RefFrameData &rhs );

   /*! @brief Copy the RefFrameData.
    *  @param source Source RefFrameData to copy from. */
   virtual void copy( const RefFrameData &source );

   /*! @brief Initialize the RefFrameData. */
   virtual void initialize();

   /*! @brief Set the time stamp associated with the RefFrameData.
    *  @param time Time stame value. */
   virtual void set_time( double time ){
      state.time = time;
   }


   /*! @brief Transform this RefFrameData into a specified parent frame.
    *  @return True on success, False on failure.
    *  @param transform_c_p Frame transformation between current parent (child) and the new parent frame.
    *  @param frame_p This frame state transformed into the new frame (parent). */
   bool transform_to_parent(
      RefFrameData const & transform_c_p,
      RefFrameData       * frame_p );

   /*! @brief Transform this RefFrameData into a specified child frame.
    *  @detail This routine is used to transform a state with respect to a child
    *  frame instead of a parent frame.  This is used when going down the tree.
    *  @return True on success, False on failure.
    *  @param transform_c_p Frame transformation between new parent (child) and the current parent frame.
    *  @param frame_c This frame state transformed into the new frame (child). */
   bool transform_to_child(
      RefFrameData const & transform_c_p,
      RefFrameData       * frame_c );

};

} // namespace SpaceFOM

#endif // SPACEFOM_REF_FRAME_DATA_HH: Do NOT put anything after this line!
