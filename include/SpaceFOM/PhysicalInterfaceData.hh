/**
@file SpaceFOM/PhysicalInterfaceData.hh
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
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, October 2023, --, Made into full class.}
@revs_end

*/

#ifndef SPACEFOM_PHYSICAL_INTERFACE_DATA_HH
#define SPACEFOM_PHYSICAL_INTERFACE_DATA_HH

#include "SpaceFOM/QuaternionData.hh"

namespace SpaceFOM
{

class PhysicalInterfaceData
{

  public:
   char          *name;        ///< @trick_units{--} Name of the physical interface.
   char          *parent_name; ///< @trick_units{--} Name of the parent entity or interface.
   double         position[3]; ///< @trick_units{m}  Position of the interface in the parent structural frame.
   QuaternionData attitude;    ///< @trick_units{--} Orientation of the interface wrt. the parent structural frame.
};

} // namespace SpaceFOM

#endif // SPACEFOM_PHYSICAL_INTERFACE_DATA_HH: Do NOT put anything after this line!
