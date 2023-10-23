/**
@file SpaceFOM/SpaceTimeCoordinateData.h
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
@trick_link_dependency{QuaternionData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, Jan 2019, --, SpaceFOM support and testing. }
@revs_end

*/

#ifndef SPACEFOM_SPACE_TIME_COORDINATE_DATA_HH
#define SPACEFOM_SPACE_TIME_COORDINATE_DATA_HH

#include "SpaceFOM/QuaternionData.hh"

namespace SpaceFOM {

typedef struct {

   double         pos[3];     ///< trick_units{m}     Position in parent frame.
   double         vel[3];     ///< trick_units{m/s}   Velocity wrt. parent frame.
   QuaternionData quat;       ///< trick_units{--}    Attitude quaternion.
   double         ang_vel[3]; ///< trick_units{rad/s} Angular velocity vector.
   double         time;       ///< trick_units{s}     Truncated Julian date in TT time scale.

} SpaceTimeCoordinateData;

} // namespace SpaceFOMh

#endif // SPACEFOM_SPACE_TIME_COORDINATE_DATA_HH: Do NOT put anything after this line!
