/*!
@file SpaceFOM/SpaceTimeCoordinateData.c
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
   : quat(),
     time( 0.0 )

{
   V_INIT( pos );
   V_INIT( vel );
   V_INIT( ang_vel );
}


/*!
 * @job_class{initialization}
 */
SpaceTimeCoordinateData::SpaceTimeCoordinateData(
   const SpaceTimeCoordinateData & source )
   : quat( source.quat ),
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
   quat.initialize();
   V_INIT( ang_vel );
   time = 0.0;
   return;
}

/*!
 * @job_class{scheduled}
 */
void SpaceTimeCoordinateData::copy( const SpaceTimeCoordinateData & source )
{
   V_COPY( this->pos, source.pos );
   V_COPY( this->vel, source.vel );
   quat.copy( source.quat );
   V_COPY( this->ang_vel, source.ang_vel );
   time = source.time;
   return;
}


/*!
 * @job_class{scheduled}
 */
SpaceTimeCoordinateData & SpaceTimeCoordinateData::operator=(
   const SpaceTimeCoordinateData & rhs )
{
   this->copy( rhs );
   return( *this );
}

