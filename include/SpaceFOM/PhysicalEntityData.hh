/**
@file SpaceFOM/PhysicalEntityData.h
@ingroup SpaceFOM
@brief A simple structure that contains the date fields required to encode
and decode a SISO Space Reference FOM PhysicalEntity data type.

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

#ifndef SPACEFOM_PYHSICAL_ENTITY_DATA_HH
#define SPACEFOM_PYHSICAL_ENTITY_DATA_HH

#include "SpaceFOM/QuaternionData.hh"
#include "SpaceFOM/SpaceTimeCoordinateData.hh"

namespace SpaceFOM {

class PhysicalEntityData{

  public:
   char *name;         ///< @trick_units{--} Name of the physical entity.
   char *type;         ///< @trick_units{--} String use to define entity type.
   char *status;       ///< @trick_units{--} String use to define entity status.
   char *parent_frame; ///< @trick_units{--} Parent frame for state representation.

   SpaceTimeCoordinateData state; ///< @trick_units{--} Space time coordinate state.

   double accel[3];     ///< @trick_units{m/s2} Entity acceleration vector.
   double rot_accel[3]; ///< @trick_units{rad/s2} Entity rotational acceleration vector.
   double cm[3];        ///< @trick_units{m} Position of the entity center of mass in the structural frame.

   QuaternionData body_wrt_struct; ///< @trick_units{--} Orientation of the body frame wrt. the structural frame.

};

} // namespace SpaceFOM

#endif // SPACEFOM_PYHSICAL_ENTITY_DATA_HH: Do NOT put anything after this line!
