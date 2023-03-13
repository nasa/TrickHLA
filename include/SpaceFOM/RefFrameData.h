/**
@file SpaceFOM/RefFrameData.h
@ingroup SpaceFOM
@brief A simple structure that contains the date fields required to encode
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

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, July 2018, --, Initial version }
@revs_end

*/

#ifndef SPACEFOM_REF_FRAME_DATA_H
#define SPACEFOM_REF_FRAME_DATA_H

#include "SpaceFOM/SpaceTimeCoordinateData.h"

#ifdef __cplusplus
// Place the struct in a namespace when C++.
namespace SpaceFOM
{
#endif

typedef struct {

   char *name;        ///< @trick_units{--} Name of the reference frame.
   char *parent_name; ///< @trick_units{--} Name of this frames parent frame.

   SpaceTimeCoordinateData state; ///< @trick_units{--} Space time coordinate state.

} RefFrameData;

#ifdef __cplusplus
} // namespace SpaceFOM
#endif

#endif // SPACEFOM_REF_FRAME_DATA_H: Do NOT put anything after this line!
