/**
@file SpaceFOM/SpaceTimeCoordinateData.hh
@ingroup SpaceFOM
@brief A simple structure that contains the date fields required to encode
and decode a SISO Space Reference FOM Space/Time Coordinate data type.

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
@trick_link_dependency{../../source/SpaceFOM/SpaceTimeCoordinateData.cpp}
@trick_link_dependency{../../source/SpaceFOM/QuaternionData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, Jan 2019, --, SpaceFOM support and testing. }
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, October 2023, --, Made into full class.}
@revs_end

*/

#ifndef SPACEFOM_SPACE_TIME_COORDINATE_DATA_HH
#define SPACEFOM_SPACE_TIME_COORDINATE_DATA_HH

// System include files.
#include <iostream>

// Trick includes.
#include "trick/vector_macros.h"

// SpaceFOM includes.
#include "SpaceFOM/QuaternionData.hh"

namespace SpaceFOM
{

class SpaceTimeCoordinateData
{

  public:
   double         pos[3];     ///< trick_units{m}     Position in parent frame.
   double         vel[3];     ///< trick_units{m/s}   Velocity wrt. parent frame.
   QuaternionData att;        ///< trick_units{--}    Attitude quaternion.
   double         ang_vel[3]; ///< trick_units{rad/s} Angular velocity vector.
   double         time;       ///< trick_units{s}     Truncated Julian date in TT time scale.

   // Default constructor.
   SpaceTimeCoordinateData();

   // Copy constructor.
   /*! @brief Copy constructor for SpaceTimeCoordinateData class.
    *  @param source Source data to copy from. */
   SpaceTimeCoordinateData( SpaceTimeCoordinateData const &source );

   /*! @brief Assignment operator for SpaceTimeCoordinateData class.
    *  @param rhs Right had side data to copy from. */
   SpaceTimeCoordinateData &operator=( SpaceTimeCoordinateData const &rhs );

   /*! @brief Equal comparison operator for SpaceTimeCoordinateData class.
    *  @param rhs Right operand data to compare to. */
   bool operator==( SpaceTimeCoordinateData const &rhs );

   /*! @brief Not equal comparison operator for SpaceTimeCoordinateData class.
    *  @param rhs Right operand data to compare to. */
   bool operator!=( SpaceTimeCoordinateData const &rhs );

   /*! @brief Initialize the state-time coordinate data. */
   void initialize();

   /*! @brief Compare a SpaceTimeCoordinateData instance to another.
    *  @return True if instances are the exactly the same, false otherwise.
    *  @param  rhs SpaceTimeCoordinateData to compare against. */
   bool is_equal( SpaceTimeCoordinateData const &rhs );

   /*! @brief Copy the state-time coordinate data.
    *  @param stc_data Source to copy from. */
   void copy( SpaceTimeCoordinateData const &stc_data );

   /*! @brief Print out the data values.
    *  @param stream Output stream. */
   void print_data( std::ostream &stream = std::cout ) const;
};

} // namespace SpaceFOM

#endif // SPACEFOM_SPACE_TIME_COORDINATE_DATA_HH: Do NOT put anything after this line!
