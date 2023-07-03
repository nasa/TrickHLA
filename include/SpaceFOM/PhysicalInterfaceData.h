/**
@file SpaceFOM/PhysicalInterfaceData.h
@ingroup SpaceFOM
@brief A simple structure that contains the date fields required to encode
and decode a SISO Space Reference FOM PhysicalInterface data type.

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

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, July 2023, --, Initial version }
@revs_end

*/

#ifndef SPACEFOM_PYHSICAL_INTERFACE_DATA_H
#define SPACEFOM_PYHSICAL_INTERFACE_DATA_H

#include "SpaceFOM/QuaternionData.h"

#ifdef __cplusplus
// Place the struct in a namespace when C++.
namespace SpaceFOM
{
#endif

typedef struct {

   char   *name;            ///< @trick_units{--} Name of the physical interface.
   char   *parent_name;     ///< @trick_units{--} Name of the parent entity or interface.
   double  position[3];     ///< @trick_units{m}  Position of the interface in the parent struct frame.
   QuaternionData attitude; ///< @trick_units{--} Orientation of the interface wrt. the parent struct frame.

} PhysicalInterfaceData;

#ifdef __cplusplus
} // namespace SpaceFOM
#endif

#endif // SPACEFOM_PYHSICAL_INTERFACE_DATA_H: Do NOT put anything after this line!
