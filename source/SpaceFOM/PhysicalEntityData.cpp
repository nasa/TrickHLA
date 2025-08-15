/*!
@file SpaceFOM/PhysicalEntityData.cpp
@ingroup SpaceFOM
@brief A simple structure that contains the date fields required to encode
and decode a SISO Space Reference FOM PhysicalEntity data type.

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
@trick_link_dependency{PhysicalEntityData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, April 2024, --, Initial version }
@revs_end

*/

// System includes.
#include <cstddef>
#include <ostream>

// SpaceFOM includes.
#include "SpaceFOM/PhysicalEntityData.hh"

using namespace std : using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
PhysicalEntityData::PhysicalEntityData()
   : name(),
     type(),
     status(),
     parent_frame(),
     state()
{
   for ( unsigned int iinc = 0; iinc < 3; iinc++ ) {
      this->accel[iinc]     = 0.0;
      this->ang_accel[iinc] = 0.0;
      this->cm[iinc]        = 0.0;
   }
}

/*!
 * @job_class{initialization}
 */
PhysicalEntityData::PhysicalEntityData( PhysicalEntityData const &source )
{
   this->copy( source ); // NOLINT
}

/*!
 * @job_class{shutdown}
 */
PhysicalEntityData::~PhysicalEntityData()
{
   return;
}

/***********************************************************************
 * PhysicalEntityData methods.
 ***********************************************************************/

/*!
 * @job_class{scheduled}
 */
PhysicalEntityData &PhysicalEntityData::operator=(
   PhysicalEntityData const &rhs )
{
   copy( rhs );

   return ( *this );
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntityData::copy( PhysicalEntityData const &source )
{
   // Copy the string based parameters.
   this->name         = source.name;
   this->type         = source.type;
   this->status       = source.status;
   this->parent_frame = source.parent_frame;

   // Copy the state.
   this->state = source.state;

   // Copy the accelerations and CM vectors.
   for ( int iinc = 0; iinc < 3; iinc++ ) {
      this->accel[iinc]     = source.accel[iinc];
      this->ang_accel[iinc] = source.ang_accel[iinc];
      this->cm[iinc]        = source.cm[iinc];
   }

   // Copy the body attitude quaternion.
   this->body_wrt_struct = source.body_wrt_struct;

   return;
}

/*!
 * @job_class{scheduled}
 */
void PhysicalEntityData::print_data( std::ostream &stream ) const
{

   // Set the print precision.
   stream.precision( 15 );

   stream << "\tname:         '" << name << "'" << endl
          << "\ttype:         '" << type << "'" << endl
          << "\tstatus:       '" << status << "'" << endl
          << "\tparent_frame: '" << parent_frame << "'" << endl;

   state.print_data( stream );

   stream << "\tacceleration: "
          << "\t\t" << accel[0] << ", "
          << "\t\t" << accel[1] << ", "
          << "\t\t" << accel[2] << endl;

   stream << "\tangular acceleration: "
          << "\t\t" << ang_accel[0] << ", "
          << "\t\t" << ang_accel[1] << ", "
          << "\t\t" << ang_accel[2] << endl;

   stream << "\tcenter of mass (cm): "
          << "\t\t" << cm[0] << ", "
          << "\t\t" << cm[1] << ", "
          << "\t\t" << cm[2] << endl;

   stream << "\tBody frame orientation:\n";
   body_wrt_struct.print_data( stream );

   return;
}
