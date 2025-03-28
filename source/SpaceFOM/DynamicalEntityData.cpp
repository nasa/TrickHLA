/*!
@file SpaceFOM/DynamicalEntityData.cpp
@ingroup SpaceFOM
@brief A simple structure that contains the date fields required to encode
and decode a SISO Space Reference FOM DynamicalEntity data type.  Note: this
does not include the PhysicalEntity data.

@copyright Copyright 2025 United States Government as represented by the
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
@trick_link_dependency{DynamicalEntityData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, February 2025, --, Initial version }
@revs_end

*/

// C includes.
#include <string.h>

// Trick includes.
#include "trick/MemoryManager.hh"
#include "trick/exec_proto.hh"
#include "trick/message_proto.h"

// Trick HLA includes.
#include "TrickHLA/CompileConfig.hh"

// SpaceFOM includes.
#include "SpaceFOM/DynamicalEntityData.hh"

using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
DynamicalEntityData::DynamicalEntityData()
{
   mass      = 1.0;
   mass_rate = 0.0;
   for ( unsigned int iinc = 0; iinc < 3; iinc++ ) {
      this->force[iinc]  = 0.0;
      this->torque[iinc] = 0.0;
      for ( unsigned int jinc = 0; jinc < 3; jinc++ ) {
         this->inertia[iinc][jinc]      = 0.0;
         this->inertia_rate[iinc][jinc] = 0.0;
      }
      this->inertia[iinc][iinc] = 1.0;
   }
}

/*!
 * @job_class{initialization}
 */
DynamicalEntityData::DynamicalEntityData( DynamicalEntityData const &source )
{
   this->copy( source );
}

/*!
 * @job_class{shutdown}
 */
DynamicalEntityData::~DynamicalEntityData()
{
   return;
}

/***********************************************************************
 * DynamicalEntityData methods.
 ***********************************************************************/

/*!
 * @job_class{scheduled}
 */
DynamicalEntityData &DynamicalEntityData::operator=(
   DynamicalEntityData const &rhs )
{
   copy( rhs );

   return ( *this );
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntityData::copy( DynamicalEntityData const &source )
{
   // Copy the mass parameters.
   this->mass      = source.mass;
   this->mass_rate = source.mass_rate;

   // Copy the force and torque vectors and inertia matrices.
   for ( int iinc = 0; iinc < 3; iinc++ ) {
      this->force[iinc]  = source.force[iinc];
      this->torque[iinc] = source.torque[iinc];
      for ( unsigned int jinc = 0; jinc < 3; jinc++ ) {
         this->inertia[iinc][jinc]      = source.inertia[iinc][jinc];
         this->inertia_rate[iinc][jinc] = source.inertia_rate[iinc][jinc];
      }
   }

   return;
}

/*!
 * @job_class{scheduled}
 */
void DynamicalEntityData::print_data( std::ostream &stream ) const
{

   // Set the print precision.
   stream.precision( 15 );

   stream << "\tmass: '" << mass << "'\n"
          << "\tmass_rate: '" << mass_rate << "'\n";
   stream << "\tforce: "
          << "\t\t" << force[0] << ", "
          << "\t\t" << force[1] << ", "
          << "\t\t" << force[2] << '\n';
   stream << "\ttorque: "
          << "\t\t" << torque[0] << ", "
          << "\t\t" << torque[1] << ", "
          << "\t\t" << torque[2] << '\n';
   stream << "\tinertia: \n"
          << "\t\t" << inertia[0][0] << ", " << inertia[1][0] << ", " << inertia[2][0] << '\n'
          << "\t\t" << inertia[0][0] << ", " << inertia[1][0] << ", " << inertia[2][0] << '\n'
          << "\t\t" << inertia[0][0] << ", " << inertia[1][0] << ", " << inertia[2][0] << '\n';
   stream << "\tinertia rate: \n"
          << "\t\t" << inertia_rate[0][0] << ", " << inertia_rate[1][0] << ", " << inertia_rate[2][0] << '\n'
          << "\t\t" << inertia_rate[0][0] << ", " << inertia_rate[1][0] << ", " << inertia_rate[2][0] << '\n'
          << "\t\t" << inertia_rate[0][0] << ", " << inertia_rate[1][0] << ", " << inertia_rate[2][0] << '\n';

   return;
}
