/*!
@file SpaceFOM/DynamicalEntity.hh
@ingroup SpaceFOM
@brief Definition of the TrickHLA SpaceFOM Dynamical entity type.

This is the base implementation for the Space Reference FOM (SpaceFOM) interface
to the Reference Frame object. This needs to be available to the SpaceFOM
initialization process for the root reference frame discovery step in the
initialization process.

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
@trick_link_dependency{../../source/SpaceFOM/DynamicalEntity.cpp}
@trick_link_dependency{../../source/SpaceFOM/PhysicalEntityBase.cpp}

@revs_title
@revs_begin
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, March 2019, --, Version 3 rewrite.}
@revs_end

*/

#ifndef SPACEFOM_DYNAMICAL_ENTITY_HH
#define SPACEFOM_DYNAMICAL_ENTITY_HH

// SpaceFOM include files.
#include "SpaceFOM/PhysicalEntityBase.hh"

namespace SpaceFOM
{

class DynamicalEntity : public SpaceFOM::PhysicalEntityBase
{
   // Let the Trick input processor access protected and private data.
   // InputProcessor is really just a marker class (does not really
   // exist - at least yet). This friend statement just tells Trick
   // to go ahead and process the protected and private data as well
   // as the usual public data.
   friend class InputProcessor;
   // IMPORTANT Note: you must have the following line too.
   // Syntax: friend void init_attr<namespace>__<class name>();
   friend void init_attrSpaceFOM__DynamicalEntity();

  public:
   // Public constructors and destructors.
   DynamicalEntity();          // Default constructor.
   virtual ~DynamicalEntity(); // Destructor.

   // Data pack and unpack routines.
   virtual void pack();   // cppcheck-suppress [uselessOverride]
   virtual void unpack(); // cppcheck-suppress [uselessOverride]

  protected:
   double force[3];           ///< @trick_units{N} Total external force on vehicle applied
                              ///       through the vehicle center of mass.
                              ///       Expressed in the vehicle struct frame.
   double torque[3];          ///< @trick_units{N*m} Total external torques on vehicle.
                              ///       Expressed in the vehicle struct frame.
   double mass;               ///< @trick_units{kg} Vehicle mass.
   double mass_rate;          /// trick_units{kg/s} Vehicle mass flow rate.
   double inertia[3][3];      /// trick_units{kg*m2} Inertia matrix in element body frame.
   double inertia_rate[3][3]; /// @trick_units{kg*m2/s} Inertia matrix in element body frame.

  private:
   // This object is not copyable
   DynamicalEntity( DynamicalEntity const & );
   DynamicalEntity &operator=( DynamicalEntity const & );
};

} // namespace SpaceFOM

#endif // SPACEFOM_DYNAMICAL_ENTITY_HH: Do NOT put anything after this line!
