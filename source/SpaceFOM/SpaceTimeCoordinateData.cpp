/*!
@file SpaceFOM/SpaceTimeCoordinateData.cpp
@ingroup SpaceFOM
@brief Simple functions that operate on space-time coordinate data.

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

@tldh
@trick_link_dependency{SpaceTimeCoordinateData.cpp}
@trick_link_dependency{QuaternionData.cpp}

@revs_begin
@rev_entry{ Edwin Z. Crues, NASA ER7, NExSyS, October 2023, --, Initial version }
@rev_entry{Edwin Z. Crues, NASA ER7, TrickHLA, October 2023, --, Made into full class.}
@revs_end

*/

// C includes.
#include <math.h>

// Trick includes.
#include "trick/vector_macros.h"

// SpaceFOM includes.
#include "SpaceFOM/SpaceTimeCoordinateData.hh"

using namespace SpaceFOM;

/*!
 * @job_class{initialization}
 */
SpaceTimeCoordinateData::SpaceTimeCoordinateData()
   : att(),
     time( 0.0 )
{
   initialize();
}

/*!
 * @job_class{initialization}
 */
SpaceTimeCoordinateData::SpaceTimeCoordinateData(
   SpaceTimeCoordinateData const &source )
   : att( source.att ),
     time( source.time )
{
   V_COPY( this->pos, source.pos );
   V_COPY( this->vel, source.vel );
   V_COPY( this->ang_vel, source.ang_vel );
}

/*!
 * @job_class{scheduled}
 */
void SpaceTimeCoordinateData::initialize()
{
   V_INIT( pos );
   V_INIT( vel );
   att.initialize();
   V_INIT( ang_vel );
   time = 0.0;
   return;
}

/*!
 * @job_class{scheduled}
 */
void SpaceTimeCoordinateData::copy( SpaceTimeCoordinateData const &stc_data )
{
   V_COPY( this->pos, stc_data.pos );
   V_COPY( this->vel, stc_data.vel );
   att.copy( stc_data.att );
   V_COPY( this->ang_vel, stc_data.ang_vel );
   time = stc_data.time;
   return;
}

/*!
 * @job_class{scheduled}
 */
SpaceTimeCoordinateData &SpaceTimeCoordinateData::operator=(
   SpaceTimeCoordinateData const &rhs )
{
   copy( rhs );
   return ( *this );
}

/*!
 * @job_class{scheduled}
 */
bool SpaceTimeCoordinateData::operator==(
   SpaceTimeCoordinateData const &rhs )
{
   return ( is_equal( rhs ) );
}

/*!
 * @job_class{scheduled}
 */
bool SpaceTimeCoordinateData::operator!=(
   SpaceTimeCoordinateData const &rhs )
{
   return ( !( is_equal( rhs ) ) );
}

/*!
 * @job_class{scheduled}
 */
bool SpaceTimeCoordinateData::is_equal(
   SpaceTimeCoordinateData const &rhs )
{
   // Compare position
   if ( ( this->pos[0] != rhs.pos[0] )
        || ( this->pos[1] != rhs.pos[1] )
        || ( this->pos[2] != rhs.pos[2] ) ) {
      return ( false );
   }

   // Compare velocity
   if ( ( this->vel[0] != rhs.vel[0] )
        || ( this->vel[1] != rhs.vel[1] )
        || ( this->vel[2] != rhs.vel[2] ) ) {
      return ( false );
   }

   // Compare attitude
   if ( !( att.is_equal( rhs.att ) ) ) {
      return ( false );
   }

   // Compare angular velocity
   if ( ( this->ang_vel[0] != rhs.ang_vel[0] )
        || ( this->ang_vel[1] != rhs.ang_vel[1] )
        || ( this->ang_vel[2] != rhs.ang_vel[2] ) ) {
      return ( false );
   }

   // Compare time
   if ( this->time != rhs.time ) {
      return ( false );
   }

   return ( true );
}

/*!
 * @job_class{scheduled}
 */
void SpaceTimeCoordinateData::print_data( std::ostream &stream ) const
{
   // Set the print precision.
   stream.precision( 15 );

   stream << "\ttime: " << time << '\n';
   stream << "\tposition: "
          << "\t\t" << pos[0] << ", "
          << "\t\t" << pos[1] << ", "
          << "\t\t" << pos[2] << '\n';
   stream << "\tvelocity: "
          << "\t\t" << vel[0] << ", "
          << "\t\t" << vel[1] << ", "
          << "\t\t" << vel[2] << '\n';
   att.print_data( stream );
   stream << "\tangular velocity: "
          << "\t\t" << ang_vel[0] << ", "
          << "\t\t" << ang_vel[1] << ", "
          << "\t\t" << ang_vel[2] << '\n';

   return;
}
